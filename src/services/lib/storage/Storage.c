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

register(STORAGE_SERVER, "GET", "/v1/storage/manifest",
	 "getStorageManifest");
register(STORAGE_SERVER, "GET", "/v1/storage/manifest/version/{}",
	 "getStorageManifestVersion");

# else

# include "~HTTP/HttpResponse.h"
# include "rest.h"

inherit RestServer;


/*
 * get storage manifest
 */
static int getStorageManifest(string context)
{
    return respond(context, HTTP_UNAUTHORIZED, nil, nil);
}

/*
 * get storage manifest version
 */
static int getStorageManifestVersion(string context, string version)
{
    return respond(context, HTTP_UNAUTHORIZED, nil, nil);
}

# endif
