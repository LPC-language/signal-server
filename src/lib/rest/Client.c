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
# include "~HTTP/HttpConnection.h"
# include "~HTTP/HttpRequest.h"
# include "~HTTP/HttpResponse.h"
# include "~HTTP/HttpField.h"
# include "services.h"
# include "rest.h"
# include <config.h>
# include <version.h>
# include <status.h>
# include <type.h>

inherit "~/lib/websocket";
private inherit "/lib/util/ascii";
private inherit json "/lib/util/json";
private inherit "/lib/util/random";
private inherit "~/lib/proto";


private object connection;	/* TLS connection */
private HttpResponse response;	/* most recent response */
private mixed handle;		/* call handle */
private StringBuffer chunks;	/* collected chunks */
private string websocket;	/* WebSocket service */
private int opcode, flags;	/* opcode and flags of last WebSocket frame */
private mapping outgoing;	/* context : callback */

/*
 * initialize
 */
static object create(string address, int port, string function,
		     varargs string httpClientPath, string tlsClientSessionPath)
{
    if (status(OBJECT_PATH(RestTlsClientSession), O_INDEX) == nil) {
	compile_object(OBJECT_PATH(RestTlsClientSession));
    }

    outgoing = ([ ]);

    if (!httpClientPath) {
	httpClientPath = HTTP1_TLS_CLIENT;
    }
    if (!tlsClientSessionPath) {
	tlsClientSessionPath = OBJECT_PATH(RestTlsClientSession);
    }

    connection = clone_object(httpClientPath, this_object(), address, port,
			      address, nil, nil, tlsClientSessionPath);
    handle = function;

    return connection;
}

/*
 * connection establised
 */
void connected()
{
    if (previous_object() == connection) {
	call_other(this_object(), handle, HTTP_OK);
    }
}

/*
 * connection failed
 */
void connectFailed(int errorcode)
{
    if (previous_object() == connection) {
	call_other(this_object(), handle, errorcode);
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
    ::wsSendChunk(connection, chunk, 0x12345678);
}

/*
 * close WebSocket connection
 */
static void sendClose(string code)
{
    if (!websocket) {
	error("Not a WebSocket connection");
    }
    ::wsSendClose(connection, code, 0x12345678);
}

/*
 * send a HTTP request
 */
private void httpRequest(string method, string host, string path, string type,
			 StringBuffer entity, mapping extraHeaders)
{
    HttpRequest request;
    HttpFields headers;
    string *indices;
    mixed *values;
    int i;

    request = new HttpRequest(1.1, method, "https://", nil, path);
    headers = new HttpFields();
    headers->add(new HttpField("Host", host));
    headers->add(new HttpField("User-Agent", ({
	new HttpProduct(APPLICATION_NAME, APPLICATION_VERSION),
	new HttpProduct(SERVER_NAME, SERVER_VERSION),
	new HttpProduct(explode(status(ST_VERSION), " ")...)
    })));
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
    request->setHeaders(headers);

    connection->sendRequest(request);
    if (entity) {
	connection->sendChunk(entity);
    }
}

/*
 * send a request
 */
static void request(string method, string host, string path, string type,
		    StringBuffer entity, mapping extraHeaders,
		    string function, varargs mixed arguments...)
{
    if (websocket) {
	string context;

	if (function) {
	    do {
		context = random_string(8);
	    } while (outgoing[context]);
	    outgoing[context] = ({ function }) + arguments;
	} else {
	    context = "\1";
	}
	sendChunk(wsRequest(method, path, entity, extraHeaders, context));
    } else {
	httpRequest(method, host, path, type, entity, extraHeaders);
	handle = ({ function }) + arguments;
    }
}

/*
 * handle a REST request or response
 */
private void call(string context, object parsed, string function, mixed *args,
		  StringBuffer entity)
{
    int i;
    mixed arg;

    for (i = sizeof(args); --i >= 0; ) {
	if (typeof(args[i]) == T_STRING) {
	    arg = parsed->headerValue(args[i]);
	    if (typeof(arg) == T_ARRAY && sizeof(arg) == 1) {
		arg = arg[0];
	    }
	    args[i] = arg;
	} else {
	    switch (args[i]) {
	    case ARG_ENTITY:
		args[i] = entity;
		break;

	    case ARG_ENTITY_JSON:
		arg = parsed->headerValue("Content-Type");
		args[i] = (!arg || lower_case(arg) == "application/json") ?
			   json::decode(entity->chunk()) : nil;
		break;
	    }
	}
    }

    call_other(this_object(), function, context, parsed, args...);
}

/*
 * handle a response
 */
static void _receiveResponse(HttpResponse response, object prev)
{
    mixed length;

    if (prev == connection) {
	::response = response;

	if (response->headerValue("Transfer-Encoding")) {
	    chunks = new StringBuffer;
	    connection->expectChunk();
	} else {
	    length = response->headerValue("Content-Length");
	    switch (typeof(length)) {
	    case T_NIL:
		call(nil, response, handle[0], handle[1 ..], nil);
		break;

	    case T_INT:
		if (length > REST_LENGTH_LIMIT) {
		    error("Content-Length too large");
		}
		if (sizeof(handle & ({ ARG_ENTITY_JSON })) != 0 &&
		    length > 65535) {
		    error("JSON entity too large");
		}
		connection->expectEntity(length);
		break;

	    default:
		error("Bad Content-Length");
	    }
	}
    }
}

/*
 * flow: handle a response
 */
void receiveResponse(HttpResponse response)
{
    call_out("_receiveResponse", 0, response, previous_object());
}

/*
 * receive a chunk
 */
static void _receiveChunk(StringBuffer chunk, HttpFields trailers, object prev)
{
    if (prev == connection) {
	if (chunk) {
	    chunks->append(chunk);
	    connection->expectChunk();
	} else {
	    chunk = chunks;
	    chunks = nil;
	    call(nil, response, handle[0], handle[1 ..], chunk);
	}
    }
}

/*
 * flow: receive a chunk
 */
void receiveChunk(StringBuffer chunk, HttpFields trailers)
{
    call_out("_receiveChunk", 0, chunk, trailers, previous_object());
}

/*
 * receive entity
 */
static void _receiveEntity(StringBuffer entity, object prev)
{
    if (prev == connection) {
	call(nil, response, handle[0], handle[1 ..], entity);
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
 * upgrade to WebSocket protocol
 */
static void upgradeToWebSocket(string service)
{
    websocket = service;
    if (connection) {
	connection->expectWsFrame();
    }
}

/*
 * send a websocket response
 */
static void respond(string context, int code, StringBuffer entity,
		    mapping extraHeaders)
{
    sendChunk(wsResponse(context, code, entity, extraHeaders));
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
 * flow: receive WebSocket frame
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
    mixed *callback;

    ({ context, request, body }) = wsReceiveRequest(chunk, "client");

    callback = REST_API->lookup("client", request->method(), request->path());
    if (!callback) {
	respond(context, HTTP_NOT_FOUND, nil, nil);
    } else {
	call(context, request, callback[0], callback[2 ..], body);
    }
}

/*
 * receive websocket response
 */
private void receiveWsResponse(StringBuffer chunk)
{
    string context;
    HttpResponse response;
    StringBuffer body;
    mixed *callback;

    ({ context, response, body }) = wsReceiveResponse(chunk);
    callback = outgoing[context];
    if (callback) {
	outgoing[context] = nil;
	call(context, response, callback[0], callback[1 ..], body);
    }
}

/*
 * receive WebSocket chunk
 */
static void _receiveWsChunk(StringBuffer chunk, object prev)
{
    int c, offset;
    string buf;

    if (previous_object() == connection) {
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
		receiveWsResponse(chunk);
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
 * finished handling response
 */
static void doneResponse()
{
    if (connection) {
	if (!websocket) {
	    connection->doneResponse();
	} else if (opcode == WEBSOCK_CLOSE) {
	    connection->terminate();
	}
    }
}

/*
 * finished sending chunk
 */
static void _doneChunk(object prev)
{
    if (prev == connection && opcode == WEBSOCK_CLOSE) {
	connection->terminate();
    }
}

/*
 * flow: finished sending chunk
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
