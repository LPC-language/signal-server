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

# include "KVstoreExp.h"
# include "services.h"

private inherit "/lib/util/random";
private inherit hex "/lib/util/hex";


# define DURATION	30 * 24 * 3600

object sessions;	/* session mappings */
object phoneIndex;	/* phoneNumber : sessionId */

/*
 * initialize registration server
 */
static void create()
{
    sessions = new KVstoreExp(100, DURATION);
    phoneIndex = new KVstoreExp(100, DURATION);
}

/*
 * get session ID and session, creating a new session if needed
 */
mixed *getSessionId(string phoneNumber)
{
    if (previous_program() == RegistrationService) {
	string sessionId;
	mapping session;

	sessionId = phoneIndex[phoneNumber];
	if (sessionId) {
	    return ({ sessionId, sessions[sessionId] });
	}

	session = ([ "phoneNumber" : phoneNumber ]);

	for (;;) {
	    sessionId = hex::format(random_string(16));
	    try {
		sessions->add(sessionId, session);
	    } catch (...) {
		continue;
	    }
	    session["id"] = sessionId;
	    return ({ phoneIndex[phoneNumber] = sessionId, session });
	}
    }
}

/*
 * get session by sessionId
 */
mapping getSession(string sessionId)
{
    if (previous_program() == RegistrationService) {
	return sessions[sessionId];
    }
}

/*
 * remove session
 */
void remove(string sessionId)
{
    if (previous_program() == RegistrationService) {
	mapping values;

	values = sessions[sessionId];
	if (values) {
	    phoneIndex[values["phoneNumber"]] = nil;
	    sessions[sessionId] = nil;
	}
    }
}
