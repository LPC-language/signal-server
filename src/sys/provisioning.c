/*
 * This file is part of https://github.com/LPC-language/signal-server
 * Copyright (C) 2025 Dworkin B.V.  All rights reserved.
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

private inherit "~/lib/phone";


# define DURATION	1 * 24 * 3600

mapping addresses;		/* provisioning addresses */
object verificationCodes;	/* stored verification codes */

/*
 * initialize provisioning server
 */
static void create()
{
    addresses = ([ ]);
    verificationCodes = new KVstoreExp(249, DURATION);
}

/*
 * register an endpoint with an address
 */
void addEndpoint(string provisioningAddr, object endpoint)
{
    addresses[provisioningAddr] = endpoint;
}

/*
 * obtain the endpoint for an address
 */
object getEndpoint(string provisioningAddr)
{
    return addresses[provisioningAddr];
}

/*
 * store verification code for new device
 */
void storeVerificationCode(string phoneNumber, string code)
{
    verificationCodes[phoneToNum(phoneNumber)] = code;
}

/*
 * get stored verification code for device
 */
string getVerificationCode(string phoneNumber)
{
    return verificationCodes[phoneToNum(phoneNumber)];
}

/*
 * remove stored verification code for device
 */
void removeVerificationCode(string phoneNumber)
{
    verificationCodes[phoneToNum(phoneNumber)] = nil;
}
