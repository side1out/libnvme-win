// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Windows-specific NVMe implementation
 *
 * Copyright (c) 2026, James Huey <side1out@yahoo.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#ifndef _SYS_SYSLOG_H
#define _SYS_SYSLOG_H 1

#define        LOG_EMERG        0        /* system is unusable */
#define        LOG_ALERT        1        /* action must be taken immediately */
#define        LOG_CRIT        2        /* critical conditions */
#define        LOG_ERR                3        /* error conditions */
#define        LOG_WARNING        4        /* warning conditions */
#define        LOG_NOTICE        5        /* normal but significant condition */
#define        LOG_INFO        6        /* informational */
#define        LOG_DEBUG        7        /* debug-level messages */

#endif