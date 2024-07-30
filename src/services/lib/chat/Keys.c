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
register(CHAT_SERVER, "PUT", "/v2/keys/signed?identity=aci",
	 "putKeysSignedAci", argHeaderAuth(), argEntityJson());
register(CHAT_SERVER, "GET", "/v2/keys/{}/{}",
	 "getKeys", argHeaderAuth(), argHeader("Unidentified-Access-Key"));

# else

# include <Continuation.h>
# include "~HTTP/HttpResponse.h"
# include "rest.h"
# include "account.h"

inherit RestServer;
private inherit uuid "~/lib/uuid";


/*
 * add keys to the server
 */
static void putKeysAci(string context, Account account, Device device,
		       mapping entity)
{
    mapping signedPreKey;

    account->updateIdentityKey(entity["identityKey"]);
    signedPreKey = entity["signedPreKey"];
    device->updateSignedPreKey(signedPreKey["keyId"], signedPreKey["publicKey"],
			       signedPreKey["signature"]);

    new Continuation("putKeys2", account->id(), device->id(), entity["preKeys"])
	->add("respondJsonOK", context)
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
static void putKeysPni(string context, Account account, Device device,
		       mapping entity)
{
    mapping signedPreKey;

    account->updatePniKey(entity["identityKey"]);
    signedPreKey = entity["signedPreKey"];
    device->updateSignedPniPreKey(signedPreKey["keyId"],
				  signedPreKey["publicKey"],
				  signedPreKey["signature"]);

    new Continuation("putKeys2", account->pni(), device->id(),
		     entity["preKeys"])
	->add("respondJsonOK", context)
	->runNext();
}

/*
 * update signed key
 */
static void putKeysSignedAci(string context, Account account, Device device,
			     mapping entity)
{
    mapping signedPreKey;

    device->updateSignedPreKey(entity["keyId"], entity["publicKey"],
			       entity["signature"]);
    respond(context, HTTP_OK, nil, nil, nil);
}

/*
 * get keys from server
 */
static void getKeys(string context, string uuid, string deviceId,
		    Account account, Device device, string accessKey)
{
    new Continuation("getKeys2", uuid::decode(uuid), deviceId)
	->chain("respondJson", context, HTTP_OK)
	->runNext();
}

/*
 * get keys
 */
static mapping getKeys2(string id, string deviceId)
{
    Account account;
    mapping results, *devices;
    mixed *keys;
    int i, size;
    Device device;
    mixed *signedPreKey;

    account = ACCOUNT_SERVER->get(id);
    results = ([ "identityKey" : account->identityKey() ]);

    if (deviceId == "*") {
	keys = KEYS_SERVER->takeKeys(id);
    } else {
	i = (int) deviceId;
	keys = ({ ({ i }) + KEYS_SERVER->takeKey(id, i) });
    }

    size = sizeof(keys);
    results["devices"] = devices = allocate(size);
    for (i = 0; i < size; i++) {
	device = account->device(keys[i][0]);
	signedPreKey = device->signedPreKey();
	devices[i] = ([
	    "deviceId" : device->id(),
	    "registrationId" : device->registrationId(),
	    "signedPreKey" : ([
		"keyId" : signedPreKey[0],
		"publicKey" : signedPreKey[1],
		"signature" : signedPreKey[2]
	    ]),
	    "preKey" : ([
		"keyId" : keys[i][1],
		"publicKey" : keys[i][2]
	    ])
	]);
    }

    return results;
}

# endif
