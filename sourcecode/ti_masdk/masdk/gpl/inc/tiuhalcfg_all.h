/*
*  This file is provided under a dual BSD/GPLv2 license.  When using or
*  redistributing this file, you may do so under either license.
*
*  GPL LICENSE SUMMARY
*
*  Copyright(c) 2007-2019 Intel Corporation.
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of version 2 of the GNU General Public License as
*  published by the Free Software Foundation.
*
*  This program is distributed in the hope that it will be useful, but
*  WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
*  The full GNU General Public License is included in this distribution
*  in the file called LICENSE.GPL.
*
*
*  Contact Information:
*  Intel Corporation
*  2200 Mission College Blvd.
*  Santa Clara, CA  97052
*
*  BSD LICENSE
*
*  Copyright(c) 2007-2019 Intel Corporation. All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*
*    * Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the following disclaimer in
*      the documentation and/or other materials provided with the
*      distribution.
*
*    * Neither the name of Intel Corporation nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*  FILE PURPOSE: Default definitions for tiudrv.h that may be overwritten by
*               the platform specific file.
*
*/

#ifndef __TIUHAL_CFG_ALL_H__
#define __TIUHAL_CFG_ALL_H__

/* Platform specific */
#define TIUHW_MAX_TCIDS 4    /* How many physical ports are there on the platform? */
#define TIUHW_MAX_TIDS        2    /* How many "TIDS" do we have? */
#define TIUHW_MIN_TIDS        1    /* How many "TIDS" do we have? */
#define TIUHW_CMN_CS_TYPE        1   /* Enable if using common chipselect type */

/* TIUHAL Specifc values */
#define TIUHAL_USE_CMN_REG_T /* 8bit */
#define TIUHAL_USE_CMN_DATA_T /* 8bit */
#define TIUHAL_HAS_SELECT_TID 1   /* set this if the platform provides a mechanism to steer the chipselect to a specific channel (selection)*/
#define TIUHAL_HAS_DESELECT_TID 1 /* set this if the platform provides a mechanism to steer the chipselect to a specific channel (deselection)*/
#undef  TIUHAL_HAS_TID_INTS       /* set this if the TIUHAL needs to support interrutps directly (normally for FPGA)*/ 
#undef  TIUHAL_HAS_DETECT         /* set this if the TIUHAL needs to support hook detection via a pin reading (normally for FPGA) */
#undef  TIUHAL_HAS_RINGER         /* set this if ring generation control is done via the HAL (FPGA/SOC) */
#undef  TIUHAL_HAS_TEARDOWN       /* set this if init() in TIUHAL allocated resources that need to be freed */
#define TIUHAL_USE_CMN_CAP_T 1    /* set this if using the common capaibilites structure */
#define TIUHAL_USE_CMN_INIT_ERR_T 1 /* set this if using the common initialization error types */
#define TIUHAL_USE_CMN_IO_ERROR_T 1 /* set this if using the common I/O error types */
#undef  TIUHAL_SUPPORT_TID_INTERRUPTS   /* set this if TIUHAL supports TID tinerrupts */
#endif

