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

# define REST_API		"/usr/MsgServer/sys/rest_api"

# define RestServer		object "/usr/MsgServer/lib/rest/Server"
# define RestClient		object "/usr/MsgServer/lib/rest/Client"
# define RestTlsClientSession	object "/usr/MsgServer/api/lib/TlsClientSession"

# define ARG_ENTITY		1
# define ARG_ENTITY_JSON	2

# define argHeader(header)	(header)
# define argEntity()		ARG_ENTITY
# define argEntityJson()	ARG_ENTITY_JSON

# define REST_LENGTH_LIMIT	4194304
