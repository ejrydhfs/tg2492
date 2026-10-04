/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2011-2019 Intel Corporation.
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
 * Copyright(c) 2011-2019 Intel Corporation. All rights reserved.
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
 * File Name: dimhw_gw.h
 *
 * Description: The file defines types used by user space wrappers for
 *              DIMHW functions implemented in hwdsp_mod kernel module.
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 */

#ifndef _DIMHW_GW_H_
#define _DIMHW_GW_H_

/* Puma6 specific definitions */
#define PUMA6_DSP_MEM_BASE         	0x04000000
#define PUMA6_DSP_REGISTER_BASE    	0x05000000
#define PUMA6_EXT_ADDR_CFG_REG     	(PUMA6_DSP_REGISTER_BASE + 0x04)
#define PUMA6_INT_POLARITY_REG     	(PUMA6_DSP_REGISTER_BASE + 0x34)
#define PUMA6_DSP_EXTERNAL_MEM_SIZE	0x00200000
#define PUMA6_NMI_REG              	0x00040008

/* Puma5 specific definitions */
#define PUMAV_DSP_MEM_BASE         	0x04000000
#define PUMAV_DSP_REGISTER_BASE    	0x05000000
#define PUMAV_EXT_ADDR_CFG_REG     	(PUMAV_DSP_REGISTER_BASE + 0x04)
#define PUMAV_INT_POLARITY_REG     	(PUMAV_DSP_REGISTER_BASE + 0x34)
#define PUMAV_DSP_EXTERNAL_MEM_SIZE	0x00200000
#define PUMAV_NMI_REG              	0x08611A34

#define DSPMOD_DEVICE_NAME "tidspmod"

/*
 * GW_DSPMOD_PARAM_T defines a type of 'param' of ioctl system call
 * for hwdsp_mod kernel module
*/
typedef union  GW_DSPMOD_PARAM_tag
{
    struct DSPMOD_PARAM_tag
    {
        unsigned short     dsp_num;
        unsigned short     bool_value; /* 0 => FALSE, 1 => TRUE */
    } u;
    unsigned long v;
} GW_DSPMOD_PARAM_T;

/* dspmod ioctl definitions for puma-5 */

#define DSPMOD_IOCTL_MAGIC 'D'

#define DSPMOD_IOCTL_QUERY_DSP_TYPE   _IOR(DSPMOD_IOCTL_MAGIC, 0, unsigned long)
#define DSPMOD_IOCTL_DSP_HALT         _IOW(DSPMOD_IOCTL_MAGIC, 1, unsigned short)
#define DSPMOD_IOCTL_QUERY_DSP_NUM    _IOR(DSPMOD_IOCTL_MAGIC, 2, unsigned long)
#define DSPMOD_IOCTL_QUERY_DSP_ADDR   _IOR(DSPMOD_IOCTL_MAGIC, 3, unsigned long)
#define DSPMOD_IOCTL_DSP_RESET        _IOW(DSPMOD_IOCTL_MAGIC, 4, unsigned long)
#define DSPMOD_IOCTL_DSP_NMI          _IOW(DSPMOD_IOCTL_MAGIC, 5, unsigned long)
#define DSPMOD_IOCTL_DSP_PWR          _IOW(DSPMOD_IOCTL_MAGIC, 6, unsigned long)
#define DSPMOD_IOCTL_RESET_TDM        _IOW(DSPMOD_IOCTL_MAGIC, 7, unsigned long)
#define DSPMOD_IOCTL_REG_READ	      _IOR(DSPMOD_IOCTL_MAGIC, 8, unsigned long)

/* Following ioctl's are not supported for Puma-5. */
#define DSPMOD_IOCTL_DSP_INT          _IO(DSPMOD_IOCTL_MAGIC, 7)
#define DSPMOD_IOCTL_DSP_LOOP         _IO(DSPMOD_IOCTL_MAGIC, 8)

#define DSPMOD_DEV_IOC_MAXNR 8

#endif /* _DIMHW_GW_H_ */
