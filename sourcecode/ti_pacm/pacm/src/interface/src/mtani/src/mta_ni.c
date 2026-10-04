/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2009-2019 Intel Corporation.

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

  BSD LICENSE

  Copyright(c) 2009-2019 Intel Corporation. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.

    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  mta_ni.c
  Description:
  Implementation of the MTA network device

*/

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/in.h>
#include <linux/ipv6.h>
#include <net/ip.h>
#include <linux/timer.h>

#include "mta_ni.h"

#if defined( CONFIG_INTEL_KERNEL_DOCSIS_SUPPORT )
#include "dbridge_esafe.h"
#endif

#define MGCP_SIGNALING_PORT 2427
#define SIP_SIGNALING_PORT 5060
#define RATE_LIMIT_UNIQUE_IP 280
#define MAX_PACKET_PER_SEC 100

/* Define this for debug messages */
#undef MTANI_DEBUG

/* This is the private data the MTA interface holds */
typedef struct mtaPrivate_st
{
    /* Keep track of the statistics. */
    struct net_device_stats     stats;

    /* Pointer to the network device. */
    struct net_device*   dev;

}mtaPrivate_t;

typedef struct mtaRateLimit_st
{
    struct in6_addr ipv6Header[RATE_LIMIT_UNIQUE_IP];
    __u32           ipv4Header[RATE_LIMIT_UNIQUE_IP];
    int             ipv6Rate[RATE_LIMIT_UNIQUE_IP];
    int             ipv4Rate[RATE_LIMIT_UNIQUE_IP];
    int             ipv6Root;
    int             ipv4Root;
}mtaRateLimit_t;

struct timer_list       rateLimitTimer;           /* Timer for free queue operation */
 
#ifdef CONFIG_INTEL_NS_DEVICE_FILTER
extern int intel_ipv6_ns_filter(struct net_device *dev,struct in6_addr* dst_addr,unsigned char banned_flags);
extern int intel_register_ns_handler (struct net_device* dev, int (*ns_handler)(struct net_device *dev,struct in6_addr* dst_addr,unsigned char banned_flags));
#endif

#ifdef CONFIG_TI_DEVICE_PROTOCOL_HANDLING
extern int ti_protocol_handler (struct net_device* dev, struct sk_buff *skb);
#endif

#ifdef CONFIG_TI_PACKET_PROCESSOR
extern int ti_hil_pp_event(unsigned long val, void *v);
#endif

static unsigned short voice_port[ CONFIG_TI_PACM_MAX_NUMBER_OF_ENDPOINTS * 2 ];
static unsigned int voice_ports_active = 0;
static unsigned short voice_drop_ports[2] = {0,0};
static unsigned char drop_non_bootp = 0;
static mtaRateLimit_t mtaRateLimitDataBase;
static bool cleanMtaRateLimitTable;

/**************************************************************************/
/*      LOCAL FUNCTIONS DECLERATIONS:                                     */
/**************************************************************************/
static int mta_open (struct net_device *dev);
static int mta_release (struct net_device *dev);
static int mta_tx (struct sk_buff *skb, struct net_device *dev);
static struct net_device_stats *mta_get_net_stats(struct net_device *dev);
static int mta_set_mac_addr(struct net_device *dev, void *addr);
static int mta_set_mac_addr(struct net_device *dev, void *addr);
static void mta_init (struct net_device *dev);
static void dropPacket(struct sk_buff *skb);


/**************************************************************************/
/*      LOCAL FUNCTIONS:                                                  */
/**************************************************************************/
static int mta_open (struct net_device *dev)
{
    /* Start queue */
    netif_start_queue (dev);
    return 0;
}

static int mta_release (struct net_device *dev)
{
    /* Stop queue */
    netif_stop_queue(dev);
    return 0;
}

static inline int mta_check_udp_port(struct sk_buff *skb, int source, bool *isSignalingPort, bool *isVoicePort )
{
    int i=0;
    unsigned short int port;

    *isSignalingPort = false;
    *isVoicePort = false;

    /* Check that this is an ethernet packet */
    if((skb->protocol != ntohs(ETH_P_IP)) && (skb->protocol != ntohs(ETH_P_IPV6)))
        return 0;

    if( !(skb_network_header(skb)) )
    {
        if( !(skb_mac_header(skb)) )
            return 0;

        /* In case the pointers are not initialized */
        skb_set_network_header(skb, ETH_HLEN);

        if(skb->protocol == ntohs(ETH_P_IP))
        {
            skb_set_transport_header(skb, (ETH_HLEN + sizeof(struct iphdr)));
        }
        else
        {
            skb_set_transport_header(skb, (ETH_HLEN + sizeof(struct ipv6hdr)));
        }
    }

    /* Continue only if its UDP */
    if(skb->protocol == ntohs(ETH_P_IP))
    {
        if( ((struct iphdr*)(skb_network_header(skb)))->protocol != IPPROTO_UDP )
        {
            return 0;
        }
    }
    else
    {
        if (((struct ipv6hdr*)(skb_network_header(skb)))->nexthdr != IPPROTO_UDP)
        {
            return 0;
        }
    }

    if(source)
        port = ntohs(((struct udphdr*)(skb_transport_header(skb)))->source);
    else
        port = ntohs(((struct udphdr*)(skb_transport_header(skb)))->dest);

    if ((MGCP_SIGNALING_PORT == port) || (SIP_SIGNALING_PORT == port))
    {
        *isSignalingPort = true;
        #ifdef MTANI_DEBUG
            printk( "mtani: found signaling packet with port %d\n", port );
        #endif
    }

    for (i = 0; i < voice_ports_active; i++)
    {
        /* Look for port match */
        if( port == voice_port[i])
        {
#ifdef MTANI_DEBUG
            printk( "mtani: found voice packet with port %d\n", port );
#endif
            *isVoicePort = true;
            return 1;
        }
    }

#ifdef MTANI_DEBUG
    printk( "*isSignalingPort = %d, isVoicePort = %d\n", (*isSignalingPort), (*isVoicePort));
#endif

    return ((*isSignalingPort) || (*isVoicePort));
}

static inline int mta_is_packet_to_drop(struct sk_buff *skb, int source )
{
    unsigned short int src_port;
    unsigned short int dst_port;

    /* Check that this is an ethernet packet */
    if(skb->protocol != ntohs(ETH_P_IP) )
        return 0;

    if( !(skb_network_header(skb)) )
    {
        if( !(skb_mac_header(skb)) )
            return 0;

        /* In case the pointers are not initialized */
        skb_set_network_header(skb, ETH_HLEN);
    }

    /* Continue only if its UDP */
    if( ((struct iphdr*)(skb_network_header(skb)))->protocol != IPPROTO_UDP )
        return 0;

    if( !(skb_transport_header(skb)) )
    {
        if( !(skb_mac_header(skb)) )
            return 0;

        /* In case the pointers are not initialized */
        skb_set_transport_header(skb, 0x22);
    }

//ARRS ADD START 
    //to ignore the packet which does not have udp header for PROD00200760
    if(((struct iphdr*)(skb_network_header(skb)))->frag_off & htons(IP_OFFSET))
    {
        return 0;
    }
//ARRS ADD END


    /* set src and dst ports accorting to incoming/outgoing */
    if (source)
    {
        src_port = ntohs(((struct udphdr*)(skb_transport_header(skb)))->source);
        dst_port = ntohs(((struct udphdr*)(skb_transport_header(skb)))->dest);
    }
    else
    {
        src_port = ntohs(((struct udphdr*)(skb_transport_header(skb)))->dest);
        dst_port = ntohs(((struct udphdr*)(skb_transport_header(skb)))->source);
    }
#ifdef MTANI_DEBUG
    printk( "\nmtani: drop inspection: src=%u, dest=%u\n VOICE_DROP_PORT_SRC=%u, VOICE_DROP_PORT_DST=%u\n",
            src_port, dst_port,
            voice_drop_ports[VOICE_DROP_PORT_SRC],
            voice_drop_ports[VOICE_DROP_PORT_DST]);
#endif

    if ((src_port == voice_drop_ports[VOICE_DROP_PORT_SRC]) &&
        (dst_port == voice_drop_ports[VOICE_DROP_PORT_DST]))
    {
        return 1;
    }
    return 0;
}

static inline int mta_is_not_bootp_packet(struct sk_buff *skb)
{
    unsigned short int port;

    if((skb->protocol != ntohs(ETH_P_IP)) && (skb->protocol != ntohs(ETH_P_IPV6)) )
    {
        return 1;
    }

    if( !(skb_network_header(skb)) )
    {
        if( !(skb_mac_header(skb)) )
        {
            return 1;
        }

        /* In case the pointers are not initialized */
        skb_set_network_header(skb, ETH_HLEN);

        if(skb->protocol == ntohs(ETH_P_IP))
        {
            skb_set_transport_header(skb, (ETH_HLEN + sizeof(struct iphdr)));
        }
        else
        {
            skb_set_transport_header(skb, (ETH_HLEN + sizeof(struct ipv6hdr)));           
        }       
    }

    /* Continue only if its UDP */
    if(skb->protocol == ntohs(ETH_P_IP))
    {
        if( ((struct iphdr*)(skb_network_header(skb)))->protocol != IPPROTO_UDP )
        {
            return 1;
        }
    }
    else
    {
        if (((struct ipv6hdr*)(skb_network_header(skb)))->nexthdr != IPPROTO_UDP)
        {
            return 1;
        }
    }

    port = ntohs(((struct udphdr*)(skb_transport_header(skb)))->dest);
    if(skb->protocol == ntohs(ETH_P_IP))
    {
        if ((port != 67) && (port != 68))
        {          
            return 1;
        }
        else
        {
#ifdef MTANI_DEBUG
            printk( "mtani: found packet with port %d\n", port );
#endif
        }
    }
    else
    {
        if ((port != 546) && (port != 547))
        {          
            return 1;
        }
        else
        {
#ifdef MTANI_DEBUG
            printk( "mtani: found packet with port %d\n", port );
#endif
        }
    }

    return 0;
}

/**************************************************************************/
/*! \fn         mtaRateLimit_init
 **************************************************************************
 *
 *  \brief      Call back function which will be used to indicate that the MTA
 *  rate limit table should be cleared, used in order to avoid using Mutexes and race conditions.
 *
 *  \param[in]  param - Params which are unused but must be passed according to function description.
 *  *  \param[out] cleanMtaRateLimitTable - Global parameter which will inidcate that the table should be cleared.
 *  \return     No return
 **************************************************************************/
static void mtaRateLimit_init()
{
    cleanMtaRateLimitTable = true;
}

int mta_rx (struct sk_buff *skb)
{
    mtaPrivate_t *mtaPrivate;
    struct ethhdr*  ptr_ethhdr;
    bool signalingPort;
    bool voicePort;
    struct ipv6hdr *ipv6Header;
    struct iphdr *ipv4Header;
    int j = 0;
    bool isIpv4Addr;

    signalingPort = false;
    voicePort = false;

    /* Get pointer to private area */
    mtaPrivate = netdev_priv( skb->dev );

    /* Check if this packet is meant for us */
    if (memcmp(mtaPrivate->dev->dev_addr, skb_mac_header(skb), ETH_ALEN) == 0)
     skb->pkt_type = PACKET_HOST;

    /* Initialize the protocol. */
    ptr_ethhdr   = (struct ethhdr *)skb_mac_header(skb);
    skb->protocol = ptr_ethhdr->h_proto;

    if (drop_non_bootp)
    {
        if (mta_is_not_bootp_packet(skb))
        {
            dev_kfree_skb(skb);
            return 0;
        }
    }

    /* Update statistics */
    mtaPrivate->stats.rx_packets++;
    mtaPrivate->stats.rx_bytes += skb->len;

#ifdef CONFIG_INTEL_DOCSIS_ICMP_IIF
    /* We use the docsis_icmp_iif to support ICMP when wan0, mta0 or erouter0 are on the same subnet */
    /* the implementation in file icmp.c*/
    skb->docsis_icmp_iif = skb->dev->ifindex;
#endif

    if  (mta_check_udp_port( skb, 0, &signalingPort, &voicePort))
    {
        if (signalingPort)
        {
            /* In order to avoid race condition we will clean the MTA rate limit on the first signaling packet which arrived after one second had passed.*/
            if (cleanMtaRateLimitTable)
            {
            #ifdef MTANI_DEBUG
                printk( "MTANI clean MTA rate limit data base\n");
            #endif
                memset(&mtaRateLimitDataBase, 0, sizeof(mtaRateLimitDataBase));
                cleanMtaRateLimitTable = false;
                mod_timer(&rateLimitTimer, jiffies + msecs_to_jiffies(1000));
            }

            if (skb->protocol == ntohs(ETH_P_IP))
            {   
                ipv4Header = ((struct iphdr*)skb_network_header(skb));
                isIpv4Addr = true;
            }
            else
            {
                ipv6Header = ((struct ipv6hdr*)skb_network_header(skb));
                isIpv4Addr = false;
            }

            /* If the address is IPV4 address*/
            if (isIpv4Addr)
            {
                for (j = 0; j < mtaRateLimitDataBase.ipv4Root; j++) 
                {
                    if (mtaRateLimitDataBase.ipv4Header[j] == ipv4Header->saddr) 
                    {
                        if (mtaRateLimitDataBase.ipv4Rate[j] > MAX_PACKET_PER_SEC)
                        {
                        #ifdef MTANI_DEBUG
                            printk( "MTANI about to drop packets from IP ipv4Header->saddr = 0x%x\n", ipv4Header->saddr);
                        #endif
                            dropPacket(skb);
                            return 0;
                        }
                        
                        mtaRateLimitDataBase.ipv4Rate[j]++;
                        #ifdef MTANI_DEBUG
                        printk( "MTANI increment count from IP ipv4Header->saddr = 0x%x, count is %d\n", ipv4Header->saddr, mtaRateLimitDataBase.ipv4Rate[j]);
                        #endif
                        break;
                    }
                }

                /* If we didn't find any match for the IP then update the table with the new IP*/
                if (j == mtaRateLimitDataBase.ipv4Root)
                {
                    /* If the table is not full then add new value to the root location*/
                    if (mtaRateLimitDataBase.ipv4Root < RATE_LIMIT_UNIQUE_IP)
                    {
                        mtaRateLimitDataBase.ipv4Header[j] = ipv4Header->saddr;
                        mtaRateLimitDataBase.ipv4Rate[j] = 1;
                        mtaRateLimitDataBase.ipv4Root++;
                    }
                    else
                    {
                    #ifdef MTANI_DEBUG
                        printk( "MTANI IPV4 rate limit table is full, this IP will not be monitored for Rate limit\n");
                    #endif
                    }
                }
            }
            else
            {
                for (j = 0; j < mtaRateLimitDataBase.ipv6Root; j++) 
                {
                    if (!(memcmp(mtaRateLimitDataBase.ipv6Header[j].s6_addr32, ipv6Header->saddr.s6_addr32, sizeof(ipv6Header->saddr.s6_addr32))))
                    {
                        if (mtaRateLimitDataBase.ipv6Rate[j] > MAX_PACKET_PER_SEC)
                        {
                            dropPacket(skb);
                            return 0;
                        }
                        
                        mtaRateLimitDataBase.ipv6Rate[j]++;
                        break;
                    }
                }

                /* If we couldn't find the specific IP in the IP list.*/
                if (j == mtaRateLimitDataBase.ipv6Root)
                {
                    /* If the table is not full then add new value to the root location*/
                    if (mtaRateLimitDataBase.ipv6Root < RATE_LIMIT_UNIQUE_IP)
                    {
                        memcpy(mtaRateLimitDataBase.ipv6Header[j].s6_addr32, ipv6Header->saddr.s6_addr32, sizeof(ipv6Header->saddr.s6_addr32));
                        mtaRateLimitDataBase.ipv6Rate[j] = 1;
                        mtaRateLimitDataBase.ipv6Root++;
                    }
                    else
                    {
                    #ifdef MTANI_DEBUG
                        printk( "MTANI IPV6 rate limit table is full, this IP will not be monitored for Rate limit\n");
                    #endif
                    }
                }
                
            }
        }

#if CONFIG_TI_PACKET_PROCESSOR
        /* Check for destination if this packet is going to Voice NI */
        if (voicePort)
        {
            static struct net_device *ptr_voiceni_dev;

            ptr_voiceni_dev = dev_get_by_name(&init_net, "vni0");

            if (NULL != ptr_voiceni_dev)
            {
                int ret;
                skb->dev = ptr_voiceni_dev;

                /* Send the packet to voice NI */
                ret = dev_queue_xmit(skb);

                dev_put(ptr_voiceni_dev);
                return ret;
            }
        }
#endif
    }

    /* Pull the Ethernet MAC Address before passing it to the stack. */
    skb_pull(skb, ETH_HLEN);

    /* Push the skb to the IP stack */
    netif_rx( skb );
    return 0;
}

static struct net_device_stats *mta_get_net_stats(struct net_device *dev)
{
    mtaPrivate_t *mtaPrivate = netdev_priv(dev);

    /* Return the statistics */
    return (struct net_device_stats *) &mtaPrivate->stats;
}

static int mta_set_mac_addr(struct net_device *dev, void *addr)
{
    struct sockaddr *sa = addr;

    memcpy( dev->dev_addr, sa->sa_data, dev->addr_len );

    return 0;
}

static int mta_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
    int err = -EFAULT;
    struct mtani_data data;
    void __user *addr = (void __user *) ifr->ifr_ifru.ifru_data;
    unsigned int i;

    switch (cmd)
    {
    case SIOCSETVOICEPORTS:
        if (copy_from_user(&data, addr, sizeof(struct mtani_data)))
            break;

        memcpy( voice_port, data.voice_port, sizeof( voice_port ) );
        err = 0;

        for(i=0; i<CONFIG_TI_PACM_MAX_NUMBER_OF_ENDPOINTS * 2; i++)
        {
            if(!voice_port[i])
            {
                break;
            }
        }
        voice_ports_active = i;
        break;

    case SIOCGETVOICEPORTS:
        if (copy_to_user(addr, voice_port, sizeof(voice_port)))
            break;
        err = 0;
        break;

    case SIOCSETVOICEDROPPORTS:
        if (copy_from_user(&data, addr, sizeof(struct mtani_data)))
            break;

        memcpy( voice_drop_ports, data.voice_drop_ports , sizeof( voice_drop_ports ) );
        err = 0;
        break;
    case SIOCGETVOICEDROPPORTS:
        if (copy_to_user(addr, voice_drop_ports, sizeof(voice_drop_ports)))
            break;

        err = 0;
        break;

    case SIOCSETBOOTP:
        if (copy_from_user(&data, addr, sizeof(struct mtani_data)))
            break;

        drop_non_bootp = data.voice_dhcp_active;
        err = 0;
        break;

    default:
        err = -EINVAL;
    }

    return err;
}

static const struct net_device_ops mta_netdev_ops = {
         .ndo_open               = mta_open,
         .ndo_start_xmit         = mta_tx,
         .ndo_stop               = mta_release,
         .ndo_get_stats          = mta_get_net_stats,
         .ndo_do_ioctl           = mta_ioctl,
         .ndo_set_mac_address    = mta_set_mac_addr,
};

static void mta_init (struct net_device *dev)
{
    unsigned char mtaAddr[ ETH_ALEN ] = { 0x08, 0x00, 0x28, 0x32, 0x06, 0x03 }; /* Dummy */

    dev->netdev_ops = &mta_netdev_ops;
    dev->addr_len = ETH_ALEN;

    ether_setup(dev);
    memcpy( dev->dev_addr, mtaAddr, ETH_ALEN );
    memset( voice_port, 0, sizeof( voice_port ) );
#ifdef CONFIG_INTEL_NS_DEVICE_FILTER
    intel_register_ns_handler(dev, intel_ipv6_ns_filter);
#endif
    printk ("mtani: Network interface initialized successfully\n");

    setup_timer(&rateLimitTimer, mtaRateLimit_init, 0); 

    /* Configure the MTA rate limit table to be cleaned on first signaling packet*/
    cleanMtaRateLimitTable = true;
    return;
}

static int mta_tx (struct sk_buff *skb, struct net_device *dev)
{
    mtaPrivate_t *mtaPrivate = netdev_priv(dev);

    /* Update statistics */
    mtaPrivate->stats.tx_packets++;
    mtaPrivate->stats.tx_bytes += skb->len;
    skb_reset_mac_header(skb);

#if defined( CONFIG_INTEL_KERNEL_DOCSIS_SUPPORT )
    if (mta_is_packet_to_drop(skb, 1))
    {
#ifdef MTANI_DEBUG
        printk( "\nmtani: dropping packet\n");
#endif
        dev_kfree_skb(skb);
    }
    else
    {
        /* Push the packet to the DOCSIS Bridge. */
        DbridgeEsafe_DeviceXmit( skb, dev );
    }
#else
    /* Currently just free the skb */
    dev_kfree_skb(skb);
#endif

    return 0;
}

static void dropPacket(struct sk_buff *skb)
{
#ifdef CONFIG_TI_DEVICE_PROTOCOL_HANDLING
    ti_protocol_handler (skb->dev, skb);
#endif
#ifdef CONFIG_TI_PACKET_PROCESSOR
    ti_hil_pp_event (TI_DOCSIS_FLTR_DISCARD_PKT, (void *)skb);
#endif
    kfree_skb (skb);
}

int __init mta_init_module (void)
{
    int result;
    struct net_device *dev;
    mtaPrivate_t *mtaPrivate;

    /* Allocate memory for network interface */
    if ( ( dev = alloc_netdev( sizeof(mtaPrivate_t), CONFIG_TI_PACM_MTA_INTERFACE, mta_init ) ) == NULL )
    {
    printk ("mtani: Error allocating memory for mta device\n");
    return ENOMEM;
    }

    /* Save the network device in the private area */
    mtaPrivate = netdev_priv( dev );
    mtaPrivate->dev = dev;

    /* Register network interface */
    if ((result = register_netdev (dev)))
    {
    printk ("mtani: Error %d initializing logical mta device\n",result);
    return result;
    }

#if defined( CONFIG_INTEL_KERNEL_DOCSIS_SUPPORT )
    /* Register MTA network interface to DOCSIS bridge */
    DbridgeEsafe_AddNetDev( dev, DBR_ESAFE_EMTA, mta_rx );
#endif

    printk ("mtani: Network interface registered successfully\n");
    return 0;
}

void __exit mta_cleanup (void)
{
    struct net_device *dev;

    /* Get the pointer to the network device */
    dev = dev_get_by_name(&init_net, CONFIG_TI_PACM_MTA_INTERFACE );

    if ( dev )
    {
         /* Unregister network interface */
     unregister_netdev (dev);

     printk ("mtani: Cleaning up Network interface\n");
    }
    else
    {
     printk ("mtani: Failed to clean up Network interface\n");
    }

    del_timer(&rateLimitTimer); 
}

MODULE_AUTHOR("Texas Instruments, Inc");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTA Network interface");

module_init (mta_init_module);
module_exit (mta_cleanup);

EXPORT_SYMBOL( mta_rx );

