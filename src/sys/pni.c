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

# include <KVstore.h>
# include "account.h"
# include "services.h"

private inherit "/lib/util/random";


object pni;	/* phoneNumber : Id */
object ipn;	/* id : phoneNumber */

/*
 * initialize PNI server
 */
static void create()
{
    pni = new KVstore(100);
    ipn = new KVstore(100);
}

/*
 * add a new phoneNumber : ID
 */
private atomic string add(string phoneNumber)
{
    string id;

    for (;;) {
	id = random_string(16);
	try {
	    ipn->add(id, phoneNumber);
	} catch (...) {
	    continue;
	}
	break;
    }
    pni[phoneNumber] = id;

    return id;
}

/*
 * get PNI for phone number
 */
string getId(string phoneNumber)
{
    string id;

    id = pni[phoneNumber];
    return (id) ? id : add(phoneNumber);
}

/*
 * get phone number for PNI
 */
string getPhoneNumber(string id)
{
    return ipn[id];
}
