/*
*  This file is provided under a dual BSD/GPLv2 license.  When using or
*  redistributing this file, you may do so under either license.
*
*  GPL LICENSE SUMMARY
*
*  Copyright(c) 2008-2019 Intel Corporation.
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
*  Copyright(c) 2008-2019 Intel Corporation. All rights reserved.
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
*  Description:
*  This interface is the bridge between user and kernel space for the TIUHAL
*  module.
*
*/

#ifndef  TIUHAL_LIN_FILE_HEADER_INC
#define  TIUHAL_LIN_FILE_HEADER_INC

/* The DEVICE_NAME is used by the kernel module and the DEVICE_PATH is used by
 * the userspace application.  BOTH MUST have the SAME basename in order for
 * this to work properly.
 */
#define TIUHAL_DEVICE_NAME "titele"
#define TIUHAL_DEVICE_PATH "/dev/titele"


/*******************************************************************************
 *  These definitions are used to communicate with the TIUHAL module
 ******************************************************************************/
#define TIUHAL_LIN_IOCTL_MAGIC 'N'
#define TIUHAL_LIN_IOCTL_MREAD  _IOWR(TIUHAL_LIN_IOCTL_MAGIC, 0,TIUHAL_LIN_RW_IOCTL_T)
#define TIUHAL_LIN_IOCTL_MWRITE _IOWR(TIUHAL_LIN_IOCTL_MAGIC, 1,TIUHAL_LIN_RW_IOCTL_T)
#define TIUHAL_LIN_IOCTL_CS_REG _IOW(TIUHAL_LIN_IOCTL_MAGIC,  2,TIUHAL_LIN_CS_IOCTL_T)
#define TIUHAL_LIN_IOCTL_IOCTL  _IOWR(TIUHAL_LIN_IOCTL_MAGIC, 3,TIUHAL_LIN_IOCTL_IOCTL_T)
#define TIUHAL_LIN_IOCTL_CONFIG _IOR(TIUHAL_LIN_IOCTL_MAGIC,  4,TIUHAL_LIN_CONFIG_IOCTL_T)

#define TIUHAL_LIN_IOCTL_MAXNR 6

#define TIUHAL_LIN_MAX_RD_WR_BYTES    80
#define TIUHAL_IOCTL_MAX_IOCTL_BYTES  32

#ifdef LANTIQ
#define TIUHAL_FLAGS_TID_DEFAULT 0
#define TIUHAL_FLAGS_TID_LANTIQ  1
#endif /* LANTIQ */

/*******************************************************************************
 *  The types and structures used in common between user and common code.
 ******************************************************************************/
typedef enum
{
    force_reset_tid=1000,
    config_gpio_lantiq_tid=1002
}TIUHAL_IOCTL_CMD_T;

typedef struct 
{
	UINT32			tcid; /* Keeping it to UINT32 to avoid alignment issues */
#ifdef LANTIQ
    UINT32                  flags;
#endif /* LANTIQ */

	TIUHAL_DATA_T 	*bytestream;
	TIUHAL_REG_T  	reg;
	UINT32        	data_len;
	TIUHAL_IO_ERR_T return_value;
}TIUHAL_LIN_RW_IOCTL_T;

typedef struct
{
	UINT32			tcid; /* Keeping it to UINT32 to avoid alignment issues */
	TIUHW_CHIPSEL_T chipselect_data;
}TIUHAL_LIN_CS_IOCTL_T;

typedef struct
{
	UINT32 return_code;
	UINT32 cmd;
	void   *in_param;
	void   *out_param;
	UINT32 num_bytes_in;
	UINT32 num_bytes_out;	
}TIUHAL_LIN_IOCTL_IOCTL_T;

typedef struct
{
	BOOL   interrupts_enabled;
	UINT32 version;
}TIUHAL_LIN_CONFIG_IOCTL_T;

/*******************************************************************************
 *  Internal HAL function prototypes.
 ******************************************************************************/
int  tiuhal_board_init(TIUHAL_CAP_T *capabilities);
void tiuhal_board_teardown(void);


#endif   /* ----- #ifndef TIUHAL_LIN_FILE_HEADER_INC  ----- */

