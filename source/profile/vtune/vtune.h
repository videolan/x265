/*****************************************************************************
 * Copyright (C) 2015 x265 project
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

#ifndef VTUNE_H
#define VTUNE_H

#include "ittnotify.h"

namespace X265_NS {

#define CPU_EVENT(x) x,
enum VTuneTasksEnum
{
#include "../cpuEvents.h"
    NUM_VTUNE_TASKS
};
#undef CPU_EVENT

extern __itt_domain* domain;
extern __itt_string_handle* taskHandle[NUM_VTUNE_TASKS];

struct VTuneScopeEvent
{
    VTuneScopeEvent(int e) { __itt_task_begin(domain, __itt_null, __itt_null, taskHandle[e]); }
    ~VTuneScopeEvent()     { __itt_task_end(domain); }
};

void vtuneInit();
void vtuneSetThreadName(const char *name, int id);

}

#endif
