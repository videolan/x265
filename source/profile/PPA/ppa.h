/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef PPA_H
#define PPA_H

/* declare enum list of users CPU events */
#define CPU_EVENT(x) x,
enum PPACpuEventEnum
{
#include "../cpuEvents.h"
    PPACpuGroupNums
};
#undef CPU_EVENT

#include "ppaApi.h"

void initializePPA();

#define PPA_INIT()               initializePPA()
#define PPAScopeEvent(e)         ppa::ProfileScope ppaScope_(e)

#endif /* PPA_H */
