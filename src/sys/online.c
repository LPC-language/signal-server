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


# include "KVstoreObj.h"


object endpoints;	/* connected WebSocket endpoints */

/*
 * initialize online registry
 */
static void create()
{
    endpoints = new KVstoreObj(194);
}

/*
 * register WebSocket endpoint
 */
void register(string id, int deviceId, object endpoint)
{
    endpoints[deviceId + id] = endpoint;
}

/*
 * get any active WebSocket endpoint
 */
object present(string id, int deviceId)
{
    return endpoints[deviceId + id];
}
