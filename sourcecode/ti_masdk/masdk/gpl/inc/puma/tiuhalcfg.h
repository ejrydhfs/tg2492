/*
*
* This file is provided under a dual BSD/GPLv2 license.  When using or
* redistributing this file, you may do so under either license.
*
* GPL LICENSE SUMMARY
*
* Copyright(c) 2008-2019 Intel Corporation.
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
* Copyright(c) 2008-2019 Intel Corporation. All rights reserved.
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
*
*
* Description:
* This file is used to configure TIUHAL options that may differ
* from the defaults.
*
*/

#ifndef  TIUHALCFG_FILE_HEADER_INC
#define  TIUHALCFG_FILE_HEADER_INC

/* NOTE: DO NOT PLACE ANY OTHER HEADER FILES OTHER THAN THE ONES BELOW! */
#include "tiuhalcfg_all.h"


/******************************************************************************
* The definitions below are used to enable/disable certain environment
* variable support.  It is assumed the customer would disable as needed
* unneeded variables in their final product to cut down the code a bit.
******************************************************************************/
#ifndef  TIUHW_MAX_TIDS
#define TIUHW_MAX_TIDS      2
#endif

#ifndef  TIUHW_MIN_TIDS
#define TIUHW_MIN_TIDS      1
#endif

#define TIUHAL_SUPPORT_TID_INTERRUPTS 1
/***********************************************************************
* 
* The section below is TIUHAL specific.  On the P5 we're communicating
* via the  SOC's internal SPI and TDM interface, the defines below are used 
*
************************************************************************/

#define TIUHAL_HAS_TEARDOWN    1
#define TIUHAL_TDM_CLKOUT       2048000
#define TIUHAL_TDM_CLKIN       10240000
#define TIUHAL_PCM_FS          8000
#define TIUHAL_TDM_FS_PER      (TIUHAL_TDM_CLKOUT/TIUHAL_PCM_FS)

#define TIUHAL_SPI_MAX_CS 			(TIUHW_MAX_TIDS*2) /* How many chip select combinations are we dealing with? */
/* End of HAL specific section */

#ifndef  TIUHW_MAX_TCIDS
#define TIUHW_MAX_TCIDS    4
#endif
#endif   /* ----- #ifndef TIUHALCFG_FILE_HEADER_INC  ----- */

