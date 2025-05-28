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

# include "~HTTP/HttpConnection.h"
# include "account.h"
# include "services.h"
# include "~/config/services"

private inherit "~/lib/phone";
private inherit "/lib/util/random";
private inherit base64 "/lib/util/base64";
private inherit uuid "~/lib/uuid";


# define SIM_CLIENT		"/usr/MsgServer/benchmark/obj/client"
# define SIM_HTTP_SERVER	"/usr/MsgServer/benchmark/obj/sim_server"
# define SIM_HTTP_CLIENT	"/usr/MsgServer/benchmark/obj/sim_client"
# define SIM_TLS_SERVER		"/usr/MsgServer/benchmark/obj/sim_tls_server"
# define SIM_TLS_CLIENT		"/usr/MsgServer/benchmark/obj/sim_tls_client"

# define ACCOUNTS		200000	/* simulated accounts */
# define CLIENTS		10000	/* connected accounts */
# define SENDERS		5000	/* accounts sending messages */
# define CORRESPONDENTS		10	/* recipients for each sender */

string certificate, key;	/* TLS certificate & key */
object user;			/* user to report to */
object *clients;		/* websocket connections */
int counter;			/* client counter */

/*
 * initialize benchmarks
 */
static void create()
{
    certificate = read_file("~/config/cert/server.pem");
    key = read_file("~/config/cert/server.key");
    user = this_user();
    clients = allocate(CLIENTS);
    call_out("build", 0, 0);
}

/*
 * add new account
 */
private string *addAccount(string phoneNumber)
{
    Device device;
    Account account;
    string password;

    password = base64::encode("\0\0\0\0\0\0\0\0\0\0" + phoneToNum(phoneNumber));
    device = new Device(1, password);
    device->update(nil, random(65536), "Signal-Android/6.20.6 Android/33",
		   FALSE, TRUE,
		   TRUE, TRUE,
		   TRUE, FALSE, TRUE,
		   TRUE, TRUE, TRUE);
    account = new Account(phoneNumber, PNI_SERVER->getId(phoneNumber), device);
    account->update(TRUE, nil,
		    random(65536), nil,
		    nil, nil,
		    base64::encode(random_string(16)),
		    FALSE, TRUE,
		    TRUE);
    ACCOUNT_SERVER->add(account);

    return ({ uuid::encode(account->id()), password });
}

/*
 * new simulated connection
 */
private object addConnection(string accountId, string password)
{
    object server, serverSim, client, clientSim;

    server = clone_object(SERVICES);
    serverSim = clone_object(SIM_TLS_SERVER, server, certificate, key, ({
	CHAT_SERVER,
	STORAGE_SERVER,
	CDSI_SERVER,
	UPTIME_SERVER,
	BACKUP_SERVER,
	VOIP_SERVER,
	VOIP_STAGING_SERVER,
	VOIP_TEST_SERVER,
	VOIP_DEV_SERVER,
	CONTENTPROXY_SERVER,
	CAPTCHA_SERVER
    }));
    server->init(serverSim);

    return clone_object(SIM_CLIENT, CHAT_SERVER, serverSim, SIM_TLS_CLIENT,
			accountId, password);
}

/*
 * build up accounts and connections, in stages
 */
static void build(int num)
{
    int i;
    string account, password;

    for (i = num, num += 10; i < num; ) {
	({ account, password }) = addAccount("+155" + (50000000 + i));
	if (i < CLIENTS) {
	    clients[i] = addConnection(account, password);
	    if (++i % 100 == 0) {
		user->message(i + " accounts connected at " + ctime(time()) +
			  "\n");
	    }
	} else if (++i % 10000 == 0) {
	    user->message(i + " accounts created at " + ctime(time()) + "\n");
	}
    }
    if (num < ACCOUNTS) {
	call_out("build", 0, num);
    } else {
	call_out("setup", 0, 0);
    }
}

/*
 * set up the benchmark
 */
void setup(int num)
{
    mapping correspondents;
    int i, j;

    if (num == 0) {
	user->message("Started " + ctime(time()) + "\n");
    }
    for (i = num, num += 100; i < num; i++) {
	correspondents = ([ ]);
	for (j = 0; j < CORRESPONDENTS; j++) {
	    correspondents[clients[random(CLIENTS)]->accountId()] = 1;
	}
	correspondents[clients[i]] = nil;
	call_out_other(clients[i], "benchmark", 0, map_indices(correspondents));
    }
    if (num < SENDERS) {
	call_out("setup", 0, num);
    }
}

/*
 * a client has finised
 */
void clientDone()
{
    call_out_summand("done", 0, 1.0);
}

/*
 * count finished clients
 */
static void done(float number)
{
    counter += (int) number;
    if (counter == SENDERS) {
	user->message("Done " + ctime(time()) + "\n");
	counter = 0;
    }
}
