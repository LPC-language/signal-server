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

private inherit "/lib/util/ascii";
private inherit json "/lib/util/json";


private object connection;	/* TLS connection */
private HttpResponse response;	/* most recent response */
private mixed handle;		/* call handle */
private StringBuffer chunks;	/* collected chunks */

/*
 * initialize
 */
static void create(string address, int port, string function)
{
    if (status(OBJECT_PATH(RestTlsClientSession), O_INDEX) == nil) {
	compile_object(OBJECT_PATH(RestTlsClientSession));
    }

    connection = clone_object(HTTP1_TLS_CLIENT, this_object(), address, port,
			      address, nil, nil,
			      OBJECT_PATH(RestTlsClientSession));
    handle = function;
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
 * send a request
 */
static void request(string method, string host, string path, string type,
		    StringBuffer entity, mapping extraHeaders,
		    string function, varargs mixed arguments...)
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

    handle = ({ function }) + arguments;
}

/*
 * handle a REST response
 */
private void call(StringBuffer entity)
{
    mixed *args;
    int i;
    string str;

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
		str = response->headerValue("Content-Type");
		args[i] = (lower_case(str) == "application/json") ?
			   json::decode(entity->chunk()) : nil;
		break;
	    }
	}
    }

    call_other(this_object(), handle[0], response->code(), args...);
}

/*
 * handle a response
 */
void receiveResponse(HttpResponse response)
{
    mixed length;

    if (previous_object() == connection) {
	::response = response;

	if (response->headerValue("Transfer-Encoding")) {
	    chunks = new StringBuffer;
	    connection->expectChunk();
	} else {
	    length = response->headerValue("Content-Length");
	    switch (typeof(length)) {
	    case T_NIL:
		call(nil);
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
 * receive a chunk
 */
void receiveChunk(StringBuffer chunk, HttpFields trailers)
{
    if (previous_object() == connection) {
	if (chunk) {
	    chunks->append(chunk);
	    connection->expectChunk();
	} else {
	    chunk = chunks;
	    chunks = nil;
	    call(chunk);
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
 * finished handling response
 */
static void doneResponse()
{
    if (connection) {
	connection->doneResponse();
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
