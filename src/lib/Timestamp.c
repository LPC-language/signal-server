/*
 * This file is part of https://github.com/LPC-language/Signal-Server
 * Copyright (C) 2024 Dworkin B.V.  All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

# include <Time.h>
# include <type.h>

inherit Time;


/*
 * create a millisecond timestamp
 */
static void create(varargs mixed val)
{
    int time;
    float mtime;

    switch (typeof(val)) {
    case T_INT:
	time = val;
	mtime = (float) (time % 1000) / 1000.0;
	time /= 1000;
	break;

    case T_STRING:
	time = (int) val[.. strlen(val) - 4];
	mtime = (float) val[strlen(val) - 3 ..] / 1000.0;
	break;

    case T_NIL:
	({ time, mtime }) = millitime();
	break;
    }
    ::create(time, mtime);
}

/*
 * export as a string
 */
string transport()
{
    int time;
    float mtime, ftime;
    string tail;

    time = time();
    mtime = mtime();

    ftime = (time < 0) ? (float) time + 4294967296.0 : (float) time;
    tail = "00000" + (int) ((fmod(ftime, 1000.0) + mtime) * 1000.0);
    return (string) (int) floor(ftime / 1000.0) + tail[strlen(tail) - 6 ..];
}
