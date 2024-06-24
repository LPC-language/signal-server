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

inherit "Sho";


# define BLOCK63	("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" +	\
			 "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" +	\
			 "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" +	\
			 "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0")

static string hashRatchet(string cv, string input)
{
    return hash_string("SHA256",
		       hash_string("SHA256", BLOCK63 + "\0" + cv + input));
}

static string hashSqueeze1(string cv, string input)
{
    return hash_string("SHA256", BLOCK63 + "\1" + cv + input);
}

static string hashSqueeze2(string cv, string input)
{
    return hash_string("SHA256", BLOCK63 + "\2" + cv + input);
}
