/*
 * This file is part of https://github.com/LPC-language/signal-server
 * Copyright (C) 2024-2025 Dworkin B.V.  All rights reserved.
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

# include "Sho.h"
# include "zkp.h"
# include "params.h"

private inherit "/lib/util/random";
private inherit hex "/lib/util/hex";


/*
 * generate a UUID
 */
static string generate()
{
    string uuid;

    uuid = random_string(16);
    uuid[6] = 0x40 + (uuid[6] & 0x0f);
    uuid[8] = 0x80 + (uuid[8] & 0x3f);

    return uuid;
}

/*
 * encode as UUID
 */
static string encode(string uuid)
{
    if (!uuid || strlen(uuid) != 16) {
	error("Bad UUID");
    }
    return hex::format(uuid[.. 3]) + "-" +
	   hex::format(uuid[4 .. 5]) + "-" +
	   hex::format(uuid[6 .. 7]) + "-" +
	   hex::format(uuid[8 .. 9]) + "-" +
	   hex::format(uuid[10 ..]);
}

/*
 * decode a UUID
 */
static string decode(string uuid)
{
    if (!uuid || strlen(uuid) != 36 || uuid[8] != '-' || uuid[13] != '-' ||
	uuid[18] != '-' || uuid[23] != '-') {
	error("Bad UUID");
    }
    return hex::decodeString(uuid[.. 7]) +
	   hex::decodeString(uuid[9 .. 12]) +
	   hex::decodeString(uuid[14 .. 17]) +
	   hex::decodeString(uuid[19 .. 22]) +
	   hex::decodeString(uuid[24 ..]);
}

/*
 * calculate M1, M2 from UUID
 */
static RistrettoPoint *points(string uuid)
{
    Sho sho;
    RistrettoPoint M1, M2;
    string bytes;

    sho = PARAMS->uuidSho();
    sho->absorb(uuid);
    sho->ratchet();
    M1 = sho->getPoint();
    bytes = hash_string("SHA256", uuid);
    bytes = bytes[.. 7] + uuid + bytes[24 .. 31];
    bytes[0] &= 0xfe;
    bytes[31] &= 0x3f;
    M2 = new RistrettoPoint(ristretto255_map(bytes));

    return ({ M1, M2 });
}
