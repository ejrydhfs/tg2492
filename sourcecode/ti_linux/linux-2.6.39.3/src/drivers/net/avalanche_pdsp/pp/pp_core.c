/*
  GPL LICENSE SUMMARY

  Copyright(c) 2017 Intel Corporation.

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
#include <asm-arm/arch-avalanche/generic/avalanche_pp_api.h>
#include <asm-arm/arch-avalanche/generic/avalanche_pdsp_api.h>
#include <asm-arm/arch-avalanche/puma6/puma6.h>
#include <asm-arm/arch-avalanche/puma6/puma6_pp.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/in.h>
#include "pp_db.h"
#include "pp_hal.h"
#include <pal.h>

/* ARRIS ADD START */
#ifndef CONFIG_MACH_PUMA5
#include <arris/arris_mod_api.h>

ARRISMOD_DEFINE( int, arris_rip_create_pp_session_hook_v2, AVALANCHE_PP_SESSION_INFO_t*, void* );
ARRISMOD_EXTERN( int, arris_rip_create_pp_session_hook_v2, AVALANCHE_PP_SESSION_INFO_t*, void* );
#endif
/* ARRIS ADD END */


static AVALANCHE_PP_RET_e    __avalanche_pp_pid_delete          ( Uint8     pid_handle );
static AVALANCHE_PP_RET_e    __avalanche_pp_vpid_delete         ( Uint8     vpid_handle );
static AVALANCHE_PP_RET_e    __avalanche_pp_session_delete      ( Uint32    session_handle,  AVALANCHE_PP_SESSION_STATS_t *  ptr_session_stats );
static AVALANCHE_PP_RET_e    __avalanche_pp_counter64_read      ( Uint64 *  dest,  volatile Ptr src );
static AVALANCHE_PP_RET_e    __avalanche_pp_flush_single_session( AVALANCHE_PP_SESSION_INFO_t *  ptr_session, Ptr     data );
static AVALANCHE_PP_RET_e    __avalanche_pp_set_session_STC_byte_msb_offset ( Uint32 session_handle);

AVALANCHE_PP_RET_e avalanche_pp_local_dev_addr(avalanche_pp_local_dev_addr_ioctl_params_t *param);

#define PP_DB_CHECK_ACTIVE_UNDER_LOCK()         \
    if (PP_DB.status != PP_DB_STATUS_ACTIVE)    \
    {                                           \
        printk("ERROR: PP Operation %s cannot be accomplished while PP status is %s\n", __FUNCTION__, PP_DB.status == PP_DB_STATUS_UNINITIALIZED ? "INACTIVE" : "PPM_PSM");     \
        PP_DB_UNLOCK();                         \
        return (PP_RC_FAILURE);                 \
    }                                           

#define PP_DB_CHECK_ACTIVE()            \
{                                       \
    PP_DB_LOCK();                       \
    PP_DB_CHECK_ACTIVE_UNDER_LOCK();    \
    PP_DB_UNLOCK();                     \
}

#define PRECMD_INDEX_SHIFT        0
#define PRECMD_COMMAND_SHIFT      8

#define PRECMD_INDEX_MASK         (0xFFu << PRECMD_INDEX_SHIFT)
#define PRECMD_COMMAND_MASK       (0xFFu << PRECMD_COMMAND_SHIFT)
#define PRECMD_COMMAND(x)         (((x) << PRECMD_COMMAND_SHIFT) & PRECMD_COMMAND_MASK)
#define PRECMD_INDEX(x)           (((x) << PRECMD_INDEX_SHIFT)& PRECMD_INDEX_MASK)

#define WORD_S0_B1_0(s0, b1, b0)  (((Uint16)s0 << 16) | ((Uint8)b1 <<  8)| ((Uint8)b0))

#define CLEAR_L4_FLAGS_FROM_LUT2_KEY(x)                 ((x) &= 0x4F)
#define NUMBER_OF_MILISECONDS_BEFORE_DDH_NOTIFY_EVENT	60000

/* ========================================================================================= */
/*                                                                                           */
/*                                                                                           */
/*                    EVENT POLL TIMER                                                       */
/*                                                                                           */
/*                                                                                           */
/* ========================================================================================= */
#define PP_TDOX_EVALUATION_PERIOD_IN_SEC           (1)
#define PP_TDOX_EVALUATION_ONCE_IN_X_TIMER         ( PP_TDOX_EVALUATION_PERIOD_IN_SEC * 1000 / gPpPollTimer.timer_poll_time_msec )    /* Divide by EVENT_POLLTIME_MSECS to get X */

typedef struct
{
    PAL_OsTimerHandle       timer_handle;
    Uint32                  timer_poll_time_msec;
    Uint32                  ddh_timer_poll_time_msec;
    Uint32                  ddh_timer_interval;
    Uint32                  ddh_timer_counter;
    Uint32                  ddh_notify_event_interval;  /* The number of timer ticks before notified the user that DDH state changed. */
    Uint32                  ddh_notify_event_counter;	/* Counter for the number of timer ticks before notified the user that DDH state changed. */
}
AVALANCHE_PP_TIMER_t;

static AVALANCHE_PP_TIMER_t     gPpPollTimer;

/**************************************************************************
 * FUNCTION NAME : void avalanche_pp_timer_handler ( Uint32 param )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is the dispatcher code that passes events
 *  to the registered event handler.
 **************************************************************************/
void    avalanche_pp_timer_handler( Uint32 param )
{
    static Uint32 evaluationIteration = 0;
    Uint16 eventIdxFW   = (Uint16)*(Uint32*)IO_PHY2VIRT(PP_HAL_EVENTS_PP_INDEX_BASE_PHY);
    Uint16 eventIdxHOST = (Uint16)*(Uint32*)IO_PHY2VIRT(PP_HAL_EVENTS_HOST_INDEX_BASE_PHY);

    /* DDH Event */
    gPpPollTimer.ddh_timer_counter++;
    if (gPpPollTimer.ddh_timer_counter == gPpPollTimer.ddh_timer_interval) 
    {
        gPpPollTimer.ddh_timer_counter = 0;
        avalanche_pp_event_report(PP_EV_DDH, 0, 0);
    }

    /* DDH Notify event */
    gPpPollTimer.ddh_notify_event_counter++;
	if (gPpPollTimer.ddh_notify_event_counter == gPpPollTimer.ddh_notify_event_interval)
    {
        avalanche_pp_event_report(PP_EV_DDH_NOTIFY, 0, 0);
        avalanche_pp_reset_ddh_notify_event_counter();
    }

    while( eventIdxHOST != eventIdxFW )
    {
        Uint16 eventCode;
        Uint32 eventAddress = IO_PHY2VIRT( PP_HAL_EVENTS_BASE_PHY ) + ( eventIdxHOST & 0xFFF );

        eventCode = *(Uint16*)(eventAddress);

        avalanche_pp_event_report( PP_EV_SESSION_EXPIRED, 
                                   (Uint32)(eventCode >> PP_HAL_EVENT_DATA_SHIFT) & PP_HAL_EVENT_DATA_MASK, 
                                   (Uint32)0 );
                
        eventIdxHOST += PP_HAL_EVENTS_ENTRY_SIZE_BYTES;
    }
    *(Uint32*) IO_PHY2VIRT(PP_HAL_EVENTS_HOST_INDEX_BASE_PHY) = eventIdxHOST;


    if (++evaluationIteration >= PP_TDOX_EVALUATION_ONCE_IN_X_TIMER)
    {
        /* TDOX sessions evaluation */
        avalanche_pp_event_report( PP_EV_MISC_TRIGGER_TDOX_EVALUATION, 0, 0 );
        evaluationIteration = 0;
    }

    PAL_osTimerStart( gPpPollTimer.timer_handle, gPpPollTimer.timer_poll_time_msec );
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_event_poll_timer_init( void )
 **************************************************************************
 * DESCRIPTION   :
 *  The function initiolaized the timer.
 * RETURNS       :
 *      0  -   Success
 *      1  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_event_poll_timer_init( void )
{
    gPpPollTimer.timer_poll_time_msec = 100;
    gPpPollTimer.ddh_timer_poll_time_msec = 1000;
    gPpPollTimer.ddh_timer_counter = 0;
    gPpPollTimer.ddh_timer_interval = gPpPollTimer.ddh_timer_poll_time_msec / gPpPollTimer.timer_poll_time_msec;
    gPpPollTimer.ddh_notify_event_counter = 0;
    gPpPollTimer.ddh_notify_event_interval = NUMBER_OF_MILISECONDS_BEFORE_DDH_NOTIFY_EVENT / gPpPollTimer.timer_poll_time_msec;

    if (PAL_osTimerCreate( avalanche_pp_timer_handler, &gPpPollTimer, &gPpPollTimer.timer_handle ))
    {
        return (PP_RC_FAILURE);
    }

    if (PAL_osTimerStart( gPpPollTimer.timer_handle, gPpPollTimer.timer_poll_time_msec ))
    {
        return (PP_RC_FAILURE);
    }

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_set_ddh_notify_event_interval(Uint32 number_of_miliseconds_for_interval)
 **************************************************************************
 * DESCRIPTION   :
 *  This function sets the interval before notify event will send to the user after defensive state change.
 *  param[in] number_of_miliseconds_for_interval - Number of milliseconds for the new notify interval.
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_set_ddh_notify_event_interval(Uint32 number_of_miliseconds_for_interval)
{
    if (number_of_miliseconds_for_interval < gPpPollTimer.timer_poll_time_msec)
    {
        /* Notification delay can't be less than the minimum unit of measure. */
        return (PP_RC_FAILURE);
    }

    gPpPollTimer.ddh_notify_event_interval = number_of_miliseconds_for_interval / (gPpPollTimer.timer_poll_time_msec * DDH_NOTIFICATIONS_NUM_OF_SAMPLING_WITHOUT_STATE_SWITCH);
    if (!gPpPollTimer.ddh_notify_event_interval)
    {
        /* Handle the dividing inaccuracy. */
        gPpPollTimer.ddh_notify_event_interval = 1;
    }

    /* Reset current notification timer */
	gPpPollTimer.ddh_notify_event_counter = 0;

    return (PP_RC_SUCCESS); 
}
/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_reset_ddh_notify_event_counter()
 **************************************************************************
 * DESCRIPTION   :
 *  This function reset the notify event counter.
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_reset_ddh_notify_event_counter()
{
    /* Reset current notification counter */
    gPpPollTimer.ddh_notify_event_counter = 0;

    return (PP_RC_SUCCESS); 
}

/* ========================================================================================= */

/* ******************************************************************** */
/*                                                                      */
/*                      ____ ___ ____                                   */
/*                     |  _ \_ _|  _ \                                  */
/*                     | |_) | || | | |                                 */
/*                     |  __/| || |_| |                                 */
/*                     |_|  |___|____/                                  */
/*                                                                      */
/*                                                                      */
/* ******************************************************************** */
    /* ****************************************** */
    /*                           _                */
    /*        ___ _ __ ___  __ _| |_ ___          */
    /*       / __| '__/ _ \/ _` | __/ _ \         */
    /*      | (__| | |  __/ (_| | ||  __/         */
    /*       \___|_|  \___|\__,_|\__\___|         */
    /*                                            */
    /* ****************************************** */

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_pid_create ( AVALANCHE_PP_PID_t * ptr_pid, void * ptr_netdev )
 **************************************************************************
 * DESCRIPTION   :
 *  The function uses the information passed to create a PID in the PP.
 *  param[in] ptr_pid - pointer to pid information
 *  param[in] ptr_netdev - pointer to network device information
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_pid_create            ( AVALANCHE_PP_PID_t * ptr_pid, void * ptr_netdev )
{
    PP_DB_PID_Entry_t *     ptr_pid_db;
    AVALANCHE_PP_RET_e      rc =  PP_RC_SUCCESS;

    if (ptr_pid->pid_handle >= AVALANCHE_PP_MAX_PID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    ptr_pid_db = &PP_DB.repository_PIDs[ ptr_pid->pid_handle ];

    PP_DB_LOCK();

    PP_DB_CHECK_ACTIVE_UNDER_LOCK();

    memcpy(&ptr_pid_db->pid, ptr_pid, sizeof(*ptr_pid)); 
    INIT_LIST_HEAD( &ptr_pid_db->pid_link_head );
    ptr_pid_db->status = PP_DB_STATUS_ACTIVE;
    PP_DB.stats.active_PIDs++;

    rc = pp_hal_pid_create( &ptr_pid_db->pid );

    PP_DB_UNLOCK();

    if (PP_RC_SUCCESS != rc)
    {
        __avalanche_pp_pid_delete( ptr_pid_db->pid.pid_handle );
        return (rc);
    }
    // Send event
    avalanche_pp_event_report( PP_EV_PID_CREATED, 
                               (Uint32)ptr_pid_db->pid.pid_handle, 
                               (Uint32)ptr_netdev );
    return (PP_RC_SUCCESS);
}
    /* ****************************************** */


    /* ****************************************** */
    /*           _      _      _                  */
    /*        __| | ___| | ___| |_ ___            */
    /*       / _` |/ _ \ |/ _ \ __/ _ \           */
    /*      | (_| |  __/ |  __/ ||  __/           */
    /*       \__,_|\___|_|\___|\__\___|           */
    /*                                            */
    /* ****************************************** */

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_pid_delete ( Uint8 pid_handle )
 **************************************************************************
 * DESCRIPTION   :
 *  The function deletes the PID in the PP.
 *  param[in] pid_handle - handle of pid to delete
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_pid_delete            ( Uint8     pid_handle )
{
    AVALANCHE_PP_RET_e      rc = __avalanche_pp_pid_delete( pid_handle );

    if (PP_RC_SUCCESS == rc)
    {
        // Send event
        avalanche_pp_event_report( PP_EV_PID_DELETED, (Uint32)pid_handle, 0 );
    }

    return (rc);
}

static AVALANCHE_PP_RET_e    __avalanche_pp_pid_delete          ( Uint8     pid_handle )
{
    PP_DB_PID_Entry_t *     ptr_pid_db;
    struct list_head *      pos = NULL;
    AVALANCHE_PP_RET_e      rc = PP_RC_SUCCESS;

    if (pid_handle >= AVALANCHE_PP_MAX_PID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    ptr_pid_db = &PP_DB.repository_PIDs[ pid_handle ];

    PP_DB_LOCK();
   
    PP_DB_CHECK_ACTIVE_UNDER_LOCK();

    // Go over all related VPIDs and delete them
    list_for_each( pos, &ptr_pid_db->pid_link_head )
    {
        PP_DB_VPID_Entry_t * entry;
        entry = list_entry(pos, PP_DB_VPID_Entry_t, pid_link);

        rc |= avalanche_pp_vpid_delete( entry->handle );
    }

    rc |= pp_hal_pid_delete( pid_handle );

    ptr_pid_db->status = PP_DB_STATUS_INITIALIZED;
    PP_DB.stats.active_PIDs--;

    PP_DB_UNLOCK();

    return (rc);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_pid_config_range ( AVALANCHE_PP_PID_RANGE_t * pid_range )
 **************************************************************************
 * DESCRIPTION   :
 *  The function uses the information passed to config PID range in the PDSP.
 *  param[in] pid_range - pointer to pid_range struct
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_pid_config_range      ( AVALANCHE_PP_PID_RANGE_t *    pid_range )
{
    PP_DB_CHECK_ACTIVE();
    return pp_hal_pid_range_create( pid_range );
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_pid_remove_range ( Uint32 port_num )
 **************************************************************************
 * DESCRIPTION   :
 *  The function uses the information passed to remove PID range in the PDSP.
 *  param[in] port_num - number of port to remove
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_pid_remove_range      ( Uint32    port_num )
{
    PP_DB_CHECK_ACTIVE_UNDER_LOCK();
    return pp_hal_pid_range_delete( port_num );
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_pid_set_flags ( Uint8 pid_handle, Uint32 new_flags )
 **************************************************************************
 * DESCRIPTION   :
 *  The function uses the information passed to modify the PID flags in the PP.
 *  param[in] pid_handle - handle of pid
 *  param[in] new_flags - new flags to set 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_pid_set_flags         ( Uint8     pid_handle,     Uint32  new_flags )
{
    PP_DB_PID_Entry_t *     ptr_pid_db;
    AVALANCHE_PP_RET_e      rc = PP_RC_SUCCESS;

    if (pid_handle >= AVALANCHE_PP_MAX_PID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    ptr_pid_db = &PP_DB.repository_PIDs[ pid_handle ];

    PP_DB_LOCK();

    PP_DB_CHECK_ACTIVE_UNDER_LOCK();

    ptr_pid_db->pid.priv_flags = (Uint8) new_flags;
    rc = pp_hal_pid_flags_set( &ptr_pid_db->pid );

    PP_DB_UNLOCK();

    return (rc);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_pid_get_list ( Uint8 * num_entries, AVALANCHE_PP_PID_t ** pid_list )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is used to get the PID list from PP DB and the number of active PID's
 *  param[in] num_entries - pointer to set number of active PID's
 *  param[in] pid_list - pointer to PID_type list 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_pid_get_list          ( Uint8 *   num_entries, AVALANCHE_PP_PID_t ** pid_list )
{
    Uint8   pid_handle = 0;

    PP_DB_LOCK();

    for (pid_handle = 0; pid_handle < AVALANCHE_PP_MAX_PID; pid_handle++)
    {
        if (PP_DB.repository_PIDs[ pid_handle ].status > PP_DB_STATUS_INITIALIZED )
        {
            *pid_list++ = &PP_DB.repository_PIDs[ pid_handle ].pid;
        }
    }

    *num_entries = (Uint8) PP_DB.stats.active_PIDs;

    PP_DB_UNLOCK();
    
    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_pid_get_info ( Uint8 pid_handle,  AVALANCHE_PP_PID_t ** ptr_pid )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is used to get the PID Information block given a handle.
 *  param[in] pid_handle - handle of PID
 *  param[in] ptr_pid - pointer to set PID info from DB 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_pid_get_info          ( Uint8     pid_handle,  AVALANCHE_PP_PID_t ** ptr_pid )
{
    if (pid_handle >= AVALANCHE_PP_MAX_PID)
    {
        return (PP_RC_INVALID_PARAM);
    }
    if (NULL == ptr_pid)
    {
        return (PP_RC_INVALID_PARAM);
    }

    *ptr_pid = &PP_DB.repository_PIDs[ pid_handle ].pid ;
    
    return (PP_RC_SUCCESS);
}



/* ******************************************************************** */
/*                                                                      */
/*                __     ______ ___ ____                                */
/*                \ \   / /  _ \_ _|  _ \                               */
/*                 \ \ / /| |_) | || | | |                              */
/*                  \ V / |  __/| || |_| |                              */
/*                   \_/  |_|  |___|____/                               */
/*                                                                      */
/*                                                                      */
/* ******************************************************************** */
    /* ****************************************** */
    /*                           _                */
    /*        ___ _ __ ___  __ _| |_ ___          */
    /*       / __| '__/ _ \/ _` | __/ _ \         */
    /*      | (__| | |  __/ (_| | ||  __/         */
    /*       \___|_|  \___|\__,_|\__\___|         */
    /*                                            */
    /* ****************************************** */

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_vpid_create ( AVALANCHE_PP_VPID_INFO_t * ptr_vpid )
 **************************************************************************
 * DESCRIPTION   :
 *  The function uses the information passed to create a VPID in the PP.
 *  param[in] ptr_vpid - pointer to VPIT information
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_vpid_create           ( AVALANCHE_PP_VPID_INFO_t *    ptr_vpid )
{
    PP_DB_VPID_Entry_t *        ptr_vpid_db;
    struct list_head *          pos = NULL;
    AVALANCHE_PP_RET_e          rc = PP_RC_SUCCESS;

    if (NULL == ptr_vpid)
    {
        return (PP_RC_INVALID_PARAM);
    }

    if (ptr_vpid->parent_pid_handle >= AVALANCHE_PP_MAX_PID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    PP_DB_LOCK();
    
    PP_DB_CHECK_ACTIVE_UNDER_LOCK();

    if (list_empty( &PP_DB.pool_VPIDs[ PP_DB_POOL_FREE ] ))
    {
        PP_DB_UNLOCK();
        return (PP_RC_OUT_OF_MEMORY);
    }

    pos = PP_DB.pool_VPIDs[ PP_DB_POOL_FREE ].next;

    ptr_vpid_db = list_entry( pos, PP_DB_VPID_Entry_t, link );

    list_move( pos, &PP_DB.pool_VPIDs[ PP_DB_POOL_BUSY ] );

    memcpy( &ptr_vpid_db->vpid, ptr_vpid, sizeof( ptr_vpid_db->vpid ) );

    ptr_vpid_db->vpid.vpid_handle   = ptr_vpid_db->handle;
    ptr_vpid->vpid_handle           = ptr_vpid_db->handle;

    PP_DB.stats.active_VPIDs++;

    list_add( &ptr_vpid_db->pid_link, &PP_DB.repository_PIDs[ ptr_vpid_db->vpid.parent_pid_handle ].pid_link_head );

    // HAL add VPID
    rc = pp_hal_vpid_create( &ptr_vpid_db->vpid );

    PP_DB_UNLOCK();

    if (PP_RC_SUCCESS != rc)
    {
        __avalanche_pp_vpid_delete( ptr_vpid_db->vpid.vpid_handle );
        return (rc);
    }

    // Send event
    avalanche_pp_event_report( PP_EV_VPID_CREATED, (Uint32)ptr_vpid_db->handle, 0 );

    return (PP_RC_SUCCESS);
}

    /* ****************************************** */
                           
    /* ****************************************** */
    /*           _      _      _                  */
    /*        __| | ___| | ___| |_ ___            */
    /*       / _` |/ _ \ |/ _ \ __/ _ \           */
    /*      | (_| |  __/ |  __/ ||  __/           */
    /*       \__,_|\___|_|\___|\__\___|           */
    /*                                            */
    /* ****************************************** */

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_vpid_delete ( Uint8 vpid_handle )
 **************************************************************************
 * DESCRIPTION   :
 *  The function deletes the VPID in the PP.
 *  param[in] vpid_handle - handle of VPID to delete 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_vpid_delete           ( Uint8     vpid_handle )
{
    AVALANCHE_PP_RET_e          rc = __avalanche_pp_vpid_delete( vpid_handle );

    if (PP_RC_SUCCESS == rc)
    {
        // Send event
        avalanche_pp_event_report( PP_EV_VPID_DELETED, (Uint32)vpid_handle, 0 );
    }

    return (rc);
}


static AVALANCHE_PP_RET_e    __avalanche_pp_vpid_delete         ( Uint8     vpid_handle )
{
    PP_DB_VPID_Entry_t *        ptr_vpid_db;
    PP_DB_Session_Entry_t *     entry;
    struct list_head *          pos = NULL;
    struct list_head *          nextPos = NULL;
    AVALANCHE_PP_RET_e          rc = PP_RC_SUCCESS;
    struct list_head *          head        = NULL;

    if (vpid_handle >= AVALANCHE_PP_MAX_VPID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    PP_DB_LOCK();

    PP_DB_CHECK_ACTIVE_UNDER_LOCK();
    
    ptr_vpid_db = &PP_DB.repository_VPIDs[ vpid_handle ];

    head = &ptr_vpid_db->list[ PP_LIST_ID_INGRESS ];
    for (pos = head->next; prefetch(pos->next), pos != (head); pos = nextPos)
    {
        nextPos = pos->next;
        entry = list_entry(pos, PP_DB_Session_Entry_t, list[ PP_LIST_ID_INGRESS ]);
        rc |= avalanche_pp_session_delete( entry->session_info.session_handle, NULL );
    }

    head = &ptr_vpid_db->list[ PP_LIST_ID_EGRESS ];
    for (pos = head->next; prefetch(pos->next), pos != (head); pos = nextPos)
    {
        nextPos = pos->next;
        entry = list_entry(pos, PP_DB_Session_Entry_t, list[ PP_LIST_ID_EGRESS ]);
        rc |= avalanche_pp_session_delete( entry->session_info.session_handle, NULL );
    }

    // HAL
    rc |= pp_hal_vpid_delete( vpid_handle );

    list_del_init( &ptr_vpid_db->pid_link );

    list_move( &ptr_vpid_db->link, &PP_DB.pool_VPIDs[ PP_DB_POOL_FREE ] );

    PP_DB.stats.active_VPIDs--;

    PP_DB_UNLOCK();

    return (rc);
}
    /* ****************************************** */


/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_vpid_set_flags ( Uint8 vpid_handle, Uint32 new_flags )
 **************************************************************************
 * DESCRIPTION   :
 *  The function uses the information passed to modify the VPID flags in the PP.
 *  param[in] vpid_handle - handle of VPID
 *  param[in] new_flags - new flags to set
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_vpid_set_flags        ( Uint8     vpid_handle,    Uint32  new_flags )
{
	AVALANCHE_PP_RET_e          rc = PP_RC_SUCCESS;
	
    if (vpid_handle >= AVALANCHE_PP_MAX_VPID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    PP_DB_LOCK();

    PP_DB_CHECK_ACTIVE_UNDER_LOCK();

    PP_DB.repository_VPIDs[ vpid_handle ].vpid.flags = (Uint8) new_flags;

    rc = pp_hal_vpid_flags_set( &PP_DB.repository_VPIDs[ vpid_handle ].vpid );

    PP_DB_UNLOCK();

    return (rc);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_vpid_get_list ( Uint8 parent_pid_handle, Uint8 * num_entries, AVALANCHE_PP_VPID_INFO_t ** vpid_list )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is used to get the VPID list from PP DB and the number of entries.
 *  param[in] parent_pid_handle - PID handle 
 *  param[in] num_entries - number of entries in list
 *  param[in] vpid_list - pointer to list head
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_vpid_get_list         ( Uint8     parent_pid_handle,  Uint8 *     num_entries, AVALANCHE_PP_VPID_INFO_t **    vpid_list )
{
    PP_DB_VPID_Entry_t *        ptr_vpid_db;
    struct list_head *          pos = NULL;
    Uint8                       count = 0;

    if (parent_pid_handle > AVALANCHE_PP_MAX_PID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    PP_DB_LOCK();

    if (parent_pid_handle == AVALANCHE_PP_MAX_PID)
    {
        list_for_each(pos, &PP_DB.pool_VPIDs[ PP_DB_POOL_BUSY ])
        {
            ptr_vpid_db = list_entry( pos, PP_DB_VPID_Entry_t, link );

            if (vpid_list)
            {
                *vpid_list++ = &ptr_vpid_db->vpid; 
            }
            count++;
        }
    }
    else
    {
        list_for_each(pos, &PP_DB.repository_PIDs[parent_pid_handle].pid_link_head)
        {
            ptr_vpid_db = list_entry( pos, PP_DB_VPID_Entry_t, pid_link );

            if (vpid_list)
            {
                *vpid_list++ = &ptr_vpid_db->vpid; 
            }
            count++;
        }
    }

    PP_DB_UNLOCK();

    *num_entries = count;

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_vpid_get_info ( Uint8 vpid_handle, AVALANCHE_PP_VPID_INFO_t ** ptr_vpid )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is used to get the VPID Information block given a handle.
 *  param[in] vpid_handle - handle of VPID
 *  param[in] ptr_vpid - pointer to set VPID info from DB 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_vpid_get_info         ( Uint8     vpid_handle,                                 AVALANCHE_PP_VPID_INFO_t **    ptr_vpid )
{
    if (vpid_handle >= AVALANCHE_PP_MAX_VPID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    if (NULL == ptr_vpid)
    {
        return (PP_RC_INVALID_PARAM);
    }

    *ptr_vpid = &PP_DB.repository_VPIDs[ vpid_handle ].vpid;

    return (PP_RC_SUCCESS);
}


/* ******************************************************************** */
/*                                                                      */
/*                 ____                _                                */
/*                / ___|  ___  ___ ___(_) ___  _ __                     */
/*                \___ \ / _ \/ __/ __| |/ _ \| '_ \                    */
/*                 ___) |  __/\__ \__ \ | (_) | | | |                   */
/*                |____/ \___||___/___/_|\___/|_| |_|                   */
/*                                                                      */
/*                                                                      */
/* ******************************************************************** */
    /* ****************************************** */
    /*                           _                */
    /*        ___ _ __ ___  __ _| |_ ___          */
    /*       / __| '__/ _ \/ _` | __/ _ \         */
    /*      | (__| | |  __/ (_| | ||  __/         */
    /*       \___|_|  \___|\__,_|\__\___|         */
    /*                                            */
    /* ****************************************** */

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_session_create ( AVALANCHE_PP_SESSION_INFO_t * ptr_session, void * pkt_ptr )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is used to create a session.
 *  param[in] ptr_session - pointer to session information
 *  param[in] pkt_ptr - 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_session_create        ( AVALANCHE_PP_SESSION_INFO_t *  ptr_session, void * pkt_ptr )
{
    PP_DB_Session_Entry_t              *ptr_session_db;
    PP_DB_Session_LUT1_hash_entry_t    *hash_entry_1 = NULL;
    PP_DB_Session_LUT2_hash_entry_t    *hash_entry_2 = NULL;
    PP_DB_Entry_t                      *tdox_entry = NULL;
    struct list_head                   *pos = NULL;
    AVALANCHE_PP_RET_e                  rc = PP_RC_SUCCESS;
    AVALANCHE_PP_RET_e                  rc_drop_list = PP_RC_FAILURE; 
    Uint32                              hash_LUT1;
    Uint32                              hash_LUT2;
    Bool                                create_LUT1 = False;
	Bool                                is_multi_drop_session = False;
    Bool                                legal_ip_type = False;
    avalanche_pp_local_dev_addr_ioctl_params_t drop_list_param;
    __Avalanche_PP_LUTs_Data_t          full_classification_lookup;
    __Avalanche_PP_LUTs_Data_t         *full_classification_lookup_ptr = &ptr_session->ingress.lookup;

    PP_DB_CHECK_ACTIVE();

	if (ptr_session->egress.drop_sess == AVALANCHE_PP_EGRESS_DROP_SESS)
	{
		/* We are going to create a drop session. If multi drop mode enable, create multi drop session. */
		if (PP_DB.defensive_state.multi_drop_enabled)
		{
			is_multi_drop_session = True;

			/* The new session is multidrop session - reset L4 in LUT2 key. */
			ptr_session->ingress.lookup.LUT2.u.fields.L4_SrcPort = 0; 
			ptr_session->ingress.lookup.LUT2.u.fields.L4_DstPort = 0;
			CLEAR_L4_FLAGS_FROM_LUT2_KEY(ptr_session->ingress.lookup.LUT2.u.fields.enable_flags);
		}
    }
    else if (ptr_session->ingress.flags & AVALANCHE_PP_INGRESS_SESSEION_PROPERTY_FLAGS_DO_L2_CLASSIFICATION) 
    {
		/* We Change classification under the following conditions:
		 *   - Not a drop session
		 *   - L2 classification is enabled
		 *   - L2 classification for all pids / specific pid which match the ingress pid
		 */ 
        /* Save the full lookup classification for the record flag */
        memcpy(&full_classification_lookup, full_classification_lookup_ptr, sizeof(ptr_session->ingress.lookup));
        full_classification_lookup_ptr = &full_classification_lookup;    
        ptr_session->ingress.lookup.LUT1.u.fields.L3.entry_type = 0;
        ptr_session->ingress.lookup.LUT1.u.fields.L3.ip_protocol = 0;
        ptr_session->ingress.lookup.LUT1.u.fields.L3.PPPoE_session_id = 0;
        memset(&ptr_session->ingress.lookup.LUT2, 0, (sizeof(ptr_session->ingress.lookup.LUT2.u.fields.WAN_addr_IP) + sizeof(ptr_session->ingress.lookup.LUT2.u.fields.IP))); 
        ptr_session->ingress.lookup.LUT1.u.fields.L3.entry_type = AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED;
        ptr_session->ingress.lookup.LUT1.u.fields.L3.enable_flags = AVALANCHE_PP_LUT1_FIELD_ENABLE_L3_ENTRY_TYPE;
        ptr_session->ingress.lookup.LUT2.u.fields.entry_type = AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED;
        ptr_session->ingress.lookup.LUT2.u.fields.enable_flags = ptr_session->ingress.lookup.LUT2.u.fields.enable_flags & (AVALANCHE_PP_LUT2_FIELD_ENABLE_1ST_VLAN | AVALANCHE_PP_LUT2_FIELD_ENABLE_2ND_VLAN);
        ptr_session->ingress.lookup.LUT2.u.fields.TOS = 0;                   
    }

    hash_LUT1 = 0x000000FF & pp_db_hash( (Uint8*)&ptr_session->ingress.lookup.LUT1, sizeof(ptr_session->ingress.lookup.LUT1), 0, AVALANCHE_PP_MAX_LUT1_KEYS);

    PP_DB_LOCK();

#ifndef CONFIG_MACH_PUMA5
    /* ARRIS ADD START */    
    ARRISMOD_CALL( arris_rip_create_pp_session_hook_v2, ptr_session, pkt_ptr );
    /* ARRIS ADD END */
#endif

    /*----------------------------------------------------------------------------------------------*/
    /*             LUT1 Search ...                                                                  */
    /*----------------------------------------------------------------------------------------------*/
    list_for_each( pos, &PP_DB.lut1_hash[ hash_LUT1 ] )
    {
        hash_entry_1=   list_entry( pos, PP_DB_Session_LUT1_hash_entry_t, lut1_hash_link );

        if ( memcmp( &hash_entry_1->lut1_data, &ptr_session->ingress.lookup.LUT1, sizeof(ptr_session->ingress.lookup.LUT1) ) )
        {
            hash_entry_1 = NULL;
        }
        else
        {
            // entry found ...
            break;
        }
    }

    if ( NULL == hash_entry_1 )
    {
        if (list_empty( &PP_DB.pool_lut1[ ptr_session->session_pool ][ PP_DB_POOL_FREE ] ))
        {
            PP_DB.stats.lut1_starvation++;
            PP_DB_UNLOCK();
            return (PP_RC_OUT_OF_MEMORY);
        }

        if (list_empty( &PP_DB.pool_lut2[ ptr_session->session_pool ][ PP_DB_POOL_FREE ] ))
        {
            PP_DB.stats.lut2_starvation++;
            PP_DB_UNLOCK();
            return (PP_RC_OUT_OF_MEMORY);
        }

        create_LUT1 = True;
        PP_DB.stats.active_lut1_keys++;
        if (PP_DB.stats.active_lut1_keys > PP_DB.stats.max_active_lut1_keys)
        {
            PP_DB.stats.max_active_lut1_keys = PP_DB.stats.active_lut1_keys;
        }

        pos = PP_DB.pool_lut1[ ptr_session->session_pool ][ PP_DB_POOL_FREE ].next;

        hash_entry_1 = list_entry( pos, PP_DB_Session_LUT1_hash_entry_t, link );

        list_move( pos, &PP_DB.pool_lut1[ ptr_session->session_pool ][ PP_DB_POOL_BUSY ] );

        list_add( &hash_entry_1->lut1_hash_link, &PP_DB.lut1_hash[ hash_LUT1 ] );

        memcpy( &hash_entry_1->lut1_data, &ptr_session->ingress.lookup.LUT1, sizeof(ptr_session->ingress.lookup.LUT1) );
    }
    /*------------------------------------------------------------------------------------------------*/

    ptr_session->ingress.lookup.LUT2.u.fields.LUT1_key = hash_entry_1->handle;

    /* TBD - Change AVALANCHE_PP_MAX_LUT1_KEYS value to AVALANCHE_PP_MAX_ACCELERATED_SESSIONS */
    hash_LUT2 = 0x000007FF & pp_db_hash( (Uint8*)&ptr_session->ingress.lookup.LUT2, sizeof(ptr_session->ingress.lookup.LUT2), 0, AVALANCHE_PP_MAX_ACCELERATED_SESSIONS);

    /*----------------------------------------------------------------------------------------------*/
    /*             LUT2 Search ...                                                                  */
    /*----------------------------------------------------------------------------------------------*/
    list_for_each( pos, &PP_DB.lut2_hash[ hash_LUT2 ] )
    {
        hash_entry_2=   list_entry( pos, PP_DB_Session_LUT2_hash_entry_t, lut2_hash_link );

        if ( memcmp( &PP_DB.repository_sessions[ hash_entry_2->handle ].session_info.ingress.lookup.LUT2, 
                     &ptr_session->ingress.lookup.LUT2, 
                     sizeof(ptr_session->ingress.lookup.LUT2) ) )
        {
            hash_entry_2 = NULL;
        }
        else
        {
            // entry found ...
            break;
        }
    }

    if ( NULL == hash_entry_2 )
    {
        if (list_empty( &PP_DB.pool_lut2[ ptr_session->session_pool ][ PP_DB_POOL_FREE ] ))
        {
            PP_DB.stats.lut2_starvation++;
            PP_DB_UNLOCK();
            return (PP_RC_OUT_OF_MEMORY);
        }

        /* ======================== */
        if ( (ptr_session->egress.vpid_handle < AVALANCHE_PP_MAX_VPID) && (PP_DB.repository_VPIDs[ ptr_session->egress.vpid_handle ].session_pre_action_cb) )
        {
            AVALANCHE_EXEC_HOOK_FN_t cb = (AVALANCHE_EXEC_HOOK_FN_t)PP_DB.repository_VPIDs[ ptr_session->egress.vpid_handle ].session_pre_action_cb;

            cb( ptr_session, PP_DB.repository_VPIDs[ ptr_session->egress.vpid_handle ].session_pre_data );
        }
        /* ======================== */

        pos = PP_DB.pool_lut2[ ptr_session->session_pool ][ PP_DB_POOL_FREE ].next;

        hash_entry_2 = list_entry( pos, PP_DB_Session_LUT2_hash_entry_t, link );

        list_move( pos, &PP_DB.pool_lut2[ ptr_session->session_pool ][ PP_DB_POOL_BUSY ] );

        list_add( &hash_entry_2->lut2_hash_link, &PP_DB.lut2_hash[ hash_LUT2 ] );

        hash_entry_2->lut1_hash_entry_ptr = hash_entry_1;
        hash_entry_1->refCount++;

        memcpy( &PP_DB.repository_sessions[ hash_entry_2->handle ].session_info, 
                ptr_session, 
                sizeof(*ptr_session) );

        PP_DB.repository_sessions[ hash_entry_2->handle ].lut2_hash_entry_ptr = hash_entry_2;

        PP_DB.repository_sessions[ hash_entry_2->handle ].session_info.session_handle   = hash_entry_2->handle;
        ptr_session->session_handle                                                     = hash_entry_2->handle;

        PP_DB.stats.active_sessions++;
        if (PP_DB.stats.active_sessions > PP_DB.stats.max_active_sessions)
        {
            PP_DB.stats.max_active_sessions = PP_DB.stats.active_sessions;
        }

        if (ptr_session->egress.pid_type == AVALANCHE_PP_PID_TYPE_DOCSIS) 
        {
            PP_DB.stats.active_us_sessions++;
        }
        
        list_add( &PP_DB.repository_sessions[ hash_entry_2->handle            ].list[ PP_LIST_ID_EGRESS ],
                  &PP_DB.repository_VPIDs   [ ptr_session->egress.vpid_handle ].list[ PP_LIST_ID_EGRESS ] );

        list_add( &PP_DB.repository_sessions[ hash_entry_2->handle             ].list[ PP_LIST_ID_INGRESS ],
                  &PP_DB.repository_VPIDs   [ ptr_session->ingress.vpid_handle ].list[ PP_LIST_ID_INGRESS ] );

        if ((IPPROTO_TCP == ptr_session->egress.ip_protocol) || (IPPROTO_TCP == ptr_session->egress.inner_ip_protocol))
        {
            list_add( &PP_DB.repository_sessions[ hash_entry_2->handle            ].list[ PP_LIST_ID_EGRESS_TCP ],
                      &PP_DB.repository_VPIDs   [ ptr_session->egress.vpid_handle ].list[ PP_LIST_ID_EGRESS_TCP ] );
        }

        if ((IPPROTO_UDP == ptr_session->egress.ip_protocol) || (IPPROTO_UDP == ptr_session->egress.inner_ip_protocol))
        {
            Uint8  docsis_vpid_handle;
            bool  isDocsis = false;

            /* Check if egress Vpid is Docsis */
            if (ptr_session->egress.pid_type == AVALANCHE_PP_PID_TYPE_DOCSIS)
            {
                docsis_vpid_handle = ptr_session->egress.vpid_handle;
                isDocsis = true;
            }
            else if (ptr_session->ingress.pid_type == AVALANCHE_PP_PID_TYPE_DOCSIS) 
            {
                docsis_vpid_handle = ptr_session->ingress.vpid_handle;
                isDocsis = true;
            }

            if (isDocsis)
            {
                list_add(&PP_DB.repository_sessions[hash_entry_2->handle].list[PP_LIST_ID_EGRESS_UDP],
                         &PP_DB.repository_VPIDs[docsis_vpid_handle].list[PP_LIST_ID_EGRESS_UDP]);
            }
        }

        ptr_session_db = &PP_DB.repository_sessions[ hash_entry_2->handle ];

        /* Copy drop indication to ptr_session_db */
        ptr_session_db->session_info.egress.drop_sess = ptr_session->egress.drop_sess;

        /*----------------------------------------------------------------------------------------------*/
        /*             TDOX allocation  ...                                                             */
        /*----------------------------------------------------------------------------------------------*/
        if ( AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED & ptr_session->egress.enable )
        {
            if (list_empty( &PP_DB.pool_TDOX[ PP_DB_POOL_FREE ] ))
            {
                ptr_session->egress.enable &= ~AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED;
                ptr_session_db->session_info.egress.enable &= ~AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED;
                PP_DB.stats.tdox_starvation++;
            }
            else
            {
                pos = PP_DB.pool_TDOX[ PP_DB_POOL_FREE ].next;

                tdox_entry = list_entry( pos, PP_DB_Entry_t, link );


                list_move( pos, &PP_DB.pool_TDOX[ PP_DB_POOL_BUSY ] );

                ptr_session_db->session_info.egress.tdox_handle = tdox_entry->handle;
                ptr_session->egress.tdox_handle                 = tdox_entry->handle;
                ptr_session_db->session_info.priority           = 0;
                ptr_session->priority                           = 0;

                list_add( &PP_DB.repository_sessions[ hash_entry_2->handle            ].list[ PP_LIST_ID_EGRESS_TDOX ],
                          &PP_DB.repository_VPIDs   [ ptr_session->egress.vpid_handle ].list[ PP_LIST_ID_EGRESS_TDOX ] );
            }
        }
        /*----------------------------------------------------------------------------------------------*/
    }
    else
    {
        // session exist
        ptr_session->session_handle = hash_entry_2->handle;

        PP_DB_UNLOCK();
        return (PP_RC_OBJECT_EXIST);
    }
    /*------------------------------------------------------------------------------------------------*/

	/* Count active drop sessions */
    if (ptr_session->egress.drop_sess == AVALANCHE_PP_EGRESS_DROP_SESS)
    {
        PP_DB.stats.active_drop_sessions++;

        if (is_multi_drop_session)
        {
            drop_list_param.op_type = ADD_ADDR;
            
            /* Add multi drop session to blacklist */
            if (AVALANCHE_PP_LUT_ENTRY_L3_IPV6 == ptr_session->ingress.lookup.LUT2.u.fields.entry_type) 
            {
                drop_list_param.addr_type = MULTI_DROP_IPV6_ADDR;
                memcpy(drop_list_param.u.ipv6, ptr_session->ingress.lookup.LUT2.u.fields.WAN_addr_IP.v6, 16); 
                legal_ip_type = true;
            }
            else if (AVALANCHE_PP_LUT_ENTRY_L3_IPV4 == ptr_session->ingress.lookup.LUT2.u.fields.entry_type)
            {
                drop_list_param.addr_type = MULTI_DROP_IPV4_ADDR;
                drop_list_param.u.ipv4 =  ptr_session->ingress.lookup.LUT2.u.fields.WAN_addr_IP.v4;
                legal_ip_type = true;
            }

            if (legal_ip_type)
            {
                rc_drop_list = avalanche_pp_local_dev_addr(&drop_list_param); 
            }

            if (((rc_drop_list == PP_RC_SUCCESS) || (rc_drop_list == PP_RC_OBJECT_EXIST)))
            {
                    PP_DB.stats.active_multi_drop_sessions++;
                    ptr_session->egress.enable = ptr_session->egress.enable | AVALANCHE_PP_EGRESS_MULTI_DROP_SESSION;
            }
            else
            {
                 /* Multi drop session create fails - do not create session */
                 printk("%s %d:\t\tMultidrop session { %d } create failed\n", __FUNCTION__, __LINE__, ptr_session->session_handle);
                 __avalanche_pp_session_delete( ptr_session_db->session_info.session_handle, NULL );

                 PP_DB_UNLOCK();
                 return (PP_RC_FAILURE); 
             }
          }
        }
    
    /* Copy egress.enable flags to session_db */
    ptr_session_db->session_info.egress.enable |= ptr_session->egress.enable;

    rc = pp_hal_session_create( &ptr_session_db->session_info, create_LUT1, full_classification_lookup_ptr);

    /* ======================== */
    if ( (ptr_session->egress.vpid_handle < AVALANCHE_PP_MAX_VPID) && (PP_DB.repository_VPIDs[ ptr_session->egress.vpid_handle ].session_post_action_cb) )
    {
        AVALANCHE_EXEC_HOOK_FN_t cb = (AVALANCHE_EXEC_HOOK_FN_t)PP_DB.repository_VPIDs[ ptr_session->egress.vpid_handle ].session_post_action_cb;

        cb( ptr_session, PP_DB.repository_VPIDs[ ptr_session->egress.vpid_handle ].session_post_data );
    }
    /* ======================== */

	/* Update the Session byte MSB of the 64B Statistical counter */
    __avalanche_pp_set_session_STC_byte_msb_offset(ptr_session_db->session_info.session_handle);


    if (rc != PP_RC_SUCCESS)
    {
        __avalanche_pp_session_delete( ptr_session_db->session_info.session_handle, NULL );

        PP_DB_UNLOCK();

        return (rc);
    }

    if (create_LUT1)
    {
        PP_DB.stats.lut1_histogram[(PP_DB.stats.active_lut1_keys - 1) / AVALANCHE_PP_LUT1_HISTOGRAM_RESOLUTION]++;
    }
    PP_DB.stats.lut2_histogram[(PP_DB.stats.active_sessions - 1) / AVALANCHE_PP_LUT2_HISTOGRAM_RESOLUTION]++;

    PP_DB_UNLOCK();

    // Send event
    avalanche_pp_event_report( PP_EV_SESSION_CREATED, (Uint32)hash_entry_2->handle, (Uint32)pkt_ptr );

    return (rc);
}

    /* ****************************************** */
    /*           _      _      _                  */
    /*        __| | ___| | ___| |_ ___            */
    /*       / _` |/ _ \ |/ _ \ __/ _ \           */
    /*      | (_| |  __/ |  __/ ||  __/           */
    /*       \__,_|\___|_|\___|\__\___|           */
    /*                                            */
    /* ****************************************** */

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_session_delete ( Uint32 session_handle, AVALANCHE_PP_SESSION_STATS_t * ptr_session_stats )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is used to delete the session.
 *  param[in] session_handle - handle of session to delete
 *  param[in] ptr_session_stats - pointer to session statistics
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_session_delete        ( Uint32    session_handle,     AVALANCHE_PP_SESSION_STATS_t *  ptr_session_stats )
{
    AVALANCHE_PP_RET_e      rc = __avalanche_pp_session_delete( session_handle, ptr_session_stats );

    if (PP_RC_SUCCESS == rc)
    {
        AVALANCHE_PP_SESSION_STATS_t    session_stats;

        avalanche_pp_get_stats_session   ( session_handle,  &session_stats );

        if (ptr_session_stats)
        {
            memcpy( ptr_session_stats, &session_stats, sizeof(session_stats) );
        }

        // Send event
        avalanche_pp_event_report( PP_EV_SESSION_DELETED, (Uint32)session_handle, (Uint32)&session_stats );
    }

    return (rc);
}

static AVALANCHE_PP_RET_e    __avalanche_pp_session_delete      ( Uint32    session_handle,     AVALANCHE_PP_SESSION_STATS_t *  ptr_session_stats )
{
    PP_DB_Session_Entry_t *             session_db_ptr = NULL;
    AVALANCHE_PP_RET_e                  rc = PP_RC_SUCCESS;
    Bool                                delete_LUT1 = False;
    PP_LIST_ID_e                        j;
    avalanche_pp_local_dev_addr_ioctl_params_t drop_list_param;
    Bool legal_ip_type = False;

    if (AVALANCHE_PP_MAX_ACCELERATED_SESSIONS <= session_handle)
    {
        return (PP_RC_INVALID_PARAM);
    }

    PP_DB_LOCK();
    
    PP_DB_CHECK_ACTIVE_UNDER_LOCK();

    session_db_ptr = &PP_DB.repository_sessions[ session_handle ];

    if (NULL == session_db_ptr->lut2_hash_entry_ptr)
    {
        printk(" Session %d has already been deleted\n", session_handle );
        return (PP_RC_INVALID_PARAM);
    }

    for (j=PP_LIST_ID_INGRESS; j<PP_LIST_ID_ALL; j++)
    {
        if (!list_empty( &session_db_ptr->list[ j ] ))
        {
            list_del_init( &session_db_ptr->list[ j ] );
        }
    }

    if (0 == --session_db_ptr->lut2_hash_entry_ptr->lut1_hash_entry_ptr->refCount)
    {
        list_del_init( &session_db_ptr->lut2_hash_entry_ptr->lut1_hash_entry_ptr->lut1_hash_link );

        list_move( &session_db_ptr->lut2_hash_entry_ptr->lut1_hash_entry_ptr->link, 
                   &PP_DB.pool_lut1[ session_db_ptr->session_info.session_pool ][ PP_DB_POOL_FREE ] );

        session_db_ptr->lut2_hash_entry_ptr->lut1_hash_entry_ptr = NULL;

        delete_LUT1 = True;
        PP_DB.stats.active_lut1_keys--;
    }

    list_del_init( &session_db_ptr->lut2_hash_entry_ptr->lut2_hash_link );

    list_move( &session_db_ptr->lut2_hash_entry_ptr->link, &PP_DB.pool_lut2[ session_db_ptr->session_info.session_pool ][ PP_DB_POOL_FREE ] );

    session_db_ptr->lut2_hash_entry_ptr = NULL;

    if(session_db_ptr->session_info.egress.drop_sess == AVALANCHE_PP_EGRESS_DROP_SESS)
    {
        PP_DB.stats.active_drop_sessions--;

		if(session_db_ptr->session_info.egress.enable & AVALANCHE_PP_EGRESS_MULTI_DROP_SESSION)
        {
            drop_list_param.op_type = RM_ADDR;
            if (AVALANCHE_PP_LUT_ENTRY_L3_IPV6 == session_db_ptr->session_info.ingress.lookup.LUT2.u.fields.entry_type) 
            {    
                drop_list_param.addr_type = MULTI_DROP_IPV6_ADDR;
                memcpy(drop_list_param.u.ipv6, session_db_ptr->session_info.ingress.lookup.LUT2.u.fields.WAN_addr_IP.v6, 16);
                legal_ip_type = True; 
            }
            else if (AVALANCHE_PP_LUT_ENTRY_L3_IPV4 == session_db_ptr->session_info.ingress.lookup.LUT2.u.fields.entry_type)
            {
                drop_list_param.addr_type = MULTI_DROP_IPV4_ADDR;
                drop_list_param.u.ipv4 = session_db_ptr->session_info.ingress.lookup.LUT2.u.fields.WAN_addr_IP.v4;
                legal_ip_type = True;
            }
            if ((legal_ip_type) && (avalanche_pp_local_dev_addr(&drop_list_param) == PP_RC_SUCCESS))
            {
                /* Count active multi drop sessions */
                PP_DB.stats.active_multi_drop_sessions--;
            }            
        }
    }

    if (session_db_ptr->session_info.egress.pid_type == AVALANCHE_PP_PID_TYPE_DOCSIS)
    {
        PP_DB.stats.active_us_sessions--;
    }

    PP_DB.stats.active_sessions--;

    if ( AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED & session_db_ptr->session_info.egress.enable )
    {
        list_move( &PP_DB.repository_TDOX[ session_db_ptr->session_info.egress.tdox_handle ].link, 
                   &PP_DB.pool_TDOX[ PP_DB_POOL_FREE ] );

        session_db_ptr->session_info.egress.enable &= ~AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED;
    }

    rc = pp_hal_session_delete( &session_db_ptr->session_info, delete_LUT1 );

    PP_DB_UNLOCK();

   return (rc);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_session_get_list ( Uint8 vpid_handle, PP_LIST_ID_e list_id, Uint32 * num_entries, Uint32 * session_handle_list )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is used to get the sessions list from LUT2 PP DB and the number of entries.
 *  param[in] vpid_handle - handle of the VPID for sessions
 *  param[in] list_id - ingress / egress / tcp / tdox
 *  param[in] num_entries - number of entries in list
 *  param[in] session_handle_list - pointer to list head
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_session_get_list      ( Uint8     vpid_handle,    PP_LIST_ID_e   list_id, Uint32 * num_entries, Uint32 * session_handle_list )
{
    struct list_head *          pos             = NULL;
    PP_DB_VPID_Entry_t *        vpid_db_ptr     = NULL;
    PP_DB_Session_Entry_t *     session_db_ptr  = NULL;

    if (vpid_handle > AVALANCHE_PP_MAX_VPID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    if (list_id > PP_LIST_ID_ALL)
    {
        return (PP_RC_INVALID_PARAM);
    }

    if (NULL == num_entries)
    {
        return (PP_RC_INVALID_PARAM);
    }

    PP_DB_LOCK();

    *num_entries = 0;

    if (vpid_handle == AVALANCHE_PP_MAX_VPID)
    {
        PP_DB_Session_LUT2_hash_entry_t *       entry;
        int pool = 0;

        for (; pool < AVALANCHE_PP_SESSIONS_POOL_MAX; pool++)
        {
            list_for_each(pos, &PP_DB.pool_lut2[ pool ][ PP_DB_POOL_BUSY ])
            {
                entry = list_entry( pos, PP_DB_Session_LUT2_hash_entry_t, link );
                if (session_handle_list)
                {
                    *session_handle_list++ = ( entry->handle );
                }
                (*num_entries)++;
            }
        }
    }
    else
    {
        PP_LIST_ID_e    list_start  = list_id;
        PP_LIST_ID_e    list_end    = list_id;

        if ( list_id == PP_LIST_ID_ALL )
        {
            list_start = PP_LIST_ID_INGRESS;
            list_end   = PP_LIST_ID_EGRESS;
        }

        for (list_id = list_start; list_id <= list_end; list_id++)
        {
            vpid_db_ptr = &PP_DB.repository_VPIDs[vpid_handle]; 

            list_for_each( pos, &vpid_db_ptr->list[ list_id ] )
            {
                session_db_ptr = list_entry( pos, PP_DB_Session_Entry_t, list[ list_id ] );
                if (session_handle_list)
                {
                    *session_handle_list++ = (session_db_ptr->session_info.session_handle);
                }
                (*num_entries)++;
            }
        }
    }

    PP_DB_UNLOCK();

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_session_get_info ( Uint32 session_handle, AVALANCHE_PP_SESSION_INFO_t** ptr_session_info )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is used to get the session information from a session handle.
 *  param[in] session_handle - handle of the session
 *  param[in] ptr_session_info - pointer to set session info
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_session_get_info      ( Uint32    session_handle,     AVALANCHE_PP_SESSION_INFO_t**   ptr_session_info )
{
    if (AVALANCHE_PP_MAX_ACCELERATED_SESSIONS <= session_handle)
    {
        return (PP_RC_INVALID_PARAM);
    }

    if (NULL == ptr_session_info)
    {
        return (PP_RC_INVALID_PARAM);
    }

    *ptr_session_info = &PP_DB.repository_sessions[session_handle].session_info; 

    if (PP_DB.repository_sessions[session_handle].lut2_hash_entry_ptr)
    {
        return (PP_RC_SUCCESS);
    }
    
    return (PP_RC_INVALID_PARAM);
}

static AVALANCHE_PP_RET_e   __avalanche_pp_flush_single_session( AVALANCHE_PP_SESSION_INFO_t *  ptr_session, Ptr     data )
{
    avalanche_pp_session_delete( ptr_session->session_handle, NULL );
    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_flush_sessions ( Uint8 vpid_handle, PP_LIST_ID_e list_id )
 **************************************************************************
 * DESCRIPTION   :
 *  The function flushes sessions from the session database for a VPID.
 *  param[in] vpid_handle - handle of the VPID
 *  param[in] list_id - ingress / egress / tcp / tdox 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_flush_sessions        ( Uint8     vpid_handle, PP_LIST_ID_e  list_id )
{
    PP_DB_CHECK_ACTIVE();

    if (vpid_handle > AVALANCHE_PP_MAX_VPID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    if (list_id > PP_LIST_ID_ALL)
    {
        return (PP_RC_INVALID_PARAM);
    }

    if (vpid_handle == AVALANCHE_PP_MAX_VPID)
    {
        // TBD
        avalanche_pp_session_list_execute( vpid_handle, PP_LIST_ID_ALL, __avalanche_pp_flush_single_session, NULL );
    }
    else
    {
        avalanche_pp_session_list_execute( vpid_handle, list_id,        __avalanche_pp_flush_single_session, NULL );
    }

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_enable_psm (void)
 **************************************************************************
 * DESCRIPTION   :
 *  This function is called to enable Power Saving mode (PSM)
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_enable_psm (void)
{     
    Int32               rc;
    Int32               enablePsm;
    pdsp_cmd_params_t   pdsp_cmd;
    Int32               reg;
    Int32*              regAddr;

    PP_DB_CHECK_ACTIVE();

   /* Configure Prefetcher command */
    pdsp_cmd.pdsp_id    = PDSP_ID_Prefetcher;
    pdsp_cmd.cmd        = PRECMD_INDEX(2) | PRECMD_COMMAND(PDSP_ENABLE_PREFETCH);
    pdsp_cmd.params[0]  = 0;
    pdsp_cmd.params_len = 0;

    printk("%s: Enable prefetcher PSM mode\n", __FUNCTION__);

    if ((rc = pdsp_cmd_send( pdsp_cmd.pdsp_id, pdsp_cmd.cmd, NULL, 0, pdsp_cmd.params, 1 )) && (rc != -8))
    {
        printk(KERN_ERR"\n%s: failed to put command(%X) to the PDSP %d rc(%d)\n", __FUNCTION__, pdsp_cmd.cmd, pdsp_cmd.pdsp_id, rc );
        return (PP_RC_FAILURE);
    }

    
    /* Power down PP PDSPs using clock gating */
    printk("%s: Halt PP PDSPs\n", __FUNCTION__);

    /* Send PSM enable Cmd to PP */
    pdsp_cmd.pdsp_id = PDSP_ID_Classifier;
    pdsp_cmd.cmd = WORD_S0_B1_0 (0x01, 0x0, PDSP_SETPSM);
    pdsp_cmd.params[0] = 0;   
    pdsp_cmd.params_len =0;

    if (PP_RC_SUCCESS != (rc = pdsp_cmd_send( pdsp_cmd.pdsp_id, pdsp_cmd.cmd, NULL, 0, pdsp_cmd.params, 0 )))
    {
        printk(KERN_ERR"\n%s: failed to put command(%X) to the PDSP %d rc(%d)\n", __FUNCTION__, pdsp_cmd.cmd, pdsp_cmd.pdsp_id, rc );
        return (PP_RC_FAILURE);
    }

    /* unSet the enable bit in all PP PDSPs */
    enablePsm = 1;
    if (PP_RC_SUCCESS != (rc = pdsp_control(PDSP_ID_Classifier, PDSPCTRL_HLT, (Ptr)&enablePsm)))
    {
        printk(KERN_ERR"\n%s: failed to set PP in PSM mode rc(%d)\n", __FUNCTION__,  rc );
        return (PP_RC_FAILURE);
    }

    if (PAL_osTimerStop( gPpPollTimer.timer_handle ))
    {
        return (PP_RC_FAILURE);
    }

    regAddr = (int*)AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_REG;
    reg = *regAddr;
    reg &= ~(AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_CPDSP1 |
             AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_LUT1   |
             AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_CPDSP2 |
             AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_LUT2   |
             AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_MPDSP  |
             AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_McDMA3);
    *regAddr = reg;


    PP_DB_LOCK();
    PP_DB.status = PP_DB_STATUS_PSM;
    PP_DB_UNLOCK();

    return PP_RC_SUCCESS;
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_disable_psm (void)
 **************************************************************************
 * DESCRIPTION   :
 *  This function is called to disable Power Saving mode (PSM)
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_disable_psm (void)
{
    Int32               rc;
    pdsp_cmd_params_t   pdsp_cmd;
    Int32               enablePsm;
    Int32               reg;
    Int32*              regAddr;

    PP_DB_LOCK();
    if (PP_DB.status != PP_DB_STATUS_PSM)
    {
        printk("ERROR: PP Operation %s cannot be accomplished while PP status is %d\n", __FUNCTION__, PP_DB.status);
        PP_DB_UNLOCK();
        return PP_RC_FAILURE;
    }
    PP_DB_UNLOCK();

    /* Power up PP PDSPs using clock gating */
    printk("%s: Run PP PDSPs\n", __FUNCTION__);
    regAddr = (int*)AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_REG;
    reg = *regAddr;
    reg |= (AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_CPDSP1 |
            AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_LUT1   |
            AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_CPDSP2 |
             AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_LUT2  |
            AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_MPDSP  |
            AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_McDMA3);
    *regAddr = reg;

    /* Disable prefetcher PSM */
   /* Configure Prefetcher command */
    pdsp_cmd.pdsp_id    = PDSP_ID_Prefetcher;
    pdsp_cmd.cmd        = PRECMD_INDEX(1) | PRECMD_COMMAND(PDSP_ENABLE_PREFETCH);
    pdsp_cmd.params[0]  = 0;
    pdsp_cmd.params_len = 0;

    printk("%s: Disable prefetcher PSM mode\n", __FUNCTION__);

    if ((rc = pdsp_cmd_send( pdsp_cmd.pdsp_id, pdsp_cmd.cmd, NULL, 0, pdsp_cmd.params, 1 )) && (rc != -8))
    {
        printk(KERN_ERR"\n%s: failed to put command(%X) to the PDSP %d rc(%d)\n", __FUNCTION__, pdsp_cmd.cmd, pdsp_cmd.pdsp_id, rc );
        return (PP_RC_FAILURE);
    }

    printk("%s: Disable PSM mode\n", __FUNCTION__);
    
    /* Set PP to run */
    enablePsm = 0;
    if (PP_RC_SUCCESS != (rc = pdsp_control( 1, PDSPCTRL_RESUME, (Ptr)&enablePsm )))
    {
        printk(KERN_ERR"\n%s: failed to disable PP PSM mode rc(%d)\n", __FUNCTION__, rc );
        return (PP_RC_FAILURE);
    }


    /* Send PSM disable Cmd to PP */
    pdsp_cmd.pdsp_id = PDSP_ID_Classifier;
    pdsp_cmd.cmd = WORD_S0_B1_0 (0x00, 0x0, PDSP_SETPSM);
    pdsp_cmd.params[0] = 0;   
    pdsp_cmd.params_len =0;

    if (PP_RC_SUCCESS != (rc = pdsp_cmd_send( pdsp_cmd.pdsp_id, pdsp_cmd.cmd, pdsp_cmd.params, 0, pdsp_cmd.params, 0 )))
    {
        printk(KERN_ERR"\n%s: failed to put command(%X) to the PDSP %d rc(%d)\n", __FUNCTION__, pdsp_cmd.cmd, pdsp_cmd.pdsp_id, rc );
        return (PP_RC_FAILURE);
    }

    if (PAL_osTimerStart( gPpPollTimer.timer_handle, gPpPollTimer.timer_poll_time_msec ))
    {
        return (PP_RC_FAILURE);
    }


    PP_DB_LOCK();
    PP_DB.status = PP_DB_STATUS_ACTIVE;
    PP_DB_UNLOCK();

    return PP_RC_SUCCESS;
  
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_psm ( Uint8 onOff )
 **************************************************************************
 * DESCRIPTION   :
 *  This function is called to disable / enable Power Saving mode (PSM)
 *  param[in] onOff - "1" = disable "0" = enable
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_psm        ( Uint8     onOff )
{
    if (onOff)
    {
        avalanche_pp_disable_psm();    
    }
    else
    {
        /* flush all sessions */
        avalanche_pp_flush_sessions( AVALANCHE_PP_MAX_VPID, PP_LIST_ID_ALL );
        avalanche_pp_enable_psm();

    }
    
    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_set_ack_suppression ( Uint8 enDis)
 **************************************************************************
 * DESCRIPTION   :
 *  The function sets the packet processor to do Ack Suppression or not to
 *  do in case Tdox is Enabled.
 *  param[in] enDis - "1" = Ack Suppression disable "0" = Ack Suppression enable
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_set_ack_suppression  ( Uint8    enDis)
{
    Int32               rc;
    pdsp_cmd_params_t   pdsp_cmd;

    /* Send ack_suppression disable/enable Cmd to PP */
    pdsp_cmd.pdsp_id = PDSP_ID_Classifier;
    pdsp_cmd.cmd = WORD_S0_B1_0 (enDis, 0x0, PDSP_SET_ACK_SUPP);
    pdsp_cmd.params[0] = 0;   
    pdsp_cmd.params_len =0;

    if (PP_RC_SUCCESS != (rc = pdsp_cmd_send( pdsp_cmd.pdsp_id, pdsp_cmd.cmd, pdsp_cmd.params, 0, pdsp_cmd.params, 0 )))
    {
        printk(KERN_ERR"\n%s: failed to put command(%X) to the PDSP %d rc(%d)\n", __FUNCTION__, pdsp_cmd.cmd, pdsp_cmd.pdsp_id, rc );
        return (PP_RC_FAILURE);
    }
    
    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_set_proxy_wait_byte_for_tx ( Uint16 waitBytes)
 **************************************************************************
 * DESCRIPTION   :
 *  The function sets the proxy pdsp wait for tx udma threshold in bytes
 *  param[in] waitBytes - wait for tx udma threshold in bytes
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_set_proxy_wait_byte_for_tx  ( Uint16    waitBytes)
{
    Int32               rc;
    pdsp_cmd_params_t   pdsp_cmd;

    /* Send ack_suppression disable/enable Cmd to PP */
    pdsp_cmd.pdsp_id = PDSP_ID_LAN_Proxy;
    pdsp_cmd.cmd = WORD_S0_B1_0 (waitBytes, 0x0, PDSP_SET_PRX_WAIT_THR);
    pdsp_cmd.params[0] = 0;   
    pdsp_cmd.params_len =0;

    if (PP_RC_SUCCESS != (rc = pdsp_cmd_send( pdsp_cmd.pdsp_id, pdsp_cmd.cmd, pdsp_cmd.params, 0, pdsp_cmd.params, 0 )))
    {
        printk(KERN_ERR"\n%s: failed to put command(%X) to the PDSP %d rc(%d)\n", __FUNCTION__, pdsp_cmd.cmd, pdsp_cmd.pdsp_id, rc );
        return (PP_RC_FAILURE);
    }
    
    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_set_traffic_prioritization_mode ( Uint8 priorityMode)
 **************************************************************************
 * DESCRIPTION   :
 *  The function sets the packet processor to do traffic prioritization according to:
 *  TOS fiels only / VLAN priority field only / The highest priority between both.
 *  param[in] priority - "0" = both - default "1" = TOS only "2" = VLAN only
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_set_traffic_prioritization_mode  ( Uint8    priorityMode)
{
    pp_hal_set_traffic_prioritization_mode( priorityMode );
    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_hw_init (void)
 **************************************************************************
 * DESCRIPTION   :
 *  The function initiolazed packet processor hw
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_hw_init        (void)
{
    int reg;

    /* Set clock to PP peripherals (Only those that are used) */
     reg = AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_PPDSP             |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_CPDSP1            |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_CPDSP2         	|
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_MPDSP             |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_QPDSP             |
          // AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_UsPrefPDSP     |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_PrefSharedRAM     |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_SessSharedRAM     |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_MiscSharedRam     |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_Counters          |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_CDMA0             |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_CDMA1             |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_CDMA2             |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_CDMA3             |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_McDMA0            |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_McDMA1            |
          // AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_McDMA2         |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_McDMA3            |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_LUT1              |
          AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_LUT2;           	//|
          // AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_LUT3);
    *(Uint32*)AVALANCHE_NWSS_GENERAL_MAILBOX_CLK_CTRL_REG = reg;

    memset( (void *)IO_PHY2VIRT(0x03200000), 0,  0x8040 );
    memset( (void *)IO_PHY2VIRT(0x03300000), 0, 0x2D000 );
    memset( (void *)IO_PHY2VIRT(0x03400300), 0,  0x1200 );


    return (PP_RC_SUCCESS);
}





/* Advanced HOOKS */
/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_session_list_execute ( Uint8 vpid_handle, PP_LIST_ID_e list_id, AVALANCHE_EXEC_HOOK_FN_t handler, Ptr data )
 **************************************************************************
 * DESCRIPTION   :
 *  The function destroys the sessions according to specified criteria
 *  param[in] vpid_handle - handle of the VPID
 *  param[in] list_id - ingress / egress / tcp / tdox
 *  param[in] handler - 
 *  param[in] data - 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_session_list_execute( Uint8     vpid_handle, PP_LIST_ID_e   list_id,    AVALANCHE_EXEC_HOOK_FN_t   handler, Ptr  data )
{
    PP_DB_Session_Entry_t *             session_db_ptr  = NULL;
    PP_DB_Session_LUT2_hash_entry_t *   lut2_hash_ptr   = NULL;
    struct list_head *                  pos         = NULL;
    struct list_head *                  pos_next    = NULL;
    struct list_head *                  head        = NULL;

    if (vpid_handle > AVALANCHE_PP_MAX_VPID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    if (list_id > PP_LIST_ID_ALL)
    {
        return (PP_RC_INVALID_PARAM);
    }

    if (NULL == handler)
    {
        return (PP_RC_INVALID_PARAM);
    }

    PP_DB_LOCK();
    {
        if ( AVALANCHE_PP_MAX_VPID == vpid_handle )
        {
            AVALANCHE_PP_SESSIONS_POOL_ID_e     pool;

            for ( pool = 0; pool < AVALANCHE_PP_SESSIONS_POOL_MAX; pool++)
            {
                head = &PP_DB.pool_lut2[ pool ][ PP_DB_POOL_BUSY ]; 

                for (pos = head->next; prefetch(pos->next), pos != (head); pos = pos_next)
                {
                    pos_next = pos->next;
                    lut2_hash_ptr = list_entry( pos, PP_DB_Session_LUT2_hash_entry_t, link );
                    handler( &PP_DB.repository_sessions[ lut2_hash_ptr->handle ].session_info, data );
                }
            }
        }
        else
        {
            PP_LIST_ID_e    list_start  = list_id;
            PP_LIST_ID_e    list_end    = list_id;

            if ( list_id == PP_LIST_ID_ALL )
            {
                list_start = PP_LIST_ID_INGRESS;
                list_end   = PP_LIST_ID_EGRESS;
            }

            for (list_id = list_start; list_id <= list_end; list_id++)
            {
                head = &PP_DB.repository_VPIDs[ vpid_handle ].list[ list_id ];

                for (pos = head->next; prefetch(pos->next), pos != (head); pos = pos_next)
                {
                    pos_next = pos->next;
                    session_db_ptr = list_entry( pos, PP_DB_Session_Entry_t, list[ list_id ] );
                    handler( &session_db_ptr->session_info, data );
                }
            }
        }
    }
    PP_DB_UNLOCK();

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_session_pre_action_bind ( Uint8 vpid_handle, AVALANCHE_EXEC_HOOK_FN_t handler, Ptr data )
 **************************************************************************
 * DESCRIPTION   :
 *  param[in] vpid_handle - handle of the VPID
 *  param[in] handler - 
 *  param[in] data - 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_session_pre_action_bind   ( Uint8     vpid_handle,  AVALANCHE_EXEC_HOOK_FN_t  handler, Ptr   data )
{
    if (vpid_handle >= AVALANCHE_PP_MAX_VPID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    PP_DB_LOCK();
    PP_DB.repository_VPIDs[ vpid_handle ].session_pre_action_cb = handler;
    PP_DB.repository_VPIDs[ vpid_handle ].session_pre_data      = data;
    PP_DB_UNLOCK();

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_session_post_action_bind ( Uint8 vpid_handle, AVALANCHE_EXEC_HOOK_FN_t handler, Ptr data )
 **************************************************************************
 * DESCRIPTION   :
 *  param[in] vpid_handle - handle of the VPID
 *  param[in] handler - 
 *  param[in] data - 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_session_post_action_bind  ( Uint8     vpid_handle,  AVALANCHE_EXEC_HOOK_FN_t  handler, Ptr   data )
{
    if (vpid_handle >= AVALANCHE_PP_MAX_VPID)
    {
        return (PP_RC_INVALID_PARAM);
    }

    PP_DB_LOCK();
    PP_DB.repository_VPIDs[ vpid_handle ].session_post_action_cb = handler;
    PP_DB.repository_VPIDs[ vpid_handle ].session_post_data      = data;
    PP_DB_UNLOCK();

    return (PP_RC_SUCCESS);
}

static AVALANCHE_PP_RET_e  __avalanche_pp_counter64_read( Uint64 * dest, volatile Ptr src )
{
    union
    {
        Uint64  lll;
        struct
        {
            Uint32  high;
            Uint32  low;
        }s;
    }
    counter64;

    
    counter64.s.high = *(Uint32 *)(src + 4);
    counter64.s.low  = *(Uint32 *)(src);

    if(counter64.s.high != *(Uint32 *)(src + 4))
    {
        counter64.s.high = *(Uint32 *)(src + 4);
        counter64.s.low  = *(Uint32 *)(src);
    }

    *dest = counter64.lll;

    return (PP_RC_SUCCESS);
}

/* Statistics API */
/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_stats_session ( Uint32 session_handle, AVALANCHE_PP_SESSION_STATS_t* ptr_session_stats )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is called to get the statistics of a particular session.
 *  param[in] session_handle - handle of the session
 *  param[in] ptr_session_stats - pointer to session statistics struct
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_get_stats_session   ( Uint32 session_handle,    AVALANCHE_PP_SESSION_STATS_t*    ptr_session_stats )
{
    volatile Uint32 *    stats_rgn;
    volatile Uint32 *    msb_offset;
    union
    {
        Uint64  dw;
        struct
        {
            Uint32  w1;
            Uint32  w0;
        }s;
    }bytes_forwarded;


    PP_DB_LOCK();

    if (PP_DB.status != PP_DB_STATUS_ACTIVE)
    {
        if (PP_DB.status == PP_DB_STATUS_PSM)
        {
            /* In PSM mode we do not fail the operation but do not inquire the PP since it is down */
            PP_DB_UNLOCK();
            return PP_RC_SUCCESS;
        }
        else
        {
            printk("ERROR: PP Operation %s cannot be accomplished while PP status is %s\n", __FUNCTION__,  "INACTIVE"); 
            PP_DB_UNLOCK();
            return PP_RC_FAILURE;
        }
    }

    PP_DB_UNLOCK();

    stats_rgn = (Uint32 *)IO_PHY2VIRT( PP_HAL_COUNTERS_SESSION_PKTS_BASE_PHY );

    ptr_session_stats->packets_forwarded = stats_rgn[ session_handle ];

    stats_rgn = (Uint32 *)IO_PHY2VIRT( PP_HAL_COUNTERS_SESSION_BYTES_BASE_PHY );

    __avalanche_pp_counter64_read( &(bytes_forwarded.dw), &stats_rgn[ session_handle * 2 ] );

    /* Reading the MSB of the session byte counter that was in session creation since it needs to be ignored */
    msb_offset = (Uint32 *)IO_PHY2VIRT (PP_HAL_STC_MSB_TBL_BASE_PHY);

    bytes_forwarded.s.w1 -= msb_offset[session_handle];

    ptr_session_stats->bytes_forwarded = bytes_forwarded.dw;


    return (PP_RC_SUCCESS);
}

AVALANCHE_PP_RET_e   __avalanche_pp_set_session_STC_byte_msb_offset ( Uint32 session_handle)
{
    AVALANCHE_PP_SESSION_STATS_t    session_stats;
    volatile Uint32 *    stats_rgn;
    volatile Uint32 *    msb_offset;
    union
    {
        Uint64  dw;
        struct
        {
            Uint32  w1;
            Uint32  w0;
        }s;
    }bytes_forwarded;
    

    stats_rgn = (Uint32 *)IO_PHY2VIRT( PP_HAL_COUNTERS_SESSION_BYTES_BASE_PHY );

    __avalanche_pp_counter64_read( &(bytes_forwarded.dw), &stats_rgn[ session_handle * 2 ] );
	
	bytes_forwarded.s.w0 = (0 - bytes_forwarded.s.w0);
	/* Reset the 32 Bit LSB in the counter to 0 */
	stats_rgn[ session_handle * 2 ] = bytes_forwarded.s.w0;

    __avalanche_pp_counter64_read( &(bytes_forwarded.dw), &stats_rgn[ session_handle * 2 ] );

    /* Saving the MSB, in order to ignore it, when reading the session bytes counter */
    msb_offset = (Uint32 *)IO_PHY2VIRT (PP_HAL_STC_MSB_TBL_BASE_PHY);
   
    msb_offset[session_handle] = bytes_forwarded.s.w1;   

    return (PP_RC_SUCCESS);
}


/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_stats_vpid ( Uint8 vpid_handle, AVALANCHE_PP_VPID_STATS_t* ptr_vpid_stats )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is called to get the statistics of a particular VPID.
 *  param[in] vpid_handle - handle of the VPID
 *  param[in] ptr_vpid_stats - pointer to VPID statistics struct
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_get_stats_vpid      ( Uint8  vpid_handle,       AVALANCHE_PP_VPID_STATS_t*       ptr_vpid_stats )
{
    volatile Uint32 *    stats_rgn_64 = (IO_PHY2VIRT( PP_HAL_COUNTERS_VPID_64BITS_BASE_PHY ) + vpid_handle * PP_COUNTERS_VPID_64BITS_ENTRY_SIZE);
    volatile Uint32 *    stats_rgn_32 = (IO_PHY2VIRT( PP_HAL_COUNTERS_VPID_32BITS_BASE_PHY ) + vpid_handle * PP_COUNTERS_VPID_32BITS_ENTRY_SIZE);

    PP_DB_LOCK();
    if (PP_DB.status != PP_DB_STATUS_ACTIVE)
    {
        if (PP_DB.status == PP_DB_STATUS_UNINITIALIZED)
        {
            printk("ERROR: PP Operation %s cannot be accomplished while PP status is %s\n", __FUNCTION__,  "INACTIVE"); 
            PP_DB_UNLOCK();
            return PP_RC_FAILURE;
        }
        else
        {
            /* In PSM mode we do not fail the operation but do not inquire the PP since it is down */
            PP_DB_UNLOCK();
            return PP_RC_SUCCESS;
        }
    }
    PP_DB_UNLOCK();

    __avalanche_pp_counter64_read( &ptr_vpid_stats->rx_byte, &stats_rgn_64[ PP_COUNTERS_VPID_64BITS_RX_BYTES_LSB_OFF/4 ] );

    ptr_vpid_stats->rx_unicast_pkt   = stats_rgn_64[ PP_COUNTERS_VPID_64BITS_RX_UCAST_PKTS_LSB_OFF/4 ];
    ptr_vpid_stats->rx_broadcast_pkt = stats_rgn_32[ PP_COUNTERS_VPID_32BITS_RX_BCAST_PKTS_OFF    /4 ];
    ptr_vpid_stats->rx_multicast_pkt = stats_rgn_32[ PP_COUNTERS_VPID_32BITS_RX_MCAST_PKTS_OFF    /4 ];
    ptr_vpid_stats->rx_discard_pkt   = stats_rgn_32[ PP_COUNTERS_VPID_32BITS_RX_DISCARDS_OFF      /4 ];


    __avalanche_pp_counter64_read( &ptr_vpid_stats->tx_byte, &stats_rgn_64[ PP_COUNTERS_VPID_64BITS_TX_BYTES_LSB_OFF/4 ] );

    ptr_vpid_stats->tx_unicast_pkt   = stats_rgn_64[ PP_COUNTERS_VPID_64BITS_TX_UCAST_PKTS_LSB_OFF/4 ];
    ptr_vpid_stats->tx_broadcast_pkt = stats_rgn_32[ PP_COUNTERS_VPID_32BITS_TX_BCAST_PKTS_OFF    /4 ];
    ptr_vpid_stats->tx_multicast_pkt = stats_rgn_32[ PP_COUNTERS_VPID_32BITS_TX_MCAST_PKTS_OFF    /4 ];
    ptr_vpid_stats->tx_discard_pkt   = stats_rgn_32[ PP_COUNTERS_VPID_32BITS_TX_DISCARDS_OFF      /4 ];

    ptr_vpid_stats->tx_error         = stats_rgn_32[ PP_COUNTERS_VPID_32BITS_TX_ERRORS_OFF        /4 ];

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_stats_global ( AVALANCHE_PP_GLOBAL_STATS_t* ptr_stats )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is called to get the statistics of the packet processor.
 *  param[in] ptr_stats - pointer to global statistics struct
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_get_stats_global    (                           AVALANCHE_PP_GLOBAL_STATS_t*     ptr_stats )
{
    PP_DB_LOCK();
    if (PP_DB.status != PP_DB_STATUS_ACTIVE)
    {
        if (PP_DB.status == PP_DB_STATUS_UNINITIALIZED)
        {
            printk("ERROR: PP Operation %s cannot be accomplished while PP status is %s\n", __FUNCTION__,  "INACTIVE"); 
            PP_DB_UNLOCK();
            return PP_RC_FAILURE;
        }
        else
        {
            /* In PSM mode we do not fail the operation but do not inquire the PP since it is down */
            PP_DB_UNLOCK();
            return PP_RC_SUCCESS;
        }
    }
    PP_DB_UNLOCK();

    ptr_stats->ppdsp_rx_pkts                = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_PPDSP_BASE_PHY) + PP_COUNTERS_PPDSP_RX_PKTS_OFF                );
    ptr_stats->ppdsp_pkts_frwrd_to_cpdsp1   = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_PPDSP_BASE_PHY) + PP_COUNTERS_PPDSP_PKTS_FRWRD_TO_CPDSP1_OFF   );
    ptr_stats->ppdsp_not_enough_descriptors = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_PPDSP_BASE_PHY) + PP_COUNTERS_PPDSP_NOT_ENOUGH_DESCRIPTORS_OFF );

    ptr_stats->cpdsp1_rx_pkts               = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_CPDSP1_BASE_PHY) + PP_COUNTERS_CPDSP1_RX_PKTS_OFF              );
    ptr_stats->cpdsp1_lut1_search_attempts  = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_CPDSP1_BASE_PHY) + PP_COUNTERS_CPDSP1_LUT1_SEARCHES_OFF        );
    ptr_stats->cpdsp1_lut1_matches          = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_CPDSP1_BASE_PHY) + PP_COUNTERS_CPDSP1_LUT1_MATCHES_OFF         );
    ptr_stats->cpdsp1_pkts_frwrd_to_cpdsp2  = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_CPDSP1_BASE_PHY) + PP_COUNTERS_CPDSP1_PKTS_FRWRD_TO_CPDSP2_OFF );
                                                         
    ptr_stats->cpdsp2_rx_pkts               = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_CPDSP2_BASE_PHY) + PP_COUNTERS_CPDSP2_RX_PKTS_OFF              );
    ptr_stats->cpdsp2_lut2_search_attempts  = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_CPDSP2_BASE_PHY) + PP_COUNTERS_CPDSP2_LUT2_SEARCHES_OFF        );
    ptr_stats->cpdsp2_lut2_matches          = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_CPDSP2_BASE_PHY) + PP_COUNTERS_CPDSP2_LUT2_MATCHES_OFF         );
    ptr_stats->cpdsp2_pkts_frwrd_to_mpdsp   = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_CPDSP2_BASE_PHY) + PP_COUNTERS_CPDSP2_PKTS_FRWRD_TO_MPDSP_OFF  );
    ptr_stats->cpdsp2_synch_timeout_events  = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_CPDSP2_BASE_PHY) + PP_COUNTERS_CPDSP2_SYNCH_TIMEOUT_EVENTS_OFF );
    ptr_stats->cpdsp2_reassembly_db_full    = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_CPDSP2_BASE_PHY) + PP_COUNTERS_CPDSP2_REASSEMBLY_DB_FULL       );
    ptr_stats->cpdsp2_reassembly_db_timeout = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_CPDSP2_BASE_PHY) + PP_COUNTERS_CPDSP2_REASSEMBLY_DB_TIMEOUT    );
                                                         
    ptr_stats->mpdsp_rx_pkts                = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_MPDSP_BASE_PHY) + PP_COUNTERS_MPDSP_PKTS_RECEIVED              );
    ptr_stats->mpdsp_ipv4_rx_pkts           = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_MPDSP_BASE_PHY) + PP_COUNTERS_MPDSP_IPV4_PKTS_RECEIVED         );
    ptr_stats->mpdsp_ipv6_rx_pkts           = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_MPDSP_BASE_PHY) + PP_COUNTERS_MPDSP_IPV6_PKTS_RECEIVED         );
    ptr_stats->mpdsp_frwrd_to_host          = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_MPDSP_BASE_PHY) + PP_COUNTERS_MPDSP_PKTS_FRWRD_TO_HOST         );
    ptr_stats->mpdsp_frwrd_to_qpdsp         = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_MPDSP_BASE_PHY) + PP_COUNTERS_MPDSP_PKTS_FRWRD_TO_QPDSP        );
    ptr_stats->mpdsp_frwrd_to_synch_q       = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_MPDSP_BASE_PHY) + PP_COUNTERS_MPDSP_PKTS_FRWRD_TO_SYNCHQ       );
    ptr_stats->mpdsp_discards               = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_MPDSP_BASE_PHY) + PP_COUNTERS_MPDSP_DISCARDS                   );
    ptr_stats->mpdsp_synchq_overflow_events = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_MPDSP_BASE_PHY) + PP_COUNTERS_MPDSP_SYNCH_OVERFLOW_EVENTS      );

    ptr_stats->prxpdsp_pkts_popped_from_In_queues = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_PRXPDSP_BASE_PHY) + PP_COUNTERS_PRXPDSP_PKTS_POPPED_FROM_IN_QUEUES );
    ptr_stats->prxpdsp_pkts_forward_to_L2switch   = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_PRXPDSP_BASE_PHY) + PP_COUNTERS_PRXPDSP_PKTS_FORWARDED_TO_L2_SWITCH);
    ptr_stats->prxpdsp_pkts_fromL2switch          = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_PRXPDSP_BASE_PHY) + PP_COUNTERS_PRXPDSP_PKTS_FROM_L2_SWITCH        );
    ptr_stats->prxpdsp_pkts_pushed_to_Prefetcher  = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_PRXPDSP_BASE_PHY) + PP_COUNTERS_PRXPDSP_PKTS_PUSHED_TO_PREFETCHER  );
    ptr_stats->prxpdsp_Not_enough_buffers         = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_PRXPDSP_BASE_PHY) + PP_COUNTERS_PRXPDSP_NOT_ENOUGH_BUFFERS         );
    ptr_stats->prxpdsp_Not_enough_Descriptors     = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_PRXPDSP_BASE_PHY) + PP_COUNTERS_PRXPDSP_NOT_ENOUGH_DESCRIPTORS     );
    ptr_stats->prxpdsp_pkts_to_small_for_padding  = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_PRXPDSP_BASE_PHY) + PP_COUNTERS_PRXPDSP_PKTS_TO_SMALL_FOR_PADDING  );

    ptr_stats->qpdsp_ooo_discards           = *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_CPDSP2_BASE_PHY) + PP_COUNTERS_QPDSP_OOO_DISCARDS_OFF      );

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_event_handler_register ( Uint32 * handle_event_handler, AVALANCHE_EVENT_HANDLER_t handler )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is the dispatcher code that passes events from PP
 *  entities to the registered event handler.
 *  param[in] handle_event_handler - pointer to event handle 
 *  param[in] handler - hendler structure
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_event_handler_register     ( Uint32 *  handle_event_handler, AVALANCHE_EVENT_HANDLER_t   handler )
{
    PP_DB_EventHandler_Entry_t * entry;

    if ((NULL == handler) || (NULL == handle_event_handler))
    {
        return (PP_RC_INVALID_PARAM);
    }

    entry = kmalloc( sizeof(PP_DB_EventHandler_Entry_t), GFP_KERNEL );

    if (NULL == entry)
    {
        return (PP_RC_OUT_OF_MEMORY);
    }

    entry->handler = handler;

    PP_DB_LOCK();

    list_add( &entry->link, &PP_DB.eventHandlers );

    PP_DB_UNLOCK();

    *handle_event_handler = (Uint32)entry;

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_event_handler_unregister ( Uint32 handle_event_handler )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is the dispatcher code that unregistered event handler.
 *  param[in] handle_event_handler - event handler handle
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_event_handler_unregister   ( Uint32    handle_event_handler )
{
    PP_DB_EventHandler_Entry_t * entry = (PP_DB_EventHandler_Entry_t *)handle_event_handler;

    if (NULL == entry)
    {
        return (PP_RC_INVALID_PARAM);
    }

    PP_DB_LOCK();

    list_del( &entry->link );

    PP_DB_UNLOCK();

    kfree( entry );

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_event_report( AVALANCHE_PP_EVENT_e event, Uint32 param1, Uint32 param2 )
 **************************************************************************
 * DESCRIPTION   :
 *  param[in] event -  
 *  param[in] param1 - 
 *  param[in] param2 - 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_event_report( AVALANCHE_PP_EVENT_e  event, Uint32 param1, Uint32 param2 )
{
    struct list_head *   pos = NULL;
    AVALANCHE_PP_RET_e  rc = PP_RC_SUCCESS;

    list_for_each( pos, &PP_DB.eventHandlers )
    {
        PP_DB_EventHandler_Entry_t * entry;
        entry = list_entry(pos, PP_DB_EventHandler_Entry_t, link);

        rc |= entry->handler( event, param1, param2 );
    }

    return (rc);
}



/* ******************************************************************** */
/*                                                                      */
/*                       ___       ____                                 */
/*                      / _ \  ___/ ___|                                */
/*                     | | | |/ _ \___ \                                */
/*                     | |_| | (_) |__) |                               */
/*                      \__\_\\___/____/                                */
/*                                                                      */
/*                                                                      */
/* ******************************************************************** */

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_qos_cluster_setup( Uint8 clst_indx, AVALANCHE_PP_QOS_CLST_CFG_t* clst_cfg )
 **************************************************************************
 * DESCRIPTION   :
 *  This function is called to setup a QoS cluster in PP.
 *  param[in] clst_indx - cluster id 
 *  param[in] clst_cfg - pointer to cluster configuration struct
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_qos_cluster_setup( Uint8     clst_indx,  AVALANCHE_PP_QOS_CLST_CFG_t*    clst_cfg )
{
    Int32   qos_q_cnt;
    Uint8   egr_q_cnt;
    Uint16  egr_q[ AVALANCHE_PP_QOS_CLST_MAX_EGRESS_QCNT ];

    if ((clst_cfg == NULL) || (clst_indx > AVALANCHE_PP_QOS_CLST_MAX_INDX) || (clst_cfg->qos_q_cnt > AVALANCHE_PP_QOS_CLST_MAX_QCNT)) 
    {
        return PP_RC_INVALID_PARAM;
    }

    PP_DB_CHECK_ACTIVE();

    /* Egress queues are configured in the cluster structure only for none-GRR clusters*/
    if (!(clst_cfg->flags & PP_QOS_CLUSTER_TYPE_GRR))
    {
        memset(egr_q, 0, sizeof(egr_q));

        /* Find out the Egress queues superset ... */
        for (qos_q_cnt = 0, egr_q_cnt = 0; qos_q_cnt < clst_cfg->qos_q_cnt; qos_q_cnt++)
        {
            AVALANCHE_PP_QOS_QUEUE_t*       qos_q_cfg = &clst_cfg->qos_q_cfg[qos_q_cnt];

            if (qos_q_cfg->q_num > AVALANCHE_PP_QOS_QUEUE_MAX_INDX)
            {
                return (PP_RC_INVALID_PARAM);
            }

            {
                Uint32  i;

                for (i = 0; i < egr_q_cnt; i++)
                {
                    if (egr_q[i] == qos_q_cfg->egr_q)
                    {
                        /* This egr_q is already configured */
                        break;
                    }
                }

                if (unlikely(i >= AVALANCHE_PP_QOS_CLST_MAX_EGRESS_QCNT))
                {
                    return (PP_RC_INVALID_PARAM);
                }

                else if (i == egr_q_cnt)
                {
                    /* Store the egress queue number to be used for cluster setup */
                    egr_q[egr_q_cnt++] = qos_q_cfg->egr_q;
                }
            }
        }
    }

    /* Configure QOS queues */
    for (qos_q_cnt = 0 ; qos_q_cnt < clst_cfg->qos_q_cnt; qos_q_cnt++)
    {
        pp_hal_qos_queue_config_set( &clst_cfg->qos_q_cfg[qos_q_cnt] );
    }

    /* Configure the cluster */
    pp_hal_qos_cluster_config_set( clst_indx, clst_cfg, &egr_q[0], egr_q_cnt );

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_qos_cluster_enable(Uint8 clst_indx)
 **************************************************************************
 * DESCRIPTION   :
 *  This function enables specified QoS cluster.
 *  param[in] clst_indx - cluster id 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_qos_cluster_enable(Uint8 clst_indx)
{
    if (clst_indx > AVALANCHE_PP_QOS_CLST_MAX_INDX)
    {
        return PP_RC_INVALID_PARAM;
    }

    PP_DB_CHECK_ACTIVE();

    return (pp_hal_qos_cluster_enable(clst_indx));
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_qos_cluster_disable(Uint8 clst_indx)
 **************************************************************************
 * DESCRIPTION   :
 *  This function disables specified QoS cluster.
 *  param[in] clst_indx - cluster id 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_qos_cluster_disable(Uint8 clst_indx)
{
    if (clst_indx > AVALANCHE_PP_QOS_CLST_MAX_INDX)
    {
        return PP_RC_INVALID_PARAM;
    }

    PP_DB_CHECK_ACTIVE();

    return (pp_hal_qos_cluster_disable(clst_indx));
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_qos_get_queue_stats ( Uint32 qos_qnum, AVALANCHE_PP_QOS_QUEUE_STATS_t* stats )
 **************************************************************************
 * DESCRIPTION   :
 *  This function retrieves the QoS statistics for the queue specified from the PP.
 *  param[in] qos_qnum - qos queue id
 *  param[in] stats - pointer to queue statistic struct 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_qos_get_queue_stats    ( Uint32    qos_qnum,   AVALANCHE_PP_QOS_QUEUE_STATS_t*     stats )
{
    Uint32 queueStcAddr;

    if (qos_qnum > AVALANCHE_PP_QOS_QUEUE_MAX_INDX) 
    {
        return PP_RC_INVALID_PARAM;
    }

    PP_DB_LOCK();
    if (PP_DB.status != PP_DB_STATUS_ACTIVE)
    {
        memset(stats, 0, sizeof(AVALANCHE_PP_QOS_QUEUE_STATS_t));
        if (PP_DB.status == PP_DB_STATUS_UNINITIALIZED)
        {
            printk("ERROR: PP Operation %s cannot be accomplished while PP status is %s\n", __FUNCTION__,  "INACTIVE"); 
            PP_DB_UNLOCK();
            return PP_RC_FAILURE;
        }
        else
        {
            /* In PSM mode we do not fail the operation but do not inquire the PP since it is down */
            PP_DB_UNLOCK();
            return PP_RC_SUCCESS;
        }
    }
    PP_DB_UNLOCK();

    /* Get info from statistical counters */ 
    queueStcAddr = (IO_PHY2VIRT(PP_HAL_COUNTERS_QPDSP_BASE_PHY) + (PP_COUNTERS_QPDSP_Q_OFF * qos_qnum));            
    stats->fwd_pkts = *(Uint32*)(queueStcAddr + PP_COUNTERS_QPDSP_Q_PKT_FRWRD_OFF);
    stats->drp_cnt  = *(Uint32*)(queueStcAddr + PP_COUNTERS_QPDSP_Q_PKT_DROP_OFF);

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_qos_set_cluster_max_global_credit( Bool creditTypeBytes, Uint8 cluster_id, Uint32 max_global_credit )
 **************************************************************************
 * DESCRIPTION   :
 *  This function set the cluster maximum global credit bytes / packets.
 *  param[in] creditTypeBytes - bytes / packets
 *  param[in] cluster_id - id of the cluster
 *  param[in] max_global_credit - parameter to set for the cluster max credit 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_qos_set_cluster_max_global_credit( Bool creditTypeBytes, Uint8 cluster_id, Uint32 max_global_credit )
{
    Uint8   egr_q_cnt;
    Uint16  egr_q[ AVALANCHE_PP_QOS_CLST_MAX_EGRESS_QCNT ];

    AVALANCHE_PP_QOS_CLST_CFG_t    clst_cfg;

    PP_DB_CHECK_ACTIVE();

    pp_hal_qos_cluster_config_get( cluster_id, &clst_cfg, &egr_q[0], &egr_q_cnt );
    if ( creditTypeBytes )
    {
        clst_cfg.max_global_credit_bytes    = max_global_credit;
    }
    else
    {
        clst_cfg.max_global_credit_packets  = (Uint16)max_global_credit;
    }
    pp_hal_qos_cluster_config_set( cluster_id, &clst_cfg, &egr_q[0], egr_q_cnt );

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_qos_set_queue_max_credit( Bool creditTypeBytes, Uint8 queue_id, Uint32 max_credit )
 **************************************************************************
 * DESCRIPTION   :
 *  This function set the qos queue maximum credit bytes / packets.
 *  param[in] creditTypeBytes - bytes / packets
 *  param[in] queue_id - id of the queue
 *  param[in] max_credit - parameter to set for the queue max credit 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_qos_set_queue_max_credit( Bool creditTypeBytes, Uint8 queue_id, Uint32 max_credit )
{
    AVALANCHE_PP_QOS_QUEUE_t    qos_q_cfg;

    PP_DB_CHECK_ACTIVE();

    qos_q_cfg.q_num = queue_id;

    pp_hal_qos_queue_config_get( &qos_q_cfg );
    if ( creditTypeBytes )
    {
        qos_q_cfg.max_credit_bytes      = max_credit;
    }
    else 
    {
        qos_q_cfg.max_credit_packets    = (Uint16)max_credit;
    }
    pp_hal_qos_queue_config_set( &qos_q_cfg );

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_qos_set_queue_iteration_credit( Bool creditTypeBytes, Uint8 queue_id, Uint32 it_credit )
 **************************************************************************
 * DESCRIPTION   :
 *  This function set the qos queue iteration credit bytes / packets.
 *  param[in] creditTypeBytes - bytes / packets
 *  param[in] queue_id - id of the queue
 *  param[in] it_credit - parameter to set for the queue iteration credit 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_qos_set_queue_iteration_credit( Bool creditTypeBytes, Uint8 queue_id, Uint32 it_credit )
{
    AVALANCHE_PP_QOS_QUEUE_t    qos_q_cfg;

    PP_DB_CHECK_ACTIVE();

    qos_q_cfg.q_num = queue_id;

    pp_hal_qos_queue_config_get( &qos_q_cfg );
    if ( creditTypeBytes )
    {
        qos_q_cfg.it_credit_bytes = it_credit;
    }
    else 
    {
        qos_q_cfg.it_credit_packets = (Uint16)it_credit;
    }
    pp_hal_qos_queue_config_set( &qos_q_cfg );

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME :avalanche_pp_qos_set_queue_congestion_threshold( Uint8 congestionTypPkt, Uint8 queue_id, Uint32 ByteThreshold ,Uint32 pktThreshold))
 **************************************************************************
 * DESCRIPTION   :
 *  This function set the qos queue iteration credit bytes / packets.
 *  param[in] congestionTypeBytes - bytes / packets
 *  param[in] queue_id - id of the queue
 *  param[in] Bytethreshold - parameter to set for the queue threshold
 *  param[in] Pktthreshold - parameter to set for the queue threshold 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_qos_set_queue_congestion_threshold( Uint8 congestionTypPkt, Uint8 queue_id,Uint32 ByteThreshold ,Uint32 pktThreshold )
{
    AVALANCHE_PP_QOS_QUEUE_t    qos_q_cfg;

    PP_DB_CHECK_ACTIVE();

    qos_q_cfg.q_num = queue_id;

    pp_hal_qos_queue_config_get( &qos_q_cfg );

    switch (congestionTypPkt)
    {
        case AVALANCHE_PP_PKT_CONGESTION_ENABLE:
            {
               qos_q_cfg.congst_thrsh_packets = (Uint16)pktThreshold;
            }
            break;
        case AVALANCHE_PP_BYTE_CONGESTION_ENABLE:
            {
                 qos_q_cfg.congst_thrsh_bytes = ByteThreshold;
            }
            break;
        case AVALANCHE_PP_PKT_AND_BYTE_CONGESTION_ENABLE:
            {
                 qos_q_cfg.congst_thrsh_packets = (Uint16)pktThreshold;
                 qos_q_cfg.congst_thrsh_bytes = ByteThreshold;
            }
            break;
        default:
            {
              return (PP_RC_INVALID_PARAM);
            }
            break;
    }

    pp_hal_qos_queue_config_set( &qos_q_cfg );

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME :avalanche_pp_qos_set_cluster_thresholds( avalanche_pp_Qos_ioctl_ClusterThrsh_t *clst_thrsh )
 **************************************************************************
 * DESCRIPTION   :
 *  This function set the qos cluster thresholds.
 *  param[in] clst_thrsh - cluster thresholds
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e   avalanche_pp_qos_set_cluster_thresholds( avalanche_pp_Qos_ioctl_ClusterThrsh_t *clst_thrsh )
{
    AVALANCHE_PP_QOS_CLST_CFG_t  clst_cfg;
    Uint8                        egr_q_cnt;
    Uint16                       egr_q[ AVALANCHE_PP_QOS_CLST_MAX_EGRESS_QCNT ];
    
    /* If cluster is not SF cluster go out */
    if (clst_thrsh->cluster_id >= PAL_CPPI41_SR_DOCSIS_TX_QOS_CLUSTER_COUNT) 
    {
        return PP_RC_INVALID_PARAM;
    }

    PP_DB_CHECK_ACTIVE();
    /* Get the Qos cluster configuration */
    pp_hal_qos_cluster_config_get( clst_thrsh->cluster_id, &clst_cfg, &egr_q[0], &egr_q_cnt );

    /* Change the cluster egress congested thresholds */
    clst_cfg.egr_congst_thrsh_bytes1   = clst_thrsh->egr_congst_thrsh_bytes1;
    clst_cfg.egr_congst_thrsh_bytes2   = clst_thrsh->egr_congst_thrsh_bytes2;
    clst_cfg.egr_congst_thrsh_bytes3   = clst_thrsh->egr_congst_thrsh_bytes3;
    clst_cfg.egr_congst_thrsh_bytes4   = clst_thrsh->egr_congst_thrsh_bytes4;
    clst_cfg.egr_congst_thrsh_packets1 = clst_thrsh->egr_congst_thrsh_packets1;
    clst_cfg.egr_congst_thrsh_packets2 = clst_thrsh->egr_congst_thrsh_packets2;
    clst_cfg.egr_congst_thrsh_packets3 = clst_thrsh->egr_congst_thrsh_packets3;
    clst_cfg.egr_congst_thrsh_packets4 = clst_thrsh->egr_congst_thrsh_packets4;

    /* Configure the cluster */
    pp_hal_qos_cluster_config_set( clst_thrsh->cluster_id, &clst_cfg, &egr_q[0], egr_q_cnt );
    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_session_tdox_capability_set( Uint32 session_handle, Bool enable )
 **************************************************************************
 * DESCRIPTION   :
 *  This function set the session tdox capability enable / disable.
 *  param[in] session_handle - handle of the session
 *  param[in] enable - "1" = enable, "0" = disable
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_session_tdox_capability_set( Uint32 session_handle, Bool     enable )
{
    struct list_head *          pos             = NULL;
    PP_DB_Entry_t *             tdox_entry      = NULL;
    PP_DB_Session_Entry_t *     ptr_session_db  = NULL;
    AVALANCHE_PP_RET_e          rc = PP_RC_SUCCESS;

    if (AVALANCHE_PP_MAX_ACCELERATED_SESSIONS <= session_handle)
    {
        return (PP_RC_INVALID_PARAM);
    }

    if (enable)
    {
        PP_DB_LOCK();

        if (list_empty( &PP_DB.pool_TDOX[ PP_DB_POOL_FREE ] ))
        {
            PP_DB.stats.tdox_starvation++;
            PP_DB_UNLOCK();
            return (PP_RC_OUT_OF_MEMORY);
        }
        else
        {
            ptr_session_db = &PP_DB.repository_sessions[ session_handle ];

            pos = PP_DB.pool_TDOX[ PP_DB_POOL_FREE ].next;

            tdox_entry = list_entry( pos, PP_DB_Entry_t, link );

            list_move( pos, &PP_DB.pool_TDOX[ PP_DB_POOL_BUSY ] );

            ptr_session_db->session_info.egress.enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED;

            ptr_session_db->session_info.egress.tdox_handle = tdox_entry->handle;
            ptr_session_db->session_info.priority                   = 0;

            if (!list_empty( &ptr_session_db->list[ PP_LIST_ID_EGRESS_TDOX ] ))
            {
                printk("%s[%d]: Error - Trying to enable TDOX for session=%d which is already in TDOX list\n", __FUNCTION__, __LINE__, session_handle);
            }
            else
            {
                list_add( &ptr_session_db->list[ PP_LIST_ID_EGRESS_TDOX ],
                          &PP_DB.repository_VPIDs[ ptr_session_db->session_info.egress.vpid_handle ].list[ PP_LIST_ID_EGRESS_TDOX ] );
            }
        }

        rc = pp_hal_session_tdox_update( &ptr_session_db->session_info );

        PP_DB_UNLOCK();
    }
    else
    {
        PP_DB_LOCK();

        ptr_session_db = &PP_DB.repository_sessions[ session_handle ];

        if ( AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED & ptr_session_db->session_info.egress.enable )
        {
            if (!list_empty( &ptr_session_db->list[ PP_LIST_ID_EGRESS_TDOX ] ))
            {
                list_del_init( &ptr_session_db->list[ PP_LIST_ID_EGRESS_TDOX ] );
            }
            else
            {
                printk("%s[%d]: Error - Trying to disable TDOX for session=%d which is not found in TDOX list\n", __FUNCTION__, __LINE__, session_handle);
            }

            list_move( &PP_DB.repository_TDOX[ ptr_session_db->session_info.egress.tdox_handle ].link, 
                       &PP_DB.pool_TDOX[ PP_DB_POOL_FREE ] );

            ptr_session_db->session_info.egress.enable &= ~AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED;
        }

        rc = pp_hal_session_tdox_update( &ptr_session_db->session_info );

        PP_DB_UNLOCK();
    }

    return (rc);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_session_tdox_capability_get( Uint32 session_handle, Bool * enable )
 **************************************************************************
 * DESCRIPTION   :
 *  This function get the session tdox capability enable / disable.
 *  param[in] session_handle - handle of the session
 *  param[in] enable - pointer to set the tdox capability status
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_session_tdox_capability_get( Uint32 session_handle, Bool *   enable )
{
    if (AVALANCHE_PP_MAX_ACCELERATED_SESSIONS <= session_handle)
    {
        return (PP_RC_INVALID_PARAM);
    }

    pp_hal_session_tdox_get( session_handle, enable );

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_version_get( AVALANCHE_PP_VERSION_t * version )
 **************************************************************************
 * DESCRIPTION   :
 *  The function is called to get the version information from the packet processor.
 *  param[in] version - pointer to version struct 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e      avalanche_pp_version_get( AVALANCHE_PP_VERSION_t * version )
{
    PP_DB_CHECK_ACTIVE();

    return pp_hal_version_get(version);
}

static inline void __avalanche_pp_printk_mac_addr(Uint8 *mac)
{
    printk("MAC address: %pM\n", mac);
}

static inline void __avalanche_pp_printk_ipv4_addr(Uint32 *ipv4)
{
    printk("IP addres: %pI4\n", ipv4);
}

static inline void __avalanche_pp_printk_ipv6_addr(Uint32 *ipv6)
{
    printk("IPv6 addres: %pI6c\n", ipv6);
}

/**************************************************************************
 * FUNCTION NAME : static AVALANCHE_PP_RET_e __avalanche_pp_addr_list(avalanche_pp_local_dev_addr_ioctl_params_t *param,
                                                                      Uint8 *list_counter, Uint8 *addr_list_ptr,
                                                                      Uint8 entry_size, Uint8 list_max)
 **************************************************************************
 * DESCRIPTION   :
 *  The function update PP local devices address lists in PP FW and DB
 *  param[in] param - Local device address data structure
 *  param[in] list_counter - Number of valid entries in the list
 *  param[in] addr_list_ptr - Pointer to address list
 *  param[in] entry_size - Size of single entry
 *  param[in] list_max - Maximum allowed entries
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
static AVALANCHE_PP_RET_e __avalanche_pp_addr_list(avalanche_pp_local_dev_addr_ioctl_params_t *param,
                                                   Uint8 *list_counter, Uint8 *addr_list_ptr,
                                                   Uint8 entry_size, Uint8 list_max)
{
    Uint8 index, counter = *list_counter;
    Uint8 *param_adr_ptr = &param->u.all;

    switch (param->op_type)
    {
    case ADD_ADDR:
        {
            /* Search if address exist */
            for (index = 0 ; index < counter ; index++)
                if (memcmp((void*)param_adr_ptr, (void*)&addr_list_ptr[index * entry_size], entry_size) == 0)
                    return (PP_RC_OBJECT_EXIST);

            if (counter >= list_max)
            {
                printk("%s: PP White list is full\n", __FUNCTION__);
                return (PP_RC_OUT_OF_MEMORY);
            }
            else
            {
                /* Set in next free entry */
                memcpy(&addr_list_ptr[counter * entry_size], param_adr_ptr, entry_size);
                pp_hal_set_pp_addr_list(param->addr_type, &addr_list_ptr[counter * entry_size], counter, counter + 1);
                (*list_counter)++;
                return (PP_RC_SUCCESS);
            }
        }
    case RM_ADDR:
        {
            /* Search if address exist */
            for (index = 0 ; index < counter ; index++)
            {
                if (memcmp((void*)param_adr_ptr, (void*)&addr_list_ptr[index * entry_size], entry_size) == 0)
                {
                    /* Entry found */
                    memset(param_adr_ptr, 0, entry_size);
                    /* Remove entry */
                    for (index = index +1 ; index < counter ; index++)
                    {
                        memcpy(&addr_list_ptr[(index-1) * entry_size], &addr_list_ptr[index * entry_size], entry_size);
                        pp_hal_set_pp_addr_list(param->addr_type, &addr_list_ptr[(index-1) * entry_size], index-1, counter); // Add
                    }
                    pp_hal_set_pp_addr_list(param->addr_type, param_adr_ptr, index-1, counter - 1); // Remove
                    memset(&addr_list_ptr[(index-1) * entry_size], 0, entry_size);
                    (*list_counter)--;
                    return (PP_RC_SUCCESS);
                }
            }
            /* No such address */
            printk("%s: Can't remove address from PP DB, entry not found\n", __FUNCTION__);
            return (PP_RC_INVALID_PARAM); // Not exist
        }
    case FLUSH_LIST:
        {
            memset(param_adr_ptr, 0, entry_size);
            if (counter)
            {
                do
                {
                    counter--;
                    memcpy(&addr_list_ptr[counter * entry_size], param_adr_ptr, entry_size);
                    pp_hal_set_pp_addr_list(param->addr_type, param_adr_ptr, counter, counter); // Remove
                } while (counter);
                *list_counter = 0;
            }
        }
        break;
    case RD_VALID_ENTRIES_COUNT:
        {
            param->valid_entries_cnt = counter;
        }
        break;
    case IS_ADDR_EXIST:
        {
            /* Search if address exist */
            for (index = 0 ; index < counter ; index++)
                if (memcmp((void*)param_adr_ptr, (void*)&addr_list_ptr[index * entry_size], entry_size) == 0)
                    return (PP_RC_SUCCESS); // Address exist
            return (PP_RC_INVALID_PARAM); // Not exist
        }
    default:
        {
            printk("%s: Unsupported PP White list operation\n", __FUNCTION__);
            return (PP_RC_INVALID_PARAM);
        }
    }

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_print_local_dev_addr(avalanche_pp_local_dev_addr_ioctl_params_t *param)
 **************************************************************************
 * DESCRIPTION   :
 *  The function prints local devices address list
 *  from the PP DB
 *  param[in] param - Local device address data structure
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_print_local_dev_addr(avalanche_pp_local_dev_addr_ioctl_params_t *param)
{
    PP_DB_Local_Dev_Address_t *db_addr_ptr = &PP_DB.local_dev_addr;
    Uint8 index;

    switch (param->addr_type)
    {
    case MTA_MAC_ADDR:
        {
            printk("MTA_MAC_ADDR address:\n");
            __avalanche_pp_printk_mac_addr(db_addr_ptr->mta_mac_addr.mac_addr);
        }
        break;
    case MTA_RTP_IPV4_ADDR:
        {
            printk("MTA_RTP_IPV4_ADDR white list in PP DB:\n");
            for (index = 0 ; index < MAX_MTA_RTP_IPS ; index++)
                __avalanche_pp_printk_ipv4_addr(&db_addr_ptr->mta_ips_white_list.rtp_ipv4_addr[index]);
        }
        break;
    case MTA_MNG_IPV4_ADDR:
        {
            printk("MTA_MNG_IPV4_ADDR white list in PP DB:\n");
            for (index = 0 ; index < MAX_MTA_MNG_IPS ; index++)
                __avalanche_pp_printk_ipv4_addr(&db_addr_ptr->mta_ips_white_list.mng_ipv4_addr[index]);
        }
        break;
    case MTA_RTP_IPV6_ADDR:
        {
            printk("MTA_RTP_IPV6_ADDR white list in PP DB:\n");
            for (index = 0 ; index < MAX_MTA_RTP_IPS ; index++)
                __avalanche_pp_printk_ipv6_addr(&db_addr_ptr->mta_ips_white_list.rtp_ipv6_addr[index][0]);
        }
        break;
    case MTA_MNG_IPV6_ADDR:
        {
            printk("MTA_MNG_IPV6_ADDR white list in PP DB:\n");
            for (index = 0 ; index < MAX_MTA_MNG_IPS ; index++)
                __avalanche_pp_printk_ipv6_addr(&db_addr_ptr->mta_ips_white_list.mng_ipv6_addr[index][0]);
        }
        break;
    case WAN_MAC_ADDR:
        {
            printk("WAN_MAC_ADDR address:\n");
            __avalanche_pp_printk_mac_addr(db_addr_ptr->wan0_mac_addr.mac_addr);
        }
        break;
    case GW_MAC_ADDR:
        {
            printk("GW_MAC_ADDR address:\n");
            __avalanche_pp_printk_mac_addr(db_addr_ptr->gw_mac_addr.mac_addr);
        }
        break;
    case LAN0_MAC_ADDR:
        {
            printk("LAN0_MAC_ADDR address:\n");
            __avalanche_pp_printk_mac_addr(db_addr_ptr->lan0_mac_addr.mac_addr);
        }
        break;
    case RND_MAC_ADDR:
        {
            printk("RND_MAC_ADDR white list in PP DB:\n");
            for (index = 0 ; index < MAX_RND_MACS ; index++)
                __avalanche_pp_printk_mac_addr(&db_addr_ptr->rnd_mac_white_list.rnd_mac_addr[index][0]);
        }
        break;
	case MULTI_DROP_IPV4_ADDR:
		{
            printk("Ipv4 Drop List in PP DB:\n");
            for (index = 0 ; index < MAX_MULTI_DROP_IPV4 ; index++)
                __avalanche_pp_printk_ipv4_addr(&db_addr_ptr->mdrop_ips_list.ipv4_addr[index]);
		}
		break;
	case MULTI_DROP_IPV6_ADDR:
        {
            printk("Ipv6 Drop List in PP DB:\n");
            for (index = 0 ; index < MAX_MULTI_DROP_IPV6 ; index++)
                __avalanche_pp_printk_ipv6_addr(&db_addr_ptr->mdrop_ips_list.ipv6_addr[index][0]);
		}
		break; 
    default:
        {
            printk("%s: Unsupported PP local device address type\n", __FUNCTION__);
            return (PP_RC_INVALID_PARAM);
        }
    }

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e AVALANCHE_PP_RET_e avalanche_pp_local_dev_addr(avalanche_pp_local_dev_addr_ioctl_params_t *param)
 **************************************************************************
 * DESCRIPTION   :
 *  The function sets local devices MAC/IPv4/IPv6 address
 *  for the packet processor.
 *  param[in] param - Local device address data structure
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_local_dev_addr(avalanche_pp_local_dev_addr_ioctl_params_t *param)
{
    PP_DB_Local_Dev_Address_t *db_addr_ptr = &PP_DB.local_dev_addr;
    AVALANCHE_PP_RET_e rc = PP_RC_SUCCESS;

    if (param == NULL)
        return (PP_RC_INVALID_PARAM);

    PP_DB_LOCK();

    PP_DB_CHECK_ACTIVE_UNDER_LOCK();

    switch (param->addr_type)
    {
    case MTA_MAC_ADDR:
        {
            rc = __avalanche_pp_addr_list(param, &db_addr_ptr->mta_mac_addr.mac_addr_cnt,
                                                  db_addr_ptr->mta_mac_addr.mac_addr,
                                                  sizeof(param->u.mac_addr), MAX_SINGLE_MAC);
        }
        break;
    case MTA_RTP_IPV4_ADDR:
        {
            rc = __avalanche_pp_addr_list(param, &db_addr_ptr->mta_ips_white_list.rtp_ipv4_cnt,
                                                  db_addr_ptr->mta_ips_white_list.rtp_ipv4_addr,
                                                  sizeof(param->u.ipv4), MAX_MTA_RTP_IPS);
        }
        break;
    case MTA_MNG_IPV4_ADDR:
        {
            rc = __avalanche_pp_addr_list(param, &db_addr_ptr->mta_ips_white_list.mng_ipv4_cnt,
                                                  db_addr_ptr->mta_ips_white_list.mng_ipv4_addr,
                                                  sizeof(param->u.ipv4), MAX_MTA_MNG_IPS);
        }
        break;
    case MTA_RTP_IPV6_ADDR:
        {
            rc = __avalanche_pp_addr_list(param, &db_addr_ptr->mta_ips_white_list.rtp_ipv6_cnt,
                                                  db_addr_ptr->mta_ips_white_list.rtp_ipv6_addr,
                                                  sizeof(param->u.ipv6), MAX_MTA_RTP_IPS);
        }
        break;
    case MTA_MNG_IPV6_ADDR:
        {
            rc = __avalanche_pp_addr_list(param, &db_addr_ptr->mta_ips_white_list.mng_ipv6_cnt,
                                                  db_addr_ptr->mta_ips_white_list.mng_ipv6_addr,
                                                  sizeof(param->u.ipv6), MAX_MTA_MNG_IPS);
        }
        break;
    case WAN_MAC_ADDR:
        {
            rc = __avalanche_pp_addr_list(param, &db_addr_ptr->wan0_mac_addr.mac_addr_cnt,
                                                  db_addr_ptr->wan0_mac_addr.mac_addr,
                                                  sizeof(param->u.mac_addr), MAX_SINGLE_MAC);
        }
        break;
    case GW_MAC_ADDR:
        {
            rc = __avalanche_pp_addr_list(param, &db_addr_ptr->gw_mac_addr.mac_addr_cnt,
                                                  db_addr_ptr->gw_mac_addr.mac_addr,
                                                  sizeof(param->u.mac_addr), MAX_SINGLE_MAC);
        }
        break;
    case LAN0_MAC_ADDR:
        {
            rc = __avalanche_pp_addr_list(param, &db_addr_ptr->lan0_mac_addr.mac_addr_cnt,
                                                  db_addr_ptr->lan0_mac_addr.mac_addr,
                                                  sizeof(param->u.mac_addr), MAX_SINGLE_MAC);
        }
        break;
    case RND_MAC_ADDR:
        {
            rc = __avalanche_pp_addr_list(param, &db_addr_ptr->rnd_mac_white_list.rnd_mac_cnt,
                                                  db_addr_ptr->rnd_mac_white_list.rnd_mac_addr,
                                                  sizeof(param->u.mac_addr), MAX_RND_MACS);
        }
        break;

    case MULTI_DROP_IPV4_ADDR:
        {
            rc = __avalanche_pp_addr_list(param, &db_addr_ptr->mdrop_ips_list.drop_ipv4_cnt, 
                                                  db_addr_ptr->mdrop_ips_list.ipv4_addr, 
                                                  sizeof(param->u.ipv4), MAX_MULTI_DROP_IPV4);
        }
        break;
    case MULTI_DROP_IPV6_ADDR:
        {
            rc = __avalanche_pp_addr_list(param, &db_addr_ptr->mdrop_ips_list.drop_ipv6_cnt, 
                                                  db_addr_ptr->mdrop_ips_list.ipv6_addr, 
                                                  sizeof(param->u.ipv6), MAX_MULTI_DROP_IPV6);
        }
        break;
    default:
        {
            printk("%s: Unsupported PP local device address type %d \n", __FUNCTION__, param->addr_type);
            PP_DB_UNLOCK();
            return (PP_RC_INVALID_PARAM);
        }
    }
    PP_DB_UNLOCK();

    return (rc);
}

/**************************************************************************
 * FUNCTION NAME : Bool avalanche_pp_state_is_active ( void )
 **************************************************************************
 * DESCRIPTION   :
 *  This function check if PP status is active
 * RETURNS       :
 *      True  -   PP not active
 *      False  -   PP active
 **************************************************************************/
Bool                  avalanche_pp_state_is_active( void )
{
    Bool rc;

    PP_DB_LOCK();
    rc = (PP_DB_STATUS_ACTIVE == PP_DB.status);
    PP_DB_UNLOCK();

    return (rc);
}

/**************************************************************************
 * FUNCTION NAME : Bool avalanche_pp_state_is_psm( void )
 **************************************************************************
 * DESCRIPTION   :
 *  This function check if PP status is psm mode
 * RETURNS       :
 *      True  -   PP not in psm mode
 *      False  -   PP in psm mode
 **************************************************************************/
Bool                  avalanche_pp_state_is_psm( void )
{
    Bool rc;

    PP_DB_LOCK();
    rc = (PP_DB_STATUS_PSM == PP_DB.status);
    PP_DB_UNLOCK();

    return (rc);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_db_stats (AVALANCHE_PP_Misc_Statistics_t * stats_ptr)
 **************************************************************************
 * DESCRIPTION   :
 *  The function is called to get the statistics of the PP DB.
 *  param[in] stats_ptr - pointer to set the PP DB statistics 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_get_db_stats (AVALANCHE_PP_Misc_Statistics_t * stats_ptr)
{
    memcpy((void*)stats_ptr, (void*)&PP_DB.stats, sizeof(AVALANCHE_PP_Misc_Statistics_t));
    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_reset_db_stats (void)
 **************************************************************************
 * DESCRIPTION   :
 *  The function reset the PP DB statistics.
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e    avalanche_pp_reset_db_stats (void)
{
    Uint32 i;

    PP_DB_LOCK();

    PP_DB.stats.max_active_lut1_keys = 0;
    PP_DB.stats.max_active_sessions = 0;
    for (i = 0; i < AVALANCHE_PP_LUT_HISTOGRAM_SIZE; i++)
    {
        PP_DB.stats.lut1_histogram[i] = 0;
    }
    PP_DB.stats.lut1_starvation = 0;
    for (i = 0; i < AVALANCHE_PP_LUT_HISTOGRAM_SIZE; i++)
    {
        PP_DB.stats.lut2_histogram[i] = 0;
    }
    PP_DB.stats.lut2_starvation = 0;
    PP_DB.stats.tdox_starvation = 0;

    PP_DB_UNLOCK();

    return (PP_RC_SUCCESS); 
}

/**************************************************************************
 * static AVALANCHE_PP_RET_e __avalanche_pp_limit_queue_host_qos(Bool limit_en, Uint8 queue_id)
 **************************************************************************
 * DESCRIPTION   :
 *  This function set enable/disable of the qos queue
 *  limitation for the docsis low host qos queue
 *  param[in] Bool limit_en - True to Enable queue limit, False to Disable
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
static AVALANCHE_PP_RET_e __avalanche_pp_limit_queue_host_qos(Bool limit_en, Uint8 queue_id)
{
    AVALANCHE_PP_QOS_QUEUE_t qos_q_cfg;
    Uint16 pkt_credit;

    PP_DB_CHECK_ACTIVE();

    qos_q_cfg.q_num = queue_id;

    pp_hal_qos_queue_config_get(&qos_q_cfg);
    if (limit_en)
    {
        /* Set 1kPPS queue limit */
        qos_q_cfg.flags |= AVALANCHE_PP_QOS_Q_LIMITEDQ;
        qos_q_cfg.max_credit_bytes = Q_LIMIT_1K_PPS_BYTS_CREDIT;
        qos_q_cfg.max_credit_packets = Q_LIMIT_1K_PPS_PKT_CREDIT;
    }
    else
    {
        /* Disable queue limit */
        qos_q_cfg.flags &= ~AVALANCHE_PP_QOS_Q_LIMITEDQ;
        qos_q_cfg.max_credit_bytes = 0;
        qos_q_cfg.max_credit_packets = 0;
    }
    avalanche_pp_qos_cluster_disable(PAL_CPPI41_SR_HOST_QOS_CLUSTER_NUM);
    pp_hal_qos_queue_config_set(&qos_q_cfg);
    avalanche_pp_qos_cluster_enable(PAL_CPPI41_SR_HOST_QOS_CLUSTER_NUM);

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_e state, Uint8 pid_handle)
 **************************************************************************
 * DESCRIPTION   :
 *  This function sets the current defensive state
 *  param[in] state - defensive state 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_e state)
{
    AVALANCHE_DDH_NOTIFY_FN_t cb;
    PP_DB.defensive_state.state = state;

    /* Check if we have a ddh notify callback registered */
    if (PP_DB.defensive_state.notify_cb)
    {
        cb = PP_DB.defensive_state.notify_cb;
        cb(&state);
    }

    return (PP_RC_SUCCESS); 
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_defensive_state(AVALANCHE_PP_DDH_STATE_e * state)
 **************************************************************************
 * DESCRIPTION   :
 *  This function returns the current defensive state
 *  param[out] state - defensive state 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_get_defensive_state(AVALANCHE_PP_DDH_STATE_e * state)
{
    *state = PP_DB.defensive_state.state;

    return (PP_RC_SUCCESS); 
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_set_l2classification_default_mode(Bool l2_classification_default_mode)
 **************************************************************************
 * DESCRIPTION   :
 *  This function sets the default l2 classification mode
 *  param[in] l2 classification mode 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_set_l2classification_default_mode(Bool l2_classification_default_mode)
{
    PP_DB.defensive_state.l2_classification_enabled_by_default = l2_classification_default_mode;
    return (PP_RC_SUCCESS); 
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_l2classification_default_mode(Bool* l2_classification_default_mode)
 **************************************************************************
 * DESCRIPTION   :
 *  This function returns the default l2 classification mode
 *  param[out] l2 classification mode
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_get_l2classification_default_mode(Bool* l2_classification_default_mode)
{
    *l2_classification_default_mode = PP_DB.defensive_state.l2_classification_enabled_by_default; 
    return (PP_RC_SUCCESS); 
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_set_defensive_mode(AVALANCHE_PP_Defensive_Mode_e defensive_mode)
 **************************************************************************
 * DESCRIPTION   :
 *  This function sets the current defensive mode
 *  param[in] defensive_mode - DDH mode (interna/externl)
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_set_defensive_mode(AVALANCHE_PP_Defensive_Mode_e defensive_mode)
{
    PP_DB.defensive_mode = defensive_mode;

    return (PP_RC_SUCCESS); 
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_defensive_mode(AVALANCHE_PP_Defensive_Mode_e* defensive_mode)
 **************************************************************************
 * DESCRIPTION   :
 *  This function returns the current defensive mode
 *  param[out] defensive_mode - DDH mode (internal/external)
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_get_defensive_mode(AVALANCHE_PP_Defensive_Mode_e* defensive_mode)
{
    *defensive_mode = PP_DB.defensive_mode;

    return (PP_RC_SUCCESS); 
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_support_multi_drop(Bool enable_multi_drop)
 **************************************************************************
 * DESCRIPTION   :
 *  This function enable/disable multi-drop
 *  param[in] enable_multi_drop - enable/disable multi-drop
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_support_multi_drop(Bool enable_multi_drop)
{
    PP_DB.defensive_state.multi_drop_enabled = enable_multi_drop;

    /* Send command to pdsp - enable multi drop. */
    return pp_hal_session_support_multi_drop(enable_multi_drop);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_support_smart_prioritization(Bool enable_smart_prioritization)
 **************************************************************************
 * DESCRIPTION   :
 *  This function enable/disable smart prioritization
 *  param[in] enable_smart_prioritization - enable/disable smart prioritization
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_support_smart_prioritization(Bool enable_smart_prioritization)
{
    PP_DB.defensive_state.smart_prioritization_enabled = enable_smart_prioritization;

    /* Enable/Disable queue limit for 1kPPS to DOCSIS LOW HOST qos queue */
    __avalanche_pp_limit_queue_host_qos(enable_smart_prioritization, PP_HAL_DOCSIS_LOW_HOST_QOS_QUEUE);

    /* Send command to pdsp - enable smart prioritization. */
    return pp_hal_session_support_smart_prioritization(enable_smart_prioritization);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_smart_prioritization(Bool *smart_prioritization_enabled)
 **************************************************************************
 * DESCRIPTION   :
 *  param[out] smart_prioritization_enabled - smart prioritization enabled/disabled
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_get_smart_prioritization(Bool *smart_prioritization_enabled)
{
    *smart_prioritization_enabled = PP_DB.defensive_state.smart_prioritization_enabled;

    return (PP_RC_SUCCESS); 
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_support_l2_classification(Bool enable_l2_classification, Uint8 pid_handle)
 **************************************************************************
 * DESCRIPTION   :
 *  This function enable/disable l2 classification
 *  param[in] enable_l2_classification - enable/disable l2 classification
 *  param[in] pid_handle
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_support_l2_classification(Bool enable_l2_classification, Uint8 pid_handle)
{
    /* Do not modify l2classidication mode if it is enabled by default  */
    if (PP_DB.defensive_state.l2_classification_enabled_by_default == False)
    {
        PP_DB.defensive_state.l2_classification_enabled = enable_l2_classification;
        PP_DB.defensive_state.l2_classification_pid     = pid_handle;

        return pp_hal_support_l2_classification((Uint8)enable_l2_classification, pid_handle);
    }

    return PP_RC_SUCCESS;
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_l2_classification(Bool *l2_classification_enabled, Uint8* pid_handle)
 **************************************************************************
 * DESCRIPTION   :
 *  param[out] l2_classification_enabled - l2_classification enabled/disabled
 *  param[out] l2_classification_pid
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_get_l2_classification(Bool *l2_classification_enabled, Uint8 *pid_handle)
{
    *l2_classification_enabled = PP_DB.defensive_state.l2_classification_enabled; 
    *pid_handle                = PP_DB.defensive_state.l2_classification_pid; 

    return (PP_RC_SUCCESS); 
}
/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_blacklist_info(Bool *multi_drop_enabled, Uint8 ip_type, AVALANCHE_PP_BLACKLIST_INFO_e * blacklist_info)
 **************************************************************************
 * DESCRIPTION   :
 *  This function returns if blacklist is full or not
 *  param[in] blacklist_info - blacklist status
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_get_blacklist_info(Bool *multi_drop_enabled, Uint8 ip_type, AVALANCHE_PP_BLACKLIST_INFO_e *blacklist_info)
{
    avalanche_pp_local_dev_addr_ioctl_params_t drop_list_param;
    Uint8 max_entries;
    AVALANCHE_PP_RET_e rc = PP_RC_SUCCESS;
    *multi_drop_enabled = PP_DB.defensive_state.multi_drop_enabled;
    
    if (AVALANCHE_PP_LUT_ENTRY_L3_IPV4 == ip_type)
    {
        drop_list_param.addr_type   = MULTI_DROP_IPV4_ADDR;
        max_entries                 = MAX_MULTI_DROP_IPV4;
    }
    else if (AVALANCHE_PP_LUT_ENTRY_L3_IPV6 == ip_type)
    {
        drop_list_param.addr_type   = MULTI_DROP_IPV6_ADDR;
        max_entries                 = MAX_MULTI_DROP_IPV6;
    }
    else
    {
        /* We do not support any other L3 protocol */
        return (PP_RC_FAILURE);
    }

    drop_list_param.op_type = RD_VALID_ENTRIES_COUNT;
    if (PP_RC_SUCCESS != avalanche_pp_local_dev_addr(&drop_list_param))
        return (PP_RC_FAILURE);

    if (drop_list_param.valid_entries_cnt == max_entries)
    {
        *blacklist_info = AVALANCHE_PP_BLACKLIST_FULL; 
    }
    else if (drop_list_param.valid_entries_cnt == 0)
    {
        *blacklist_info = AVALANCHE_PP_BLACKLIST_EMPTY;
    }
    else 
    {
        *blacklist_info = AVALANCHE_PP_BLACKLIST_IN_USE;
    }

    return (PP_RC_SUCCESS); 
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_host_packets(Uint32 *host_packets)
 **************************************************************************
 * DESCRIPTION   :
 *  This function returns number of packets forwarded to host
 *  param[out] host_packets
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_get_host_packets(Uint32 *host_packets)
{
    *host_packets= *(Uint32*)(IO_PHY2VIRT(PP_HAL_COUNTERS_MPDSP_BASE_PHY) + PP_COUNTERS_MPDSP_PKTS_FRWRD_TO_HOST);
    return (PP_RC_SUCCESS); 
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_defensive_state_notify_bind(AVALANCHE_DDH_NOTIFY_FN_t  handler)
 **************************************************************************
 * DESCRIPTION   :
 *  param[in] handler - 
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_defensive_state_notify_bind(AVALANCHE_DDH_NOTIFY_FN_t handler)
{
    if (handler == NULL)
    {
        return (PP_RC_INVALID_PARAM);
    }

    PP_DB.defensive_state.notify_cb = handler; 

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_update_bithash (Uint16 hash, Bool add_operation)
 **************************************************************************
 * DESCRIPTION   : Update the bithash table
 *  param[in] hash - hash value to add/remove
 *  param[in] add_operation - True to add, False to remove
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_update_bithash (Uint16 hash, Bool add_operation)
{
    PP_DB_CHECK_ACTIVE();

    PP_DB_LOCK();

    if (add_operation)
    {
        PP_DB.bithash_refcnt[hash]++;
        
        /* If this is the 1st time we enable the bit, Set the bit in hash */
        if (PP_DB.bithash_refcnt[hash] == 1)
        {
            pp_hal_update_bithash(hash, True);
        }
    }
    else
    {
        if (PP_DB.bithash_refcnt[hash] != 0)
        {
            PP_DB.bithash_refcnt[hash]--;
            
            /* If this is the last time we disable the bit, Clear the bit in hash */
            if (PP_DB.bithash_refcnt[hash] == 0)
            {
                pp_hal_update_bithash(hash, False);
            }
        }
        else
        {
            printk("%s:%d: Error removing hash %d from bithash - Entry is not set\n", __FUNCTION__, __LINE__, hash);
        }
    }

    PP_DB_UNLOCK();

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_flush_bithash ()
 **************************************************************************
 * DESCRIPTION   : Flushes the bithash table
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_flush_bithash ()
{
    PP_DB_CHECK_ACTIVE();

    PP_DB_LOCK();
    pp_hal_flush_bithash();
    PP_DB_UNLOCK();

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_bithash (Uint8** bithash_table)
 **************************************************************************
 * DESCRIPTION   : get the bithash table
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_get_bithash (Uint8** bithash_table)
{
    *bithash_table = PP_DB.bithash_refcnt;

    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_get_scb_entry (AVALANCHE_PP_SCB_Entry_t **scb_entry)
 **************************************************************************
 * DESCRIPTION   : Return a connection bypass entry
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_get_scb_entry (AVALANCHE_PP_SCB_Entry_t **scb_entry, Uint32 entry_index)
{
    if (entry_index >= AVALANCHE_PP_MAX_ACCELERATED_SESSIONS)
    {
        *scb_entry = NULL;
        return (PP_RC_FAILURE); 
    }

    *scb_entry = &PP_DB.repoistory_scb[entry_index];
    return (PP_RC_SUCCESS);
}

/**************************************************************************
 * FUNCTION NAME : AVALANCHE_PP_RET_e avalanche_pp_flush_scb_db ()
 **************************************************************************
 * DESCRIPTION   : flush scb db
 * RETURNS       :
 *      0  -   Success
 *      >0  -   Error
 **************************************************************************/
AVALANCHE_PP_RET_e avalanche_pp_flush_scb_db ()
{
    memset(PP_DB.repoistory_scb, 0, sizeof(PP_DB.repoistory_scb));
    return (PP_RC_SUCCESS);
}

/* Short Connection Bypass API */
EXPORT_SYMBOL(avalanche_pp_flush_scb_db);
EXPORT_SYMBOL(avalanche_pp_get_scb_entry);

/* Defensive mechanism API */
EXPORT_SYMBOL(avalanche_pp_set_defensive_state);
EXPORT_SYMBOL(avalanche_pp_get_defensive_state);
EXPORT_SYMBOL(avalanche_pp_set_l2classification_default_mode);
EXPORT_SYMBOL(avalanche_pp_get_l2classification_default_mode);
EXPORT_SYMBOL(avalanche_pp_support_multi_drop);
EXPORT_SYMBOL(avalanche_pp_support_smart_prioritization);
EXPORT_SYMBOL(avalanche_pp_get_smart_prioritization);
EXPORT_SYMBOL(avalanche_pp_support_l2_classification);
EXPORT_SYMBOL(avalanche_pp_get_l2_classification);
EXPORT_SYMBOL(avalanche_pp_get_blacklist_info);
EXPORT_SYMBOL(avalanche_pp_get_host_packets);
EXPORT_SYMBOL(avalanche_pp_defensive_state_notify_bind);

/* Smart Prioritization API */
EXPORT_SYMBOL(avalanche_pp_update_bithash);
EXPORT_SYMBOL(avalanche_pp_flush_bithash);
EXPORT_SYMBOL(avalanche_pp_get_bithash);

/* PID and VPID Management API */
EXPORT_SYMBOL( avalanche_pp_pid_create       );
EXPORT_SYMBOL( avalanche_pp_pid_delete       );
EXPORT_SYMBOL( avalanche_pp_pid_config_range );
EXPORT_SYMBOL( avalanche_pp_pid_remove_range );
EXPORT_SYMBOL( avalanche_pp_pid_set_flags    );
EXPORT_SYMBOL( avalanche_pp_pid_get_list     );
EXPORT_SYMBOL( avalanche_pp_pid_get_info     );

EXPORT_SYMBOL( avalanche_pp_vpid_create      );
EXPORT_SYMBOL( avalanche_pp_vpid_delete      );
EXPORT_SYMBOL( avalanche_pp_vpid_set_flags   );
EXPORT_SYMBOL( avalanche_pp_vpid_get_list    );
EXPORT_SYMBOL( avalanche_pp_vpid_get_info    );

/* Session Management API */
EXPORT_SYMBOL( avalanche_pp_session_create   );
EXPORT_SYMBOL( avalanche_pp_session_delete   );
EXPORT_SYMBOL( avalanche_pp_session_get_list );
EXPORT_SYMBOL( avalanche_pp_session_get_info );
EXPORT_SYMBOL( avalanche_pp_flush_sessions   );

EXPORT_SYMBOL( avalanche_pp_session_list_execute     );
EXPORT_SYMBOL( avalanche_pp_session_pre_action_bind  );
EXPORT_SYMBOL( avalanche_pp_session_post_action_bind );

/* Statistics API */
EXPORT_SYMBOL( avalanche_pp_get_stats_session );
EXPORT_SYMBOL( avalanche_pp_get_stats_vpid    );
EXPORT_SYMBOL( avalanche_pp_get_stats_global  );

EXPORT_SYMBOL( avalanche_pp_event_handler_register   );
EXPORT_SYMBOL( avalanche_pp_event_handler_unregister );
EXPORT_SYMBOL( avalanche_pp_event_report             );


/* QoS API. */
EXPORT_SYMBOL( avalanche_pp_qos_cluster_setup   );
EXPORT_SYMBOL( avalanche_pp_qos_cluster_enable  );
EXPORT_SYMBOL( avalanche_pp_qos_cluster_disable );
EXPORT_SYMBOL( avalanche_pp_qos_get_queue_stats );

// Not sure if we still need these ...
EXPORT_SYMBOL( avalanche_pp_qos_set_cluster_max_global_credit );
EXPORT_SYMBOL( avalanche_pp_qos_set_queue_max_credit          );
EXPORT_SYMBOL( avalanche_pp_qos_set_queue_iteration_credit    );
EXPORT_SYMBOL( avalanche_pp_qos_set_queue_congestion_threshold);
EXPORT_SYMBOL( avalanche_pp_qos_set_cluster_thresholds        );

/* Power Saving Mode (PSM) API. */
EXPORT_SYMBOL( avalanche_pp_psm      );
EXPORT_SYMBOL( avalanche_pp_hw_init  );


/* MISC APIs */
EXPORT_SYMBOL( avalanche_pp_session_tdox_capability_set );
EXPORT_SYMBOL( avalanche_pp_session_tdox_capability_get );
EXPORT_SYMBOL( avalanche_pp_version_get                 );
EXPORT_SYMBOL( avalanche_pp_local_dev_addr);
EXPORT_SYMBOL( avalanche_pp_print_local_dev_addr);

EXPORT_SYMBOL( avalanche_pp_get_db_stats );
EXPORT_SYMBOL( avalanche_pp_reset_db_stats );
EXPORT_SYMBOL( avalanche_pp_state_is_active );
