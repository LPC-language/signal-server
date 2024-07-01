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
# include "~HTTP/HttpConnection.h"
# include "~HTTP/HttpField.h"
# include "services.h"
# include "rest.h"
# include "account.h"
# include "~/config/services"
# include <config.h>
# include <version.h>
# include <status.h>
# include <type.h>

private inherit "/lib/util/ascii";
private inherit base64 "/lib/util/base64";
private inherit json "/lib/util/json";
private inherit uuid "~/lib/uuid";
private inherit "~/lib/proto";


private object connection;	/* TLS connection */
private HttpRequest request;	/* most recent request */
private mixed *handle;		/* call handle */
private string login, password;	/* websocket authentication */
private int websocket;		/* WebSocket protocol enabled? */
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
 * response code => comment
 */
private string comment(int code)
{
    switch (code) {
    case HTTP_CONTINUE:			return "Continue";
    case HTTP_SWITCHING_PROTOCOLS:	return "Switching Protocols";
    case HTTP_OK:			return "OK";
    case HTTP_CREATED:			return "Created";
    case HTTP_ACCEPTED:			return "Accepted";
    case HTTP_NON_AUTHORITATIVE_INFORMATION:
					return "Non-authoritative information";
    case HTTP_NO_CONTENT:		return "No Content";
    case HTTP_RESET_CONTENT:		return "Reset Content";
    case HTTP_PARTIAL_CONTENT:		return "Partial Content";
    case HTTP_MULTIPLE_CHOICES:		return "Multiple Choices";
    case HTTP_MOVED_PERMANENTLY:	return "Moved Permanently";
    case HTTP_FOUND:			return "Found";
    case HTTP_SEE_OTHER:		return "See Other";
    case HTTP_NOT_MODIFIED:		return "Not Modified";
    case HTTP_USE_PROXY:		return "Use Proxy";
    case HTTP_TEMPORARY_REDIRECT:	return "Temporary Redirect";
    case HTTP_PERMANENT_REDIRECT:	return "Permanent Redirect";
    case HTTP_BAD_REQUEST:		return "Bad Request";
    case HTTP_UNAUTHORIZED:		return "Unauthorized";
    case HTTP_PAYMENT_REQUIRED:		return "Payment Required";
    case HTTP_FORBIDDEN:		return "Forbidden";
    case HTTP_NOT_FOUND:		return "Not Found";
    case HTTP_METHOD_NOT_ALLOWED:	return "Method Not Allowed";
    case HTTP_NOT_ACCEPTABLE:		return "Not Acceptable";
    case HTTP_PROXY_AUTHENTICATION_REQUIRED:
					return "Proxy Authentication Required";
    case HTTP_REQUEST_TIMEOUT:		return "Request Timeout";
    case HTTP_CONFLICT:			return "Conflict";
    case HTTP_GONE:			return "Gone";
    case HTTP_LENGTH_REQUIRED:		return "Length Required";
    case HTTP_PRECONDITION_FAILED:	return "Precondition Failed";
    case HTTP_CONTENT_TOO_LARGE:	return "Content Too Large";
    case HTTP_URI_TOO_LONG:		return "URI Too Long";
    case HTTP_UNSUPPORTED_MEDIA_TYPE:	return "Unsupported Media Type";
    case HTTP_RANGE_NOT_SATISFIABLE:	return "Range Not Satisfiable";
    case HTTP_EXPECTATION_FAILED:	return "Expectation Failed";
    case HTTP_MISDIRECTED_REQUEST:	return "Misdirected Request";
    case HTTP_UNPROCESSABLE_CONTENT:	return "Unprocessable Content";
    case HTTP_UPGRADE_REQUIRED:		return "Upgrade Required";
    case HTTP_INTERNAL_ERROR:		return "Internal Error";
    case HTTP_NOT_IMPLEMENTED:		return "Not Implemented";
    case HTTP_BAD_GATEWAY:		return "Bad Gateway";
    case HTTP_SERVICE_UNAVAILABLE:	return "Service Unavailable";
    case HTTP_GATEWAY_TIMEOUT:		return "Gateway Timeout";
    case HTTP_VERSION_NOT_SUPPORTED:	return "Version Not Supported";
    default:				return "";
    }
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
 * send a websocket response
 */
private void wsRespond(string context, int code, StringBuffer entity,
		       mapping extraHeaders)
{
    StringBuffer response, chunk;
    string *indices;
    mixed *values;
    int sz, i;

    response = new StringBuffer("\10" + protoAsn(context) +
				"\20" + protoInt(code) +
				"\32" + protoString(comment(code)));
    if (entity) {
	response->append("\42");
	response->append(protoStrbuf(entity));
    }
    if (extraHeaders) {
	indices = map_indices(extraHeaders);
	values = map_values(extraHeaders);
	for (sz = sizeof(indices), i = 0; i < sz; i++) {
	    response->append(
		"\52" + protoString(indices[i] + ": " +
				    ((typeof(values[i]) == T_ARRAY) ?
				      values[i][0] : values[i]))
	    );
	}
    }

    chunk = new StringBuffer("\010\002\032");
    chunk->append(protoStrbuf(response));
    connection->sendWsChunk(WEBSOCK_BINARY, WEBSOCK_FIN, 0, chunk);
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
	wsRespond(context, code, entity, extraHeaders);
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
int receiveRequest(int code, HttpRequest request)
{
    string host, str;
    mixed length;

    if (previous_object() == connection) {
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
 * receive entity
 */
void receiveEntity(StringBuffer entity)
{
    if (previous_object() == connection) {
	call(nil, request, entity, handle);
    }
}

/*
 * upgrade connection to WebSocket protocol
 */
static void upgradeToWebSocket(varargs string login, string password)
{
    ::login = login;
    ::password = password;
    websocket = TRUE;
}

/*
 * receive WebSocket frame
 */
void receiveWsFrame(int opcode, int flags, int len)
{
    if (previous_object() == connection) {
	::opcode = opcode;
	::flags = flags;
    }
}

/*
 * receive WebSocket request
 */
private void receiveWsRequest(StringBuffer chunk)
{
    int c, offset;
    string buf, verb, path, context, headers, header;
    StringBuffer body;
    HttpRequest request;
    mixed *handle;

    ({ c, buf, offset }) = parseByte(chunk, nil, 0);
    if (c != 012) {
	error("WebSocketRequestMessage.verb expected");
    }
    ({ verb, buf, offset }) = parseString(chunk, buf, offset);
    ({ c, buf, offset }) = parseByte(chunk, buf, offset);
    if (c != 022) {
	error("WebSocketRequestMessage.path expected");
    }
    ({ path, buf, offset }) = parseString(chunk, buf, offset);
    ({ c, buf, offset }) = parseByte(chunk, buf, offset);
    if (c == 032) {
	({ body, buf, offset }) = parseStrbuf(chunk, buf, offset);
	({ c, buf, offset }) = parseByte(chunk, buf, offset);
    }
    if (c != 040) {
	error("WebSocketRequestMessage.id expected");
    }
    ({ context, buf, offset }) = parseAsn(chunk, buf, offset);

    for (headers = ""; offset < strlen(buf) || chunk->length() != 0;
	 headers += header + "\n") {
	({ c, buf, offset }) = parseByte(chunk, buf, offset);
	if (c != 052) {
	    error("WebSocketRequestMessage.headers expected");
	}
	({ header, buf, offset }) = parseString(chunk, buf, offset);
    }

    request = new HttpRequest(1.1, verb, nil, CHAT_SERVER, path);
    if (strlen(headers) != 0) {
	request->setHeaders(new RemoteHttpFields(headers));
    }

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
void receiveWsChunk(StringBuffer chunk)
{
    int c, offset;
    string buf;
    StringBuffer mesg;

    if (previous_object() == connection) {
	if (opcode == WEBSOCK_CLOSE) {
	    connection->sendWsChunk(WEBSOCK_CLOSE, WEBSOCK_FIN, 0, chunk);
	} else {
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
		({ mesg, buf, offset }) = parseStrbuf(chunk, buf, offset);
		receiveWsRequest(mesg);
		break;

	    case 2:	/* response */
		({ c, buf, offset }) = parseByte(chunk, buf, offset);
		if (c != 032) {
		    error("WebSockMessage.response expected");
		}
		({ mesg, buf, offset }) = parseStrbuf(chunk, buf, offset);
		error("Response not implemented");
		break;

	    default:
		error("Unknown websocket message");
	    }
	}
    }
}

/*
 * send a WebSocket chunk
 */
static void sendWebSocket(int opcode, int flags, StringBuffer chunk)
{
    connection->sendWsChunk(opcode, flags, 0, chunk);
}

/*
 * finished sending response
 */
void doneChunk()
{
    if (previous_object() == connection) {
	if (!websocket) {
	    connection->doneRequest();
	} else if (opcode == WEBSOCK_CLOSE) {
	    connection->terminate();
	} else {
	    connection->expectWsFrame();
	}
    }
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
void disconnected()
{
    if (previous_object() == connection) {
	close();
    }
}
