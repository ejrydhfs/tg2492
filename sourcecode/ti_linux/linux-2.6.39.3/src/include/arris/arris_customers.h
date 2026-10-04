/*
(c) Copyright 2020 CommScope, Inc.
Copyright (c) 2008-2019 ARRIS Enterprises, LLC

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
/*
////////////////////////////////////////////////////////////////////////////////
//
// arris_customers.h     Definitions for the Arris Customers
//
////////////////////////////////////////////////////////////////////////////////
*/
#include "status.h"
#ifndef __ARRIS_CUSTOMERS_H__
#define __ARRIS_CUSTOMERS_H__

/*
*  Below enum for Customer ID should be in sync with the enum mentioned 
*  in Dev Wiki Page : http://devwiki.arrisi.com/wiki/IPTV_DEV_BRCM#Customer_Enumeration
*  Make sure to update the enum in Dev Wiki Page when the below enum is
*  changed.
*/

//Customer index. This is used with MFG_CUST_INDEX NVM setting
enum
{
    CUSTOMER_RESERVED,      // 0
    CUSTOMER_DEFAULT,       // 1
    CUSTOMER_COMCAST,       // 2
    CUSTOMER_TIMEWARNER,    // 3
    CUSTOMER_CABLEVISION,   // 4
    CUSTOMER_COX,           // 5
    CUSTOMER_VTR,           // 6
    CUSTOMER_UPC,           // 7
    CUSTOMER_VIRGIN,        // 8
    CUSTOMER_TELENET,       // 9
    CUSTOMER_JNC,           // 10
    CUSTOMER_CHARTER,       // 11
    CUSTOMER_SUDDENLINK,    // 12
    CUSTOMER_BEND,          // 13
    CUSTOMER_BUCKEYE,       // 14
    CUSTOMER_COMPORIUM,     // 15
    CUSTOMER_NEWWAVE,       // 16
    CUSTOMER_ROGERS,        // 17
    CUSTOMER_SHAW,          // 18
    CUSTOMER_WAVE,          // 19
    CUSTOMER_ZIGGO,         // 20
    CUSTOMER_OPEN_21,       // 21
    CUSTOMER_COGECO,        // 22
    CUSTOMER_LiPR,          // 23
    CUSTOMER_RETAIL,        // 24
    CUSTOMER_NETBR,         // 25
    CUSTOMER_FLOW,          // 26
    CUSTOMER_MASTER,        // 27
    CUSTOMER_CELLCOM,       // 28
    CUSTOMER_KDG,           // 29
    CUSTOMER_MEDIACOM,      // 30
    CUSTOMER_VOO,           // 31
    CUSTOMER_ONO,           // 32 
    CUSTOMER_CVARGENTINA,   // 33
    CUSTOMER_UNE,           // 34
    CUSTOMER_TIGO,          // 35
    CUSTOMER_IZZI,          // 36
    CUSTOMER_MAX            // 37
};

/*Function to check whether a customer wants to use a config
 *  * same as that of CUSTOMER_DEFAULT */
static inline int customerSettingsSameAsDefault(int cust_index)
{
    switch(cust_index)
    {
       case CUSTOMER_DEFAULT:
       case CUSTOMER_COX:
       case CUSTOMER_CVARGENTINA:
       case CUSTOMER_UNE:
       case CUSTOMER_TIGO:
       case CUSTOMER_IZZI:
       {
           return STATUS_OK;
       }
       default:
       {
           return STATUS_NOK;
       }
    }
}
#endif /* __ARRIS_CUSTOMERS_H__ */
