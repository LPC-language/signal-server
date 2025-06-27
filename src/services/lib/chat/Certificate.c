/*
 * This file is part of https://github.com/LPC-language/signal-server
 * Copyright (C) 2024-2025 Dworkin B.V.  All rights reserved.
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
	 "getCertificateDelivery", argHeaderAuth());
register(CHAT_SERVER, "GET", "/v1/certificate/auth/{}",
	 "getCertificateAuth", argHeaderAuth());

# else

# include "~HTTP/HttpResponse.h"
# include "rest.h"
# include "account.h"
# include "certificate.h"
# include "protocol.h"

inherit RestServer;
private inherit base64 "/lib/util/base64";
private inherit uuid "~/lib/uuid";


/*
 * get SenderCertificate
 */
static void getCertificateDelivery(string context, Account account,
				   Device device)
{
    call_out("getCertificateDelivery2", 0, context, account, device->id());
}

static void getCertificateDelivery2(string context, Account account,
				    int deviceId)
{
    respondJson(context, HTTP_OK, ([
	"certificate" :
	base64::encode(CERT_SERVER->generate(account, deviceId,
					     account->phoneNumber()))
    ]));
}

static void getCertificateAuth(string context, string param, Account account,
			       Device device)
{
    int start, end, time;
    string id, pni;
    object credential;
    mapping *credentials, *callLinkAuthCredentials;

    if (sscanf(param, "group?redemptionStartSeconds=%d&redemptionEndSeconds=%d",
	       start, end) != 2) {
	respond(context, HTTP_NOT_FOUND, nil, nil);
    }

    id = account->id();
    pni = account->pni();
    credentials = ({ });
    callLinkAuthCredentials = ({ });
    for (time = start; time <= end; time += 86400) {
	credential = new AuthCredentialWithPniResponse(id, pni, time,
						       secure_random(32));
	credentials += ({ ([
	    "credential" : base64::encode(credential->transport()),
	    "redemptionTime" : time
	]) });
	credential = new CallLinkAuthCredentialResponse(id, time,
							secure_random(32)),
	callLinkAuthCredentials += ({ ([
	    "credential" : base64::encode(credential->transport()),
	    "redemptionTime" : time
	]) });
    }

    respondJson(context, HTTP_OK, ([
	"credentials" : credentials,
	"callLinkAuthCredentials" : callLinkAuthCredentials,
	"pni" : uuid::encode(pni)
    ]));
}

# endif
