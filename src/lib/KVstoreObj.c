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

# include <KVstore.h>

inherit KVstore;


/*
 * KVstore with object values
 */
static void create(int maxSize)
{
    ::create(maxSize, nil, "/usr/MsgServer/obj/kvnode_obj");
}

/*
 * set K/V
 */
void set(string key, object value)
{
    ::set(key, ({ value }));
}

/*
 * add K/V
 */
void add(string key, object value)
{
    ::add(key, ({ value }));
}

/*
 * change K/V
 */
void change(string key, object value)
{
    ::change(key, ({ value }));
}
