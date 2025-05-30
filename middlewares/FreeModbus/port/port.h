/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: port.h ,v 1.60 2013/08/13 15:07:05 Armink add Master Functions $
 */

#ifndef _PORT_H
#define _PORT_H

#include "mbconfig.h"

#include "stdint.h"
#include "gr_includes.h"
#include "cmsis_os2.h"
#include "app_log.h"

#define INLINE
#define PR_BEGIN_EXTERN_C        extern "C" {

#define APP_TIMER_WAIT_FOR_QUEUE 2

#define assert_param(x)          gr_assert_param(x)

static uint32_t regPrimask;

__STATIC_INLINE uint32_t ENTER_CRITICAL_SECTION(void)
{
    uint32_t regPrimask = __get_PRIMASK();
    __disable_irq();
}

__STATIC_INLINE void EXIT_CRITICAL_SECTION(void)
{
    __set_PRIMASK(regPrimask);
}

typedef uint8_t BOOL;

typedef unsigned char UCHAR;
typedef char          CHAR;

typedef uint16_t USHORT;
typedef int16_t  SHORT;

typedef uint32_t ULONG;
typedef int32_t  LONG;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define PR_END_EXTERN_C }
#endif
