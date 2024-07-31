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

# include "zkp.h"

private inherit asn "/lib/util/asn";


/*
 * Stateful Hash Objects
 */

static string hashRatchet(string cv, string input);
static string hashSqueeze1(string cv, string input);
static string hashSqueeze2(string cv, string input);

private string cv;	/* 32 "key" bytes */
private string input;	/* collected input */

/*
 * absorb input
 */
void absorb(string str)
{
    if (!input) {
	input = str;
    } else {
	input += str;
    }
}

/*
 * ratchet absorbed input
 */
void ratchet()
{
    if (input) {
	cv = hashRatchet(cv, input);
	input = nil;
    }
}

/*
 * obtain output
 */
string squeeze(int length)
{
    string output;
    int i;

    if (input) {
	error("Not ratcheted");
    }

    output = "";
    for (i = 0; strlen(output) < length; i++) {
	output += hashSqueeze1(cv, asn::extend(asn::encode(i), 8));
    }

    cv = hashSqueeze2(cv, asn::extend(asn::encode(length), 8));
    input = nil;

    return output[.. length - 1];
}

/*
 * initialize Stateful Hash Object
 */
static void create(string label)
{
    cv = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    absorb(label);
    ratchet();
}

/*
 * clone the current stateful object
 */
object clone()
{
    return copy_object();
}

/*
 * squeeze a scalar from the sho
 */
Scalar getScalar()
{
    return new Scalar(squeeze(64));
}

/*
 * squeeze a point from the sho
 */
RistrettoPoint getPoint()
{
    string bytes;

    bytes = squeeze(64);
    return new RistrettoPoint(ristretto255_add(ristretto255_map(bytes[.. 31]),
					       ristretto255_map(bytes[32 ..])));
}

/*
 * squeeze a point from the sho, simplified
 */
RistrettoPoint getPointSingleElligator()
{
    return new RistrettoPoint(ristretto255_map(squeeze(32)));
}
