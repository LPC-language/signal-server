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

# include "account.h"


private string version;
private string name;
private string avatar;
private string aboutEmoji;
private string about;
private string paymentAddress;
private string commitment;

static void create(string version)
{
    ::version = version;
}

void update(string name, string avatar, string aboutEmoji, string about,
	    string paymentAddress, string commitment)
{
    ::name = name;
    ::avatar = avatar;
    ::aboutEmoji = aboutEmoji;
    ::about = about;
    ::paymentAddress = paymentAddress;
    ::commitment = commitment;
}


string version()	{ return version; }
string name()		{ return name; }
string avatar()		{ return avatar; }
string aboutEmoji()	{ return aboutEmoji; }
string about()		{ return about; }
string paymentAddress()	{ return paymentAddress; }
string commitment()	{ return commitment; }
