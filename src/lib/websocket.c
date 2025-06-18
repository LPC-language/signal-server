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
# include <type.h>

private inherit asn "/lib/util/asn";
private inherit "proto";


private int closing;		/* closing WebSocket connection */

/*
 * response code => comment
 */
static string comment(int code)
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
 * send a StringBuffer chunk via WebSocket
 */
static void wsSendChunk(object connection, StringBuffer chunk, varargs int mask)
{
    if (closing) {
	error("WebSocket connection is closing");
    }
    connection->sendWsChunk(WEBSOCK_BINARY, WEBSOCK_FIN, mask, chunk);
}

/*
 * close WebSocket connection
 */
static void wsSendClose(object connection, string code, varargs int mask)
{
    if (!closing) {
	closing = TRUE;
	connection->sendWsChunk(WEBSOCK_CLOSE, WEBSOCK_FIN, mask,
				new StringBuffer(code));
    }
}

/*
 * prepare a WebSocket request
 */
static StringBuffer wsRequest(string verb, string path, StringBuffer body,
			      mapping extraHeaders, string context)
{
    StringBuffer request, chunk;
    string *indices;
    mixed *values;
    int sz, i;

    request = new StringBuffer("\12" + protoString(verb) +
			       "\22" + protoString(path));
    if (body) {
	request->append("\32");
	request->append(protoStrbuf(body));
    }
    request->append("\40");
    request->append(protoAsn(context));

    if (extraHeaders) {
	indices = map_indices(extraHeaders);
	values = map_values(extraHeaders);
	for (sz = sizeof(indices), i = 0; i < sz; i++) {
	    request->append(
		"\52" + protoString(indices[i] + ":" +
				    ((typeof(values[i]) == T_ARRAY) ?
				      values[i][0] : values[i]))
	    );
	}
    }

    chunk = new StringBuffer("\010\001\022");
    chunk->append(protoStrbuf(request));
    return chunk;
}

/*
 * prepare a websocket response
 */
static StringBuffer wsResponse(string context, int code, StringBuffer entity,
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
		"\52" + protoString(indices[i] + ":" +
				    ((typeof(values[i]) == T_ARRAY) ?
				      values[i][0] : values[i]))
	    );
	}
    }

    chunk = new StringBuffer("\010\002\032");
    chunk->append(protoStrbuf(response));
    return chunk;
}

/*
 * receive WebSocket request
 */
static mixed *wsReceiveRequest(StringBuffer chunk, string service)
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
    context = asn::unsignedExtend(context, 8);

    for (headers = ""; !parseDone(chunk, buf, offset); headers += header + "\n")
    {
	({ c, buf, offset }) = parseByte(chunk, buf, offset);
	if (c != 052) {
	    error("WebSocketRequestMessage.headers expected");
	}
	({ header, buf, offset }) = parseString(chunk, buf, offset);
    }

    request = new HttpRequest(1.1, verb, nil, service, path);
    if (strlen(headers) != 0) {
	request->setHeaders(new RemoteHttpFields(headers));
    }

    return ({ context, request, body });
}

/*
 * receive WebSocket response
 */
static mixed *wsReceiveResponse(StringBuffer chunk)
{
    int c, offset, code;
    string buf, context, message, headers, header;
    StringBuffer body;
    HttpResponse response;
    mixed *handle;

    ({ c, buf, offset }) = parseByte(chunk, nil, 0);
    if (c != 010) {
	error("WebSocketResponseMessage.id expected");
    }
    ({ context, buf, offset }) = parseAsn(chunk, buf, offset);
    context = asn::unsignedExtend(context, 8);
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
	    ({ body, buf, offset }) = parseStrbuf(chunk, buf, offset);
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

    return ({ context, response, body });
}
