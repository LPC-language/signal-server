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

# include <kernel/user.h>
# include "~HTTP/HttpConnection.h"
# include "services.h"
# include "~/config/services"


# define SYS_INITD	"/usr/System/initd"

object userd;			/* system user server */
string certificate, key;	/* TLS certificate & key */

/*
 * initialize TLS server
 */
static void create()
{
    userd = find_object(USERD);
    SYS_INITD->set_connection_manager_by_port("binary", 8443, this_object());
    certificate = read_file("~/config/cert/server.pem");
    key = read_file("~/config/cert/server.key");
}

/*
 * new TLS connection
 */
object select(string str)
{
    if (previous_object() == userd) {
	object connection, server;

	server = clone_object(SERVICES);
	connection = clone_object(HTTP1_TLS_SERVER, server, certificate, key,
				  FALSE, ({
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
	    CAPTCHA_SERVER,
	    UPDATES_SERVER
	}));
	server->init(connection);
	return connection;
    }
}

/*
 * initial connection mode
 */
int query_mode(object obj)
{
    return MODE_RAW;
}

/*
 * initial timeout
 */
int query_timeout(object obj)
{
    return 10;
}
