/*
    inout.cpp


    flexemu, an MC6809 emulator running FLEX
    Copyright (C) 1997-2018  W. Schwotzer

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "misc1.h"
#include "keyboard.h"


KeyboardIO::KeyboardIO()
{
    reset_parallel();
}

void KeyboardIO::reset_parallel()
{
    std::lock_guard<std::mutex> guard(parallel_mutex);
    key_buffer_parallel.clear();
}

void KeyboardIO::put_char_parallel(Byte key, bool &do_notify)
{
    std::lock_guard<std::mutex> guard(parallel_mutex);
    bool was_empty = key_buffer_parallel.empty();
    key_buffer_parallel.push_back(key);
    if (was_empty)
    {
        do_notify = true;
    }
}

bool KeyboardIO::has_key_parallel()
{
    bool result;

    std::lock_guard<std::mutex> guard(parallel_mutex);
    result = !key_buffer_parallel.empty();

    return result;
}

// Read character and remove it from the queue.
// Input should always be polled before read_char_parallel.
Byte KeyboardIO::read_char_parallel(bool &do_notify)
{
    Byte result = 0x00;

    std::lock_guard<std::mutex> guard(parallel_mutex);
    if (!key_buffer_parallel.empty())
    {
        result = key_buffer_parallel.front();
        key_buffer_parallel.pop_front();

        // If there are still characters in the
        // buffer set CA1 flag again.
        if (!key_buffer_parallel.empty())
        {
            do_notify = true;
        }
    }

    return result;
}

// Read character, but leave it in the queue.
// Input should always be polled before read_queued_ch.
Byte KeyboardIO::peek_char_parallel()
{
    Byte result = 0x00;

    std::lock_guard<std::mutex> guard(parallel_mutex);
    if (!key_buffer_parallel.empty())
    {
        result = key_buffer_parallel.front();
    }

    return result;
}

void KeyboardIO::set_bell(Word /*x_percent*/)
{
#ifdef _WIN32
    Beep(400, 100);
#endif
#ifdef UNIX
    static char bell = BELL;

    ssize_t count = write(fileno(stdout), &bell, 1);
    (void)count; // satisfy compiler
#endif
}

void KeyboardIO::put_value(unsigned int x_keyMask)
{
    keyMask = x_keyMask;
}

void KeyboardIO::get_value(unsigned int *x_keyMask)
{
    if (x_keyMask != nullptr)
    {
        *x_keyMask = keyMask;
    }
}