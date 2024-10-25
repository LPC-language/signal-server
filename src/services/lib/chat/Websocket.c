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

# ifdef REGISTER

register(CHAT_SERVER, "GET", "/v1/websocket/",
	 "getWebsocket", argHeader("Upgrade"), argHeader("Connection"),
	 argHeader("Sec-WebSocket-Key"), argHeader("Sec-WebSocket-Version"));
register(CHAT_SERVER, "GET", "/v1/websocket/{}",
	 "getWebsocketLogin", argHeader("Upgrade"), argHeader("Connection"),
	 argHeader("Sec-WebSocket-Key"), argHeader("Sec-WebSocket-Version"));

# else

# include <String.h>
# include <Continuation.h>
# include "~HTTP/HttpResponse.h"
# include "~HTTP/HttpField.h"
# include "rest.h"
# include "account.h"
# include "messages.h"
# include <type.h>

inherit RestServer;
private inherit base64 "/lib/util/base64";
private inherit json "/lib/util/json";
private inherit "/lib/util/random";
private inherit uuid "~/lib/uuid";

private mapping outgoing;	/* context : callback */

/*
 * initialize websocket layer
 */
static void create()
{
    outgoing = ([ ]);
}

static int getWebsocket(string context, string upgrade, string connection,
			string key, string version)
{
    if (upgrade == "websocket" && connection == "Upgrade" && key &&
	version == "13") {
	return upgradeToWebSocket("chat", key);
    } else {
	return respond(context, HTTP_BAD_REQUEST, nil, nil);
    }
}

static int getWebsocketLogin(string context, string param, string upgrade,
			     string connection, string key, string version)
{
    string login, password, id;
    int deviceId;

    if (sscanf(param, "?login=%s&password=%s", login, password) != 2) {
	return respond(context, HTTP_BAD_REQUEST, nil, nil);
    }

    if (upgrade == "websocket" && connection == "Upgrade" && key &&
	version == "13") {
	id = login;
	deviceId = 1;
	sscanf(id, "%s.%d", id, deviceId);
	id = uuid::decode(id);
	new Continuation("getWebsocketLogin2", id, deviceId, password)
	    ->chain("getWebsocketLogin3", context, key, login, password,
		    id, deviceId)
	    ->runNext();
    } else {
	return respond(context, HTTP_BAD_REQUEST, nil, nil);
    }
}

static int getWebsocketLogin2(string id, int deviceId, string password)
{
    Account account;
    Device device;

    account = ACCOUNT_SERVER->get(id);
    return (account &&
	    (device=account->device(deviceId)) &&
	     device->verifyPassword(password));
}

static void getWebsocketLogin3(string context, string key, string login,
			       string password, string id, int deviceId,
			       int success)
{
    if (success) {
	upgradeToWebSocket("chat", key, login, password);
	ONLINE_REGISTRY->register(id, deviceId, this_object());
	call_out_other(MESSAGE_SERVER, "processEnvelopes", 0, id, deviceId,
		       this_object());
    } else {
	respond(context, HTTP_UNAUTHORIZED, nil, nil);
    }
}

/*
 * send a WebSocket request
 */
void chatSendRequest(string verb, string path, StringBuffer body,
		     mapping extraHeaders, Continuation cont,
		     varargs mixed arguments...)
{
    string context;
    StringBuffer chunk;

    if (cont) {
	/* expect a response */
	do {
	    context = random_string(8);
	} while (outgoing[context]);
	outgoing[context] = ({ cont }) + arguments;
    } else {
	context = "\1";		/* no response */
    }
    sendChunk(wsRequest(verb, path, body, extraHeaders, context));
}

/*
 * receive WebSocket response
 */
static void chatReceiveResponse(StringBuffer chunk)
{
    string context;
    HttpResponse response;
    StringBuffer entity;
    mixed *handle;

    ({ context, response, entity }) = wsReceiveResponse(chunk);

    handle = outgoing[context];
    outgoing[context] = nil;
    if (handle) {
	mixed *args;
	int i;

	args = handle[1 ..];
	for (i = sizeof(args); --i >= 0; ) {
	    if (typeof(args[i]) == T_STRING) {
		args[i] = response->headerValue(args[i]);
	    } else {
		switch (args[i]) {
		case ARG_ENTITY:
		    args[i] = entity;
		    break;

		case ARG_ENTITY_JSON:
		    args[i] = (entity) ? json::decode(entity->chunk()) : nil;
		    break;
		}
	    }
	}

	handle[0]->runNext(response->code(), args...);
    }
}

# endif
