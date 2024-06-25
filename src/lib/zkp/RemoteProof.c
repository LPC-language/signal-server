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

inherit Proof;


/*
 * initialize a proof from bytes
 */
static void create(string blob)
{
    int len, num, i;
    Scalar challenge, *response;

    len = strlen(blob);
    num = len / 32;
    if (num < 2 || num > 256 || (len & 31)) {
	error("Bad proof");
    }

    challenge = new Scalar(blob[0 .. 31]);
    response = allocate(num - 1);
    for (i = 1; i < num; i++) {
	response[i - 1] = new Scalar(blob[i * 32 .. i * 32 + 31]);
    }
    ::create(challenge, response);
}
