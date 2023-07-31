/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Code based on busybox-1.23.2
 *
 * Copyright 2003, Glenn McGrath
 * Copyright 2006, Rob Landley <rob@landley.net>
 * Copyright 2010, Denys Vlasenko
 */

#include <common.h>
#include <base64.h>

/* Conversion table.  for base 64 */
static const char uuenc_tbl_base64[65 + 1] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/',
	'=' /* termination character */,
	'\0' /* needed for uudecode.c only */
};

/*
 * Decode base64 encoded string. Stops on '\0'.
 *
 */
int decode_base64(char *p_dst, int dst_len, const char *src)
{
	char *dst = p_dst;
	int length = 0;

	while (dst_len > 0) {
		unsigned char six_bit[4];
		int count = 0;

		/* Fetch up to four 6-bit values */
		while (count < 4) {
			const char *table_ptr;
			int ch;

			/*
			 * Get next _valid_ character.
			 * uuenc_tbl_base64[] contains this string:
			 *  0         1         2         3         4         5         6
			 *  01234567890123456789012345678901234567890123456789012345678901234
			 * "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="
			 */
			do {
				ch = *src;
				if (ch == '\0')
					goto ret;

				src++;
				table_ptr = strchr(uuenc_tbl_base64, ch);
			} while (!table_ptr);

			/* Convert encoded character to decimal */
			ch = table_ptr - uuenc_tbl_base64;

			/* ch is 64 if char was '=', otherwise 0..63 */
			if (ch == 64)
				break;

			six_bit[count] = ch;
			count++;
		}

		/*
		 * Transform 6-bit values to 8-bit ones.
		 * count can be < 4 when we decode the tail:
		 * "eQ==" -> "y", not "y NUL NUL".
		 * Note that (count > 1) is always true,
		 * "x===" encoding is not valid:
		 * even a single zero byte encodes as "AA==".
		 * However, with current logic we come here with count == 1
		 * when we decode "==" tail.
		 */
		if (count > 1)
			*dst++ = six_bit[0] << 2 | six_bit[1] >> 4;
		if (count > 2)
			*dst++ = six_bit[1] << 4 | six_bit[2] >> 2;
		if (count > 3)
			*dst++ = six_bit[2] << 6 | six_bit[3];
		/*
		 * Note that if we decode "AA==" and ate first '=',
		 * we just decoded one char (count == 2) and now we'll
		 * do the loop once more to decode second '='.
		 */
		dst_len -= count-1;
		/* last character was "=" */
		if (count != 0)
			length += count - 1;
	}
ret:
	p_dst = dst;

	return length;
}
