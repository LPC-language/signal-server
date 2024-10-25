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

# include <kernel/user.h>
# include "~HTTP/HttpRequest.h"
# include "~HTTP/HttpField.h"
# include "~HTTP/HttpConnection.h"
# include "~HTTP/SimUser.h"
# include "~TLS/tls.h"

inherit server Http1TlsServer;
inherit user SimUser;


string *hosts;		/* server hostnames */
private int received;	/* received input? */
private int requests;	/* received at least one request? */

/*
 * initialize HTTP connection object
 */
static void create(object server, string certificate, string key,
		   varargs string *hosts, string requestPath, string fieldsPath,
		   string tlsServerSessionPath)
{
    ::hosts = hosts;
    if (!requestPath) {
	requestPath = OBJECT_PATH(RemoteHttpRequest);
    }
    if (!fieldsPath) {
	fieldsPath = OBJECT_PATH(RemoteHttpFields);
    }
    if (!tlsServerSessionPath) {
	tlsServerSessionPath = OBJECT_PATH(TlsServerSession);
    }
    server::create(server, certificate, key, requestPath, fieldsPath,
		   tlsServerSessionPath);
    user::create(MODE_RAW);
}

/*
 * establish connection with client
 */
void startConnection(object client)
{
    connection(client);
}

static int inactivityTimeout()	{ return 1000; }

/*
 * receive a request
 */
static int receiveRequest(int code, HttpRequest request)
{
    requests = TRUE;
    return ::receiveRequest(code, request);
}

/*
 * receive a message
 */
static int receive_message(string str)
{
    if (!received) {
	received = TRUE;
	return tlsAccept(str, FALSE, ((hosts) ? hosts : ({ }))...);
    }
    return tlsReceive(str);
}

/*
 * terminate connection
 */
static void logout(int quit)
{
    tlsClose(quit);
    destruct_object(this_object());
}

/*
 * send remainder of message
 */
static int message_done()
{
    return messageDone();
}

/*
 * is there buffered input?
 */
static int buffered_input()
{
    return bufferedInput();
}

/*
 * reprocess pending input
 */
static void restart_input()
{
    restartInput();
}

/*
 * time out if no request was received in time
 */
static int timeout()
{
    return !requests;
}
