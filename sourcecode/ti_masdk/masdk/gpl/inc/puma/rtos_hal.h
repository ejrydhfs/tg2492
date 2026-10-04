/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2003-2019 Intel Corporation.
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
 * Copyright(c) 2003-2019 Intel Corporation. All rights reserved.
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
 *******************************************************************************
 * FILE PURPOSE:    RTOS HAL
 *******************************************************************************
 * FILE NAME:       rtos_hal.h
 *
 * DESCRIPTION:     This include file contains definitions to abstract the
 *                  RTOS for the TIUHAL & TIUDRV layer.
 *
 ******************************************************************************/


#ifndef __RTOS_HAL_H__
#define __RTOS_HAL_H__
#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/kernel.h>
#include <stdarg.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <asm-arm/arch-avalanche/generic/haltypes.h>
#include <asm-arm/arch-avalanche/generic/soc.h>

/*******************************************************************************
 *
 * System servies needed by TIUHW
 *
 ******************************************************************************/

#define DRV_MDELAY(X,Y)			mdelay((Y))
#define DRV_DEBUG_MSG(SPY_KEY,...)		printk(KERN_INFO __VA_ARGS__ )
#define DRV_ERROR_MSG(SPY_KEY,...)		printk(KERN_ERR __VA_ARGS__ )	
#define DRV_TRACE_MSG(...)		    /* DO NOTHING */
/*#define DRV_TRACE_MSG(...)		    printk(KERN_ERR __VA_ARGS__ )  */

#define DRV_ALLOC(X)			vmalloc((X))
#define DRV_FREE(X)				vfree((X))
#define	DRV_GETENV(X)			
#define DRV_STRTOL(X,Y,Z)			simple_strtol((X),(Y),(Z))

#define INT_CONNECT(name,intr,hndlr)    \
    ((request_irq(intr,hndlr,0,name,NULL)) ? FALSE : TRUE)

#define INT_DISCONNECT(intr)            \
    free_irq(intr,NULL)

#define INT_HANDLER_PROTO(name)         \
    void name(int irq, void *dev_id, struct pt_regs *reg)
    
/*******************************************************************************
 *
 * local types needed by TIUHW that are NOT present in haltypes.h
 *
 ******************************************************************************/
#ifndef SINT8
#define SINT8 INT8
#endif

#ifndef SINT16
#define SINT16 short
#endif

#ifndef TCID
#define TCID   UINT8
#endif

#ifndef SINT32
#define SINT32 INT32
#endif

#ifndef CHAR
#define CHAR char
#endif

#ifndef OK
#define OK TRUE
#endif

#ifndef NOK
#define NOK FALSE
#endif

#ifndef PRIVATE
#define PRIVATE static
#endif
#else 

/*******************************************************************************
 *  Userspace version of the above kernel funcitonality/macros...
 ******************************************************************************/
#include "swpform.h"
#include "ggconfig.h"
#include "tsgtcid.h"
#define DRV_MDELAY(X,Y)			XtSleep(0,MSEC_TO_TICKS(Y))
#define DRV_DEBUG_MSG(SPY_KEY,...)		XdbgPrintf(__VA_ARGS__ )
#define DRV_ERROR_MSG(SPY_KEY,...)		XdbgPrintf(__VA_ARGS__ )	
#define DRV_TRACE_MSG(...)		    /* DO NOTHING */
#define DRV_ALLOC(X)			malloc((X))
#define DRV_FREE(X)				free((X))
		
#define DRV_STRTOL(X,Y,Z)		strtol((X),(Y),(Z))
/* #define DRV_TRACE_MSG(...)		    XdbgPrintf(__VA_ARGS__)  */

#endif /* __KERNEL__ */
#endif /* __RTOS_HAL_H__ */
