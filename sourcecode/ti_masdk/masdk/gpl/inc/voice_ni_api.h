/*
 This file is provided under a dual BSD/GPLv2 license.  When using or
 redistributing this file, you may do so under either license.

 GPL LICENSE SUMMARY

 Copyright(c) 2012-2019 Intel Corporation.

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

 Copyright(c) 2012-2019 Intel Corporation. All rights reserved.

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
*/

#ifndef  VOICE_NI_API_FILE_HEADER_INC
#define  VOICE_NI_API_FILE_HEADER_INC

#include <linux/sockios.h>


#define SIOCGETVOICENI_PARAMS (SIOCDEVPRIVATE +0)

#define VOICENI_DEV_NAME "vni0"

/*******************************************************************************************
 * Packet Processor Configuration Information needed to be sent to DSP in HW_CONFIG message
 ******************************************************************************************/
typedef struct VOICENI_PP_CONFIG {
  UINT16 pid;                   /* DSP's assigned PID */
  UINT16 cppi_queue_mgr_id;     /* CPPI queue manager ID number */
  UINT16 cppi_bd_queue_id;      /* CPPI free descriptor queue ID for DSP */
  UINT16 buffer_pool_mgr_id;    /* CPPI buffer pool Manager ID */
  UINT16 buffer_pool_id;        /* CPPI buffer pool ID */
  UINT16 cppi_dsp_rx_queue_id;  /* CPPI RX queue ID */
  UINT16 cppi_dsp_tx_queue_id;  /* CPPI TX queue ID */
  UINT16 buffer_pool_size;      /* CPPI buffer pool buffer size */
  UINT16 buffer_pool_count;     /* CPPI buffer pool buffer count */
  UINT16 buffer_pool_refcount;  /* CPPI buffer pool buffer refcount enable/disable */
  UINT16 cppi_vni_rx_q;         /* CPPI VNI RX queue */
  UINT16 cppi_vni_tx_q;         /* CPPI VNI TX queue (to PP) */
  UINT16 cppi_acc_rx_ch;        /* CPPI VNI Accumulator RX CH */
  UINT16 cppi_acc_rx_intv;      /* CPPI VNI Accumulator interrupt vector */
  UINT16 cppi_dsp_rx_infra_qid; /* CPPI DSP infrastructure DMA input queue */
  UINT16 cppi_dsp_rx_infra_ch;  /* CPPI DSP infrastructure DMA TX channel */
  UINT16 cppi_dsp_rx_infra_dma; /* CPPI DSP infrastructure DMA ID */
  UINT16 cppi_dsp_rx_infra_tdq; /* CPPI DSP infrastructure DMA teardown queue */
  UINT16 cppi_infra_bd_queue_id;/* CPPI free descriptor queue ID for Infrastructure DMA */
} VOICENI_PP_CONFIG_T;

#endif
