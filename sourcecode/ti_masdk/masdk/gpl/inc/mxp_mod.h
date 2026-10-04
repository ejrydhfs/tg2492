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
 *  File Name: mxp_mod.h
 *
 *  Description: Definitions and types for Linux MXP library
 *
 */

#ifndef _MXP_MOD_H_
#define _MXP_MOD_H_

/* current version of Linux MXP doesn't use dynamic MXP configuration */
#define MAX_TMROBJS      1023
#define MXP_TASK_MAX     128
#define MAX_MASSAGES     16384
#define MAX_QUEUES       1024
#define MAX_SEGMENTS     8
#define MAX_TIMERS       650

#define MAX_NAME_LEN   16
#define MIN_TASK_STACKSIZE 0x4000

#define MXP_LOCK   0x0100
#define MXP_UNLOCK 0x0101

#define MAX_CLOCK      0x7FFFFFFF

struct TMROBJ_tag;

typedef void (*TrmEventHook_T)(struct TMROBJ_tag *this);

typedef struct TMROBJ_tag{
  unsigned int   _index;
  unsigned long  _wakeUpTime;
  TrmEventHook_T actionCB;
  void           *owner;
  int            wait4event;  
} TMROBJ_T;

/* MXP system call parameter type */
typedef struct {
  int result;
  union {
    struct {             /* MXP_TASK_ALLOC      */
      char   name[MAX_NAME_LEN];
      int    prio;
      int    pid;
      int    tid;
    } task;
    struct {
      int    tid;
      int    timeout;
    } task_cmd;
    struct {
      int          tid;
      unsigned long events;
      int          condition;
      unsigned int timeout;
    } ev;
    struct {
      int           qid;
      unsigned int  timeout;
      void          *msg_ptr;
      int           depth;
      unsigned long events;
      int           tid;
      char          name[MAX_NAME_LEN];
    } q;
    struct {
      int           tmr_id;
      unsigned long ev_fl;
      int           qid;
      void         *msg;
      int           tsk_id;
      unsigned long timeout;
      unsigned long reload;
    } tmr;
  } cp;
} MXP_CMD_T;

/* MXP memory management system call parameter type */
typedef struct {
  int             result;
  unsigned int    segId;
  unsigned int    partId;
  int             length;
  int             blocks;
  void           *ptr;
  char            name[MAX_NAME_LEN];
} MXP_MEM_CMD_T;

#ifdef __KERNEL__
#include <linux/wait.h>

/* kernel only task control block fields */
typedef struct {
  wait_queue_head_t  gate_lock;
  TMROBJ_T           tmrobj;
} MXP_SUBTCB_T;

/* queue types */
typedef struct msg_container_t {
  struct msg_container_t *p;
  struct msg_container_t *n;
  void            *msg;
} MSG_CONTAINER_T;

typedef struct msg_queue_t {
  MSG_CONTAINER_T *head;
  MSG_CONTAINER_T *tail;
  int             msgcnt;
  int             depth;
  int             wait4msg;
  char            name[16];
  int             taskId;
  unsigned long   events;
  wait_queue_head_t  queue_lock;
  int             state; /* 0 - free; 1 - busy */
  //ARRIS ADD Start
  int             hiwater_cnt; //hiwater of msg number.
  //ARRIS ADD End
} MSG_QUEUE_T;

#endif

/* common user and kernel task control block fields */
typedef struct {
  int             busy;
  int             pid;
  int             priority;

  unsigned long   events_posted;
  unsigned long   events_mask;
  int             events_condition;
  int             wait4event;

  char            name[MAX_NAME_LEN];
  unsigned int    event_cnt;

} MXP_TCB_T;

#define MXP_PROC_DIR_NAME "timxp"

/* MXP core ioctl definitions */

#define MXP_CORE_DEVICE_NAME "timxpcore"
#define MXPCORE_IOCTL_MAGIC 'M'

#define MXP_QUEUE_POST     _IOWR(MXPCORE_IOCTL_MAGIC, 0, MXP_CMD_T) 
#define MXP_QUEUE_WAIT     _IOWR(MXPCORE_IOCTL_MAGIC, 1, MXP_CMD_T)  
#define MXP_EVENT_POST     _IOWR(MXPCORE_IOCTL_MAGIC, 2, MXP_CMD_T)  
#define MXP_EVENT_WAIT     _IOWR(MXPCORE_IOCTL_MAGIC, 3, MXP_CMD_T)  
#define MXP_EVENT_CLEAR    _IOWR(MXPCORE_IOCTL_MAGIC, 4, MXP_CMD_T)  
#define MXP_EVENT_INQUIRY  _IOWR(MXPCORE_IOCTL_MAGIC, 5, MXP_CMD_T)  
#define MXP_QUEUE_CREATE   _IOWR(MXPCORE_IOCTL_MAGIC, 6, MXP_CMD_T)  
#define MXP_QUEUE_DELETE   _IOWR(MXPCORE_IOCTL_MAGIC, 7, MXP_CMD_T)  
#define MXP_QUEUE_IDENTIFY _IOWR(MXPCORE_IOCTL_MAGIC, 8, MXP_CMD_T)  
#define MXP_QUEUE_INQUIRY  _IOWR(MXPCORE_IOCTL_MAGIC, 9, MXP_CMD_T)  
#define MXP_QUEUE_DELALL   _IOWR(MXPCORE_IOCTL_MAGIC, 10, MXP_CMD_T) 
#define MXP_TMR_CREATE     _IOWR(MXPCORE_IOCTL_MAGIC, 11, MXP_CMD_T) 
#define MXP_TMR_START      _IOWR(MXPCORE_IOCTL_MAGIC, 12, MXP_CMD_T) 
#define MXP_TMR_ABORT      _IOWR(MXPCORE_IOCTL_MAGIC, 13, MXP_CMD_T) 
#define MXP_TMR_DELETE     _IOWR(MXPCORE_IOCTL_MAGIC, 14, MXP_CMD_T) 
#define MXP_TMR_GETTICK    _IOWR(MXPCORE_IOCTL_MAGIC, 15, MXP_CMD_T) 
#define MXP_TASK_ALLOC     _IOWR(MXPCORE_IOCTL_MAGIC, 16, MXP_CMD_T) 
#define MXP_TASK_IDENTIFY  _IOWR(MXPCORE_IOCTL_MAGIC, 17, MXP_CMD_T) 
#define MXP_TASK_FREE      _IOWR(MXPCORE_IOCTL_MAGIC, 18, MXP_CMD_T) 
#define MXP_TASK_SLEEP     _IOWR(MXPCORE_IOCTL_MAGIC, 19, MXP_CMD_T) 

#define MXPCORE_DEV_IOC_MAXNR 19

/* MXP mem ioctl definitions */

#define MXP_MEM_DEVICE_NAME "timxpmem"
#define MXPMEM_IOCTL_MAGIC 'M'

#define MXP_MEM_BLK_ALLOC     _IOWR(MXPMEM_IOCTL_MAGIC, 0, MXP_MEM_CMD_T) 
#define MXP_MEM_BLK_FREE      _IOWR(MXPMEM_IOCTL_MAGIC, 1, MXP_MEM_CMD_T) 
#define MXP_MEM_PART_INQUIRY  _IOWR(MXPMEM_IOCTL_MAGIC, 2, MXP_MEM_CMD_T) 
#define MXP_MEM_PART_IDENTIFY _IOWR(MXPMEM_IOCTL_MAGIC, 3, MXP_MEM_CMD_T) 
#define MXP_MEM_PART_CREATE   _IOWR(MXPMEM_IOCTL_MAGIC, 4, MXP_MEM_CMD_T) 
#define MXP_MEM_SEG_CREATE    _IOWR(MXPMEM_IOCTL_MAGIC, 5, MXP_MEM_CMD_T) 
#define MXP_MEM_SEG_IDENTIFY  _IOWR(MXPMEM_IOCTL_MAGIC, 6, MXP_MEM_CMD_T) 
#define MXP_MEM_SEG_GETNAME   _IOWR(MXPMEM_IOCTL_MAGIC, 7, MXP_MEM_CMD_T) 
#define MXP_MEM_SEG_INIT      _IOWR(MXPMEM_IOCTL_MAGIC, 8, MXP_MEM_CMD_T) 

#define MXPMEM_DEV_IOC_MAXNR 8

#endif /* _MXP_MOD_H_ */
