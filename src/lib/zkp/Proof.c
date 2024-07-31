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


private Scalar challenge;
private Scalar *response;

/*
 * initialize proof
 */
static void create(Scalar challenge, Scalar *response)
{
    ::challenge = challenge;
    ::response = response;
}

/*
 * export as a blob
 */
string transport()
{
    int size, i;
    string *arr;

    size = sizeof(response);
    arr = allocate(size);
    for (i = 0; i < size; i++) {
	arr[i] = response[i]->bytes();
    }

    return challenge->bytes() + implode(arr, "");
}


Scalar challenge()	{ return challenge; }
Scalar *response()	{ return response[..]; }
