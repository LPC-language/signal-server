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
# include "protocol.h"

inherit ProfileKeyCredentialRequest;


static void create(string blob)
{
    if (strlen(blob) != 329 || blob[161 .. 168] != "\xa0\0\0\0\0\0\0\0") {
	error("Bad ProfileKeyCredentialRequest");
    }

    ::create(new RistrettoPoint(blob[1 .. 32]),
	     new RistrettoPoint(blob[33 .. 64]),
	     new RistrettoPoint(blob[65 .. 96]),
	     new RistrettoPoint(blob[97 .. 128]),
	     new RistrettoPoint(blob[129 .. 160]),
	     blob[169 ..]);
}
