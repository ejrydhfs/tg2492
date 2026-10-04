/*
  GPL LICENSE SUMMARY

  Copyright(c) 2016 - 2017 Intel Corporation.

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

#ifndef PP_HAL_H
#define PP_HAL_H

#include <linux/kernel.h>
#include <asm-arm/arch-avalanche/generic/avalanche_pp_api.h>
#include <asm-arm/arch-avalanche/generic/avalanche_pdsp_api.h>
#include <asm-arm/arch-avalanche/puma6/puma6_cppi.h>

typedef struct
{
    Uint8   IngressVPID;
    Uint8   StatusFlags;
    Uint16  SessionFlags;

    Uint32  TimeoutThresh;
    Uint32  ReferenceTime;

    Uint8   SynchQ;
    Uint8   State;
    Uint16  Res1;
} 
PP_HAL_SESSION_BASE_REC_t;

typedef struct
{
    Uint8                   Priority;
    Uint8                   EgressVPID;
    Uint8                   FramingCode;
    Uint8                   NewHeaderSize;

    Uint16                  TxDestTag;
    Uint16                  TxQueueBase;

    Uint8                   EgressFlags;
    Uint8                   NextEgressRecIdx;
    Uint8                   EgressPidType;
    Uint8                   UsPayloadLenOff;

    Uint32                  UsTurboDoxAck;

    AVALANCHE_PP_PSI_t      psi;
} 
PP_HAL_SESSION_EGRESS_REC_t;

typedef struct
{
    Uint16  L4ChecksumDelta;
    Uint16  ModFlags;

    Uint32  IpSrc;
    Uint32  IpDst;

    Uint16  PortSrc;
    Uint16  PortDst;
    Uint16  L3ChecksumDelta;
    Uint8   Tos;
    Uint8   Res1;
} 
PP_HAL_MODIFICATION_REC_t;

typedef struct
{
    Uint16  L4ChecksumDelta;
    Uint16  ModFlags;
    Uint32  Ipv6Addr[4];    
} 
PP_HAL_MODIFICATION_REC_V6_t;

/* ======================================================== */

typedef struct
{   
    PP_HAL_SESSION_BASE_REC_t       BaseRecord;
    PP_HAL_SESSION_EGRESS_REC_t     EgressRecord;
    PP_HAL_MODIFICATION_REC_t       ModificationRecord;

    Uint8                           NewHeader[24];
} 
PP_HAL_SESSION_INFO_t;

/* ======================================================== */

typedef struct
{   
    PP_HAL_SESSION_EGRESS_REC_t     EgressRecord;
    PP_HAL_MODIFICATION_REC_t       ModificationRecord;

    Uint8                           NewHeader[24];
}
PP_HAL_SESSION_INFO_MULTICAST_t;

/* ======================================================== */

AVALANCHE_PP_RET_e      pp_hal_init(void);

AVALANCHE_PP_RET_e      pp_hal_pid_create(AVALANCHE_PP_PID_t *ptr_pid);
AVALANCHE_PP_RET_e      pp_hal_pid_delete(Uint8 pid_handle);
AVALANCHE_PP_RET_e      pp_hal_pid_range_create(AVALANCHE_PP_PID_RANGE_t *ptr_pid_range_cfg);
AVALANCHE_PP_RET_e      pp_hal_pid_range_delete(Uint8 port);
AVALANCHE_PP_RET_e      pp_hal_pid_flags_set(AVALANCHE_PP_PID_t *ptr_pid);

AVALANCHE_PP_RET_e      pp_hal_vpid_create(AVALANCHE_PP_VPID_INFO_t *ptr_vpid);
AVALANCHE_PP_RET_e      pp_hal_vpid_delete(Uint8 vpid_handle);
AVALANCHE_PP_RET_e      pp_hal_vpid_flags_set(AVALANCHE_PP_VPID_INFO_t *ptr_vpid);

AVALANCHE_PP_RET_e      pp_hal_session_create(AVALANCHE_PP_SESSION_INFO_t *session_cfg, Bool create_LUT1, __Avalanche_PP_LUTs_Data_t *full_classification_lookup_ptr);
AVALANCHE_PP_RET_e      pp_hal_session_delete(AVALANCHE_PP_SESSION_INFO_t *session_cfg, Bool delete_LUT1);

AVALANCHE_PP_RET_e      pp_hal_session_tdox_update(AVALANCHE_PP_SESSION_INFO_t *session_cfg);
AVALANCHE_PP_RET_e      pp_hal_session_tdox_get(Uint32 session_handle, Bool *enabled);
AVALANCHE_PP_RET_e      pp_hal_version_get(AVALANCHE_PP_VERSION_t *version);
AVALANCHE_PP_RET_e      pp_hal_session_support_multi_drop(Bool support_multi_drop);
AVALANCHE_PP_RET_e      pp_hal_session_support_smart_prioritization(Bool support_smart_prioritization);
AVALANCHE_PP_RET_e      pp_hal_display_session_extended_info(Uint32 session_handle, Int8 *buffer, Int32 *size);
AVALANCHE_PP_RET_e      pp_hal_display_qos_queue_info(Uint32 queue_id, Int8 *buffer, Int32 *size);
AVALANCHE_PP_RET_e      pp_hal_display_qos_cluster_info(Uint32 session_handle, Int8 *buffer, Int32 *size);
AVALANCHE_PP_RET_e      pp_hal_set_host_qos_cluster(void);
AVALANCHE_PP_RET_e      pp_hal_support_l2_classification(Uint8 classType, Uint8 pidNum);
AVALANCHE_PP_RET_e      pp_hal_update_bithash(Uint16 bit_index, Bool add_operation);
AVALANCHE_PP_RET_e      pp_hal_flush_bithash();
/* ======================================================== */

typedef struct
{
    Uint16 egr_q;                    /* The Queue index of the forwarding queue */
    Uint8  flags;                    /* Specifies how the frames in the QOS queue should be handled */
    Uint8  reserved;

    Uint32 iteration_credit_bytes;   /* The amount of byte credit that the queue receives every 25us. */
    Uint32 total_credit_bytes;       /* The total amount of forwarding byte credit that the queue is currently holding */
    Uint32 max_credit_bytes;         /* The max amount of forwarding byte crerdit that the queue is allowed to hold at the end of the 25US iteration */

    Uint16 iteration_credit_pkts;    /* The amount of packet credit that the queue receives every 25us. */
    Uint16 total_credit_pkts;        /* The total amount of forwarding pkt credit that the queue is currently holding */
   
    Uint16 max_credit_pkts;          /* The max amount of forwarding packet crerdit that the queue is allowed to hold at the end of the 25US iteration */
    Uint16 congst_thrsh_pkts;        /* The size in packets at which point the Qos queue is considered to be congested */ 

    Uint32 congst_thrsh_bytes;       /* The size in bytes at which point the Qos queue is considered to be congested */
    Uint32 w7;                       /* Reserved */

}
PP_HAL_QOS_QUEUE_t;


typedef struct
{
    Uint32                      global_credit_bytes;        /* The amount of global credit bytes available to the next Qos queue in the cluster */

    Uint16                      global_credit_pkts;         /* The amount of global credit packet available to the next Qos queue in the cluster */
    Uint16                      max_global_credit_pkts;     /* The max amount of global credit pkts allowed carring over to the next queue. */
      
    Uint32                      max_global_credit_bytes;    /* The max amount of global credit bytes allowed carring over to the next queue. */

    Uint8                       flags;                      /* Cluster flags */
    Uint8                       it_divider;                 /* Iteration divider for slow rate credits */

    Uint8                       egr_q_cnt;                  /* The total number of egress queues sampled to obtain the egress queue congestion estimation */
#define rrFirstBitmask          egr_q_cnt                   /* For GRR groups cluster, This fiels must be on cluster top FW structure */

    Uint8                       qos_q_cnt;                  /* The number of QOS queues in the cluster (1 to 9) */

    Uint8                       qos_q3;                     /* The queue index (0 to 127) of each QOS queue in the cluster listed in priority order */
    Uint8                       qos_q2;
    Uint8                       qos_q1;
    Uint8                       qos_q0;

    Uint8                       qos_q7;
    Uint8                       qos_q6;
    Uint8                       qos_q5;
    Uint8                       qos_q4; 

    Uint16                      egr_q1;                     /* The Queue index of every egress queue enumerated in Egress Queue Count */
    Uint16                      egr_q0;

    Uint16                      egr_q3;
    Uint16                      egr_q2;

    Uint32                      egr_congst_thrsh_bytes1;	/* Egress Congestion Threshold bytes point 1  */
    Uint32                      egr_congst_thrsh_bytes2;	/* Egress Congestion Threshold bytes point 2  */
    Uint32                      egr_congst_thrsh_bytes3;    /* Egress Congestion Threshold bytes point 3  */
    Uint32                      egr_congst_thrsh_bytes4;	/* Egress Congestion Threshold bytes point 4  */
           
    Uint16                      egr_congst_thrsh_pkts1;     /* Egress Congestion Threshold Packets point 4  */
    Uint16                      egr_congst_thrsh_pkts2;     /* Egress Congestion Threshold Packets point 3  */
    Uint16                      egr_congst_thrsh_pkts3;     /* Egress Congestion Threshold Packets point 2  */
    Uint16                      egr_congst_thrsh_pkts4;     /* Egress Congestion Threshold Packets point 1  */

    Uint32                      w14;                        /* Reserved */
    Uint32                      w15;                        /* Reserved */
}
PP_HAL_QOS_CLST_CFG_t;

AVALANCHE_PP_RET_e pp_hal_qos_cluster_config_set(Uint8 clst_indx, AVALANCHE_PP_QOS_CLST_CFG_t *clst_cfg, Uint16 *egr_queues, Uint8 egr_qcount);
AVALANCHE_PP_RET_e pp_hal_qos_cluster_config_get(Uint8 clst_indx, AVALANCHE_PP_QOS_CLST_CFG_t *clst_cfg, Uint16 *egr_queues, Uint8 *egr_qcount);

AVALANCHE_PP_RET_e pp_hal_qos_queue_config_set(AVALANCHE_PP_QOS_QUEUE_t *qos_q_cfg);
AVALANCHE_PP_RET_e pp_hal_qos_queue_config_get(AVALANCHE_PP_QOS_QUEUE_t *qos_q_cfg);

AVALANCHE_PP_RET_e pp_hal_qos_cluster_enable(Uint8 clust_index);
AVALANCHE_PP_RET_e pp_hal_qos_cluster_disable(Uint8 clust_index);

AVALANCHE_PP_RET_e pp_hal_set_traffic_prioritization_mode(Uint8 priorityMode);

AVALANCHE_PP_RET_e pp_hal_set_pp_addr_list(avalanche_pp_local_addr_type_e addr_type, Uint8 *addr_ptr, Uint8 addr_index, Uint8 list_counter);

#define PP_HAL_MAX_PKT_SIZE                            2048
/* Host QoS Cluster */
/* Queues */
#define PP_HAL_DOCSIS_LOW_HOST_QOS_QUEUE               (PAL_CPPI41_SR_CLUSTER19_HOST_DOCSIS_LOW_QPDSP_QOS_Q_NUM - PAL_CPPI41_SR_QPDSP_QOS_Q_BASE)
#define PP_HAL_DOCSIS_MED_LOW_HOST_QOS_QUEUE           (PAL_CPPI41_SR_CLUSTER19_HOST_DOCSIS_MED_LOW_QPDSP_QOS_Q_NUM - PAL_CPPI41_SR_QPDSP_QOS_Q_BASE)
#define PP_HAL_DOCSIS_MED_HOST_QOS_QUEUE               (PAL_CPPI41_SR_CLUSTER19_HOST_DOCSIS_MED_QPDSP_QOS_Q_NUM - PAL_CPPI41_SR_QPDSP_QOS_Q_BASE)
#define PP_HAL_DOCSIS_MED_HIGH_HOST_QOS_QUEUE          (PAL_CPPI41_SR_CLUSTER19_HOST_DOCSIS_MED_HIGH_QPDSP_QOS_Q_NUM - PAL_CPPI41_SR_QPDSP_QOS_Q_BASE)
#define PP_HAL_DOCSIS_HIGH_HOST_QOS_QUEUE              (PAL_CPPI41_SR_CLUSTER19_HOST_DOCSIS_HIGH_QPDSP_QOS_Q_NUM - PAL_CPPI41_SR_QPDSP_QOS_Q_BASE)
#define PP_HAL_L2SW_LOW_HOST_QOS_QUEUE                 (PAL_CPPI41_SR_CLUSTER19_HOST_L2SW_LOW_QPDSP_QOS_Q_NUM - PAL_CPPI41_SR_QPDSP_QOS_Q_BASE)
#define PP_HAL_L2SW_MED_HOST_QOS_QUEUE                 (PAL_CPPI41_SR_CLUSTER19_HOST_L2SW_MED_QPDSP_QOS_Q_NUM - PAL_CPPI41_SR_QPDSP_QOS_Q_BASE)
#define PP_HAL_L2SW_HIGH_HOST_QOS_QUEUE                (PAL_CPPI41_SR_CLUSTER19_HOST_L2SW_HIGH_QPDSP_QOS_Q_NUM - PAL_CPPI41_SR_QPDSP_QOS_Q_BASE)
/* Bitmask of 0x57 means Q2-Q3 in RR group, Q4-Q5 in RR group, Q6-Q7 in RR group */
#define HOST_CLUSTER_RR_BITMASK                        0x57
/* 7 for 7K PPS */
#define HOST_MAX_PPS_CREDIT                            7
#define HOST_ETH_MAX_QUEUE_LEN_PKTS                    100
#define HOST_ETH_MAX_QUEUE_LEN_BYTES                   (HOST_ETH_MAX_QUEUE_LEN_PKTS * PP_HAL_MAX_PKT_SIZE)
#define HOST_DOCSIS_MAX_QUEUE_LEN_PKTS                 100
#define HOST_DOCSIS_MAX_QUEUE_LEN_BYTES                (HOST_DOCSIS_MAX_QUEUE_LEN_PKTS * PP_HAL_MAX_PKT_SIZE)
#define MAX_CREDIT_BYTES                               0xFFFF
/* Dividing 25uSec by 40 for 1mSec */
#define IT_DIVIDER_FOR_1MILI_SEC                       40
/* 1 for 1K PPS */
#define Q_LIMIT_1K_PPS_PKT_CREDIT                      1
#define Q_LIMIT_1K_PPS_BYTS_CREDIT                     (Q_LIMIT_1K_PPS_PKT_CREDIT * PP_HAL_MAX_PKT_SIZE)

#define PP_HAL_PRIORITY_3BIT_MASK                      0x07
#define PP_HAL_PRIORITY_TOS_PRIO_SHIFT                 5
#define PP_HAL_MAPPING_DIVIDER_FOR_2_PRIORITIES_LEVELS 4
#define PP_HAL_MAPPING_DIVIDER_FOR_4_PRIORITIES_LEVELS 2
#define PP_HAL_MAX_PRIORITIES_LEVELS                   4
#define PP_HAL_PRIORITY_TOS_VLAN                       0
#define PP_HAL_PRIORITY_TOS_ONLY                       1
#define PP_HAL_PRIORITY_VLAN_ONLY                      2
#define PP_HAL_PRIORITY_DISABLE_PRIORITIZATION         3
#define PP_HAL_PRIORITY_INVALID_VALUE                  0xFF

#define PP_HAL_EVENTS_BASE_PHY                         (0x03200000)
#define PP_HAL_EVENTS_PP_INDEX_BASE_PHY                (0x03208004)
#define PP_HAL_EVENTS_HOST_INDEX_BASE_PHY              (0x03208008)

#define PP_HAL_EVENTS_NUM                              2048        /* Up to 2048 events */
#define PP_HAL_EVENTS_ENTRY_SIZE_BYTES                 2           /* Event entry size is 2 bytes */
#define PP_HAL_EVENTS_ENTRY_SIZE_BITS                  (PP_EVENTS_ENTRY_SIZE_BYTES * 8)
#define PP_HAL_EVENT_ID_SHIFT                          12
#define PP_HAL_EVENT_ID_MASK                           0xF
#define PP_HAL_EVENT_DATA_SHIFT                        0
#define PP_HAL_EVENT_DATA_MASK                         0xFFF
#define PP_HAL_BITHASH_ADDR                            (0x03408814)

// Reassembly DB Timeout Threshold. Used to change the Reassembly DB Timeout Threshold from the proc file
#define PP_HAL_REASSEMBLY_DB_TIMEOUT_THRESHOLD_ADD     (0x0332C0F0)
#define PP_HAL_REASSEMBLY_DB_TIMEOUT_THRESHOLD_MASK    (0xFFFFFF00)

#define PP_HAL_MTA_MAC_BASE_PHY                        (0x0340C900)
#define PP_HAL_WAN_MAC_BASE_PHY                        (0x0340C908)
#define PP_HAL_GW_MAC_BASE_PHY                         (0x0340C910)
#define PP_HAL_LAN0_MAC_BASE_PHY                       (0x0340C918)

#define PP_HAL_RND_MAC_VALID_CNT_BASE_PHY              (0x0340C920)
#define PP_HAL_RND_MAC_BASE_PHY                        (0x0340C924)

#define PP_HAL_MTA_RTP_V4_VALID_CNT_BASE_PHY           (0x0340C944)
#define PP_HAL_MTA_MNG_V4_VALID_CNT_BASE_PHY           (0x0340C945)
#define PP_HAL_MTA_RTP_V6_VALID_CNT_BASE_PHY           (0x0340C946)
#define PP_HAL_MTA_MNG_V6_VALID_CNT_BASE_PHY           (0x0340C947)
#define PP_HAL_MTA_RTP_V4_IPS_WHITE_LIST_BASE_PHY      (0x0340C948)
#define PP_HAL_MTA_MNG_V4_IPS_WHITE_LIST_BASE_PHY      (0x0340C958)
#define PP_HAL_MTA_RTP_V6_IPS_WHITE_LIST_BASE_PHY      (0x0340C960)
#define PP_HAL_MTA_MNG_V6_IPS_WHITE_LIST_BASE_PHY      (0x0340C9A0)
#define PP_HAL_MDROP_V4_VALID_CNT_BASE_PHY             (0x0340C9C0)
#define PP_HAL_MDROP_V6_VALID_CNT_BASE_PHY             (0x0340C9C1)
#define PP_HAL_MDROP_V4_IPS_LIST_BASE_PHY              (0x0340C9C4)
#define PP_HAL_MDROP_V6_IPS_LIST_BASE_PHY              (0x0340C9E0)

#define PP_HAL_STC_MSB_TBL_BASE_PHY                    (0x0340A850)

#define PP_HAL_COUNTERS_SESSION_BYTES_BASE_PHY         (0x03588400)
#define PP_HAL_COUNTERS_VPID_64BITS_BASE_PHY           (0x03588000)
#define PP_HAL_COUNTERS_VPID_32BITS_BASE_PHY           (0x0358D000)
#define PP_HAL_COUNTERS_SESSION_PKTS_BASE_PHY          (0x0358D400)
#define PP_HAL_COUNTERS_PPDSP_BASE_PHY                 (0x0358F520)       /* This is actually UsPrefPDSP */
#define PP_HAL_COUNTERS_CPDSP1_BASE_PHY                (0x0358F480)
#define PP_HAL_COUNTERS_CPDSP2_BASE_PHY                (0x0358F500)
#define PP_HAL_COUNTERS_MPDSP_BASE_PHY                 (0x0358F580)
#define PP_HAL_COUNTERS_QPDSP_BASE_PHY                 (0x03587600)        /* Increment by 1 */
#define PP_HAL_COUNTERS_PRXPDSP_BASE_PHY               (0x03587400)
#define PP_HAL_COUNTERS_X_QPDSP_BASE_PHY               (0x0358F600)        /* Increment by value */


// QPDSP 32bits counters offsets (form PP_HAL_COUNTERS_QPDSP_BASE_PHY + (QosQId*0x8) )
#define PP_COUNTERS_QPDSP_Q_OFF                        0x08
#define PP_COUNTERS_QPDSP_Q_PKT_FRWRD_OFF              0x00
#define PP_COUNTERS_QPDSP_Q_PKT_DROP_OFF               0x04

// VPID 32bits counters offsets (from PP_HAL_COUNTERS_VPID_32BITS_BASE_PHY)
#define PP_COUNTERS_VPID_32BITS_RX_BCAST_PKTS_OFF      0x00
#define PP_COUNTERS_VPID_32BITS_RX_MCAST_PKTS_OFF      0x04
#define PP_COUNTERS_VPID_32BITS_RX_DISCARDS_OFF        0x08
#define PP_COUNTERS_VPID_32BITS_TX_BCAST_PKTS_OFF      0x0C
#define PP_COUNTERS_VPID_32BITS_TX_MCAST_PKTS_OFF      0x10
#define PP_COUNTERS_VPID_32BITS_TX_DISCARDS_OFF        0x14
#define PP_COUNTERS_VPID_32BITS_TX_ERRORS_OFF          0x18
#define PP_COUNTERS_VPID_32BITS_ENTRY_SIZE             0x20

// VPID 64bits counters offsets (from PP_HAL_COUNTERS_VPID_64BITS_BASE_PHY)
#define PP_COUNTERS_VPID_64BITS_RX_BYTES_LSB_OFF       0x00
#define PP_COUNTERS_VPID_64BITS_RX_BYTES_MSB_OFF       0x04
#define PP_COUNTERS_VPID_64BITS_RX_UCAST_PKTS_LSB_OFF  0x08
#define PP_COUNTERS_VPID_64BITS_RX_UCAST_PKTS_MSB_OFF  0x0C
#define PP_COUNTERS_VPID_64BITS_TX_BYTES_LSB_OFF       0x10
#define PP_COUNTERS_VPID_64BITS_TX_BYTES_MSB_OFF       0x14
#define PP_COUNTERS_VPID_64BITS_TX_UCAST_PKTS_LSB_OFF  0x18
#define PP_COUNTERS_VPID_64BITS_TX_UCAST_PKTS_MSB_OFF  0x1C
#define PP_COUNTERS_VPID_64BITS_ENTRY_SIZE             0x20

// PPDSP 32bits counters offsets (from PP_COUNTERS_PPDSP_BASE_VIRT)
#define PP_COUNTERS_PPDSP_RX_PKTS_OFF                  0x00
#define PP_COUNTERS_PPDSP_PKTS_FRWRD_TO_CPDSP1_OFF     0x04
#define PP_COUNTERS_PPDSP_NOT_ENOUGH_DESCRIPTORS_OFF   0x08

// CPDSP1 32bits counters offsets (from PP_HAL_COUNTERS_CPDSP1_BASE_PHY)
#define PP_COUNTERS_CPDSP1_RX_PKTS_OFF                 0x00
#define PP_COUNTERS_CPDSP1_LUT1_SEARCHES_OFF           0x04
#define PP_COUNTERS_CPDSP1_LUT1_MATCHES_OFF            0x08
#define PP_COUNTERS_CPDSP1_PKTS_FRWRD_TO_CPDSP2_OFF    0x0C

// CPDSP2 32bits counters offsets (from PP_HAL_COUNTERS_CPDSP2_BASE_PHY)
#define PP_COUNTERS_CPDSP2_RX_PKTS_OFF                 0x00
#define PP_COUNTERS_CPDSP2_LUT2_SEARCHES_OFF           0x04
#define PP_COUNTERS_CPDSP2_LUT2_MATCHES_OFF            0x08
#define PP_COUNTERS_CPDSP2_PKTS_FRWRD_TO_MPDSP_OFF     0x0C
#define PP_COUNTERS_CPDSP2_SYNCH_TIMEOUT_EVENTS_OFF    0x10
#define PP_COUNTERS_CPDSP2_REASSEMBLY_DB_FULL          0x14
#define PP_COUNTERS_CPDSP2_REASSEMBLY_DB_TIMEOUT       0x18
#define PP_COUNTERS_QPDSP_OOO_DISCARDS_OFF             0x1C

// MPDSP 32bits counters offsets (from PP_HAL_COUNTERS_MPDSP_BASE_PHY)
#define PP_COUNTERS_MPDSP_PKTS_RECEIVED                0x00
#define PP_COUNTERS_MPDSP_IPV4_PKTS_RECEIVED           0x04
#define PP_COUNTERS_MPDSP_IPV6_PKTS_RECEIVED           0x08
#define PP_COUNTERS_MPDSP_PKTS_FRWRD_TO_HOST           0x0C
#define PP_COUNTERS_MPDSP_PKTS_FRWRD_TO_QPDSP          0x10
#define PP_COUNTERS_MPDSP_PKTS_FRWRD_TO_SYNCHQ         0x14
#define PP_COUNTERS_MPDSP_DISCARDS                     0x18
#define PP_COUNTERS_MPDSP_SYNCH_OVERFLOW_EVENTS        0x1C

// PrxPDSP 32bits counters offsets (from PP_HAL_COUNTERS_PRXPDSP_BASE_PHY)
#define PP_COUNTERS_PRXPDSP_PKTS_POPPED_FROM_IN_QUEUES  0x20
#define PP_COUNTERS_PRXPDSP_PKTS_FORWARDED_TO_L2_SWITCH 0x24
#define PP_COUNTERS_PRXPDSP_PKTS_FROM_L2_SWITCH         0x28
#define PP_COUNTERS_PRXPDSP_PKTS_PUSHED_TO_PREFETCHER   0x2C
#define PP_COUNTERS_PRXPDSP_NOT_ENOUGH_BUFFERS          0x30
#define PP_COUNTERS_PRXPDSP_NOT_ENOUGH_DESCRIPTORS      0x34
#define PP_COUNTERS_PRXPDSP_PKTS_TO_SMALL_FOR_PADDING   0x38

extern volatile Uint32 gPpHalDsLiteUsFragIPv4;

#endif // PP_HAL_H

