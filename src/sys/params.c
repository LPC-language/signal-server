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

# include "zkp.h"
# include "Sho.h"


private RistrettoPoint *cParams;	/* credential params */
private RistrettoPoint *pkcParams;	/* ProfileKeyCommitment param */
private Sho ssSho;			/* ServerSecret sho */
private Sho sSho;			/* signkey sho */
private Sho pkcSho;			/* ProfileKeCredential sho */
private Sho uuidSho;			/* uuid sho */
private Sho timeSho;			/* Time Sho */

/*
 * initialize system parameters
 */
static void create()
{
    Sho sho;

    sho = new ShoHmacSha256("Signal_ZKGroup_20200424_Constant_Credentials_SystemParams_Generate");
    sho->absorb("");
    sho->ratchet();
    cParams = ({
	sho->getPoint(),		/* G_w */
	sho->getPoint(),		/* G_wprime */
	sho->getPoint(),		/* G_x0 */
	sho->getPoint(),		/* G_x1 */
	sho->getPoint(),		/* G_y1 */
	sho->getPoint(),		/* G_y2 */
	sho->getPoint(),		/* G_y3 */
	sho->getPoint(),		/* G_y4 */
	sho->getPoint(),		/* G_m1 */
	sho->getPoint(),		/* G_m2 */
	sho->getPoint(),		/* G_m3 */
	sho->getPoint(),		/* G_m4 */
	sho->getPoint(),		/* G_V */
	sho->getPoint(),		/* G_z */
	sho->getPoint(),		/* G_y5 */
	sho->getPoint(),		/* G_y6 */
	sho->getPoint()			/* G_m5 */
    });
    cParams = cParams[.. 3] +		/* G_w, G_wprime, G_x0-1 */
	      cParams[4 .. 7] +		/* G_y1-4 */
	      cParams[14 .. 15] +	/* G_y5-6 */
	      cParams[8 .. 11] +	/* G_m1-4 */
	      cParams[16 .. 16] +	/* G_m5 */
	      cParams[12 .. 13];	/* G_V, G_z */

    sho = new ShoHmacSha256("Signal_ZKGroup_20200424_Constant_ProfileKeyCommitment_SystemParams_Generate");
    sho->absorb("");
    sho->ratchet();
    pkcParams = ({
	sho->getPoint(),
	sho->getPoint(),
	sho->getPoint()
    });

    ssSho = new ShoHmacSha256("Signal_ZKGroup_20200424_Random_ServerSecretParams_Generate");
    sSho = new ShoHmacSha256("Signal_ZKGroup_20200424_Random_ServerSecretParams_Sign");
    pkcSho = new ShoHmacSha256("Signal_ZKGroup_20220508_Random_ServerSecretParams_IssueExpiringProfileKeyCredential");
    uuidSho = new ShoHmacSha256("Signal_ZKGroup_20200424_UID_CalcM1");
    timeSho = new ShoHmacSha256("Signal_ZKGroup_20220524_Timestamp_Calc_m");
}


RistrettoPoint *credentialParams()		{ return cParams[..]; }
RistrettoPoint *profileKeyCommitmentParams()	{ return pkcParams[..]; }
Sho serverSecretSho()				{ return ssSho->clone(); }
Sho signSho()					{ return sSho->clone(); }
Sho profileKeyCredentialSho()			{ return pkcSho->clone(); }
Sho uuidSho()					{ return uuidSho->clone(); }
Sho timeSho()					{ return timeSho->clone(); }
