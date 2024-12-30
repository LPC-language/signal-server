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

# ifdef REGISTER

register("client", "PUT", "/api/v1/message", "receiveMessage");
register("client", "PUT", "/api/v1/queue/empty", "ignoredMessage");

# else

# include <String.h>
# include "~HTTP/HttpRequest.h"
# include "~HTTP/HttpResponse.h"
# include "rest.h"
# include "~/config/services"

inherit RestClient;
private inherit "/lib/util/random";
private inherit base64 "/lib/util/base64";


# define TLS_CLIENT_SESSION	"/usr/MsgServer/benchmark/lib/TlsClientSession"
# define BENCHMARK		"/usr/MsgServer/benchmark/sys/benchmark"

string accountId;	/* account connected to */
string password;	/* account password */
string key;		/* websocket key */

/*
 * create client of simulated connection
 */
static void create(string host, object serverSim, string clientSimPath,
		   string accountId, string password)
{
    object clientSim;

    ::accountId = accountId;
    ::password = password;
    clientSim = ::create(host, 443, "created", clientSimPath,
			 TLS_CLIENT_SESSION);
    serverSim->startConnection(clientSim);
    clientSim->startConnection(serverSim);
}

/*
 * start if connection established
 */
static void created(int status)
{
    if (status == HTTP_OK) {
	call_out("start", 0);
    } else {
	error("Connection failed for " + accountId);
    }
}

/*
 * start websocket connection
 */
static void start()
{
    key = base64::encode(random_string(16));
    request("GET", CHAT_SERVER,
	    "/v1/websocket/?login=" + accountId + "&password=" + password, nil,
	    nil, ([
		"Upgrade" : ({ "websocket" }),
		"Connection" : ({ "Upgrade" }),
		"Sec-WebSocket-Key" : key,
		"Sec-WebSocket-Version" : "13"
	    ]),
	    "establish", argHeader("Connection"), argHeader("Upgrade"),
	    argHeader("Sec-WebSocket-Accept"));
}

/*
 * establish websocket connection
 */
static void establish(string context, HttpResponse response, string connection,
		      string upgrade, string accept)
{
    if (response->code() == HTTP_SWITCHING_PROTOCOLS &&
	connection == "Upgrade" && upgrade == "websocket" &&
	accept == base64::encode(hash_string("SHA1", key + WEBSOCKET_GUID))) {
	upgradeToWebSocket("chat");
    } else {
	error("Websocket connection failed for " + accountId);
    }
}

/*
 * send message
 */
void sendMessage(string destination)
{
    request("PUT", nil, "/v1/messages/" + destination + "?story=false", nil,
	    new StringBuffer("{\"destination\":\"" + destination + "\",\"messages\":[{\"content\":\"Mwj98wgSIQWdyRFxXnAytg/qS43FwLTrzfXusiwTM0wo+ez39wb7PBohBS/8DEVi5o9MJNllwqM25f8JaqhUcMW4iqZbyWS52ap7ItMBMwohBb6qB641rH295FTRV2Cm/deVKuefRnsxAD7VjEwFHdwwEAEYACKgAdOjCiPFsciJmjqzRsecODbtvMdxOL9LoqMMERgRsE2MVWDNlJxGYlVh/zZVj73M5YbxgrzsYxh4huZlf7bk+xTOdcPoqU7YE4Zo+qO8v9QLzhrwq3Df1gn+iqnweiTWRlrmZpERJTEpJHvdgZMIxPoxyuUavSJ7QTjFz0SUTLmpA3ZfovBf40/o8oKgyveUtZ6WjC1BvZc5+8p3TUAKYfqhvzGCw6n9qSj4ATCn05IF\",\"destinationDeviceId\":1,\"destinationRegistrationId\":7395,\"type\":3}],\"online\":false,\"timestamp\":" + time() + "000,\"urgent\":true}"),
	    ([ "content-type" : "application/json" ]),
	    "ignoredResponse");
}

/*
 * receive message
 */
static void receiveMessage(string context, HttpRequest request)
{
    respond(context, HTTP_OK, nil, nil);
}

/*
 * run benchmark
 */
void benchmark(string *correspondents, varargs int n)
{
    sendMessage(correspondents[n]);
    if (++n < sizeof(correspondents)) {
	call_out("benchmark", 0, correspondents, n);
    } else {
	call_out_other(BENCHMARK, "done", 0);
    }
}

string accountId()	{ return accountId; }

# endif
