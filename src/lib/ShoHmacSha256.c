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

inherit "Sho";
inherit "~TLS/api/lib/hkdf";


static string hashRatchet(string cv, string input)
{
    return HMAC(cv, input + "\0", "SHA256");
}

static string hashSqueeze1(string cv, string input)
{
    return HMAC(cv, input + "\1", "SHA256");
}

static string hashSqueeze2(string cv, string input)
{
    return HMAC(cv, input + "\2", "SHA256");
}