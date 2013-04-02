/*
 * File             : ppaApi.h
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

#ifndef _PPA_API_H_
#define _PPA_API_H_

namespace ppa {
// PPA private namespace

typedef unsigned short EventID;
typedef unsigned char GroupID;

class Base
{
public:
    virtual ~Base() {}

    virtual bool isEventFiltered(EventID eventId) const = 0;
    virtual bool configEventById(EventID eventId, bool filtered) const = 0;
    virtual int  configGroupById(GroupID groupId, bool filtered) const = 0;
    virtual void configAllEvents(bool filtered) const = 0;
    virtual EventID  registerEventByName(const char *pEventName) = 0;
    virtual GroupID registerGroupByName(const char *pGroupName) = 0;
    virtual EventID registerEventInGroup(const char *pEventName, GroupID groupId) = 0;
    virtual void triggerStartEvent(EventID eventId) = 0;
    virtual void triggerEndEvent(EventID eventId) = 0;
    virtual void triggerTidEvent(EventID eventId, unsigned int data) = 0;
    virtual void triggerDebugEvent(EventID eventId, unsigned int data0, unsigned int data1) = 0;

    virtual EventID getEventId(int index) const = 0;

protected:
    virtual void init(const char **pNames, int eventCount) = 0;
};

}

#endif //_PPA_API_H_
