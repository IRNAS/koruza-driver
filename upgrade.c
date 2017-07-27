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
#include "upgrade.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <syslog.h>

int upgrade_init(struct uci_context *uci)
{
  // TODO: Make upgrade script configurable via UCI.
  return 0;
}

int upgrade_start()
{
  // Fork and detach current process.
  pid_t pid = fork();
  if (pid < 0) {
    return -1;
  }

  // Parent returns success.
  if (pid > 0) {
    syslog(LOG_INFO, "Starting upgrade.");
    return 0;
  }

  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

  umask(0);

  // Create new session.
  pid_t sid = setsid();
  if (sid < 0) {
    exit(1);
  }

  // Release lock over current directory.
  if ((chdir("/")) < 0) {
    exit(1);
  }

  // Redirect everything to /dev/null.
  freopen("/dev/null", "r", stdin);
  freopen("/dev/null", "w", stdout);
  freopen("/dev/null", "w", stderr);

  // Exec the upgrade script.
  if (execl(UPGRADE_SCRIPT, UPGRADE_SCRIPT, (char*) NULL) < 0) {
    exit(1);
  }

  return 0;
}
