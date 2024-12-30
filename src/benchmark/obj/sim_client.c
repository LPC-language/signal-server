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

inherit client Http1Client;
inherit user SimUser;


/*
 * initialize HTTP connection object
 */
static void create(object client, string address, int port, string host,
		   string responsePath, string fieldsPath,
		   string tlsClientSessionPath)
{
    if (!responsePath) {
	responsePath = OBJECT_PATH(RemoteHttpResponse);
    }
    if (!fieldsPath) {
	fieldsPath = OBJECT_PATH(RemoteHttpFields);
    }
    client::create(client, address, port, responsePath, fieldsPath);
    user::create(MODE_LINE);
}

/*
 * establish connection with server
 */
void startConnection(object server)
{
    connection(server);
    connected();
}

static int inactivityTimeout()	{ return 1000000; }

/*
 * receive a message
 */
static void receive_message(string str)
{
    receiveBytes(str);
}

/*
 * connection terminated
 */
static void logout(int quit)
{
    close(quit);
    destruct_object(this_object());
}

/*
 * output remainder of message
 */
static void message_done()
{
    messageDone();
}
