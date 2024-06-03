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

register(CHAT_SERVER, "PUT", "/v1/accounts/gcm/",
	 "putAccountsGcm", argHeader("Authorization"), argEntityJson());

# else

# include <String.h>
# include <Continuation.h>
# include "~HTTP/HttpResponse.h"
# include "~HTTP/HttpField.h"
# include "rest.h"
# include "account.h"

inherit RestServer;
private inherit "/lib/util/ascii";
private inherit base64 "/lib/util/base64";
private inherit uuid "~/lib/uuid";


/*
 * set gcmId for device
 */
static int putAccountsGcm(HttpAuthentication authorization, mapping entity)
{
    string uuid;
    int deviceId;

    if (!authorization || lower_case(authorization->scheme()) != "basic") {
	return respond(HTTP_BAD_REQUEST, nil, nil);
    }
    sscanf(base64::decode(authorization->authentication()), "%s:", uuid);
    deviceId = 1;
    sscanf(uuid, "%s.%d", uuid, deviceId);
    uuid = uuid::decode(uuid);

    new Continuation("putAccountsGcm2", uuid, deviceId,
		     entity["gcmRegistrationId"])
	->add("putAccountsGcm3")
	->runNext();
}

static void putAccountsGcm2(string accountId, int deviceId, string gcmId)
{
    Account account;
    Device device;

    account = ACCOUNT_SERVER->get(accountId);
    device = account->device(deviceId);
    if (gcmId != device->gcmId()) {
	device->setGcmId(gcmId);
	device->setFetchesMessages(FALSE);
    }
}

static void putAccountsGcm3()
{
    respond(HTTP_OK, "application/json;charset=utf-8", new StringBuffer("{}"));
}

# endif
