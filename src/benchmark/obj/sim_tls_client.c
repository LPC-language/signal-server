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
# include "~HTTP/HttpResponse.h"
# include "~HTTP/HttpField.h"
# include "~HTTP/HttpConnection.h"
# include "~HTTP/SimUser.h"
# include "~TLS/tls.h"

inherit client Http1TlsClient;
inherit user SimUser;


string host;		/* server hostname */

/*
 * initialize HTTP connection object
 */
static void create(object client, string address, int port, string host,
		   string responsePath, string fieldsPath,
		   string tlsClientSessionPath)
{
    ::host = host;
    if (!responsePath) {
	responsePath = OBJECT_PATH(RemoteHttpResponse);
    }
    if (!fieldsPath) {
	fieldsPath = OBJECT_PATH(RemoteHttpFields);
    }
    if (!tlsClientSessionPath) {
	tlsClientSessionPath = OBJECT_PATH(TlsClientSession);
    }
    client::create(client, address, port, responsePath, fieldsPath,
		   tlsClientSessionPath);
    user::create(MODE_RAW);
}

/*
 * establish connection with server
 */
void startConnection(object server)
{
    connection(server);
    tlsConnect(host);
}

static int inactivityTimeout()	{ return 1000; }

/*
 * receive a message
 */
static int receive_message(string str)
{
    return tlsReceive(str);
}

/*
 * connection terminated
 */
static void logout(int quit)
{
    tlsClose(quit);
    destruct_object(this_object());
}

/*
 * output remainder of message
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
