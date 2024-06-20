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

register(CHAT_SERVER, "PUT", "/v1/accounts/gcm/",
	 "putAccountsGcm", argHeaderAuth(), argEntityJson());
register(CHAT_SERVER, "PUT", "/v1/accounts/attributes/",
	 "putAccountsAttributes", argHeaderAuth(), argHeader("X-Signal-Agent"),
	 argEntityJson());

# else

# include "rest.h"
# include "account.h"

inherit RestServer;


/*
 * set gcmId for device
 */
static void putAccountsGcm(string context, Account account, Device device,
			   mapping entity)
{
    string gcmId;

    gcmId = entity["gcmRegistrationId"];
    if (gcmId != device->gcmId()) {
	device->setGcmId(gcmId);
	device->setFetchesMessages(FALSE);
    }

    call_out("respondJsonOK", 0, context);
}

static void putAccountsAttributes(string context, Account account,
				  Device device, string agent, mapping entity)
{
    mapping cap;

    cap = entity["capabilities"];
    device->update(entity["name"], entity["registrationId"], agent,
		   entity["fetchesMessages"], cap["announcementGroup"],
		   cap["changeNumber"], cap["giftBadges"],
		   cap["paymentActivation"], cap["pni"], cap["senderKey"],
		   cap["storage"], cap["stories"], cap["uuid"]);
    account->update(entity["discoverableByPhoneNumber"], entity["pin"],
		    entity["pniRegistrationId"], entity["recoveryPassword"],
		    entity["registrationLock"], entity["signalingKey"],
		    entity["unidentifiedAccessKey"],
		    entity["unrestrictedUnidentifiedAccess"], entity["video"],
		    entity["voice"]);

    call_out("respondJsonOK", 0, context);
}

# endif
