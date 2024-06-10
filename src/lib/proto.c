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

private inherit asn "/lib/util/asn";


/*
 * protobuf-encode an integer
 */
static string protoInt(int value)
{
    string str, c;

    str = "";
    c = ".";
    while (value & -128) {
	c[0] = 128 | (value & 127);
	value >>= 7;
	str += c;
    }

    c[0] = value;
    return str + c;
}

/*
 * protobuf-encode a string
 */
static string protoString(string str)
{
    return protoInt(strlen(str)) + str;
}

/*
 * protobuf-encode time
 */
static string protoTime(int time, float mtime)
{
    string str;

    str = "\1\0\0\0\0\0\0\0\0";
    str = asn::reverse(asn_add(asn_mult(asn::encode(time), "\x03\xe8", str),
			       asn::encode((int) (mtime * 1000.0)), str));
    return (str + "\0\0\0\0\0\0\0")[.. 7];
}
