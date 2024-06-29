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

# include "~TLS/x509.h"

private inherit asn "/lib/util/asn";


/*
 * padding
 */
private string ps(int length)
{
    string str;

    str = "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";
    while (strlen(str) < length) {
	str += str;
    }
    return str[.. length - 1];
}

/*
 * EMSA-PKCS1-v1_5 encode with SHA256
 */
private string emsa_pkcs1_v15(string message, int len)
{
    string t;

    t = "\x30\x31\x30\x0d\x06\x09" + OID_SHA256 + "\x05\x00\x04\x20" +
	hash_string("SHA256", message);
    return "\x00\x01" + ps(len - 3 - strlen(t)) + "\x00" + t;
}

/*
 * produce RS256 signature (RFC 8017 section 8.2)
 */
static string rs256Sign(string message, X509Key key)
{
    int len;
    string modulus, p, q, t, m, d;

    modulus = "\0" + key->modulus();
    p = "\0" + key->prime1();
    q = "\0" + key->prime2();

    len = (asn::bits(modulus) + 7) / 8;
    message = emsa_pkcs1_v15(message, len);

    m = asn_pow(message, "\0" + key->exponent2(), q);
    d = asn_sub(asn_pow(message, "\0" + key->exponent1(), p), m, p);
    if (d[0] & 0x80) {
	d = asn_add(d, p, p);
    }
    m = asn_add(asn_mult(asn_mult("\0" + key->coefficient(), d, p),
			 q,
			 modulus),
		m,
		modulus);

    if (strlen(m) > len) {
	m = m[1 ..];
    } else {
	m = asn::unsignedExtend(m, len);
    }
    return m;
}
