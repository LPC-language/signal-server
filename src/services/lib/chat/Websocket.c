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

# ifdef REGISTER

register(CHAT_SERVER, "GET", "/v1/websocket/",
	 "getWebsocket", argHeader("Upgrade"), argHeader("Connection"),
	 argHeader("Sec-WebSocket-Key"), argHeader("Sec-WebSocket-Version"));
register(CHAT_SERVER, "GET", "/v1/websocket/{}",
	 "getWebsocketLogin", argHeader("Upgrade"), argHeader("Connection"),
	 argHeader("Sec-WebSocket-Key"), argHeader("Sec-WebSocket-Version"));

# else

# include <String.h>
# include "~HTTP/HttpResponse.h"
# include "~HTTP/HttpField.h"
# include "rest.h"
# include "account.h"

inherit RestServer;
private inherit base64 "/lib/util/base64";
private inherit hex "/lib/util/hex";
private inherit "~/lib/proto";


# define GUID	"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

static int getWebsocket(string upgrade, string connection, string key,
			string version)
{
    int code;

    if (upgrade == "websocket" && connection == "Upgrade" && key != nil &&
	version == "13") {
	code = respond(HTTP_SWITCHING_PROTOCOLS, nil, nil, ([
	    "Upgrade" : ({ "websocket" }),
	    "Connection" : ({ "Upgrade" }),
	    "Sec-WebSocket-Accept" : base64::encode(hash_string("SHA1",
								key + GUID))
	]));
	upgradeToWebSocket();

	return code;
    } else {
	return respond(HTTP_BAD_REQUEST, nil, nil);
    }
}

static int getWebsocketLogin(string param, string upgrade, string connection,
			     string key, string version)
{
    string uuid, password;

    if (sscanf(param, "?login=%s&password=%s", uuid, password) != 2) {
	return respond(HTTP_BAD_REQUEST, nil, nil);
    }

    return getWebsocket(upgrade, connection, key, version);
}

# endif
