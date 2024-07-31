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
private inherit asn "/lib/util/asn";
private inherit "~/lib/proto";
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
	MESSAGE_SERVER->processEnvelopes(id, deviceId, this_object());
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
    StringBuffer request, chunk;
    string context, *indices;
    mixed *values;
    int sz, i;

    request = new StringBuffer("\12" + protoString(verb) +
			       "\22" + protoString(path));
    if (body) {
	request->append("\32");
	request->append(protoStrbuf(body));
    }
    if (cont) {
	/* expect a response */
	do {
	    context = random_string(8);
	} while (outgoing[context]);
	outgoing[context] = ({ cont }) + arguments;
    } else {
	context = "\1";		/* no response */
    }
    request->append("\40");
    request->append(protoAsn(context));

    if (extraHeaders) {
	indices = map_indices(extraHeaders);
	values = map_values(extraHeaders);
	for (sz = sizeof(indices), i = 0; i < sz; i++) {
	    request->append(
		"\52" + protoString(indices[i] + ": " +
				    ((typeof(values[i]) == T_ARRAY) ?
				      values[i][0] : values[i]))
	    );
	}
    }

    chunk = new StringBuffer("\010\001\022");
    chunk->append(protoStrbuf(request));
    sendChunk(chunk);
}

/*
 * receive WebSocket response
 */
static void chatReceiveResponse(StringBuffer chunk)
{
    int c, offset, code;
    string buf, id, message, headers, header;
    StringBuffer entity;
    HttpResponse response;
    mixed *handle;

    ({ c, buf, offset }) = parseByte(chunk, nil, 0);
    if (c != 010) {
	error("WebSocketResponseMessage.id expected");
    }
    ({ id, buf, offset }) = parseAsn(chunk, buf, offset);
    id = asn::unsignedExtend(id, 8);
    ({ c, buf, offset }) = parseByte(chunk, buf, offset);
    if (c != 020) {
	error("WebSocketResponseMessage.status expected");
    }
    ({ code, buf, offset }) = parseInt(chunk, buf, offset);
    ({ c, buf, offset }) = parseByte(chunk, buf, offset);
    if (c != 032) {
	error("WebSocketResponseMessage.message expected");
    }
    ({ message, buf, offset }) = parseString(chunk, buf, offset);

    if (!parseDone(chunk, buf, offset)) {
	({ c, buf, offset }) = parseByte(chunk, buf, offset);
	switch (c) {
	case 042:
	    ({ entity, buf, offset }) = parseStrbuf(chunk, buf, offset);
	    if (parseDone(chunk, buf, offset)) {
		break;
	    }
	    ({ c, buf, offset }) = parseByte(chunk, buf, offset);
	    if (c != 052) {
		error("WebSocketResponseMessage.headers expected");
	    }
	    /* fall through */
	case 052:
	    headers = "";
	    for (;;) {
		({ header, buf, offset }) = parseString(chunk, buf, offset);
		headers += header + "\n";
		if (parseDone(chunk, buf, offset)) {
		    break;
		}

		({ c, buf, offset }) = parseByte(chunk, buf, offset);
		if (c != 052) {
		    error("WebSocketResponseMessage.headers expected");
		}
	    }
	    break;

	default:
	    error("WebSocketResponseMessage.headers expected");
	}
    }

    response = new HttpResponse(1.1, code, message);
    if (headers) {
	response->setHeaders(new RemoteHttpFields(headers));
    }

    handle = outgoing[id];
    outgoing[id] = nil;
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
