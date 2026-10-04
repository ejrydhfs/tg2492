/*
  GPL LICENSE SUMMARY

  Copyright(c) 2014 Intel Corporation.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
  The full GNU General Public License is included in this distribution
  in the file called LICENSE.GPL.

  Contact Information:
    Intel Corporation
    2200 Mission College Blvd.
    Santa Clara, CA  97052
*/


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>       /* everything... */
#include <linux/types.h>    /* size_t */
#include <linux/uaccess.h>
#include <linux/cdev.h>     /* cdev utilities */
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include "pdsp.h"
#include "ramtest.h"
#include <mach/puma.h>
#include <avalanche_pdsp_api.h>


#if (0)
#define PDSP_DBG(fmt,args...) printk(fmt , ##args)
#else
#define PDSP_DBG(x,...)
#endif


#if (1)
    #define PDSP_LOCK(sem)      __pdsp_lock()
    #define PDSP_UNLOCK(sem)    __pdsp_unlock()
#else
    #define PDSP_LOCK(sem)      down(sem)
    #define PDSP_UNLOCK(sem)    up(sem)
#endif 


/*
 * PDSP Registers structure.
 *  The structure instance variable points to PDSP register space directly.
 */
typedef volatile struct
{
    Uint32    control;             /* 0x00 */
    Uint32    status;              /* 0x04 */
    Uint32    wakeup_en;           /* 0x08 */
    Uint32    cycle_count;         /* 0x0C */
    Uint32    stall_count;         /* 0x10 */
    Uint32    pad0[3];
    Uint32    table_block_index0;  /* 0x20 */
    Uint32    table_block_index1;  /* 0x24 */
    Uint32    table_program_ptr0;  /* 0x28 */
    Uint32    table_program_ptr1;  /* 0x2C */

} pdsp_regs_t;

#define PDSP_CTL_BIG_EN_BIT         (1 << 14)
#define PDSP_CTL_EN_BIT             (1 <<  1)
#define PDSP_CTL_N_RST_BIT          (1 <<  0)
#define PDSP_REG_RUN_STATE_BIT      (1<<15)
#define PDSP_REG_SLEEP_BIT          (1<<2)
#define PDSP_REG_COUNT_ENABLE_BIT   (1<<3)
#define PDSP_REG_SINGLE_STEP_BIT    (1<<8)


/*
 * PDSP register space overlay pointer.
 *
 */
typedef pdsp_regs_t* PDSP_RegsOvly;

typedef Int32 (*pdsp_trg_cmd_func_t)(struct pdsp_info *, pdsp_cmd_t);
typedef Int32 (*pdsp_rsp_sts_func_t)(struct pdsp_info *);

typedef volatile struct pdsp_info
{
    pdsp_id_t               id;
    struct semaphore *      sem_ptr;
    Uint32 *                cmd_rsp_ram;
    Uint32                  cmd_id_mask;
    Uint32                  cmd_status_mask;
    Uint32 *                param_ram;
    Uint32                  wait_count;
    Uint32 *                isr_reg;
    Uint32                  isr_mask;

    pdsp_trg_cmd_func_t     trg_cmd;
    pdsp_rsp_sts_func_t     rsp_sts;

    PDSP_RegsOvly           regs;
    Uint32                  iram_size;
    Uint32 *                iram;
    Uint32 *                dbg_regs;

}
pdsp_info_t;


typedef volatile struct sram_info
{
    Uint32 *                sram;
    Uint32                  sram_size;
}
scr_ram_info_t;

static struct semaphore *   __pdsp_download_sem;
static Uint8 *              __pdsp_download_current_addr = NULL;
static Int32    __pdsp_trigger_cmd   (pdsp_info_t *pdsp, pdsp_cmd_t cmd);
static Int32    __pdsp_rsp_status    (pdsp_info_t *pdsp);

#if defined (CONFIG_MACH_PUMA6)
#define MAX_BREAKPOINTS         10
#define MAX_SINGLE_STEP_CMDS    100
#define NUM_OF_DEBUG_REGS       32
#define HALT_OP_CODE            0x2A000000

static Uint32 debugRegs[NUM_OF_DEBUG_REGS];

#define PRINT_PDSP_ID(pdsp_id) (((pdsp_id) >= (sizeof(pdsp_id_to_string)/sizeof(char*)))     ? "INVALID_PDSP_ID"  : pdsp_id_to_string[(pdsp_id)])

typedef struct bp_info_s
{
    Uint32                  opCode;
    Uint32                  commandNum;
    pdsp_id_t               pdsp_id;
}bp_info_t;

typedef struct pdsp_breakpoint_info_s
{
    Uint32                  numOfBreakPoints;
    bp_info_t               breakpoints[MAX_BREAKPOINTS];
}pdsp_breakpoint_info_t;

static pdsp_breakpoint_info_t pdsp_breakpoint_info;

void halt_or_resume_pdsp(pdsp_ctrl_op_t operation, pdsp_id_t  pdsp_id);
void print_debug_regs(pdsp_cmd_params_t   *usr_params);
Uint32 pdspHasBreakPointOn(pdsp_cmd_params_t   *bp_params);
void singleStep_enable(pdsp_cmd_params_t   *usr_params);
void singleStep_disable(pdsp_cmd_params_t   *usr_params);
void breakpoint_enable(pdsp_cmd_params_t   *usr_params);
void breakpoint_disable(pdsp_cmd_params_t   *usr_params);
void breakpoint_print(pdsp_cmd_params_t   *usr_params);
void delete_all_breakpoints(void);
void run_to_opCode(pdsp_cmd_params_t   *usr_params);
void show_status(pdsp_cmd_params_t   *usr_params);
void modify_dbg_regs(pdsp_cmd_params_t   *usr_params);
#endif

static scr_ram_info_t g_ram_info[] =
{
    { .sram = (Uint32 *)    IO_PHY2VIRT(0x03200000),   .sram_size = 0x0000A000 },
    { .sram = (Uint32 *)    IO_PHY2VIRT(0x03300000),   .sram_size = 0x0002D000 },
    { .sram = (Uint32 *)    IO_PHY2VIRT(0x03400000),   .sram_size = 0x0000E000 }
};

#define SIZE_4K     0x1000
#define SIZE_8K     0x2000

#if defined (CONFIG_MACH_PUMA6)
static char* pdsp_id_to_string[] =
{
    "Prefetcher",   //  _PPDSP     
    "Classifier",   //  _CPDSP1
    "Classifier2",  //  _CPDSP2 only in P6 
    "Modifier",     //  _MPDSP     
    "QoS",          //  _QPDSP  
    "LAN_Proxy",    //  _PrxPDSP only in P6 
};
#endif

#if defined (CONFIG_MACH_PUMA6)
static pdsp_info_t g_pdsp_info[] =
{
    {   /* PPDSP */ 
	    /* This is actually UsPrefPDSP but called PPDSP in code prespective */
        .id             = PDSP_ID_Prefetcher                        ,
        .regs           = (PDSP_RegsOvly)   AVALANCHE_NWSS_PPDSP_CTRL_RGN_BASE ,
        .iram           = (Uint32 *)        AVALANCHE_NWSS_PPDSP_IRAM_RGN_BASE ,
        .dbg_regs       = (Uint32 *)        AVALANCHE_NWSS_PPDSP_DBG_RGN_BASE ,
        .iram_size      = SIZE_4K                                   ,
        .cmd_status_mask= 0xFF000000                                ,
        .cmd_id_mask    = 0x0000FF00                                ,
        .cmd_rsp_ram    = (Uint32 *)        IO_PHY2VIRT(0x03400200) ,
        .param_ram      = (Uint32 *)        IO_PHY2VIRT(0x03400204) ,
        .wait_count     = 10000000                                  ,
        .trg_cmd        = (pdsp_trg_cmd_func_t)   __pdsp_trigger_cmd,
        .rsp_sts        = (pdsp_rsp_sts_func_t)   __pdsp_rsp_status ,
        .isr_reg        = NULL                                      ,
        .isr_mask       = 0                                         ,
    },
    {   /* CPDSP */
        .id             = PDSP_ID_Classifier                        ,
        .regs           = (PDSP_RegsOvly)   AVALANCHE_NWSS_CPDSP_CTRL_RGN_BASE ,
        .iram           = (Uint32 *)        AVALANCHE_NWSS_CPDSP_IRAM_RGN_BASE ,
        .dbg_regs       = (Uint32 *)        AVALANCHE_NWSS_CPDSP_DBG_RGN_BASE ,
        .iram_size      = SIZE_4K                                   ,
        .cmd_id_mask    = 0x000000FF                                ,
        .cmd_rsp_ram    = (Uint32 *)        IO_PHY2VIRT(0x03400000) ,
        .param_ram      = (Uint32 *)        IO_PHY2VIRT(0x03400010) ,
        .wait_count     = 10000000                                  ,
        .trg_cmd        = (pdsp_trg_cmd_func_t)   __pdsp_trigger_cmd,
        .rsp_sts        = (pdsp_rsp_sts_func_t)   __pdsp_rsp_status ,
        .isr_reg        = (Uint32 *)        AVALANCHE_NWSS_GENERAL_MAILBOX_STAT_REG ,
        .isr_mask       = 0x00000001                                ,
    },
    {   /* CPDSP2 */
        .id             = PDSP_ID_Classifier2                       ,
        .regs           = (PDSP_RegsOvly)   AVALANCHE_NWSS_CPDSP2_CTRL_RGN_BASE ,
        .iram           = (Uint32 *)        AVALANCHE_NWSS_CPDSP2_IRAM_RGN_BASE ,
        .dbg_regs       = (Uint32 *)        AVALANCHE_NWSS_CPDSP2_DBG_RGN_BASE ,
        .iram_size      = SIZE_4K                                   ,
        .cmd_id_mask    = 0x000000FF                                ,
        .cmd_rsp_ram    = (Uint32 *)        IO_PHY2VIRT(0x03400004) ,
        .param_ram      = (Uint32 *)        IO_PHY2VIRT(0x03400010) ,
        .wait_count     = 10000000                                  ,
        .trg_cmd        = (pdsp_trg_cmd_func_t)   __pdsp_trigger_cmd,
        .rsp_sts        = (pdsp_rsp_sts_func_t)   __pdsp_rsp_status ,
        .isr_reg        = (Uint32 *)        AVALANCHE_NWSS_GENERAL_MAILBOX_STAT_REG ,
        .isr_mask       = 0x00000002                                ,
    },
    {   /* MPDSP */
        .id             = PDSP_ID_Modifier                          ,
        .regs           = (PDSP_RegsOvly)   AVALANCHE_NWSS_MPDSP_CTRL_RGN_BASE ,
        .iram           = (Uint32 *)        AVALANCHE_NWSS_MPDSP_IRAM_RGN_BASE ,
        .dbg_regs       = (Uint32 *)        AVALANCHE_NWSS_MPDSP_DBG_RGN_BASE ,
        .iram_size      = SIZE_8K                                   ,
        .cmd_id_mask    = 0x000000FF                                ,
        .cmd_rsp_ram    = (Uint32 *)        IO_PHY2VIRT(0x03400008) ,
        .param_ram      = (Uint32 *)        IO_PHY2VIRT(0x03400010) ,
        .wait_count     = 10000000                                  ,
        .trg_cmd        = (pdsp_trg_cmd_func_t)   __pdsp_trigger_cmd,
        .rsp_sts        = (pdsp_rsp_sts_func_t)   __pdsp_rsp_status ,
        .isr_reg        = (Uint32 *)        AVALANCHE_NWSS_GENERAL_MAILBOX_STAT_REG ,
        .isr_mask       = 0x00000004                                ,
    },
    {   /* QPDSP */
        .id             = PDSP_ID_QoS                               ,
        .regs           = (PDSP_RegsOvly)   AVALANCHE_NWSS_QPDSP_CTRL_RGN_BASE ,
        .iram           = (Uint32 *)        AVALANCHE_NWSS_QPDSP_IRAM_RGN_BASE ,
        .dbg_regs       = (Uint32 *)        AVALANCHE_NWSS_QPDSP_DBG_RGN_BASE ,
        .iram_size      = SIZE_4K                                   ,
        .cmd_id_mask    = 0x000000FF                                ,
        .cmd_rsp_ram    = (Uint32 *)        IO_PHY2VIRT(0x0340000C) ,
        .param_ram      = (Uint32 *)        IO_PHY2VIRT(0x03400010) ,
        .wait_count     = 10000000                                  ,
        .trg_cmd        = (pdsp_trg_cmd_func_t)   __pdsp_trigger_cmd,
        .rsp_sts        = (pdsp_rsp_sts_func_t)   __pdsp_rsp_status ,
        .isr_reg        = (Uint32 *)        AVALANCHE_NWSS_GENERAL_MAILBOX_STAT_REG ,
        .isr_mask       = 0x00000008                                ,
    },
    {   /* PrxPDSP */
        .id             = PDSP_ID_LAN_Proxy                         ,
        .regs           = (PDSP_RegsOvly)   AVALANCHE_NWSS_LAN_PrxyPDSP_CTRL_RGN_BASE ,
        .iram           = (Uint32 *)        AVALANCHE_NWSS_LAN_PrxyPDSP_IRAM_RGN_BASE ,
        .dbg_regs       = (Uint32 *)        AVALANCHE_NWSS_LAN_PrxyPDSP_DBG_RGN_BASE ,
        .iram_size      = SIZE_4K                                   ,
        .cmd_id_mask    = 0x000000FF                                ,
        .cmd_rsp_ram    = (Uint32 *)        IO_PHY2VIRT(0x03400180) ,
        .param_ram      = (Uint32 *)        IO_PHY2VIRT(0x03400184) ,
        .wait_count     = 10000000                              ,
        .trg_cmd        = (pdsp_trg_cmd_func_t)   __pdsp_trigger_cmd,
        .rsp_sts        = (pdsp_rsp_sts_func_t)   __pdsp_rsp_status ,
        .isr_reg        = NULL                                      ,
        .isr_mask       = 0                                         ,
    },
    {   /* CoePDSP */
        .id             = PDSP_ID_CoE                                ,
        .regs           = (PDSP_RegsOvly)   AVALANCHE_NWSS_COE_PDSP_CTRL_RGN_BASE ,
        .iram           = (Uint32 *)        AVALANCHE_NWSS_COE_PDSP_IRAM_RGN_BASE ,
        .dbg_regs       = (Uint32 *)        AVALANCHE_NWSS_COE_PDSP_DBG_RGN_BASE ,
        .iram_size      = SIZE_4K                                   ,
        .cmd_id_mask    = 0x000000FF                                ,
        .cmd_rsp_ram    = (Uint32 *)        IO_PHY2VIRT(0x03400280) ,
        .param_ram      = (Uint32 *)        IO_PHY2VIRT(0x03400284) ,
        .wait_count     = 10000000                                  ,
        .trg_cmd        = (pdsp_trg_cmd_func_t)   __pdsp_trigger_cmd,
        .rsp_sts        = (pdsp_rsp_sts_func_t)   __pdsp_rsp_status ,
        .isr_reg        = NULL                                      ,
        .isr_mask       = 0                                         ,
    },
};

#else

static pdsp_info_t g_pdsp_info[] =
{
    {   /* PPDSP */
        .id             = PDSP_ID_Prefetcher                        ,
        .regs           = (PDSP_RegsOvly)   AVALANCHE_NWSS_PPDSP_CTRL_RGN_BASE ,
        .iram           = (Uint32 *)        AVALANCHE_NWSS_PPDSP_IRAM_RGN_BASE ,
        .iram_size      = SIZE_4K                                   ,
        .cmd_status_mask= 0xFF000000                                ,
        .cmd_id_mask    = 0x0000FF00                                ,
        .cmd_rsp_ram    = (Uint32 *)        IO_PHY2VIRT(0x03167C00) ,
        .param_ram      = (Uint32 *)        IO_PHY2VIRT(0x03167C04) ,
        .wait_count     = 10000000                                  ,
        .trg_cmd        = (pdsp_trg_cmd_func_t)   __pdsp_trigger_cmd,
        .rsp_sts        = (pdsp_rsp_sts_func_t)   __pdsp_rsp_status 
        
    },
    {   /* CPDSP */
        .id             = PDSP_ID_Classifier                        ,
        .regs           = (PDSP_RegsOvly)   AVALANCHE_NWSS_CPDSP_CTRL_RGN_BASE ,
        .iram           = (Uint32 *)        AVALANCHE_NWSS_CPDSP_IRAM_RGN_BASE ,
        .iram_size      = SIZE_4K                                   ,
        .cmd_id_mask    = 0x000000FF                                ,
        .cmd_rsp_ram    = (Uint32 *)        IO_PHY2VIRT(0x03100000) , 
        .param_ram      = (Uint32 *)        IO_PHY2VIRT(0x03100004) , 
        .wait_count     = 10000000                                  ,
        .trg_cmd        = (pdsp_trg_cmd_func_t)   __pdsp_trigger_cmd,
        .rsp_sts        = (pdsp_rsp_sts_func_t)   __pdsp_rsp_status 
        
    },
    {   /* MPDSP */
        .id             = PDSP_ID_Modifier                          ,
        .regs           = (PDSP_RegsOvly)   AVALANCHE_NWSS_MPDSP_CTRL_RGN_BASE ,
        .iram           = (Uint32 *)        AVALANCHE_NWSS_MPDSP_IRAM_RGN_BASE ,
        .iram_size      = SIZE_4K                                   ,
        .cmd_id_mask    = 0x000000FF                                ,
        .cmd_rsp_ram    = (Uint32 *)        IO_PHY2VIRT(0x03110000) ,
        .param_ram      = (Uint32 *)        IO_PHY2VIRT(0x03110004) ,
        .wait_count     = 10000000                                  ,
        .trg_cmd        = (pdsp_trg_cmd_func_t)   __pdsp_trigger_cmd,
        .rsp_sts        = (pdsp_rsp_sts_func_t)   __pdsp_rsp_status 
    },
    {   /* QPDSP */
        .id             = PDSP_ID_QoS                               ,
        .regs           = (PDSP_RegsOvly)   AVALANCHE_NWSS_QPDSP_CTRL_RGN_BASE ,
        .iram           = (Uint32 *)        AVALANCHE_NWSS_QPDSP_IRAM_RGN_BASE ,
        .iram_size      = SIZE_4K                                   ,
        .cmd_id_mask    = 0x000000FF                                ,
        .cmd_rsp_ram    = (Uint32 *)        IO_PHY2VIRT(0x03120000) ,
        .param_ram      = (Uint32 *)        IO_PHY2VIRT(0x03120004) ,
        .wait_count     = 10000000                                  ,
        .trg_cmd        = (pdsp_trg_cmd_func_t)   __pdsp_trigger_cmd,
        .rsp_sts        = (pdsp_rsp_sts_func_t)   __pdsp_rsp_status 
    },
};

#endif

static struct
{
    atomic_t                            lock_nesting;
    Uint32                              lock_state;
}
gPdspPrivate = { {0},0 };

static void  __pdsp_lock( void )
{
    Uint32  state;
    local_irq_save( state );  

    if ( 1 == atomic_add_return( 1, &gPdspPrivate.lock_nesting ) )
    {
        gPdspPrivate.lock_state = state;  
    }
}

static void  __pdsp_unlock( void )
{
    if ( 0 == atomic_sub_return( 1, &gPdspPrivate.lock_nesting ) )
    {
        local_irq_restore( gPdspPrivate.lock_state );
    }
}



static Int32 __pdsp_trigger_cmd(pdsp_info_t *pdsp, pdsp_cmd_t cmd)
{
    *pdsp->cmd_rsp_ram = cmd;
    if (pdsp->isr_reg)
    {
        *pdsp->isr_reg = pdsp->isr_mask;
    }
    return 0;
}

static Int32 __pdsp_rsp_status(pdsp_info_t *pdsp)
{
    Uint32 wait_count = pdsp->wait_count;

    if (pdsp->isr_reg)
    {
        while((*pdsp->isr_reg & pdsp->isr_mask) && wait_count)
        {
            wait_count--;
        }
    }
    else
    {
        while((*pdsp->cmd_rsp_ram & pdsp->cmd_id_mask) && wait_count)
        {
            wait_count--;
        }
    }

    if(!wait_count)
    {
        printk ("pdsp_rsp_status: Timeout waiting for PDSDP(%d).\n", pdsp->id);
        return (-1);
    }

    return 0;
}





Int32 pdsp_cmd_send    (pdsp_id_t               pdsp_id,
                        pdsp_cmd_t              cmd_word, 
                        void *wr_ptr,   Uint32  wr_word, 
                        void *rd_ptr,   Uint32  rd_word)
{
    pdsp_info_t *pdsp = &g_pdsp_info[pdsp_id];
    volatile Uint32 *param_buf    = pdsp->param_ram;
    Uint32* wr_wptr = (Uint32*) wr_ptr;
    Uint32* rd_wptr = (Uint32*) rd_ptr;
    Uint32 ret_code;

    if (pdsp->id != pdsp_id)
    {
        return -1;
    }

    PDSP_LOCK( g_pdsp_info[ pdsp_id ].sem_ptr );

    PDSP_DBG("COMMAND: %#x = %#x: ", (Uint32)pdsp->cmd_rsp_ram, (Uint32)cmd_word);
    while(wr_word--)
    {
        PDSP_DBG ("%#x = %#x ", (Uint32)param_buf, *wr_wptr);
        *param_buf++ = *wr_wptr++;
    }
    PDSP_DBG ("\n");

    pdsp->trg_cmd(pdsp, cmd_word);

    if(-1 == pdsp->rsp_sts(pdsp))
    {
        PDSP_UNLOCK( g_pdsp_info[ pdsp_id ].sem_ptr );
        return (-1);
    }

    if (pdsp->cmd_status_mask)
    {
        Uint32 mask = pdsp->cmd_status_mask;
        ret_code = *pdsp->cmd_rsp_ram & pdsp->cmd_status_mask;
        while (0 ==(mask & 0x1))
        {
            ret_code >>= 1;
            mask >>= 1;
        }
        param_buf = pdsp->param_ram;
    }
    else
    {
        ret_code = *pdsp->param_ram;
        param_buf = pdsp->param_ram + 1;
    }

    while(rd_word--)
        *rd_wptr++ = *param_buf++;

    PDSP_UNLOCK( g_pdsp_info[ pdsp_id ].sem_ptr );

    return ((ret_code == SR_RETCODE_SUCCESS) ? 0 : -ret_code);
}
EXPORT_SYMBOL(pdsp_cmd_send);

Int32 pdsp_control (pdsp_id_t   pdsp_id, Uint32 ctl_op, Ptr ctl_data)
{
    Int32   retcode = 0;

    if (g_pdsp_info[pdsp_id].id != pdsp_id)
    {
        return SRPDSP_EINVINDEX;
    }
    
    PDSP_LOCK( g_pdsp_info[ pdsp_id ].sem_ptr );

    switch (ctl_op)
    {
        case PDSPCTRL_HLT:
            g_pdsp_info[pdsp_id].regs->control &= ~(PDSP_CTL_EN_BIT);
            break;

        case PDSPCTRL_STEP:
            g_pdsp_info[pdsp_id].regs->control 
                    = (g_pdsp_info[pdsp_id].regs->control | (PDSP_CTL_EN_BIT)) | (PDSP_REG_SINGLE_STEP_BIT);
            break;

        case PDSPCTRL_FREERUN:
            g_pdsp_info[pdsp_id].regs->control 
                    = (g_pdsp_info[pdsp_id].regs->control | (PDSP_CTL_EN_BIT)) & ~(PDSP_REG_SINGLE_STEP_BIT);
            break;

        case PDSPCTRL_RESUME:
            g_pdsp_info[pdsp_id].regs->control |= (PDSP_CTL_EN_BIT);
            break;

        case PDSPCTRL_RST:
            {
                
                int wait_count = 10000000;
                g_pdsp_info[pdsp_id].regs->control = 0;

                while(wait_count--)
                {
                    if(g_pdsp_info[pdsp_id].regs->control & (PDSP_CTL_N_RST_BIT))
                    {
                        break;
                    }
                }

                if(!wait_count) 
                {
                    printk ("Timeout waiting for reset complete for PDSDP(%d)\n", pdsp_id);
                    retcode = SRPDSP_EINTERROR;
                    break;
                }
            }
            
            break;
                
        case PDSPCTRL_START:
            {
                g_pdsp_info[pdsp_id].regs->control = ((*((Uint16*)ctl_data)) << 16) 
                                                    | (PDSP_CTL_EN_BIT) 
                                                    | (PDSP_REG_COUNT_ENABLE_BIT);

                if (g_pdsp_info[pdsp_id].rsp_sts( &g_pdsp_info[pdsp_id] ))
                {
                    retcode = SRPDSP_EINTERROR;
                    break;
                }
            }
            break;


        default:
            printk ("Unsupported PDSP control option (%d)\n", ctl_op);
            retcode = SRPDSP_EINVCMD;
            break;
    }

    PDSP_UNLOCK( g_pdsp_info[ pdsp_id ].sem_ptr );

    return retcode;
}

#if defined (CONFIG_MACH_PUMA6)
inline void print_all_pdsp_units(void)
{
    pdsp_id_t   pdsp_id;

    printk("\n\nPDSP_ID   PDSP Name\n");
    printk("-------   ---------\n");
    for (pdsp_id = PDSP_ID_Prefetcher ; pdsp_id < PDSP_ID_CoE; pdsp_id++)
    {
        printk(" %3u       %s\n", pdsp_id, PRINT_PDSP_ID(pdsp_id));
    }
}

void halt_or_resume_pdsp(pdsp_ctrl_op_t operation, pdsp_id_t  pdsp_id)
{
    Int32     rc;
    Uint32    pdsp_param = 0;
    pdsp_id_t pdsp_id_itr;

    if (-1 == pdsp_id)
    {
        printk("\nPlease enter a valid PDSP ID from the list below: \n");
        print_all_pdsp_units();
        return;
    }
    else if (255 == pdsp_id)
    {
        printk("\nThe following PDSPs were set to %s: \n", (operation == PDSPCTRL_HLT) ? "HALT" : "RESUME");
        for (pdsp_id_itr = PDSP_ID_Prefetcher ; pdsp_id_itr < PDSP_ID_CoE ; pdsp_id_itr++)
        {
            if (NULL == g_pdsp_info[pdsp_id_itr].iram)
            {
                continue;
            }

            if ((rc = pdsp_control( pdsp_id_itr, operation, &pdsp_param )))
            {
                printk(KERN_ERR"\n%s: failed to halt the PDSP %s rc(%d)\n", __FUNCTION__, PRINT_PDSP_ID(pdsp_id_itr), rc );
            }

            printk("PDSP [%s] \n", PRINT_PDSP_ID(pdsp_id_itr));
        }

        return;
    }
    else
    {
        if (pdsp_id >= PDSP_ID_CoE)
        {
            printk(KERN_ERR"\nInvalid PDSP [%s]\n", PRINT_PDSP_ID(pdsp_id));
            return;
        }

        if (NULL == g_pdsp_info[pdsp_id].iram)
        {
            printk("PDSP [%s] is not configured. Abort operarion \n", PRINT_PDSP_ID(pdsp_id));
            return;
        }

        if ((rc = pdsp_control( pdsp_id, operation, &pdsp_param )))
        {
            printk(KERN_ERR"\n%s: failed to %s the PDSP %s rc(%d)\n", __FUNCTION__, (operation == PDSPCTRL_HLT) ? "HALT" : "RESUME", PRINT_PDSP_ID(pdsp_id), rc );
            return ;
        }

        printk("\nPDSP [%s] was %s \n", PRINT_PDSP_ID(pdsp_id), (operation == PDSPCTRL_HLT) ? "HALT" : "RESUMED");
    }

}

void print_debug_regs(pdsp_cmd_params_t   *usr_params)
{
    Uint32 reg, regValue;

    printk("\n\n[%s] Command Line = 0x%04x\n\n", PRINT_PDSP_ID(usr_params->pdsp_id), be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->status));
    for (reg = 0 ; reg < NUM_OF_DEBUG_REGS ; reg++)
    {
        regValue = be32_to_cpu(*(g_pdsp_info[usr_params->pdsp_id].dbg_regs + reg));

        if (regValue != debugRegs[reg])
        {
            printk("R%02u: %08x**  ", reg, regValue);
        }
        else
        {
            printk("R%02u: %08x    ", reg, regValue);
        }

        if (0 == ((reg+1) % 4))
        {
            printk("\n");
        }
    }

    printk("\n");

    memcpy(&debugRegs[0], g_pdsp_info[usr_params->pdsp_id].dbg_regs, sizeof(debugRegs));
}

Uint32 pdspHasBreakPointOn(pdsp_cmd_params_t   *bp_params)
{
    Uint32 slot;

    for (slot = 0; slot < MAX_BREAKPOINTS ; slot++)
    {
        // if (PDSP current cmd == breakpoint cmd) && (opCode != 0) && it's the same PDSP then we found a breakpoint
        if (pdsp_breakpoint_info.breakpoints[slot].commandNum == be32_to_cpu(g_pdsp_info[bp_params->pdsp_id].regs->status) &&
            pdsp_breakpoint_info.breakpoints[slot].pdsp_id    == bp_params->pdsp_id                           &&
            pdsp_breakpoint_info.breakpoints[slot].opCode     != 0                                              )
        {
            bp_params->cmd = be32_to_cpu(g_pdsp_info[bp_params->pdsp_id].regs->status); // copy the command line in order to delete it afterwards
            return 1;
        }
    }

    return 0;
}

void singleStep_enable(pdsp_cmd_params_t   *usr_params)
{
    Int32       rc;
    Uint32      pdsp_param = 0;
    Uint32      shouldHalt;
    Bool        hasBreakPoint, breakPointWasRemoved;
    pdsp_cmd_params_t   bp_params;

    hasBreakPoint = breakPointWasRemoved = False;

    if (usr_params->pdsp_id == -1)
    {
        printk("\nPlease enter a valid PDSP ID from the list below: \n");
        print_all_pdsp_units();
        return;
    }

    if (usr_params->pdsp_id >= PDSP_ID_CoE)
    {
        printk(KERN_ERR"\nInvalid PDSP [%s]\n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }

    if (NULL == g_pdsp_info[usr_params->pdsp_id].iram)
    {
        printk("PDSP [%s] is not configured. Abort operarion \n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }
    if (MAX_SINGLE_STEP_CMDS < usr_params->cmd)
    {
        printk(KERN_ERR"\n%s: Invalid Num of commands for PDSP [%s]. Max single step commands : [%u]\n", __FUNCTION__, PRINT_PDSP_ID(usr_params->pdsp_id), MAX_SINGLE_STEP_CMDS);
        return;
    }

    if (0 == usr_params->cmd)
    {
        print_debug_regs(usr_params);
        return;
    }

    if ((be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->control) & (PDSP_REG_SINGLE_STEP_BIT)) == 0)
    {
        printk("\nChange PDSP [%s] to Single Step. Number Of steps = [%u] \n", PRINT_PDSP_ID(usr_params->pdsp_id), usr_params->cmd);
    }

    // Halt only if PDSP is enabled and not in single step mode
    shouldHalt = ( (be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->control) & (PDSP_CTL_EN_BIT)) &&
                  ((be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->control) & (PDSP_REG_SINGLE_STEP_BIT)) == 0)  );
    if (shouldHalt)
    {
        halt_or_resume_pdsp(PDSPCTRL_HLT, usr_params->pdsp_id);

        print_debug_regs(usr_params);
    }

    bp_params.pdsp_id = usr_params->pdsp_id;
    hasBreakPoint     = pdspHasBreakPointOn(&bp_params);
    if (hasBreakPoint && (usr_params->cmd == 1))
    {
        // if we have a breakpoint and we have left 1 command then remove it and progress
        // if we have more than one command left, the operator might wanted us to stop at the breakpoint, so we don't remove it
        breakpoint_disable(&bp_params);
        breakPointWasRemoved = True;
    }

    while (usr_params->cmd--)
    {
        if ((rc = pdsp_control(usr_params->pdsp_id, PDSPCTRL_STEP, &pdsp_param)))
        {
            printk(KERN_ERR"\n%s: failed to halt PDSP %s rc(%d)\n", __FUNCTION__, PRINT_PDSP_ID(usr_params->pdsp_id), rc );
            return;
        }

        print_debug_regs(usr_params);
    }

    if (hasBreakPoint && breakPointWasRemoved)
    {
        // Return breakpoint that we removed
        breakpoint_enable(&bp_params);
    }
}

void singleStep_disable(pdsp_cmd_params_t   *usr_params)
{
    Int32       rc;
    Uint32      pdsp_param = 0;

    if (usr_params->pdsp_id == -1)
    {
        printk("\nPlease enter a valid PDSP ID from the list below: \n");
        print_all_pdsp_units();
        return;
    }

    if (usr_params->pdsp_id >= PDSP_ID_CoE)
    {
        printk(KERN_ERR"\nInvalid PDSP [%s]\n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }

    if (NULL == g_pdsp_info[usr_params->pdsp_id].iram)
    {
        printk("PDSP [%s] is not configured. Abort operarion\n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }

    printk("\nChange PDSP [%s] to free running. \n", PRINT_PDSP_ID(usr_params->pdsp_id));

    if ((rc = pdsp_control(usr_params->pdsp_id, PDSPCTRL_FREERUN, &pdsp_param)))
    {
        printk(KERN_ERR"\n%s: failed to halt PDSP %s rc(%d)\n", __FUNCTION__, PRINT_PDSP_ID(usr_params->pdsp_id), rc );
        return;
    }
}

void breakpoint_print(pdsp_cmd_params_t   *usr_params)
{
    Int32 availableSlot;

    if (usr_params->pdsp_id == -1)
    {
        printk("\nPlease enter a valid PDSP ID from the list below: \n");
        print_all_pdsp_units();
        return;
    }
    else if (255 == usr_params->pdsp_id)
    {
        for (availableSlot = 0; availableSlot < MAX_BREAKPOINTS ; availableSlot++)
        {
            if (0 != pdsp_breakpoint_info.breakpoints[availableSlot].opCode)
            {
                printk("PDSP [%s] \t\t - Command Line [0x%04x]\n", PRINT_PDSP_ID(pdsp_breakpoint_info.breakpoints[availableSlot].pdsp_id), pdsp_breakpoint_info.breakpoints[availableSlot].commandNum);
            }
        }
    }
    else
    {
        if (usr_params->pdsp_id >= PDSP_ID_CoE)
        {
            printk(KERN_ERR"\nInvalid PDSP [%s]\n", PRINT_PDSP_ID(usr_params->pdsp_id));
            return;
        }

        if (NULL == g_pdsp_info[usr_params->pdsp_id].iram)
        {
            printk("PDSP [%s] is not configured. Abort operarion \n", PRINT_PDSP_ID(usr_params->pdsp_id));
            return;
        }

        for (availableSlot = 0; availableSlot < MAX_BREAKPOINTS ; availableSlot++)
        {
            if ((0                   != pdsp_breakpoint_info.breakpoints[availableSlot].opCode) &&
                (usr_params->pdsp_id == pdsp_breakpoint_info.breakpoints[availableSlot].pdsp_id) )
            {
                printk("PDSP [%s] \t\t - Command Line [0x%04x]\n", PRINT_PDSP_ID(usr_params->pdsp_id), pdsp_breakpoint_info.breakpoints[availableSlot].commandNum);
            }
        }
    }
}

void breakpoint_enable(pdsp_cmd_params_t   *usr_params)
{
    Int32       availableSlot;
    Uint32      shouldHalt;

    if (usr_params->pdsp_id == -1)
    {
        printk("\nPlease enter a valid PDSP ID from the list below: \n");
        print_all_pdsp_units();
        return;
    }

    if (usr_params->pdsp_id >= PDSP_ID_CoE)
    {
        printk(KERN_ERR"\nInvalid PDSP [%s]\n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }

    if (NULL == g_pdsp_info[usr_params->pdsp_id].iram)
    {
        printk("PDSP [%s] is not configured. Abort operarion \n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }

    if (pdsp_breakpoint_info.numOfBreakPoints >= MAX_BREAKPOINTS)
    {
        printk(KERN_ERR"\n%s: Reached to the max num of breakpoints (%u) \n", __FUNCTION__, MAX_BREAKPOINTS);
        return;
    }


    for (availableSlot = 0; availableSlot < MAX_BREAKPOINTS ; availableSlot++)
    {
        if (0 != pdsp_breakpoint_info.breakpoints[availableSlot].opCode)
        {
            if (pdsp_breakpoint_info.breakpoints[availableSlot].commandNum == usr_params->cmd &&
                pdsp_breakpoint_info.breakpoints[availableSlot].pdsp_id    == usr_params->pdsp_id)
            {
                printk(KERN_ERR"\n%s: BreakPoint for PDSP %s - command id (%u) is already configured \n", __FUNCTION__, PRINT_PDSP_ID(usr_params->pdsp_id), usr_params->cmd);
                return;
            }
        }
    }
    if (usr_params->cmd >= g_pdsp_info[usr_params->pdsp_id].iram_size)
    {
        printk(KERN_ERR"\n%s: OP Code command index(0x%x) > iRam size(0x%x) for PDSP [%s]\n", __FUNCTION__, usr_params->cmd, g_pdsp_info[usr_params->pdsp_id].iram_size, PRINT_PDSP_ID(usr_params->pdsp_id));
    }

    shouldHalt = be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->control) & (PDSP_CTL_EN_BIT);
    if (shouldHalt)
    {
        halt_or_resume_pdsp(PDSPCTRL_HLT, usr_params->pdsp_id);
    }

    for (availableSlot = 0; availableSlot < MAX_BREAKPOINTS ; availableSlot++)
    {
        if (0 == pdsp_breakpoint_info.breakpoints[availableSlot].opCode)
        {
            break;
        }
    }

    if (availableSlot == MAX_BREAKPOINTS)
    {
        printk(KERN_ERR"\n%s: available slot was not found\n", __FUNCTION__);
        return;
    }

    pdsp_breakpoint_info.breakpoints[availableSlot].pdsp_id     = usr_params->pdsp_id;
    pdsp_breakpoint_info.breakpoints[availableSlot].commandNum  = usr_params->cmd;
    pdsp_breakpoint_info.breakpoints[availableSlot].opCode      = be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].iram[usr_params->cmd]);
    pdsp_breakpoint_info.numOfBreakPoints++;

    g_pdsp_info[usr_params->pdsp_id].iram[usr_params->cmd] = cpu_to_be32(HALT_OP_CODE);

    printk("\nBreakPoint was set in PDSP [%s] commandNum=[0x%x] opCode[0x%x]. iRam Base[0x%x]  iRam cmd address[0x%x]\n",
           PRINT_PDSP_ID(usr_params->pdsp_id),
           pdsp_breakpoint_info.breakpoints[availableSlot].commandNum,
           pdsp_breakpoint_info.breakpoints[availableSlot].opCode,
           (Uint32)IO_VIRT2PHY(g_pdsp_info[usr_params->pdsp_id].iram),
           (Uint32)IO_VIRT2PHY(&g_pdsp_info[usr_params->pdsp_id].iram[usr_params->cmd]));

    if (shouldHalt)
    {
        halt_or_resume_pdsp(PDSPCTRL_RESUME, usr_params->pdsp_id);
    }
}

void delete_all_breakpoints(void)
{
    Uint32 slot;
    pdsp_cmd_params_t   usr_params;

    printk("\n");

    for (slot = 0; slot < MAX_BREAKPOINTS ; slot++)
    {
        if (pdsp_breakpoint_info.breakpoints[slot].opCode != 0)
        {
            usr_params.pdsp_id = pdsp_breakpoint_info.breakpoints[slot].pdsp_id;
            usr_params.cmd     = pdsp_breakpoint_info.breakpoints[slot].commandNum;
            breakpoint_disable(&usr_params);
            printk("\n");
        }
    }
}

void run_to_opCode(pdsp_cmd_params_t   *usr_params)
{
    Uint32      shouldHalt, pdsp_param, numOfCommands, reqCommand;

    pdsp_param = numOfCommands = 0;

    reqCommand = usr_params->cmd;

    if (usr_params->pdsp_id == -1)
    {
        printk("\nPlease enter a valid PDSP ID from the list below: \n");
        print_all_pdsp_units();
        return;
    }

    if (usr_params->pdsp_id >= PDSP_ID_CoE)
    {
        printk(KERN_ERR"\nInvalid PDSP [%s]\n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }

    if (NULL == g_pdsp_info[usr_params->pdsp_id].iram)
    {
        printk("PDSP [%s] is not configured. Abort operarion \n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }
    if (usr_params->cmd >= g_pdsp_info[usr_params->pdsp_id].iram_size)
    {
        printk(KERN_ERR"\n%s: OP Code command index(%u) > iRam size(%u) for PDSP [%s]\n", __FUNCTION__, usr_params->cmd, g_pdsp_info[usr_params->pdsp_id].iram_size, PRINT_PDSP_ID(usr_params->pdsp_id));
    }

    // Halt only if PDSP is enabled and not in single step mode in order to stop and read current command
    shouldHalt = ( (be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->control) & (PDSP_CTL_EN_BIT)) &&
                  ((be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->control) & (PDSP_REG_SINGLE_STEP_BIT)) == 0)  );
    if (shouldHalt)
    {
        halt_or_resume_pdsp(PDSPCTRL_HLT, usr_params->pdsp_id);
    }

    while ((be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->status) != reqCommand) &&
           (numOfCommands++ < MAX_SINGLE_STEP_CMDS))
    {
        usr_params->cmd = 1;                // every time we do a single step
        singleStep_enable(usr_params);
    }

    if (be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->status) == reqCommand)
    {
        printk("\nPDSP [%s] reached to command [0x%x] \n\n", PRINT_PDSP_ID(usr_params->pdsp_id), reqCommand);
    }
    else
    {
        printk("\nPDSP [%s] couldn't reach to command [0x%x]. Stopped at command [0x%x] \n\n", PRINT_PDSP_ID(usr_params->pdsp_id), reqCommand, be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->status));
    }
}

void breakpoint_disable(pdsp_cmd_params_t   *usr_params)
{
    Int32       slot;
    Uint32      shouldHalt;

    if (usr_params->pdsp_id == -1)
    {
        printk("\nPlease enter a valid PDSP ID from the list below: \n");
        print_all_pdsp_units();
        return;
    }

    if (usr_params->pdsp_id >= PDSP_ID_CoE)
    {
        printk(KERN_ERR"\nInvalid PDSP [%s]\n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }

    if (NULL == g_pdsp_info[usr_params->pdsp_id].iram)
    {
        printk("PDSP [%s] is not configured. Abort operarion \n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }
    if (usr_params->cmd >= g_pdsp_info[usr_params->pdsp_id].iram_size)
    {
        printk(KERN_ERR"\n%s: OP Code command index(%u) > iRam size(%u) for PDSP [%s]\n", __FUNCTION__, usr_params->cmd, g_pdsp_info[usr_params->pdsp_id].iram_size, PRINT_PDSP_ID(usr_params->pdsp_id));
    }

    shouldHalt = be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->control) & (PDSP_CTL_EN_BIT);
    if (shouldHalt)
    {
        halt_or_resume_pdsp(PDSPCTRL_HLT, usr_params->pdsp_id);
    }

    for (slot = 0; slot < MAX_BREAKPOINTS ; slot++)
    {
        if (usr_params->cmd     == pdsp_breakpoint_info.breakpoints[slot].commandNum &&
            usr_params->pdsp_id == pdsp_breakpoint_info.breakpoints[slot].pdsp_id    &&
            0                   != pdsp_breakpoint_info.breakpoints[slot].opCode)
        {
            break;
        }
    }

    if (MAX_BREAKPOINTS == slot)
    {
        printk(KERN_ERR"\n%s: failed to find breakpoint for command [0x%x] in PDSP [%s]\n", __FUNCTION__, usr_params->cmd, PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }

    g_pdsp_info[usr_params->pdsp_id].iram[usr_params->cmd]  = cpu_to_be32(pdsp_breakpoint_info.breakpoints[slot].opCode) ;

    printk("\nBreakPoint was removed in PDSP [%s] commandNum=[0x%x] opCode[0x%x]. iRam Base[0x%x]  iRam cmd address[0x%x]\n",
           PRINT_PDSP_ID(usr_params->pdsp_id),
           pdsp_breakpoint_info.breakpoints[slot].commandNum,
           pdsp_breakpoint_info.breakpoints[slot].opCode,
           (Uint32)IO_VIRT2PHY(g_pdsp_info[usr_params->pdsp_id].iram),
           (Uint32)IO_VIRT2PHY(&g_pdsp_info[usr_params->pdsp_id].iram[usr_params->cmd]));

    pdsp_breakpoint_info.breakpoints[slot].opCode = 0;
    pdsp_breakpoint_info.numOfBreakPoints--;

    if (shouldHalt)
    {
        halt_or_resume_pdsp(PDSPCTRL_RESUME, usr_params->pdsp_id);
    }
}

void show_status(pdsp_cmd_params_t   *usr_params)
{
    pdsp_cmd_params_t   pdsp_cmd;
    pdsp_id_t           pdsp_id;
    Uint32              isCtrlBitEnabled, isSingleStep;
    Int32               getStatusRc;

    printk("\n\n");


    printk(" Id       PDSP            STATUS            MODE      \n");
    printk(" --  -------------    -------------     ------------  \n");

    for (pdsp_id = PDSP_ID_Prefetcher; pdsp_id < PDSP_ID_CoE ; pdsp_id++)
    {
        if (NULL == g_pdsp_info[pdsp_id].iram)
        {
            printk(" %2u  %-13s    N/A \n", pdsp_id, PRINT_PDSP_ID(pdsp_id));
        }
        else
        {
            isCtrlBitEnabled = (be32_to_cpu(g_pdsp_info[pdsp_id].regs->control) & (PDSP_CTL_EN_BIT));
            isSingleStep = (be32_to_cpu(g_pdsp_info[pdsp_id].regs->control) & (PDSP_REG_SINGLE_STEP_BIT));
            printk(" %2u  %-13s    %-13s    %s\n", pdsp_id, PRINT_PDSP_ID(pdsp_id), (isCtrlBitEnabled ? "Enable " : "Halted "), (isSingleStep ? "Single step" : "Free running"));
        }
    }

    printk("\n\n");
}

void modify_dbg_regs(pdsp_cmd_params_t   *usr_params)
{
    Uint32      oldRegValue;

    if (usr_params->pdsp_id == -1)
    {
        printk("\nPlease enter a valid PDSP ID from the list below: \n");
        print_all_pdsp_units();
        return;
    }

    if (usr_params->pdsp_id >= PDSP_ID_CoE)
    {
        printk("\nInvalid PDSP [%s]\n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }

    if (NULL == g_pdsp_info[usr_params->pdsp_id].iram)
    {
        printk("PDSP [%s] is not configured. Abort operarion \n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }

    if (usr_params->params[0] >= NUM_OF_DEBUG_REGS)
    {
        printk("\nInvalid debug reg[%u]. valid value (0-31)\n", usr_params->params[0]);
        return;
    }

    if ( (be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->control) & (PDSP_CTL_EN_BIT)) &&
        ((be32_to_cpu(g_pdsp_info[usr_params->pdsp_id].regs->control) & (PDSP_REG_SINGLE_STEP_BIT)) == 0)  )
    {
        printk("\nPDSP [%s] is not in single step. Pls move to single step first \n", PRINT_PDSP_ID(usr_params->pdsp_id));
        return;
    }

    oldRegValue = be32_to_cpu(*(g_pdsp_info[usr_params->pdsp_id].dbg_regs + usr_params->params[0]));

    *(g_pdsp_info[usr_params->pdsp_id].dbg_regs + usr_params->params[0]) = cpu_to_be32(usr_params->params[1]);

    printk("\nPDSP [%s] debug Reg [%u]: old value [0x%x] --> new value [0x%x]\n", PRINT_PDSP_ID(usr_params->pdsp_id), usr_params->params[0], oldRegValue, usr_params->params[1]);

    print_debug_regs(usr_params);
}
#endif

/* **************************************************************************************** */
/*                                                                                          */
/*                                                                                          */
/*                                                                                          */
/*                  PDSP character device and related data                                  */
/*                                                                                          */
/*                                                                                          */
/*                                                                                          */
/* **************************************************************************************** */

static ssize_t __pdspDownload (struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    if (NULL == __pdsp_download_current_addr)
    {
        return -ERESTARTSYS;
    }

    /*Copy the internal HAL data structure*/
    if (copy_from_user((void *)__pdsp_download_current_addr, buf, count))
    {
        printk("%s : failed to copy data from user\n", __FUNCTION__);
        return -EFAULT;
    }

    __pdsp_download_current_addr += count;
    *ppos += count;
    return   count;
}


static long __pdspIoctl ( struct file * filp , unsigned int cmd , unsigned long arg )
{
    pdsp_id_t   pdsp_id;
    Int32       rc;
    Uint32      pdsp_param = 0;
   
    if (copy_from_user(&pdsp_id, (void __user *)arg, sizeof(pdsp_id)))
    {
        printk(KERN_ERR"\n%s: failed to copy from user\n", __FUNCTION__);
        return -EFAULT;
    }

    switch (cmd)
    {
    case PDSP_DRIVER_PUT_CMD:
        {
            pdsp_cmd_params_t   usr_params;

            if (copy_from_user(&usr_params, (void __user *)arg, sizeof(usr_params)))
            {
                printk(KERN_ERR"\n%s: failed to copy from user\n", __FUNCTION__);
                return -EFAULT;
            }

            if (rc = pdsp_cmd_send( pdsp_id, 
                           usr_params.cmd,
                           usr_params.params,   usr_params.params_len/sizeof(Uint32),
                           usr_params.params,   usr_params.params_len/sizeof(Uint32) ))
            {
                printk(KERN_ERR"\n%s: failed to put command(%X) to the PDSP %d rc(%d)\n", __FUNCTION__, usr_params.cmd, pdsp_id, rc );
                return -EINVAL;
            }

            if (copy_to_user((void __user *)arg, &usr_params, sizeof(usr_params)))
            {
                printk(KERN_ERR"\n%s: failed to copy response to user\n", __FUNCTION__);
                return -EFAULT;
            }
        }
        break;

    case PDSP_DRIVER_RESET_PDSP:
        {
            if (rc = pdsp_control( pdsp_id, PDSPCTRL_RST, &pdsp_param ))
            {
                printk(KERN_ERR"\n%s: failed to reset the PDSP %d rc(%d)\n", __FUNCTION__, pdsp_id, rc );
                return -EINVAL;
            }
        }
        break;

    case PDSP_DRIVER_START_PDSP:
        {
            if (rc = pdsp_control( pdsp_id, PDSPCTRL_START, &pdsp_param ))
            {
                printk(KERN_ERR"\n%s: failed to start the PDSP %d rc(%d)\n", __FUNCTION__, pdsp_id, rc );
                return -EINVAL;
            }
        }
        break;

    case PDSP_DRIVER_TEST_IRAM:
        {
            if ( g_pdsp_info[pdsp_id].id != pdsp_id )
            {
                return -EINVAL;
            }

            PDSP_LOCK( g_pdsp_info[ pdsp_id ].sem_ptr );

            if (rc = avalanche_do_ram_test( g_pdsp_info[pdsp_id].iram , g_pdsp_info[pdsp_id].iram_size ))
            {
                printk("###############################################################################\n");
                printk(" PP PDSP(%d) IRAM check failed, ret=%d \n", pdsp_id, rc );
                printk("###############################################################################\n");
                BUG();
            }

            memset( g_pdsp_info[pdsp_id].iram, 0, g_pdsp_info[pdsp_id].iram_size );

            PDSP_UNLOCK( g_pdsp_info[ pdsp_id ].sem_ptr );

            if (rc)
            {
                return -EIO; 
            }
        }
        break;

    case PDSP_DRIVER_DOWNLOAD_START:
        {
            down(__pdsp_download_sem);
            __pdsp_download_current_addr = g_pdsp_info[ pdsp_id ].iram;
        }
        break;

    case PDSP_DRIVER_DOWNLOAD_FINISH:
        {
            *g_pdsp_info[ pdsp_id ].cmd_rsp_ram = 0xFFFFFFFF;

            __pdsp_download_current_addr = NULL;
            up(__pdsp_download_sem);
        }
        break;

#if defined (CONFIG_MACH_PUMA6)
    case PDSP_DRIVER_HALT_PDSP:
        {
            pdsp_cmd_params_t   usr_params;

            if (copy_from_user(&usr_params, (void __user *)arg, sizeof(usr_params)))
            {
                printk(KERN_ERR"\n%s: failed to copy from user\n", __FUNCTION__);
                return -EFAULT;
            }

            halt_or_resume_pdsp(PDSPCTRL_HLT, usr_params.pdsp_id);
        }
        break;

    case PDSP_DRIVER_RESUME_PDSP:
        {
            pdsp_cmd_params_t   usr_params;

            if (copy_from_user(&usr_params, (void __user *)arg, sizeof(usr_params)))
            {
                printk(KERN_ERR"\n%s: failed to copy from user\n", __FUNCTION__);
                return -EFAULT;
            }

            halt_or_resume_pdsp(PDSPCTRL_RESUME, usr_params.pdsp_id);
        }
        break;

    case PDSP_DRIVER_PDSP_RUN_TO_OP_CODE:
        {
            pdsp_cmd_params_t   usr_params;

            if (copy_from_user(&usr_params, (void __user *)arg, sizeof(usr_params)))
            {
                printk(KERN_ERR"\n%s: failed to copy from user\n", __FUNCTION__);
                return -EFAULT;
            }

            run_to_opCode(&usr_params);
        }
        break;

    case PDSP_DRIVER_PDSP_BREAKPOINT_PRINT:
        {
            pdsp_cmd_params_t   usr_params;

            if (copy_from_user(&usr_params, (void __user *)arg, sizeof(usr_params)))
            {
                printk(KERN_ERR"\n%s: failed to copy from user\n", __FUNCTION__);
                return -EFAULT;
            }

            breakpoint_print(&usr_params);
        }
        break;

    case PDSP_DRIVER_PDSP_BREAKPOINT_ENABLE:
        {
            pdsp_cmd_params_t   usr_params;

            if (copy_from_user(&usr_params, (void __user *)arg, sizeof(usr_params)))
            {
                printk(KERN_ERR"\n%s: failed to copy from user\n", __FUNCTION__);
                return -EFAULT;
            }

            breakpoint_enable(&usr_params);
        }
        break;

    case PDSP_DRIVER_PDSP_BREAKPOINT_DISABLE:
        {
            pdsp_cmd_params_t   usr_params;

            if (copy_from_user(&usr_params, (void __user *)arg, sizeof(usr_params)))
            {
                printk(KERN_ERR"\n%s: failed to copy from user\n", __FUNCTION__);
                return -EFAULT;
            }

            breakpoint_disable(&usr_params);
        }
        break;

    case PDSP_DRIVER_PDSP_SHOW_STATUS:
        {
            pdsp_cmd_params_t   usr_params;

            if (copy_from_user(&usr_params, (void __user *)arg, sizeof(usr_params)))
            {
                printk(KERN_ERR"\n%s: failed to copy from user\n", __FUNCTION__);
                return -EFAULT;
            }

            show_status(&usr_params);
        }
        break;

    case PDSP_DRIVER_PDSP_MODIFY_DBG_REG:
        {
            pdsp_cmd_params_t   usr_params;

            if (copy_from_user(&usr_params, (void __user *)arg, sizeof(usr_params)))
            {
                printk(KERN_ERR"\n%s: failed to copy from user\n", __FUNCTION__);
                return -EFAULT;
            }

            modify_dbg_regs(&usr_params);
        }
        break;

    case PDSP_DRIVER_PDSP_SINGLE_STEP_ENABLE:
        {
            pdsp_cmd_params_t   usr_params;

            if (copy_from_user(&usr_params, (void __user *)arg, sizeof(usr_params)))
            {
                printk(KERN_ERR"\n%s: failed to copy from user\n", __FUNCTION__);
                return -EFAULT;
            }

            singleStep_enable(&usr_params);
        }
        break;

    case PDSP_DRIVER_PDSP_SINGLE_STEP_DISABLE:
        {
            pdsp_cmd_params_t   usr_params;

            if (copy_from_user(&usr_params, (void __user *)arg, sizeof(usr_params)))
            {
                printk(KERN_ERR"\n%s: failed to copy from user\n", __FUNCTION__);
                return -EFAULT;
            }

            singleStep_disable(&usr_params);
        }
        break;

    case PDSP_DRIVER_PDSP_BREAKPOINT_DELETE_ALL:
        {
            delete_all_breakpoints();
        }
        break;
#endif
        default:
        break;
    }

    return 0; 
}

#define DEVICE_MAJOR    100
#define DEVICE_NAME     "pdsp"

static struct file_operations   pdspCdevFops =
{
    .owner              = THIS_MODULE,
    .unlocked_ioctl     = __pdspIoctl,
    .write              = __pdspDownload,
};

static void         __module_pdsp_exit (void)
{
    printk ( KERN_INFO " MODULE: PDSP Stop  ...\n");

#if 1
    /* Freeing the major number */
    unregister_chrdev ( DEVICE_MAJOR , DEVICE_NAME );
#else
    device_destroy( pdspdev_class, pdsp_dev );
    class_destroy( pdspdev_class );
    cdev_del( pdspc_dev );
    unregister_chrdev_region( pdsp_dev, 1 );
#endif

    if (__pdsp_download_sem)
    {
        kfree(__pdsp_download_sem);
    }

    if (g_pdsp_info[PDSP_ID_Classifier].sem_ptr) 
    {
        kfree(g_pdsp_info[PDSP_ID_Classifier].sem_ptr); 
    }

#if defined (CONFIG_MACH_PUMA6)
    if (g_pdsp_info[PDSP_ID_LAN_Proxy].sem_ptr)
    {
        kfree(g_pdsp_info[PDSP_ID_LAN_Proxy].sem_ptr); 
    }
#endif
    if (g_pdsp_info[PDSP_ID_Prefetcher].sem_ptr)
    {
        kfree(g_pdsp_info[PDSP_ID_Prefetcher].sem_ptr); 
    }

#if defined (CONFIG_MACH_PUMA6)
    if (g_pdsp_info[PDSP_ID_CoE].sem_ptr)
    {
        kfree(g_pdsp_info[PDSP_ID_CoE].sem_ptr); 
    }
#endif
}


static int __init   __module_pdsp_init (void)
{
    int                 ret;
    struct device * dev_ret;

    printk ( " MODULE: PDSP Start ...\n");

    /* ************************************************************************ */
    /*                                                                          */
    /*         Interface device initialization stuff ....                       */
    /*                                                                          */
    /* ************************************************************************ */

    /* Registering device */
    if ( register_chrdev ( DEVICE_MAJOR , DEVICE_NAME , &pdspCdevFops ) < 0 )
    {
        printk ( KERN_WARNING "memory: cannot obtain major number %d\n", DEVICE_MAJOR );
        return -EIO;
    }

    {
        struct semaphore *      sem;
        if (NULL ==(sem = kmalloc( sizeof(struct semaphore), GFP_KERNEL )))
        {
            __module_pdsp_exit();
            return -ENOMEM;
        }

        sema_init( sem, 1 );

        g_pdsp_info[ PDSP_ID_Classifier  ].sem_ptr =
#if defined (CONFIG_MACH_PUMA6)
        g_pdsp_info[ PDSP_ID_Classifier2 ].sem_ptr = 
#endif
        g_pdsp_info[ PDSP_ID_Modifier    ].sem_ptr = 
        g_pdsp_info[ PDSP_ID_QoS         ].sem_ptr = sem;

#if defined (CONFIG_MACH_PUMA6)
        if (NULL ==(sem = kmalloc( sizeof(struct semaphore), GFP_KERNEL )))
        {
            __module_pdsp_exit();
            return -ENOMEM;
        }

        sema_init( sem, 1 );


        g_pdsp_info[ PDSP_ID_LAN_Proxy   ].sem_ptr = sem;
#endif

        if (NULL ==(sem = kmalloc( sizeof(struct semaphore), GFP_KERNEL )))
        {
            __module_pdsp_exit();
            return -ENOMEM;
        }

        sema_init( sem, 1 );

        g_pdsp_info[ PDSP_ID_Prefetcher  ].sem_ptr = sem;

        if (NULL ==(sem = kmalloc( sizeof(struct semaphore), GFP_KERNEL )))
        {
            __module_pdsp_exit();
            return -ENOMEM;
        }

        sema_init( sem, 1 );

#if defined (CONFIG_MACH_PUMA6)
        if (NULL ==(sem = kmalloc( sizeof(struct semaphore), GFP_KERNEL )))
        {
            __module_pdsp_exit();
            return -ENOMEM;
        }

        sema_init( sem, 1 );

        g_pdsp_info[ PDSP_ID_CoE  ].sem_ptr = sem;
#endif
        __pdsp_download_sem = sem;
    }

    return 0;
}

MODULE_AUTHOR("Intel Corporation");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDSP Driver");

module_init(__module_pdsp_init);
module_exit(__module_pdsp_exit);


