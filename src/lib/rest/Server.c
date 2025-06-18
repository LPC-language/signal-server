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

# include <String.h>
# include "~HTTP/HttpRequest.h"
# include "~HTTP/HttpResponse.h"
# include "~HTTP/HttpConnection.h"
# include "~HTTP/HttpField.h"
# include "services.h"
# include "rest.h"
# include "account.h"
# include "credentials.h"
# include "~/config/services"
# include <config.h>
# include <version.h>
# include <status.h>
# include <type.h>

inherit "~/lib/websocket";
private inherit "/lib/util/ascii";
private inherit base64 "/lib/util/base64";
private inherit json "/lib/util/json";
private inherit uuid "~/lib/uuid";
private inherit "~/lib/proto";


private object connection;	/* TLS connection */
private HttpRequest request;	/* most recent request */
private mixed *handle;		/* call handle */
private string login, password;	/* websocket authentication */
private string websocket;	/* WebSocket service */
private int opcode, flags;	/* opcode and flags of last WebSocket frame */

/*
 * establish connection
 */
void init(object connection)
{
    if (!::connection) {
	::connection = connection;
    }
}

/*
 * send a StringBuffer chunk via WebSocket
 */
static void sendChunk(StringBuffer chunk)
{
    if (!websocket) {
	error("Not a WebSocket connection");
    }
    ::wsSendChunk(connection, chunk);
}

/*
 * close WebSocket connection
 */
static void sendClose(string code)
{
    if (!websocket) {
	error("Not a WebSocket connection");
    }
    ::wsSendClose(connection, code);
}

/*
 * send a HTTP response
 */
private void httpRespond(int code, string type, StringBuffer entity,
			 mapping extraHeaders)
{
    HttpResponse response;
    HttpFields headers;
    string *indices;
    mixed *values;
    int i;

    response = new HttpResponse(1.1, code, comment(code));
    headers = new HttpFields();
    headers->add(new HttpField("Date", new HttpTime));
    headers->add(new HttpField("Server", ({
	new HttpProduct(APPLICATION_NAME, APPLICATION_VERSION),
	new HttpProduct(SERVER_NAME, SERVER_VERSION),
	new HttpProduct(explode(status(ST_VERSION), " ")...)
    })));
    if (code == HTTP_BAD_REQUEST || code == HTTP_NOT_FOUND ||
	code == HTTP_CONTENT_TOO_LARGE) {
	headers->add(new HttpField("Connection", ({ "close" })));
    }
    if (entity) {
	headers->add(new HttpField("Content-Type", type));
	headers->add(new HttpField("Content-Length", entity->length()));
    }
    if (extraHeaders) {
	indices = map_indices(extraHeaders);
	values = map_values(extraHeaders);
	for (i = sizeof(indices); --i >= 0; ) {
	    headers->add(new HttpField(indices[i], values[i]));
	}
    }
    response->setHeaders(headers);

    connection->sendResponse(response);
    if (entity) {
	connection->sendChunk(entity);
    }
}

/*
 * send a response
 */
static int respond(string context, int code, string type, StringBuffer entity,
		   varargs mapping extraHeaders)
{
    request = nil;
    handle = nil;
    if (websocket) {
	if (type) {
	    if (!extraHeaders) {
		extraHeaders = ([ ]);
	    }
	    extraHeaders["Content-Type"] = type;
	}
	sendChunk(wsResponse(context, code, entity, extraHeaders));
    } else {
	httpRespond(code, type, entity, extraHeaders);
    }

    return code;
}

/*
 * respond with JSON body
 */
static int respondJson(string context, int code, mapping entity,
		       varargs mapping extraHeaders)
{
    return respond(context, code, "application/json;charset=utf-8",
		   new StringBuffer(json::encode(entity)), extraHeaders);
}

/*
 * respond OK with empty JSON body
 */
static int respondJsonOK(string context, varargs mapping extraHeaders)
{
    return respondJson(context, HTTP_OK, ([ ]), extraHeaders);
}

/*
 * substitute argument
 */
static mixed substArg(string context, HttpRequest request, StringBuffer entity,
		      mixed *args, int i, mixed *handle)
{
    mixed arg;
    HttpAuthentication auth;
    string str, password;
    int deviceId;

    arg = args[i];
    switch (arg) {
    case ARG_HEADER_AUTH:
    case ARG_HEADER_OPT_AUTH:
	if (i != 0) {
	    error("Authorization must be first");
	}

	try {
	    auth = request->headerValue("Authorization");
	    if (!auth) {
		if (login) {
		    str = login;
		    password = ::password;
		} else if (arg == ARG_HEADER_OPT_AUTH) {
		    call_out(handle[0], 0, context,
			     (handle[1] + ({ nil, nil }) + args[1 ..])...);
		    return 0;
		} else {
		    return respond(context, HTTP_BAD_REQUEST, nil, nil);
		}
	    } else if (lower_case(auth->scheme()) != "basic" ||
		       sscanf(base64::decode(auth->authentication()), "%s:%s",
			      str, password) != 2) {
		return respond(context, HTTP_BAD_REQUEST, nil, nil);
	    }

	    deviceId = 1;
	    sscanf(str, "%s.%d", str, deviceId);
	    str = uuid::decode(str);
	    call_out("authCall", 0, context, str, deviceId, password,
		     args[1 ..], handle);
	    return 0;
	} catch (...) {
	    return respond(context, HTTP_BAD_REQUEST, nil, nil);
	}

    case ARG_ENTITY:
	return entity;

    case ARG_ENTITY_JSON:
	try {
	    arg = request->headerValue("Content-Type");
	    if (lower_case(arg) != "application/json") {
		error("Content-Type not JSON");
	    }
	    return json::decode(entity->chunk());
	} catch (...) {
	    return respond(context, HTTP_BAD_REQUEST, nil, nil);
	}
	break;

    default:
	if (typeof(arg) == T_STRING) {
	    arg = request->headerValue(arg);
	    if (typeof(arg) == T_ARRAY && sizeof(arg) == 1) {
		arg = arg[0];
	    }
	    return arg;
	}
	error("Unkown argument type");
    }
}

/*
 * handle a REST call
 */
private int call(string context, HttpRequest request, StringBuffer entity,
		 mixed *handle)
{
    mixed *args, arg;
    int i;

    args = handle[2 ..];
    for (i = sizeof(args); --i >= 0; ) {
	arg = substArg(context, request, entity, args, i, handle);
	switch (typeof(arg)) {
	case T_INT:
	    return arg;

	case T_ARRAY:
	    args = args[.. i - 1] + arg + args[i + 1 ..];
	    break;

	default:
	    args[i] = arg;
	    break;
	}
    }

    return call_other(this_object(), handle[0], context, (handle[1] + args)...);
}

/*
 * authenticated call
 */
static void authCall(string context, string accountId, int deviceId,
		     string password, mixed *args, mixed *handle)
{
    Account account;
    Device device;

    account = ACCOUNT_SERVER->get(accountId);
    if (account) {
	device = account->device(deviceId);
	if (device && device->verifyPassword(password)) {
	    call_other(this_object(), handle[0], context,
		       (handle[1] + ({ account, device }) + args)...);
	    return;
	}
    }

    respond(context, HTTP_UNAUTHORIZED, nil, nil);
}

/*
 * handle a request
 */
static int _receiveRequest(int code, HttpRequest request, object prev)
{
    string host, str;
    mixed length;

    if (prev == connection) {
	::request = request;

	if (code != 0) {
	    return respond(nil, code, nil, nil);
	}

	host = request->host();
	if (!host) {
	    return respond(nil, HTTP_BAD_REQUEST, nil, nil);
	}
	str = previous_object()->host();
	if (str && lower_case(str) != lower_case(host)) {
	    return respond(nil, HTTP_BAD_REQUEST, nil, nil);
	}

	str = request->scheme();
	if (str && str != "https://") {
	    return respond(nil, HTTP_NOT_FOUND, nil, nil);
	}

	if (request->headerValue("Transfer-Encoding")) {
	    return respond(nil, HTTP_CONTENT_TOO_LARGE, nil, nil);
	}

	handle = REST_API->lookup(host, request->method(), request->path());
	if (!handle) {
	    return respond(nil, HTTP_NOT_FOUND, nil, nil);
	}

	length = request->headerValue("Content-Length");
	switch (typeof(length)) {
	case T_NIL:
	    return call(nil, request, nil, handle);

	case T_INT:
	    if (length > REST_LENGTH_LIMIT) {
		return respond(nil, HTTP_CONTENT_TOO_LARGE, nil, nil);
	    }
	    if (sizeof(handle & ({ ARG_ENTITY_JSON })) != 0 && length > 65535) {
		return respond(nil, HTTP_CONTENT_TOO_LARGE, nil, nil);
	    }
	    connection->expectEntity(length);
	    break;

	default:
	    return respond(nil, HTTP_BAD_REQUEST, nil, nil);
	}
    }
}

/*
 * flow: handle a request
 */
void receiveRequest(int code, HttpRequest request)
{
    call_out("_receiveRequest", 0, code, request, previous_object());
}

/*
 * receive entity
 */
static void _receiveEntity(StringBuffer entity, object prev)
{
    if (prev == connection) {
	call(nil, request, entity, handle);
    }
}

/*
 * flow: receive entity
 */
void receiveEntity(StringBuffer entity)
{
    call_out("_receiveEntity", 0, entity, previous_object());
}

/*
 * authenticate for an account
 */
static void authenticate(string login, string password)
{
    ::login = login;
    ::password = password;
}

/*
 * upgrade connection to WebSocket protocol
 */
static int upgradeToWebSocket(string service, string key, varargs string login,
			      string password)
{
    int code;

    code = respond(nil, HTTP_SWITCHING_PROTOCOLS, nil, nil, ([
	"Upgrade" : ({ "websocket" }),
	"Connection" : ({ "Upgrade" }),
	"Sec-WebSocket-Accept" :
		    base64::encode(hash_string("SHA1", key + WEBSOCKET_GUID))
    ]));

    websocket = service;
    authenticate(login, password);

    if (connection) {
	connection->expectWsFrame();
    }

    return code;
}

/*
 * receive WebSocket frame
 */
static void _receiveWsFrame(int opcode, int flags, int len, object prev)
{
    if (prev == connection) {
	::opcode = opcode;
	::flags = flags;
    }
}

/*
 * receive WebSocket frame
 */
void receiveWsFrame(int opcode, int flags, int len)
{
    call_out("_receiveWsFrame", 0, opcode, flags, len, previous_object());
}

/*
 * receive WebSocket request
 */
private void receiveWsRequest(StringBuffer chunk)
{
    string context;
    HttpRequest request;
    StringBuffer body;
    mixed *handle;

    ({ context, request, body }) = wsReceiveRequest(chunk, CHAT_SERVER);

    handle = REST_API->lookup(CHAT_SERVER, request->method(), request->path());
    if (!handle) {
	respond(context, HTTP_NOT_FOUND, nil, nil);
    } else {
	call(context, request, body, handle);
    }
}

/*
 * receive WebSocket chunk
 */
static void _receiveWsChunk(StringBuffer chunk, object prev)
{
    int c, offset;
    string buf;

    if (prev == connection) {
	if (opcode == WEBSOCK_CLOSE) {
	    wsSendClose(connection, chunk->chunk());
	} else if (websocket == "chat") {
	    ({ c, buf, offset }) = parseByte(chunk, nil, 0);
	    if (c != 010) {
		error("WebSocketMessage.type expected");
	    }
	    ({ c, buf, offset }) = parseInt(chunk, buf, offset);
	    switch (c) {
	    case 1:	/* request */
		({ c, buf, offset }) = parseByte(chunk, buf, offset);
		if (c != 022) {
		    error("WebSockMessage.request expected");
		}
		({ chunk, buf, offset }) = parseStrbuf(chunk, buf, offset);
		receiveWsRequest(chunk);
		break;

	    case 2:	/* response */
		({ c, buf, offset }) = parseByte(chunk, buf, offset);
		if (c != 032) {
		    error("WebSockMessage.response expected");
		}
		({ chunk, buf, offset }) = parseStrbuf(chunk, buf, offset);
		call_other(this_object(), "chatReceiveResponse", chunk);
		break;

	    default:
		error("Unknown websocket message");
	    }
	} else {
	    call_other(this_object(), websocket + "ReceiveChunk", chunk);
	}

	if (connection) {
	    connection->expectWsFrame();
	}
    }
}

/*
 * flow: receive WebSocket chunk
 */
void receiveWsChunk(StringBuffer chunk)
{
    call_out("_receiveWsChunk", 0, chunk, previous_object());
}

/*
 * finished sending response
 */
static void _doneChunk(object prev)
{
    if (prev == connection) {
	if (!websocket) {
	    connection->doneRequest();
	} else if (opcode == WEBSOCK_CLOSE) {
	    connection->terminate();
	}
    }
}

/*
 * flow: finished sending response
 */
void doneChunk()
{
    call_out("_doneChunk", 0, previous_object());
}

/*
 * break the connection
 */
static void terminate()
{
    if (connection) {
	connection->terminate();
    }
}

/*
 * close connection
 */
static void close()
{
    destruct_object(this_object());
}

/*
 * cleanup after TLS connection ends
 */
static void _disconnected(object prev)
{
    if (prev == connection) {
	close();
    }
}

/*
 * flow: cleanup after TLS connection ends
 */
void disconnected()
{
    call_out("_disconnected", 0, previous_object());
}
