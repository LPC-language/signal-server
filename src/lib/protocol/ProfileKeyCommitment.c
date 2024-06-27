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


private RistrettoPoint J1, J2, J3;

static void create(RistrettoPoint J1, RistrettoPoint J2, RistrettoPoint J3)
{
    ::J1 = J1;
    ::J2 = J2;
    ::J3 = J3;
}

string transport()
{
    return "\0" + J1->bytes() + J2->bytes() + J3->bytes();
}


RistrettoPoint J1()	{ return J1; }
RistrettoPoint J2()	{ return J2; }
RistrettoPoint J3()	{ return J3; }
