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

private inherit "hash";


# define DISCOVERABLE		0
# define UNIDENTIFIED_ACCESS	1
# define VIDEO			2
# define VOICE			3

private string id;
private mapping devices;
private string phoneNumber;
private string pni;
private string agent;
private string pin;
private int pniRegistrationId;
private string recoveryPassword;
private string registrationLock;
private string signalingKey;
private string unidentifiedAccessKey;
private string identityKey;
private string pniKey;
private int flags;

/*
 * initialize Account
 */
static void create(string phoneNumber, string pni, string password,
		   string agent, Device device, int discoverable,
		   int fetchesMessages, string name, string pin,
		   int pniRegistrationId, string recoveryPassword,
		   int registrationId, string registrationLock,
		   string signalingKey, string unidentifiedAccessKey,
		   int unidentifiedAccess, int video, int voice)
{
    ::phoneNumber = phoneNumber;
    ::pni = pni;
    ::agent = agent;
    devices = ([ device->id() : device ]);
    device->setRegistrationId(registrationId);
    device->setFetchesMessages(fetchesMessages);
    device->setAuthTokenHash(hash(password)...);
    device->setName(name);

    ::pin = pin;
    ::pniRegistrationId = pniRegistrationId;
    ::recoveryPassword = recoveryPassword;
    ::registrationLock = registrationLock;
    ::signalingKey = signalingKey;
    ::unidentifiedAccessKey = unidentifiedAccessKey;
    flags |= discoverable << DISCOVERABLE;
    flags |= unidentifiedAccess << UNIDENTIFIED_ACCESS;
    flags |= video << VIDEO;
    flags |= voice << VOICE;
}

/*
 * set account ID
 */
void setId(string id)
{
    if (previous_program() == ACCOUNT_SERVER) {
	::id = id;
    }
}

Device device(int deviceId, varargs string password)
{
    Device device;
    string tokenHash, salt;

    device = devices[deviceId];
    if (device) {
	if (!password) {
	    return device;
	}
	({ tokenHash, salt }) = device->authTokenHash();
	if (hash(password, salt)[0] == tokenHash) {
	    return device;
	}
    }
}

void updateIdentityKey(string key)
{
    /*
     * avoid object modification if possible
     */
    if (identityKey != key) {
	identityKey = key;
    }
}

void updatePniKey(string key)
{
    /*
     * avoid object modification if possible
     */
    if (pniKey != key) {
	pniKey = key;
    }
}


string id()			{ return id; }
string phoneNumber()		{ return phoneNumber; }
string pni()			{ return pni; }
string agent()			{ return agent; }
int discoverable()		{ return (flags >> DISCOVERABLE) & 1; }
string pin()			{ return pin; }
int pniRegistrationId()		{ return pniRegistrationId; }
string registrationLock()	{ return registrationLock; }
string identityKey()		{ return identityKey; }
string pniKey()			{ return pniKey; }
int unidentifiedAccess()	{ return (flags >> UNIDENTIFIED_ACCESS) & 1; }
int video()			{ return (flags >> VIDEO) & 1; }
int voice()			{ return (flags >> VOICE) & 1; }
