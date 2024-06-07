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

private inherit "~TLS/api/lib/hkdf";
private inherit hex "/lib/util/hex";


string userDeriveKey;	/* 32 bytes */
string key;

/*
 * credentials server
 */
static void create()
{
    /* XXX */
    userDeriveKey = secure_random(32);
    key = secure_random(32);
}

void initialize(string userDeriveKey, string key)
{
    ::userDeriveKey = userDeriveKey;
    ::key = key;
}

string *generate(string id, int derive, int truncate, int prepend)
{
    int time;
    string data, signature;

    if (derive) {
	id = hex::format(HMAC(userDeriveKey, id, "SHA256")[.. 9]);
    }
    time = time();
    data = id + ":" + time;
    signature = HMAC(key, data, "SHA256");
    if (truncate) {
	signature = signature[.. 9];
    }
    return ({ id, ((prepend) ? data : time) + ":" + hex::format(signature) });
}
