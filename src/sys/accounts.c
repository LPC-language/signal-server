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
# include "services.h"

private inherit "/lib/util/random";


object accounts;	/* accountId : account */
object phoneIndex;	/* phoneNumber : accountId */
object usernameIndex;	/* username : accountId */

/*
 * initialize account server
 */
static void create()
{
    accounts = new KVstore(100);
    phoneIndex = new KVstore(100);
    usernameIndex = new KVstore(100);
}

/*
 * add new account
 */
atomic void add(Account account)
{
    string accountId, username;

    if (previous_program() == RegistrationService) {
	for (;;) {
	    accountId = random_string(16);
	    try {
		accounts->add(accountId, account);
	    } catch (...) {
		continue;
	    }
	    break;
	}

	account->setId(accountId);

	/*
	 * extra indices for the database
	 */
	phoneIndex->add(account->phoneNumber(), accountId);
	username = account->username();
	if (username) {
	    usernameIndex->add(username, accountId);
	}
    }
}

/*
 * get by account ID
 */
Account get(string accountId)
{
    return accounts[accountId];
}

/*
 * get by phone number
 */
Account getByNumber(string phoneNumber)
{
    return accounts[phoneIndex[phoneNumber]];
}

/*
 * get by user name
 */
Account getByName(string username)
{
    return accounts[usernameIndex[username]];
}