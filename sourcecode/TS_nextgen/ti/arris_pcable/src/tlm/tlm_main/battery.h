/*

Copyright (c) 2015-2016 ARRIS Enterprises, Inc.

All rights reserved.

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

/******************************************************************************
 * FILE PURPOSE: Holds paths to the actual charger images.
 ******************************************************************************
 * FILE NAME:     battery.h
 *
 * DESCRIPTION:   Holds interface and path names for accessing the Atmel and
 *                TI charger devices.
 *                
 *******************************************************************************/

#ifndef BATTERY_H
#define BATTERY_H
#include <linux/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef bool
    #define bool int
#endif

#ifndef false
    #define false 0
#endif

#ifndef true
    #define true 1
#endif

#define TLM_MODULE_ROOT_DIR "/lib/modules/2.6.39.3/drivers/tlm/"

typedef struct
{
   char app_name[32];
   uint32 app_crc;
   char st_name[32];
   uint32 st_crc;
   char letters[4];
} chargerFwStruct;

#define LOCATION_OF_ATMEL_KERNEL_MODULE   "/lib/modules/2.6.39.3/drivers/tlm/atmel.ko"
#define LOCATION_OF_TIBAT_KERNEL_MODULE   "/lib/modules/2.6.39.3/drivers/tlm/tibat.ko"


#define ATMEL_APP_MAJOR_NUMBER            112
#define TI_APP_MAJOR_NUMBER               112

#define ATMEL_APP_DEVICE_NAME             "/dev/atmel"
#define TI_APP_DEVICE_NAME                "/dev/tibat"

#define ATMEL_SIZE_OF_FLASH 	          (0x5000)
#define TIBAT_SIZE_OF_FLASH               (0x4000)

#define TIBAT_PAGE_SIZE 		  (1024)
#define ATMEL_PAGE_SIZE 		  (256)

#define BATTERY_APP_VERSION_LENGTH        (9) // 01.02ABC - Add one for null terminator

#define LOCATION_OF_TI_BINARY_VERSION     (0xE4)
#define LOCATION_OF_ATMEL_BINARY_VERSION  (0x1C4)
#define MAIN_START_ADDRESS 		  (0xC000)

#define BATTERY_IOC_MAGIC                 'g'
#define BATTERY_IOCTDEBUG                 _IO(BATTERY_IOC_MAGIC,0)
#define BATTERY_IOCTWRITEFUSE             _IO(BATTERY_IOC_MAGIC,0)
#define BATTERY_IOCTERASEAPPANDBOOT       _IO(BATTERY_IOC_MAGIC,1)
#define BATTERY_IOCTCRC                   _IO(BATTERY_IOC_MAGIC,2)

//#define BATTERY_DEBUG 1  // comment out to enable debug ioctl

int  battery_load_kernel_module(tCharger charger);
int  battery_free_kernel_module(tCharger charger);
int  battery_load_firmware_binary(const char *pBinary, tCharger charger);
int  battery_retrieve_firmware_revision_from_binary(const char *pBinaryPath, char *pRetStr, int length, tCharger charger);
bool battery_verify_binary(unsigned int crcInfo, tCharger charger);



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

