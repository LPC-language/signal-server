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

register(CHAT_SERVER, "GET", "/v1/config",
	 "getConfig", argHeaderAuth());

# else

# include "~HTTP/HttpResponse.h"
# include "account.h"

inherit "RestServer";


/*
 * default configuration
 */
static void getConfig(Account account, Device device)
{
    call_out("getConfig2", 0);
}

static void getConfig2()
{
    respondJson(HTTP_OK, ([ "config" : ({ }) ]));
}

# endif
