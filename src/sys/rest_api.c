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

# include "rest.h"
# include <type.h>
# include "~/config/services"
# include"services.h"


private mapping api;		/* REST API */

/*
 * register a new path
 */
void register(string host, string method, string path, string function,
	      mixed arguments...)
{
    mixed map;
    string *a, str;
    int sz, i;


    map = api;
    a = ({ host, method }) + explode(path + "/", "/");
    for (sz = sizeof(a), i = 0; i < sz; i++) {
	str = a[i];
	if (typeof(map[str]) != T_MAPPING) {
	    map[str] = ([ ]);
	}
	map = map[str];
    }
    map[nil] = ({ function }) + arguments;
}

/*
 * lookup a path
 */
mixed *lookup(string host, string method, string path)
{
    string *args, *a, str;
    mapping map;
    int sz, i;
    mixed *call;

    args = ({ });
    map = api;
    sscanf(path, "%s?%s", path, str);
    a = ({ host, method }) + explode(path + "/", "/");
    sz = sizeof(a);
    if (str) {
	a[sz - 1] += "?" + str;
    }
    for (i = 0; i < sz; i++) {
	str = a[i];
	if (map[str]) {
	    map = map[str];
	} else if (map["{}"]) {
	    map = map["{}"];
	    args += ({ str });
	} else {
	    return nil;
	}
    }

    call = map[nil];
    if (!call) {
	return nil;
    }
    return ({ call[0], args }) + call[1 ..];
}

/*
 * initialize registry
 */
static void create()
{
    api = ([ ]);

# define REGISTER
# include "~/services/lib/Chat.c"
# include "~/services/lib/Storage.c"
}
