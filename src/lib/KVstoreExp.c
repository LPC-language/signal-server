/*
 * This file is part of https://github.com/LPC-language/signal-server
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

# include <KVstore.h>

inherit KVstore;


private int duration;	/* K/V duration */

/*
 * KVstore with expiration
 */
static void create(int maxSize, int duration)
{
    ::create(maxSize, nil, "/usr/MsgServer/obj/kvnode_exp");
    ::duration = duration;
}

/*
 * get K/V
 */
mixed get(string key)
{
    mixed value;

    value = ::get(key);
    return (value) ? value[1] : nil;
}

/*
 * get duration
 */
int getDuration(string key)
{
    mixed value;

    value = ::get(key);
    return (value) ? value[0] - time() : 0;
}

/*
 * set K/V
 */
void set(string key, mixed value)
{
    ::set(key, ({ time() + duration, value }));
}

/*
 * add K/V
 */
void add(string key, mixed value)
{
    ::add(key, ({ time() + duration, value }));
}

/*
 * change K/V
 */
void change(string key, mixed value)
{
    ::change(key, ({ time() + duration, value }));
}
