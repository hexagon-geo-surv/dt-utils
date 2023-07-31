/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * (C) Copyright 2015 Pengutronix, Marc Kleine-Budde <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __KEYSTORE_H
#define __KEYSTORE_H

int keystore_get_secret(const char *name, const unsigned char **key, int *key_len);

#endif
