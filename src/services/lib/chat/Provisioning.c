/*
 * This file is part of https://github.com/LPC-language/signal-server
 * Copyright (C) 2025 Dworkin B.V.  All rights reserved.
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

register(CHAT_SERVER, "PUT", "/v1/provisioning/{}", "putProvisioning",
	 argHeaderAuth(), argEntityJson());

# else

# include "~HTTP/HttpResponse.h"
# include "rest.h"
# include "account.h"
# include "provisioning.h"

inherit RestServer;
private inherit base64 "/lib/util/base64";
private inherit "~/lib/proto";


static int putProvisioning(string context, string address, Account account,
			   Device device, mapping entity)
{
    string body;
    object endpoint;

    body = entity["body"];
    endpoint = PROVISIONING->getEndpoint(address);
    if (body && endpoint) {
	call_out_other(endpoint, "provisioningMessage", 0,
		       base64::decode(body));
	return respondJsonOK(context);
    } else {
	return respond(context, HTTP_NOT_FOUND, nil, nil);
    }
}

# endif
