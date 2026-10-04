/*
  GPL LICENSE SUMMARY

  Copyright(c) 2014-2017 Intel Corporation.

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

#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/netdevice.h>
#include <linux/netlink.h>
#include <linux/inetdevice.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/if_ether.h>
#include "../../8021q/vlan.h"
#include <linux/if_pppox.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/icmp.h>
#include <linux/icmpv6.h>
#include <linux/ppp_defs.h>
#include <asm-arm/arch-avalanche/generic/pal_cppi41.h>
#include <linux/ti_hil.h>
#include <linux/mroute.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include "../../bridge/br_private.h"
#include <linux/proc_fs.h>
#include <net/sock.h> 
#include <linux/etherdevice.h>
#include <asm-arm/arch-avalanche/generic/pal.h>
#include "linux/ti_pp_path.h"

#include <mach/puma.h>
#include <mach/hardware.h>


#include <asm-arm/arch-avalanche/puma6/puma6_pp.h>

#ifndef TI_MAX_DEVICE_INDEX
#define TI_MAX_DEVICE_INDEX 64
#endif




#ifdef CONFIG_NETFILTER
#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#endif

#include "linux/cat_l2switch_netdev.h"

//-------------------------------------------------------------------
//
// ########  ######## ######## #### ##    ## ########  ######
// ##     ## ##       ##        ##  ###   ## ##       ##    ##
// ##     ## ##       ##        ##  ####  ## ##       ##
// ##     ## ######   ######    ##  ## ## ## ######    ######
// ##     ## ##       ##        ##  ##  #### ##             ##
// ##     ## ##       ##        ##  ##   ### ##       ##    ##
// ########  ######## ##       #### ##    ## ########  ######
//
//-------------------------------------------------------------------

#define IPV6_TRAFFIC_CLASS(h)                       (((h->priority & 0x0F) << 4) | ((h->flow_lbl[0] & 0xF0) >> 4))
#define DEFAULT_SESSION_TIMEOUT_SEC                 1
#define DEFAULT_ICMP_SESSION_TIMEOUT_SEC            2
#define DEFAULT_TDOX_TIMEOUT_MSEC                   3000
#define MAX_DDH_NOTIFICATION_MESSAGE_LENGTH         256

#define DOCSIS_FW_PACKET_API_TCP_HIGH_PRIORITY      (1 << 10)

extern unsigned int nf_ct_udp_rtsp_timeout_stream;
/* GEN PP dropped packest, used with TI_CT_GEN_DISCARD_PKT. This is a bitmap. User sets bits according to protocol : IPv4/IPv6 */
#define DROPPED_PACKETS_BITMAP_SET(__n)             do { dropped_packets_bit_map |= (1 << (__n)); } while (0)
#define DROPPED_PACKETS_BITMAP_UNSET(__n)           do { dropped_packets_bit_map &= ~(1 << (__n)); } while (0)
#define DROPPED_PACKETS_BITMAP_IS_SET(__n)          ((dropped_packets_bit_map & (1 << (__n))) != 0)

/************************************************************************/
/*  TCP Options Numbers                                                 */
/************************************************************************/
#define TCP_OPTION_CODE_EOL         0  // 0  End of Option List                     [RFC793]
#define TCP_OPTION_CODE_NOP         1  // 1  No-Operation                           [RFC793]
                                       // 2  Maximum Segment Size                   [RFC793]
                                       // 3  WSOPT - Window Scale                   [RFC1323]
                                       // 4  SACK Permitted                         [RFC2018]
                                       // 5  SACK                                   [RFC2018]
                                       // 6  Echo (obsoleted by option 8)           [RFC1072]
                                       // 7  Echo Reply (obsoleted by option 8)     [RFC1072]
#define TCP_OPTION_CODE_TIMESTAMP   8  // 8  TSOPT - Time Stamp Option              [RFC1323]
                                       // 9  Partial Order Connection Permitted     [RFC1693]
                                       // 10 Partial Order Service Profile          [RFC1693]
                                       // 11 CC                                     [RFC1644]
                                       // 12 CC.NEW                                 [RFC1644]
                                       // 13 CC.ECHO                                [RFC1644]
                                       // 14 TCP Alternate Checksum Request         [RFC1146]
                                       // 15 TCP Alternate Checksum Data            [RFC1146]
                                       // 16 Skeeter                                [Knowles]
                                       // 17 Bubba                                  [Knowles]
                                       // 18 Trailer Checksum Option                [Subbu & Monroe]
                                       // 19 MD5 Signature Option                   [RFC2385]
                                       // 20 SCPS Capabilities                      [Scott]
                                       // 21 Selective Negative Acknowledgements    [Scott]
                                       // 22 Record Boundaries                      [Scott]
                                       // 23 Corruption experienced                 [Scott]
                                       // 24 SNAP                                   [Sukonnik]
                                       // 25 Unassigned (released 2000-12-18)
                                       // 26 TCP Compression Filter                 [Bellovin]
                                       // 27 Quick-Start Response                   [RFC4782]
                                       // 28 User Timeout Option                    [RFC-ietf-tcpm-tcp-uto-11.txt]

#define DDH_DBG
// #define BITHASH_DBG
// #define HIL_DBG
// #define SCB_DBG

#ifdef HIL_DBG
/* Debug print, also print function name and line number */
#define DBGPRINTK(fmt, args...) printk("[HIL DBG] %s(%d): " fmt "\n", __FUNCTION__ , __LINE__, ## args)
#else
#define DBGPRINTK(fmt, args...)
#endif

#ifdef BITHASH_DBG
/* Debug print, also print function name and line number */
#define BHPRINTK(fmt, args...) printk("[BH DBG] %s(%d): " fmt "\n", __FUNCTION__ , __LINE__, ## args)
#else
#define BHPRINTK(fmt, args...)
#endif

#ifdef SCB_DBG
/* Debug print, also print function name and line number */
#define SCBPRINTK(fmt, args...) printk("[SCB DBG] %s(%d): " fmt "\n", __FUNCTION__ , __LINE__, ## args)
#else
#define SCBPRINTK(fmt, args...)
#endif

//----------------------------------------------------------------------------------------------
//
//  ######  ######## ########  ##     ##  ######  ######## ##     ## ########  ########  ######
// ##    ##    ##    ##     ## ##     ## ##    ##    ##    ##     ## ##     ## ##       ##    ##
// ##          ##    ##     ## ##     ## ##          ##    ##     ## ##     ## ##       ##
//  ######     ##    ########  ##     ## ##          ##    ##     ## ########  ######    ######
//       ##    ##    ##   ##   ##     ## ##          ##    ##     ## ##   ##   ##             ##
// ##    ##    ##    ##    ##  ##     ## ##    ##    ##    ##     ## ##    ##  ##       ##    ##
//  ######     ##    ##     ##  #######   ######     ##     #######  ##     ## ########  ######
//
//----------------------------------------------------------------------------------------------

#define PP_COUNTER_UDP_MASK        4
#define PP_COUNTER_TCP_MASK        5
#define PP_COUNTER_IPSEC_MASK      10
#define PP_COUNTER_INNER_GRE_MASK  24

typedef enum
{
    PP_IP_PROT_NA        =  0,    
    PP_IP_PROT_GRE       =  BIT(0),             
    PP_IP_PROT_DSLITE    =  BIT(1),            
    PP_IP_PROT_UDP       =  BIT(2),            
    PP_IP_PROT_TCP       =  BIT(3),    
    PP_IP_PROT_IPSEC     =  BIT(4),             
    PP_IP_PROT_INNER_GRE =  BIT(5)                 
}
hil_pp_ip_prot_e;

#define PP_COUNTER_TCP        ((PP_IP_PROT_TCP) - PP_COUNTER_TCP_MASK) 
#define PP_COUNTER_UDP        ((PP_IP_PROT_UDP) - PP_COUNTER_UDP_MASK) 
#define PP_COUNTER_IPSEC      ((PP_IP_PROT_IPSEC) - PP_COUNTER_IPSEC_MASK) 
#define PP_COUNTER_INNER_GRE  ((PP_IP_PROT_INNER_GRE) - PP_COUNTER_INNER_GRE_MASK) 

typedef enum
{
     udp_pkts          = 0,     /*  PP_IP_PROT_UDP - PP_COUNTER_UDP_MASK =  4-4 = 0 */
     gre_udp_pkts      = 1,     /* (PP_IP_PROT_UDP + PP_IP_PROT_GRE) - PP_COUNTER_UDP_MASK =  5-4 = 1 */ 
     dslite_udp_pkts   = 2,     /* (PP_IP_PROT_UDP + PP_IP_PROT_DSLITE) - PP_COUNTER_UDP_MASK =  6-4 = 2 */ 

     tcp_pkts          = 3,    /*  PP_IP_PROT_TCP -  PP_COUNTER_TCP_MASK =  8-5 = 3 */                           
     gre_tcp_pkts      = 4,    /* (PP_IP_PROT_TCP +  PP_IP_PROT_GRE) - PP_COUNTER_TCP_MASK =  9-5 = 4 */      
     dslite_tcp_pkts   = 5,    /* (PP_IP_PROT_TCP +  PP_IP_PROT_DSLITE) - PP_COUNTER_TCP_MASK =  10-5 = 5 */   

     ipsec_pkts        = 6,   /*  PP_IP_PROT_IPSEC - PP_COUNTER_IPSEC_MASK =  16-10 = 6 */                        
     gre_ipsec_pkts    = 7,   /* (PP_IP_PROT_IPSEC + PP_IP_PROT_GRE) - PP_COUNTER_IPSEC_MASK =  17-10 = 7 */      
     dslite_ipsec_pkts = 8,   /* (PP_IP_PROT_IPSEC + PP_IP_PROT_DSLITE) - PP_COUNTER_IPSEC_MASK =  18-10 = 8 */  

     gre_gre_pkts      = 9,    /* (PP_IP_PROT_INNER_GRE + PP_IP_PROT_GRE) - PP_COUNTER_INNER_GRE_MASK =  33 - 24 = 9 */ 
     max_pkts_counters = 10
}
hil_pkts_counter_e;

typedef struct
{
    Bool    hil_disabled;
    Bool    tdox_disabled;
    Bool    tdox_dbg;
    Bool    discard_OOO;
    Bool    extended_session_timeout;
    Bool    qos_disabled;
    Bool    ipv6rd_pp_support;
    Bool    ping_acceleration;
    Bool    tdox_timer_default;

    Bool	is_gateway;
    Bool    is_hybrid;
    Bool	reserved1;
    Bool	reserved2;

    Uint32  tdoxEvalTimeMsec;
    Uint32  tdoxEvalMaxSessionsTimer;
    Uint32  tdoxEvalMidSessionsTimer;
    Uint32  tdoxEvalMinSessionsTimer;
    Uint32  tdoxEvalMaxSessions;
    Uint32  tdoxEvalMinSessions;

    Uint32  tdoxEvalAvgPktSizeThresh;
    Uint32  tdoxEvalAvgBPTunnelPktSizeThresh;
    Uint32  tdoxEvalPpsThresh;
    

    Uint32  num_bypassed_pkts;
    Uint32  num_other_pkts;
    Uint32  num_ingress_pkts;

    Uint32  ingress_counters_ipProt_pkts[max_pkts_counters];

    Uint32  num_egress_pkts;
    Uint32  num_egress_pkt_fail_extract;
    Uint32  num_egress_pkt_tcp_syn;
    Uint32  num_egress_pkt_gre_bypassed;
    Uint32  num_egress_pkt_fail_sess_int;
    Uint32  num_egress_pkt_scb;
    Uint32  egress_counters_ipProt_pkts[max_pkts_counters];

    Uint32  num_null_drop_pkts;

    Uint32  num_total_sessions;
    Uint32  num_error;
    Uint32  extended_timeout_MaxSessions;

    /* DDH Variables*/
    Uint32  ddh_consecutive_high_pps_ctr;    /* Counts number of seconds we got high pps */
    Uint32  ddh_consecutive_low_pps_ctr;     /* Counts number of seconds we got low pps */
    Uint32  ddh_consecutive_high_sps_ctr;    /* Counts number of seconds we got high sps */
    Uint32  ddh_consecutive_low_sps_ctr;     /* Counts number of seconds we got low sps */
    Uint32  ddh_host_forwarded_packets;      /* packets forwarded to host */
    Uint32  ddh_last_cycle_host_packets;     /* last cycle packets forwarded to host */
    Uint32  ddh_last_cycle_created_sessions; /* last cycle created sessions */
    Uint32  ddh_l2_classification_pid;       /* All packets received from this PID will be classified with L2 only (DS-Defensive mode) */    
    Uint32  ddh_num_scb_packets_threshold;   /* Threshold to determine how many packets should bypass egress hook */
    Uint32  ddh_num_scb_connections_threshold;/* Threshold to determine how many scb connections should be active until we trigger ddh */
    Uint32  ddh_num_scb_hash_collisions;     /* Number of SCB hash collisions */
    Uint32  ddh_active_short_connections;    /* Active SCB connections */
    Uint32  ddh_max_active_short_connections;/* Max active SCB connections */
    Uint32  ddh_zombie_short_connections;    /* Zombie SCB connections */
    unsigned long ddh_cycle_duration_jiffies;
    unsigned long ddh_last_timer_tick_jiffies;
}
global_hil_db_t;

typedef struct
{
    /* Mark if the previous notificsation was about attack\normal state. */
    AVALANCHE_PP_DDH_STATE_e    previous_notified_state;
    AVALANCHE_PP_DDH_STATE_e    previous_state;
    /* 
     * Count how many samples found the system in the current state.
	 * If we sample the system in different state, the counter is zero. 
	 * If we sample the system in the current state X times, notify.  .
	 */
    Uint32                      sampling_in_current_state_counter;
}
ddh_notifications_t;
//-----------------------------------------------------------------------------------------------------------------------------------------------
//
// ########  ########   #######  ########  #######  ######## ##    ## ########  ########  ######
// ##     ## ##     ## ##     ##    ##    ##     ##    ##     ##  ##  ##     ## ##       ##    ##
// ##     ## ##     ## ##     ##    ##    ##     ##    ##      ####   ##     ## ##       ##
// ########  ########  ##     ##    ##    ##     ##    ##       ##    ########  ######    ######
// ##        ##   ##   ##     ##    ##    ##     ##    ##       ##    ##        ##             ##
// ##        ##    ##  ##     ##    ##    ##     ##    ##       ##    ##        ##       ##    ##
// ##        ##     ##  #######     ##     #######     ##       ##    ##        ########  ######
//
//-----------------------------------------------------------------------------------------------------------------------------------------------

/************************************************/
/* HIL Profile Functions                        */
/************************************************/
static Int32 hil_intrusive_init(void);
static Int32 hil_intrusive_deinit(void);
static int hil_netsubsystem_event_handler(unsigned int module_id, unsigned long event_id, void* ptr);
static Int32 hil_netsubsystem_pp_handler(Uint32 event_id, void* ptr);
static AVALANCHE_PP_RET_e hil_pp_event_handler(AVALANCHE_PP_EVENT_e event_id, Uint32 param1, Uint32 param2);

/************************************************/
/* Ingress Hook Functions                       */
/************************************************/
Int32 hil_ingress_hook(struct sk_buff* skb);
static Int32 hil_extract_packet_ingress(struct sk_buff* skb, struct net_device* dev);
static Int32 hil_extract_l2_ingress(struct ethhdr* ptr_ethhdr, AVALANCHE_PP_INGRESS_SESSION_PROPERTY_t* ingress_property, Uint16 vpid_vlan_tci);
static Int32 hil_extract_ipv4_ingress(struct iphdr* ptr_iphdr,
                                        void** ptr_l4hdr,
                                        AVALANCHE_PP_INGRESS_SESSION_PROPERTY_t* ingress_property,
                                        Bool acceptFragL3,
                                        hil_pp_ip_prot_e ExtTunnel);
static Int32 hil_extract_ipv6_ingress(struct ipv6hdr* ptr_ipv6hdr,
                                        void** ptr_l4hdr,
                                        AVALANCHE_PP_INGRESS_SESSION_PROPERTY_t* ingress_property,
                                        Bool acceptFragL3,
                                        hil_pp_ip_prot_e ExtTunnel);
static Int32 hil_extract_l4_ingress(void* ptr_l4hdr, AVALANCHE_PP_INGRESS_SESSION_PROPERTY_t* ingress_property);

/************************************************/
/* Egress Hook Functions                        */
/************************************************/
Int32 hil_egress_hook(struct sk_buff* skb);
static Int32 hil_extract_packet_egress(struct sk_buff* skb, struct net_device* dev, AVALANCHE_PP_SESSION_INFO_t* session_info);
static Int32 hil_extract_l2_egress(struct ethhdr* ptr_ethhdr, struct sk_buff* skb, AVALANCHE_PP_EGRESS_SESSION_PROPERTY_t* egress_property);
static Int32 hil_extract_ipv4_egress(struct ethhdr* ptr_ethhdr,
                                        struct iphdr* ptr_iphdr,
                                        void** ptr_l4hdr,
                                        AVALANCHE_PP_SESSION_INFO_t* session_info,
                                        Bool acceptFragL3);
static Int32 hil_extract_ipv6_egress(struct ethhdr* ptr_ethhdr,
                                        struct ipv6hdr* ptr_ipv6hdr,
                                        void** ptr_l4hdr,
                                        struct iphdr** ptr_dsLiteIphdr,
                                        AVALANCHE_PP_SESSION_INFO_t* session_info,
                                        Bool acceptFragL3);
static Int32 hil_extract_l4_egress(void * ptr_l4hdr, struct iphdr* ptr_iphdr, struct ipv6hdr* ptr_ipv6hdr, AVALANCHE_PP_EGRESS_SESSION_PROPERTY_t* egress_property);
static Int32 hil_session_intelligence(AVALANCHE_PP_SESSION_INFO_t* session_info);
static Int32 hil_choose_session_pool(AVALANCHE_PP_SESSION_INFO_t* session_info);
static Int32 hil_choose_session_timeout(AVALANCHE_PP_SESSION_INFO_t* session_info);
/************************************************/
/* NULL Hook Functions                          */
/************************************************/
Int32 hil_null_hook(struct sk_buff* skb, Int32 null_vpid);

/************************************************/
/* General Packet Parsing Functions             */
/************************************************/
static Int32 hil_get_layers_pointers(Int8* ptr_data, struct ethhdr** ptr_ethhdr, struct pppoe_hdr** ptr_pppoe, struct iphdr** ptr_iphdr, struct ipv6hdr** ptr_ipv6hdr);
static Int32 hil_check_ip_protocol(Uint16 ip_protocol, Bool ingress_pkt, hil_pp_ip_prot_e ExtTunnel,  Bool updateCounter);
static Uint16 hil_ipv4_checksum(Uint8 *ipv4Header, Uint16 ipv4HeaderLen);
static Int32 hil_scan_ipv6(struct ipv6hdr* ptr_ipv6hdr, Uint8* ipv6HeaderLen, Uint8* nexthdr, Int16 * fragOffs, Uint8 * FragHdrOffs, Uint8 * PrevFragHdrOffs, hil_pp_ip_prot_e ExtTunnel, Bool Ingress_pkt, Bool updateCounter);

/************************************************/
/* MFC Functions                                */
/************************************************/
#ifdef CONFIG_IP_MULTICAST
static Int32 hil_mfc_delete_session_by_mc_group(Int32 mc_group);
#endif /* CONFIG_IP_MULTICAST */


/************************************************/
/* TDOX Functions                               */
/************************************************/
static Int32 hil_tdox_check_and_enable(void * ptr_l4hdr, AVALANCHE_PP_SESSION_INFO_t* session_info);
static void  hil_check_and_update_TdoxEvalTime(void);
AVALANCHE_PP_RET_e hil_tdox_manager(AVALANCHE_PP_SESSION_INFO_t *session_info, Ptr data);

/************************************************/
/* PROC Functions                               */
/************************************************/
static int hil_write_cmds(struct file *file, const char *buffer, unsigned long  count, void *data);
static Int32 hil_show_cmd_handler(Int32 argc, Int8* argv[]);
static Int32 hil_set_cmd_handler(Int32 argc, Int8* argv[]);
static Int32 hil_reset_cmd_handler(Int32 argc, Int8* argv[]);
static int hil_read_devs(char* buf, char **start, off_t offset, int count, int *eof, void *data);
static void hil_test_session_creation(void);
void hil_delete_drop_session(Uint32 src_ip,Uint32 dst_ip,Uint16 src_port, Uint16 dst_port) ;
static void pushPacketToPrefetcher(int direction);

/************************************************/
/* Connection Tracking Function                 */
/************************************************/
#ifdef CONFIG_NETFILTER
static void hil_add_session_handle_to_ct_mapper_list (Uint16 new_session_handle,Uint16 prev_new_session_handle);
static Bool hil_find_session_handle_in_ct_mapper_list (Uint16 ct_session_handle,Uint16 pp_session_handle);
static void hil_delete_session_handle_in_ct_mapper_list (Uint16 pp_session_handle,enum ip_conntrack_dir dir);
#endif

#if defined(CONFIG_NF_CONNTRACK) || defined(CONFIG_NF_CONNTRACK_MODULE)
static void __hil_calc_bithash (struct sk_buff* skb);
static void __hil_scb_add (struct sk_buff* skb);
static void __hil_scb_remove(struct sk_buff* skb);
static void __hil_scb_timeout(struct nf_conn* conntrack);
static void __hil_scb_attach_conntrack(struct sk_buff* skb, struct nf_conn* conntrack);
static Bool __hil_scb_is_egress_bypass (struct sk_buff* skb);
#endif

/************************************************/
/* DDH Functions                                */
/************************************************/
static void __hil_ddh_init();
static Bool __hil_ddh_process(struct sk_buff* skb);
static void __hil_ddh_event();
static Uint32 __hil_ddh_calc_host_pps();
static Uint32 __hil_ddh_calc_session_rate();
static Uint32 __hil_ddh_calc_object_per_second(Uint32* previous_object_count, Uint32* current_object_count);
static void __hil_calc_mac_hash(AVALANCHE_PP_SESSION_INFO_t *session_info, Bool add_operation);
static void __hil_ddh_reset_pps_counters();

#define IS_BRIDGE()    (!global_hil_db.is_hybrid)
#define IS_HYBRID()    (global_hil_db.is_hybrid)
#define IS_GW()        (False)

/************************************************/
/* extern Functions                             */
/************************************************/
extern Int32 ti_register_egress_hook_handler(struct net_device* dev, Int32 (*egress_hook)(struct sk_buff *skb));
extern Int32 ti_deregister_egress_hook_handler(struct net_device* dev);
extern Uint32 pp_db_hash(register Uint8 *k, register Uint32  length, register Uint32  initval, Uint32 result_modulo);
extern AVALANCHE_PP_RET_e   avalanche_pp_local_dev_addr(avalanche_pp_local_dev_addr_ioctl_params_t *param);
extern AVALANCHE_PP_RET_e   avalanche_pp_update_bithash(Uint16 hash, Bool add_operation);
extern AVALANCHE_PP_RET_e   avalanche_pp_flush_bithash(void);
extern AVALANCHE_PP_RET_e   avalanche_pp_get_scb_entry(AVALANCHE_PP_SCB_Entry_t **scb_entry, Uint32 entry_index);
extern AVALANCHE_PP_RET_e   avalanche_pp_flush_scb_db();

#ifdef CONFIG_TI_PACKET_PROCESSOR_STATS
extern TI_HIL_START_SESSION ti_hil_start_session_notification_cb;
extern TI_HIL_DELETE_SESSION ti_hil_delete_session_notification_cb;
#endif


//-----------------------------------------------------------------------------------------------------------------------------------------------
//
//  ######   ##        #######  ########     ###    ##          ##     ##    ###    ########  ####    ###    ########  ##       ########  ######
// ##    ##  ##       ##     ## ##     ##   ## ##   ##          ##     ##   ## ##   ##     ##  ##    ## ##   ##     ## ##       ##       ##    ##
// ##        ##       ##     ## ##     ##  ##   ##  ##          ##     ##  ##   ##  ##     ##  ##   ##   ##  ##     ## ##       ##       ##
// ##   #### ##       ##     ## ########  ##     ## ##          ##     ## ##     ## ########   ##  ##     ## ########  ##       ######    ######
// ##    ##  ##       ##     ## ##     ## ######### ##           ##   ##  ######### ##   ##    ##  ######### ##     ## ##       ##             ##
// ##    ##  ##       ##     ## ##     ## ##     ## ##            ## ##   ##     ## ##    ##   ##  ##     ## ##     ## ##       ##       ##    ##
//  ######   ########  #######  ########  ##     ## ########       ###    ##     ## ##     ## #### ##     ## ########  ######## ########  ######
//
//-----------------------------------------------------------------------------------------------------------------------------------------------
Int32 pp_session_timeout_sec = DEFAULT_SESSION_TIMEOUT_SEC;   /* This is the default session IDLE timeout in seconds. */

Uint16 g_conf_ip_protocol = 0;
bool g_conf_ip_prot_valid = False;

global_hil_db_t global_hil_db =
{
    .hil_disabled               =   False,
    .tdox_disabled              =   False,
    .ipv6rd_pp_support             =   False,
    /* This is not recommended option to disable the discard_OOO (can impact docsis cert by causing out of order at the beginning of sessions) */
    .discard_OOO                      =   True,
    .extended_session_timeout         =   True,
    .tdox_dbg                         =   False,
    .qos_disabled                     =   False,
    .tdox_timer_default               =   False,
    .ping_acceleration                =   True,
    .is_hybrid                        =   False,

    .tdoxEvalTimeMsec                 =   DEFAULT_TDOX_TIMEOUT_MSEC,  /* Time in Msec */
    .tdoxEvalMaxSessionsTimer         =   DEFAULT_TDOX_TIMEOUT_MSEC,  /* Time in Msec */
    .tdoxEvalMidSessionsTimer         =   1000,                       /* Time in Msec */
    .tdoxEvalMinSessionsTimer         =   100,                        /* Time in Msec */
    .tdoxEvalMinSessions              =   32,                         /* Number of sessions */
    .tdoxEvalMaxSessions              =   512,                        /* Number of sessions */

    .tdoxEvalAvgPktSizeThresh         =   128,
    .tdoxEvalAvgBPTunnelPktSizeThresh =   200,
    .tdoxEvalPpsThresh                =   100,
    .num_bypassed_pkts                =   0,
    .num_other_pkts                   =   0,
    .num_ingress_pkts                 =   0,
    .ingress_counters_ipProt_pkts     =   {0},
    .num_egress_pkts                  =   0,
    .num_egress_pkt_fail_extract      =   0,
    .num_egress_pkt_tcp_syn           =   0,    
    .num_egress_pkt_gre_bypassed      =   0,
    .num_egress_pkt_fail_sess_int     =   0,
    .num_egress_pkt_scb               =   0,
    .egress_counters_ipProt_pkts      =   {0},
    .num_null_drop_pkts               =   0,
    .num_total_sessions               =   0,
    .extended_timeout_MaxSessions     =   128,
                                          
                                          
    .num_error                        = 0,
    .ddh_host_forwarded_packets       = 0,
    .ddh_last_cycle_host_packets      = 0,
    .ddh_last_cycle_created_sessions  = 0,
    .ddh_cycle_duration_jiffies       = 0,
    .ddh_consecutive_high_pps_ctr     = 0,
    .ddh_consecutive_low_pps_ctr      = 0,
    .ddh_consecutive_high_sps_ctr     = 0,
    .ddh_consecutive_low_sps_ctr      = 0,
    .ddh_num_scb_packets_threshold    = DDH_NUM_SHORT_CONNECTION_PACKETS_THRESHOLD,
    .ddh_num_scb_connections_threshold= DDH_NUM_SHORT_CONNECTIONS_THRESHOLD,
    .ddh_active_short_connections     = 0,
    .ddh_num_scb_hash_collisions      = 0,
    .ddh_max_active_short_connections = 0,
    .ddh_zombie_short_connections     = 0,
    .ddh_l2_classification_pid        = PP_CNI_PID_BASE
};

ddh_notifications_t    ddh_notify = 
{
    .previous_notified_state = AVALANCHE_PP_DDH_STATE_NORMAL,
	.previous_state = AVALANCHE_PP_DDH_STATE_NORMAL,
    .sampling_in_current_state_counter = 0,
};

/* Default Profile. */
TI_HIL_PROFILE hil_intrusive_profile = 
{
    .name            = "intrusive_pp2k",
    .profile_handler = hil_netsubsystem_event_handler,
    .profile_init    = hil_intrusive_init,
    .profile_deinit  = hil_intrusive_deinit,
};

Uint32 pp_event_handler;

#ifdef CONFIG_NETFILTER
/* This is a global data structure which maps the session handle to corresponding connection tracking entries. */
struct ct_mapper
{
    Uint16 next;
    Uint16 previous;
    struct nf_conn* conntrack;

};
struct ct_mapper hil_session_ct_mapper [AVALANCHE_PP_MAX_ACCELERATED_SESSIONS];
#endif

/* Packet terminating VPID handle */
static Int32  docsis_vpid_handle = -1;
static Int32  docsis_null_vpid_handle = -1;
static Int32  netfilter_null_vpid_handle = -1;

static Int32 dropped_packets_bit_map = 0;

static Uint8   *usPktDataIngressP = NULL;
static Uint16   usPktDataIngressSize = 0;
static Uint8   *usPktDataEgressP = NULL;
static Uint16   usPktDataEgressSize = 0;
static Uint8   *dsPktDataIngressP = NULL;
static Uint16   dsPktDataIngressSize = 0;
static Uint8   *dsPktDataEgressP = NULL;
static Uint16   dsPktDataEgressSize = 0;

/* global netlink socket structure for DDH notifications. */
static struct sock*    netlink_socket = NULL;
/* The user application pid (proccess ID) to send DDH notifications. */
static pid_t           user_application_pid = -1;

//-----------------------------------------------------------------------------------------------------------------------------------------------
//
// ##        #######   ######     ###    ##          ######## ##     ## ##    ##  ######  ######## ####  #######  ##    ##  ######
// ##       ##     ## ##    ##   ## ##   ##          ##       ##     ## ###   ## ##    ##    ##     ##  ##     ## ###   ## ##    ##
// ##       ##     ## ##        ##   ##  ##          ##       ##     ## ####  ## ##          ##     ##  ##     ## ####  ## ##
// ##       ##     ## ##       ##     ## ##          ######   ##     ## ## ## ## ##          ##     ##  ##     ## ## ## ##  ######
// ##       ##     ## ##       ######### ##          ##       ##     ## ##  #### ##          ##     ##  ##     ## ##  ####       ##
// ##       ##     ## ##    ## ##     ## ##          ##       ##     ## ##   ### ##    ##    ##     ##  ##     ## ##   ### ##    ##
// ########  #######   ######  ##     ## ########    ##        #######  ##    ##  ######     ##    ####  #######  ##    ##  ######
//
//-----------------------------------------------------------------------------------------------------------------------------------------------
/**************************************************************************
 * FUNCTION NAME : netlink_receive_message
 **************************************************************************
 * DESCRIPTION:
 *  Callback for DDH notifications netlink socket. The Purpose of the
 *  function is to set the pid (proccess ID) of the user application to
 *  notify, so the kernel will can send the information.
 *
 * RETURNS:
 * void
 **************************************************************************/
static void netlink_receive_message(struct sk_buff *skb)
{
    struct nlmsghdr*    netlink_header = (struct nlmsghdr *)skb->data;

    /* Save the pid of the sending process */
    user_application_pid = netlink_header->nlmsg_pid;
}

/**************************************************************************
 * FUNCTION NAME : hil_intrusive_init
 **************************************************************************
 * DESCRIPTION:
 *  Initialization function for the PP2K Intrusive mode profile.
 *
 * RETURNS:
 *  0   -   Success
 *  <0  -   Error
 **************************************************************************/
static Int32 hil_intrusive_init(void)
{
    struct proc_dir_entry*  ptr_dir_entry;
    struct proc_dir_entry*  ptr_dev_entry;
#ifdef CONFIG_NETFILTER
    int                     index = 0;
#endif

    /* Init ddh */
    __hil_ddh_init();

    /* Register an event handler to listen to events. */
    if (avalanche_pp_event_handler_register(&pp_event_handler, hil_pp_event_handler) != PP_RC_SUCCESS)
    {
        printk ("Error: Event Handler register failed\n");
        return -1;
    }

    /* Create the PROC Entry used by the TCA Configuration Engine. */
    ptr_dir_entry = create_proc_entry("ti_pp" ,0644, init_net.proc_net);
    ptr_dev_entry = create_proc_entry("ti_pp_dev" ,0644, init_net.proc_net);

    if( (ptr_dir_entry == NULL) || (ptr_dev_entry == NULL ) )
    {
        printk ("Error: Unable to create Packet Processor proc entry.\n");
        return -1;
    }
    ptr_dir_entry->data      = NULL;
    ptr_dir_entry->read_proc  = NULL;
    ptr_dir_entry->write_proc = hil_write_cmds;


    ptr_dev_entry->data      = NULL;
    ptr_dev_entry->read_proc  = hil_read_devs;
    ptr_dev_entry->write_proc = NULL;;

#ifdef CONFIG_NETFILTER
    /* Initialize the HIL Session to Connection Tracking Mapper. */
    for (index = 0; index < AVALANCHE_PP_MAX_ACCELERATED_SESSIONS; index++)
    {
        hil_session_ct_mapper[index].conntrack = NULL;
        hil_session_ct_mapper[index].next = TI_PP_SESSION_CT_IDLE;
        hil_session_ct_mapper[index].previous = TI_PP_SESSION_CT_IDLE;
    }
#endif /* CONFIG_NETFILTER */

    /* Initialize PP Path */
    PPP_Init();

    FREE_RUNNING_COUNTER_ENABLE();
       
    /* Open netlink socket for DDH notifications */
    printk(KERN_INFO "Open netlink socket for DDH notifications.\n");

    netlink_socket = netlink_kernel_create(&init_net, NETLINK_DDH_NOTIFICATIONS, 0, netlink_receive_message, NULL, THIS_MODULE);
    if (!netlink_socket)
    {
		printk(KERN_ERR "Error: Failed to create netlink socket for DDH notifications.\n");
        return -ECHILD;
    }

    printk(KERN_INFO "Netlink socket for DDH notifications opened.\n");

    /* Profile has been successfully initialized. */
    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_intrusive_deinit
 **************************************************************************
 * DESCRIPTION   :
 *  Deinitialization function which deinitializes and unregisters the default
 *  profile with the HIL Core.
 *
 * RETURNS       :
 *  0   -   Success
 *  <0  -   Error
 **************************************************************************/
static Int32 hil_intrusive_deinit(void)
{
    if (netlink_socket)
    {
        /* Close DDH notifications netlink socket */
        printk(KERN_INFO "Close netlink socket for DDH notifications\n");
        netlink_kernel_release(netlink_socket);
    }

    return 0;
}

#ifdef CONFIG_NETFILTER
AVALANCHE_PP_RET_e    __hil_delete_drop_session( AVALANCHE_PP_SESSION_INFO_t *  ptr_session, Ptr     data )
{
    int verdict = 0;
    struct nf_conn *    conntrack = (struct nf_conn *)data;

    Uint32  ip_cmp_size = 4;

    /* ARRIS CHANGE START */
    /* Strict check, only let IPv4 & IPv6 fall through */
    if ((AVALANCHE_PP_LUT_ENTRY_L3_IPV4 != ptr_session->ingress.lookup.LUT1.u.fields.L3.entry_type)
        && (AVALANCHE_PP_LUT_ENTRY_L3_IPV6 != ptr_session->ingress.lookup.LUT1.u.fields.L3.entry_type
           || conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.src.l3num != PF_INET6))
    {
        return (PP_RC_SUCCESS);
    }

    if (AVALANCHE_PP_PID_TYPE_DOCSIS == ptr_session->ingress.pid_type)
    {
        if (AVALANCHE_PP_LUT_ENTRY_L3_IPV6 == ptr_session->ingress.lookup.LUT1.u.fields.L3.entry_type)
        {
            verdict |= ipv6_addr_equal(&conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.in6, (struct in6_addr*)ptr_session->ingress.lookup.LUT2.u.fields.WAN_addr_IP.v6);
            verdict |= ipv6_addr_equal(&conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip6, (struct in6_addr*)ptr_session->ingress.lookup.LUT1.u.fields.L3.LAN_addr_IP.v6);
        }
        else
        {
            verdict |= memcmp( &conntrack->tuplehash[ IP_CT_DIR_REPLY ].tuple.src.u3.ip,   &ptr_session->ingress.lookup.LUT2.u.fields.WAN_addr_IP.v4   , ip_cmp_size );
            verdict |= memcmp( &conntrack->tuplehash[ IP_CT_DIR_REPLY ].tuple.dst.u3.ip,   &ptr_session->ingress.lookup.LUT1.u.fields.L3.LAN_addr_IP.v4, ip_cmp_size );
        }
    } 
    else
    {
        if (AVALANCHE_PP_LUT_ENTRY_L3_IPV6 == ptr_session->ingress.lookup.LUT1.u.fields.L3.entry_type)
        {
            verdict |= ipv6_addr_equal(&conntrack->tuplehash[ IP_CT_DIR_REPLY ].tuple.dst.u3.in6,   (struct in6_addr*)ptr_session->ingress.lookup.LUT2.u.fields.WAN_addr_IP.v6);
            verdict |= ipv6_addr_equal(&conntrack->tuplehash[ IP_CT_DIR_REPLY ].tuple.src.u3.in6,   (struct in6_addr*)ptr_session->ingress.lookup.LUT1.u.fields.L3.LAN_addr_IP.v6);
        }
        else
        {
            verdict |= memcmp( &conntrack->tuplehash[ IP_CT_DIR_REPLY ].tuple.dst.u3.ip,   &ptr_session->ingress.lookup.LUT2.u.fields.WAN_addr_IP.v4   , ip_cmp_size );
            verdict |= memcmp( &conntrack->tuplehash[ IP_CT_DIR_REPLY ].tuple.src.u3.ip,   &ptr_session->ingress.lookup.LUT1.u.fields.L3.LAN_addr_IP.v4, ip_cmp_size );
        }
    }
    /* ARRIS CHANGE END */

    verdict |= memcmp( &conntrack->tuplehash[ IP_CT_DIR_REPLY ].tuple.dst.u.all, &ptr_session->ingress.lookup.LUT2.u.fields.L4_DstPort, 2 );
    verdict |= memcmp( &conntrack->tuplehash[ IP_CT_DIR_REPLY ].tuple.src.u.all, &ptr_session->ingress.lookup.LUT2.u.fields.L4_SrcPort, 2 );

    if (0 == verdict)
    {
        avalanche_pp_session_delete( ptr_session->session_handle, NULL );
    }

    return (PP_RC_SUCCESS);
}
#endif


/**************************************************************************
 * FUNCTION NAME : hil_netsubsystem_event_handler
 **************************************************************************
 * DESCRIPTION   :
 *  This is the HIL Intrusive event handler which will capture all events from the networking sub-system.
 *
 * RETURNS       :
 *  0   -   Success
 *  <0  -   Error
 **************************************************************************/
static int hil_netsubsystem_event_handler(unsigned int module_id, unsigned long event_id, void* ptr)
{
    /* Process based on the module identifier */
    switch (module_id)
    {
        case TI_PP:
        {
            hil_netsubsystem_pp_handler(event_id, ptr);
            break;
        }

        default:
            break;
    }
    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_netsubsystem_pp_handler
 **************************************************************************
 * DESCRIPTION   :
 *  Default HIL PP Handler.
 *
 * RETURNS       :
 *  0   -   Success
 *  <0  -   Error
 **************************************************************************/
static Int32 hil_netsubsystem_pp_handler(Uint32 event_id, void* ptr)
{
    struct net_device*           dev, *input_dev;
    struct sk_buff*              skb;
    AVALANCHE_PP_DDH_STATE_e     state;
    Uint8                        new_l2classification_deafult_mode;
    Bool                         l2classification_deafult_mode;
#ifdef CONFIG_NETFILTER
    avalanche_pp_local_dev_addr_ioctl_params_t rnd_list_param;
    struct ct_entry_created_t*  entry_info;
    struct nf_conn*             conntrack;
    struct xt_table*            t;
#endif

    /* Process the events. */
    switch (event_id)
    {
        case TI_DOCSIS_FLTR_DISCARD_PKT:
        {
            Uint32 lockKey;

            PAL_osProtectEntry(PAL_OSPROTECT_INTERRUPT, &lockKey);

            skb = (struct sk_buff*) ptr;
            input_dev = dev_get_by_index(&init_net, skb->skb_iif);
            if ((docsis_null_vpid_handle == -1) && (input_dev && input_dev->vpid_handle != -1))
            {
                if (avalanche_pp_vpid_create(&input_dev->vpid_block) != PP_RC_SUCCESS)
                {
                    printk ("Error: Unable to create Null VPID %d for Device %s\n", docsis_null_vpid_handle, input_dev->name);
                }
                else
                {
                    docsis_null_vpid_handle = input_dev->vpid_block.vpid_handle;
                    avalanche_pp_vpid_set_flags(docsis_null_vpid_handle, AVALANCHE_PP_VPID_FLG_TX_DISBL | AVALANCHE_PP_VPID_FLG_RX_DISBL);
                }
            }

            if (!WARN(input_dev == NULL, "%s - %d: dev_get_by_index(&init_net,skb->skb_iif %d); returned NULL", __func__, __LINE__, skb->skb_iif))
            {
                dev_put(input_dev); 
            }

            hil_null_hook(skb, docsis_null_vpid_handle);
            PAL_osProtectExit(PAL_OSPROTECT_INTERRUPT, lockKey);
            break;
        }

        case TI_DOCSIS_FLTR_ADD:
        case TI_DOCSIS_FLTR_DEL:
        case TI_DOCSIS_FLTR_CHG:
        case TI_DOCSIS_CLASSIFY_ADD:
        case TI_DOCSIS_CLASSIFY_DEL:
        case TI_DOCSIS_CLASSIFY_CHG:
        case TI_DOCSIS_MCAST_DEL:
        case TI_DOCSIS_DSID_CHG:
        {
            avalanche_pp_flush_sessions( AVALANCHE_PP_MAX_VPID, PP_LIST_ID_ALL );
            break;
        }

        case TI_DOCSIS_SESSIONS_DEL:
        {
            Uint32 numSessions = ((Uint32*)ptr)[0];
            Uint32 *sessList = &(((Uint32*)ptr)[1]);
            Uint32 i;
            for (i = 0; i < numSessions; i++)
            {
                avalanche_pp_session_delete(sessList[i], NULL);

                if (global_hil_db.tdox_dbg)
                {
                    printk("TDOX-DBG DeleteSession: ses=%d\n", sessList[i]);
                }
            }

            break;
        }

        case TI_DOCSIS_VOICE_SESSIONS_DEL:
        {
            static struct net_device *ptr_voiceni_dev;
            ptr_voiceni_dev = dev_get_by_name(&init_net, "vni0");
            if (ptr_voiceni_dev)
            {
                avalanche_pp_flush_sessions( ptr_voiceni_dev->vpid_handle, PP_LIST_ID_ALL );
                printk("Flush all VOICE sessions\n");
                dev_put(ptr_voiceni_dev);
            }
            break; 
        }

        case TI_PP_ADD_VPID:            /* Event to add vpid, for example when GW add vlan to LSD */
        case TI_BRIDGE_PORT_FORWARD:
        {
            /* Event indicates that a port attached to the bridge has moved to the FORWARDING state.
             * The host bridge will only forward packets in this state. Thus this is the time we create the VPID */
            dev = (struct net_device*)ptr;

            /* Do not create any VPIDs if there is no parent PID defined */
            if (dev->vpid_block.parent_pid_handle == (Uint8)(-1))
            {
                break;
            }

            if (dev->vpid_handle == -1)
            {
                AVALANCHE_PP_PID_t *ptr_pid;

                if (!global_hil_db.qos_disabled)
                {
                    if (NULL != dev->qos_setup_hook)
                    {
                        if (NETDEV_PP_QOS_PROFILE_DEFAULT != dev->qos_virtual_scheme_idx)
                        {
                            dev->qos_setup_hook(dev);
                        }
                    }
                }

                /* Create the VPID */
                if (avalanche_pp_vpid_create(&dev->vpid_block) != PP_RC_SUCCESS)
                {
                    printk ("Error: Unable to create VPID %d PID %d Device %s\n", dev->vpid_handle, dev->pid_handle, dev->name);
                    return -1;
                }
                dev->vpid_handle = dev->vpid_block.vpid_handle;

                if (avalanche_pp_pid_get_info(dev->vpid_block.parent_pid_handle, &ptr_pid) == PP_RC_SUCCESS)
                {
                    if (ptr_pid->type == AVALANCHE_PP_PID_TYPE_DOCSIS)
                    {
                        docsis_vpid_handle = dev->vpid_handle;
                    }
                }
                
            }

            {
                int rc ;
                rc = ti_register_protocol_handler(dev, hil_ingress_hook);     /* Install the Ingress Hook. */
                printk("====> PP2K Ingress registered %d [%p] <=====\n", rc, hil_ingress_hook);

                rc = ti_register_egress_hook_handler(dev, hil_egress_hook);   /* Install the Egress Hook. */
                printk("====> PP2K Egress registered %d [%p] <=====\n", rc, hil_egress_hook);
            }
            break;
        }

        case TI_PP_REMOVE_VPID:         /* Event to remove vpid, for example when GW add vlan to LSD */
        case TI_BRIDGE_PORT_DELETE:     /* Event indicates that a device has been removed from the bridge. This event is generated when the user executes the brctl delif command to remove a port from the bridge */
        case TI_BRIDGE_PORT_DISABLED:   /* Event indicates that a port attached to the bridge has been moved to the disabled state */
        {
            dev = (struct net_device*)ptr;

            if (dev->vpid_block.parent_pid_handle == (Uint8)(-1))
            {
                /* Do not touch VPIDs that do not have any parent PID defined */
                break;
            }

            /* If the device has a VPID handle; it needs to be removed from the packet processor. */
            if (dev->vpid_handle != -1)
            {
                /* Delete the VPID */
                if (avalanche_pp_vpid_delete(dev->vpid_handle) != PP_RC_SUCCESS)
                {
                    printk ("Error: Unable to delete VPID %d PID %d Device %s\n", dev->vpid_handle, dev->pid_handle, dev->name);
                    return -1;
                }
                dev->vpid_handle = -1;

                if (event_id == TI_PP_REMOVE_VPID || event_id == TI_BRIDGE_PORT_DISABLED)
                {
                    AVALANCHE_PP_PID_t *ptr_pid;

                    avalanche_pp_pid_get_info(dev->vpid_block.parent_pid_handle, &ptr_pid);
                    if (ptr_pid->type == AVALANCHE_PP_PID_TYPE_ETHERNET)
                    {
                        Uint32 queueIndex;
                        Uint32 divertCommand;
                        Uint32 qmgr = PAL_CPPI41_QUEUE_MGR_PARTITION_SR;
                        PAL_Handle handle = PAL_cppi4Init (NULL,(Ptr)PAL_CPPI41_QUEUE_MGR_PARTITION_SR);

                        for (queueIndex = 0; queueIndex < 1; queueIndex++)
                        {
                            divertCommand =  (PAL_CPPI41_RECYCLE_INFRA_INPUT_LOW_Q_NUM << 16);  // Setup destination Queue
                            divertCommand += ptr_pid->tx_pri_q_map[queueIndex];                 // Setup source Queue

                            PAL_cppi4Control( handle, PAL_CPPI41_IOCTL_QUEUE_DIVERT, (Ptr)divertCommand, &qmgr );
                        }

                        PAL_cppi4Exit(handle, NULL);
                    }
                }
            }

            /* Uninstall the Ingress & Egress Hooks */
            ti_deregister_protocol_handler (dev);
            ti_deregister_egress_hook_handler (dev);
            break;
        }

        case TI_BRIDGE_FDB_CREATED:
        case TI_BRIDGE_FDB_DELETED:
        {
            break;
        }

        case TI_BRIDGE_PACKET_FLOODED:
        {
            /* Event indicates that the packet will now be flooded onto all interfaces. This can happen in any of the following cases:-
             *  a) Unicast packet but no matching FDB entry is found.
             *  b) Broadcast packet
             *  c) Multicast packet but no layer2 extensions eg IGMP snooping exists */
            skb = (struct sk_buff*) ptr;

            /* In the intrusive mode profile these packets are not considered as candidates for acceleration so mark the packet BYPASS mode so that the egress hook is bypassed */
            skb->pp_packet_info.flags |= TI_HIL_PACKET_FLAG_PP_SESSION_BYPASS;
            break;
        }

        case TI_ROUTE_ADDED:
        case TI_ROUTE_DELETED:
        {
            /*  Event indicates that a new route is being added/deleted. A route change could affect the existing sessions in the PP.
             *  So, the HIL profile  could choose to delete all sessions that are affected by the route change / flush the entire session table to make sure that there is no inconsistency due to the route change */
            break;
        }

#ifdef CONFIG_NETFILTER
        case TI_CT_ENTRY_CREATED:
        {
            /* Event generated from the connection tracking layer to indicate that a connection tracking entry
             * has been created. This could be used by the system profile to analyze the connection and indicate
             * immediately if the connection is worthy of accleration or not? If the profile deems that the
             * connection can not be accelerated it sets the status flag in the connection tracking entry to BYPASS
             * mode. This will ensure that all packets matching the connection will also have the BYPASS mode
             * set. This can be used for performance optimizations as the Egress Hook and Session Intelligence
             * will be less burdened. */

            entry_info  = (struct ct_entry_created_t *)ptr;
            conntrack   = entry_info->ct;
            skb         = entry_info->skb;

#if defined(CONFIG_NF_CONNTRACK) || defined(CONFIG_NF_CONNTRACK_MODULE)
            if ((IS_GW() || IS_HYBRID()) &&
                (conntrack != NULL) && (skb != NULL) &&
                ((struct ethhdr*)skb_mac_header(skb) != NULL) &&
                (((struct ethhdr*)skb_mac_header(skb))->h_dest != NULL))
            {
                /* If TI_PP_US_CONNECTION is not set */
                if(!(conntrack->ti_pp_status_flag & TI_PP_US_CONNECTION))
                {
                    rnd_list_param.op_type      = IS_ADDR_EXIST;
                    rnd_list_param.addr_type    = RND_MAC_ADDR;
                    memcpy((void*)(rnd_list_param.u.mac_addr), (void *)(((struct ethhdr*)skb_mac_header(skb))->h_dest), 6);

                    /* Check if destination mac is in the rnd mac list */
                    if (avalanche_pp_local_dev_addr(&rnd_list_param) == PP_RC_SUCCESS)
                    {
                        conntrack->ti_pp_status_flag |= TI_PP_US_CONNECTION;
                    }
                }

                /* Init scb id with invalid value */
                conntrack->ti_pp_scb_id = AVALANCHE_PP_MAX_ACCELERATED_SESSIONS;
            }

            /* Attach conntrack to current scb */
            __hil_scb_attach_conntrack(skb, conntrack);
#endif
            /* Check if the conntrack was associated with an ALG? In our profile we dont want these sessions
             * to be accelerated so set the connection tracking entry to operate in BYPASS mode. */
            if ((nfct_help(conntrack)) != NULL)
            {
                conntrack->ti_pp_status_flag |= TI_PP_BYPASS;
            }

            break;
        }

        case TI_CT_DEATH_BY_TIMEOUT:
        {
            /* Event indicates that the connection tracking entry has timed out. Use this event to
             * determine if the connection tracking entry needs to be deleted or not? If the profile
             * wants to prevent the death of the connection tracking entry it should ensure that the
             * PP staus flag does NOT set the TI_PP_KILL_CONNTRACK bit and if so be the case the
             * connection tracking entry is now owned by the System Profile and it has the repsonsibility
             * of cleaning it.
             * This event needs to be handled only if CONFIG_NETFILTER is enabled else this event can
             * be safetly ignored. */
            conntrack = (struct nf_conn *)ptr;

            if (IS_TI_PP_SESSION_CT_INVALID(conntrack->tuplehash[IP_CT_DIR_ORIGINAL].ti_pp_session_handle) &&
                IS_TI_PP_SESSION_CT_INVALID(conntrack->tuplehash[IP_CT_DIR_REPLY   ].ti_pp_session_handle))
            {
                /* Neither of the flows in the connection tracking entry are being accelerated.
                 * This implies that we should just go ahead and delete the entry? */
                /* ARRIS ADD BEGIN: For PD3954 */
                //conntrack->ti_pp_status_flag |= TI_PP_KILL_CONNTRACK;
                const struct nf_conn_help *help;
                const struct nf_conntrack_helper *helper;
                struct nf_conn *ct_master;
                int rtsp_flag = 0;

                ct_master = conntrack->master;
                if (ct_master)
                {
                    help = nfct_help(ct_master);
                    if (help)
                    {
                        helper = rcu_dereference(help->helper);
                        if (helper && ((helper->name[0] == 'r') && (helper->name[1] == 't') 
                                    && (helper->name[2] == 's') && (helper->name[3] == 'p')))
                        {
                            rtsp_flag = 1;
                        }
                    }
                }
                
                if (rtsp_flag == 0)
                    conntrack->ti_pp_status_flag |= TI_PP_KILL_CONNTRACK;
                else
                {
                    if ((conntrack->ti_pp_status_flag & TI_PP_RTSP_TIMEOUT) == 0)
                    {
                        conntrack->timeout.expires = nf_ct_udp_rtsp_timeout_stream + jiffies;
                        printk(KERN_DEBUG "add a timer to delay the rtsp ct session delete.\n");
                        add_timer(&conntrack->timeout);
                        conntrack->ti_pp_status_flag |= TI_PP_RTSP_TIMEOUT;
                    }
                    else
                    {
                        conntrack->ti_pp_status_flag &= ~(TI_PP_RTSP_TIMEOUT);
                        conntrack->ti_pp_status_flag |= TI_PP_KILL_CONNTRACK;
                        printk(KERN_DEBUG "rtsp ct session timeout, delete ct %d\n", conntrack->ti_pp_status_flag);
                    }
                }
                /* ARRIS ADD END: For PD3954 */
                
                if(IS_GW() || IS_HYBRID())
                {
                    /* We have calculated tuple hash over this connection */
                    if (conntrack->ti_pp_status_flag & TI_PP_DS_RESPONSE_HASH_VALID)
                    {
                        BHPRINTK("Remove hash 0x%x", conntrack->ti_pp_ds_response_hash);
                        avalanche_pp_update_bithash(conntrack->ti_pp_ds_response_hash, False);
                    }
                }

                /* Remove scb connection upon conntrack timeout */
                __hil_scb_timeout(conntrack);
            }
            else
            {
                /* The flows are still being accelerated; so keep the connection tracking timer alive */
                conntrack->timeout.expires = (pp_session_timeout_sec * HZ) + jiffies;
                add_timer(&conntrack->timeout);
            }
            break;
        }

        case TI_CT_NETFILTER_TABLE_UPDATE:
        {
            t = (struct xt_table *)ptr; /* Get the netfilter table */

            /* Flush all sessions only for NAT... No need to do anything for Mangle and Firewall */
            if (strcmp (t->name, "nat") == 0 || strcmp (t->name, "filter") == 0)
            {
                if(IS_GW() || IS_HYBRID())
                {
                    avalanche_pp_flush_bithash();
                    BHPRINTK("Flushing bithash");
                }

                if (avalanche_pp_flush_sessions( AVALANCHE_PP_MAX_VPID, PP_LIST_ID_ALL ) != PP_RC_SUCCESS)
                {
                    printk ("Error: Unable to flush all sessions\n");
                    return 0;
                }
                printk ("NAT Table update all sessions flushed\n");
            }
            break;
        }

        case TI_CT_NETFILTER_CANCEL_DISCARD_ACCELERATION:
        {
            avalanche_pp_session_list_execute( netfilter_null_vpid_handle,  PP_LIST_ID_EGRESS, __hil_delete_drop_session, ptr );
            avalanche_pp_session_list_execute( docsis_null_vpid_handle,     PP_LIST_ID_EGRESS, __hil_delete_drop_session, ptr );
            break;
        }
#endif

        case TI_IP_DISCARD_PKT_IPV4:
        case TI_IP_DISCARD_PKT_IPV6:
            if (((event_id == TI_IP_DISCARD_PKT_IPV4) && DROPPED_PACKETS_BITMAP_IS_SET(4)) ||
                ((event_id == TI_IP_DISCARD_PKT_IPV6) && DROPPED_PACKETS_BITMAP_IS_SET(6)))
            {
                /* Hanlde event*/
            }
            else
            {
                break;  /* Appropriate bit is not set, discard event */
            }
        /* event handle for martian source/destination packet drop */
        case IP_DISCARD_MARTIAN_PKT_IPV4:
        case IP_DISCARD_MARTIAN_PKT_IPV6:
#ifdef CONFIG_NETFILTER
        case TI_CT_NETFILTER_DISCARD_PKT:
#endif
        {
            Uint32 lockKey;

            PAL_osProtectEntry(PAL_OSPROTECT_INTERRUPT, &lockKey);

            skb = (struct sk_buff*) ptr;

            if (!(skb->skb_iif))
            {
                PAL_osProtectExit(PAL_OSPROTECT_INTERRUPT, lockKey);
                break;
            }
            input_dev = dev_get_by_index(&init_net,skb->skb_iif);
            
            if ((netfilter_null_vpid_handle == -1) && (input_dev && input_dev->vpid_handle != -1) && (!avalanche_pp_state_is_psm()))
            {
                if (avalanche_pp_vpid_create(&input_dev->vpid_block) != PP_RC_SUCCESS)
                {
                    printk ("Error: Unable to create Null VPID %d Device %s\n", netfilter_null_vpid_handle, input_dev->name);
                }
                else
                {
                    netfilter_null_vpid_handle = input_dev->vpid_block.vpid_handle;
                    avalanche_pp_vpid_set_flags(netfilter_null_vpid_handle, AVALANCHE_PP_VPID_FLG_TX_DISBL | AVALANCHE_PP_VPID_FLG_RX_DISBL);
                }
            }

            hil_null_hook(skb, netfilter_null_vpid_handle);
            if (!WARN(input_dev == NULL, "%s - %d: dev_get_by_index(&init_net,skb->skb_iif %d); returned NULL", __func__, __LINE__, skb->skb_iif))
            {
                dev_put(input_dev); 
            }
            PAL_osProtectExit(PAL_OSPROTECT_INTERRUPT, lockKey);
            break;
        }

#ifdef CONFIG_IP_MULTICAST
        case TI_MC_SESSION_DELETED:
        {
            struct mfc_cache* ptr_mfc_cache;
            if(NULL == ptr)
            {
                printk ("FATAL Error: Multicast params is NULL\n");
                break;
            }

            ptr_mfc_cache = (struct mfc_cache* )ptr;
            hil_mfc_delete_session_by_mc_group(ptr_mfc_cache->mfc_mcastgrp);
            break;
        }
#endif

#ifdef CONFIG_VLAN_8021Q
        case TI_VLAN_DEV_CREATED:
        {
            /* Get the pointer to the network device. */
            dev = (struct net_device*)ptr;

            /* We know for sure that this is attached to a VLAN interface, so configure the VPID Information Block appropriately.
             * The egress MTU of the interface needs to be correctly handled to account for the VLAN Header */
            dev->vpid_block.type            = AVALANCHE_PP_VPID_VLAN;
            dev->vpid_block.vlan_identifier = (vlan_dev_info(dev))->vlan_id;
            break;
        }

        case TI_VLAN_DEV_DELETED:
        {
            break;
        }
#endif

        case TI_DDH_CHANGE_L2CLASSIFICATION_DEFAULT_MODE:
        {
            new_l2classification_deafult_mode = *(Uint8*)ptr;

            avalanche_pp_get_l2classification_default_mode(&l2classification_deafult_mode);
            avalanche_pp_get_defensive_state(&state);

            /* Disable "L2classification as default" */
            if ((new_l2classification_deafult_mode == 0) && (l2classification_deafult_mode == True))
            {
                avalanche_pp_set_l2classification_default_mode(False);

                /* If we are in normal state, disable l2 calssification */
                if (state == AVALANCHE_PP_DDH_STATE_NORMAL)
                {
                    avalanche_pp_support_l2_classification(False, PP_PID_NUM_INVALID);
                }
            }
           
            /* Enable "L2classification as default" */
            if ((new_l2classification_deafult_mode == 1) && (l2classification_deafult_mode == False))
            {
                avalanche_pp_set_l2classification_default_mode(True);

                /* If we are in normal state, enable l2 calssification */
                if (state == AVALANCHE_PP_DDH_STATE_NORMAL)
                {
                    avalanche_pp_support_l2_classification(True, PP_PID_NUM_ALL); 
                }
            }
            break;
        }

        default:
        {
            printk ("Intrusive -> Does not handle event 0x%x\n", event_id);
            break;
        }
    }

    /* Successfully handled the event. */
    return 0;
}

/**************************************************************************
 * FUNCTION NAME : ddh_notify_user_application
 **************************************************************************
 * DESCRIPTION   :
 *  The function send a message to user application about ddh new state.
 **************************************************************************/
static AVALANCHE_PP_RET_e ddh_notify_user_application(char* message)
{
    struct nlmsghdr*    netlink_header;
    Uint32              message_size = strnlen(message, MAX_DDH_NOTIFICATION_MESSAGE_LENGTH);
    /* Allocate new netlink message */
    struct sk_buff*     skb_out = nlmsg_new(message_size, 0);

    if (!skb_out)
    {
        printk(KERN_ERR "Failed to allocate new skb.\n");
		return PP_RC_FAILURE; 
    }

    /* Fill the message. */
    netlink_header = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, message_size, 0);
    NETLINK_CB(skb_out).dst_group = 0;
    strncpy(nlmsg_data(netlink_header), message, message_size);

	/* No user application is listening to DDH notifications. */
	if (user_application_pid < 0)
	{
		return PP_RC_INVALID_PARAM; 
	}

	/* Send notification */
	if (nlmsg_unicast(netlink_socket, skb_out, user_application_pid) < 0)
    {
        printk(KERN_ERR "Error while sending back to user, pid = { %d } \n", user_application_pid);
		return PP_RC_FAILURE; 
    }

	return PP_RC_SUCCESS;
}

/**************************************************************************
 * FUNCTION NAME : hil_pp_event_handler
 **************************************************************************
 * DESCRIPTION   :
 *  The function is the registered event handler which listens to all events
 *  that arise from the Packet Processor Subsystem.
 **************************************************************************/
static AVALANCHE_PP_RET_e hil_pp_event_handler(AVALANCHE_PP_EVENT_e event_id, Uint32 param1, Uint32 param2)
{
    AVALANCHE_PP_SESSION_INFO_t *session = NULL;

    /* Process each event based on the module which generated the event. */
    switch (event_id)
    {
        case PP_EV_PID_CREATED:
        {
            struct net_device *dev = (struct net_device *)param2;

            if (!global_hil_db.qos_disabled)
            {
                if ((NULL != dev) && (NULL != dev->qos_setup_hook))
                {
                    dev->qos_setup_hook( dev );
                }
            }
            printk ("PP Operation PP_EV_PID_CREATED, pid_handle=%d @ %s device\n", param1, param2 ? dev->name : "NULL" );
            break;
        }

        case PP_EV_PID_DELETED:
        {
            printk ("PP Operation PP_EV_PID_DELETED, pid_handle=%d\n", param1);
            break;
        }

        case PP_EV_VPID_CREATED:
        {
            printk ("PP Operation PP_EV_VPID_CREATED, vpid_handle=%d\n", param1);
            break;
        }
        case PP_EV_VPID_DELETED:
        {
            printk ("PP Operation PP_EV_VPID_DELETED, vpid_handle=%d\n", param1);
            break;
        }

        case PP_EV_SESSION_CREATED:
        {
            global_hil_db.num_total_sessions++;

#ifdef CONFIG_TI_PACKET_PROCESSOR_STATS
            if (ti_hil_start_session_notification_cb)
            {
                if (((struct sk_buff*)param2)->pp_packet_info.pp_session.egress.vpid_handle == docsis_null_vpid_handle)
                {
                    ti_hil_start_session_notification_cb(param1, TI_DOCSIS_PP_SESSION_TYPE_DISCARDING, ((struct sk_buff*)param2));
                }
                else
                {
                    ti_hil_start_session_notification_cb(param1, TI_DOCSIS_PP_SESSION_TYPE_FORWARDING, ((struct sk_buff*)param2));
                }
            }
#endif

#ifdef CONFIG_NETFILTER
            {
                struct sk_buff * skb = (struct sk_buff *)param2;

                /* Once the session has been created; check if the packet had passed through the connection tracking hooks and pass the session handle to that layer */
                if (skb->nfct != NULL)
                {
                    /* Session has passed through the connection tracking hooks. Now get the connection tracking entry and set the hooks correctly */
                    struct nf_conn*     conntrack   = (struct nf_conn *)skb->nfct;
                    Uint8               dir         = CTINFO2DIR(skb->nfctinfo);  // enum ip_conntrack_dir
                    Uint16              ct_session  = conntrack->tuplehash[ dir ].ti_pp_session_handle;
                    Uint16 new_session_handle;
                    Uint16 prev_new_session_handle;
                    Uint32 lockKey;

                    /* Check if the current session handle is valid or not? */
                    if (!IS_TI_PP_SESSION_CT_INVALID( ct_session ))
                    {
                        /* Handle was valid. Now we need to ensure that the current session handle matches the one we just created */
                        if ( ct_session != (Uint16)param1 )
                        {
                            /* The existing session handle does not match the one we had.
                             *
                             * This should typically not happen because the PPM has an inbuilt duplicate session detection logic which will detect this and return the same session handle.
                             * (Example * of this is the TCP Control Packets will have the same session handle passed to them at this stage so this code will never get executed)
                             *
                             * The fact that the control came here is that the PPM duplicate session detection logic failed or because there was something in the packet different which the conntrack
                             * did not care about. One of the known occurrences is the TOS Byte difference. Anyway in this case we need to handle the condition gracefully.
                             * So currently we ignore the new session handle and dont link it with the connection tracking entry.
                             *
                             * One more technique that can be used to solve this problem is to ignore the TOS byte in the LUT configuration */

                            /*if the ti_pp_sessions_count is 0 then something went wrong.
                              it means that the connection tracking entry is mapped to a session
                              but the refrence count which should reflect the number of sessions that this connection tracking entry is mapped to does not match*/
                            if (conntrack->tuplehash[ dir ].ti_pp_sessions_count == 0)
                                printk ("ERROR --> Existing session %d new session %d.\n", ct_session, param1);
                        }
                    }
                    PAL_osProtectEntry(PAL_OSPROTECT_INTERRUPT, &lockKey);

                    /*add the new_session_handle to the ct mapper list 
                      the mapper's list is a bidirectional list containg sessions that belong to the same connection tracking entry */
                      
                    new_session_handle = (Uint16)param1;
                    prev_new_session_handle = conntrack->tuplehash[ dir ].ti_pp_session_handle;

                   
                    hil_add_session_handle_to_ct_mapper_list (new_session_handle,prev_new_session_handle);

                    /*update the session handle in the conntrack. the 4 upper bits represent the validity of the session- 
                      0 means session is valid, any other value - session is not valid .
                      the 12 lower bits represent the session number. the bit operation & guarantee .                        
                      that the ti_pp_session_handle is saved as a valid session in the conntrack */
                    conntrack->tuplehash[ dir ].ti_pp_session_handle = (new_session_handle & 0x0fff);  
                    conntrack->tuplehash[ dir ].ti_pp_sessions_count++;  
                     /* Map the session handle and connection tracking entry together */        
                    hil_session_ct_mapper[ new_session_handle ].conntrack = conntrack;  
                    PAL_osProtectExit(PAL_OSPROTECT_INTERRUPT, lockKey);

                    /* When session is created we must remove this SCB connection from our count */
                    __hil_scb_remove(skb);
                }
            }
#endif
            break;
        }

        case PP_EV_SESSION_EXPIRED:
        {
            if (avalanche_pp_session_delete(param1, NULL) < 0)
            {
                printk ("Error: Unable to delete session %d\n", param1);
                return PP_RC_FAILURE;
            }

            if (global_hil_db.tdox_dbg)
            {
                printk("TDOX-DBG DeleteSession: ses=%d\n", param1);
            }
            break;
        }

        case PP_EV_SESSION_DELETED:
        {
            avalanche_pp_session_get_info(param1, &session);
            __hil_calc_mac_hash(session, False);

#ifdef CONFIG_TI_PACKET_PROCESSOR_STATS
            if (ti_hil_delete_session_notification_cb)
            {
                AVALANCHE_PP_SESSION_STATS_t *session_stats = (AVALANCHE_PP_SESSION_STATS_t*)param2;

                session_stats->bytes_forwarded += (4 * session_stats->packets_forwarded);  /* add the 4 bytes the PP is ignoring for each packet */

                ti_hil_delete_session_notification_cb(param1, session_stats->packets_forwarded, session_stats->bytes_forwarded);
            }
#endif

#ifdef CONFIG_NETFILTER
            {
                struct nf_conn* ct = hil_session_ct_mapper[ param1 ].conntrack;
                Uint32 lockKey;
                enum ip_conntrack_dir dir;

                /* Once the session has been removed; check if the session handle was present in the mapper.
                   This implies that the session handle and connection tracking entry were connected to each other */
                if (ct != NULL)
                {
                    PAL_osProtectEntry(PAL_OSPROTECT_INTERRUPT, &lockKey);

                    if (hil_find_session_handle_in_ct_mapper_list (ct->tuplehash[ IP_CT_DIR_REPLY ].ti_pp_session_handle,param1) == true) 
                    {
                        dir = IP_CT_DIR_REPLY;
                        
                        ct->tuplehash[IP_CT_DIR_REPLY].ti_pp_sessions_count--; 
                        if (ct->tuplehash[ IP_CT_DIR_REPLY ].ti_pp_sessions_count == 0)
                        {
                            ct->tuplehash[ IP_CT_DIR_REPLY ].ti_pp_session_handle = TI_PP_SESSION_CT_TCP_UPDATE;
                            if (IS_TI_PP_SESSION_CT_INVALID(ct->tuplehash[ IP_CT_DIR_ORIGINAL ].ti_pp_session_handle))
                            {
                                ct->tuplehash[ IP_CT_DIR_ORIGINAL ].ti_pp_session_handle = TI_PP_SESSION_CT_TCP_UPDATE;
                            }
                        }
                        hil_delete_session_handle_in_ct_mapper_list (param1,dir);
                    }
                    else if (hil_find_session_handle_in_ct_mapper_list (ct->tuplehash[ IP_CT_DIR_ORIGINAL ].ti_pp_session_handle,param1) == true)
                    {
                        dir = IP_CT_DIR_ORIGINAL;
                        
                        ct->tuplehash[ IP_CT_DIR_ORIGINAL ].ti_pp_sessions_count--;
                        if (ct->tuplehash[ IP_CT_DIR_ORIGINAL ].ti_pp_sessions_count == 0)
                        {
                            ct->tuplehash[ IP_CT_DIR_ORIGINAL ].ti_pp_session_handle = TI_PP_SESSION_CT_TCP_UPDATE;
                            if (IS_TI_PP_SESSION_CT_INVALID(ct->tuplehash[ IP_CT_DIR_REPLY ].ti_pp_session_handle))
                            {
                                ct->tuplehash[IP_CT_DIR_REPLY].ti_pp_session_handle = TI_PP_SESSION_CT_TCP_UPDATE;
                            }
                        }
                        hil_delete_session_handle_in_ct_mapper_list (param1,dir);
                    }
                    else
                    {
                        /*update current entry*/
                        hil_session_ct_mapper[param1].next = TI_PP_SESSION_CT_IDLE;
                        hil_session_ct_mapper[param1].previous = TI_PP_SESSION_CT_IDLE;
                        hil_session_ct_mapper[param1].conntrack = NULL;
                    }
                    PAL_osProtectExit(PAL_OSPROTECT_INTERRUPT, lockKey);
                }
            }
#endif
            break;
        }
    
    
        case PP_EV_MISC_TRIGGER_TDOX_EVALUATION:
        {
            if (!global_hil_db.tdox_disabled)
            {
                hil_check_and_update_TdoxEvalTime();
                avalanche_pp_session_list_execute(docsis_vpid_handle, PP_LIST_ID_EGRESS_TCP, hil_tdox_manager, NULL);
            }
            break;
        }

        case PP_EV_PID_CREATE_FAIL:
        {
            struct net_device *dev = (struct net_device *)param2;

            printk("FATAL Error: PP Operation PP_EV_PID_CREATE_FAIL, pid_handle=%d @ %s device\n", param1, param2 ? dev->name : "NULL");
            break;
        }

        case PP_EV_PID_DELETE_FAIL:
        {
            printk("FATAL Error: PP Operation PP_EV_PID_DELETE_FAIL, pid_handle=%d\n", param1);
            break;
        }

        case PP_EV_VPID_CREATE_FAILED:
        {
            printk("FATAL Error: PP Operation PP_EV_VPID_CREATE_FAILED, vpid_handle=%d\n", param1);
            break;
        }

        case PP_EV_VPID_DELETE_FAILED:
        {
            printk("FATAL Error: PP Operation PP_EV_VPID_DELETE_FAILED, vpid_handle=%d\n", param1);
            break;
        }

        case PP_EV_SESSION_CREATE_FAILED:
        {
            printk("Error: PP Operation PP_EV_SESSION_CREATE_FAILED, session_handle=%d\n", param1);
            break;
        }

        case PP_EV_SESSION_DELETE_FAILED:
        {
            printk("Error: PP Operation PP_EV_SESSION_DELETE_FAILED, session_handle=%d\n", param1);
            break;
        }

        case PP_EV_DDH:
        {
            __hil_ddh_event();
            break;
        }

        case PP_EV_DDH_NOTIFY:
        {
            AVALANCHE_PP_DDH_STATE_e        state;
            AVALANCHE_PP_Misc_Statistics_t  stats;
			AVALANCHE_PP_RET_e              rc = PP_RC_SUCCESS;
            char                            message[MAX_DDH_NOTIFICATION_MESSAGE_LENGTH];

            if (avalanche_pp_get_defensive_state(&state) != PP_RC_SUCCESS)
            {
                printk("Error: Falied to get current defensive state.\n");
                return PP_RC_FAILURE;
            }

			/* Increment the counter. */
			ddh_notify.sampling_in_current_state_counter++;

			/* 
             * Notify user about defensive state changes only when: 
             * 1. The change was from/to normal state. 
             * 2. Last sampling was in the opposite state (Do not notify when the system switched between DDoS states). 
             * 3. This is the third sample that the PP is in the current state. 
             */
			if (((state != ddh_notify.previous_notified_state) && ((ddh_notify.previous_notified_state == AVALANCHE_PP_DDH_STATE_NORMAL) || (state == AVALANCHE_PP_DDH_STATE_NORMAL))) &&
			    (state == ddh_notify.previous_state) && (ddh_notify.sampling_in_current_state_counter == DDH_NOTIFICATIONS_NUM_OF_SAMPLING_WITHOUT_STATE_SWITCH))
			{
				/* State didn't change, and the PP already sampled in the current state the intervel. */
				ddh_notify.previous_notified_state = state;
				avalanche_pp_get_db_stats(&stats);

				sprintf(message, "Defensive state switched to: %s\nNumber of US sessions is: %u\nNumber of DS sessions is: %u\n", (state == AVALANCHE_PP_DDH_STATE_NORMAL) ? "Normal" : "DDoS", stats.active_us_sessions, (stats.active_sessions - stats.active_us_sessions));
				if (ddh_notify_user_application(message) == PP_RC_FAILURE) 
				{
					printk("Error: Falied to notify user application about the defensive state.\n");
					rc = PP_RC_FAILURE;
				}
			}
			else if ((state != ddh_notify.previous_state) && ((ddh_notify.previous_state == AVALANCHE_PP_DDH_STATE_NORMAL) || (state == AVALANCHE_PP_DDH_STATE_NORMAL)))
			{
				/* 
				 * Reset the notify counter - after states switch (under attack/normal), we need to count again to 
				 * verify that the system is in stable state. 
				 */
				ddh_notify.sampling_in_current_state_counter = 0;
				ddh_notify.previous_state = state;
			}

			return rc;
        }

        default:
        {
            printk ("FATAL Error: Unknown Event 0x%x\n", event_id);
            break;
        }
    }

    return PP_RC_SUCCESS;
}

#ifdef CONFIG_NETFILTER
/**************************************************************************
 * FUNCTION NAME : hil_find_session_handle_in_ct_mapper_list
 **************************************************************************
 * DESCRIPTION   :
 * param - ct_session_handle - the session handle held in the connection tracking entry
 * session_handle - the session_handle needed to be searched
 * return- true if session_handle was found in the list , otherwise false
 **************************************************************************/
static Bool hil_find_session_handle_in_ct_mapper_list (Uint16 ct_session_handle,Uint16 session_handle)
{
    Bool rc = false;
    
    if ((IS_TI_PP_SESSION_CT_INVALID(ct_session_handle)) || (IS_TI_PP_SESSION_CT_INVALID(session_handle)))
    {
        rc = false;
    }
    else if (ct_session_handle == session_handle)
    {
        rc = true;
    }
    else
    {
        Uint16 cur_session = ct_session_handle;
        Uint16 sessions_count = 0; 
        /* The session_count test is for protection - if from some reason the link list was broken so we dont want to get a endless loop */
        while ((hil_session_ct_mapper[cur_session].previous != ct_session_handle) && (sessions_count < AVALANCHE_PP_MAX_ACCELERATED_SESSIONS))
        {
            if (hil_session_ct_mapper[cur_session].previous == session_handle)
            {
                rc = true;
                break;
            }
            cur_session = hil_session_ct_mapper[cur_session].previous;
            if (cur_session >= AVALANCHE_PP_MAX_ACCELERATED_SESSIONS)
            {
                break;
            }
            sessions_count++;
        }

        if (sessions_count >= AVALANCHE_PP_MAX_ACCELERATED_SESSIONS)
        {
            printk(KERN_ERR"Error found in hil_find_session_handle_in_ct_mapper_list. \n");
        }
    }
    return rc;
}

/**************************************************************************
 * FUNCTION NAME : hil_add_session_handle_to_ct_mapper_list
 **************************************************************************
 * DESCRIPTION   : the function adds the session handle to the ct_mapper's list
 * param - new_session_handle - the new session handle to add to the ct_mapper's list
 * param - prev_new_session_handle - the last session handle that was inserted to the list
 * return - void
 **************************************************************************/

static void  hil_add_session_handle_to_ct_mapper_list (Uint16 new_session_handle,Uint16 prev_new_session_handle)
{
    Uint16 post_new_session;
    
    if (IS_TI_PP_SESSION_CT_INVALID( prev_new_session_handle ))
    {
        prev_new_session_handle = new_session_handle;
        post_new_session = new_session_handle;
    }
    else
    {
        post_new_session = hil_session_ct_mapper[prev_new_session_handle ].next;
    }

    /*update the ct_mapper*/
    hil_session_ct_mapper[new_session_handle].next = post_new_session;
    hil_session_ct_mapper[new_session_handle].previous = prev_new_session_handle;
    hil_session_ct_mapper[post_new_session].previous = new_session_handle;
    hil_session_ct_mapper[prev_new_session_handle].next = new_session_handle;
}

/**************************************************************************
 * FUNCTION NAME : hil_delete_session_handle_in_ct_mapper_list
 **************************************************************************
 * DESCRIPTION   : the function deletes the session handle from the ct_mapper's list
 * param - del_session_handle - the session handle to delete from the ct_mapper
 * dir - the connection tracking tuple's direction (could be either IP_CT_ORIGINAL/REPLY)
 * return - void
 **************************************************************************/
static void hil_delete_session_handle_in_ct_mapper_list (Uint16 del_session_handle,enum ip_conntrack_dir dir)
{
    Uint16 next_session;
    Uint16 previous_session;
    
    /*if the current ti_pp_session_handle of the conntrack points to session_handle then we need to change its value to a different session handle 
      since the current session_handle will be removed from the list shortly*/
    if(hil_session_ct_mapper[del_session_handle].conntrack->tuplehash[dir].ti_pp_session_handle == del_session_handle)
           hil_session_ct_mapper[del_session_handle].conntrack->tuplehash[dir].ti_pp_session_handle = hil_session_ct_mapper[del_session_handle].next;

    /*update the list */
    next_session = hil_session_ct_mapper[del_session_handle].next;
    previous_session = hil_session_ct_mapper[del_session_handle].previous;
    hil_session_ct_mapper[next_session].previous = previous_session;
    hil_session_ct_mapper[previous_session].next = next_session;

    /*update current entry*/
    hil_session_ct_mapper[del_session_handle].next = TI_PP_SESSION_CT_IDLE;
    hil_session_ct_mapper[del_session_handle].previous = TI_PP_SESSION_CT_IDLE;
    hil_session_ct_mapper[del_session_handle].conntrack = NULL;

    return;
}

#endif /*CONFIG_NETFILTER*/


/**************************************************************************
 * FUNCTION NAME : hil_ingress_hook
 **************************************************************************
 * DESCRIPTION   :
 *  The function is registered as the device specific protocol handler for all networking devices which exist in the system which have a valid VPID.
 *  The function extracts the information pertinent to creation of the session interface and stores it in the SKB.
 *
 * RETURNS:
 *  Always returns 0.
 **************************************************************************/
Int32 hil_ingress_hook(struct sk_buff* skb)
{
    if (global_hil_db.hil_disabled || (!avalanche_pp_state_is_active()))
    {
        return 0;
    }

    global_hil_db.num_ingress_pkts++;

    /* Extract all the fields from the packet and populate the Ingress Packet Descriptor. */
    if (hil_extract_packet_ingress(skb, skb->dev) < 0)
    {
        return 0;
    }

#if defined(CONFIG_NF_CONNTRACK) || defined(CONFIG_NF_CONNTRACK_MODULE)
    __hil_scb_add(skb);
#endif 

    /* Packet has passed through the Ingress Hooks. */
    skb->pp_packet_info.flags |= TI_HIL_PACKET_FLAG_PP_SESSION_INGRESS_RECORDED;
    skb->pp_packet_info.pp_session.priority = skb->ti_meta_info & 0x7;
    skb->pp_packet_info.pp_session.cluster = 0;

    return 0;
}


/**************************************************************************
 * FUNCTION NAME : hil_extract_packet_ingress
 **************************************************************************
 * DESCRIPTION   :
 *  The function is called to extract the various Layer2, Layer3 and Layer4 fields and populate the packet description structure for ingress packet
 *
 * RETURNS       :
 *  0   -   Success
 *  <0  -   Error
 **************************************************************************/
static Int32 hil_extract_packet_ingress(struct sk_buff* skb, struct net_device* dev)
{
    struct ethhdr*      ptr_ethhdr = NULL;
    struct pppoe_hdr*   ptr_pppoehdr = NULL;
    struct iphdr*       ptr_iphdr = NULL;
    void *              ptr_l4hdr = NULL;
    Int8*               ptr_data = NULL;

    /* IPv6 params */
    struct ipv6hdr*     ptr_ipv6hdr = NULL;
    Uint8               ipv6HeaderLen;
    Uint8               nexthdr;
    Int16               ipv6Offs;
    Uint16              prot;
    avalanche_pp_local_dev_addr_ioctl_params_t param;

    /* Flag indicating whether the fragmented datagram can be processed.
     * We allow process of L3-frag. packet only if first frag. and:
     *  - Outer DSLite
     *  - Inner DSLite
     *  - Outer GRE
     *  - PPPoE'ed traffic
     *
     */
    Bool                acceptFragL3 = False;
    hil_pp_ip_prot_e ExtTunnel = PP_IP_PROT_NA;
    
    AVALANCHE_PP_INGRESS_SESSION_PROPERTY_t* ingress_property = &skb->pp_packet_info.pp_session.ingress;


    memset( &ingress_property->lookup.LUT1, 0, (sizeof( ingress_property->lookup.LUT1 ) + sizeof( ingress_property->lookup.LUT2 )) );

    ingress_property->lookup.LUT1.u.fields.L2.entry_type = AVALANCHE_PP_LUT_ENTRY_L2_UNDEFINED;
    ingress_property->lookup.LUT1.u.fields.L3.entry_type = AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED;
    ingress_property->isTunnel = False;
    ingress_property->isGreGre = 0;
    ingress_property->tunnel_type = AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED;
    
    /* Check if the device is a PID or not? */
    if (dev->pid_handle != -1)
    {
        /* Valid PID Handle: Packet has been received at the lowest level; at this point in time we can extract all the L2/L3/L4 information from the packet */
        ptr_data = (Int8 *)skb_mac_header(skb);
        if( !ptr_data )
        {
            return -1;  /* Without this data we cannot continue */
        }

        /* Check if the device is a VPID handle or not? */
        if (dev->vpid_handle != -1)
        {
            /* The only missing information not available at the PID layer is the VPID handle on which the packet was actually received/transmitted */
            ingress_property->vpid_handle = dev->vpid_handle;
        }

        /* Get the PID type */
        if (ingress_property->pid_type == AVALANCHE_PP_PID_TYPE_UNDEFINED)
        {
            AVALANCHE_PP_PID_t *pid;
            if (avalanche_pp_pid_get_info(dev->pid_handle, &pid) != PP_RC_SUCCESS)
            {
                return -1;
            }

            ingress_property->pid_type = pid->type;
            ingress_property->lookup.LUT1.u.fields.L2.pid_handle = pid->pid_handle;
        }

        if (hil_get_layers_pointers(ptr_data, &ptr_ethhdr, &ptr_pppoehdr, &ptr_iphdr, &ptr_ipv6hdr) != 0)
        {
            return -1;
        }

        if (ptr_ipv6hdr != NULL)
        {
        
            if (hil_scan_ipv6(ptr_ipv6hdr, &ipv6HeaderLen, &nexthdr, &ipv6Offs, 0, 0, ExtTunnel, True, False) != 0)
            {
                return -1;
            }
            if (nexthdr == IPPROTO_GRE)
            {
                ExtTunnel = PP_IP_PROT_GRE;
                ptr_data = (Uint8*)ptr_ipv6hdr + ipv6HeaderLen;         /* Go to GRE header */

                if (AVALANCHE_PP_PID_TYPE_DOCSIS == ingress_property->pid_type)
                {
                    memcpy(ingress_property->ipAddressForChecksum.ipAdress.Ipv6, ptr_ipv6hdr->daddr.s6_addr32, sizeof(ptr_ipv6hdr->daddr.s6_addr32)); 
                }
                else
                {
                    memcpy(ingress_property->ipAddressForChecksum.ipAdress.Ipv6, ptr_ipv6hdr->saddr.s6_addr32, sizeof(ptr_ipv6hdr->saddr.s6_addr32));
                }

            }
        }
        else if (ptr_iphdr != NULL)
        {
            
            if (ptr_iphdr->protocol == IPPROTO_GRE)
            {
                        ExtTunnel = PP_IP_PROT_GRE;
                        ptr_data = (Uint8*)ptr_iphdr + (ptr_iphdr->ihl * 4);    /* Go to GRE header */
                        ingress_property->ipAddressForChecksum.ipAdress.natInfo.srcAddIpV4 = ptr_iphdr->saddr; 
                        ingress_property->ipAddressForChecksum.ipAdress.natInfo.dstAddIpv4 = ptr_iphdr->daddr; 
                        ingress_property->ipAddressForChecksum.ipAdress.natInfo.tos = ptr_iphdr->tos; 

            }
        }

        if (ExtTunnel == PP_IP_PROT_GRE)
        {
                prot = *((Uint16 *)(ptr_data + 2));

                Uint16 flags = *((Uint16 *)(ptr_data));
                if (flags & 0x2000) // key bit is set: L3 GRE 8 bytes header
                {
                    ptr_data += 4;
                }
                switch (prot) 
                {
                case ETH_P_TEB:
                    break;
                case ETH_P_IP:
                    ptr_iphdr = ptr_data + 4;
                    ptr_ipv6hdr = NULL;
                    break;
                case ETH_P_IPV6:
                    ptr_ipv6hdr = ptr_data + 4;
                    ptr_iphdr = NULL;
                    break;
                default:
                    return -1;  /* unsupported GRE */
                }

            /* Supported GRE */
            ingress_property->lookup.LUT1.u.fields.L3.entry_type = AVALANCHE_PP_LUT_ENTRY_L3_GRE;
            ingress_property->lookup.LUT1.u.fields.L3.enable_flags |= AVALANCHE_PP_LUT1_FIELD_ENABLE_L3_ENTRY_TYPE;
            ingress_property->isTunnel = True;
            ingress_property->tunnel_type = AVALANCHE_PP_LUT_ENTRY_L3_GRE;
                
            ptr_data += 4; /* Skip GRE header */
            if (prot == ETH_P_TEB) 
            {
                if (hil_get_layers_pointers(ptr_data, &ptr_ethhdr, &ptr_pppoehdr, &ptr_iphdr, &ptr_ipv6hdr) != 0)
                {
                    return -1;
                }
            }

            /* Support Fragmentation at Ingress for possible GRE by pass */
            acceptFragL3 = True;

        }

        /* At this stage all the headers have been located and are pointing at the correct locations. Time to start populating the Packet Properties */

        /* Extract Layer2 information */
        if (hil_extract_l2_ingress(ptr_ethhdr, ingress_property, skb->vpid_vlan_tci) != 0)
        {
            return -1;
        }

        /* dirty hack: GRE L3 gets special treatment */
        if (ExtTunnel == PP_IP_PROT_GRE && (prot == ETH_P_IP || prot == ETH_P_IPV6)) 
        {
            /*use the Ether type of the encapsulated L3 */
            ingress_property->lookup.LUT1.u.fields.L2.eth_type = prot;

            /* Set VPID's VLAN (if exists) */
            AVALANCHE_PP_VPID_INFO_t *ptr_vpid;
            avalanche_pp_vpid_get_info(ingress_property->vpid_handle, &ptr_vpid);
            
            /* If GRE L3, use external VPID VLAN */
            if (ptr_vpid->type == AVALANCHE_PP_VPID_VLAN)
            {
                if ( (skb->vpid_vlan_tci & VLAN_VID_MASK) != (ptr_vpid->vlan_identifier) )
                {
				   return -1;
                }
                ingress_property->lookup.LUT2.u.fields.firstVLAN = skb->vpid_vlan_tci;
                ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_1ST_VLAN;
            }
        }

        /* Add extraction of PPPoE fields */
        if (ptr_pppoehdr != NULL)
        {
            ingress_property->isTunnel = True;
            ingress_property->lookup.LUT1.u.fields.L3.PPPoE_session_id = __constant_ntohs(ptr_pppoehdr->length);
            acceptFragL3 = True;
        }
        else
        {
            ingress_property->lookup.LUT1.u.fields.L3.PPPoE_session_id = AVALANCHE_PP_SESSION_PPPOE_INVALID;
        }

        /* Extract Layer3 information */
        if (ptr_iphdr != NULL)
        {
            if (hil_extract_ipv4_ingress(ptr_iphdr, &ptr_l4hdr, ingress_property, acceptFragL3, ExtTunnel) != 0)
            {
                return -1;
            }
        }
        else if (ptr_ipv6hdr != NULL)
        {
            if (hil_extract_ipv6_ingress(ptr_ipv6hdr, &ptr_l4hdr, ingress_property, acceptFragL3, ExtTunnel) != 0)
            {
                return -1;
            }
        }

        if (ingress_property->flags & AVALANCHE_PP_INGRESS_SESSEION_PROPERTY_FLAGS_DO_L2_CLASSIFICATION)
        {
            ptr_l4hdr = NULL;
        }

        /* Extract Layer4 information */
        if (ptr_l4hdr != NULL)
        {
            if (hil_extract_l4_ingress(ptr_l4hdr, ingress_property) != 0)
            {
                return -1;
            }
        }

        ingress_property->lookup.LUT2.u.fields.entry_type = ingress_property->lookup.LUT1.u.fields.L3.entry_type;
    }

    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_extract_l2_ingress
 **************************************************************************
 * DESCRIPTION   :
 *  Extract L2 fields
 *
 * RETURNS       :
 *  0 for success
 **************************************************************************/
static Int32 hil_extract_l2_ingress(struct ethhdr* ptr_ethhdr, AVALANCHE_PP_INGRESS_SESSION_PROPERTY_t* ingress_property, Uint16 vpid_vlan_tci)
{
    Bool l2_classification_enabled;
    Uint8 l2_classification_pid;
    AVALANCHE_PP_VPID_INFO_t *ptr_vpid;
    avalanche_pp_local_dev_addr_ioctl_params_t param;
    Bool addrExist;

    addrExist = False;

    /* Check did we have an Ethernet header. */
    if (ptr_ethhdr == NULL)
        return -1;

    avalanche_pp_get_l2_classification (&l2_classification_enabled, &l2_classification_pid);
    if (l2_classification_enabled && ((l2_classification_pid == ingress_property->lookup.LUT1.u.fields.L2.pid_handle) || (l2_classification_pid == PP_PID_NUM_ALL)))
    {
        param.op_type = IS_ADDR_EXIST;
        memcpy(param.u.mac_addr, (void *)&ptr_ethhdr->h_dest, 6);
        if (AVALANCHE_PP_PID_TYPE_DOCSIS == ingress_property->pid_type)//check GW ADDR, WAN, MTA
        {
            if (IS_HYBRID())
            {
               param.addr_type = GW_MAC_ADDR;
               if(PP_RC_SUCCESS == avalanche_pp_local_dev_addr(&param))
               {
                  addrExist = True;
               }
            }
            param.addr_type = MTA_MAC_ADDR;
            if((!addrExist) && (PP_RC_SUCCESS == avalanche_pp_local_dev_addr(&param)))
            {
                addrExist = True;
            }
            param.addr_type = WAN_MAC_ADDR;
            if((!addrExist) && (PP_RC_SUCCESS == avalanche_pp_local_dev_addr(&param)))
            {
                addrExist = True;
            }
        }
        else //check Rnd Addr
        {
           if (IS_HYBRID())
           {
               param.addr_type = RND_MAC_ADDR;
               if(PP_RC_SUCCESS == avalanche_pp_local_dev_addr(&param))//if the packet is NOT routed and we dont have a match then Do L2 classification
               {
                  addrExist = True;
               }
           }
        }
        if(!addrExist)
        {
           ingress_property->flags |= AVALANCHE_PP_INGRESS_SESSEION_PROPERTY_FLAGS_DO_L2_CLASSIFICATION;
        }
    }

    /* Ethernet header was detected. Initialize the various fields */
    ingress_property->lookup.LUT1.u.fields.L2.entry_type = AVALANCHE_PP_LUT_ENTRY_L2_ETHERNET;
    ingress_property->lookup.LUT1.u.fields.L2.enable_flags |= AVALANCHE_PP_LUT1_FIELD_ENABLE_L2_ENTRY_TYPE;


    /* Set VPID's VLAN (if exists) */
    avalanche_pp_vpid_get_info(ingress_property->vpid_handle, &ptr_vpid);
    
    /* If ingress pkt is GRE L2 -  vlan field in LUT2 key, is only the inner L2 vlan */
    if ((ptr_vpid->type == AVALANCHE_PP_VPID_VLAN) && (ingress_property->lookup.LUT1.u.fields.L3.entry_type != AVALANCHE_PP_LUT_ENTRY_L3_GRE))
    {
        if ( (vpid_vlan_tci & VLAN_VID_MASK) != (ptr_vpid->vlan_identifier) )
        {
            printk("hil_extract_l2_ingress: Cannot create session since there is no-match in VPID VLAN identifier %d %d\n",(vpid_vlan_tci & VLAN_VID_MASK), (ptr_vpid->vlan_identifier));
            return -1;
        }
        ingress_property->lookup.LUT2.u.fields.firstVLAN = vpid_vlan_tci;
        ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_1ST_VLAN;
    }

    /* Set MAC SRC/DST */
    memcpy(ingress_property->lookup.LUT1.u.fields.L2.dstmac, (void *)&ptr_ethhdr->h_dest, 6);
    memcpy(ingress_property->lookup.LUT1.u.fields.L2.srcmac, (void *)&ptr_ethhdr->h_source, 6);
    ingress_property->lookup.LUT1.u.fields.L2.enable_flags |= AVALANCHE_PP_LUT1_FIELD_ENABLE_MAC_DST | AVALANCHE_PP_LUT1_FIELD_ENABLE_MAC_SRC;


    /* Set packet's VLAN (if exists) */
    if (ptr_ethhdr->h_proto == __constant_htons(ETH_P_8021Q))
    {
        struct vlan_hdr* ptr_vlanheader = (struct vlan_hdr *)((Int8*)ptr_ethhdr + sizeof(struct ethhdr));      /* Get the VLAN header */

        if (ingress_property->lookup.LUT2.u.fields.enable_flags & AVALANCHE_PP_LUT2_FIELD_ENABLE_1ST_VLAN)
        {
            ingress_property->lookup.LUT2.u.fields.secondVLAN = ptr_vlanheader->h_vlan_TCI;
            ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_2ND_VLAN;
        }
        else
        {
            ingress_property->lookup.LUT2.u.fields.firstVLAN = ptr_vlanheader->h_vlan_TCI;
            ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_1ST_VLAN;
        }

        ingress_property->lookup.LUT1.u.fields.L2.eth_type = ptr_vlanheader->h_vlan_encapsulated_proto;
    }
    else
    {
        ingress_property->lookup.LUT1.u.fields.L2.eth_type = ptr_ethhdr->h_proto;
    }

    ingress_property->lookup.LUT1.u.fields.L2.enable_flags |= AVALANCHE_PP_LUT1_FIELD_ENABLE_ETH_TYPE;

    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_extract_ipv4_ingress
 **************************************************************************
 * DESCRIPTION   :
 *  Extract IPv4 fields
 *
 * RETURNS       :
 *  0 for success
 **************************************************************************/
static Int32 hil_extract_ipv4_ingress(struct iphdr* ptr_iphdr,
                                        void  ** ptr_l4hdr,
                                        AVALANCHE_PP_INGRESS_SESSION_PROPERTY_t* ingress_property,
                                        Bool acceptFragL3,
                                        hil_pp_ip_prot_e ExtTunnel)
{
    struct icmphdr* ptr_icmphdr;

    /* Check that packet IP Protocol is supported */
    if (hil_check_ip_protocol(ptr_iphdr->protocol, True, ExtTunnel, True) != 0)
    {
        return -1;
    }

    if ((IPPROTO_GRE == ptr_iphdr->protocol)&&(ExtTunnel == PP_IP_PROT_GRE))
    {
        ingress_property->isGreGre = 1;
    }

    /* For IPSEC packets set LUT1 entry type */
    if ( (ptr_iphdr->protocol == IPPROTO_AH)|| (ptr_iphdr->protocol == IPPROTO_ESP) )
    {
        ingress_property->tunnel_type = AVALANCHE_PP_LUT_ENTRY_L3_IPSEC;
        ingress_property->isTunnel = True;
        /* Update only for this function */
        acceptFragL3 = True;
    }

    /* Ensure ipv4 frag rules, We allow process of ipv4 frag. packet only if first frag. and tunneled (by acceptFragL3) */
    if ((ptr_iphdr->frag_off & IP_OFFSET) || ((ptr_iphdr->frag_off & IP_MF) && (acceptFragL3 == False)))
    {
        return -1;
    }

    /* For both TCP & UDP protocols the PORT information lies at the same location. */
    if ((ptr_iphdr->protocol == IPPROTO_TCP) || (ptr_iphdr->protocol == IPPROTO_UDP))
    {
        *ptr_l4hdr = (void *)((Int8 *)ptr_iphdr + ptr_iphdr->ihl * 4);
    }
    else if (ptr_iphdr->protocol == IPPROTO_ICMP)
    {
        ptr_icmphdr = (struct icmphdr *)((Int8 *)ptr_iphdr + ptr_iphdr->ihl * 4);
        if( ((ptr_icmphdr->code == 0) && ((ptr_icmphdr->type == ICMP_ECHO)||(ptr_icmphdr->type == ICMP_ECHOREPLY)) && global_hil_db.ping_acceleration))
        {
            *ptr_l4hdr = (void *)((Int8 *)ptr_iphdr + ptr_iphdr->ihl * 4);
        }
    }

    /* IPv4 header was detected. Initialize the various fields */
    if (ingress_property->lookup.LUT1.u.fields.L3.entry_type == AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED)
    {
        ingress_property->lookup.LUT1.u.fields.L3.entry_type = AVALANCHE_PP_LUT_ENTRY_L3_IPV4; 
        ingress_property->lookup.LUT1.u.fields.L3.enable_flags |= AVALANCHE_PP_LUT1_FIELD_ENABLE_L3_ENTRY_TYPE;
    }
    
    if (AVALANCHE_PP_PID_TYPE_DOCSIS == ingress_property->pid_type)
    {
        ingress_property->lookup.LUT1.u.fields.L3.LAN_addr_IP.v4 = ptr_iphdr->daddr;
        ingress_property->lookup.LUT2.u.fields.WAN_addr_IP.v4 = ptr_iphdr->saddr;
    }
    else
    {
        ingress_property->lookup.LUT1.u.fields.L3.LAN_addr_IP.v4 = ptr_iphdr->saddr;
        ingress_property->lookup.LUT2.u.fields.WAN_addr_IP.v4 = ptr_iphdr->daddr;
    }


    ingress_property->lookup.LUT2.u.fields.TOS = ptr_iphdr->tos;
    ingress_property->lookup.LUT1.u.fields.L3.ip_protocol = ptr_iphdr->protocol;

    ingress_property->lookup.LUT1.u.fields.L3.enable_flags |= AVALANCHE_PP_LUT1_FIELD_ENABLE_LAN_IPv4 | AVALANCHE_PP_LUT1_FIELD_ENABLE_IP_PROTOCOL;
    ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_WAN_IP | AVALANCHE_PP_LUT2_FIELD_ENABLE_IP_TOS;
    
    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_extract_ipv6_ingress
 **************************************************************************
 * DESCRIPTION   :
 *  extracts HIL relevant fields from the IPv6 header
 *
 * RETURNS       :
 *  0 for success
 **************************************************************************/
static Int32 hil_extract_ipv6_ingress(struct ipv6hdr* ptr_ipv6hdr,
                                        void** ptr_l4hdr,
                                        AVALANCHE_PP_INGRESS_SESSION_PROPERTY_t* ingress_property,
                                        Bool acceptFragL3,
                                        hil_pp_ip_prot_e ExtTunnel)
{
    Uint8 nexthdr;
    Uint8 ipv6HeaderLen;
    Int16 fragOffs;
    struct icmp6hdr* ptr_icmp6hdr;

    if (hil_scan_ipv6(ptr_ipv6hdr, &ipv6HeaderLen, &nexthdr, &fragOffs, 0, 0, ExtTunnel, True, True) != 0)
    {
        return -1;
    }

    /* Enforce L3 frag rules if frag detected */
    if (fragOffs != -1)
    {
        /* Continue only if first fragment and DSlite or other supported tunnel (by acceptFragL3) */
        if (!(((nexthdr == IPPROTO_IPIP)||(nexthdr == IPPROTO_AH)||(nexthdr == IPPROTO_ESP) || (acceptFragL3 == True)) && !fragOffs))
        {
            return -1;
        }
    }

    if (nexthdr == IPPROTO_IPIP)
    {
        /* DSLite */
        ingress_property->isTunnel = True;
        struct iphdr *ptr_dsLiteIphdr = (struct iphdr *)((Uint8 *)ptr_ipv6hdr + ipv6HeaderLen);

        /* Check if we support the IP */
        if (hil_check_ip_protocol(ptr_dsLiteIphdr->protocol, True, PP_IP_PROT_DSLITE, True) != 0)
        {
            return -1;
        }

        /* For IPSEC packets set LUT1 entry type to Multi Tunnels since this packet is also Ds Lite */
        if ( (ptr_dsLiteIphdr->protocol == IPPROTO_AH)|| (ptr_dsLiteIphdr->protocol == IPPROTO_ESP))
        {
            ingress_property->tunnel_type = AVALANCHE_PP_LUT_ENTRY_L3_IPSEC;
        }

        if ((ptr_dsLiteIphdr->protocol == IPPROTO_TCP) || (ptr_dsLiteIphdr->protocol == IPPROTO_UDP))
        {
            /* Fragment is only handled if first fragment and outer L3 is not fragged */
            if ((ptr_dsLiteIphdr->frag_off & IP_OFFSET) || ((ptr_dsLiteIphdr->frag_off & IP_MF) && (fragOffs != -1)))
            {
                return -1;
            }

            *ptr_l4hdr = (void *)((Uint8 *)(ptr_dsLiteIphdr) + ptr_dsLiteIphdr->ihl * 4);
        }


        ingress_property->lookup.LUT1.u.fields.L3.entry_type = AVALANCHE_PP_LUT_ENTRY_L3_DSLITE;
        ingress_property->lookup.LUT1.u.fields.L3.enable_flags |= AVALANCHE_PP_LUT1_FIELD_ENABLE_L3_ENTRY_TYPE;
        ingress_property->lookup.LUT1.u.fields.L3.ip_protocol = IPPROTO_IPIP;
        ingress_property->lookup.LUT1.u.fields.L3.enable_flags |= AVALANCHE_PP_LUT1_FIELD_ENABLE_IP_PROTOCOL;
        ingress_property->lookup.LUT2.u.fields.IP.v4_dsLite = ptr_dsLiteIphdr->daddr;           /* Save dsLite_dst_ip to be able to classify according to it */
        ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_DSLITE_IPV4; 

    }
    else
    {
        if ((nexthdr == IPPROTO_TCP) || (nexthdr == IPPROTO_UDP))
        {
            *ptr_l4hdr = (void *)((Uint8 *)ptr_ipv6hdr + ipv6HeaderLen);
        }
        /* For IPSEC packets */
        else if ( (nexthdr == IPPROTO_AH)|| (nexthdr == IPPROTO_ESP) )
        {
            ingress_property->tunnel_type = AVALANCHE_PP_LUT_ENTRY_L3_IPSEC;
            ingress_property->isTunnel = True;
        }
        /* For Ping */
        else if (nexthdr == IPPROTO_ICMPV6)
        {
            ptr_icmp6hdr = (struct icmp6hdr *)((Uint8 *)ptr_ipv6hdr + ipv6HeaderLen);
            if ((ptr_icmp6hdr->icmp6_code == 0) && (ptr_icmp6hdr->icmp6_type == ICMPV6_ECHO_REQUEST) && global_hil_db.ping_acceleration)
            {
                ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_ICMP_ECHO_REQ;
            }
            else if ((ptr_icmp6hdr->icmp6_code == 0) && (ptr_icmp6hdr->icmp6_type == ICMPV6_ECHO_REPLY) && global_hil_db.ping_acceleration)
            {
                ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_ICMP_ECHO_REP;
            }
        }
        if (ingress_property->lookup.LUT1.u.fields.L3.entry_type == AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED)
        {
            ingress_property->lookup.LUT1.u.fields.L3.entry_type = AVALANCHE_PP_LUT_ENTRY_L3_IPV6;
            ingress_property->lookup.LUT1.u.fields.L3.enable_flags |= AVALANCHE_PP_LUT1_FIELD_ENABLE_L3_ENTRY_TYPE;
        }
        memcpy(ingress_property->lookup.LUT2.u.fields.IP.v6_FlowLabel, ptr_ipv6hdr->flow_lbl, sizeof(ptr_ipv6hdr->flow_lbl));
        ingress_property->lookup.LUT2.u.fields.IP.v6_FlowLabel[0] &= 0xF;   // 4 MSbits belongs to Traffic Class field
        ingress_property->lookup.LUT2.u.fields.IP.v6_FlowLabel[3] = 0;      // Make sure last byte is cleared
        ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_IPV6_FLOW;
       
    }

    if (AVALANCHE_PP_PID_TYPE_DOCSIS == ingress_property->pid_type)
    {
        memcpy(ingress_property->lookup.LUT1.u.fields.L3.LAN_addr_IP.v6, ptr_ipv6hdr->daddr.s6_addr32, sizeof(ptr_ipv6hdr->daddr.s6_addr32));
        memcpy(ingress_property->lookup.LUT2.u.fields.WAN_addr_IP.v6, ptr_ipv6hdr->saddr.s6_addr32, sizeof(ptr_ipv6hdr->saddr.s6_addr32));
    }
    else
    {
        memcpy(ingress_property->lookup.LUT1.u.fields.L3.LAN_addr_IP.v6, ptr_ipv6hdr->saddr.s6_addr32, sizeof(ptr_ipv6hdr->saddr.s6_addr32));
        memcpy(ingress_property->lookup.LUT2.u.fields.WAN_addr_IP.v6, ptr_ipv6hdr->daddr.s6_addr32, sizeof(ptr_ipv6hdr->daddr.s6_addr32));
    }

    ingress_property->lookup.LUT2.u.fields.TOS = IPV6_TRAFFIC_CLASS(ptr_ipv6hdr);
    if ( !(ingress_property->lookup.LUT1.u.fields.L3.enable_flags & AVALANCHE_PP_LUT1_FIELD_ENABLE_IP_PROTOCOL) )
    {
        ingress_property->lookup.LUT1.u.fields.L3.ip_protocol = ptr_ipv6hdr->nexthdr;
        ingress_property->lookup.LUT1.u.fields.L3.enable_flags |= AVALANCHE_PP_LUT1_FIELD_ENABLE_IP_PROTOCOL;
    }
    

    ingress_property->lookup.LUT1.u.fields.L3.enable_flags |= AVALANCHE_PP_LUT1_FIELD_ENABLE_LAN_IPv6;
    ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_WAN_IP | AVALANCHE_PP_LUT2_FIELD_ENABLE_IP_TOS;
    
    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_extract_l4_ingress
 **************************************************************************
 * DESCRIPTION   :
 *  extracts HIL relevant fields from the L4 header
 *
 * RETURNS       :
 *  0 for success
 **************************************************************************/
static Int32 hil_extract_l4_ingress(void* ptr_l4hdr, AVALANCHE_PP_INGRESS_SESSION_PROPERTY_t* ingress_property)
{
    if ((ptr_l4hdr != NULL))
    {
        switch (ingress_property->lookup.LUT1.u.fields.L3.ip_protocol)
        {
        case IPPROTO_ICMP:
            {
                struct icmphdr *ptr_icmphdr = (struct icmphdr *)ptr_l4hdr;
                if ((ptr_icmphdr->code == 0) && (ptr_icmphdr->type == ICMP_ECHO) && global_hil_db.ping_acceleration)
                {
                    ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_ICMP_ECHO_REQ |AVALANCHE_PP_LUT2_FIELD_ENABLE_SRC_PORT ;
                }
                else // ICMP_ECHOREPLY
                {
                    ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_ICMP_ECHO_REP |AVALANCHE_PP_LUT2_FIELD_ENABLE_SRC_PORT ;
                }
                ingress_property->lookup.LUT2.u.fields.L4_SrcPort =  ptr_icmphdr->un.echo.id;
                ingress_property->lookup.LUT2.u.fields.L4_DstPort = 0;
            }
            break;

        case IPPROTO_IPIP:
        case IPPROTO_UDP:
        case IPPROTO_TCP:
        default:
            {
                struct tcphdr *ptr_tcphdr = (struct tcphdr *)ptr_l4hdr;
                ingress_property->lookup.LUT2.u.fields.L4_SrcPort = ptr_tcphdr->source;
                ingress_property->lookup.LUT2.u.fields.L4_DstPort = ptr_tcphdr->dest;
                ingress_property->lookup.LUT2.u.fields.enable_flags |= AVALANCHE_PP_LUT2_FIELD_ENABLE_SRC_PORT | AVALANCHE_PP_LUT2_FIELD_ENABLE_DST_PORT;
            }
            break;
        }
    }

   return 0;
}


/**************************************************************************
 * FUNCTION NAME : hil_egress_hook
 **************************************************************************
 * DESCRIPTION   :
 *  The function is the egress hook that is called when a packet is to be transmitted on a session interfaces.
 *  The function extracts the information pertinent to creation of the session interface. It then checks if this packet was "routed/bridged".
 *  If yes then control is passed to the Plugin Logic to determine creation of the session.
 *
 * RETURNS:
 *  Always returns 0.
 **************************************************************************/
Int32 hil_egress_hook(struct sk_buff* skb)
{
    AVALANCHE_PP_RET_e rc;
    AVALANCHE_PP_SESSION_INFO_t* session_info = &skb->pp_packet_info.pp_session; /* Get the pointer to the session information block */
    static Uint8 six_zero_bytes[6] = {0x00,0x00,0x00,0x00,0x00,0x00};

    /*mac header points to the wrong address in case of dslite tunneling. 
      the reset operation moves the pointer to the correct location.
      in case of non tunneling it will not change its position*/
    skb_reset_mac_header(skb);

#if defined(CONFIG_NF_CONNTRACK) || defined(CONFIG_NF_CONNTRACK_MODULE)
    __hil_calc_bithash(skb);

    /* If this function returns true we need to bypass egress hook */
    if (__hil_scb_is_egress_bypass(skb))
    {
        return 0;
    }
#endif

    session_info->egress.isGreGre = 0;
    session_info->session_handle = AVALANCHE_PP_MAX_ACCELERATED_SESSIONS;

    // TBD TI_PPM_EGRESS_QUEUE_INVALID
    if (TI_PPM_EGRESS_QUEUE_INVALID == skb->pp_packet_info.egress_queue)
    {
        /* Scale down the priority of the egress queue */
        if ((skb->dev->vpid_block.qos_clusters_count) && (!global_hil_db.qos_disabled))
        {
            if (NULL != skb->dev->qos_select_hook)
            {
                skb->pp_packet_info.egress_queue = skb->dev->qos_select_hook(skb);
            }
        }
        else 
        {
            session_info->cluster  = 0xFF;
            session_info->priority = 0;
        }
        /************************************************************************/
    }

    if (global_hil_db.hil_disabled || (!avalanche_pp_state_is_active()))
    {
        return 0;
    }

    if ((skb->pp_packet_info.flags & TI_HIL_PACKET_FLAG_PP_SESSION_INGRESS_RECORDED) == 0)
    {
        /* If the Packet has not HIT the ingress hook there is no point in creating the session since the packet is locally generated */
        global_hil_db.num_other_pkts++;
        return 0;
    }

    if  (skb->pp_packet_info.flags & TI_HIL_PACKET_FLAG_PP_SESSION_BYPASS)
    {
        /* The Host Intelligence layers have decided not to "acclerate" this session */
        global_hil_db.num_bypassed_pkts++;
        return 0;
    }

    global_hil_db.num_egress_pkts++;

    /* We can only proceed to the next step if all the information has been extracted */
    if (skb->dev->pid_handle == -1)
    {
        return 0;
    }

    /* Extract all the fields from the packet and populate the Egress Packet Descriptor */
    if (hil_extract_packet_egress(skb, skb->dev, session_info) < 0)
    {
        global_hil_db.num_egress_pkt_fail_extract++;
        return 0;
    }

    if (session_info->egress.enable & AVALANCHE_PP_EGRESS_FIELD_ENABLE_TCP_SYN)
    {
        /* In case it's a SYN packet make it expedited in DOCSIS Upstream */
        skb->ti_meta_info |= DOCSIS_FW_PACKET_API_TCP_HIGH_PRIORITY;
    }

    /* Check if the session is ROUTABLE or not by comparing the L2 destination MAC Address at the Ingress and Egress.
     * If they are not the same it is safe to assume that the session was routed */
    session_info->is_routable_session = 0; /* Default to bridged */

    if ((session_info->ingress.lookup.LUT1.u.fields.L2.entry_type == AVALANCHE_PP_LUT_ENTRY_L2_ETHERNET) && (session_info->egress.l2_packet_type == AVALANCHE_PP_LUT_ENTRY_L2_ETHERNET))
    {

        if (memcmp((void *)&session_info->ingress.lookup.LUT1.u.fields.L2.dstmac, (void *)&session_info->egress.dstmac, 6) != 0)
        {

            // For GRE L2 (but not GRE L3) egress packet need to compare with the inner L2 MAC address
            if ((session_info->egress.isTunnel) && (session_info->egress.tunnel_type == AVALANCHE_PP_LUT_ENTRY_L3_GRE) && 
                (memcmp((void *)&session_info->egress.greInnerDstMac, (void *)six_zero_bytes, 6) != 0))
            {

                if (memcmp((void *)&session_info->ingress.lookup.LUT1.u.fields.L2.dstmac, (void *)&session_info->egress.greInnerDstMac, 6) != 0)
                {
                    /* Check if this is GRE by pass */
                    if (session_info->ingress.lookup.LUT1.u.fields.L3.entry_type != AVALANCHE_PP_LUT_ENTRY_L3_GRE)
                    {
                        /* GRE US - support routed session */
                        session_info->is_routable_session = 1; 
                    } 
                    else
                    {
                                        
                        if ( ((session_info->egress.isGreGre) &&(!session_info->ingress.isGreGre))||
                            ((!session_info->egress.isGreGre) &&(session_info->ingress.isGreGre)) )
                        {

                            session_info->is_routable_session = 1;
                        }
                        else
                        {   
                            /* (GRE in GRE)/GRE by pass - support only bridge session */
                            global_hil_db.num_egress_pkt_gre_bypassed++;
                            return 0;
                        }
                    }
                }

            } 
            else
            {

                session_info->is_routable_session = 1; /* The destination MAC address are not the same; most definately a routed session */
            }

        }
    }

    /* Skip session inteligence */
    if ((session_info->ingress.flags & AVALANCHE_PP_INGRESS_SESSEION_PROPERTY_FLAGS_DO_L2_CLASSIFICATION) == 0) 
    {
        /* Once all the fields have been extracted. Check if the session can be created or not? */
        if (hil_session_intelligence(session_info) == 0)
        {
            global_hil_db.num_egress_pkt_fail_sess_int++;
            return 0;
        }
    }

    if( hil_choose_session_timeout(session_info) < 0 )
    {
        return 0;
    }

    /* For Ping sessions, minimum timeout is 2 SEC to prevent timeout of 1 sec which is the default for ping pps */
    if ((session_info->ingress.lookup.LUT1.u.fields.L3.ip_protocol == IPPROTO_ICMP) || (session_info->ingress.lookup.LUT1.u.fields.L3.ip_protocol == IPPROTO_ICMPV6))
    {
        if (pp_session_timeout_sec == DEFAULT_SESSION_TIMEOUT_SEC)
        {
            session_info->session_timeout = DEFAULT_ICMP_SESSION_TIMEOUT_SEC * 1000000;
        }
    }

    if (session_info->egress.enable & AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED)
    {
        skb->ti_meta_info |= DOCSIS_FW_PACKET_API_TCP_HIGH_PRIORITY;
    }

    if (hil_choose_session_pool(session_info) < 0)
    {
        return 0;
    }

    __hil_calc_mac_hash(session_info, True);

    rc = avalanche_pp_session_create(session_info, (void*)skb);

    if (rc == PP_RC_SUCCESS)
    {
        if (global_hil_db.tdox_dbg)
        {
            printk("TDOX-DBG CreateSession: ses=%d, vpid=%d, protocol=%d, src=%d, dst=%d\n", session_info->session_handle, session_info->egress.vpid_handle, session_info->egress.ip_protocol, session_info->egress.L4_SrcPort, session_info->egress.L4_DstPort);

            if (session_info->egress.enable & AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED)
            {
                printk("TDOX-DBG AssociateTdox (Start): ses=%d, tdox=%d\n", session_info->session_handle, session_info->egress.tdox_handle);
            }
        }
    }
    else
    {
        __hil_calc_mac_hash(session_info, False);
    }

    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_extract_packet_egress
 **************************************************************************
 * DESCRIPTION   :
 *  The function is called to extract the various Layer2, Layer3 and Layer4 fields and populate the packet description structure for egress packet
 * 
 * RETURNS       :
 *  0   -   Success
 *  <0  -   Error
 **************************************************************************/
static Int32 hil_extract_packet_egress(struct sk_buff* skb, struct net_device* dev, AVALANCHE_PP_SESSION_INFO_t* session_info)
{
    struct ethhdr* ptr_ethhdr = NULL;
    struct pppoe_hdr* ptr_pppoehdr = NULL;
    struct iphdr* ptr_iphdr = NULL;
    struct iphdr* ptr_dsLiteIphdr = NULL;
    void * ptr_l4hdr = NULL;
    struct ipv6hdr* ptr_ipv6hdr = NULL;
    Int8* ptr_data = (Int8 *)skb_mac_header(skb);
    AVALANCHE_PP_EGRESS_SESSION_PROPERTY_t* egress_property = &session_info->egress;

    /* Set some fields with default values before start working */
    egress_property->enable = 0;
    egress_property->l2_packet_type = AVALANCHE_PP_LUT_ENTRY_L2_UNDEFINED;
    egress_property->l3_packet_type = AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED;
    egress_property->tunnel_type    = AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED;
    egress_property->isTunnel = False;

    /* Flag indicating whether the fragmented datagram can be processed.
     * We allow process of L3-frag. packet only if first frag. and:
     *  - Outer DSLite
     *  - Inner DSLite
     *  - Outer GRE
     *  - PPPoE'ed traffic
     *
     */
    Bool                acceptFragL3 = False;

#ifdef CONFIG_TI_META_DATA
    egress_property->psi_word = skb->ti_meta_info;
    egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_PSI;
#endif
    
    /* Check if the device is a PID or not? */
    if (dev->pid_handle != -1)
    {
        /* Valid PID Handle: Packet has been received at the lowest level; at this point in time we can extract all the L2/L3/L4 information from the packet */
        if( !ptr_data )
        {
            /* Without this data we cannot continue */
            return -1;
        }

        /* Check if the device is a VPID handle or not? */
        if (dev->vpid_handle != -1)
        {
            /* The only missing information not available at the PID layer is the VPID handle on which the packet was actually received/transmitted */
            egress_property->vpid_handle = dev->vpid_handle;
        }

        /* Get the PID type */
        if (egress_property->pid_type == AVALANCHE_PP_PID_TYPE_UNDEFINED)
        {
            AVALANCHE_PP_PID_t *pid;

            if (avalanche_pp_pid_get_info(dev->pid_handle, &pid) != PP_RC_SUCCESS)
            {
                return -1;
            }
            egress_property->pid_type = pid->type;
        }

        if (hil_get_layers_pointers(ptr_data, &ptr_ethhdr, &ptr_pppoehdr, &ptr_iphdr, &ptr_ipv6hdr) != 0)
        {
            return -1;
        }

        /* At this stage all the headers have been located and are pointing at the correct locations. Time to start populating the Packet Properties */

        /* Extract Layer2 information */
        if (hil_extract_l2_egress(ptr_ethhdr, skb, egress_property) != 0)
        {
            return -1;
        }

        /* Add extraction of PPPoE fields */
        if (ptr_pppoehdr != NULL)
        {
            /* For anywan */
            egress_property->isTunnel = True;
            acceptFragL3 = True;

            egress_property->pppoe_sid = __constant_ntohs(ptr_pppoehdr->sid);
            if (ptr_iphdr != NULL)
            {
                egress_property->wrapHeaderDataLenOffset = (Uint8*)ptr_iphdr - (Uint8*)ptr_ethhdr + offsetof(struct iphdr, tot_len);
            }
            else if (ptr_ipv6hdr != NULL)
            {
                egress_property->wrapHeaderDataLenOffset = (Uint8*)ptr_ipv6hdr - (Uint8*)ptr_ethhdr + offsetof(struct ipv6hdr, payload_len);
            }
        }
        else
        {
            egress_property->pppoe_sid = AVALANCHE_PP_SESSION_PPPOE_INVALID;
        }

        /* Extract Layer3 information */
        if (ptr_iphdr != NULL)
        {
            if (hil_extract_ipv4_egress(ptr_ethhdr, ptr_iphdr, &ptr_l4hdr, session_info, acceptFragL3) != 0)
            {
                return -1;
            }
        }
        else if (ptr_ipv6hdr != NULL)
        {
            if (hil_extract_ipv6_egress(ptr_ethhdr, ptr_ipv6hdr, &ptr_l4hdr, &ptr_dsLiteIphdr, session_info, acceptFragL3) != 0)
            {
                return -1;
            }
        }

        
        /* Extract Layer4 information */
        if (ptr_l4hdr != NULL)
        {
            if (hil_extract_l4_egress(ptr_l4hdr, ptr_iphdr, ptr_ipv6hdr, egress_property) != 0)
            {
                return -1;
            }

            if (hil_tdox_check_and_enable(ptr_l4hdr, session_info) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_extract_l2_egress
 **************************************************************************
 * DESCRIPTION   :
 *  Extract L2 fields
 *
 * RETURNS       :
 *  0 for success
 **************************************************************************/
static Int32 hil_extract_l2_egress(struct ethhdr* ptr_ethhdr, struct sk_buff* skb, AVALANCHE_PP_EGRESS_SESSION_PROPERTY_t* egress_property)
{
    /* Check did we have an Ethernet header. */
    if (ptr_ethhdr != NULL)
    {
        /* Ethernet header was detected. Initialize the various fields */
        egress_property->l2_packet_type = AVALANCHE_PP_LUT_ENTRY_L2_ETHERNET;

        /* Set MAC SRC/DST */
        memcpy(egress_property->dstmac, (void *)&ptr_ethhdr->h_dest, 6);
        memcpy(egress_property->srcmac, (void *)&ptr_ethhdr->h_source, 6);
        egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_L2;

        /* Set packet's VLAN (if exists) */
        if (ptr_ethhdr->h_proto == __constant_htons(ETH_P_8021Q))
        {
            struct vlan_hdr* ptr_vlanheader = (struct vlan_hdr *)((Int8*)ptr_ethhdr + sizeof(struct ethhdr));      /* Get the VLAN header */

            egress_property->vlan = ptr_vlanheader->h_vlan_TCI;
            egress_property->vlan |= VLAN_PRIO_MASK & (((Uint16)(skb->ti_meta_info)) << VLAN_PRIO_SHIFT);
            egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_VLAN;

            egress_property->eth_type = ptr_vlanheader->h_vlan_encapsulated_proto;
        }
        else if ( vlan_tx_tag_present(skb) )
        {
            egress_property->vlan = vlan_tx_tag_get(skb);
            egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_VLAN;
            egress_property->eth_type = ptr_ethhdr->h_proto;
        }
        else
        {
            AVALANCHE_PP_VPID_INFO_t *ptr_vpid;

            avalanche_pp_vpid_get_info(egress_property->vpid_handle, &ptr_vpid);

            if (ptr_vpid->type == AVALANCHE_PP_VPID_VLAN)
            {
                egress_property->vlan |= VLAN_PRIO_MASK & (((Uint16)(skb->ti_meta_info)) << VLAN_PRIO_SHIFT);
            }

            egress_property->eth_type = ptr_ethhdr->h_proto;
        }
    }

    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_extract_ipv4_egress
 **************************************************************************
 * DESCRIPTION   :
 *  Extract IPv4 fields
 *
 * RETURNS       :
 *  0 for success
 **************************************************************************/
static Int32 hil_extract_ipv4_egress(struct ethhdr* ptr_ethhdr,
                                        struct iphdr* ptr_iphdr,
                                        void** ptr_l4hdr,
                                        AVALANCHE_PP_SESSION_INFO_t* session_info,
                                        Bool acceptFragL3)
{
    Int8* ptr_data;
    Int16 fragOffs;
    struct iphdr* gre_inner_ipv4hdr = NULL;
    struct ipv6hdr* gre_inner_ipv6hdr = NULL;
    AVALANCHE_PP_EGRESS_SESSION_PROPERTY_t* egress_property = &session_info->egress;
    Bool    greByPass = False;
    hil_pp_ip_prot_e   ipProt =  PP_IP_PROT_NA;

    /* Ensure ipv4 fag rules, We allow process of ipv4 frag. packet only if first frag. and tunneled (by acceptFragL3) or GRE */
    if ((ptr_iphdr->frag_off & IP_OFFSET) ||
            ((ptr_iphdr->frag_off & IP_MF) && 
            !((acceptFragL3 == True) || (ptr_iphdr->protocol == IPPROTO_GRE)|| (ptr_iphdr->protocol == IPPROTO_ESP)|| (ptr_iphdr->protocol == IPPROTO_AH))) )
    {
        return -1;
    }

    if (ptr_iphdr->protocol == IPPROTO_GRE)
    {
        __sum16 savedCheck;
        __be16  savedTotLen;
        __be16  savedId;
        Bool    err = False;
        __be16  savedFragOff;
        Uint16  encap_prot;
        Uint16  gre_flags;
        

        /* For AnyWan */
        egress_property->isTunnel = True;

        /* Check if we support this type of US GRE by looking at the GRE protocol type field */
        ptr_data = (Uint8*)ptr_iphdr + (ptr_iphdr->ihl * 4);    /* Go to GRE header */
        encap_prot = *((Uint16 *)(ptr_data + 2));

        switch (encap_prot) 
        {
        case ETH_P_TEB:
        case ETH_P_IP:
        case ETH_P_IPV6:
            break;
        default:
            return -1;  /* unsupported GRE */
        }

        gre_flags = *((Uint16 *)(ptr_data));
        if (gre_flags & 0x2000) // key bit is set, GRE header is 8 bytes
        {
            ptr_data += 4;
        }

        /* Supported US GRE */
        egress_property->tunnel_type = AVALANCHE_PP_LUT_ENTRY_L3_GRE;
        ipProt = PP_IP_PROT_GRE;
        /* Now need to calculate checksum with payload length=0 and identification is incremented by =0x100
           (we start with Identification high number so that host and PP packets will not have same Identification at session start) */
        /* Save the origianl values */
        savedCheck = ptr_iphdr->check;
        savedTotLen = ptr_iphdr->tot_len;
        savedId = ptr_iphdr->id;
        savedFragOff = ptr_iphdr->frag_off;

        /* Change them to allow better PP utilization */
        ptr_iphdr->check = 0;
        ptr_iphdr->tot_len = 0;
        ptr_iphdr->id = 0xFFF;
        ptr_iphdr->frag_off = 0; /* offset is 0 anyway */

        ptr_iphdr->check = hil_ipv4_checksum((Uint8*)ptr_iphdr, ptr_iphdr->ihl * 4);



        /* Setup ipv4HdrRaw parameters - Save encapsulating L2, L3, GRE and encapsulated L2 */
        egress_property->wrapHeaderDataLenOffset = (Uint8*)ptr_iphdr - (Uint8*)ptr_ethhdr + offsetof(struct iphdr, tot_len);
        egress_property->wrapHeaderLen = (ptr_iphdr->ihl * 4) + 4 + sizeof(struct ethhdr); /* 4 is for the GRE header */
        

        if (encap_prot == ETH_P_TEB) 
        {
            if (egress_property->wrapHeaderLen <= AVALNCHE_PP_WRAP_HEADER_MAX_LEN)
            {
                struct ethhdr* encap_ptr_ethhdr;
                Uint16 encap_prortocol_type;
               
                /*  For GRE US -Save encapsulating L2, L3, GRE and encapsulated L2 */
                memcpy(egress_property->wrapHeader, ptr_iphdr, egress_property->wrapHeaderLen);
                egress_property->wrapHeader_type = AVALANCHE_PP_LUT_ENTRY_L3_IPV4;
                

                /* If the encapsulated packet contains VLAN then add it to the template */
                encap_ptr_ethhdr = (struct ethhdr *)(ptr_data + 4); /* Get the pointer to the encapsulated Ethernet header */
                encap_prortocol_type = encap_ptr_ethhdr->h_proto;   /* Get the protocol type. */

                /* Set MAC SRC/DST from the inner MAC addrs of the inner L2 */
                memcpy(egress_property->greInnerDstMac, (void *)&encap_ptr_ethhdr->h_dest, 6);
                memcpy(egress_property->greInnerSrcMac, (void *)&encap_ptr_ethhdr->h_source, 6);


                /* Check the protocol field. If the protocol is VLAN or not?  */
                if (encap_prortocol_type == __constant_htons(ETH_P_8021Q))
                {
                    if ( egress_property->wrapHeaderLen + VLAN_HLEN <= AVALNCHE_PP_WRAP_HEADER_MAX_LEN)
                    {
                        /* Get the VLAN header. */
                        struct vlan_hdr* ptr_vlanheader_gre = (struct vlan_hdr *)((Uint8*)encap_ptr_ethhdr + ETH_HLEN);

                        /* Add the VLAN header to the template */
                        memcpy(egress_property->wrapHeader + egress_property->wrapHeaderLen, ptr_vlanheader_gre, VLAN_HLEN);
                        egress_property->wrapHeaderLen += VLAN_HLEN;
                        
                        
                        if (ptr_vlanheader_gre->h_vlan_encapsulated_proto == __constant_htons(ETH_P_IP))
                        {
                            gre_inner_ipv4hdr = (struct iphdr*) (((char*) ptr_vlanheader_gre) + VLAN_HLEN);
                        }
                        else if (ptr_vlanheader_gre->h_vlan_encapsulated_proto == __constant_htons(ETH_P_IPV6))
                        {
                            gre_inner_ipv6hdr = (struct ipv6hdr*) (((char*) ptr_vlanheader_gre) + VLAN_HLEN);
                        }
                        else
                        {
                            err = True;
                        }
                    }
                    else
                    {
                        /* There is not enough space to save the US GRE Encapsulation Header */
                        err = True;
                    }
                }
                else if (encap_prortocol_type == __constant_htons(ETH_P_IP))
                {
                    gre_inner_ipv4hdr = (struct iphdr*) (((char*) encap_ptr_ethhdr) + ETH_HLEN);
                }
                else if (encap_prortocol_type == __constant_htons(ETH_P_IPV6))
                {
                    gre_inner_ipv6hdr = (struct ipv6hdr*) (((char*) encap_ptr_ethhdr) + ETH_HLEN);
                }
                else
                {
                    //printk("%s %d\n", __FUNCTION__, __LINE__);
                    err = True;
                }
               
    #if 0
                printk("%s[%d]: wrapHeader= %08X.%08X.%08X.%08X.%08X.%08X.%08X.%08X.%08X.%08X.%08X.%08X.%08X\n", __FUNCTION__, __LINE__,
                       *(Uint32*)&egress_property->wrapHeader[0], *(Uint32*)&egress_property->wrapHeader[4], *(Uint32*)&egress_property->wrapHeader[8],
                       *(Uint32*)&egress_property->wrapHeader[12], *(Uint32*)&egress_property->wrapHeader[16], *(Uint32*)&egress_property->wrapHeader[20],
                       *(Uint32*)&egress_property->wrapHeader[24], *(Uint32*)&egress_property->wrapHeader[28], *(Uint32*)&egress_property->wrapHeader[32],
                       *(Uint32*)&egress_property->wrapHeader[36], *(Uint32*)&egress_property->wrapHeader[40], *(Uint32*)&egress_property->wrapHeader[44]);
    #endif
            }
            else
            {
                /* There is not enough space to save the US GRE Encapsulation Header */
                //printk("%s %d\n", __FUNCTION__, __LINE__);
                err = True;
            }
        } /* endif encap_prot == TEB */
        else if (encap_prot == ETH_P_IP)
        {
            if (egress_property->wrapHeaderLen <= AVALNCHE_PP_WRAP_HEADER_MAX_LEN)
            {
                gre_inner_ipv4hdr = (struct iphdr*) ((char*) (ptr_data + 4));
            }
            else
            {
                err = True;
            }
        }
        else if (encap_prot == ETH_P_IPV6)
        {
            if (egress_property->wrapHeaderLen <= AVALNCHE_PP_WRAP_HEADER_MAX_LEN)
            {
                gre_inner_ipv6hdr = (struct ipv6hdr*) ((char*) (ptr_data + 4));
            }
            else
            {
                err = True;
            }
        }
        else
        {
            //printk("%s something wrong with GRE protocol type\n", __FUNCTION__);
        }

        /* Restore the origianl values */
        ptr_iphdr->check = savedCheck;
        ptr_iphdr->tot_len = savedTotLen;
        ptr_iphdr->id = savedId;
        ptr_iphdr->frag_off = savedFragOff;
        if (err)
        {
            return -1;
        }
        else
        {
            egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_ENCAPSULATION; 
        }
    }

    if (gre_inner_ipv6hdr)
    {
        Uint8  ipv6HeaderLen;
        Uint8  nexthdr;

        if (hil_scan_ipv6(gre_inner_ipv6hdr, &ipv6HeaderLen, &nexthdr, &fragOffs, 0, 0, ipProt, False, True) != 0)
        {
            return -1;
        }
    
         /* Check if packet hit ingress hook with GRE - check if  this is GRE by pass or GRE GRE DS */
        if (session_info->ingress.lookup.LUT1.u.fields.L3.entry_type == AVALANCHE_PP_LUT_ENTRY_L3_GRE )
        {
            /* If this is not GRE in GRE */
            if (!(nexthdr == IPPROTO_GRE))  
            {
                /* If this is not GRE GRE DS - set GRE by pass flag */
                if(!(session_info->ingress.lookup.LUT1.u.fields.L3.ip_protocol == IPPROTO_GRE))
                {
                    greByPass = True;
                }
                /* For GRE GRE DS and for GRE BP - no warp header is needed */
                egress_property->enable &= ~AVALANCHE_PP_EGRESS_FIELD_ENABLE_ENCAPSULATION;
                memset(egress_property->wrapHeader, 0, AVALNCHE_PP_WRAP_HEADER_MAX_LEN);
                egress_property->wrapHeaderLen = 0;
                egress_property->wrapHeaderDataLenOffset = 0;
            }
            else 
            {
                session_info->egress.isGreGre = 1;
            }
        }

        /* we do not support inner GRE L3 frag except for: GRE BP and IPSEC */
        if ((fragOffs != -1)&& (!greByPass)&& (!( (nexthdr == IPPROTO_AH)|| (nexthdr == IPPROTO_ESP) ) ) )
        {
            return -1;
        }

        /* If this is DS Gre Gre do not extacrt the inner fileds*/
        if ((!session_info->ingress.isGreGre))
        {
            if (nexthdr == IPPROTO_TCP || nexthdr == IPPROTO_UDP)
            {
                *ptr_l4hdr = (void *)((Uint8 *)gre_inner_ipv6hdr + ipv6HeaderLen);
            }
            

            /* IPv6 header was detected. Initialize the various fields. */
            if (egress_property->l3_packet_type == AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED)
            {
                egress_property->l3_packet_type = AVALANCHE_PP_LUT_ENTRY_L3_IPV6;
            }

            memcpy(egress_property->DST_IP.v6, gre_inner_ipv6hdr->daddr.s6_addr32, sizeof(gre_inner_ipv6hdr->daddr.s6_addr32));
            memcpy(egress_property->SRC_IP.v6, gre_inner_ipv6hdr->saddr.s6_addr32, sizeof(gre_inner_ipv6hdr->saddr.s6_addr32));
            egress_property->TOS = IPV6_TRAFFIC_CLASS(gre_inner_ipv6hdr);
            egress_property->ip_protocol = nexthdr;
            egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_IP;

            return 0;
        }
    }
    /* in case of GRE, use encapsulated ip info */
    else if (gre_inner_ipv4hdr)
    {
        struct ptr_iphdr* ptr_iphdr_temp = ptr_iphdr;
        ptr_iphdr = gre_inner_ipv4hdr;

        /* Check if IP Protocol is supported */ 
        if( hil_check_ip_protocol(ptr_iphdr->protocol , False, ipProt, True) != 0 )
        {
            return -1;
        }

        /* Check if packet hit ingress hook with GRE - check if this is GRE by pass or GRE GRE DS  */
        if (session_info->ingress.lookup.LUT1.u.fields.L3.entry_type == AVALANCHE_PP_LUT_ENTRY_L3_GRE )
        {
            /* If this is not GRE in GRE US, then this is GRE BP or GRE GRE DS, no GRE warp header needs to be added. */
            if (!(ptr_iphdr->protocol == IPPROTO_GRE))
            {
               /*If this is not GRE GRE DS - set GRE By Pass flag */
               if(!(session_info->ingress.lookup.LUT1.u.fields.L3.ip_protocol == IPPROTO_GRE)) 
               {
                   greByPass = True;
               }
               /* For GRE GRE DS and for GRE BP - no warp header is needed */
                egress_property->enable &= ~AVALANCHE_PP_EGRESS_FIELD_ENABLE_ENCAPSULATION;
                memset(egress_property->wrapHeader, 0, AVALNCHE_PP_WRAP_HEADER_MAX_LEN);
                egress_property->wrapHeaderLen = 0;
                egress_property->wrapHeaderDataLenOffset = 0;
            }
            else 
            {
                session_info->egress.isGreGre = 1;             
            }

        }

        /* we do not support inner GRE L3 frag unless its GRE by pass and IPSEC*/
        if( (gre_inner_ipv4hdr->frag_off & (IP_OFFSET | IP_MF)) && (!greByPass) && (!( (ptr_iphdr->protocol == IPPROTO_AH)|| (ptr_iphdr->protocol == IPPROTO_ESP) ) ))
        {
            return -1;
        }
        /*For DS GRE (egress L2,L3,GRE,L3,L4   ingress L2,L3,GRE,L2,L3,GRE,L3,L4)*/
        if (session_info->ingress.isGreGre)
        {
            ptr_iphdr = ptr_iphdr_temp;
        }
    } 
    /* Check if IP Protocol is supported */ 
    else if( hil_check_ip_protocol(ptr_iphdr->protocol , False, ipProt, True) != 0 )
    {
        return -1;
    }

    /* For IPSEC packets mark that egress packet has tunnel */
    if ( (ptr_iphdr->protocol == IPPROTO_AH)|| (ptr_iphdr->protocol == IPPROTO_ESP) )
    { 
        /* Only if this is not GRE packet*/
        if (!egress_property->isTunnel)
        {
            egress_property->tunnel_type =  AVALANCHE_PP_LUT_ENTRY_L3_IPSEC;
            egress_property->isTunnel = True;      
        }
    }

    /* If Protocol is TCP or UDP. For both these protocols the PORT information lies at the same location. */
    if ((ptr_iphdr->protocol == IPPROTO_TCP) || (ptr_iphdr->protocol == IPPROTO_UDP))
    {
        *ptr_l4hdr = (void *)((Int8 *)ptr_iphdr + ptr_iphdr->ihl*4);
    }
    else if (ptr_iphdr->protocol == IPPROTO_ICMP)
    {

        struct icmphdr *ptr_icmphdr = (struct icmphdr *)((Int8 *)ptr_iphdr + ptr_iphdr->ihl * 4);
        if( ((ptr_icmphdr->code == 0) && ((ptr_icmphdr->type == ICMP_ECHO)||(ptr_icmphdr->type == ICMP_ECHOREPLY)) && global_hil_db.ping_acceleration))
        {
            *ptr_l4hdr = (void *)((Int8 *)ptr_iphdr + ptr_iphdr->ihl * 4);
        }
        else
        {
            return -1;
        }
    }

    /* IPv4 header was detected. Initialize the various fields. */
    if (egress_property->l3_packet_type == AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED)
    {
        egress_property->l3_packet_type = AVALANCHE_PP_LUT_ENTRY_L3_IPV4; 
    }
    /*For up stream GRE (ingress L2,L3,GRE,L3,L4   egress L2,L3,GRE,L2,L3,GRE,L3,L4)*/
    if ((session_info->egress.isGreGre)&&(!session_info->ingress.isGreGre))
    {
        ptr_iphdr = gre_inner_ipv4hdr;
    }
    
    egress_property->DST_IP.v4 = ptr_iphdr->daddr;
    egress_property->SRC_IP.v4 = ptr_iphdr->saddr;
    egress_property->TOS = ptr_iphdr->tos;
    egress_property->ip_protocol = ptr_iphdr->protocol;
    egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_IP;

    
    return 0;
}

/**************************************************************************
 * FUNCTION NAME : __hil_ipv6hdr_to_wraphdr
 **************************************************************************
 * DESCRIPTION   :
 *  Builds ipv6 part wrap header from ipv6 header, deleting fragmentation header if exists
 *
 * RETURNS       :
 *  void
 **************************************************************************/
static inline void __hil_build_wraphdr_ipv6(AVALANCHE_PP_EGRESS_SESSION_PROPERTY_t* egress_property,
                                            struct ipv6hdr* ptr_ipv6hdr,
                                            Int16 fragOffs,
                                            Uint8 fragHdrOffs,
                                            Uint8 prevFragHdrOffs)
{

    /* Situation in which Frag. header exists but has MF==0 and Offs==0 is not expected on eggress,
     * This is subject to change in case we will want to accelerate tunnels in bridge mode
     */
    if (fragOffs == -1)
    {
        /* No fragmentation header */
        memcpy(egress_property->wrapHeader, ptr_ipv6hdr, egress_property->wrapHeaderLen);
    }
    else
    {
        /* Copy eliminating frag header, since in case of accelerated fragmentation, fw will add frag header */
        memcpy(egress_property->wrapHeader, ptr_ipv6hdr, fragHdrOffs);
        memcpy(egress_property->wrapHeader + fragHdrOffs, ((Uint8*) ptr_ipv6hdr) + fragHdrOffs + 8, egress_property->wrapHeaderLen - (fragHdrOffs + 8));

        egress_property->wrapHeaderLen -= 8;

        /* need to update previous header at PrevFragHdrOffs */
        egress_property->wrapHeader[prevFragHdrOffs] = *(((Uint8 *) ptr_ipv6hdr) + fragHdrOffs);
    }
}

/**************************************************************************
 * FUNCTION NAME : hil_extract_ipv6_egress
 **************************************************************************
 * DESCRIPTION   :
 *  extracts HIL relevant fields from the IPv6 header
 *
 * RETURNS       :
 *  0 for success
 **************************************************************************/
static Int32 hil_extract_ipv6_egress(struct ethhdr* ptr_ethhdr,
                                        struct ipv6hdr* ptr_ipv6hdr,
                                        void** ptr_l4hdr,
                                        struct iphdr** ptr_dsLiteIphdr,
                                        AVALANCHE_PP_SESSION_INFO_t* session_info,
                                        Bool acceptFragL3)
{
    Uint8  ipv6HeaderLen;
    Uint8  nexthdr;
    Int16 fragOffs;
    Uint8 fragHdrOffs;
    Uint8 prevFragHdrOffs;
    Bool    greByPass = False;
    AVALANCHE_PP_EGRESS_SESSION_PROPERTY_t* egress_property = &session_info->egress;
    hil_pp_ip_prot_e   ipProt =  PP_IP_PROT_NA;
    
    if (hil_scan_ipv6(ptr_ipv6hdr, &ipv6HeaderLen, &nexthdr, &fragOffs, &fragHdrOffs, &prevFragHdrOffs, ipProt, False, True) != 0)
    {
        return -1;
    }

    /* Enforse L3 frag rules */
    if (fragOffs != -1)
    {
        /* Accept first frags of DSLite or other tunnels */
        if (fragOffs ||
            !((nexthdr == IPPROTO_IPIP) || (nexthdr == IPPROTO_GRE)|| (nexthdr == IPPROTO_AH)|| (nexthdr == IPPROTO_ESP) || (acceptFragL3 == True))) 
        {
            return -1;
        }
    }

    if (nexthdr == IPPROTO_IPIP)
    {
        /* For AnyWan */
        egress_property->isTunnel = True;

        *ptr_dsLiteIphdr = (struct iphdr *)((Uint8 *)ptr_ipv6hdr + ipv6HeaderLen);

        ipProt = PP_IP_PROT_DSLITE;
        /* Check if IP Protocol is supported */ 
        if( hil_check_ip_protocol((*ptr_dsLiteIphdr)->protocol, False, ipProt, True) != 0 )
        {
            return -1;
        }

        if (((*ptr_dsLiteIphdr)->protocol == IPPROTO_TCP) || ((*ptr_dsLiteIphdr)->protocol == IPPROTO_UDP))
        {
            /* Fragment is only handled if first fragment and outer L3 is not fragged */
            if (((*ptr_dsLiteIphdr)->frag_off & IP_OFFSET) || (((*ptr_dsLiteIphdr)->frag_off & IP_MF) && (fragOffs != -1)))
            {
                return -1;
            }

            *ptr_l4hdr = (void *)((Uint8 *)(*ptr_dsLiteIphdr) + (*ptr_dsLiteIphdr)->ihl * 4);
        }
        
        if (ipv6HeaderLen > AVALNCHE_PP_WRAP_HEADER_MAX_LEN)
        {
            ipv6HeaderLen = AVALNCHE_PP_WRAP_HEADER_MAX_LEN;
        }


        if (AVALANCHE_PP_LUT_ENTRY_L3_DSLITE == session_info->ingress.lookup.LUT1.u.fields.L3.entry_type) /* DSLITE BYPASS*/
        {
                /* For DSLITE BYPASS- no wrap header is needed */
                egress_property->enable &= ~AVALANCHE_PP_EGRESS_FIELD_ENABLE_ENCAPSULATION;
                memset(egress_property->wrapHeader, 0, AVALNCHE_PP_WRAP_HEADER_MAX_LEN);
                egress_property->wrapHeaderLen = 0;
                egress_property->wrapHeaderDataLenOffset = 0;
        }
        else
        {
            egress_property->wrapHeaderDataLenOffset = (Uint8*)ptr_ipv6hdr - (Uint8*)ptr_ethhdr + offsetof(struct ipv6hdr, payload_len);
            egress_property->wrapHeaderLen = ipv6HeaderLen;

            __hil_build_wraphdr_ipv6(egress_property,
                                    ptr_ipv6hdr,
                                    fragOffs,
                                    fragHdrOffs,
                                    prevFragHdrOffs);

            egress_property->wrapHeader_type = AVALANCHE_PP_LUT_ENTRY_L3_IPV6;
            ((struct ipv6hdr*)egress_property->wrapHeader)->payload_len = 0; // reset the payload length value as it is irrelevant for the session creation and the DsLite template
            egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_ENCAPSULATION;
        }
        
        egress_property->tunnel_type = AVALANCHE_PP_LUT_ENTRY_L3_DSLITE;
        egress_property->inner_ip_protocol = (*ptr_dsLiteIphdr)->protocol;

        /* end of DSLite processing */
    }
    else if (nexthdr == IPPROTO_GRE)
    {
        struct iphdr* gre_inner_ipv4hdr = NULL;
        struct ipv6hdr* gre_inner_ipv6hdr = NULL;
        Uint16 prot, gre_flags;


        egress_property->isTunnel = True;
        ipProt = PP_IP_PROT_GRE;    
        Bool err = False;

        /* Check if we support this type of US GRE by looking at the GRE protocol type field */
        Uint8* ptr_data = (Uint8*)ptr_ipv6hdr + ipv6HeaderLen;         /* Go to GRE header */
        prot = *((Uint16 *)(ptr_data + 2));
        
        switch (prot) 
        {
        case ETH_P_TEB:
        case ETH_P_IP:
        case ETH_P_IPV6:
            break;
        default:
            return -1;  /* unsupported GRE */
        }

        gre_flags = *((Uint16 *)(ptr_data));
        if (gre_flags & 0x2000) // key bit is set, GRE header is 8 bytes
        {
            ptr_data += 4;
        }        


        /* Supported US GRE */
        egress_property->tunnel_type = AVALANCHE_PP_LUT_ENTRY_L3_GRE;


        /* Setup ipv4HdrRaw parameters - Save encapsulating L2, L3, GRE and encapsulated L2 */
        egress_property->wrapHeaderDataLenOffset = (Uint8 *)ptr_ipv6hdr - (Uint8 *)ptr_ethhdr + offsetof(struct ipv6hdr, payload_len);
        egress_property->wrapHeaderLen = ipv6HeaderLen + 4 + sizeof(struct ethhdr); /* 4 is for the GRE header */
        
        if (prot == ETH_P_TEB)
        {
            if (egress_property->wrapHeaderLen <= AVALNCHE_PP_WRAP_HEADER_MAX_LEN)
            {
                struct ethhdr* encap_ptr_ethhdr;
                Uint16 encap_prortocol_type;

              

                __hil_build_wraphdr_ipv6(egress_property,
                                             ptr_ipv6hdr,
                                             fragOffs,
                                             fragHdrOffs,
                                             prevFragHdrOffs);

                egress_property->wrapHeader_type = AVALANCHE_PP_LUT_ENTRY_L3_IPV6;
                

                /* If the encapsulated packet contains VLAN then add it to the template */
                encap_ptr_ethhdr = (struct ethhdr *)(ptr_data + 4); /* Get the pointer to the encapsulated Ethernet header */
                encap_prortocol_type = encap_ptr_ethhdr->h_proto;   /* Get the protocol type. */

                /* Set MAC SRC/DST from the inner MAC addrs of the inner L2 */
                memcpy(egress_property->greInnerDstMac, (void *)&encap_ptr_ethhdr->h_dest, 6);
                memcpy(egress_property->greInnerSrcMac, (void *)&encap_ptr_ethhdr->h_source, 6);

                /* Check the protocol field. If the protocol is VLAN or not?  */
                if (encap_prortocol_type == __constant_htons(ETH_P_8021Q))
                {
                    /* Get the VLAN header. */
                    struct vlan_hdr* ptr_vlanheader_gre = (struct vlan_hdr *)((Uint8*)encap_ptr_ethhdr + ETH_HLEN);

                    if (egress_property->wrapHeaderLen + VLAN_HLEN <= AVALNCHE_PP_WRAP_HEADER_MAX_LEN)
                    {
                        
                        /* Add the VLAN header to the template */
                        memcpy(egress_property->wrapHeader + egress_property->wrapHeaderLen, ptr_vlanheader_gre, VLAN_HLEN);
                        egress_property->wrapHeaderLen += VLAN_HLEN;
                        
                        
                    }
                    else
                    {
                        /* There is not enough space to save the US GRE Encapsulation Header */
                        err = True;
                    }

                    if (ptr_vlanheader_gre->h_vlan_encapsulated_proto == __constant_htons(ETH_P_IP))
                    {
                        gre_inner_ipv4hdr = (struct iphdr*) (((char*) ptr_vlanheader_gre) + VLAN_HLEN);
                    }
                    else if (ptr_vlanheader_gre->h_vlan_encapsulated_proto == __constant_htons(ETH_P_IPV6))
                    {
                        gre_inner_ipv6hdr = (struct ipv6hdr*) (((char*) ptr_vlanheader_gre) + VLAN_HLEN);
                    }
                    else
                    {
                        err = True;
                    }
                }
                else if (encap_prortocol_type == __constant_htons(ETH_P_IP))
                {
                    gre_inner_ipv4hdr = (struct iphdr*) (((char*) encap_ptr_ethhdr) + ETH_HLEN);
                }
                else if (encap_prortocol_type == __constant_htons(ETH_P_IPV6))
                {
                    gre_inner_ipv6hdr = (struct ipv6hdr*) (((char*) encap_ptr_ethhdr) + ETH_HLEN);
                }
                else
                {
                    err = True;
                }
            }
            else
            {
                err = True;
            }
        } /* endif encap_prot == TEB */
        else if (prot == ETH_P_IP)
        {
            if (egress_property->wrapHeaderLen <= AVALNCHE_PP_WRAP_HEADER_MAX_LEN)
            {
                gre_inner_ipv4hdr = (struct iphdr*) ((char*) (ptr_data + 4));
            }
            else
            {
                err = True;
            }
        }
        else if (prot == ETH_P_IPV6)
        {
            if (egress_property->wrapHeaderLen <= AVALNCHE_PP_WRAP_HEADER_MAX_LEN)
            {
                gre_inner_ipv6hdr = (struct ipv6hdr*) ((char*) (ptr_data + 4));
            }
            else
            {
                err = True;
            }
        }
        else
        {
            //printk("%s something wrong with GRE protocol type\n", __FUNCTION__);
        }

        if (err)
        {
            printk("US GRE: not enough space for the header\n");
            return -1;
        }
        else
        {
            /* Enable Wrap header */
            egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_ENCAPSULATION;
            
            if (gre_inner_ipv4hdr)
            {
                /* Check if IP Protocol is supported */ 
                if( hil_check_ip_protocol(gre_inner_ipv4hdr->protocol, False, ipProt, True) != 0 )
                {
                    return -1;
                }

                /* Check if packet hit ingress hook with GRE - check this is GRE by pass or GRE GRE DS  */
                if (session_info->ingress.lookup.LUT1.u.fields.L3.entry_type == AVALANCHE_PP_LUT_ENTRY_L3_GRE )
                {
                    /* If this is not GRE GRE US */
                    if (!(gre_inner_ipv4hdr->protocol == IPPROTO_GRE)) 
                    {
                        /*If this is not GRE GRE DS - set GRE By Pass flag */
                        if(!(session_info->ingress.lookup.LUT1.u.fields.L3.ip_protocol == IPPROTO_GRE))
                        {
                            greByPass = True;
                        }
                        /* For GRE GRE DS and for GRE BP - no warp header is needed */
                        egress_property->enable &= ~AVALANCHE_PP_EGRESS_FIELD_ENABLE_ENCAPSULATION;
                        memset(egress_property->wrapHeader, 0, AVALNCHE_PP_WRAP_HEADER_MAX_LEN);
                        egress_property->wrapHeaderLen = 0;
                        egress_property->wrapHeaderDataLenOffset = 0;
                    }
                    else
                    {
                        session_info->egress.isGreGre = 1;
                    }

                } 
                else if ((gre_inner_ipv4hdr->protocol == IPPROTO_TCP) || (gre_inner_ipv4hdr->protocol == IPPROTO_UDP))
                {
                    /* Dont process inner GRE L3 fragment  */
                    if ((gre_inner_ipv4hdr->frag_off & (IP_OFFSET | IP_MF)) && (!greByPass))
                    {
                        return -1;
                    }
                    *ptr_l4hdr = (void *)((Int8 *)gre_inner_ipv4hdr + gre_inner_ipv4hdr->ihl*4);
                }
                /* If this is DS Gre Gre do not extacrt the inner fileds*/
                if (!session_info->ingress.isGreGre)
                {
                    /* IPv4 header was detected. Initialize the various fields. */
                    if (egress_property->l3_packet_type == AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED)
                    {
                        egress_property->l3_packet_type = AVALANCHE_PP_LUT_ENTRY_L3_IPV4;
                    }

                    egress_property->DST_IP.v4 = gre_inner_ipv4hdr->daddr;
                    egress_property->SRC_IP.v4 = gre_inner_ipv4hdr->saddr;
                    egress_property->TOS = gre_inner_ipv4hdr->tos;
                    egress_property->ip_protocol = gre_inner_ipv4hdr->protocol;
                    egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_IP;

                    return 0;
                }
            }

            else if (gre_inner_ipv6hdr)
            {
                Uint8  ipv6HeaderLen;
                Uint8  nexthdr;
                struct ipv6hdr* ptr_ipv6hdrTmp = ptr_ipv6hdr;
                ptr_ipv6hdr = gre_inner_ipv6hdr;

                if (hil_scan_ipv6(ptr_ipv6hdr, &ipv6HeaderLen, &nexthdr, &fragOffs, 0, 0, ipProt, False, True) != 0)
                {
                    return -1;
                }

                /* Check if packet hit ingress hook with GRE - check if this is GRE by pass  or GRE GRE DS */
                if (session_info->ingress.lookup.LUT1.u.fields.L3.entry_type == AVALANCHE_PP_LUT_ENTRY_L3_GRE )
                {
                    /* If this is not GRE GRE US */
                    if(!(nexthdr == IPPROTO_GRE)) 
                    {
                        /*If this is not GRE GRE DS - set GRE By Pass flag */
                        if(!(session_info->ingress.lookup.LUT1.u.fields.L3.ip_protocol == IPPROTO_GRE))
                        {
                            greByPass = True;
                        }
                        /* For GRE GRE DS and for GRE BP - no warp header is needed */
                        egress_property->enable &= ~AVALANCHE_PP_EGRESS_FIELD_ENABLE_ENCAPSULATION;
                        memset(egress_property->wrapHeader, 0, AVALNCHE_PP_WRAP_HEADER_MAX_LEN);
                        egress_property->wrapHeaderLen = 0;
                        egress_property->wrapHeaderDataLenOffset = 0;
                    }
                    else
                    {
                        session_info->egress.isGreGre = 1;
                    }
                }
                if (!session_info->ingress.isGreGre)
                {
                    /* we do not support inner GRE L3 frag */
                    if ((fragOffs != -1)&& (!greByPass) && (!((nexthdr == IPPROTO_AH)|| (nexthdr == IPPROTO_ESP))))
                    {
                     return -1;
                    }

               
                    if (nexthdr == IPPROTO_TCP || nexthdr == IPPROTO_UDP)
                    {
                    *ptr_l4hdr = (void *)((Uint8 *)ptr_ipv6hdr + ipv6HeaderLen);
                    }
                }
                else
                {
                     ptr_ipv6hdr = ptr_ipv6hdrTmp;
                }
                

            }
        }
        /* end of GRE processing */
    }
    else if (nexthdr == IPPROTO_TCP || nexthdr == IPPROTO_UDP)
    {
        *ptr_l4hdr = (void *)((Uint8 *)ptr_ipv6hdr + ipv6HeaderLen);
    }
    /* If IPV6 NAT we not support icmp acceleration */
    else if (IPPROTO_ICMPV6 == nexthdr)
    {
        struct icmp6hdr *ptr_icmp6hdr;
        if (AVALANCHE_PP_PID_TYPE_DOCSIS == session_info->ingress.pid_type)
        {
            if (memcmp((void *)&session_info->ingress.lookup.LUT2.u.fields.WAN_addr_IP.v6, (void *)&ptr_ipv6hdr->saddr.s6_addr32, sizeof(ptr_ipv6hdr->saddr.s6_addr32)) != 0)
            {
                return -1;
            }
        }
        else
        {
            if (memcmp((void *)&session_info->ingress.lookup.LUT1.u.fields.L3.LAN_addr_IP.v6,(void *)&ptr_ipv6hdr->saddr.s6_addr32, sizeof(ptr_ipv6hdr->saddr.s6_addr32))!= 0)
            {
                return -1;
            }

        }

        ptr_icmp6hdr = (struct icmp6hdr *)((Uint8 *)ptr_ipv6hdr + ipv6HeaderLen);
        if (!((ptr_icmp6hdr->icmp6_code == 0) && (global_hil_db.ping_acceleration) &&
            ((ptr_icmp6hdr->icmp6_type == ICMPV6_ECHO_REQUEST) || (ptr_icmp6hdr->icmp6_type == ICMPV6_ECHO_REPLY))))
        {
                return -1;
        }
    }
    /* For IPSEC packets check that this packet was recevied with IPSEC header at ingress hook*/
    else if ( (nexthdr == IPPROTO_AH)|| (nexthdr == IPPROTO_ESP) )
    {
      egress_property->tunnel_type =  AVALANCHE_PP_LUT_ENTRY_L3_IPSEC;
      egress_property->isTunnel = True;      
    } 

    if (egress_property->l3_packet_type == AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED)
    {
        egress_property->l3_packet_type = AVALANCHE_PP_LUT_ENTRY_L3_IPV6;
    }

    memcpy(egress_property->DST_IP.v6, ptr_ipv6hdr->daddr.s6_addr32, sizeof(ptr_ipv6hdr->daddr.s6_addr32));
    memcpy(egress_property->SRC_IP.v6, ptr_ipv6hdr->saddr.s6_addr32, sizeof(ptr_ipv6hdr->saddr.s6_addr32));
    egress_property->TOS = IPV6_TRAFFIC_CLASS(ptr_ipv6hdr);
    egress_property->ip_protocol = ptr_ipv6hdr->nexthdr;
    egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_IP;

    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_extract_l4_egress
 **************************************************************************
 * DESCRIPTION   :
 *  extracts HIL relevant fields from the L4 header
 *
 * RETURNS       :
 *  0 for success
 **************************************************************************/
static Int32 hil_extract_l4_egress(void * ptr_l4hdr, struct iphdr* ptr_iphdr, struct ipv6hdr* ptr_ipv6hdr, AVALANCHE_PP_EGRESS_SESSION_PROPERTY_t* egress_property)
{
    struct icmphdr *ptr_icmphdr;
    struct tcphdr  *ptr_tcphdr;

    if (ptr_l4hdr == NULL)
        return -1;

    switch (egress_property->ip_protocol)
    {
        case IPPROTO_ICMP:
        {
            ptr_icmphdr = (struct icmphdr *)ptr_l4hdr;
            egress_property->L4_SrcPort = ptr_icmphdr->un.echo.id;
            egress_property->L4_DstPort = 0;
            egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_L4;
            break;
        }
        case IPPROTO_IPIP:
        case IPPROTO_UDP:
        case IPPROTO_TCP:
        {
            /* ptr_tcphdr header IS L4 HEADER not manadory that it will be TCP */
            ptr_tcphdr = (struct tcphdr *)ptr_l4hdr;
            egress_property->L4_SrcPort = ptr_tcphdr->source;
            egress_property->L4_DstPort = ptr_tcphdr->dest;
            egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_L4;

            if (egress_property->ip_protocol == IPPROTO_TCP)
            {
                if (ptr_iphdr != NULL)
                {
                    /*check if we didn't pass the packet len, ip total len contains L3 + L4 + payload */
                    if ((ptr_iphdr->tot_len) < ((ptr_iphdr->ihl * 4) + (ptr_tcphdr->doff * 4)))
                    {
                        return -1;
                    }
                }
                else if (ptr_ipv6hdr !=NULL)
                {
                    /*check if we didn't pass the packet len, payload_len contains L4 + payload */
                    if ((ptr_ipv6hdr->payload_len) < (ptr_tcphdr->doff * 4))
                    {
                        return -1;
                    }
                }

                if (ptr_tcphdr->syn || ptr_tcphdr->fin || ptr_tcphdr->rst)
                {
                    /* Setting SYN will skip session opening logic and pass the packet as high priority */
                    /* Setting CTRL will cause the network interface to send the packet without a valid session, so packet will not be discarded */
                    egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_TCP_SYN |
                                               AVALANCHE_PP_EGRESS_FIELD_ENABLE_TCP_CTRL;
                }
            }
            break;
        }
        default:
        {
            return -1;
        }
    }

    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_session_intelligence
 **************************************************************************
 * DESCRIPTION   :
 *  The function is the host intelligence layer which decides if the Session
 *  is worthy of accleration or not?
 *
 * RETURNS:
 *  0   -  Session is NOT accelerated
 *  1   -  Session is accelerated
 **************************************************************************/
static Int32 hil_session_intelligence(AVALANCHE_PP_SESSION_INFO_t* session_info)
{
    /* Acclerate only IPv4 or IPv6 traffic */
    if (session_info->ingress.lookup.LUT1.u.fields.L3.entry_type == AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED)
    {
        return 0;
    }

    /* Don't accelerate MAC Broadcast Packets */
    if ((session_info->ingress.lookup.LUT1.u.fields.L2.enable_flags & AVALANCHE_PP_LUT1_FIELD_ENABLE_MAC_DST) && (session_info->ingress.lookup.LUT1.u.fields.L2.dstmac[0] == 0xFF))
    {
        return 0;
    }

    /* Accelerate only the supported IP protocol*/
    if(hil_check_ip_protocol(session_info->ingress.lookup.LUT1.u.fields.L3.ip_protocol,True, PP_IP_PROT_NA, False) != 0)
    {
        return 0;
    }

    /* If Tunnel Trmination */
    if (((session_info->egress.isTunnel) && (!session_info->ingress.isTunnel))||
        ((!session_info->egress.isTunnel) && (session_info->ingress.isTunnel)) )
    {
        /* Dont Support IPSEC Trmination */
        if( (session_info->ingress.tunnel_type == AVALANCHE_PP_LUT_ENTRY_L3_IPSEC)||
            (session_info->egress.tunnel_type == AVALANCHE_PP_LUT_ENTRY_L3_IPSEC))
        {
            return 0;
        }
        /* Dont support Icmp with tunnel*/
        if ((IPPROTO_ICMP == session_info->ingress.lookup.LUT1.u.fields.L3.ip_protocol)||
            (IPPROTO_ICMP == session_info->egress.ip_protocol )||
            (IPPROTO_ICMPV6 == session_info->ingress.lookup.LUT1.u.fields.L3.ip_protocol)||
            (IPPROTO_ICMPV6 == session_info->egress.ip_protocol))
        {
            return 0;
        }

    }
    /* Check all types of tunnels */
    if (session_info->egress.isTunnel && session_info->ingress.isTunnel)
    {
         Bool acceptTunnel = False;
         if ((AVALANCHE_PP_LUT_ENTRY_L3_DSLITE == session_info->egress.tunnel_type ) && 
             (AVALANCHE_PP_LUT_ENTRY_L3_DSLITE == session_info->ingress.lookup.LUT1.u.fields.L3.entry_type)) 
         {
             /*Support In DsLite ByPass*/
             acceptTunnel = True;
         }

        /* Accept GRE By Pass and GRE in GRE */
        if ((session_info->egress.tunnel_type == AVALANCHE_PP_LUT_ENTRY_L3_GRE) && 
            (session_info->ingress.tunnel_type == AVALANCHE_PP_LUT_ENTRY_L3_GRE))
        {
            acceptTunnel = True;
        }

        /* DS Packet */ 
        if (session_info->ingress.pid_type == AVALANCHE_PP_PID_TYPE_DOCSIS)
        {
            /* In DS - if IPSEC -support only IPSEC By Pass */
            if (session_info->ingress.tunnel_type == AVALANCHE_PP_LUT_ENTRY_L3_IPSEC)
            {
                if (session_info->egress.tunnel_type == AVALANCHE_PP_LUT_ENTRY_L3_IPSEC)
                {
                    acceptTunnel = True;
                }
            }
        }
        /* US Packet */
        else if (session_info->egress.pid_type == AVALANCHE_PP_PID_TYPE_DOCSIS)
        {
            /* Ingress IPSEC  */
            if (session_info->ingress.tunnel_type == AVALANCHE_PP_LUT_ENTRY_L3_IPSEC)
            {
                /* Egress packet can be GRE, Ds-Lite, IPSEC */
                if( (session_info->egress.tunnel_type == AVALANCHE_PP_LUT_ENTRY_L3_IPSEC)||
                    (session_info->egress.tunnel_type == AVALANCHE_PP_LUT_ENTRY_L3_GRE)||
                    (session_info->egress.tunnel_type == AVALANCHE_PP_LUT_ENTRY_L3_DSLITE)) 
                {
                    /* Inner IP Protocol must be IPSEC */
                    if( (session_info->egress.ip_protocol == IPPROTO_AH)||(session_info->egress.ip_protocol == IPPROTO_ESP) )
                    {
                        acceptTunnel = True;
                    } 
                    else if ( (session_info->egress.inner_ip_protocol == IPPROTO_AH) || (session_info->egress.inner_ip_protocol == IPPROTO_ESP))
                    {
                       acceptTunnel = True;
                    }
                }
            }
        }
        /* If this is unsupported type of tunneled session exit */
        if (!acceptTunnel)
        {
            return 0;
        }
    }

    if (session_info->egress.enable & AVALANCHE_PP_EGRESS_FIELD_ENABLE_TCP_SYN)
    {
        /* In case it's a SYN packet - do not open the session yet */
        global_hil_db.num_egress_pkt_tcp_syn++;
        return 0;
    }

    /* Accelerate the session. */
    return 1;
}

/**************************************************************************
 * FUNCTION NAME : hil_choose_session_pool
 **************************************************************************
 * DESCRIPTION:
 *  The function initializes the session_pool parameter according to set of conditions
 *
 * RETURNS:
 *  None
 **************************************************************************/
static Int32 hil_choose_session_pool(AVALANCHE_PP_SESSION_INFO_t* session_info)
{
    AVALANCHE_PP_VPID_INFO_t *vpid;
    AVALANCHE_PP_PID_t *pid;

    session_info->session_pool = AVALANCHE_PP_SESSIONS_POOL_DATA;   /* Default */

    if (avalanche_pp_vpid_get_info(session_info->ingress.vpid_handle, &vpid) != PP_RC_SUCCESS)
    {
        return -1;
    }
    if (avalanche_pp_pid_get_info(vpid->parent_pid_handle, &pid) != PP_RC_SUCCESS)
    {
        return -1;
    }
    if (pid->pid_handle >= PP_C55_PID_BASE && pid->pid_handle < PP_C55_PID_BASE + PP_C55_PID_COUNT)
    {
        session_info->session_pool = AVALANCHE_PP_SESSIONS_POOL_VOICE;
    }
    else
    {
        if (avalanche_pp_vpid_get_info(session_info->egress.vpid_handle, &vpid) != PP_RC_SUCCESS)
        {
            return -1;
        }
        if (avalanche_pp_pid_get_info(vpid->parent_pid_handle, &pid) != PP_RC_SUCCESS)
        {
            return -1;
        }
        if (pid->pid_handle >= PP_C55_PID_BASE && pid->pid_handle < PP_C55_PID_BASE + PP_C55_PID_COUNT)
        {
            session_info->session_pool = AVALANCHE_PP_SESSIONS_POOL_VOICE;
        }
    }
    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_choose_session_timeout
 **************************************************************************
 * DESCRIPTION:
 *  The function initializes the session timeout parameter according to set of conditions
 *
 * RETURNS:
 *  None
 **************************************************************************/
static Int32 hil_choose_session_timeout(AVALANCHE_PP_SESSION_INFO_t* session_info)
{
     /* All sessions created here have a standard session timeout */
     session_info->session_timeout = pp_session_timeout_sec * 1000000;

    /* If default is enable, Set the the Evaluation Timer as the defult, and not according to the active sessions number */
    if (!(global_hil_db.extended_session_timeout))
    {
        return 0;
    }

     /* UDP Sessions have a diffrent session timeout */
    if ((session_info->egress.ip_protocol == IPPROTO_UDP) || (session_info->egress.inner_ip_protocol == IPPROTO_UDP))
    {
        Uint32 num_entries;
        AVALANCHE_PP_VPID_INFO_t * ptr_vpid;
		
        // Get vpid info failed, use a standard session timeout
        if (avalanche_pp_vpid_get_info(session_info->egress.vpid_handle, &ptr_vpid) != PP_RC_SUCCESS)
        {
            printk(KERN_ERR "Get VPID info for egress VPID %d failed\n", session_info->egress.vpid_handle);
            return 0;
        }
        
        //  If Drop session configure to 1 second only
        if (ptr_vpid->flags & (AVALANCHE_PP_VPID_FLG_TX_DISBL | AVALANCHE_PP_VPID_FLG_RX_DISBL))
        {
           printk(KERN_DEBUG "Egress VPID %d has DROP Flags %x \n", session_info->egress.vpid_handle, ptr_vpid->flags);
           return 0;
        }

        /* Get active UDP session number from /to the WAN  */
        if ( avalanche_pp_session_get_list( docsis_vpid_handle, PP_LIST_ID_EGRESS_UDP, &num_entries, NULL) == 0)
        {
            if (num_entries < global_hil_db.extended_timeout_MaxSessions)
            {
                session_info->session_timeout = 60 * 1000000;
            }
        }
    }

    return 0;

}
/**************************************************************************
 * FUNCTION NAME : hil_copy_ingress_to_egress ( AVALANCHE_PP_SESSION_INFO_t* session_info )
 **************************************************************************
 * DESCRIPTION   : the function copied the ungress property
 * from session information to the egress property.
 * We use that function in order to fill the egress
 * property in case of null sessions creation
 * param - session_info - pointer to session information struct (ingress property must be already set)
 * return - void
 **************************************************************************/
static void hil_copy_ingress_to_egress(AVALANCHE_PP_SESSION_INFO_t* session_info)
{
    session_info->egress.enable = 0;

    /* L2 */
    memcpy(session_info->egress.dstmac, session_info->ingress.lookup.LUT1.u.fields.L2.dstmac, 6 );
    memcpy(session_info->egress.srcmac, session_info->ingress.lookup.LUT1.u.fields.L2.srcmac, 6 );
    session_info->egress.eth_type = session_info->ingress.lookup.LUT1.u.fields.L2.eth_type;
    session_info->egress.enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_L2;

    /* L3 */
    if (AVALANCHE_PP_PID_TYPE_DOCSIS == session_info->ingress.pid_type)
    {
        if (AVALANCHE_PP_LUT_ENTRY_L3_IPV4 == session_info->ingress.lookup.LUT1.u.fields.L3.entry_type)
        {
            session_info->egress.DST_IP.v4 = session_info->ingress.lookup.LUT1.u.fields.L3.LAN_addr_IP.v4;
            session_info->egress.SRC_IP.v4 = session_info->ingress.lookup.LUT2.u.fields.WAN_addr_IP.v4;
        }
        else
        {
            memcpy(session_info->egress.DST_IP.v6, session_info->ingress.lookup.LUT1.u.fields.L3.LAN_addr_IP.v6, 16 );
            memcpy(session_info->egress.SRC_IP.v6, session_info->ingress.lookup.LUT2.u.fields.WAN_addr_IP.v6, 16 );
        }
    } 
    else
    {
        if (AVALANCHE_PP_LUT_ENTRY_L3_IPV4 == session_info->ingress.lookup.LUT1.u.fields.L3.entry_type)
        {
            session_info->egress.l3_packet_type = AVALANCHE_PP_LUT_ENTRY_L3_IPV4; 
            session_info->egress.DST_IP.v4 = session_info->ingress.lookup.LUT2.u.fields.WAN_addr_IP.v4;
            session_info->egress.SRC_IP.v4 = session_info->ingress.lookup.LUT1.u.fields.L3.LAN_addr_IP.v4;
        }
        else
        {
            session_info->egress.l3_packet_type = AVALANCHE_PP_LUT_ENTRY_L3_IPV6;
            memcpy(session_info->egress.DST_IP.v6, session_info->ingress.lookup.LUT2.u.fields.WAN_addr_IP.v6, 16 );
            memcpy(session_info->egress.SRC_IP.v6, session_info->ingress.lookup.LUT1.u.fields.L3.LAN_addr_IP.v6, 16 );
        }
    }
    session_info->egress.TOS = session_info->ingress.lookup.LUT2.u.fields.TOS;
    session_info->egress.enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_IP;

    /* L4 */
    session_info->egress.ip_protocol = session_info->ingress.lookup.LUT1.u.fields.L3.ip_protocol;
    session_info->egress.L4_DstPort = session_info->ingress.lookup.LUT2.u.fields.L4_DstPort;
    session_info->egress.L4_SrcPort = session_info->ingress.lookup.LUT2.u.fields.L4_SrcPort;
    session_info->egress.enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_L4;
}
/**************************************************************************
 * FUNCTION NAME : hil_null_hook
 **************************************************************************
 * DESCRIPTION:
 *  This function creates session to terminating VPID. All the packets like ingress one will be routed to it and dropped in the future.
 *  The function is very similar to hil_egress_hook.
 * 
 * RETURNS:
 *  Always returns 0.
 **************************************************************************/
Int32 hil_null_hook(struct sk_buff* skb, Int32 null_vpid)
{
    AVALANCHE_PP_RET_e rc;
    AVALANCHE_PP_SESSION_INFO_t* session_info = &skb->pp_packet_info.pp_session;
    struct net_device* input_dev;

    if (global_hil_db.hil_disabled || (!avalanche_pp_state_is_active()) || (null_vpid == -1))
    {
        return 0;
    }

    if ((skb->pp_packet_info.flags & TI_HIL_PACKET_FLAG_PP_SESSION_INGRESS_RECORDED) == 0)
    {
        /* If the Packet has not HIT the ingress hook there is no point in creating the session since the packet is locally generated */
        global_hil_db.num_other_pkts++;
        return 0;
    }

    if  (skb->pp_packet_info.flags & TI_HIL_PACKET_FLAG_PP_SESSION_BYPASS)
    {
        /* The Host Intelligence layers have decided not to "acclerate" this session */
        global_hil_db.num_bypassed_pkts++;
        return 0;
    }

    global_hil_db.num_null_drop_pkts++;

    input_dev = dev_get_by_index(&init_net,skb->skb_iif);
    if(input_dev == NULL)
    {
         return 0;
    }

    /* Need to copy the ingress property to egress property instead of the extract egress
     * we don't really need egress property to create null session but we do that copy just 
     * to be on the safe side and if ever someone will adding some egress test as condition 
     * to create the session */
    hil_copy_ingress_to_egress(session_info);

    /* We can only proceed to the next step if all the information has been extracted */
    if (input_dev->pid_handle == -1)
    {
        dev_put(input_dev);
        return 0;
    }
    dev_put(input_dev);

    /* Override the VPID */
    session_info->egress.vpid_handle = null_vpid;

    /* Check if the session is ROUTABLE or not by comparing the L2 destination MAC Address at the Ingress and Egress.
     * If they are not the same it is safe to assume that the session was routed */
    session_info->is_routable_session = 0; /* Default to bridged */
    
    session_info->egress.drop_sess = AVALANCHE_PP_EGRESS_DROP_SESS;
    /* Once all the fields have been extracted. Check if the session can be created or not? */
    if (hil_session_intelligence(session_info) == 0)
    {
        return 0;
    }

    if( hil_choose_session_timeout(session_info) < 0 )
    {
        return 0;
    }

    /* For Ping sessions, minimum timeout is 2 SEC to prevent timeout of 1 sec which is the default for ping pps */
    if ((session_info->ingress.lookup.LUT1.u.fields.L3.ip_protocol == IPPROTO_ICMP) || (session_info->ingress.lookup.LUT1.u.fields.L3.ip_protocol == IPPROTO_ICMPV6))
    {
        if (pp_session_timeout_sec == DEFAULT_SESSION_TIMEOUT_SEC)
        {
            session_info->session_timeout = DEFAULT_ICMP_SESSION_TIMEOUT_SEC * 1000000;
        }
    }

    session_info->priority = 0;
    session_info->cluster  = 0;

    session_info->session_pool = AVALANCHE_PP_SESSIONS_POOL_DATA;

    /* Check if ddh allows us to open a new drop session */
    if(!__hil_ddh_process(skb))
    {
        return 0;
    }

    rc = avalanche_pp_session_create(session_info, (void*)skb);
    if ((rc != PP_RC_SUCCESS) && (rc != PP_RC_OBJECT_EXIST))
    {
        global_hil_db.num_error++;
        return -1;
    }

    return 0;
}
/**************************************************************************
 * FUNCTION NAME : hil_check_ip_protocol
 **************************************************************************
 * DESCRIPTION   :
 *  check if IP Protocol is supported by PP layer And update Hil counters
 *
 * RETURNS       :
 *  0 for success
 **************************************************************************/
static Int32 hil_check_ip_protocol(Uint16 ip_protocol, Bool ingress_pkt, hil_pp_ip_prot_e ExtTunnel, Bool updateCounter)
{
    hil_pp_ip_prot_e ipProtCounter = PP_IP_PROT_NA;

    /* Check if IP protocol is supported */
    switch (ip_protocol)
    {
    case IPPROTO_TCP:
        /* Tcp is supported, update the tcp pkts counter */
        ipProtCounter = PP_COUNTER_TCP;
        break;

    case IPPROTO_UDP:
        /* Udp is supported, update the tcp pkts counter */
        ipProtCounter = PP_COUNTER_UDP;
        break;

    case IPPROTO_ICMP:
    case IPPROTO_ICMPV6:
        /* Icmp/Icmpv6 is supported */
        return 0;

    case IPPROTO_AH:
        /* Authentication Header in IPSEC is supported, update the tcp pkts counter */
        ipProtCounter = PP_COUNTER_IPSEC;
        break;

    case IPPROTO_ESP:
        /*  Encapsulating Security Payload  in IPSEC is supported, update the tcp pkts counter */
        ipProtCounter = PP_COUNTER_IPSEC;
        break;

    case IPPROTO_GRE:
        /* For  - dont update the counter*/
        if (ExtTunnel == PP_IP_PROT_NA)
        {
            return 0;
        } 
        else
        {
            if (ExtTunnel != PP_IP_PROT_GRE)
            {
                return -1;
            } 
            else
            {
                ipProtCounter = PP_COUNTER_INNER_GRE;
            }
        }
        break;

    case IPPROTO_IPIP:
        /* Ds Lite can't be an inner IP Protocol in Multiple Tunneled packets */
        if (ExtTunnel != PP_IP_PROT_NA)
        {
            return -1;
        } 
        else
        {
            return 0;
        }
        break;

    default:
        if (g_conf_ip_prot_valid)
        {
            if (ip_protocol == g_conf_ip_protocol)
            {
                return 0;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
        break;
    }

    if (updateCounter)
    {
        /* Calculate the conter that will be updated. */
        ipProtCounter += ExtTunnel;
        
        /* Update the ingress or egress counters */
        if (ingress_pkt)
        {
            global_hil_db.ingress_counters_ipProt_pkts[ipProtCounter]++;
        } 
        else
        {
            global_hil_db.egress_counters_ipProt_pkts[ipProtCounter]++;
        }
    }

    return 0;

}

/**************************************************************************
 * FUNCTION NAME : hil_get_layers_pointers
 **************************************************************************
 * DESCRIPTION   :
 *  extracts HIL relevant fields from the L2 header
 *
 * RETURNS       :
 *  0 for success
 **************************************************************************/
static Int32 hil_get_layers_pointers(Int8* ptr_data, struct ethhdr** ptr_ethhdr, struct pppoe_hdr** ptr_pppoe, struct iphdr** ptr_iphdr, struct ipv6hdr** ptr_ipv6hdr)
{
    Uint16 protocol_type;

    *ptr_ethhdr = (struct ethhdr *)ptr_data;        /* Get the pointer to the Ethernet header */
    ptr_data = ptr_data + sizeof(struct ethhdr);    /* Skip the Ethernet header */
    protocol_type = (*ptr_ethhdr)->h_proto;         /* Get the protocol type */

    /* Check the protocol field. If the protocol is VLAN or not? */
    if (protocol_type == __constant_htons(ETH_P_8021Q))
    {
        protocol_type = ((struct vlan_hdr *)ptr_data)->h_vlan_encapsulated_proto;   /* The new protocol is encapsulated into the VLAN header */
        ptr_data += sizeof(struct vlan_hdr);                                        /* Skip the 4 bytes of the VLAN header */
    }

    /* We have skipped the layer2 information; so try and get the layer3 information. */
    *ptr_ipv6hdr = NULL;
    *ptr_iphdr = NULL;
    *ptr_pppoe = NULL;

    switch (protocol_type)
    {
        case __constant_htons(ETH_P_IP):
        {
            /* Get the pointer to the IPv4 Header. */
            *ptr_iphdr = (struct iphdr *)ptr_data;
            break;
        }
        case __constant_htons(ETH_P_IPV6):
        {
            /* Get the pointer to the IPv6 Header. */
            *ptr_ipv6hdr = (struct ipv6hdr *)ptr_data;
            break;
        }
        case __constant_htons(ETH_P_PPP_SES):
        {
            /* PPP Packet: Skip the PPP header. */
            ptr_data = ptr_data + 6;

            /* Get the PPP Protocol Information*/
            protocol_type = *((Uint16 *) ptr_data);
            /* Add support for IPv6 */
            if ((protocol_type == __constant_htons(PPP_IP)) || (protocol_type == __constant_htons(PPP_IPV6)))
            {
                *ptr_pppoe = (struct pppoe_hdr*) (ptr_data - 6);
                if (protocol_type == __constant_htons(PPP_IP))
                {
                    /* IP Packet. */
                    *ptr_iphdr = (struct iphdr *)(ptr_data + 2);
                }
                else
                {
                    *ptr_ipv6hdr = (struct ipv6hdr *)(ptr_data + 2);
                }
                break;
            }

            /* Fall throught to default case */
        }
        default:
        {
            /* This is a default condition to handle any packet type which is not understood. These packets are currently not accelerated by the PP. Example ARP Packets etc. */
            return -1;
        }
    }

    return 0;
}


/**************************************************************************
 * FUNCTION NAME : hil_ipv4_checksum
 **************************************************************************
 * DESCRIPTION   :
 *  Utility function to calculate IPv4 checksum field
 *
 * RETURNS       :
 *  ... the checksum
 **************************************************************************/
static Uint16 hil_ipv4_checksum(Uint8 *ipv4Header, Uint16 ipv4HeaderLen)
{
    Uint32 checksum;
    Uint16 i;

    for (i = 0, checksum = 0; i < ipv4HeaderLen ; i += 2)
    {
        checksum += (Uint32)((ipv4Header[i] << 8) & 0xFF00) + (ipv4Header[i+1] & 0xFF);
    }

    while (checksum & 0xFFFF0000)
    {
        checksum = (checksum >> 16) + (checksum & 0xFFFF);
    }

    return ((Uint16)~checksum);
}


/**************************************************************************
 * FUNCTION NAME : hil_scan_ipv6
 **************************************************************************
 * DESCRIPTION   :
 *  Scan IPv6 header till reaching one of our supported protocols
 *
 * RETURNS       :
 *      fragOffs -
 *          value indicates frag. offset or -1 if not a fragment
 *      FragHdrOffs -
 *          offset of frag. header from start of the header. can be 0 if not required
 *      PrevFragHdrOffs -
 *          offset of next_header field which is previous to frag. header. Must be non zero if FragHdrOffs != 0
 *
 *  0 for success
 **************************************************************************/
static Int32 hil_scan_ipv6(struct ipv6hdr* ptr_ipv6hdr,
                                Uint8* ipv6HeaderLen,
                                Uint8* nexthdr,
                                Int16 * fragOffs,
                                Uint8 * FragHdrOffs,
                                Uint8 * PrevFragHdrOffs,
                                hil_pp_ip_prot_e ExtTunnel,
                                Bool  Ingress_pkt,
                                Bool updateCounter)
{
    struct ipv6_opt_hdr* hdr = NULL;
    Uint32 hdrlen;
    Uint32 nextOffset;

    *fragOffs = -1;
    *nexthdr = ptr_ipv6hdr->nexthdr;
    *ipv6HeaderLen = sizeof(struct ipv6hdr);
    Uint8 prev_hdr_offs = offsetof(struct ipv6hdr, nexthdr);

    if (FragHdrOffs)
    {
        *FragHdrOffs = 0;
        *PrevFragHdrOffs = 0;
    }

    /* Stop at one of the supported protocols */
    /* Iterate through well-known extenstion headers till we reach a supported protocol */
    while (hil_check_ip_protocol(*nexthdr, Ingress_pkt, ExtTunnel, updateCounter) != 0)
    {
        /* If this is the last next header */
        if (*nexthdr == NEXTHDR_NONE) 
        {
            return -1;
        }

        hdr = (struct ipv6_opt_hdr*)((Uint8 *)ptr_ipv6hdr + *ipv6HeaderLen);

        if (*nexthdr == NEXTHDR_FRAGMENT)
        {
            hdrlen = 8;
            /* In case fragmentation header contain  offset or MF */
            if (ntohs(((struct frag_hdr*) hdr)->frag_off) & ~0x6)
            {
                *fragOffs = ntohs(((struct frag_hdr*) hdr)->frag_off) & ~0x7;
                if (FragHdrOffs)
                {
                    *FragHdrOffs = ((void*) hdr) - ((void*) ptr_ipv6hdr);
                    *PrevFragHdrOffs = prev_hdr_offs;
                }
            }
        }
        else
        {
            hdrlen = ipv6_optlen(hdr);
        }

        nextOffset = (Uint32)*ipv6HeaderLen + hdrlen;

        /* preventing a wrap - if the offset exceeds 255 bytes we exit the loop. */
        if (nextOffset > 0xFF)
        {
            return -1;
        }
        else
        {
            prev_hdr_offs = *ipv6HeaderLen;
            *ipv6HeaderLen = (Uint8)nextOffset;
        }

        *nexthdr = hdr->nexthdr;
    }

    return 0;
}

#ifdef CONFIG_IP_MULTICAST
/**************************************************************************
 * FUNCTION NAME : hil_mfc_delete_session_by_mc_group
 **************************************************************************
 * DESCRIPTION   :
 *  The function deletes a session by a multicast group identifier
 **************************************************************************/
static Int32 hil_mfc_delete_session_by_mc_group(Int32 mc_group)
{
    Int32 session_handle ;
    AVALANCHE_PP_SESSION_INFO_t *session;

    /* Cycle through all the entries */
    for (session_handle = AVALANCHE_PP_MAX_ACCELERATED_SESSIONS-1; session_handle > -1; session_handle--)
    {
        /* Get the pointer to the Session Information */
        if (avalanche_pp_session_get_info(session_handle, &session) != PP_RC_SUCCESS)
        {
            continue;
        }

        if (session->ingress.lookup.LUT1.u.fields.L3.LAN_addr_IP.v4 == mc_group)
        {
            avalanche_pp_session_delete(session_handle, NULL);

            if (global_hil_db.tdox_dbg)
            {
                printk("TDOX-DBG DeleteSession: ses=%d\n", session_handle);
            }
            return 0;
        }
    }

    return -1;
}
#endif


/**************************************************************************
 * FUNCTION NAME : hil_tdox_check_and_enable
 **************************************************************************
 * DESCRIPTION   : This function sets the Tdox Enable Flag according to set of conditions
 * RETURNS:
 *  0 = OK, -1 = error.
 **************************************************************************/
static Int32 hil_tdox_check_and_enable(void* ptr_l4hdr, AVALANCHE_PP_SESSION_INFO_t* session_info)
{
    AVALANCHE_PP_INGRESS_SESSION_PROPERTY_t* ingress_property = &session_info->ingress;
    AVALANCHE_PP_EGRESS_SESSION_PROPERTY_t* egress_property = &session_info->egress;
    struct tcphdr *ptr_tcphdr = (struct tcphdr *)ptr_l4hdr;

    if (global_hil_db.tdox_disabled)
    {
        return 0;   /* TCP acceleration feature is disabled */
    }

    if (egress_property->pid_type != AVALANCHE_PP_PID_TYPE_DOCSIS)
    {
        return 0;   /* TCP acceleration is done only on traffic toward the RF */
    }

    if (ingress_property->lookup.LUT1.u.fields.L3.ip_protocol != IPPROTO_TCP)
    {
        return 0;   /* TCP acceleration is done only on TCP... */
    }

    if (session_info->egress.l3_packet_type == AVALANCHE_PP_LUT_ENTRY_L3_UNDEFINED)
    {
        return 0;   /* TCP acceleration is done only on supported L3 */
    }

    if (ptr_tcphdr->syn)
    {
        egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_TCP_SYN;
        return 0;  /* In case it's a SYN packet - do not open the session yet */
    }

    /* extract ack number */
    if (ptr_tcphdr->ack)
    {
        Int8 *ptr_data_head   = (Int8 *)ptr_tcphdr + ptr_tcphdr->doff * 4;
        Int8 *ptr_tcp_options = (Int8 *)ptr_tcphdr + sizeof(struct tcphdr);

        if (ptr_data_head > ptr_tcp_options)
        {
            /********************************************************/
            /* Go over the options and look for timestamp option    */
            /********************************************************/
            while (ptr_tcp_options < ptr_data_head)
            {
                Uint8 tcp_opt_type;
                Uint8 tcp_opt_len;

                tcp_opt_type = *(ptr_tcp_options);
                
                if (TCP_OPTION_CODE_EOL == tcp_opt_type )
                {
                    /*TCP_OPTION_CODE_EOL - This is used at the end of all options*/
                    break;
                }
                if (tcp_opt_type > TCP_OPTION_CODE_NOP)
                {
                    /************************************************/
                    /* T.L.V. styled option                         */
                    /************************************************/
                    tcp_opt_len =  *(ptr_tcp_options + 1);

                    if (TCP_OPTION_CODE_TIMESTAMP == tcp_opt_type)
                    {
                        egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_SKIP_TIMESTAMP;
                        break;
                    }

                    if(tcp_opt_len > 0)
                    {
                        ptr_tcp_options += tcp_opt_len;
                    }
                    else
                    {
                        return -1;  /* an illegal option length - don't accelerate this session. Probably the server will reset the connection */
                    }
                }
                else
                {
                    ptr_tcp_options++;  /* Options EOL and NOP are without parameters */
                }
            }
        }

        egress_property->tdox_tcp_ack_number = ptr_tcphdr->ack_seq;
        egress_property->enable |= AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED;
    }
    else
    {
        return -1;  /* TCP with NO ACK flag set - do not accelerate */
    }

    return 0;
}

/**************************************************************************
 * FUNCTION NAME : hil_check_and_update_TdoxEvalTime
 **************************************************************************
 * DESCRIPTION   : This function sets the Tdox Timer Evaluation according to the number of active TCP US sessions
 *                 unless if the default Timer is enabled
 **************************************************************************/
void hil_check_and_update_TdoxEvalTime(void)
{
    Uint32 num_entries;

    /* If default is enable, Set the the Evaluation Timer as the defult, and not according to the active sessions */
    if (global_hil_db.tdox_timer_default)
    {
        global_hil_db.tdoxEvalTimeMsec = DEFAULT_TDOX_TIMEOUT_MSEC;

        if (global_hil_db.tdox_dbg)
        {
            printk("TDOX-DBG Update Eval Timer: default Timer is set to %d \n", global_hil_db.tdoxEvalTimeMsec);
        }
        return;
    }

    if (docsis_vpid_handle < 0) 
    {
        global_hil_db.tdoxEvalTimeMsec = DEFAULT_TDOX_TIMEOUT_MSEC;

        if (global_hil_db.tdox_dbg)
        {
            printk("TDOX-DBG docsis_vpid_handle isn't set Default timeout was set \n"); 
        }
        return;
    }

    /* Get active session from egress TCP list */
    if (avalanche_pp_session_get_list( docsis_vpid_handle, PP_LIST_ID_EGRESS_TCP, &num_entries, NULL) != 0 )
    {
        global_hil_db.tdoxEvalTimeMsec = DEFAULT_TDOX_TIMEOUT_MSEC;
        return;
    }

    
    if (num_entries <= global_hil_db.tdoxEvalMinSessions) 
    {
        global_hil_db.tdoxEvalTimeMsec = global_hil_db.tdoxEvalMinSessionsTimer;
    }
    else if (num_entries <= global_hil_db.tdoxEvalMaxSessions) 
    {
        global_hil_db.tdoxEvalTimeMsec = global_hil_db.tdoxEvalMidSessionsTimer;  
    }
    else
    {
        global_hil_db.tdoxEvalTimeMsec = global_hil_db.tdoxEvalMaxSessionsTimer;
    }

    if (global_hil_db.tdox_dbg)
    {
       printk("TDOX-DBG Update Eval Timer: active sessions [%d], Timer is set to %d\n", num_entries, global_hil_db.tdoxEvalTimeMsec);
    }

}

/**************************************************************************
 * FUNCTION NAME : hil_tdox_manager
 **************************************************************************
 * DESCRIPTION: This function is the TDOX manager - making decisions if a session is applicabale for TDOX or not
 * RETURNS:
 *  AVALANCHE_PP_RET_e
 **************************************************************************/
AVALANCHE_PP_RET_e hil_tdox_manager(AVALANCHE_PP_SESSION_INFO_t *session_info, Ptr data)
{
    AVALANCHE_PP_SESSION_STATS_t sessionStats;
    AVALANCHE_PP_RET_e rc;
    Uint32  currentTime;
    Uint32  deltaTimeMsec;
    Uint32  pktDelta;
    Uint32  bytesDelta;
    Uint32  averagePktSize;
    Uint32  packetsPerSecond;
    Uint32  tdoxEvalAvgPktSizeThresh;
    Bool    tdoxEnabled;

    AVALANCHE_PP_SESSION_TDOX_STATS_t * tdox_stats;
    
    if (session_info->egress.enable & AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED)
    {
        if ( avalanche_pp_session_tdox_capability_get( session_info->session_handle, &tdoxEnabled ) )
        {
            return (PP_RC_FAILURE);
        }
        
        if (False == tdoxEnabled)
        {
            /* This is TDOX session that has been deprecated by the PP FW. The associated TDOX ID need to be freed */
            if ( avalanche_pp_session_tdox_capability_set( session_info->session_handle, tdoxEnabled ) )
            {
               return (PP_RC_FAILURE);
            }
            return PP_RC_SUCCESS;
        }
    }

    if ((rc = (avalanche_pp_get_stats_session(session_info->session_handle, &sessionStats))) != PP_RC_SUCCESS)
    {
        return rc;
    }

    tdox_stats = &session_info->tdox_stats;

    currentTime = jiffies;
    deltaTimeMsec = ((currentTime - tdox_stats->last_update_time) * 1000) / HZ;  /* Scale down the delta to milliseconds */

    if ((tdox_stats->last_update_time != 0) && (deltaTimeMsec < global_hil_db.tdoxEvalTimeMsec))
    {
        /* Resolution for checking the traffic is at least tdoxEvalMinTime */
        return PP_RC_SUCCESS;
    }

    pktDelta = sessionStats.packets_forwarded - tdox_stats->packets_forwarded;
    bytesDelta = sessionStats.bytes_forwarded - tdox_stats->bytes_forwarded;
    tdox_stats->packets_forwarded = sessionStats.packets_forwarded;
    tdox_stats->bytes_forwarded = sessionStats.bytes_forwarded;

    if (tdox_stats->last_update_time == 0)
    {
        /* If this is the first time, just update current time and wait to next iteration */
        if (global_hil_db.tdox_dbg)
        {
            printk("TDOX-DBG Update: ses=%d, TDOX=%s, deltaTimeMsec=%d, pktDelta=%d, bytesDelta=%d\n", 
                   session_info->session_handle, ((session_info->egress.enable & AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED) ? "EN" : "DIS"), 
                   0, pktDelta, bytesDelta);
        }
        
        tdox_stats->last_update_time = currentTime;
        return PP_RC_SUCCESS;
    }

    tdox_stats->last_update_time = currentTime;

    if (sessionStats.packets_forwarded < 100)
    {
        return (PP_RC_SUCCESS);   /* Do not do any logic on the session if it is only established */
    }

    averagePktSize = pktDelta ? (bytesDelta / pktDelta) : 0;
    if (pktDelta < 0xFFFFFFFF / 1000)
    {
        packetsPerSecond = (pktDelta * 1000) / deltaTimeMsec;
    }
    else
    {
        packetsPerSecond = (pktDelta / deltaTimeMsec) * 1000;
    }

    if (global_hil_db.tdox_dbg)
    {
        printk("TDOX-DBG Update: ses=%d, TDOX=%s, tdox=%d, deltaTimeMsec=%d, pktDelta=%d, bytesDelta=%d, PPS=%d, AvgPktSize=%d\n", 
               session_info->session_handle, ((session_info->egress.enable & AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED) ? "EN" : "DIS"), 
               session_info->egress.tdox_handle, deltaTimeMsec, pktDelta, bytesDelta, packetsPerSecond, averagePktSize);
    }

    /* Set the tdox avrage packet size according to the session */ 
    if (session_info->ingress.isTunnel && session_info->egress.isTunnel)
    {
       /* If Session is tunnel by pass - the threshold is bigger since more layers headers are part of the pkt  */
       tdoxEvalAvgPktSizeThresh = global_hil_db.tdoxEvalAvgBPTunnelPktSizeThresh;
    }
    else
    {
      tdoxEvalAvgPktSizeThresh = global_hil_db.tdoxEvalAvgPktSizeThresh;
    }

    if (session_info->egress.enable & AVALANCHE_PP_EGRESS_FIELD_ENABLE_TDOX_ENABLED)
    {
        /* This is a TDOX session - check for conditions to disable TDOX on this session */
        if ((packetsPerSecond < global_hil_db.tdoxEvalPpsThresh) || (averagePktSize > tdoxEvalAvgPktSizeThresh))
        {
            if (global_hil_db.tdox_dbg)
            {
                printk("TDOX-DBG FreeTdox (Evaluation): ses=%d, tdox=%d, PPS=%d, PktSize=%d\n", session_info->session_handle, session_info->egress.tdox_handle, packetsPerSecond, averagePktSize);
            }

            if ( avalanche_pp_session_tdox_capability_set( session_info->session_handle, False ) )
            {
                return (PP_RC_FAILURE);
            }
        }
    }
    else
    {
        /* This is not a TDOX session - check for conditions to enable TDOX on this session */
        if ((packetsPerSecond >= global_hil_db.tdoxEvalPpsThresh) && (averagePktSize <= tdoxEvalAvgPktSizeThresh))
        {
            if ( avalanche_pp_session_tdox_capability_set( session_info->session_handle, True ) )
            {
                return (PP_RC_FAILURE);
            }
            if (global_hil_db.tdox_dbg)
            {
                printk("TDOX-DBG AssociateTdox (Threshold): ses=%d, tdox=%d\n", session_info->session_handle, session_info->egress.tdox_handle);
            }
        }
    }

    return PP_RC_SUCCESS;
}


/**************************************************************************
 * FUNCTION NAME : hil_write_cmds
 **************************************************************************
 * DESCRIPTION   :
 *  Interface for the Intrusive HIL. This is used to debug and display various
 *  packet processor entity information from the console.
 *
 * RETURNS       :
 *  -1              - Error.
 *  Non-Zero        - Success.
 ***************************************************************************/
static int hil_write_cmds(struct file *file, const char *buffer, unsigned long count, void *data)
{
    Int8*   pp_cmd;
    Int8*   argv[10];
    Int32   argc = 0;
    Int8*   ptr_cmd;
    Int8*   delimitters = " \n\t";
    char*   ptr_next_tok;

    Uint8   **testPktP = NULL;
    Uint16  *testPktSizeP = NULL;

    pp_cmd = kmalloc(count+1, GFP_KERNEL);
    if (pp_cmd == NULL)
    {
        printk("Could not allocate %lu bytes for pp_cmd\n", count);
        return -ENOMEM;
    }

    /* Initialize the buffer before using it. count+1 in order to put null at the end of the buffer after it will be copy from user*/
    memset ((void *)&pp_cmd[0], 0, count+1);
    memset ((void *)&argv[0], 0, sizeof(argv));

    /* Copy from user space. */
    if (copy_from_user (pp_cmd, buffer, count))
    {
        kfree(pp_cmd);
        return -EFAULT;
    }

    if (pp_cmd[0] == 0)
    {
         kfree(pp_cmd);
         return -EINVAL;
    }

    /* Extract the first command. */
    ptr_next_tok = pp_cmd;

    /* Parse all the commands typed. */
    while (1)
    {
        ptr_cmd = strsep(&ptr_next_tok, delimitters);
        if (ptr_cmd == NULL)
        {
            /* 'strsep' returns null if there was no tok when it gets to '\0' */
            break;
        }
        argv[argc++] = ptr_cmd;

        if (ptr_next_tok == NULL)
        {
            /* no next tok*/
            break;
        }
        /* Validate if the user entered more commands.*/
        if (argc >=10)
        {
            printk ("ERROR: Incorrect too many parameters dropping the command\n");
            kfree(pp_cmd);
            return -EFAULT;
        }
    }


    /******************************* Command Handlers *******************************/
    /* Display Command Handlers */
    if (strncmp(argv[0], "show", strlen("show")) == 0)
    {
        /* Call the Show Command Handler. */
        if (hil_show_cmd_handler(argc, argv) < 0)
        {
            kfree(pp_cmd);
            return -EFAULT;
        }
    }

    /* Set Command Handlers */
    else if (strncmp(argv[0], "set", strlen("set")) == 0)
    {
        /* Call the Set Command Handler. */
        if (hil_set_cmd_handler (argc, argv) < 0)
        {
            kfree(pp_cmd);
            return -EFAULT;
        }
    }

    /* Deinitialize Command Handlers */
    else if (strncmp(argv[0], "reset", strlen("reset")) == 0)
    {
        /* Call the Reset Command Handler. */
        if (hil_reset_cmd_handler (argc, argv) < 0)
        {
            kfree(pp_cmd);
            return -EFAULT;
        }
    }

    /* cable_pp: disable/enable capability */
    else if (strcmp(argv[0], "enable") == 0)
    {
        global_hil_db.hil_disabled = False;
    }

    else if (strcmp(argv[0], "disable") == 0)
    {
        global_hil_db.hil_disabled = True;
        avalanche_pp_flush_scb_db();
        global_hil_db.ddh_active_short_connections = 0;
        global_hil_db.ddh_zombie_short_connections = 0;
        SCBPRINTK("**** Flush SCB DB ****");
        avalanche_pp_flush_sessions( AVALANCHE_PP_MAX_VPID, PP_LIST_ID_ALL );
    }

    else if (strcmp(argv[0], "tdox") == 0)
    {
        global_hil_db.tdox_disabled = False;
    }

    else if (strcmp(argv[0], "notdox") == 0)
    {
        global_hil_db.tdox_disabled = True;
    }

    else if (strcmp(argv[0], "ackSupp") == 0)
    {
        Int8 pram = *(argv[1]);
        Int32 enDis = 0;
        if(pram == '1')
        {
            enDis = 1;
        }
        // TBD
        // avalanche_pp_set_ack_suppression(enDis);
    }

    else if ((strcmp(argv[0], "qos") == 0) && (global_hil_db.qos_disabled) )
    {
        Uint32 i;
        Uint32 lockKey = 0;

        PAL_osProtectEntry(PAL_OSPROTECT_INTERRUPT, &lockKey);

        // go over all devices and setup QoS
        for( i = 0; i < TI_MAX_DEVICE_INDEX; i++ )
        {
            struct net_device *cur_dev = 0;
            cur_dev = dev_get_by_index(&init_net, i);
            if( !cur_dev )
                continue;

            if (-1 == cur_dev->vpid_handle)
            {
                dev_put(cur_dev);
                continue;
            }

            if (NULL != cur_dev->qos_setup_hook)
            {
                cur_dev->qos_setup_hook( cur_dev );
            }
            dev_put(cur_dev);
        }
        global_hil_db.qos_disabled = False;  // Update qos_disabled flag
        avalanche_pp_flush_sessions( AVALANCHE_PP_MAX_VPID, PP_LIST_ID_ALL );

        PAL_osProtectExit(PAL_OSPROTECT_INTERRUPT, lockKey);
    }

    else if ((strcmp(argv[0], "noqos") == 0) && (!global_hil_db.qos_disabled) )
    {
        Uint32 i;
        Uint32 lockKey = 0;

        PAL_osProtectEntry(PAL_OSPROTECT_INTERRUPT, &lockKey);

        // go over all devices and shutdown QoS
        for( i = 0; i < TI_MAX_DEVICE_INDEX; i++ )
        {
            struct net_device *cur_dev = 0;
            cur_dev = dev_get_by_index(&init_net, i);
            if( !cur_dev )
                continue;

            if (-1 == cur_dev->vpid_handle)
            {
                dev_put(cur_dev);
                continue;
            }

            if (NULL != cur_dev->qos_shutdown_hook)
            {
                cur_dev->qos_shutdown_hook( cur_dev );
            }
            dev_put(cur_dev);
        }
        global_hil_db.qos_disabled = True;  // Update qos_disabled flag
        avalanche_pp_flush_sessions( AVALANCHE_PP_MAX_VPID, PP_LIST_ID_ALL );

        PAL_osProtectExit(PAL_OSPROTECT_INTERRUPT, lockKey);
    }
    else if (strcmp(argv[0], "flush_all_sessions") == 0)
    {
        avalanche_pp_flush_sessions( AVALANCHE_PP_MAX_VPID, PP_LIST_ID_ALL );
    }

    else if (strcmp(argv[0], "createSession") == 0)
    {
        hil_test_session_creation();
    }

    else if (strcmp(argv[0], "printUsPktDataIngress") == 0)
    {
        testPktP = &usPktDataIngressP;
        testPktSizeP = &usPktDataIngressSize;
    }

    else if (strcmp(argv[0], "printUsPktDataEgress") == 0)
    {
        testPktP = &usPktDataEgressP;
        testPktSizeP = &usPktDataEgressSize;
    }

    else if (strcmp(argv[0], "printDsPktDataIngress") == 0)
    {
        testPktP = &dsPktDataIngressP;
        testPktSizeP = &dsPktDataIngressSize;
    }

    else if (strcmp(argv[0], "printDsPktDataEgress") == 0)
    {
        testPktP = &dsPktDataEgressP;
        testPktSizeP = &dsPktDataEgressSize;
    }

    else if (strcmp(argv[0], "pushDsPkt") == 0) 
    {
        pushPacketToPrefetcher(0);
    }

    else if (strcmp(argv[0], "pushUsPkt") == 0) 
    {
        pushPacketToPrefetcher(1);
    }

    if (testPktSizeP != NULL)
    {
        Uint32 i;

        if (*testPktP)
        {
            Bool backspaceNeeded = False;
            for (i = 0; i < *testPktSizeP; i++)
            {
                backspaceNeeded = True;
                printk("%02X:", (*testPktP)[i]);
                if ((i & 0x7) == 0x7)
                {
                    printk("\b \b\n");
                    backspaceNeeded = False;
                }
            }
            if (backspaceNeeded)
            {
                printk("\b \b");
            }
            printk("\n");
        }
        else
        {
            printk("Was not set!!!\n");
        }
    }

    kfree(pp_cmd);
    return count;
}

/**************************************************************************
 * FUNCTION NAME : hil_show_cmd_handler
 **************************************************************************
 * DESCRIPTION   :
 *  This function is the Packet Processor show command handler.
 *
 * RETURNS       :
 *  -1      - Error.
 *  0       - Success.
 ***************************************************************************/
static Int32 hil_show_cmd_handler(Int32 argc, Int8* argv[])
{
    /* Validate the number of arguments that have been passed */
    if (argc != 2 && argc != 3 && argc != 4)
    {
        printk ("ERROR: Incorrect Number of parameters passed.\n");
        return -1;
    }

    /* Check if Analysis Information needs to be displayed? */
    if (strcmp(argv[1], "stats") == 0)
    {
        AVALANCHE_PP_Misc_Statistics_t stats;
        Uint32 index;

        printk ("HIL  State is                               : %s\n", (global_hil_db.hil_disabled)? "Disabled":"Enabled" );
        printk ("PP   State is                               : %s\n", (avalanche_pp_state_is_active()) ? "Active" : ((avalanche_pp_state_is_psm()) ? "Power Save" : "Inactive"));
        printk ("TDOX State is                               : %s\n", (global_hil_db.tdox_disabled)? "Disabled":"Enabled" );
        printk ("DiscardOOO State is                         : %s\n", (global_hil_db.discard_OOO)? "Enabled":"Disabled" );
        printk ("QoS  State is                               : %s\n", (global_hil_db.qos_disabled)? "Disabled":"Enabled" );
        printk ("Bypassed pkts                               : %u\n", global_hil_db.num_bypassed_pkts );
        printk ("Other    pkts                               : %u\n", global_hil_db.num_other_pkts);
        printk ("Fail Egress extract pkts                    : %u\n", global_hil_db.num_egress_pkt_fail_extract);
        printk ("Fail Egress Tcp SYN pkts                    : %u\n", global_hil_db.num_egress_pkt_tcp_syn); 
        printk ("Fail Egress SCB pkts                        : %u\n", global_hil_db.num_egress_pkt_scb); 
        printk ("Fail Egress GRE bypass                      : %u\n", global_hil_db.num_egress_pkt_gre_bypassed); 
        printk ("Fail Egress session inteligance pkts        : %u\n", global_hil_db.num_egress_pkt_fail_sess_int);

        if (g_conf_ip_prot_valid)
        {
            printk("Configured IP Protocol                      : %u\n", g_conf_ip_protocol);
        }
        
        printk ("\n|                   |  Ingress   |   Egress   |\n");
        printk ("|-------------------|------------|------------|\n");
        printk ("| pkts              | %10u | %10u |\n", global_hil_db.num_ingress_pkts, global_hil_db.num_egress_pkts);
        printk ("| udp pkts          | %10u | %10u |\n", global_hil_db.ingress_counters_ipProt_pkts[udp_pkts], global_hil_db.egress_counters_ipProt_pkts[udp_pkts]);
        printk ("| gre udp pkts      | %10u | %10u |\n", global_hil_db.ingress_counters_ipProt_pkts[gre_udp_pkts], global_hil_db.egress_counters_ipProt_pkts[gre_udp_pkts]);
        printk ("| dslite udp pkts   | %10u | %10u |\n", global_hil_db.ingress_counters_ipProt_pkts[dslite_udp_pkts], global_hil_db.egress_counters_ipProt_pkts[dslite_udp_pkts]);

        printk ("| tcp pkts          | %10u | %10u |\n", global_hil_db.ingress_counters_ipProt_pkts[tcp_pkts], global_hil_db.egress_counters_ipProt_pkts[tcp_pkts]);
        printk ("| gre tcp pkts      | %10u | %10u |\n", global_hil_db.ingress_counters_ipProt_pkts[gre_tcp_pkts], global_hil_db.egress_counters_ipProt_pkts[gre_tcp_pkts]);
        printk ("| dslite tcp pkts   | %10u | %10u |\n", global_hil_db.ingress_counters_ipProt_pkts[dslite_tcp_pkts], global_hil_db.egress_counters_ipProt_pkts[dslite_tcp_pkts]);

        printk ("| ipsec pkts        | %10u | %10u |\n", global_hil_db.ingress_counters_ipProt_pkts[ipsec_pkts], global_hil_db.egress_counters_ipProt_pkts[ipsec_pkts]);
        printk ("| gre ipsec pkts    | %10u | %10u |\n", global_hil_db.ingress_counters_ipProt_pkts[gre_ipsec_pkts], global_hil_db.egress_counters_ipProt_pkts[gre_ipsec_pkts]);
        printk ("| dslite ipsec pkts | %10u | %10u |\n", global_hil_db.ingress_counters_ipProt_pkts[dslite_ipsec_pkts], global_hil_db.egress_counters_ipProt_pkts[dslite_ipsec_pkts]);
        printk ("| gre gre pkts      | %10u | %10u |\n", global_hil_db.ingress_counters_ipProt_pkts[gre_gre_pkts], global_hil_db.egress_counters_ipProt_pkts[gre_gre_pkts]);
        printk ("|-------------------|------------|------------|\n\n");
 
        printk ("Null     pkts                               : %u\n", global_hil_db.num_null_drop_pkts );
        printk ("Total Sessions                              : %u\n", global_hil_db.num_total_sessions);
        printk ("Total Errors                                : %u\n", global_hil_db.num_error);
        printk ("Global Timeout                              : %u sec\n", pp_session_timeout_sec);
        printk ("Extended Timeout                            : %s\n", (global_hil_db.extended_session_timeout)? "Enabled": "Disabled" );
        printk ("Max Sessions For Extended Timeout           : %u \n", global_hil_db.extended_timeout_MaxSessions);
        printk ("Reserved LUT1 Keys for voice sessions       : %u\n", AVALANCHE_PP_MAX_ACCELERATED_VOICE_SESSIONS/2);
        printk ("Reserved voice sessions                     : %u\n", AVALANCHE_PP_MAX_ACCELERATED_VOICE_SESSIONS);

        avalanche_pp_get_db_stats(&stats);
        printk ("Active PIDs                                 : %u\n", stats.active_PIDs);
        printk ("Active VPIDs                                : %u\n", stats.active_VPIDs);
        printk ("Active LUT1 keys                            : %u\n", stats.active_lut1_keys);
        printk ("Max active LUT1 keys                        : %u\n", stats.max_active_lut1_keys);
        printk ("Active sessions                             : %u\n", stats.active_sessions);
        printk ("Active US sessions                          : %u\n", stats.active_us_sessions);
        printk ("Active drop sessions                        : %u\n", stats.active_drop_sessions);
		printk ("Active multi drop sessions                  : %u\n", stats.active_multi_drop_sessions);
        printk ("Max active sessions                         : %u\n", stats.max_active_sessions);

        printk ("|    LUT1 utilization          |       LUT2 utilization        |\n");
        printk ("|------------------------------|-------------------------------|\n");
        for (index = 0; index < AVALANCHE_PP_LUT_HISTOGRAM_SIZE; index++)
        {
            printk ("| range [%3d to %3d] : %7u | range [%4d to %4d] : %6u |\n", (AVALANCHE_PP_MAX_LUT1_KEYS/AVALANCHE_PP_LUT_HISTOGRAM_SIZE)*index, (AVALANCHE_PP_MAX_LUT1_KEYS/AVALANCHE_PP_LUT_HISTOGRAM_SIZE)*(index+1) -1, stats.lut1_histogram[index],
                                                                                 (AVALANCHE_PP_MAX_ACCELERATED_SESSIONS/AVALANCHE_PP_LUT_HISTOGRAM_SIZE)*index, (AVALANCHE_PP_MAX_ACCELERATED_SESSIONS/AVALANCHE_PP_LUT_HISTOGRAM_SIZE)*(index+1) -1, stats.lut2_histogram[index]);
        }
        printk ("| LUT1 starvation    : %7u | LUT2 starvation      : %6u |\n", stats.lut1_starvation, stats.lut2_starvation);
        printk ("|------------------------------|-------------------------------|\n");
                
        if (avalanche_pp_state_is_active()) 
        {
            Int32 index;
            AVALANCHE_PP_QOS_QUEUE_STATS_t qos_stats;

            printk ("\nQoS queues statistics:\n");
            printk (" queue | forwarded  | discarded  | owner\n");
            printk ("-------+------------+------------+--------------\n");
            for(index = 0; index <= (PAL_CPPI41_SR_QPDSP_QOS_Q_LAST - PAL_CPPI41_SR_QPDSP_QOS_Q_BASE); index++)
            {
                avalanche_pp_qos_get_queue_stats(index, &qos_stats);
                if (qos_stats.fwd_pkts | qos_stats.drp_cnt)
                {
                    printk (" %5u | %10u | %10u | %s\n",index, qos_stats.fwd_pkts, qos_stats.drp_cnt, PAL_CPPI41_GET_QNAME( index + PAL_CPPI41_SR_QPDSP_QOS_Q_BASE ));
                }
            }
            printk ("-------+------------+------------+--------------\n");
        }
        return 0;
    }

    if (strcmp(argv[1], "tdoxStats") == 0)
    {
        AVALANCHE_PP_Misc_Statistics_t  stats;
        avalanche_pp_get_db_stats(&stats);

        printk ("TDOX Default Timer State is                 : %s\n", (global_hil_db.tdox_timer_default)? "Enabled":"Disabled" );
        printk ("TDOX Evaluation Time                        : %u ms\n", global_hil_db.tdoxEvalTimeMsec);       
        printk ("TDOX Evaluation Timer of Max sessions       : %u ms\n", global_hil_db.tdoxEvalMaxSessionsTimer);
        printk ("TDOX Evaluation Timer of Mid sessions       : %u ms\n", global_hil_db.tdoxEvalMidSessionsTimer);
        printk ("TDOX Evaluation Timer of Min sessions       : %u ms\n", global_hil_db.tdoxEvalMinSessionsTimer);
        printk ("TDOX Evaluation Max Sessions                : %u \n", global_hil_db.tdoxEvalMaxSessions);
        printk ("TDOX Evaluation Min Sessions                : %u \n", global_hil_db.tdoxEvalMinSessions);
        printk ("TDOX Evaluation Average PktSize Threshold   : %u bytes\n", global_hil_db.tdoxEvalAvgPktSizeThresh);
        printk ("TDOX Evaluation Tunnel BP PktSize Threshold : %u bytes\n", global_hil_db.tdoxEvalAvgBPTunnelPktSizeThresh);
        printk ("TDOX Evaluation PPS Threshold               : %u PPS\n", global_hil_db.tdoxEvalPpsThresh);
        printk ("TDOX starvation                             : %u\n", stats.tdox_starvation);

        return 0;
    }

    if (strcmp(argv[1], "ddhStats") == 0)
    {
#ifdef DDH_DBG
        AVALANCHE_PP_Defensive_Mode_e   ddh_mode;
        avalanche_pp_get_defensive_mode(&ddh_mode);

        Bool l2_classification_enabled, l2_classification_pid, multi_drop_enabled, smart_prioritization_enabled;
        AVALANCHE_PP_BLACKLIST_INFO_e   blacklist_info;
        AVALANCHE_PP_DDH_STATE_e        state;

        avalanche_pp_get_l2_classification(&l2_classification_enabled, &l2_classification_pid);
        avalanche_pp_get_blacklist_info(&multi_drop_enabled, AVALANCHE_PP_LUT_ENTRY_L3_IPV4, &blacklist_info);
        avalanche_pp_get_smart_prioritization(&smart_prioritization_enabled);
        avalanche_pp_get_defensive_state(&state);

        printk ("Defensive Mode                              : %s\n", (ddh_mode == AVALANCHE_PP_DDH_MODE_INTERNAL) ? "Internal" : ((ddh_mode == AVALANCHE_PP_DDH_MODE_EXTERNAL) ? "External" : "Unknown"));
        printk ("Defensive State                             : %d\n", state);
        printk ("L2Classification                            : %s\n", (l2_classification_enabled) ? "Enabled" : "Disabled");
        printk ("MultiDrop                                   : %s\n", (multi_drop_enabled) ? "Enabled" : "Disabled");                                                                                                                    
        printk ("Smart Prioritization                        : %s\n", (smart_prioritization_enabled) ? "Enabled" : "Disabled");
        printk ("SCB Packet Threshold                        : %d\n", global_hil_db.ddh_num_scb_packets_threshold);
        printk ("SCB Connections Threshold                   : %d\n", global_hil_db.ddh_num_scb_connections_threshold);
        printk ("SCB Active Connections                      : %u\n", global_hil_db.ddh_active_short_connections);
        printk ("SCB Max Active Connections                  : %u\n", global_hil_db.ddh_max_active_short_connections);
        printk ("SCB Zombie Connections                      : %u\n", global_hil_db.ddh_zombie_short_connections);
        printk ("SCB Hash Collisions                         : %u\n", global_hil_db.ddh_num_scb_hash_collisions);
#endif

        return 0;
    }

#ifdef CONFIG_NETFILTER

    if (strcmp(argv[1], "mapper") == 0)
    {
        unsigned int    lockKey;
        Int16           i;
        Uint16          num_displayed_sessions = (Uint16)simple_strtol(argv[2], NULL, 0);

        /* Check if number of sessions is not 1-2048*/
        if ((num_displayed_sessions < 1 ) || (num_displayed_sessions > AVALANCHE_PP_MAX_ACCELERATED_SESSIONS ) )
        {    
            printk("Invalid argument\n"); 
            return 0;
        }

        PAL_osProtectEntry(PAL_OSPROTECT_INTERRUPT, &lockKey);
        printk("\n");
        printk("###############################\n");
        printk("# mapper info #\n");
        printk("###############################\n\n");
                
        
        for (i = (AVALANCHE_PP_MAX_ACCELERATED_SESSIONS - 1); (num_displayed_sessions != 0); i--, num_displayed_sessions--) 
        {
            printk("# session [%d] ",i);
            if (IS_TI_PP_SESSION_CT_INVALID(hil_session_ct_mapper[i].previous )) 
            {
                printk("# previous [invalid] ");
            }
            else
            {
                printk("# previous [%d] ",hil_session_ct_mapper[i].previous);
            }
            if (IS_TI_PP_SESSION_CT_INVALID(hil_session_ct_mapper[i].next)) 
            {
                printk("# next [invalid] ");
            }
            else
            {
                printk("# next [%d] ",hil_session_ct_mapper[i].next);
            }
            if (hil_session_ct_mapper[i].conntrack != NULL) 
            {
                if (hil_find_session_handle_in_ct_mapper_list(hil_session_ct_mapper[i].conntrack->tuplehash[IP_CT_DIR_REPLY].ti_pp_session_handle,i) == true)
                {
                    printk("# conntrack_session [%d] \n",hil_session_ct_mapper[i].conntrack->tuplehash[IP_CT_DIR_REPLY].ti_pp_session_handle);
                }
                else if (hil_find_session_handle_in_ct_mapper_list(hil_session_ct_mapper[i].conntrack->tuplehash[IP_CT_DIR_ORIGINAL].ti_pp_session_handle,i) == true)
                {
                    printk("# conntrack_session [%d] \n",hil_session_ct_mapper[i].conntrack->tuplehash[IP_CT_DIR_ORIGINAL].ti_pp_session_handle);
                }
                else 
                {
                    printk("\n");
                }
            }
            else
            {
                printk("conntrack is null\n");
            }
        }
        PAL_osProtectExit(PAL_OSPROTECT_INTERRUPT, lockKey);
        return 0;
     }
#endif

    /* Control comes here if the scope was not understood */
    return -1;
}

/**************************************************************************
 * FUNCTION NAME : hil_set_cmd_handler
 **************************************************************************
 * DESCRIPTION   :
 *  This function is the Packet Processor set command handler.
 *
 * RETURNS       :
 *  -1      - Error.
 *  0       - Success.
 ***************************************************************************/
static Int32 hil_set_cmd_handler(Int32 argc, Int8* argv[])
{
    Uint8   **testPktP = NULL;
    Uint16  *testPktSizeP = NULL;

    if (strcmp(argv[1], "timeout") == 0)
    {
        Int32 tmp = (Int32) simple_strtol(argv[2], NULL, 0);

        if (tmp < 0)
        {
            return -1;
        }

        pp_session_timeout_sec = tmp;

        return 0;
    }
    else if (strcmp(argv[1], "ipProtocol") == 0)
    {
        if (strcmp(argv[2], "unset") == 0)
        {
            g_conf_ip_prot_valid = False;
            g_conf_ip_protocol = 0;
        }
        else
        {
            Int32 tmp = (Int32) simple_strtol(argv[2], NULL, 0);

            if( (tmp < 0) || (tmp > 255))
            {
                return -1;
            }

            g_conf_ip_prot_valid = True;
            g_conf_ip_protocol = tmp;
        }

        return 0;
    }

    else if (strcmp(argv[1], "prioritization") == 0)
    {
        Uint8 tmp = (Uint8) simple_strtol(argv[2], NULL, 0);

        if (tmp > 3)
        {
            printk("Parameter is out of range. Allowed range: 0 to 3" );
            return -1;
        }
        avalanche_pp_set_traffic_prioritization_mode( tmp );
        return 0;
    }

    else if (strcmp(argv[1], "pingAcc") == 0)
    {
        Int32 enDis = (Int32)simple_strtol(argv[2], NULL, 0);

        if (enDis < 0 || enDis > 1)
        {
            return -1;
        }

        global_hil_db.ping_acceleration = enDis;

        return 0;
    }

    else if (strcmp(argv[1], "scbPacketTh") == 0)
    {
        global_hil_db.ddh_num_scb_packets_threshold = (Uint32) simple_strtol(argv[2], NULL, 0);

        return 0;
    }

    else if (strcmp(argv[1], "scbConnectionTh") == 0)
    {
        global_hil_db.ddh_num_scb_connections_threshold = (Uint32) simple_strtol(argv[2], NULL, 0);

        return 0;
    }

    else if (strcmp(argv[1], "tdoxDbg") == 0)
    {
        Int32 enDis = (Int32)simple_strtol(argv[2], NULL, 0);

        if (enDis < 0 || enDis > 1)
        {
            return -1;
        }

        global_hil_db.tdox_dbg = enDis;
        return 0;
    }
    else if (strcmp(argv[1], "tdoxDefaultTimer") == 0) 
    {
        Int32 enDis = (Int32)simple_strtol(argv[2], NULL, 0);
        if (enDis < 0 || enDis > 1)
        {
            return -1;
        }
        global_hil_db.tdox_timer_default = enDis;
        return 0;
    }
    else if (strcmp(argv[1], "ExtendedSessionTimeout") == 0)
    {
        Int32 enDis = (Int32)simple_strtol(argv[2], NULL, 0);
        if (enDis < 0 || enDis > 1)
        {
            return -1;
        }
        global_hil_db.extended_session_timeout = enDis;

        return 0;
    }
    else if (strcmp(argv[1], "tdoxDefaultTimer") == 0)
    {
        Int32 enDis = (Int32)simple_strtol(argv[2], NULL, 0);
        if (enDis < 0 || enDis > 1)
        {
            return -1;
        }
        global_hil_db.tdox_timer_default = enDis;
        return 0;
    }
    else if (strcmp(argv[1], "ExtendedSessionTimeout") == 0)
    {
        Int32 enDis = (Int32)simple_strtol(argv[2], NULL, 0);
        if (enDis < 0 || enDis > 1)
        {
            return -1;
        }
        global_hil_db.extended_session_timeout = enDis;

        return 0;
    }
    else if (strcmp(argv[1], "tdoxEvalAvgPktSize") == 0)
    {
        Int32 threshold = (Int32) simple_strtol(argv[2], NULL, 0);

        if (threshold < 0)
        {
            return -1;
        }

        global_hil_db.tdoxEvalAvgPktSizeThresh = threshold;

        return 0;
    }
    else if (strcmp(argv[1], "tdoxEvalAvgBPTunnelPktSize") == 0)
    {
        Int32 threshold = (Int32) simple_strtol(argv[2], NULL, 0);

        if (threshold < 0)
        {
            return -1;
        }

        global_hil_db.tdoxEvalAvgBPTunnelPktSizeThresh = threshold;

        return 0;

    }
    else if (strcmp(argv[1], "tdoxEvalPPS") == 0)
    {
        Int32 threshold = (Int32) simple_strtol(argv[2], NULL, 0);

        if (threshold < 0)
        {
            return -1;
        }

        global_hil_db.tdoxEvalPpsThresh = threshold;

        return 0;
    }

    else if (strcmp(argv[1], "tdoxEvalTime") == 0)
    {
        Int32 threshold = (Int32) simple_strtol(argv[2], NULL, 0);

        if (threshold < 0)
        {
            return -1;
        }

        global_hil_db.tdoxEvalTimeMsec = threshold;

        return 0;
    }
    else if (strcmp(argv[1], "tdoxEvalSession") == 0) 
    {
        Int32 threshold = (Int32) simple_strtol(argv[3], NULL, 0);

        if( threshold < 0 )
        {
            return -1;
        }
        if (threshold >= AVALANCHE_PP_MAX_ACCELERATED_SESSIONS)
        {
            threshold = AVALANCHE_PP_MAX_ACCELERATED_SESSIONS;
        }
        if (strcmp(argv[2], "Max") == 0)
        {
             global_hil_db.tdoxEvalMaxSessions = threshold;
        }
        else if (strcmp(argv[2], "Min") == 0)
        {
            global_hil_db.tdoxEvalMinSessions = threshold;
        }
        return 0;
    }
    else if (strcmp(argv[1], "tdoxEvalThresholds") == 0)
    {
        Int32 threshold = (Int32) simple_strtol(argv[3], NULL, 0);
        if (threshold < 0)
        {
            return -1;
        }
        if (strcmp(argv[2], "Max") == 0)
        {
             global_hil_db.tdoxEvalMaxSessionsTimer = threshold;
        }
        else if (strcmp(argv[2], "Mid") == 0)
        {
            global_hil_db.tdoxEvalMidSessionsTimer = threshold;
        }
        else if (strcmp(argv[2], "Min") == 0)
        {
            global_hil_db.tdoxEvalMinSessionsTimer = threshold;
        }
        return 0;
    }
    else if (strcmp(argv[1], "extendedTimeoutMaxSession") == 0)
    {
        Int32 threshold = (Int32) simple_strtol(argv[2], NULL, 0);
        if( threshold < 0 )
        {
            return -1;
        }
        if (threshold >= AVALANCHE_PP_MAX_ACCELERATED_SESSIONS)
        {
            threshold = AVALANCHE_PP_MAX_ACCELERATED_SESSIONS;
        }
        global_hil_db.extended_timeout_MaxSessions = threshold;
        return 0;
    }

    else if (strcmp(argv[1], "discardOOO") == 0)
    {
        Int32 enDis = (Int32)simple_strtol(argv[2], NULL, 0);

        if (enDis < 0 || enDis > 1)
        {
            return -1;
        }

        global_hil_db.discard_OOO = enDis;

        return 0;
    }

    else if (strcmp(argv[1], "ProxyTxWaitThr") == 0)
    {
        Uint16 waitBytes = (Uint16)simple_strtol(argv[2], NULL, 0);

        avalanche_pp_set_proxy_wait_byte_for_tx(waitBytes);
        return 0;
    }

    else if (strcmp(argv[1], "dropped_packets_bit_map") == 0)
    {
        Int32 cmd;
        Int32 val;
 
        /* Check for bug of last arg */ 
        if (!isalnum(*argv[argc-1]))
        {
            /* Ignore last */
            argc--;
        }

        if (argc == 2) 
        {
            /* Show */
            printk("dropped_packets_bit_map 0x%X\n", dropped_packets_bit_map);
            return 0;
        }
        else if (argc < 4)
        {
            /* Message. Silenty ignore argc > 4 due to bug(??) in caller when providing argc/argv */
            printk("ERROR: dropped_packets_bit_map 0x%X - %d parameters: argv[0] %s, argv[1] %s, argv[2] %s, argv[3] %s",
                   dropped_packets_bit_map, argc
                   ,argc > 0 ? argv[0] : NULL       /* Print agrv[i] if it exists */
                   ,argc > 1 ? argv[1] : NULL
                   ,argc > 2 ? argv[2] : NULL
                   ,argc > 3 ? argv[3] : NULL
                   );
            return -1;
        }

        cmd = (Int32) simple_strtol(argv[2], NULL, 0);
        val = (Int32) simple_strtol(argv[3], NULL, 0);
        if (cmd == 0)
        {
            /* Unset, remove appropriate bit */
            DROPPED_PACKETS_BITMAP_UNSET(val);
        }
        else
        {
            /* Set, set appropriate bit */
            DROPPED_PACKETS_BITMAP_SET(val);
        }

        return 0;
    }
 
    else if (strcmp(argv[1], "usPktDataIngress") == 0)
    {
        testPktP = &usPktDataIngressP;
        testPktSizeP = &usPktDataIngressSize;
    }
    else if (strcmp(argv[1], "usPktDataEgress") == 0)
    {
        testPktP = &usPktDataEgressP;
        testPktSizeP = &usPktDataEgressSize;
    }
    else if (strcmp(argv[1], "dsPktDataIngress") == 0)
    {
        testPktP = &dsPktDataIngressP;
        testPktSizeP = &dsPktDataIngressSize;
    }
    else if (strcmp(argv[1], "dsPktDataEgress") == 0)
    {
        testPktP = &dsPktDataEgressP;
        testPktSizeP = &dsPktDataEgressSize;
    }

    if (testPktSizeP != NULL)
    {
        Uint32 templateSize = 0;
        Int8 *endptr;

        if (argc != 4)
        {
            printk("Test commands format: set **PktData*gress PktString PktLength\n");
            return -EINVAL;
        }

        /* if testPktP is already allocated - make sure to free it before reallocating */
        if (*testPktP)
        {
            kfree(*testPktP);
        }

        /* Allocate *testPktP and reset it */
        *testPktSizeP = (Uint32)simple_strtol(argv[3], NULL, 0);
        *testPktP = kmalloc(*testPktSizeP, GFP_KERNEL);
        if (*testPktP == NULL)
        {
            printk("Could not allocate %d bytes for testPktP\n", *testPktSizeP);
            return -ENOMEM;
        }
        memset(*testPktP, 0, *testPktSizeP);

        /* Copy the packet */
        endptr = argv[2];
        while (true)
        {
            long val;
            val = (Int32)simple_strtol(endptr, (char**)&endptr, 16);
            if ((val < 0) || (val > 0xFF) || ((*endptr != ':') && (*endptr != '-') && (*endptr != '\0')))
            {
                printk("\nError found\n");
                kfree(*testPktP);
                return -EINVAL;
            }
            (*testPktP)[templateSize++] = (Int8)val;
            if (*endptr == '\0')
            {
                break;
            }
            if (templateSize > *testPktSizeP)
            {
                printk("Warning: packet size received is shorter than the template - truncating packet\n");
                break;
            }
            endptr++;
        }

        return 0;
    }

    /* Control comes here if the command was not understood. */
    return -EINVAL;
}

/**************************************************************************
 * FUNCTION NAME : hil_reset_cmd_handler
 **************************************************************************
 * DESCRIPTION   :
 *  This function is the Packet Processor reset command handler.
 *
 * RETURNS       :
 *  -1      - Error.
 *  0       - Success.
 ***************************************************************************/
static Int32 hil_reset_cmd_handler(Int32 argc, Int8* argv[])
{
    /* Validate the number of arguments that have been passed. */
    if (argc < 2)
    {
        printk ("ERROR: Incorrect Number of parameters passed.\n");
        return -1;
    }

    /* Reset the session timeout to default, i.e. */
    if (strcmp(argv[1], "timeout") == 0)
    {
        pp_session_timeout_sec = DEFAULT_SESSION_TIMEOUT_SEC;
        printk ("Session timeout returned to default of %d seconds.\n", DEFAULT_SESSION_TIMEOUT_SEC);

        /* Work is completed. */
        return 0;
    }

    /* Reset the HIL Analysis stats. */
    if (strcmp(argv[1], "stats") == 0)
    {
        /* Initialize the counters for the HIL Analysis. */
        global_hil_db.num_total_sessions = 0;
        global_hil_db.num_error = 0;
        global_hil_db.num_bypassed_pkts = 0;
        global_hil_db.num_other_pkts = 0;
        global_hil_db.num_ingress_pkts = 0;
        global_hil_db.num_egress_pkts  = 0;
        global_hil_db.num_null_drop_pkts = 0;

        int i;
        for (i = 0; i < max_pkts_counters; i++) 
        {
            global_hil_db.egress_counters_ipProt_pkts[i] = 0;
            global_hil_db.ingress_counters_ipProt_pkts[i] = 0;
        }

        avalanche_pp_reset_db_stats();

        return 0;
    }
    

    return -1;
}


static int hil_read_devs(char* buf, char **start, off_t offset, int count, int *eof, void *data)
{
    Uint32 i;
    Int32 len=0;
    Uint32 limit = count - 80;

    for(i = 0; i < TI_MAX_DEVICE_INDEX; i++ )
    {
         struct net_device *cur_dev = 0;
         AVALANCHE_PP_VPID_STATS_t vpid_stats;

         cur_dev = dev_get_by_index(&init_net, i);
         if( !cur_dev )
         {
              continue;
         }
         if (cur_dev->vpid_handle >= AVALANCHE_PP_MAX_VPID || cur_dev->vpid_handle < 0)
         {
             dev_put(cur_dev);
             continue;
         }

         /* Get the VPID statistics. */
         if( avalanche_pp_get_stats_vpid(cur_dev->vpid_handle , &vpid_stats) != PP_RC_SUCCESS )
         {
              dev_put(cur_dev);
              continue;
         }

         /* Print the statistics on the console */
         if( len < limit )
             len += sprintf(buf + len, "   /dev/%s: vpid=%d pid=%d \n", cur_dev->name, cur_dev->vpid_handle, cur_dev->pid_handle );
         if( len < limit )
             len += sprintf(buf + len, "-----------------------------------------\n");
         if( len < limit )
             len += sprintf(buf + len, "Rx Unicast   Packets: %u\n", vpid_stats.rx_unicast_pkt);
         if( len < limit )
             len += sprintf(buf + len, "Rx Broadcast Packets: %u\n", vpid_stats.rx_broadcast_pkt);
         if( len < limit )
             len += sprintf(buf + len, "Rx Multicast Packets: %u\n", vpid_stats.rx_multicast_pkt);
         if( len < limit )
             len += sprintf(buf + len, "Rx Bytes            : %llu\n", vpid_stats.rx_byte);
         if( len < limit )
             len += sprintf(buf + len, "Rx Discard          : %u\n", vpid_stats.rx_discard_pkt);
         if( len < limit )
             len += sprintf(buf + len, "Tx Unicast   Packets: %u\n", vpid_stats.tx_unicast_pkt);
         if( len < limit )
             len += sprintf(buf + len, "Tx Broadcast Packets: %u\n", vpid_stats.tx_broadcast_pkt);
         if( len < limit )
             len += sprintf(buf + len, "Tx Multicast Packets: %u\n", vpid_stats.tx_multicast_pkt);
         if( len < limit )
             len += sprintf(buf + len, "Tx Bytes            : %llu\n", vpid_stats.tx_byte);
         if( len < limit )
             len += sprintf(buf + len, "Tx Errors           : %u\n", vpid_stats.tx_error);
         if( len < limit )
             len += sprintf(buf + len, "Tx Discards         : %u\n\n", vpid_stats.tx_discard_pkt);
         dev_put(cur_dev);
    }

    return len;
}

static void hil_test_session_creation(void)
{
    struct sk_buff *skb;
    struct net_device *cniDev;
    struct net_device *ethDev;
    Int8 cniVpid;
    Int8 ethVpid;
    Int8 tempVpid;
    AVALANCHE_PP_VPID_INFO_t *vpid;
    AVALANCHE_PP_PID_t *pid;
    AVALANCHE_PP_SESSION_INFO_t *session_info;
    Bool    usSession = False;
    Bool    dsSession = False;
    Int32 i,j;
    i = 0;

    if ((usPktDataIngressP != NULL) && (usPktDataEgressP != NULL))
    {
        usSession = True;
    }
    if ((dsPktDataIngressP != NULL) && (dsPktDataEgressP != NULL))
    {
        dsSession = True;
    }
    if ((usSession == False) && (dsSession == False))
    {
        printk("hil_test_session_creation: Cannot create session since one of the packets was not configured\n");
        return;
    }
    else if (usSession == False)
    {
        printk("hil_test_session_creation: Cannot create US session since one of the packets was not configured\n");
    }
    else if (dsSession == False)
    {
        printk("hil_test_session_creation: Cannot create DS session since one of the packets was not configured\n");
    }

    cniVpid = -1;
    ethVpid = -1;
    for (tempVpid = AVALANCHE_PP_MAX_VPID - 1; tempVpid >= 0; tempVpid--)
    {
        if (avalanche_pp_vpid_get_info(tempVpid, &vpid) == 0)
        {
            if (avalanche_pp_pid_get_info(vpid->parent_pid_handle, &pid) == 0)
            {
                if (AVALANCHE_PP_PID_TYPE_DOCSIS == pid->type)
                {
                    cniVpid = tempVpid;
                    if (ethVpid != -1)
                    {
                        break;
                    }
                }
                if (AVALANCHE_PP_PID_TYPE_ETHERNET == pid->type)
                {
                    ethVpid = tempVpid;
                    if (cniVpid != -1)
                    {
                        break;
                    }
                }
            }
        }
    }
    if ((ethVpid == -1) || (cniVpid == -1))
    {
        printk("hil_test_session_creation: Failed to acquire ethVpid or cniVpid\n");
        return;
    }
    if(!(skb = dev_alloc_skb(2048)))
    {
        printk("hil_test_session_creation: Failed to allocate skb\n");
        return;
    }

    skb->mac_header = skb->data;
    // TBD
    skb->pp_packet_info.egress_queue = TI_PPM_EGRESS_QUEUE_INVALID;

    cniDev = __dev_get_by_name(&init_net, "cni0");
    ethDev = __dev_get_by_name(&init_net, "l2sd0");

    if (usSession) 
    {
        /********************/
        /* US ingress packet*/
        /********************/
        session_info = &skb->pp_packet_info.pp_session;
        session_info->ingress.vpid_handle = ethVpid;
        session_info->egress.vpid_handle = cniVpid;
        memcpy(skb->data, usPktDataIngressP, usPktDataIngressSize);


        skb->skb_iif = ethDev->ifindex;
        skb->dev = ethDev;
        ////printk("%s - %d\n", __FUNCTION__, i++);
        //hil_ingress_hook(skb);
        //ethDev = __dev_get_by_name(&init_net, "l2sd0.2");
        //skb->skb_iif = ethDev->ifindex;
        //skb->dev = ethDev;
        skb->vpid_vlan_tci = 2;

        //printk("%s INGRESS START\n", __FUNCTION__);
        j = hil_ingress_hook(skb); 
        //printk("%s INGRESS END\n", __FUNCTION__);
        //printk("%s - %d, ingress hook ret code: %d\n", __FUNCTION__, __LINE__, j);


        /*******************/
        /* US egress packet*/
        /*******************/

        skb->skb_iif = cniDev->ifindex;
        skb->dev = cniDev;
        memcpy(skb->data, usPktDataEgressP, usPktDataEgressSize);

        //printk("%s EGRESS START\n", __FUNCTION__);
        j = hil_egress_hook(skb);
        //printk("%s EGRESS END\n", __FUNCTION__);
        //printk("%s - %d, egress hook ret code: %d\n", __FUNCTION__, __LINE__, j);

    }

    if (dsSession) 
    {
        /********************/
        /* DS ingress packet*/
        /********************/
        skb->skb_iif = cniDev->ifindex;
        skb->dev = cniDev;
        memset((Uint8*)&skb->pp_packet_info.pp_session, 0, sizeof(skb->pp_packet_info.pp_session));
        session_info = &skb->pp_packet_info.pp_session;
        session_info->ingress.vpid_handle = cniVpid;
        session_info->egress.vpid_handle = ethVpid;
        memcpy(skb->data, dsPktDataIngressP, dsPktDataIngressSize);
        /*int k;
        printk("printing packet\n=====\n");
        for (k=0; k<10; k++) {
            printk("%02X ", *(skb->data + k));
        }*/

        //printk("%s INGRESS\n====\n\n", __FUNCTION__);
        j = hil_ingress_hook(skb); 
        //printk("%s INGRESS COMPLETE ret code: %d, ran with skb->data at: %p\n\n", __FUNCTION__, j, skb->data);

        dev_put(cniDev);

        /*******************/
        /* DS egress packet*/
        /*******************/
        memcpy(skb->data, dsPktDataEgressP, dsPktDataEgressSize);

        skb->skb_iif = ethDev->ifindex;
        skb->dev = ethDev;
        skb->vpid_vlan_tci = 2;
        //hil_egress_hook(skb);
        //ethDev = __dev_get_by_name(&init_net, "l2sd0");
        //skb->skb_iif = ethDev->ifindex;
        //skb->dev = ethDev;


        //printk("%s EGRESS\n====\n\n", __FUNCTION__);
        j = hil_egress_hook(skb);
        //printk("%s EGRESS COMPLETE ret code: %d\n\n", __FUNCTION__, j);

        dev_put(ethDev);
    }

    /* Change sessions state to forwarding and set session serial number */
    *(Uint16*)((Uint32)(((Uint32)IO_ADDRESS(0x0330000C)) + (2047 * 0x50))) = 0x0003;
    if (usSession && dsSession)
    {
        *(Uint16*)((Uint32)(((Uint32)IO_ADDRESS(0x0330000C)) + (2046 * 0x50))) = 0x0103;
    }
}
Int32 HIL_get_qos_discard_ooo_state(void)
{
    return global_hil_db.discard_OOO;
}

EXPORT_SYMBOL(HIL_get_qos_discard_ooo_state);


static void pushPacketToPrefetcher(int direction)
{
#if 0
    PAL_Handle                  palHandle;
    Cppi4Queue                  freeQ;
    Cppi4Queue                  destQ;
    PAL_Cppi4QueueHnd           freeQHnd;
    PAL_Cppi4QueueHnd           dstQueueHnd;
    Cppi4EmbdDescPp             *freeQDesc;
    Ptr                         buffer;
    struct net_device           *ingressDev, *cniDev;
    Int8                        ingressPid;
    Int32                       len;
    int k;

    static int warn=0;
    if (warn == 0) {
        printk("\n################################\n#\n#  Use of This Function Without Going Over its Code First is Discouraged!\n#\n############################\n\n");
        warn = 1;
    }

    // cheat by creating a CNI vpid (not created if no CMTS registration)
    cniDev = __dev_get_by_name(&init_net, "cni0");
    len = avalanche_pp_vpid_create(&cniDev->vpid_block);
    printk("CNI avalanche_pp_vpid_create return code: %d\n", len);

    if (direction == 0) {
        ingressDev = __dev_get_by_name(&init_net, "cni0");
        if (dsPktDataEgressP == NULL) 
        {
            printk("%s using hardcoded DS packets\n", __FUNCTION__);
            dsPktDataEgressP = dsPktDataEgress_GreL3;
            dsPktDataIngressP = dsPktDataIngress_GreL3;
            dsPktDataIngressSize = sizeof(dsPktDataIngress_GreL3);
            dsPktDataEgressSize  = sizeof(dsPktDataEgress_GreL3);   
        }

        usPktDataIngressP = NULL; // avoid creating US session

        printk("printing DS ingress packet packet\n=====\n");
        for (k=0; k<dsPktDataIngressSize; k++) {
            printk("%02X ", *(dsPktDataIngressP + k));
        }  
        printk("\n\n");
        printk("printing DS egress packet packet\n=====\n");
        for (k=0; k<dsPktDataEgressSize; k++) {
            printk("%02X ", *(dsPktDataEgressP + k));
        } 
        printk("\n\n");
    }
    else 
    {
        ingressDev = __dev_get_by_name(&init_net, "l2sd0");
        if (usPktDataIngressP == NULL) 
        {
            printk("%s using hardcoded US packets\n", __FUNCTION__);
            usPktDataIngressP = usPktDataIngress_GreL3;
            usPktDataEgressP = usPktDataEgress_GreL3;
            usPktDataIngressSize = sizeof(usPktDataIngress_GreL3);
            usPktDataEgressSize  = sizeof(usPktDataEgress_GreL3);
        }

        dsPktDataIngressP = NULL; // avoid creating DS session

        printk("printing US ingress packet packet\n=====\n");
        for (k=0; k<usPktDataIngressSize; k++) {
            printk("%02X ", *(usPktDataIngressP + k));
        }  
        printk("\n\n");
        printk("printing US egress packet packet\n=====\n");
        for (k=0; k<usPktDataEgressSize; k++) {
            printk("%02X ", *(usPktDataEgressP + k));
        } 
        printk("\n\n");
    }
    if (ingressDev == NULL)
    {
        //printk("%s: Cannot get ingress device %s\n", __FUNCTION__);
        return;
    }

    // open session
    hil_test_session_creation();

    ingressPid = ingressDev->vpid_block.parent_pid_handle;
    dev_put(ingressDev);

    /* Allocate descriptor and buffer */
    palHandle = PAL_cppi4Init (NULL,(Ptr)PAL_CPPI41_QUEUE_MGR_PARTITION_SR); // NULL, NULL
    if (palHandle == NULL)
    {
        //printk("%s %d Error: handle is NULL\n", __FUNCTION__, __LINE__);
        return;
    }
    
    freeQ.qMgr = PAL_CPPI41_QUEUE_MGR_PARTITION_SR;
    freeQ.qNum = PAL_CPPI41_SR_PROXY_PDSP_LOW_FD_EMB_Q_NUM; 
    // use proxy freeQ
    if(!(freeQHnd = PAL_cppi4QueueOpenNoReset(palHandle, freeQ)))
    {
        printk(KERN_ERR "Unable to open free desc queue #%d\n", freeQ.qNum);
        return;
    }

    freeQDesc = (Cppi4EmbdDescPp*)PAL_cppi4QueuePop(freeQHnd);
    if (freeQDesc == NULL)
    {
        printk("pushToSeq got NULL when popping freeQDesc\n");
        return;
    }
    
    freeQDesc = (Cppi4EmbdDescPp*)PAL_CPPI4_PHYS_2_VIRT((Uint32)freeQDesc);

    buffer = PAL_cppi4BufPopBuf(palHandle, (Cppi4BufPool){PAL_CPPI41_QUEUE_MGR_PARTITION_SR, PAL_CPPI41_BMGR_POOL11});
    // what buffer pool to use?
    if (buffer == NULL)
    {
        printk("pushToPrefetcher got NULL when popping buffer\n");
        return;
    }
    
    buffer = (Ptr)PAL_CPPI4_PHYS_2_VIRT((Uint32)buffer + 0x80);

    /* Intitalize buffer */
    if (direction == 0) 
    {
        len = dsPktDataIngressSize;
        memcpy(buffer, dsPktDataIngressP, len);
    }
    else
    {
        usPktDataIngressP = usPktDataIngress_GreL3_VLAN;
        usPktDataIngressSize = sizeof(usPktDataIngress_GreL3_VLAN);
        len = usPktDataIngressSize;
        memcpy(buffer, usPktDataIngressP, len);
    }

    PAL_CPPI4_CACHE_WRITEBACK((unsigned long)buffer, len);

    /* Initialize descriptor */
    freeQDesc->descInfo |= (1 << CPPI41_EM_DESCINFO_PSWSIZE_SHIFT) + len;
    freeQDesc->tagInfo |= ingressPid << CPPI41_EM_TAGINFO_SRCPORT_SHIFT;
    freeQDesc->pktInfo |= (1 << CPPI41_EM_PKTINFO_RETPOLICY_SHIFT) + (CPPI41_EM_PKTINFO_PKTTYPE_ETH << CPPI41_EM_PKTINFO_PKTTYPE_SHIFT);
    //freeQDesc->pktInfo += (freeQ.qMgr << 12 | freeQ.qNum);
    freeQDesc->Buf.BufInfo = (1 << CPPI41_EM_BUF_VALID_SHIFT) + (PAL_CPPI41_BMGR_POOL11 << CPPI41_EM_BUF_POOL_SHIFT) + len;
    freeQDesc->Buf.BufPtr = PAL_CPPI4_VIRT_2_PHYS((Uint32)buffer);
    PAL_CPPI4_CACHE_WRITEBACK((unsigned long)freeQDesc, 28);

    /* Push descriptor to Prefetcher */
    destQ.qMgr = PAL_CPPI41_QUEUE_MGR_PARTITION_SR;
    destQ.qNum = PAL_CPPI41_SR_PPDSP_LOW_Q_NUM;
    dstQueueHnd = PAL_cppi4QueueOpenNoReset(palHandle, destQ);
    PAL_cppi4QueuePush(dstQueueHnd, (Ptr)PAL_CPPI4_VIRT_2_PHYS((Uint32)freeQDesc), PAL_CPPI4_DESCSIZE_2_QMGRSIZE(64), len);
    printk("pushed descriptor %X, buffer %X\n", PAL_CPPI4_VIRT_2_PHYS((Uint32)freeQDesc), PAL_CPPI4_VIRT_2_PHYS((Uint32)buffer));
#else
    printk("Not currently compiled. If compiling, make sure to provide default US and DS packets\n");
#endif
}

AVALANCHE_PP_RET_e avalanche_pp_set_external_defensive_state(avalanche_pp_defensive_state_ioctl_param_t state)
{
    if (IS_BRIDGE())
    {
        /* Bridge mode - Move to Normal mode. */
        if (state == AVALANCHE_PP_DDH_STATE_NORMAL)
        {
            DBGPRINTK("disabling l2classification");
            avalanche_pp_support_l2_classification(False, PP_PID_NUM_INVALID);
            avalanche_pp_support_smart_prioritization(False);
            avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_NORMAL);
            DBGPRINTK("Switching to NORMAL");
        }
        /* Bridge mode - Move to Full-Defensive mode. */
        else if (state == AVALANCHE_PP_DDH_STATE_FULL_DEFENSIVE)
        {
            DBGPRINTK("enabling multi-drop");
            avalanche_pp_support_multi_drop(True);
            DBGPRINTK("enabling l2classification for all pids");
            avalanche_pp_support_l2_classification(True, PP_PID_NUM_ALL);
            avalanche_pp_support_smart_prioritization(True);
            avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_FULL_DEFENSIVE);
            DBGPRINTK("Switching to FULL_DEFENSIVE");
        }
        /* Bridge mode - Move to DS-Defensive mode. */
        else if (state == AVALANCHE_PP_DDH_STATE_DS_DEFENSIVE)
        {
            DBGPRINTK("enabling multi-drop");
            avalanche_pp_support_multi_drop(True);
            DBGPRINTK("enabling l2classification for DS Pid");
            avalanche_pp_support_l2_classification(True, global_hil_db.ddh_l2_classification_pid);
            avalanche_pp_support_smart_prioritization(True);
            avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_DS_DEFENSIVE);
            DBGPRINTK("Switching to DS_DEFENSIVE");
        }
        else
        {
            /* Switch to the given state is illegal from the current state. */
            printk("Switch to state { %d } failed.\n", state);
            return PP_RC_FAILURE;
        }
	}

	if (IS_HYBRID())
    {
        /* Hybrid Mode - Move to Full Defensive state */
        if (state == AVALANCHE_PP_DDH_STATE_FULL_DEFENSIVE)
        {
            DBGPRINTK("enabling multi-drop");
            avalanche_pp_support_multi_drop(True);
            DBGPRINTK("enabling l2classification for all pids");
            avalanche_pp_support_l2_classification(True, PP_PID_NUM_ALL);
            DBGPRINTK("enabling smart prioritization");
            avalanche_pp_support_smart_prioritization(True);
            avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_FULL_DEFENSIVE);
            DBGPRINTK("Switching to FULL_DEFENSIVE");
        }
        /* Hybrid Mode - Move to Normal state */
        else if (state == AVALANCHE_PP_DDH_STATE_NORMAL)
        {
            DBGPRINTK("disabling l2classification");
            avalanche_pp_support_l2_classification(False, PP_PID_NUM_INVALID);
            DBGPRINTK("disabling smart prioritization");
            avalanche_pp_support_smart_prioritization(False);
            avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_NORMAL);
            DBGPRINTK("Switching to NORMAL");
        }
        else
        {
            /* Switch to the given state is illegal from the current state. */
            printk("Switch to state { %d } failed.\n", state);
            return PP_RC_FAILURE;
        }
    }

    return PP_RC_SUCCESS;
}

/**************************************************************************
 * FUNCTION NAME : void __hil_ddh_init
 **************************************************************************
 * DESCRIPTION   :
 *  This function initiates the ddh structure
 *
 * return - void
 ***************************************************************************/
static void __hil_ddh_init()
{
    avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_NORMAL);

#ifdef CONFIG_TI_DDH_GW
    DBGPRINTK("Hybrid Mode");
    global_hil_db.is_hybrid = True;
#endif

    global_hil_db.ddh_last_timer_tick_jiffies = jiffies;
}

/**************************************************************************
 * FUNCTION NAME : void __hil_ddh_process
 **************************************************************************
 * DESCRIPTION   :
 *  This function will be called from within hil_null_hook and decide if to open a session or not
 *
 * return - Indication whether to continue with processing or not
 ***************************************************************************/
static Bool __hil_ddh_process(struct sk_buff* skb)
{
    AVALANCHE_PP_DDH_STATE_e         	state;
    AVALANCHE_PP_Misc_Statistics_t   	stats;
    AVALANCHE_PP_BLACKLIST_INFO_e    	blacklist_info;
    Bool                             	multi_drop_enabled;
    AVALANCHE_PP_RET_e               	rc = PP_RC_SUCCESS;
	AVALANCHE_PP_Defensive_Mode_e	    ddh_mode;

    /* Get the current defensive state */
    avalanche_pp_get_defensive_state (&state);
    rc = avalanche_pp_get_blacklist_info(&multi_drop_enabled, skb->pp_packet_info.pp_session.ingress.lookup.LUT1.u.fields.L3.entry_type, &blacklist_info);
	avalanche_pp_get_defensive_mode(&ddh_mode);

    /* Multi-drop enabled */
    if (multi_drop_enabled)
    {
        /* Make sure we have a valid blacklist info */
        if (rc != PP_RC_SUCCESS)
            return False;

        /* If blacklist is full, stop processing */
        if (blacklist_info == AVALANCHE_PP_BLACKLIST_FULL)
        {
			if (ddh_mode != AVALANCHE_PP_DDH_MODE_EXTERNAL)
			{
				/* GW Mode - DoS -> DDoS */
				if (IS_GW() && (state == AVALANCHE_PP_DDH_STATE_DOS))
				{
					/* Reset Full-Defensive pps counter */
					global_hil_db.ddh_consecutive_low_pps_ctr = 0;
                    global_hil_db.ddh_consecutive_low_sps_ctr = 0;

					DBGPRINTK("enabling smart prioritization");
					avalanche_pp_support_smart_prioritization(True);
					avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_DDOS);
					DBGPRINTK("Blacklist full, switching to DDoS");
				}

				/* Hybrid Mode - Normal -> Full Defensive */
				if (IS_HYBRID() && (state == AVALANCHE_PP_DDH_STATE_NORMAL))
				{
					/* Reset Full-Defensive pps counter */
					global_hil_db.ddh_consecutive_low_pps_ctr = 0;
                    global_hil_db.ddh_consecutive_low_sps_ctr = 0;

					DBGPRINTK("enabling smart prioritization");
					avalanche_pp_support_smart_prioritization(True);
					DBGPRINTK("enabling l2classification for all pids");
                    avalanche_pp_support_l2_classification(True, PP_PID_NUM_ALL);
					avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_FULL_DEFENSIVE);
					DBGPRINTK("Blacklist full, switching to Defensive");
				}
			}
            /* Blacklist full - Do not open multi-drop session */
            return False;
        }
        else
        {
            return True;
        }
    }

    /* Multi-drop disabled */
    else
    {
        avalanche_pp_get_db_stats(&stats);

        /* Check if we passed maximum allowed drop sessions */
        if (stats.active_drop_sessions >= DDH_MAX_DROP_SESSIONS) 
        {
            /* Enable multi-drop */
            DBGPRINTK("enabling multi-drop");
            avalanche_pp_support_multi_drop(True);

            if (ddh_mode != AVALANCHE_PP_DDH_MODE_EXTERNAL)
            {
                /* GW Mode - Normal -> DoS */
                if(IS_GW() && (state == AVALANCHE_PP_DDH_STATE_NORMAL))
                {
                    avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_DOS);
                    DBGPRINTK("Drop sessions > %d, switching to DoS", DDH_MAX_DROP_SESSIONS);
                }
            }
        }

        return True;
    }
}

/**************************************************************************
 * FUNCTION NAME : void __hil_ddh_event
 **************************************************************************
 * DESCRIPTION   :
 *  This function will be called from the hil_time context and will do transition between ddh states
 *
 * return - void
 ***************************************************************************/
static void __hil_ddh_event()
{
    AVALANCHE_PP_BLACKLIST_INFO_e       blacklist_IPv4_info;
    AVALANCHE_PP_BLACKLIST_INFO_e       blacklist_IPv6_info;
    Bool                                multi_drop_enabled;
    Uint32                              host_pps = 0, host_sps = 0;
    Uint32                              session_utilization = 0;
    Uint32                              us_session_utilization = 0;

    AVALANCHE_PP_Defensive_Mode_e       ddh_mode;
    AVALANCHE_PP_DDH_STATE_e            state;
    AVALANCHE_PP_Misc_Statistics_t      stats;
    AVALANCHE_PP_QOS_QUEUE_STATS_t      qosStats;
    unsigned long                       current_timestamp = jiffies;

    /* Summarize total cycle duration  */
    global_hil_db.ddh_cycle_duration_jiffies += (current_timestamp - global_hil_db.ddh_last_timer_tick_jiffies); 
    global_hil_db.ddh_last_timer_tick_jiffies = current_timestamp;

    /* Get the current defensive state */
    avalanche_pp_get_defensive_state (&state);

    /* Bridge & Hybrid Mode */
    if(IS_BRIDGE() || IS_HYBRID())
    {
        /* Exit multi-drop mode if needed */
        avalanche_pp_get_blacklist_info(&multi_drop_enabled, AVALANCHE_PP_LUT_ENTRY_L3_IPV4, &blacklist_IPv4_info);
        avalanche_pp_get_blacklist_info(&multi_drop_enabled, AVALANCHE_PP_LUT_ENTRY_L3_IPV6, &blacklist_IPv6_info);
        if (multi_drop_enabled && (blacklist_IPv4_info == AVALANCHE_PP_BLACKLIST_EMPTY) && (blacklist_IPv6_info == AVALANCHE_PP_BLACKLIST_EMPTY))
        {
            if (IS_HYBRID() && (state == AVALANCHE_PP_DDH_STATE_FULL_DEFENSIVE))
            {
                DBGPRINTK("Do not exit multi-drop if in hybrid and full defensive");
            }
            else
            {
                DBGPRINTK("Blacklist empty, disabling multi drop");
                avalanche_pp_support_multi_drop(False);
            }
        }

        /* Calculate session utilization for Normal & DS-Defensive modes */
        if ((state == AVALANCHE_PP_DDH_STATE_DS_DEFENSIVE) || (state == AVALANCHE_PP_DDH_STATE_NORMAL))
        {
            /* Get statistics */
            avalanche_pp_get_db_stats(&stats);

            if (stats.active_sessions != 0)
            {
                session_utilization     = (stats.active_sessions * 100) / AVALANCHE_PP_MAX_ACCELERATED_SESSIONS;
                us_session_utilization  = (stats.active_us_sessions * 100) / stats.active_sessions;
                /* DBGPRINTK("session utilization is %d, us session utilization is %d", session_utilization, us_session_utilization); */
            }
        }
    }

    avalanche_pp_get_defensive_mode(&ddh_mode);
    if (ddh_mode == AVALANCHE_PP_DDH_MODE_EXTERNAL) 
    {
        /* DDH change state only according to user command - do nothing. */
        return;
    }

    /* Calculate pps */
    host_pps = __hil_ddh_calc_host_pps();
    host_sps = __hil_ddh_calc_session_rate();

    /* Check high pps every second */
    if (host_pps >= DDH_HOST_PPS_THRESHOLD)
    {
        global_hil_db.ddh_consecutive_high_pps_ctr++;
        global_hil_db.ddh_consecutive_low_pps_ctr = 0;
    }
    else
    {
        global_hil_db.ddh_consecutive_high_pps_ctr = 0;
        global_hil_db.ddh_consecutive_low_pps_ctr++;
    }

    /* Check high sps every second */
    if (host_sps >= DDH_MAX_SESSIONS_PER_SECOND)
    {
        global_hil_db.ddh_consecutive_high_sps_ctr++;
        global_hil_db.ddh_consecutive_low_sps_ctr = 0;
    }
    else
    {
        global_hil_db.ddh_consecutive_high_sps_ctr = 0;
        global_hil_db.ddh_consecutive_low_sps_ctr++;
    }

    /*
    DBGPRINTK("state - %d, high_pps_ctr:%d, low_pps_ctr:%d", state,
              global_hil_db.ddh_consecutive_high_pps_ctr,
              global_hil_db.ddh_consecutive_low_pps_ctr);
    */
    __hil_ddh_reset_pps_counters();

    /* Bridge Mode */
    if(IS_BRIDGE())
    {
        /*************************************/
        /**** Normal -> DS/Full-Defensive ****/
        /*************************************/

        if (state == AVALANCHE_PP_DDH_STATE_NORMAL)
        {
            /* Normal -> Full-Defensive */
            if ( (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0) && (session_utilization >= DDH_LOW_SESSION_UTILIZATION_THRESHOLD) && (us_session_utilization >= DDH_US_SESSION_UTILIZATION_THRESHOLD)) ||
                 (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_LONG_TIMEOUT_SECONDS == 0)) ||
                 (global_hil_db.ddh_consecutive_high_sps_ctr && (global_hil_db.ddh_consecutive_high_sps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0)) )
            {
                /* Reset Full-Defensive pps counter */
                global_hil_db.ddh_consecutive_low_pps_ctr = 0;
                global_hil_db.ddh_consecutive_low_sps_ctr = 0;

                DBGPRINTK("enabling multi-drop");
                avalanche_pp_support_multi_drop(True);
                DBGPRINTK("enabling l2classification for all pids");
                avalanche_pp_support_l2_classification(True, PP_PID_NUM_ALL);
                avalanche_pp_support_smart_prioritization(True);
                avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_FULL_DEFENSIVE);

                if (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0) && (session_utilization >= DDH_LOW_SESSION_UTILIZATION_THRESHOLD) && (us_session_utilization >= DDH_US_SESSION_UTILIZATION_THRESHOLD))
                    DBGPRINTK("Host pps is %d for more than 10 seconds & high us session utilization , switching to FULL_DEFENSIVE", host_pps);
                else if (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_LONG_TIMEOUT_SECONDS == 0))
                    DBGPRINTK("Host pps is %d for more than %d seconds, switching to FULL_DEFENSIVE", host_pps, DDH_EVENT_LONG_TIMEOUT_SECONDS);
                else if (global_hil_db.ddh_consecutive_high_sps_ctr && (global_hil_db.ddh_consecutive_high_sps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0))
                    DBGPRINTK("Host sps is %d for more than %d seconds, switching to FULL_DEFENSIVE", host_sps, DDH_EVENT_SHORT_TIMEOUT_SECONDS);
                return;
            }

            /* Normal -> DS-Defensive */
            if (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0) && (session_utilization >= DDH_LOW_SESSION_UTILIZATION_THRESHOLD))
            {
                /* Reset DS-Defensive pps counters */
                global_hil_db.ddh_consecutive_high_pps_ctr = 0;
                global_hil_db.ddh_consecutive_high_sps_ctr = 0;
                global_hil_db.ddh_consecutive_low_sps_ctr = 0;

                DBGPRINTK("enabling multi-drop");
                avalanche_pp_support_multi_drop(True);
                DBGPRINTK("enabling l2classification for DS Pid");
                avalanche_pp_support_l2_classification(True, global_hil_db.ddh_l2_classification_pid);
                avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_DS_DEFENSIVE);
                DBGPRINTK("Host pps is %d for more than %d seconds & high session utilization , switching to DS_DEFENSIVE", host_pps, DDH_EVENT_SHORT_TIMEOUT_SECONDS);
                return;
            }
        } /* Normal */

        /*************************************/
        /****** Full-Defensive -> Normal *****/
        /*************************************/

        if (state == AVALANCHE_PP_DDH_STATE_FULL_DEFENSIVE)
        {
            if ((global_hil_db.ddh_consecutive_low_pps_ctr >= DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS) && (global_hil_db.ddh_consecutive_low_sps_ctr >= DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS))
            {
                DBGPRINTK("disabling l2classification");
                avalanche_pp_support_l2_classification(False, PP_PID_NUM_INVALID);
                avalanche_pp_support_smart_prioritization(False);
                avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_NORMAL);
                DBGPRINTK("Low pps for DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS seconds, Switching to NORMAL");
                return;
            }
        } /* Full-Defensive */

        /****************************************************/
        /****** DS-Defensive -> Normal / Full-Defensive *****/
        /****************************************************/

        if (state == AVALANCHE_PP_DDH_STATE_DS_DEFENSIVE)
        {
            /* DS-Defensive -> Normal */
            if (global_hil_db.ddh_consecutive_low_pps_ctr >= DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS)
            {
                /* Reset Normal pps counters */
                global_hil_db.ddh_consecutive_high_sps_ctr = 0;

                DBGPRINTK("disabling l2classification");
                avalanche_pp_support_l2_classification(False, PP_PID_NUM_INVALID);
                avalanche_pp_support_smart_prioritization(False);
                avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_NORMAL);
                DBGPRINTK("Low pps for DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS seconds, Switching to NORMAL");
                return;
            }

            /* DS-Defensive -> Full-Defensive */
            if ( (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_LONG_TIMEOUT_SECONDS == 0)) ||
                 ((global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0) && (session_utilization >= DDH_HIGH_SESSION_UTILIZATION_THRESHOLD)) ||
                 (global_hil_db.ddh_consecutive_high_sps_ctr && (global_hil_db.ddh_consecutive_high_sps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0)) )
            {
                /* Reset Full-Defensive pps counter */
                global_hil_db.ddh_consecutive_low_pps_ctr = 0;
                global_hil_db.ddh_consecutive_low_sps_ctr = 0;

                DBGPRINTK("enabling l2classification for all pids");
                avalanche_pp_support_l2_classification(True, PP_PID_NUM_ALL); 
                avalanche_pp_support_smart_prioritization(True);
                avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_FULL_DEFENSIVE);

                if (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_LONG_TIMEOUT_SECONDS == 0))
                    DBGPRINTK("Host pps is %d for more than %d seconds, switching to FULL_DEFENSIVE", host_pps, DDH_EVENT_LONG_TIMEOUT_SECONDS);
                else if ((global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0) && (session_utilization >= DDH_HIGH_SESSION_UTILIZATION_THRESHOLD))
                    DBGPRINTK("Host pps is %d for more than %d seconds, switching to FULL_DEFENSIVE", host_pps, DDH_EVENT_SHORT_TIMEOUT_SECONDS);
                else if (global_hil_db.ddh_consecutive_high_sps_ctr && (global_hil_db.ddh_consecutive_high_sps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0))
                    DBGPRINTK("Host sps is %d for more than %d seconds, switching to FULL_DEFENSIVE", host_sps, DDH_EVENT_SHORT_TIMEOUT_SECONDS);
                return;
            }
        } /* DS-Defensive */
    }

    /* GW Mode */
    else if(IS_GW())
    {
        /************************/
        /**** Normal -> DDoS ****/
        /************************/

        if (state == AVALANCHE_PP_DDH_STATE_NORMAL)
        {
            /* Normal -> DDoS */
            if ( (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_LONG_TIMEOUT_SECONDS == 0)) ||
                 (global_hil_db.ddh_consecutive_high_sps_ctr && (global_hil_db.ddh_consecutive_high_sps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0)) )
            {
                /* Reset DDoS pps counter */
                global_hil_db.ddh_consecutive_low_pps_ctr = 0;
                global_hil_db.ddh_consecutive_low_sps_ctr = 0;

                DBGPRINTK("enabling multi drop");
                avalanche_pp_support_multi_drop(True);
                DBGPRINTK("enabling smart prioritization");
                avalanche_pp_support_smart_prioritization(True);
                avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_DDOS);

                if (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_LONG_TIMEOUT_SECONDS == 0))
                    DBGPRINTK("Host pps is %d for more than %d seconds, switching to DDoS", host_pps, DDH_EVENT_LONG_TIMEOUT_SECONDS);
                else if (global_hil_db.ddh_consecutive_high_sps_ctr && (global_hil_db.ddh_consecutive_high_sps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0))
                    DBGPRINTK("Host sps is %d for more than %d seconds, switching to DDoS", host_sps, DDH_EVENT_LONG_TIMEOUT_SECONDS);
                return;
            }
        } /* Normal */

        /*******************************/
        /****** DDoS -> Normal/DoS *****/
        /*******************************/

        if (state == AVALANCHE_PP_DDH_STATE_DDOS)
        {
            if ((global_hil_db.ddh_consecutive_low_pps_ctr >= DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS) && (global_hil_db.ddh_consecutive_low_sps_ctr >= DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS))
            {
                avalanche_pp_get_blacklist_info(&multi_drop_enabled, AVALANCHE_PP_LUT_ENTRY_L3_IPV4, &blacklist_IPv4_info);
                avalanche_pp_get_blacklist_info(&multi_drop_enabled, AVALANCHE_PP_LUT_ENTRY_L3_IPV6, &blacklist_IPv6_info);

                /* DDoS -> Normal */
                if ((blacklist_IPv4_info == AVALANCHE_PP_BLACKLIST_EMPTY) && (blacklist_IPv6_info == AVALANCHE_PP_BLACKLIST_EMPTY))
                {
                    DBGPRINTK("disabling multi drop");
                    avalanche_pp_support_multi_drop(False);
                    DBGPRINTK("disabling smart prioritization");
                    avalanche_pp_support_smart_prioritization(False);
                    avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_NORMAL);
                    DBGPRINTK("Low pps for DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS seconds, Blacklist empty, Switching to NORMAL");
                }

                /* Blacklists are full - DDoS */
                else if ((blacklist_IPv4_info == AVALANCHE_PP_BLACKLIST_FULL) || (blacklist_IPv6_info == AVALANCHE_PP_BLACKLIST_FULL))
                {
                    /* Stay in DDoS */
                    DBGPRINTK("Low pps for DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS seconds, Blacklist is full, Staying in DDoS");
                }

                /* Blacklists are not empty and not full  - DDoS -> DoS */
                else
                {
                    DBGPRINTK("disabling smart prioritization");
                    avalanche_pp_support_smart_prioritization(False);
                    avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_DOS);
                    DBGPRINTK("Low pps for DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS seconds, Blacklist not empty, Switching to DoS");
                }

                return;
            }
        } /* DDoS */

        /****************************/
        /**** DoS -> Normal/DDoS ****/
        /****************************/

        if (state == AVALANCHE_PP_DDH_STATE_DOS)
        {
            avalanche_pp_get_db_stats(&stats);
            avalanche_pp_get_blacklist_info(&multi_drop_enabled, AVALANCHE_PP_LUT_ENTRY_L3_IPV4, &blacklist_IPv4_info);
            avalanche_pp_get_blacklist_info(&multi_drop_enabled, AVALANCHE_PP_LUT_ENTRY_L3_IPV6, &blacklist_IPv6_info);

            /* DoS -> Normal */
            if ((blacklist_IPv4_info == AVALANCHE_PP_BLACKLIST_EMPTY) && (blacklist_IPv6_info == AVALANCHE_PP_BLACKLIST_EMPTY) && (stats.active_drop_sessions < DDH_MAX_DROP_SESSIONS)) 
            {
                DBGPRINTK("disabling multi drop");
                avalanche_pp_support_multi_drop(False);
                avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_NORMAL);
                DBGPRINTK("Blacklist empty, drop sessions are low, Switching to NORMAL");
                return;
            }

            /* DoS -> DDoS */
            if (global_hil_db.ddh_consecutive_high_sps_ctr && (global_hil_db.ddh_consecutive_high_sps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0))
            {
                /* Reset Full-Defensive pps counter */
                global_hil_db.ddh_consecutive_low_pps_ctr = 0;

                DBGPRINTK("enabling smart prioritization");
                avalanche_pp_support_smart_prioritization(True);
                avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_DDOS);
                DBGPRINTK("Host sps is %d for more than %d seconds, switching to DDoS", host_sps, DDH_EVENT_SHORT_TIMEOUT_SECONDS);
                return;
            }
        } /* DoS */
    }

    /* Hybrid Mode */
    else if(IS_HYBRID())
    {
        /**********************************/
        /**** Normal -> Full-Defensive ****/
        /**********************************/

        if (state == AVALANCHE_PP_DDH_STATE_NORMAL)
        {
            /* Normal -> Full-Defensive */
            if ( (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0) && (session_utilization >= DDH_LOW_SESSION_UTILIZATION_THRESHOLD) && (us_session_utilization >= DDH_US_SESSION_UTILIZATION_THRESHOLD)) ||
                 (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_LONG_TIMEOUT_SECONDS == 0)) ||
                 (global_hil_db.ddh_consecutive_high_sps_ctr && (global_hil_db.ddh_consecutive_high_sps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0)) ||
                 (global_hil_db.ddh_consecutive_low_pps_ctr && (global_hil_db.ddh_consecutive_low_pps_ctr % DDH_EVENT_LONG_TIMEOUT_SECONDS == 0) && (global_hil_db.ddh_num_scb_hash_collisions >= DDH_NUM_SCB_COLLISIONS_THRESHOLD)) ||
                 (global_hil_db.ddh_active_short_connections >= global_hil_db.ddh_num_scb_connections_threshold))
            {
                /* Reset Full-Defensive pps counter */
                global_hil_db.ddh_consecutive_low_pps_ctr = 0;
                global_hil_db.ddh_consecutive_low_sps_ctr = 0;
                global_hil_db.ddh_num_scb_hash_collisions = 0;

                DBGPRINTK("enabling multi-drop");
                avalanche_pp_support_multi_drop(True);
                DBGPRINTK("enabling l2classification for all pids");
                avalanche_pp_support_l2_classification(True, PP_PID_NUM_ALL);
                DBGPRINTK("enabling smart prioritization");
                avalanche_pp_support_smart_prioritization(True);
                avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_FULL_DEFENSIVE);

                if (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0) && (session_utilization >= DDH_LOW_SESSION_UTILIZATION_THRESHOLD) && (us_session_utilization >= DDH_US_SESSION_UTILIZATION_THRESHOLD))
                    DBGPRINTK("Host pps is %d for more than %d seconds & high us session utilization , switching to FULL_DEFENSIVE", host_pps, DDH_EVENT_SHORT_TIMEOUT_SECONDS);
                else if (global_hil_db.ddh_consecutive_high_pps_ctr && (global_hil_db.ddh_consecutive_high_pps_ctr % DDH_EVENT_LONG_TIMEOUT_SECONDS == 0))
                    DBGPRINTK("Host pps is %d for more than %d seconds, switching to FULL_DEFENSIVE", host_pps, DDH_EVENT_LONG_TIMEOUT_SECONDS);
                else if (global_hil_db.ddh_consecutive_high_sps_ctr && (global_hil_db.ddh_consecutive_high_sps_ctr % DDH_EVENT_SHORT_TIMEOUT_SECONDS == 0))
                    DBGPRINTK("Host sps is %d for more than %d seconds, switching to FULL_DEFENSIVE", host_sps, DDH_EVENT_SHORT_TIMEOUT_SECONDS);
                return;
            }

            /* Reset low pps ctr every DDH_EVENT_LONG_TIMEOUT_SECONDS */
            if (global_hil_db.ddh_consecutive_low_pps_ctr >= DDH_EVENT_LONG_TIMEOUT_SECONDS)
            {
                global_hil_db.ddh_consecutive_low_pps_ctr = 0;
                global_hil_db.ddh_num_scb_hash_collisions = 0;
            }
        }

        /*************************************/
        /****** Full-Defensive -> Normal *****/
        /*************************************/

        if (state == AVALANCHE_PP_DDH_STATE_FULL_DEFENSIVE)
        {
            if ((global_hil_db.ddh_consecutive_low_pps_ctr >= DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS) && (global_hil_db.ddh_consecutive_low_sps_ctr >= DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS))
            {
                global_hil_db.ddh_consecutive_low_pps_ctr = 0;
                global_hil_db.ddh_num_scb_hash_collisions = 0;

                DBGPRINTK("disabling l2classification");
                avalanche_pp_support_l2_classification(False, PP_PID_NUM_INVALID);
                DBGPRINTK("disabling smart prioritization");
                avalanche_pp_support_smart_prioritization(False);
                avalanche_pp_set_defensive_state(AVALANCHE_PP_DDH_STATE_NORMAL);
                DBGPRINTK("Low pps for DDH_EVENT_BACK_TO_NORMAL_TIMEOUT_SECONDS seconds, Switching to NORMAL");
                return;
            }
        } /* Full-Defensive */
    }

    return;
}

/**************************************************************************
 * FUNCTION NAME : Uint32 __hil_ddh_calc_host_pps
 **************************************************************************
 * DESCRIPTION   :
 *  This function calcultes host pps
 *
 * return - host pps
 ***************************************************************************/
static Uint32 __hil_ddh_calc_host_pps()
{
    Uint32  calculated_pps;

    avalanche_pp_get_host_packets (&(global_hil_db.ddh_host_forwarded_packets));
    calculated_pps = __hil_ddh_calc_object_per_second(&(global_hil_db.ddh_last_cycle_host_packets), &(global_hil_db.ddh_host_forwarded_packets));;

    /* DBGPRINTK("pps:%d\n", calculated_pps); */
    return calculated_pps; 
}

/**************************************************************************
 * FUNCTION NAME : Uint32 __hil_ddh_calc_session_rate
 **************************************************************************
 * DESCRIPTION   :
 *  This function calcultes session creation rate
 *
 * return - session created per second
 ***************************************************************************/
static Uint32 __hil_ddh_calc_session_rate()
{
    Uint32  calculated_sps = __hil_ddh_calc_object_per_second(&(global_hil_db.ddh_last_cycle_created_sessions), &(global_hil_db.num_total_sessions));

    /* DBGPRINTK("sps:%d\n", calculated_sps); */
    return calculated_sps; 
}

/**************************************************************************
 * FUNCTION NAME : Uint32 __hil_ddh_calc_object_per_second (Uint32* previous_object_count, Uint32* current_object_count)
 **************************************************************************
 * DESCRIPTION   :
 *  This function calcultes object rate
 *
 * return - object rate per second
 ***************************************************************************/
static Uint32 __hil_ddh_calc_object_per_second(Uint32* previous_object_count, Uint32* current_object_count)
{
    Uint32  total_objects_created_in_cycle;
    Uint32  calculated_objects_per_second;
    Uint32  actual_cycle_time_ms = jiffies_to_msecs(global_hil_db.ddh_cycle_duration_jiffies);

    /* Handle wraparound */
    if (*current_object_count >= *previous_object_count)
    {
        total_objects_created_in_cycle  = *current_object_count - *previous_object_count;
    }
    else
    {
        total_objects_created_in_cycle = UINT_MAX - *previous_object_count + *current_object_count;
    }

    if (actual_cycle_time_ms)
    {
        calculated_objects_per_second = ((total_objects_created_in_cycle * 1000) / actual_cycle_time_ms);
    }
    else
    {
        calculated_objects_per_second = 0;
    }

    return calculated_objects_per_second; 
}

/**************************************************************************
 * FUNCTION NAME : void __hil_calc_mac_hash(AVALANCHE_PP_SESSION_INFO_t *session_info, Bool add_operation)
 **************************************************************************
 * DESCRIPTION   :
 *  This function calcultes the mac hash of an US connection
 *
 * return
 ***************************************************************************/
static void __hil_calc_mac_hash(AVALANCHE_PP_SESSION_INFO_t *session_info, Bool add_operation)
{
    Uint16  *mac_ptr;
    Uint16  hash_result;
    Bool    smart_prio_enabled;
    Bool    calc_hash = False;

    if (session_info == NULL)
    {
        BHPRINTK("session_info is NULL");
        return;
    }

    if (add_operation)
    {
        avalanche_pp_get_smart_prioritization(&smart_prio_enabled); 

         BHPRINTK("smart=%d. routable=%d. egress_pid=%d. egress_enable=0x%x", smart_prio_enabled,
                                                                              session_info->is_routable_session,
                                                                              session_info->egress.pid_type,
                                                                              session_info->egress.enable);

        /* Calculate opposite hash */
        if ( smart_prio_enabled &&
            !(session_info->is_routable_session) &&
             (session_info->egress.pid_type == AVALANCHE_PP_PID_TYPE_DOCSIS) &&
            !(session_info->egress.enable & AVALANCHE_PP_EGRESS_FIELD_MAC_HASH_CALCULATED)) 
        {
            session_info->egress.enable |= AVALANCHE_PP_EGRESS_FIELD_MAC_HASH_CALCULATED;
            calc_hash = True;
        }
    }
    else
    {
        if (session_info->egress.enable & AVALANCHE_PP_EGRESS_FIELD_MAC_HASH_CALCULATED)
        {
            calc_hash = True;
        }
    }

    if (calc_hash)
    {
        /* source mac */
        mac_ptr = (Uint16*)(session_info->ingress.lookup.LUT1.u.fields.L2.dstmac);
        hash_result = *mac_ptr;            /* 1st 16 bits */
        hash_result ^= *(mac_ptr + 1);     /* 2nd 16 bits */ 
        hash_result ^= *(mac_ptr + 2);     /* 3rd 16 bits */

        /* destination mac */
        mac_ptr = (Uint16*)(session_info->ingress.lookup.LUT1.u.fields.L2.srcmac);
        hash_result ^= *(mac_ptr);         /* 1st 16 bits */
        hash_result ^= *(mac_ptr + 1);     /* 2nd 16 bits */
        hash_result ^= *(mac_ptr + 2);     /* 3rd 16 bits */

        avalanche_pp_update_bithash (hash_result, add_operation);

        BHPRINTK("%s opposite mac hash: 0x%x. src_mac: %pM. dst_mac: %pM", add_operation ? "Add" : "Remove" , hash_result,
                                                                           (session_info->ingress.lookup.LUT1.u.fields.L2.dstmac),
                                                                           (session_info->ingress.lookup.LUT1.u.fields.L2.srcmac));
    }
}

#if defined(CONFIG_NF_CONNTRACK) || defined(CONFIG_NF_CONNTRACK_MODULE)
/**************************************************************************
 * FUNCTION NAME : void __hil_scb_add(struct sk_buff* skb)
 **************************************************************************
 * DESCRIPTION   :
 *  This function adds a new SCB connection
 *  It also counts number of active SCB connections
 *
 * return - True if this packet should bypass egress hook
 ***************************************************************************/
static void __hil_scb_add(struct sk_buff* skb)
{
    Uint32  lockKey;
    Uint8   dir;
    Bool    is_gw_mac = False;
    Bool    is_rnd_mac = False;
    Bool    is_wan_mac = False;
    Bool    is_mta_mac = False;
    Uint32  hash_LUT1;
    Uint32  hash_LUT2;
    Uint32  scb_hash;
    avalanche_pp_local_dev_addr_ioctl_params_t list_param;
    AVALANCHE_PP_SCB_Entry_t *scb_entry;
    const unsigned char *dest_mac = ((struct ethhdr*)skb_mac_header(skb))->h_dest;

    if(!IS_HYBRID())
        return;

    /* Put invalid scb id */
    skb->pp_packet_info.pp_session.scb_id = AVALANCHE_PP_MAX_ACCELERATED_SESSIONS;

    if(skb->pp_packet_info.pp_session.ingress.lookup.LUT1.u.fields.L3.ip_protocol != IPPROTO_UDP)
    {
        SCBPRINTK("Protocol %d is not supported, only UDP (skb 0x%x)", skb->pp_packet_info.pp_session.ingress.lookup.LUT1.u.fields.L3.ip_protocol, (Uint32)skb);
        return;
    }

    if(dest_mac != NULL)
    {
        if (is_broadcast_ether_addr(dest_mac))
        {
            SCBPRINTK("Broadcast Traffic (skb 0x%x)", (Uint32)skb);
            return;
        }

        if (is_multicast_ether_addr(dest_mac))
        {
            SCBPRINTK("Multicast Traffic (skb 0x%x)", (Uint32)skb);
            return;
        }

        list_param.op_type      = IS_ADDR_EXIST;
        memcpy((void*)(list_param.u.mac_addr), (void *)dest_mac, 6);

       /* Check if destination mac is in the rnd mac list */
        list_param.addr_type    = RND_MAC_ADDR;
        if (avalanche_pp_local_dev_addr(&list_param) == PP_RC_SUCCESS)
        {
            is_rnd_mac = True;
        }

        /* Check if destination mac is GW mac */
        list_param.addr_type    = GW_MAC_ADDR;
        if (avalanche_pp_local_dev_addr(&list_param) == PP_RC_SUCCESS)
        {
            is_gw_mac = True;
        }

        /* Check if destination mac is WAN mac */
        list_param.addr_type    = WAN_MAC_ADDR;
        if (avalanche_pp_local_dev_addr(&list_param) == PP_RC_SUCCESS)
        {
            is_wan_mac = True;
        }

        /* Check if destination mac is MTA mac */
        list_param.addr_type    = MTA_MAC_ADDR;
        if (avalanche_pp_local_dev_addr(&list_param) == PP_RC_SUCCESS)
        {
            is_mta_mac = True;
        }

        /* Check if traffic is bridged or not */
        if (is_rnd_mac || is_gw_mac || is_wan_mac || is_mta_mac)
        {
            SCBPRINTK("Routed Traffic (skb 0x%x)", (Uint32)skb);
            return;
        }
    }
    else
    {
        SCBPRINTK("Cannot extract h_dest (skb 0x%x)", (Uint32)skb);
        return;
    }

    SCBPRINTK("Dest MAC: %pM", list_param.u.mac_addr);

    hash_LUT1 = 0x000000FF & pp_db_hash((Uint8*) & skb->pp_packet_info.pp_session.ingress.lookup.LUT1, sizeof(skb->pp_packet_info.pp_session.ingress.lookup.LUT1), 0, AVALANCHE_PP_MAX_LUT1_KEYS);
    hash_LUT2 = 0x000007FF & pp_db_hash((Uint8*) & skb->pp_packet_info.pp_session.ingress.lookup.LUT2, sizeof(skb->pp_packet_info.pp_session.ingress.lookup.LUT2), 0, AVALANCHE_PP_MAX_ACCELERATED_SESSIONS); 
    scb_hash  = hash_LUT2 ^ hash_LUT1;
    if(avalanche_pp_get_scb_entry(&scb_entry, scb_hash) != PP_RC_SUCCESS)
    {
        SCBPRINTK("Cannot get scb_entry index %d", scb_hash);
        return;
    }

    /* Zombie connection, do not bypass */
    if (scb_entry->scb_flags & SCB_ZOMBIE) 
    {
        SCBPRINTK("SCB %d is zombie (packets %d)", scb_hash, scb_entry->scb_packet_count);
        return;
    }

    /* Valid connection means this is not the 1st packet in this connection */
    if (scb_entry->scb_flags & SCB_VALID_CONNECTION) 
    {
        /* Conntrack attached with previous packet */
        if (scb_entry->scb_flags & SCB_CONNTRACK_ATTACHED) 
        {
            /* Threshold has not exceeded, bypass egress hook */
            if (scb_entry->scb_packet_count < global_hil_db.ddh_num_scb_packets_threshold) 
            {
                skb->pp_packet_info.flags |= TI_HIL_PACKET_FLAG_PP_SCB_VALID;
                skb->pp_packet_info.flags |= TI_HIL_PACKET_FLAG_PP_SCB_CONNTRACK_ATTACHED;
                SCBPRINTK("SCB %d Bypass egress hook (packets %d)", scb_hash, scb_entry->scb_packet_count);
                scb_entry->scb_packet_count++;
            }
            else
            {
                SCBPRINTK("SCB %d Threshold exceeded (packets %d)", scb_hash, scb_entry->scb_packet_count);
            }

            /* Always put SCB ID on skb */
            skb->pp_packet_info.pp_session.scb_id = scb_hash;
        }

        /* Connection still doesn't have conntrack attached */
        else
        {
            SCBPRINTK("SCB %d haven't attached conntrack (packets %d)", scb_hash, scb_entry->scb_packet_count);
            memset(scb_entry, 0, sizeof(AVALANCHE_PP_SCB_Entry_t));
            global_hil_db.ddh_active_short_connections--;
        }
    }

    /* 1st packet in this connection */
    else
    {
        memset(scb_entry, 0, sizeof(AVALANCHE_PP_SCB_Entry_t));
        skb->pp_packet_info.pp_session.scb_id = scb_hash;
        skb->pp_packet_info.flags |= TI_HIL_PACKET_FLAG_PP_SCB_VALID;
        scb_entry->scb_flags |= SCB_VALID_CONNECTION;
        scb_entry->scb_packet_count = 1;
        global_hil_db.ddh_active_short_connections++;
        SCBPRINTK("New scb_entry index %d", scb_hash);

        if (global_hil_db.ddh_active_short_connections > global_hil_db.ddh_max_active_short_connections)
        {
            global_hil_db.ddh_max_active_short_connections = global_hil_db.ddh_active_short_connections;
        }
    }
}

/**************************************************************************
 * FUNCTION NAME : void __hil_scb_remove(struct sk_buff* skb)
 **************************************************************************
 * DESCRIPTION   :
 *  This function removes a SCB connection
 *
 * return
 ***************************************************************************/
static void __hil_scb_remove(struct sk_buff* skb)
{
    AVALANCHE_PP_SCB_Entry_t *scb_entry;

    if(IS_HYBRID())
    {
        if(skb->pp_packet_info.pp_session.scb_id != AVALANCHE_PP_MAX_ACCELERATED_SESSIONS)
        {
            if(avalanche_pp_get_scb_entry(&scb_entry, skb->pp_packet_info.pp_session.scb_id) == PP_RC_SUCCESS)
            {
                /* If we have conntrack attached, turn connection to zombie */
                if (scb_entry->scb_flags & SCB_CONNTRACK_ATTACHED) 
                {
                    SCBPRINTK("Mark scb_entry index %d as zombie", skb->pp_packet_info.pp_session.scb_id);
                    scb_entry->scb_flags |= SCB_ZOMBIE;
                    global_hil_db.ddh_zombie_short_connections++;
                }

                /* If we do not have conntrack just remove this entry */
                else
                {
                    SCBPRINTK("Removing SCB %d that haven't attached conntrack (packets %d)", skb->pp_packet_info.pp_session.scb_id, scb_entry->scb_packet_count);
                    memset(scb_entry, 0, sizeof(AVALANCHE_PP_SCB_Entry_t));
                }

                global_hil_db.ddh_active_short_connections--;
            }
            else
            {
                SCBPRINTK("Cannot get scb_entry index %d", skb->pp_packet_info.pp_session.scb_id);
            }
        }
    }
}

/**************************************************************************
 * FUNCTION NAME : void __hil_scb_timeout(struct nf_conn* conntrack)
 **************************************************************************
 * DESCRIPTION   :
 *  This function removes a SCB connection
 *
 * return
 ***************************************************************************/
static void __hil_scb_timeout(struct nf_conn* conntrack)
{
    AVALANCHE_PP_SCB_Entry_t *scb_entry;

    if(IS_HYBRID())
    {
        /* If we decide to delete this conntrack we should also remove this SCB connection from our count */
        if (conntrack->ti_pp_scb_id != AVALANCHE_PP_MAX_ACCELERATED_SESSIONS) 
        {
            if (avalanche_pp_get_scb_entry(&scb_entry, conntrack->ti_pp_scb_id) == PP_RC_SUCCESS)
            {
                if (scb_entry->conntrack_handle == (Uint32)conntrack) 
                {
                    /* If this is a zombie connection just decrement counter */
                    if (scb_entry->scb_flags & SCB_ZOMBIE)
                    {
                        global_hil_db.ddh_zombie_short_connections--;
                        SCBPRINTK("Decrement zombie scb_entry index %d", conntrack->ti_pp_scb_id);
                    }

                    /* This is not a zombie connection meaning a session was never created */
                    else
                    {
                        global_hil_db.ddh_active_short_connections--;
                    }

                    SCBPRINTK("Remove scb_entry index %d (conntrack 0x%x)", conntrack->ti_pp_scb_id, (Uint32)conntrack);
                    memset(scb_entry, 0, sizeof(AVALANCHE_PP_SCB_Entry_t));
                }
            }
            else
            {
                SCBPRINTK("Cannot get scb_entry index %d", conntrack->ti_pp_scb_id);
            }
        }
    }
}

/**************************************************************************
 * FUNCTION NAME : Bool __hil_scb_is_egress_bypass(struct sk_buff* skb)
 **************************************************************************
 * DESCRIPTION   :
 *  This function decides if to bypass ingress according to SCB
 *
 * return - True if this packet should bypass egress hook
 ***************************************************************************/
static Bool __hil_scb_is_egress_bypass(struct sk_buff* skb)
{
    AVALANCHE_PP_SCB_Entry_t *scb_entry;

    if (IS_HYBRID())
    {
        /* Check if we need to bypass egress hook */
        if(skb->pp_packet_info.flags & TI_HIL_PACKET_FLAG_PP_SCB_VALID)
        {
            /* If both flags are set, bypass egress hook */
            if (skb->pp_packet_info.flags & TI_HIL_PACKET_FLAG_PP_SCB_CONNTRACK_ATTACHED) 
            {
                SCBPRINTK("Bypass egress hook");
                global_hil_db.num_egress_pkt_scb++;
                return True;
            }
            else
            {
                /* If conntrack is not set, we need to remove this SCB */
                if(avalanche_pp_get_scb_entry(&scb_entry, skb->pp_packet_info.pp_session.scb_id) == PP_RC_SUCCESS)
                {
                    /* Connection must be valid */
                    if (scb_entry->scb_flags & SCB_VALID_CONNECTION) 
                    {
                        /* Mark invalid scb id */
                        skb->pp_packet_info.pp_session.scb_id = AVALANCHE_PP_MAX_ACCELERATED_SESSIONS;
                        memset(scb_entry, 0, sizeof(AVALANCHE_PP_SCB_Entry_t));
                        global_hil_db.ddh_active_short_connections--;
                        SCBPRINTK("Removing scb_entry index %d since there is no conntrack attached", skb->pp_packet_info.pp_session.scb_id);
                    }
                    else
                    {
                        SCBPRINTK("Cannot remove scb_entry index %d becase entry is not valid", skb->pp_packet_info.pp_session.scb_id);
                    }
                }
                else
                {
                    SCBPRINTK("Cannot get scb_entry index %d", skb->pp_packet_info.pp_session.scb_id);
                }
            }
        }
    }

    return False;
}

/**************************************************************************
 * FUNCTION NAME : Bool __hil_scb_attach_conntrack(struct sk_buff* skb)
 **************************************************************************
 * DESCRIPTION   :
 *  This function attaches conntrack to the scb
 *
 * return
 ***************************************************************************/
static void __hil_scb_attach_conntrack(struct sk_buff* skb, struct nf_conn* conntrack)
{
    AVALANCHE_PP_SCB_Entry_t*   scb_entry;

    if (IS_HYBRID())
    {
        /* Handle short connection bypass */
        if ((skb->pp_packet_info.flags & TI_HIL_PACKET_FLAG_PP_SCB_VALID) && (conntrack != NULL))
        {
            if (avalanche_pp_get_scb_entry(&scb_entry, skb->pp_packet_info.pp_session.scb_id) == PP_RC_SUCCESS) 
            {
                if (scb_entry->scb_flags & SCB_VALID_CONNECTION) 
                {
                    /* If SCB entry does not contain conntrack handle */
                    if (scb_entry->conntrack_handle == 0) 
                    {
                        SCBPRINTK("Attaching conntrack (0x%x) to scb_entry index %d", (Uint32)conntrack, skb->pp_packet_info.pp_session.scb_id);
                    }

                    /* If SCB entry already contains conntrack handle */
                    else
                    {
                        /* Overwrite conntrack handle */
                        SCBPRINTK("Overwriting conntrack (0x%x) to scb_entry index %d", (Uint32)conntrack, skb->pp_packet_info.pp_session.scb_id);
                        global_hil_db.ddh_num_scb_hash_collisions++;
                    }

                    conntrack->ti_pp_scb_id = skb->pp_packet_info.pp_session.scb_id;
                    scb_entry->conntrack_handle = (Uint32)conntrack;
                    scb_entry->scb_flags |= SCB_CONNTRACK_ATTACHED;
                    skb->pp_packet_info.flags |= TI_HIL_PACKET_FLAG_PP_SCB_CONNTRACK_ATTACHED;
                }
                else
                {
                    SCBPRINTK("scb_entry index %d is not valid", skb->pp_packet_info.pp_session.scb_id);
                }
            }
            else
            {
                SCBPRINTK("Cannot get scb_entry index %d", skb->pp_packet_info.pp_session.scb_id);
            }
        }
    }
}


/**************************************************************************
 * FUNCTION NAME : void __hil_calc_bithash(struct sk_buff* skb)
 **************************************************************************
 * DESCRIPTION   :
 *  This function calcultes the opposite bithash of an US connection
 *
 * return
 ***************************************************************************/
static void __hil_calc_bithash(struct sk_buff* skb)
{
    Uint32 tuple_ports = 0;
    Uint32 tuple_hash = 0;
    Uint16 reduced_tuple_hash;
    Uint32 ip_addr_idx;
    avalanche_pp_local_dev_addr_ioctl_params_t gw_list;
    struct nf_conn* conntrack;

    conntrack = skb->nfct;
    if (conntrack == NULL)
    {
        return;
    }

    if (IS_GW() || IS_HYBRID())
    {
        /* Check if TI_PP_US_CONNECTION is set */
        if((conntrack->ti_pp_status_flag & TI_PP_US_CONNECTION) &&
           !(conntrack->ti_pp_status_flag & TI_PP_DS_RESPONSE_HASH_VALID) &&
           ((struct ethhdr*)skb_mac_header(skb) != NULL) &&
           (((struct ethhdr*)skb_mac_header(skb))->h_source != NULL))
        {
            gw_list.addr_type    = GW_MAC_ADDR;
            gw_list.op_type      = IS_ADDR_EXIST;
            memcpy((void *)(gw_list.u.mac_addr), (void*)(((struct ethhdr *)skb_mac_header(skb))->h_source), 6); 

            /* Check if gw mac is the src mac */
            if (avalanche_pp_local_dev_addr(&gw_list) != PP_RC_SUCCESS)
            {
                return;
            }

            /* Xor source & destination IP's */
            switch (conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.src.l3num)
            {
                /* IPv4 */
                case AF_INET:
                {
                    tuple_hash  = conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip ^ conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip;
                    break;
                }

                /* IPv6 */
                case AF_INET6:
                {
                    /* Source IPv6 */
                    tuple_hash = conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip6[0];
                    for (ip_addr_idx = 1 ; ip_addr_idx < 4 ; ip_addr_idx++)
                    {
                        tuple_hash ^= conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip6[ip_addr_idx];
                    }

                    /* Destination IPv6 */
                    for (ip_addr_idx = 0 ; ip_addr_idx < 4 ; ip_addr_idx++)
                    {
                        tuple_hash ^= conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip6[ip_addr_idx];
                    }
                    break;
                }

                default:
                {
                    BHPRINTK("Unknown L3 type: %d", conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.src.l3num);
                    return;
                }
            }

            /* Store ports in 32 bit */
            tuple_ports = (conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all << 16) | (conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all);

            /* Add ports to hash*/
            tuple_hash  ^= tuple_ports;

            /* Add protocol to hash */
            tuple_hash  ^= conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.dst.protonum;

            /* Reduce hash to 16 bit */
            reduced_tuple_hash = (Uint16)(tuple_hash >> 16) ^ ((Uint16)tuple_hash);

            /* Store hash in conntrack */
            conntrack->ti_pp_ds_response_hash = reduced_tuple_hash;

            /* Set hash valid flag in conntrack */
            conntrack->ti_pp_status_flag |= TI_PP_DS_RESPONSE_HASH_VALID;

            BHPRINTK("Adding bithash: 0x%x. src_ip: %pI4. dst_ip: %pI4. ports: 0x%x. Protoctol: %d",
                      reduced_tuple_hash,
                      &(conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip),
                      &(conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u3.ip),
                      tuple_ports,
                      conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.dst.protonum);

            avalanche_pp_update_bithash (reduced_tuple_hash, True);
        }
    }
}
#endif

/**************************************************************************
 * FUNCTION NAME : void __hil_ddh_reset_pps_counters
 **************************************************************************
 * DESCRIPTION   :
 *  This function resets all the pps related counters
 *
 * return - void
 ***************************************************************************/
static void __hil_ddh_reset_pps_counters()
{
    global_hil_db.ddh_cycle_duration_jiffies = 0;
    global_hil_db.ddh_last_cycle_host_packets = global_hil_db.ddh_host_forwarded_packets;
    global_hil_db.ddh_last_cycle_created_sessions = global_hil_db.num_total_sessions; 
    global_hil_db.ddh_host_forwarded_packets = 0;
}

/**************************************************************************
 * FUNCTION NAME : ti_hil_enable_6rd_pp
 **************************************************************************
 * DESCRIPTION   :
 *  Enable 6RD PP Support (Protocol 41) API 
 *
 * RETURNS       :
 *  0   -   Success
 *  <0  -   Error
 **************************************************************************/
int ti_hil_enable_6rd_pp (void)
{
    printk(KERN_DEBUG "%s: Start Enable 6RD PP Support\n", __FUNCTION__);

    global_hil_db.ipv6rd_pp_support = True;

    return 0;
}

/**************************************************************************
 * FUNCTION NAME : ti_hil_disable_6rd_pp
 **************************************************************************
 * DESCRIPTION   :
 *  Disable 6RD PP Support (Protocol 41) API 
 *
 * RETURNS       :
 *  0   -   Success
 *  <0  -   Error
 **************************************************************************/
int ti_hil_disable_6rd_pp (void)
{
    printk(KERN_DEBUG "%s: Start Disable PP 6RD Support\n", __FUNCTION__);

    global_hil_db.ipv6rd_pp_support = False;

    return 0;
}

EXPORT_SYMBOL(ti_hil_enable_6rd_pp);
EXPORT_SYMBOL(ti_hil_disable_6rd_pp);

