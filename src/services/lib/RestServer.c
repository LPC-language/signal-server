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

# include <String.h>
# include "~HTTP/HttpRequest.h"
# include "~HTTP/HttpResponse.h"
# include "~HTTP/HttpField.h"
# include "rest.h"
# include "account.h"

inherit RestServer;
private inherit "/lib/util/ascii";
private inherit base64 "/lib/util/base64";
private inherit uuid "~/lib/uuid";


/*
 * substitute argument
 */
static mixed substArg(HttpRequest request, StringBuffer entity, mixed *args,
		      int i, mixed *handle)
{
    mixed arg;
    HttpAuthentication auth;
    string str, password;
    int deviceId;

    arg = args[i];
    if (arg == ARG_HEADER_AUTH) {
	if (i != 0) {
	    error("Authorization must be first");
	}

	try {
	    auth = request->headerValue("Authorization");
	    if (!auth || lower_case(auth->scheme()) != "basic" ||
		sscanf(base64::decode(auth->authentication()), "%s:%s", str,
		       password) != 2) {
		error("Bad authentication");
	    }
	    deviceId = 1;
	    sscanf(str, "%s.%d", str, deviceId);
	    str = uuid::decode(str);
	} catch (...) {
	    return respond(HTTP_BAD_REQUEST, nil, nil);
	}

	call_out("authCall", 0, str, deviceId, password, args[1 ..], handle);
	return 0;
    }

    return ::substArg(request, entity, args, i, handle);
}

/*
 * authenticated call
 */
static void authCall(string accountId, int deviceId, string password,
		     mixed *args, mixed *handle)
{
    Account account;
    Device device;

    account = ACCOUNT_SERVER->get(accountId);
    if (account) {
	device = account->device(deviceId);
	if (device && device->verifyPassword(password)) {
	    call_other(this_object(), handle[0],
		       (handle[1] + ({ account, device }) + args)...);
	    return;
	}
    }

    respond(HTTP_UNAUTHORIZED, nil, nil);
}

static void respondJsonOK()
{
    respondJson(HTTP_OK, ([ ]));
}
