/*
* This file is provided under a dual BSD/GPLv2 license.  When using or
* redistributing this file, you may do so under either license.
*
* GPL LICENSE SUMMARY
*
* Copyright(c) 2012-2019 Intel Corporation.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of version 2 of the GNU General Public License as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
* The full GNU General Public License is included in this distribution
* in the file called LICENSE.GPL.
*
*
* Contact Information:
* Intel Corporation
* 2200 Mission College Blvd.
* Santa Clara, CA  97052
*
* BSD LICENSE
*
* Copyright(c) 2012-2019 Intel Corporation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.
*
*   * Neither the name of Intel Corporation nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#define DRV_NAME    "Voice Network Interface driver"
#define DRV_VERSION    "0.0.1"


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/mii.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/proc_fs.h>

#include "pal.h"
#include "pal_cppi41.h"
#include <linux/kernel.h>
#include <sockios.h>
#include "voice_ni_api.h"


#define VOICENI_RXINT_NUM                  (AVALANCHE_INTD_BASE_INT + PAL_CPPI41_VOICE_DSP_C55_ACC_RX_INTV)
#define VOICENI_INTD_HOST_NUM    0


#define GET_VOICENI_BD_PTR(base, num)              (((Uint32)base) + (((Uint32)num) * PAL_CPPI41_VOICE_DSP_C55_EMB_BD_SIZE))

/* Accumulator channel setup defines */
#define VOICENI_ACC_PAGE_NUM_ENTRY         (32)
#define VOICENI_ACC_NUM_PAGE               (2)
#define VOICENI_ACC_ENTRY_TYPE             (PAL_CPPI41_ACC_ENTRY_TYPE_D)
#define VOICENI_ACC_RX_CH_COUNT 1

/* Byte size of page = number of entries per page * size of each entry */
#define VOICENI_ACC_PAGE_BYTE_SZ           (VOICENI_ACC_PAGE_NUM_ENTRY * ((VOICENI_ACC_ENTRY_TYPE + 1) * sizeof(unsigned int*)))
/* byte size of list = byte size of page * number of pages * */
#define VOICENI_ACC_LIST_BYTE_SZ           (VOICENI_ACC_PAGE_BYTE_SZ * VOICENI_ACC_NUM_PAGE)


#define MAX_SKB_SIZE    1024

typedef struct voiceni_net_dev_t
{
    /* Pointer to the network device. */
    struct net_device*          ptr_device;

    /* Keep track of the statistics. */
    struct net_device_stats     stats;
}VOICENI_NET_DEV_T;



/************************************************************************/
/*     voiceNI driver private data                                          */
/************************************************************************/
typedef struct voicenid_private_t 
{
    PAL_Handle                  pal_hnd;            /* The handle to PAL layer */
    Ptr                         buf_pool;           /* The handle to the buffer pool */
    PAL_Cppi4QueueHnd           free_bd_queue;      /* free queue of BD's used by both DSP and voiceNI */
    PAL_Cppi4QueueHnd           free_infra_bd_queue;/* free queue of BD's used by Infrastructure DMA */
    Ptr                         free_bd_pool;       /* pool of BD's used by both DSP and voiceNI */
    PAL_Cppi4QueueHnd           voiceni_rx_queue;   /*  voice NI RX queue, PP sends packets to this q when no sesson
                                                        match/PP disable/PSM mode */
    PAL_Cppi4QueueHnd           voiceni_tx_queue;   /*  voice NI TX queue, voice NI  sends packets to this q destined 
                                                        for DSP*/
    PAL_Cppi4QueueHnd           dsp_rx_queue;       /* DSP RX queue PP  to DSP  */
    PAL_Cppi4QueueHnd           dsp_tx_queue;       /* DSP TX queue- DSP to PP*/

    int                         pid_hnd;            /* PID handle for DSP/voiceNI */
    int                         vpid_hnd;           /* VPID handle for DSP/voiceNI */

    PAL_Cppi4AccChHnd           acc_hnd;            /* handle to the accumulator channel */
    struct Cppi4EmbdDesc**      rxAcc_chan_list[VOICENI_ACC_RX_CH_COUNT];       /* Rx acc channel list(s) */
    Ptr                         rxAcc_chan_list_base[VOICENI_ACC_RX_CH_COUNT];  /* Rx acc channel lists base*/

    int rx_interrupt_count;
    int rx_bd_count;            /* number of packets received from PP egress queue */
    int tx_bd_count;            /* number of packets sent to PP ingress Q*/
    int rx_skb_count;           /* number of packets received from network interface */
    int buff_pop_failure;       /* number of times buffer pop failures */
    int bd_queue_pop_failure;   /* number of times bd queue pop failures */
    int rx_buffers_freed;       /* number of buffers freed */
    int rx_bd_with_virtBD_null;  /* number of times virtual desctiptor pointer is NULL */
    struct tasklet_struct       tx_tasklet;     /* Tx completion processing tasklet */ 
    struct net_device *         ptr_emtani_device; /* handle to MTANI network device */

    struct net_device_stats     stats;
    struct net_device*          ptr_device;
    VOICENI_NET_DEV_T net_dev;
   
}VOICENID_PRIVATE_T;

