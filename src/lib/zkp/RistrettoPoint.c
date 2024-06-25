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


private string bytes;		/* canonical representation */

/*
 * initialize ristretto point
 */
static void create(string bytes)
{
    ::bytes = bytes;
}

/*
 * add another ristretto point
 */
RistrettoPoint add(RistrettoPoint point)
{
    bytes = ristretto255_add(bytes, point->bytes());
    return this_object();
}

/*
 * subtract another ristretto point
 */
RistrettoPoint sub(RistrettoPoint point)
{
    bytes = ristretto255_sub(bytes, point->bytes());
    return this_object();
}

/*
 * negate
 */
RistrettoPoint neg()
{
    bytes = ristretto255_neg(bytes);
    return this_object();
}

/*
 * multiply with a scalar
 */
RistrettoPoint mult(Scalar scalar)
{
    bytes = ristretto255_mult(bytes, scalar->bytes());
    return this_object();
}

/*
 * equal to another point?
 */
int equals(RistrettoPoint point)
{
    return (bytes == point->bytes());
}

/*
 * point + point
 */
static RistrettoPoint operator+ (RistrettoPoint point)
{
    return copy_object()->add(point);
}

/*
 * point - point
 * -point
 */
static RistrettoPoint operator- (varargs RistrettoPoint point)
{
    if (point) {
	return copy_object()->sub(point);
    } else {
	return copy_object()->neg();
    }
}

/*
 * point * point
 */
static RistrettoPoint operator* (Scalar scalar)
{
    return copy_object()->mult(scalar);
}


string bytes()		{ return bytes; }
