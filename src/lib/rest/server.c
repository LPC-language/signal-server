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
# include "application.h"
# include "rest.h"
# include <config.h>
# include <version.h>
# include <status.h>
# include <type.h>

private inherit "/lib/util/ascii";
private inherit json "/lib/util/json";


# define REGISTRY	"/usr/MsgServer/sys/registry"

private object connection;	/* TLS connection */
private HttpRequest request;	/* most recent request */
private mixed *handle;		/* call handle */

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
 * send a response
 */
static int respond(int code, string type, StringBuffer entity,
		   varargs mapping extraHeaders)
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

    return code;
}

/*
 * handle a REST call
 */
private int call(StringBuffer entity)
{
    mixed *args;
    int i;
    string str;

    args = handle[2 + handle[1] ..];
    for (i = sizeof(args); --i >= 0; ) {
	if (typeof(args[i]) == T_STRING) {
	    args[i] = request->headerValue(args[i]);
	} else {
	    switch (args[i]) {
	    case ARG_ENTITY:
		args[i] = entity;
		break;

	    case ARG_ENTITY_JSON:
		try {
		    str = request->headerValue("Content-Type");
		    if (lower_case(str) != "application/json") {
			error("Content-Type not JSON");
		    }
		    args[i] = json::decode(entity->chunk());
		} catch (...) {
		    return respond(HTTP_BAD_REQUEST, nil, nil);
		}
		break;
	    }
	}
    }

    return call_other(this_object(), handle[0], args...);
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
	    return respond(code, nil, nil);
	}

	host = request->host();
	if (!host) {
	    return respond(HTTP_BAD_REQUEST, nil, nil);
	}
	str = previous_object()->host();
	if (str && lower_case(str) != lower_case(host)) {
	    return respond(HTTP_BAD_REQUEST, nil, nil);
	}

	str = request->scheme();
	if (str && str != "https://") {
	    return respond(HTTP_NOT_FOUND, nil, nil);
	}

	if (request->headerValue("Transfer-Encoding")) {
	    return respond(HTTP_CONTENT_TOO_LARGE, nil, nil);
	}

	handle = REGISTRY->lookup(host, request->method(), request->path());
	if (!handle) {
	    return respond(HTTP_NOT_FOUND, nil, nil);
	}

	length = request->headerValue("Content-Length");
	switch (typeof(length)) {
	case T_NIL:
	    return call(nil);

	case T_INT:
	    if (length > REST_LENGTH_LIMIT) {
		return respond(HTTP_CONTENT_TOO_LARGE, nil, nil);
	    }
	    if (sizeof(handle & ({ ARG_ENTITY_JSON })) != 0 && length > 65535) {
		return respond(HTTP_CONTENT_TOO_LARGE, nil, nil);
	    }
	    connection->expectEntity(length);
	    break;

	default:
	    return respond(HTTP_BAD_REQUEST, nil, nil);
	}
    }
}

/*
 * receive entity
 */
void receiveEntity(StringBuffer entity)
{
    if (previous_object() == connection) {
	call(entity);
    }
}

/*
 * finished sending response
 */
void doneChunk()
{
    if (previous_object() == connection) {
	connection->doneRequest();
    }
}

/*
 * cleanup after TLS connection ends
 */
void disconnect()
{
    if (previous_object() == connection) {
	destruct_object(this_object());
    }
}
