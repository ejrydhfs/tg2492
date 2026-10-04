/*
 *
 * dpp_6rd_pp_ctl.h
 * Description:
 * DOCSIS Packet Processor 6RD (protocol 41) control header file
 *
 * DESCRIPTION:   Provides DOCSIS Packet Processor 6RD support(protocol 41) 
 *                configuration and control function. 6RD PP support might bring some
 *                problematic effects, the feature need to be configurable.
 *
 *                Author: Brian Liu
 *
 * (c) Copyright 2020 CommScope, Inc.
 * Copyright 2013-2019, ARRIS Enterprises, LLC
 *
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * GPLv2 license:
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
*/

#ifndef _DPP_6RD_PP_CTL_H_
#define _DPP_6RD_PP_CTL_H_

/**************************************************************************/
/*      INCLUDES                                                          */
/**************************************************************************/

/**************************************************************************/
/*      INTERFACE  Defines and Structs                                    */
/**************************************************************************/

/**************************************************************************/
/*      INTERFACE FUNCTIONS Prototypes:                                   */
/**************************************************************************/

int ti_hil_enable_6rd_pp(void);
int ti_hil_disable_6rd_pp(void);

/*! \fn int Dpp6rdPPCtl_Init(void)
 *  \brief DOCSIS Packet Processor 6RD (protocol 41) Ctl initialization.
 *  \param[in] no input.
 *  \param[out] no output.
 *  \return 0 or error code.
 */
int Dpp6rdPPCtl_Init(void);

/*! \fn int Dpp6rdPPCtl_Exit(void)
 *  \brief DOCSIS Packet Processor 6RD (protocol 41) Ctl exit.
 *  \param[in] no input.
 *  \param[out] no output.
 *  \return 0 or error code.
 */
int Dpp6rdPPCtl_Exit(void);

#endif

