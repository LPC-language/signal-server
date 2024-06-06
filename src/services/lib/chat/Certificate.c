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

register(CHAT_SERVER, "GET", "/v1/certificate/delivery",
	 "getCertificateDelivery", argHeader("Authorization"));

# else

# include <String.h>
# include <Continuation.h>
# include "~HTTP/HttpResponse.h"
# include "~HTTP/HttpField.h"
# include "rest.h"
# include "account.h"
# include "certificate.h"

inherit RestServer;
private inherit "/lib/util/ascii";
private inherit base64 "/lib/util/base64";
private inherit json "/lib/util/json";
private inherit uuid "~/lib/uuid";


/*
 * get SenderCertificate
 */
static int getCertificateDelivery(HttpAuthentication authorization)
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

    new Continuation("getCertificateDelivery1", uuid)
	->chain("getCertificateDelivery2", deviceId)
	->runNext();
}

/*
 * get account
 */
static Account getCertificateDelivery1(string accountId)
{
    return ACCOUNT_SERVER->get(accountId);
}

/*
 * get certificate for account/device/phonenumber
 */
static void getCertificateDelivery2(int deviceId, Account account)
{
    string str;

    str = base64::encode(CERT_SERVER->generate(account, deviceId,
					       account->phoneNumber()));
    respondJson(HTTP_OK, ([ "certificate" : str ]));
}

# endif
