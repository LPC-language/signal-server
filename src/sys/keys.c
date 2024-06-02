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


object keys;	/* accountId : keys */

/*
 * initialize key server
 */
static void create()
{
    keys = new KVstore(100);
}

/*
 * store keys
 */
atomic void store(string id, int deviceId, mapping *preKeys)
{
    mapping deviceMap, pkMap, pk;
    int i;

    deviceMap = keys[id];
    if (!deviceMap) {
	keys[id] = deviceMap = ([ ]);
    }

    pkMap = ([ ]);
    for (i = sizeof(preKeys); --i >= 0; ) {
	pk = preKeys[i];
	pkMap[pk["keyId"]] = pk["publicKey"];
    }
    deviceMap[deviceId] = pkMap;
}

/*
 * take one key
 */
mixed *take(string id, int deviceId)
{
    mapping pkMap;
    int *indices, keyId;
    string pubKey;

    pkMap = keys[id][deviceId];
    indices = map_indices(pkMap);
    if (sizeof(indices) == 0) {
	return nil;
    }
    keyId = indices[0];
    pubKey = pkMap[keyId];
    pkMap[keyId] = nil;

    return ({ keyId, pubKey });
}

/*
 * count keys
 */
int count(string id, int deviceId)
{
    return map_sizeof(keys[id][deviceId]);
}

/*
 * delete all keys for ID
 */
void deleteUuid(string id)
{
    keys[id] = nil;
}

/*
 * delete all keys for device
 */
void deleteDevice(string id, int deviceId)
{
    keys[id][deviceId] = nil;
}
