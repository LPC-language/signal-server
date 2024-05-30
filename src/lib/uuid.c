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

private inherit hex "/lib/util/hex";


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
