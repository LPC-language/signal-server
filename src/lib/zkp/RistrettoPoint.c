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


private string bytes;		/* canonical representation */

/*
 * initialize ristretto point
 */
static void create(string bytes)
{
    ::bytes = bytes;
}

/*
 * point + point
 */
static RistrettoPoint operator+ (RistrettoPoint point)
{
    return new RistrettoPoint(ristretto255_add(bytes, point->bytes()));
}

/*
 * point - point
 * -point
 */
static RistrettoPoint operator- (varargs RistrettoPoint point)
{
    if (point) {
	return new RistrettoPoint(ristretto255_sub(bytes, point->bytes()));
    } else {
	return new RistrettoPoint(ristretto255_neg(bytes));
    }
}

/*
 * point * point
 */
static RistrettoPoint operator* (Scalar scalar)
{
    return new RistrettoPoint(ristretto255_mult(bytes, scalar->bytes()));
}

/*
 * equal to another point?
 */
int equals(RistrettoPoint point)
{
    return (bytes == point->bytes());
}


string bytes()		{ return bytes; }
