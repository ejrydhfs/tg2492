/*
**  igmpproxy - IGMP proxy based multicast router
**  Copyright (C) 2005 Johnny Egeland <johnny@rlo.org>
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
**----------------------------------------------------------------------------
**
**  This software is derived work from the following software. The original
**  source code has been modified from it's original state by the author
**  of igmpproxy.
**
**  smcroute 0.92 - Copyright (C) 2001 Carsten Schill <carsten@cschill.de>
**  - Licensed under the GNU General Public License, version 2
**
**  mrouted 3.9-beta3 - COPYRIGHT 1989 by The Board of Trustees of
**  Leland Stanford Junior University.
**  - Original license can be found in the Stanford.txt file.
**
*/
/**
*   request.c
*
*   Functions for recieveing and processing IGMP requests.
*
*/

#include "igmpproxy.h"
#include "inet6.h"
#include "timers.h"
#include "groups.h"


/**
*   Handles incoming membership reports, and
*   appends them to the routing table.
*/

void acceptGroupReport(uint16_t ifindex, mifi_t mifi, uint32_t src, uint32_t group, uint8_t report_code)
{
    struct mvif  *sourceVif= &mvifs[mifi];

    // Sanitycheck the group adress...
    if (!IN_MULTICAST( ntohl(group) )) {
        log_msg(LOG_WARNING, 0, "The group address %s is not a valid Multicast group.",
                inetFmt(group));
        return;
    }


    if (sourceVif->InAdr.s_addr == src) {
        log_msg(LOG_NOTICE, 0, "The IGMP message was from myself. Ignoring.");
        return;
    }

    // We have a IF so check that it's an downstream IF.
    if (sourceVif->uv_flags & VIFF_DOWNSTREAM) {


        IF_DEBUG(DEBUG_MLD_PROTO)

        log_msg( LOG_DEBUG, 0,
                 "accepting multicast listener report: "
                 "interface #%dv%d: %s src %s, grp %s\n",
                 ifindex, mifi,
                 sourceVif->mvif_name, inetFmt(src), inetFmt(group));


        // The membership report was OK... Insert it into the route table..
        sourceVif->uv_in_mld_report++;

        recv_listener_report(ifindex, mifi, src, group, report_code);

    } else {
        // Log the state of the interface the report was recieved on.
        IF_DEBUG(DEBUG_MLD_PROTO)
        log_msg(LOG_INFO, 0, "Membership report was recieved on the upstream interface. Ignoring.");

    }

}

/**
*   Recieves and handles a group leave message.
*/

void acceptLeaveMessage(int ifindex, mifi_t mifi, uint32_t src, uint32_t group) {

    struct mvif  *sourceVif= &mvifs[mifi];

    /* Sanity check the group adress... */
    if (!IN_MULTICAST( ntohl(group) )) {
        log_msg(LOG_WARNING, 0, "acceptLeaveMessage:The group address %s is not a valid Multicast group.",
                inetFmt(group));
        return;
    }

    IF_DEBUG(DEBUG_MLD_PROTO)
    log_msg(LOG_DEBUG, 0,
            "Got leave message from %s to %s. Starting last member detection.",
            inetFmt(src), inetFmt(group));


    // We have a IF so check that it's an downstream IF.
    if (sourceVif->uv_flags & VIFF_DOWNSTREAM) {

        IF_DEBUG(DEBUG_MLD_PROTO)

        log_msg( LOG_DEBUG, 0,
                 "accepting multicast LEAVE message: "
                 "interface #%dv%d: %s src %s, grp %s\n",
                 ifindex, mifi,
                 sourceVif->mvif_name, inetFmt(src), inetFmt(group));


        recv_listener_done(ifindex, mifi, src, group);

    } else {
        // just ignore the leave request...
        log_msg(LOG_DEBUG, 0, "The found if for %s was not downstream. Ignoring leave request.", inetFmt(src));
    }
}
/* ARRIS ADD START for FCCD */
/*
  Most parts of this function is copied from recv_listener_report since this function is kind of resume of recv_listener_report
*/
struct listaddr * process_delayed_listener_report(short ifindex , mifi_t mifi, uint32_t  listener, uint32_t mcast, u_int8_t mld_version)
{
    struct mvif *v = &mvifs[mifi];
    struct listaddr *g_lan;

    /*
     * Look for the group in our group list; if not found, just return without any process
     */
    if (!(g_lan = find_multicast_group(v, mcast)))
    {
        log_msg(LOG_ERR, 0, "[fccd] no group found for g=%s l=%s on interface %s", inetFmt(mcast), inetFmt(listener), v->mvif_name);
        return NULL;
    }
    if (!g_lan->fccd_queued)
    {
        log_msg(LOG_ERR, 0, "[fccd] inconsistence found for g=%s l=%s on interface %s, ignore!", inetFmt(mcast), inetFmt(listener), v->mvif_name);
        return NULL;
    }
    g_lan->fccd_queued = 0;

    /* Synchronize LAN and WAN/upstream databases (list of groups ) */
    mld_merge_with_upstream ( mifi , mcast, mld_version, // /merge group
                              0, // no source
                              0);   // no source filter_mode
    if ( v->uv_older_host_present == 0 ) // Sanity
    {
        v->uv_older_host_present =v->uv_mld_version;
    }

    IF_DEBUG(DEBUG_MLD_PROTO)
    log_msg(LOG_DEBUG, 0, "%s:group mode V%x differ from report V%x for group G=%s on LAN=%s is older then intervace V%x", __func__,
            g_lan->ma_comp_mode, mld_version,
            inetFmt(mcast) , v->mvif_name,v->uv_older_host_present
           );
    if ( (( v->uv_older_host_present == IGMPv3 ))  &&
            (mld_version == IGMPv2 || mld_version == IGMPv1 ) )
    {  /* downgrading from v3 to lower protocol version */
        // Switching mode from V2 to V1  RFC3810 8.3.2.  In the Presence of MLDv1 Multicast Address Listeners
        IF_DEBUG(DEBUG_MLD_PROTO)
        log_msg(LOG_DEBUG, 0, "%s:Switching mode from V% to V%d for group G=%s on LAN=%s", __func__,
                g_lan->ma_comp_mode, mld_version,
                inetFmt(mcast) , v->mvif_name);
        stop_all_sources_timers(v,g_lan); /* source timers will be restarted om timer expiration or when reports arrive */

        if ( (mld_version == IGMPv1  ) )
        {

            start_back2Igmpv2_timer(v, g_lan, MLD6_OLDER_VERSION_HOST_PRESENT);

            g_lan->ma_comp_mode = IGMPv1;
            if (g_lan->group_back2Igmpv3_timer)
                stop_back2Igmpv3_timer (v, g_lan) ; /* Only one version timer is allowed to run  */

            v->uv_older_host_present |= g_lan->ma_comp_mode;
        }
        if ( (mld_version == IGMPv2 )  )
        {

            start_back2Igmpv3_timer(v, g_lan, MLD6_OLDER_VERSION_HOST_PRESENT);

            g_lan->ma_comp_mode = IGMPv2;
            if ( g_lan->group_back2Igmpv2_timer)
                stop_back2Igmpv2_timer (v, g_lan)  ; /* Only one version timer is allowed to run  */

            v->uv_older_host_present |= g_lan->ma_comp_mode;
        }

    }
    else
    {
        if (g_lan->ma_comp_mode == IGMPv2 &&
                ( mld_version == IGMPv1 ) )
        { /* downgrading from v2 to lower protocl version */
            // Switching mode from V2 to V1  RFC3810 8.3.2.  In the Presence of MLDv1 Multicast Address Listeners
            IF_DEBUG(DEBUG_MLD_PROTO)
            log_msg(LOG_DEBUG, 0, "%s:Switching mode from V% to V%d for group G=%s on LAN=%s", __func__,
                    g_lan->ma_comp_mode, mld_version,
                    inetFmt(mcast) , v->mvif_name);

            create_back2Igmpv2_timer (v, g_lan, IgmpV1_TimerExpire_and_back_to_Higher_Igmp);	/* Create timer to wait for membership expiration */
            start_back2Igmpv2_timer(v, g_lan, MLD6_OLDER_VERSION_HOST_PRESENT);
            g_lan->ma_comp_mode = IGMPv1;
            v->uv_older_host_present |=g_lan->ma_comp_mode;
        }
    }
    v->uv_older_host_present |=g_lan->ma_comp_mode;
    return g_lan;
}

/* ARRIS ADD end */
/*
 * Process Multicast Group membership report and update LAN membership database group list
 * ifundex - input LAN interface
 * src     - sender of report
 * mcast - Multicast Group
 */
/* shared with MLDv1-compat mode in mld6v2_proto.c */
struct listaddr * recv_listener_report(short ifindex , mifi_t mifi, uint32_t   src, uint32_t mcast, u_int8_t mld_version)
{
    struct mvif *v = &mvifs[mifi];
    struct listaddr *g_lan;

    /*
     * Look for the group in our group list; if found,
     *  1) if necessary, shift to MLDv1-compat-mode
     *  2) just reset MLD-related timers (nothing special is necessary
     *     regarding compat-mode, since an MLDv2 TO_EX{NULL} message
     *     is also handled in here in the same manner as MLDv1 report).
     */
    g_lan = find_multicast_group(v, mcast);

    if (g_lan != NULL)
    {
        IF_DEBUG(DEBUG_MLD_PROTO)
        log_msg(LOG_DEBUG, 0, " Report for exiting group G_LAN=%s on %s",inetFmt(mcast) , v->mvif_name);
        inc_group_listener(g_lan, mifi, src); // ARRIS ADD
        /* the group  FOUND */
        /* stop retransmit  timers */
        stop_rxmt_timer(v, g_lan);

        start_report_timer( v, g_lan, MCAST_LISTENER_INTERVAL );

        // Prepare next retransmit when (if) leave|done message will be heard
        g_lan->ma_llqc =(v->uv_fastleave ) ? 0 :  v->uv_mld_llqc;  // Prepare retrt count of Group Specific queries
        g_lan->ma_CheckingListenerState = FALSE;

    }

    else
    {

        /* MAKE NEW GROUP , and add it to the list and update kernel cache. */


        if ( (g_lan = make_new_group(  mifi, mcast, src, mld_version)) == NULL) // ARRIS CHANGES
        {
            log_msg(LOG_ERR, 0, "make_new_group G_LAN=%s on interface %s failed", inetFmt(mcast) ,v->mvif_name );
            return;
        }
        /* ARRIS ADD START for FCCD */
        if (fccd_queue_join(g_lan, mifi, src, mld_version))
        {
            // JOIN queued, need to return
            log_msg(LOG_DEBUG, 0, "make_new_group G=%s L=%s is delayed by fccd", inetFmt(mcast), inetFmt(src));
            return g_lan;
        }
        /* ARRIS ADD END */
        /* Synchronize LAN and WAN/upstream databases (list of groups ) */
        mld_merge_with_upstream ( mifi , mcast, mld_version, // /merge group
                                  0, // no source
                                  0);	// no source filter_mode
    }
    if ( v->uv_older_host_present == 0 ) // Sanity
    {
        v->uv_older_host_present =v->uv_mld_version;
    }
    
    IF_DEBUG(DEBUG_MLD_PROTO)
    log_msg(LOG_DEBUG, 0, "%s:group mode V%x differ from report V%x for group G=%s on LAN=%s is older then intervace V%x", __func__,
            g_lan->ma_comp_mode, mld_version,
            inetFmt(mcast) , v->mvif_name,v->uv_older_host_present
           );
    if ( (( v->uv_older_host_present == IGMPv3 ))  &&
            (mld_version == IGMPv2 || mld_version == IGMPv1 ) )
    {  /* downgrading from v3 to lower protocol version */
        // Switching mode from V2 to V1  RFC3810 8.3.2.  In the Presence of MLDv1 Multicast Address Listeners
        IF_DEBUG(DEBUG_MLD_PROTO)
        log_msg(LOG_DEBUG, 0, "%s:Switching mode from V% to V%d for group G=%s on LAN=%s", __func__,
                g_lan->ma_comp_mode, mld_version,
                inetFmt(mcast) , v->mvif_name);
        stop_all_sources_timers(v,g_lan); /* source timers will be restarted om timer expiration or when reports arrive */

        if ( (mld_version == IGMPv1  ) )
        {

            start_back2Igmpv2_timer(v, g_lan, MLD6_OLDER_VERSION_HOST_PRESENT);

            g_lan->ma_comp_mode = IGMPv1;
            if (g_lan->group_back2Igmpv3_timer)
                stop_back2Igmpv3_timer (v, g_lan) ; /* Only one version timer is allowed to run  */

            v->uv_older_host_present |= g_lan->ma_comp_mode;
        }
        if ( (mld_version == IGMPv2 )  )
        {

            start_back2Igmpv3_timer(v, g_lan, MLD6_OLDER_VERSION_HOST_PRESENT);

            g_lan->ma_comp_mode = IGMPv2;
            if ( g_lan->group_back2Igmpv2_timer)
                stop_back2Igmpv2_timer (v, g_lan)  ; /* Only one version timer is allowed to run  */

            v->uv_older_host_present |= g_lan->ma_comp_mode;
        }

    }
    else
    {
        if (g_lan->ma_comp_mode == IGMPv2 &&
                ( mld_version == IGMPv1 ) )
        { /* downgrading from v2 to lower protocl version */
            // Switching mode from V2 to V1  RFC3810 8.3.2.  In the Presence of MLDv1 Multicast Address Listeners
            IF_DEBUG(DEBUG_MLD_PROTO)
            log_msg(LOG_DEBUG, 0, "%s:Switching mode from V% to V%d for group G=%s on LAN=%s", __func__,
                    g_lan->ma_comp_mode, mld_version,
                    inetFmt(mcast) , v->mvif_name);

            create_back2Igmpv2_timer (v, g_lan, IgmpV1_TimerExpire_and_back_to_Higher_Igmp);	/* Create timer to wait for membership expiration */
            start_back2Igmpv2_timer(v, g_lan, MLD6_OLDER_VERSION_HOST_PRESENT);
            g_lan->ma_comp_mode = IGMPv1;
            v->uv_older_host_present |=g_lan->ma_comp_mode;
        }
    }
    v->uv_older_host_present |=g_lan->ma_comp_mode;
    return g_lan;
}
void
recv_listener_done(
    short ifindex,
    mifi_t mifi,
    u_int32_t src,uint32_t  mcast )
{
    struct mvif *v = &mvifs[mifi];
    register struct listaddr *g;
    int ret = FALSE;


    /*
     * Look for the group in our group list in order to set up a
     * short-timeout query.
     */
    g = find_multicast_group(v,mcast);

    if ( g == NULL) {
        log_msg(LOG_DEBUG, 0, "[accept_done_message] for G=%s not exist on interface %s\n",
                inetFmt(mcast), v->mvif_name);;
        return;
    }
    IF_DEBUG(DEBUG_MLD)
    log_msg(LOG_DEBUG,0, "[accept_done_message] for G=%s\n",
            inetFmt( g->mcast_group) );

    if ( v->uv_fastleave ) // ARRIS CHANGES //PEGATRON MOD, PD57118
    {
        IF_DEBUG(DEBUG_MLD_PROTO)
        log_msg(LOG_DEBUG,0,
                "Received  done_message for G=%s on FastLeave track \n",
                inetFmt( g->mcast_group) );

        delete_group(mifi ,g, src); // Deletes on LAN, and if required on Upstream WAN too // ARRIS CHANGE
        return;
    }

    //PEGATRON ADD START, PD57118. 
    //When user channel switch, it would send leave and join. 
    //If leave immediately, DBC-REQ would handle leave first and then wait 3 secs to handle join in next DBC-REQ
    //To reduce user waitting, need to delay leave and make it behind of join packet.
    if ( !dec_group_listener(g, mifi, src) )
    {
        IF_DEBUG(DEBUG_MLD_PROTO)
        log_msg(LOG_DEBUG,0,
                "Received  done_message for G=%s on soft FastLeave track. delay %lu ns \n",
                inetFmt( g->mcast_group), v->uv_leave_delay_time);
        if ( v->uv_leave_delay_time > 0 ) //delay leave package
        {
            g->ma_llqc = 1;
            start_rxmt_timer_fastLeave ( v, g, v->uv_leave_delay_time);
        }
        else //fast leave
        {
            delete_group(mifi ,g, src);
        }
        return;
    }   
    //PEGATRON ADD END, PD57118
    /* still waiting for a reply to a query, ignore the done */
    if  (  g->ma_CheckingListenerState == TRUE  )
    {
        IF_DEBUG(DEBUG_MLD)
        log_msg(LOG_DEBUG,0, "Ignoring repeated done_message for G=%s\n",
                inetFmt((g->mcast_group)));
        return;
    }
    g->ma_llqc=MCAST_LAST_LISTENER_QUERY_COUNT;
    sendGroupSpecificMemberQuery( v, g);

    v->uv_out_mld_query++;

    start_report_timer (v, g,  MCAST_LAST_LISTENER_QUERY_TIMER);               // Lower report timer

    start_rxmt_timer ( v, g, MCAST_LAST_LISTENER_QUERY_INTERVAL );

}


