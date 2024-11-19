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


object keys;	/* accountId : keys */

/*
 * initialize key server
 */
static void create()
{
    keys = new KVstore(199);
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
mixed *takeKey(string id, int deviceId)
{
    mapping pkMap;
    int *keyIds, keyId;
    string pubKey;

    pkMap = keys[id][deviceId];
    keyIds = map_indices(pkMap);
    if (sizeof(keyIds) == 0) {
	return nil;
    }
    keyId = keyIds[0];
    pubKey = pkMap[keyId];
    pkMap[keyId] = nil;

    return ({ keyId, pubKey });
}

/*
 * take one key for every device
 */
atomic mixed **takeKeys(string id)
{
    mapping devMap, *devices, pkMap;
    int *deviceIds, *keyIds, keyId, size, i;
    mixed **result;
    string pubKey;

    devMap = keys[id];
    deviceIds = map_indices(devMap);
    devices = map_values(devMap);
    result = ({ });
    for (size = sizeof(devices), i = 0; i < size; i++) {
	pkMap = devices[i];
	keyIds = map_indices(pkMap);
	if (sizeof(keyIds) != 0) {
	    keyId = keyIds[0];
	    pubKey = pkMap[keyId];
	    pkMap[keyId] = nil;
	    result += ({ ({ deviceIds[i], keyId, pubKey }) });
	}
    }

    return result;
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
