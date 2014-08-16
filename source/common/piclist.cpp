/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@multicorewareinc.com>
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

#include "common.h"
#include "piclist.h"
#include "frame.h"

using namespace x265;

void PicList::pushFront(Frame& pic)
{
    X265_CHECK(!pic.m_next && !pic.m_prev, "piclist: picture already in list\n"); // ensure pic is not in a list
    pic.m_next = m_start;
    pic.m_prev = NULL;

    if (m_count)
    {
        m_start->m_prev = &pic;
        m_start = &pic;
    }
    else
    {
        m_start = m_end = &pic;
    }
    m_count++;
}

void PicList::pushBack(Frame& pic)
{
    X265_CHECK(!pic.m_next && !pic.m_prev, "piclist: picture already in list\n"); // ensure pic is not in a list
    pic.m_next = NULL;
    pic.m_prev = m_end;

    if (m_count)
    {
        m_end->m_next = &pic;
        m_end = &pic;
    }
    else
    {
        m_start = m_end = &pic;
    }
    m_count++;
}

Frame *PicList::popFront()
{
    if (m_start)
    {
        Frame *temp = m_start;
        m_count--;

        if (m_count)
        {
            m_start = m_start->m_next;
            m_start->m_prev = NULL;
        }
        else
        {
            m_start = m_end = NULL;
        }
        temp->m_next = temp->m_prev = NULL;
        return temp;
    }
    else
        return NULL;
}

Frame* PicList::getPOC(int poc)
{
    Frame *pic = m_start;
    while (pic && pic->getPOC() != poc)
        pic = pic->m_next;
    return pic;
}

Frame *PicList::popBack()
{
    if (m_end)
    {
        Frame* temp = m_end;
        m_count--;

        if (m_count)
        {
            m_end = m_end->m_prev;
            m_end->m_next = NULL;
        }
        else
        {
            m_start = m_end = NULL;
        }
        temp->m_next = temp->m_prev = NULL;
        return temp;
    }
    else
        return NULL;
}

void PicList::remove(Frame& pic)
{
#if _DEBUG
    Frame *tmp = m_start;
    while (tmp && tmp != &pic)
    {
        tmp = tmp->m_next;
    }

    X265_CHECK(tmp == &pic, "piclist: pic being removed was not in list\n"); // verify pic is in this list
#endif

    m_count--;
    if (m_count)
    {
        if (m_start == &pic)
            m_start = pic.m_next;
        if (m_end == &pic)
            m_end = pic.m_prev;

        if (pic.m_next)
            pic.m_next->m_prev = pic.m_prev;
        if (pic.m_prev)
            pic.m_prev->m_next = pic.m_next;
    }
    else
    {
        m_start = m_end = NULL;
    }

    pic.m_next = pic.m_prev = NULL;
}
