/*
 * koruza-driver - KORUZA driver
 *
 * Copyright (C) 2017 Jernej Kos <jernej@kos.mx>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef KORUZA_DRIVER_UPGRADE_H
#define KORUZA_DRIVER_UPGRADE_H

#include <uci.h>
#include <libubus.h>

// Location of the upgrade script that is called to perform upgrade.
#define UPGRADE_SCRIPT "/usr/bin/koruza-upgrade"

int upgrade_init(struct uci_context *uci);
int upgrade_start();

#endif
