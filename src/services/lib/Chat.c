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

# ifdef REGISTER

/*
 * register REST API endpoints
 */

# include "chat/Registration.c"
# include "chat/Keys.c"
# include "chat/Accounts.c"
# include "chat/Websocket.c"
# include "chat/Certificate.c"
# include "chat/Config.c"
# include "chat/Profile.c"
# include "chat/Backup.c"
# include "chat/Storage.c"
# include "chat/Directory.c"
# include "chat/Messages.c"
# include "chat/Devices.c"
# include "chat/Provisioning.c"

# else

inherit "chat/Registration";
inherit "chat/Keys";
inherit "chat/Accounts";
inherit "chat/Websocket";
inherit "chat/Certificate";
inherit "chat/Config";
inherit "chat/Profile";
inherit "chat/Backup";
inherit "chat/Storage";
inherit "chat/Directory";
inherit "chat/Messages";
inherit "chat/Devices";
inherit "chat/Provisioning";

# endif
