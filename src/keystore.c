/*
 * keystore - simple keystore
 *
 * Copyright (c) 2015 Pengutronix, Marc Kleine-Budde <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

int __attribute__((weak)) keystore_get_secret(const char *name, const unsigned char **key, int *key_len)
{
	return 0;
}
