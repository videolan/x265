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

extern ppa::Base *ppabase;

struct ProfileScope
{
    ppa::EventID id;

    ProfileScope(int e) { if (ppabase) { id = ppabase->getEventId(e); ppabase->triggerStartEvent(id); } else id = 0; }
    ~ProfileScope()     { if (ppabase) ppabase->triggerEndEvent(id); }
};

}

#endif //_PPA_API_H_
