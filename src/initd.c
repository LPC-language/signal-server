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

/*
 * initialize message server
 */
static void create()
{
    compile_object("api/lib/TlsClientSession");
    compile_object("lib/KVstoreExp");
    compile_object("lib/Device");
    compile_object("lib/Account");
    compile_object("lib/Profile");
    compile_object("lib/ShoHmacSha256");
    compile_object("lib/ShoSha256");
    compile_object("lib/zkp/Scalar");
    compile_object("lib/zkp/RistrettoPoint");
    compile_object("lib/zkp/RemoteProof");
    compile_object("lib/zkp/Statement");
    compile_object("lib/protocol/RemoteProfileKeyCommitment");
    compile_object("lib/protocol/RemoteProfileKeyCredentialRequest");
    compile_object("obj/oneshot");
    compile_object("obj/fcm_sender");
    compile_object("obj/kvnode_exp");
    compile_object("sys/tls_server");
    compile_object("sys/rest_api");
    compile_object("sys/cert");
    compile_object("sys/credentials");
    compile_object("sys/registration");
    compile_object("sys/fcm_relay");
    compile_object("sys/accounts");
    compile_object("sys/pni");
    compile_object("sys/keys");
    compile_object("sys/profiles");
    compile_object("sys/params");
    compile_object("services/obj/server");
}
