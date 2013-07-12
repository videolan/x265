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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#ifndef _PPA_H_
#define _PPA_H_

#if !defined(ENABLE_PPA)

#define PPA_INIT()
#define PPAStartCpuEventFunc(e)
#define PPAStopCpuEventFunc(e)
#define PPAScopeEvent(e)

#else

/* declare enum list of users CPU events */
#define PPA_REGISTER_CPU_EVENT(x) x,
enum PPACpuEventEnum
{
#include "ppaCPUEvents.h"
    PPACpuGroupNums
};

#undef PPA_REGISTER_CPU_EVENT

#define PPA_INIT()               initializePPA()
#define PPAStartCpuEventFunc(e)  if (ppabase) ppabase->triggerStartEvent(ppabase->getEventId(e))
#define PPAStopCpuEventFunc(e)   if (ppabase) ppabase->triggerEndEvent(ppabase->getEventId(e))
#define PPAScopeEvent(e)         _PPAScope __scope_(e)

#include "ppaApi.h"

void initializePPA();
extern ppa::Base *ppabase;

class _PPAScope
{
protected:

    ppa::EventID m_id;

public:

    _PPAScope(int e) { if (ppabase) { m_id = ppabase->getEventId(e); ppabase->triggerStartEvent(m_id); } }

    ~_PPAScope()     { if (ppabase) ppabase->triggerEndEvent(m_id); }
};

#endif // if !defined(ENABLE_PPA)

#endif /* _PPA_H_ */
