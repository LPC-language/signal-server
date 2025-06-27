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

# ifdef REGISTER

register(CHAT_SERVER, "GET", "/v1/devices/", "getDevices", argHeaderAuth());
register(CHAT_SERVER, "GET", "/v1/devices/provisioning/code",
	 "getDevicesProvisioningCode", argHeaderAuth());
register(CHAT_SERVER, "PUT", "/v1/devices/{}",
	 "putDevicesCode", argHeader("Authorization"),
	 argHeader("X-Signal-Agent"), argEntityJson());
register(CHAT_SERVER, "PUT", "/v1/devices/unauthenticated_delivery",
	 "putDevicesUnauthenticatedDelivery", argHeaderAuth());
register(CHAT_SERVER, "PUT", "/v1/devices/capabilities",
	 "putDevicesCapabilities", argHeaderAuth(), argEntityJson());

# else

# include "~HTTP/HttpResponse.h"
# include "~HTTP/HttpField.h"
# include "rest.h"
# include "account.h"
# include "provisioning.h"

inherit RestServer;
private inherit "/lib/util/ascii";
private inherit base64 "/lib/util/base64";
private inherit uuid "~/lib/uuid";


static int getDevices(string context, Account account, Device device)
{
    mixed *devices;
    int sz, i;

    devices = account->devices();
    for (sz = sizeof(devices), i = 0; i < sz; i++) {
	device = devices[i];
	devices[i] = ([
	    "id" : device->id(),
	    "name" : device->name(),
	    "lastSeen" : device->lastSeen(),
	    "created" : device->created()
	]);
    }
    return respondJson(context, HTTP_OK, ([ "devices" : devices ]));
}

static int getDevicesProvisioningCode(string context, Account account,
				      Device device)
{
    string verificationCode;

    verificationCode = (string) (100000 + random(900000));
    PROVISIONING->storeVerificationCode(account->phoneNumber(),
					verificationCode);
    return respondJson(context, HTTP_OK,
		       ([ "verificationCode" : verificationCode ]));
}

static int putDevicesCode(string context, string code,
			  HttpAuthentication authorization, string agent,
			  mapping entity)
{
    string phoneNumber, password;

    if (!authorization || lower_case(authorization->scheme()) != "basic") {
	return respond(context, HTTP_BAD_REQUEST, nil, nil);
    }
    sscanf(base64::decode(authorization->authentication()), "%s:%s",
	   phoneNumber, password);
    if (PROVISIONING->getVerificationCode(phoneNumber) != code) {
	return respond(context, HTTP_FORBIDDEN, nil, nil);
    }
    PROVISIONING->removeVerificationCode(phoneNumber);

    call_out("putDevicesCode2", 0, context, phoneNumber, password, agent,
	     entity);
    return 0;
}

static void putDevicesCode2(string context, string phoneNumber, string password,
			    string agent, mapping entity)
{
    Account account;
    string uuid;
    mapping capabilities;
    int deviceId;
    Device device;

    account = ACCOUNT_SERVER->getByNumber(phoneNumber);
    uuid = uuid::encode(account->id());
    deviceId = account->nextDeviceId();
    authenticate(uuid + "." + deviceId, password);

    capabilities = entity["capabilities"];
    device = new Device(deviceId, password);
    device->update(entity["name"], entity["registrationId"], agent,
		   entity["fetchesMessages"], capabilities["announcementGroup"],
		   capabilities["changeNumber"], capabilities["giftBadges"],
		   FALSE, capabilities["pni"], capabilities["senderKey"],
		   FALSE, capabilities["stories"], FALSE);
    account->addDevice(device);

    respondJson(context, HTTP_OK, ([
	"uuid" : uuid,
	"pni" : uuid::encode(account->pni()),
	"deviceId" : deviceId
    ]));
}

static int putDevicesUnauthenticatedDelivery(string context, Account account,
					     Device device)
{
    return respond(context, HTTP_OK, nil, nil);
}

static int putDevicesCapabilities(string context, Account account,
				  Device device, mapping entity)
{
    device->updateCapabilities(entity["announcementGroup"],
			       entity["changeNumber"], entity["giftBadges"],
			       FALSE, entity["pni"], entity["senderKey"],
			       FALSE, entity["stories"], FALSE);
    return respond(context, HTTP_OK, nil, nil);
}

# endif
