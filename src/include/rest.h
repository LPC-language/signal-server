/*
 * This file is part of https://github.com/LPC-language/signal-server
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
# define RestTlsClientSession	object "/usr/MsgServer/lib/TlsClientSession"

# define WEBSOCKET_GUID		"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

# define ARG_ENTITY		1
# define ARG_ENTITY_JSON	2
# define ARG_HEADER_AUTH	3
# define ARG_HEADER_OPT_AUTH	4

# define argEntity()		ARG_ENTITY
# define argEntityJson()	ARG_ENTITY_JSON
# define argHeaderAuth()	ARG_HEADER_AUTH
# define argHeaderOptAuth()	ARG_HEADER_OPT_AUTH
# define argHeader(header)	(header)

# define REST_LENGTH_LIMIT	4194304
