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
# include <Continuation.h>
# include "~HTTP/HttpResponse.h"
# include "rest.h"

inherit RestClient;


string address;			/* request address */
string path;			/* request path */
StringBuffer entity;		/* request entity */
mapping headers;		/* request extra headers */
Continuation callback;		/* result callback */

/*
 * initialize POST request connection
 */
static void create(string address, int port, string path, StringBuffer entity,
		   mapping headers, Continuation callback)
{
    ::create(address, port, "created");
    ::address = address;
    ::path = path;
    ::entity = entity;
    ::headers = headers;
    ::callback = callback;
}

/*
 * send POST request
 */
static void created(int code)
{
    if (code == HTTP_OK) {
	request("POST", address, path, "application/json", entity, headers,
		"response", argEntity());
    } else {
	/* connection failed */
	callback->runNext(code, nil);
	destruct_object(this_object());
    }
}

/*
 * callback with response
 */
static void response(string context, HttpResponse response, StringBuffer entity)
{
    callback->runNext(response->code(), entity);
    callback = nil;
    doneResponse();
    terminate();
}

/*
 * close connection
 */
static void close()
{
    if (callback) {
	callback->runNext(-1, nil);
    }
    ::close();
}
