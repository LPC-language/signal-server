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

# include <String.h>

private inherit asn "/lib/util/asn";


# define ASN64	"\1\0\0\0\0\0\0\0\0"

/*
 * protobuf-encode an integer, wireType = 0
 */
static string protoInt(int value)
{
    string str, c;

    str = "";
    c = ".";
    while (value & -128) {
	c[0] = 0x80 | (value & 0x7f);
	value >>= 7;
	str += c;
    }

    c[0] = value;
    return str + c;
}

/*
 * protobuf-encode an ASN, wireType = 0
 */
static string protoAsn(string value)
{
    int len;
    string str, c;

    if ((len=strlen(value)) > 9 || (len == 9 && value[0] != '\0')) {
	error("ASN too large");
    }
    str = "";
    c = ".";
    while (len != 1) {
	c[0] = 0x80 | (value[len - 1] & 0x7f);
	str += c;
	value = asn_rshift(value, 7);
	len = strlen(value);
    }

    return str + value;
}

/*
 * protobuf-encode a 64 bit entity, wireType = 1
 */
static string protoFixed64(string str)
{
    return (str + "\0\0\0\0\0\0\0")[.. 7];
}

/*
 * protobuf-encode time, wireType = 1
 */
static string protoTime(int time, float mtime)
{
    string str;

    str = asn::reverse(asn_add(asn_mult("\0" + asn::encode(time), "\x03\xe8",
					ASN64),
			       asn::encode((int) (mtime * 1000.0)), ASN64));
    return protoFixed64(str);
}

/*
 * protobuf-encode a string, wireType = 2
 */
static string protoString(string str)
{
    return protoInt(strlen(str)) + str;
}

/*
 * protobuf-encode a StringBuffer, wireType = 2
 */
static StringBuffer protoStrbuf(StringBuffer str)
{
    StringBuffer chunk;

    chunk = new StringBuffer(protoInt(str->length()));
    chunk->append(str);

    return chunk;
}

/*
 * protobuf-encode a 32 bit entity, wireType = 5
 */
static string protoFixed32(string str)
{
    return (str + "\0\0\0")[.. 3];
}

/*
 * parse a byte
 */
static mixed *parseByte(StringBuffer chunk, string buf, int offset)
{
    if (!buf || offset >= strlen(buf)) {
	buf = chunk->chunk();
	if (!buf) {
	    return ({ -1, buf, offset });
	}
	offset = 0;
    }

    return ({ buf[offset], buf, offset + 1 });
}

/*
 * parse an integer
 */
static mixed *parseInt(StringBuffer chunk, string buf, int offset)
{
    int c, value, shift;

    if (!buf) {
	buf = chunk->chunk();
	offset = 0;
    }

    value = shift = 0;
    do {
	if (offset >= strlen(buf)) {
	    buf = chunk->chunk();
	    offset = 0;
	}
	c = buf[offset++];
	value |= (c & 0x7f) << shift;
	shift += 7;
    } while (c & 0x80);

    return ({ value, buf, offset });
}

/*
 * parse an ASN
 */
static mixed *parseAsn(StringBuffer chunk, string buf, int offset)
{
    int c, shift;
    string value, b;

    if (!buf) {
	buf = chunk->chunk();
	offset = 0;
    }

    value = "\0";
    b = ".";
    shift = 0;
    do {
	if (offset >= strlen(buf)) {
	    buf = chunk->chunk();
	    offset = 0;
	}
	c = buf[offset++];
	b[0] = c & 0x7f;
	value = asn_add(value, asn_lshift(b, shift, ASN64), ASN64);
	shift += 7;
    } while (c & 0x80);

    return ({ value, buf, offset });
}

/*
 * get a number of bytes from a chunk
 */
static mixed *parseBytes(StringBuffer chunk, string buf, int offset, int len)
{
    string str;

    if (!buf) {
	buf = chunk->chunk();
	offset = 0;
    }

    str = "";
    while (len > strlen(buf) - offset) {
	len -= strlen(buf) - offset;
	str += buf[offset ..];
	buf = chunk->chunk();
	offset = 0;
    }

    return ({ str + buf[offset .. offset + len - 1], buf, offset + len });
}

/*
 * parse a 64 bit entity
 */
static mixed *parseFixed64(StringBuffer chunk, string buf, int offset)
{
    return parseBytes(chunk, buf, offset, 8);
}

/*
 * parse time
 */
static mixed *parseTime(StringBuffer chunk, string buf, int offset)
{
    string str;

    ({ str, buf, offset }) = parseFixed64(chunk, buf, offset);
    str = asn::reverse(str);
    return ({
	asn::decode(asn_div(str, "\x03\xe8", ASN64)),
	(float) asn::decode(asn_mod(str, "\x03\xe8")) / 1000.0,
	buf,
	offset
    });
}

/*
 * parse a string
 */
static mixed *parseString(StringBuffer chunk, string buf, int offset)
{
    int len;

    ({ len, buf, offset }) = parseInt(chunk, buf, offset);
    return parseBytes(chunk, buf, offset, len);
}

/*
 * parse a StringBuffer
 */
static mixed *parseStrbuf(StringBuffer chunk, string buf, int offset)
{
    int len;
    StringBuffer str;

    ({ len, buf, offset }) = parseInt(chunk, buf, offset);
    str = new StringBuffer;
    while (len > strlen(buf) - offset) {
	len -= strlen(buf) - offset;
	str->append(buf[offset ..]);
	buf = chunk->chunk();
	offset = 0;
    }

    str->append(buf[offset .. offset + len - 1]);
    return ({ str, buf, offset + len });
}

/*
 * parse a 32 bit entity
 */
static mixed *parseFixed32(StringBuffer chunk, string buf, int offset)
{
    return parseBytes(chunk, buf, offset, 4);
}

/*
 * nothing left to parse?
 */
static int parseDone(StringBuffer chunk, string buf, int offset)
{
    return (offset >= strlen(buf) && chunk->length() == 0);
}
