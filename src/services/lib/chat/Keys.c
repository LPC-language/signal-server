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

# ifdef REGISTER

register(CHAT_SERVER, "PUT", "/v2/keys/?identity=aci",
	 "putKeysAci", argHeaderAuth(), argEntityJson());
register(CHAT_SERVER, "PUT", "/v2/keys/?identity=pni",
	 "putKeysPni", argHeaderAuth(), argEntityJson());

# else

# include <Continuation.h>
# include "rest.h"
# include "account.h"

inherit RestServer;


/*
 * add keys to the server
 */
static void putKeysAci(Account account, Device device, mapping entity)
{
    mapping signedPreKey;

    account->updateIdentityKey(entity["identityKey"]);
    signedPreKey = entity["signedPreKey"];
    device->updateSignedPreKey(signedPreKey["keyId"], signedPreKey["publicKey"],
			       signedPreKey["signature"]);

    new Continuation("putKeys2", account->id(), device->id(), entity["preKeys"])
	->add("respondJsonOK")
	->runNext();
}

/*
 * store preKeys
 */
static void putKeys2(string id, int deviceId, mapping *preKeys)
{
    KEYS_SERVER->store(id, deviceId, preKeys);
}

/*
 * add keys to the server
 */
static void putKeysPni(Account account, Device device, mapping entity)
{
    mapping signedPreKey;

    account->updatePniKey(entity["identityKey"]);
    signedPreKey = entity["signedPreKey"];
    device->updateSignedPniPreKey(signedPreKey["keyId"],
				  signedPreKey["publicKey"],
				  signedPreKey["signature"]);

    new Continuation("putKeys2", account->pni(), device->id(),
		     entity["preKeys"])
	->add("respondJsonOK")
	->runNext();
}

# endif
