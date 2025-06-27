/*
 * This file is part of https://github.com/LPC-language/signal-server
 * Copyright (C) 2024-2025 Dworkin B.V.  All rights reserved.
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

# include "zkp.h"
# include "Sho.h"
# include "params.h"
# include "credentials.h"

private inherit asn "/lib/util/asn";


/*
 * time in seconds encoded in 8 bytes, big endian
 */
static string timeBytes(int time)
{
    return asn::unsignedExtend(asn::extend(asn::encode(time), 4), 8);
}

/*
 * get a scalar from a time
 */
static Scalar timeScalar(int time)
{
    Sho sho;

    sho = PARAMS->timeSho();
    sho->absorb(timeBytes(time));
    sho->ratchet();
    return sho->getScalar();
}

/*
 * time % 86400, where time can be negative
 */
static int timeDay(int time)
{
    time >>= 7;
    return (time - time % 675) << 7;
}
