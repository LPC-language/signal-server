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

register(CHAT_SERVER, "PUT", "/v1/profile/",
	 "putProfile", argHeaderAuth(), argEntityJson());
register(CHAT_SERVER, "GET", "/v1/profile/{}/{}/{}",
	 "getProfileKeyCredential", argHeaderOptAuth(),
	 argHeader("Unidentified-Access-Key"));

# else

# include <Continuation.h>
# include "~HTTP/HttpResponse.h"
# include "rest.h"
# include "account.h"
# include "protocol.h"

inherit RestServer;
private inherit base64 "/lib/util/base64";
private inherit hex "/lib/util/hex";
private inherit uuid "~/lib/uuid";


/*
 * upload profile
 */
static void putProfile(string context, Account account, Device device,
		       mapping entity)
{
    /* XXX ignore badges */
    new Continuation("putProfile2", account->id(), entity)
	->add("respondJsonOK", context)
	->runNext();
}

static void putProfile2(string accountId, mapping entity)
{
    Profile profile;

    /* XXX ignore avatar */
    profile = PROFILE_SERVER->get(accountId,
				  hex::decodeString(entity["version"]));
    profile->update(entity["name"], nil, entity["aboutEmoji"], entity["about"],
		    entity["paymentAddress"],
		    base64::decode(entity["commitment"]));
}

static int getProfileKeyCredential(string context, string uuid, string version,
				   string credentialRequest, Account account,
				   Device device, string accessKey)
{
    if (sscanf(credentialRequest, "%s?credentialType=expiringProfileKey",
	       credentialRequest) == 0) {
	return respond(context, HTTP_BAD_REQUEST, nil, nil);
    }

    /* XXX permitted? */
    new Continuation("getProfileKeyCredential2", account->id(),
		     hex::decodeString(version),
		     hex::decodeString(credentialRequest))
	->chain("respondJson", context, HTTP_OK)
	->runNext();
}

static mapping getProfileKeyCredential2(string accountId, string version,
				        string credentialRequest)
{
    Profile profile;
    ProfileKeyCommitment commitment;
    ProfileKeyCredentialRequest request;
    int expirationTime;
    ProfileKeyCredentialResponse response;

    profile = PROFILE_SERVER->get(accountId, version);
    commitment = new RemoteProfileKeyCommitment(profile->commitment());
    request = new RemoteProfileKeyCredentialRequest(commitment,
						    credentialRequest);

    expirationTime = (time() + 7 * 86400) >> 7;
    expirationTime -= expirationTime % 675;
    expirationTime <<= 7;
    response = new ProfileKeyCredentialResponse(request, accountId,
						expirationTime,
						secure_random(32));

    return ([ "credential" : base64::encode(response->transport()) ]);
}
# endif
