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


# include <KVstore.h>
# include "account.h"


object profiles;	/* id : profile */

/*
 * initialize profile server
 */
static void create()
{
    profiles = new KVstore(100);
}

Profile get(string id, string version)
{
    Profile profile;

    id = version + id;
    profile = profiles[id];
    if (!profile) {
	profile = profiles[id] = new Profile(version);
    }
    return profile;
}
