/*
 * File             : ppa.h
 *                    ( C++ header file )
 *
 * Description      : Parallel Path Analyzer
 *                    ( This is a source of the PPA SDK. )
 *                    Include this file in your application.
 *
 * Copyright        : MulticoreWare Inc.
 *                    ( http://www.multicorewareinc.com )
 *                    Copyright (c) 2011 MulticoreWare Inc. All rights reserved
 *
 *
 * License          : PPA Software License v0.1
 *
 *
 * This software is governed by MulticoreWare Inc. Permission to use, reproduce,
 * copy, modify, display, distribute, execute and transmit this software
 * for any purpose and any use of its accompanying documentation is hereby granted.
 * Provided that the above copyright notice appear in all copies, and that
 * this software is not used in advertising or publicity without specific, written
 * prior permission by MulticoreWare Inc, and that the following disclaimer is
 * included.
 *
 *
 * THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF DESIGN, MERCHANTABILITY
 * OR FITNESS FOR A PARTICULAR PURPOSE, AND WITHOUT WARRANTY AS TO NON-INFRINGEMENT
 * OR THE PERFORMANCE OR RESULTS YOU MAY OBTAIN BY USING THE SOFTWARE.
 */

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
