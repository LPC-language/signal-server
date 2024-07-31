/*
 * This file is part of https://github.com/LPC-language/signal-server
 * Copyright (C) 2024 Dworkin B.V.
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

# include "~TLS/tls.h"

inherit TlsClientSession;


/*
 * override cipherSuites
 */
static string *cipherSuites()
{
    return ({
	TLS_AES_256_GCM_SHA384,
	TLS_AES_128_GCM_SHA256,
	TLS_CHACHA20_POLY1305_SHA256,
	TLS_AES_128_CCM_SHA256,
	TLS_AES_128_CCM_8_SHA256
    });
}

/*
 * override supportedGroups
 */
static string *supportedGroups()
{
    return ({
	TLS_X25519,
	TLS_X448,
	TLS_SECP256R1,
	TLS_SECP384R1,
	TLS_SECP521R1,
	TLS_FFDHE2048,
	TLS_FFDHE3072,
	TLS_FFDHE4096,
	TLS_FFDHE6144,
	TLS_FFDHE8192
    });
}
