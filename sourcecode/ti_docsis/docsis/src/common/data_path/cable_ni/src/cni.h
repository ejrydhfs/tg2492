/*

(c) Copyright 2020 CommScope, Inc.
Copyright 2011-2019, ARRIS Enterprises, LLC

All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.


Alternatively, this software may be distributed under the terms of the
GNU General Public License ("GPL") version 2 as published by the Free
Software Foundation.


THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

GPLv2 license:

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/
  
typedef struct
{
    unsigned int    cmd;    /**< Command */
    void            *data;  /**< Data provided with the command - depending upon command */
} CniDrvPrivIoctl;

typedef enum
{
    DISABLE_FILTER,
    ENABLE_FILTER_WITH_BYPASS,
    ENABLE_FILTER_WITHOUT_BYPASS
} CniSocketFilter_t;

typedef struct
{
    CniSocketFilter_t  enable;
    struct sockaddr addr;
} CniDrvDbridgeBypass;

typedef struct
{
    unsigned long       rx_packets;         /* total packets received	 */
    unsigned long       tx_packets;         /* total packets transmitted */
    unsigned long long  rx_bytes;           /* total bytes received 	 */
    unsigned long long  tx_bytes;           /* total bytes transmitted	 */
    unsigned long       rx_errors;          /* bad packets received		 */
    unsigned long       tx_errors;          /* packet transmit problems	 */
    unsigned long       rx_drops;           /* dropped receive packets   */
    unsigned long       tx_drops;           /* dropped tranmit packets   */

    unsigned long       filtered_rx_packets;/* matched rx packets */
    unsigned long       filtered_rx_bytes;  /* matched rx bytes   */
    unsigned long       filtered_tx_packets;/* matched tx packets */
    unsigned long       filtered_tx_bytes;  /* matched tx bytes   */

    int lro_enabled;
    unsigned long       rx_aggregated;      /* total packets processed by LRO */
    unsigned long       rx_flushed;         /* total handoffs to IP from LRO  */
} CniDrvStats;

/* CNI new Ioctl's created - using a value with a base that can be adjusted as per needs */
#define CNI_IOCTL_BASE          0

/* Filtering IOCTL */
#define CNI_LRO_ENABLE          (CNI_IOCTL_BASE + 0)
#define CNI_DOCBRIDGE_BYPASS    (CNI_IOCTL_BASE + 1)
#define CNI_GET_STATS           (CNI_IOCTL_BASE + 2)
