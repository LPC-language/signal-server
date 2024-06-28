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

# include "zkp.h"

private inherit asn "/lib/util/asn";


# define ELL ("\x10\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x14\xde\xf9\xde" +	\
	      "\xa2\xf7\x9c\xd6\x58\x12\x63\x1a\x5c\xf5\xd3\xed")

private string number;	/* internal big-endian ASN of scalar */

/*
 * initialize scalar from little-endian bytes
 */
static void create(string bytes, varargs int internal)
{
    number = (internal) ? bytes : asn_mod("\0" + asn::reverse(bytes), ELL);
}

/*
 * scalar + scalar
 */
static Scalar operator+ (Scalar scalar)
{
    return new Scalar(asn_add(number, scalar->internal(), ELL), TRUE);
}

/*
 * scalar - scalar
 * -scalar
 */
static Scalar operator- (varargs Scalar scalar)
{
    if (scalar) {
	string bytes;

	bytes = asn_sub(number, scalar->internal(), ELL);
	if (bytes[0] & 0x80) {
	    bytes = asn_add(bytes, ELL, ELL);
	}
	return new Scalar(bytes, TRUE);
    } else {
	return new Scalar(asn_sub(ELL, number, ELL), TRUE);
    }
}

/*
 * scalar * scalar
 */
static Scalar operator* (Scalar scalar)
{
    return new Scalar(asn_mult(number, scalar->internal(), ELL), TRUE);
}

/*
 * equal to another scalar?
 */
int equals(Scalar scalar)
{
    return (number == scalar->internal());
}


string bytes()		{ return asn::reverse(asn::extend(number, 32)); }
string internal()	{ return number; }
