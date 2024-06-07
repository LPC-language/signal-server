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

register(CHAT_SERVER, "PUT", "/v1/profile/",
	 "putProfile", argHeaderAuth(), argEntityJson());

# else

# include <Continuation.h>
# include "~HTTP/HttpResponse.h"
# include "account.h"

inherit "../RestServer";
inherit base64 "/lib/util/base64";


/*
 * upload profile
 */
static void putProfile(Account account, Device device, mapping entity)
{
    /* XXX ignore badges */
    new Continuation("putProfile2", account->id(), entity)
	->add("putProfile3")
	->runNext();
}

static void putProfile2(string accountId, mapping entity)
{
    Profile profile;

    /* XXX ignore avatar */
    profile = PROFILE_SERVER->get(accountId, entity["version"]);
    profile->update(entity["name"], nil, entity["aboutEmoji"], entity["about"],
		    entity["paymentAddress"],
		    base64::decode(entity["commitment"]));
}

static void putProfile3()
{
    respondJson(HTTP_OK, ([ ]));
}

# endif
