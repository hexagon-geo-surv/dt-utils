/*
 * Copyright (C) 2015 Pengutronix, Uwe Kleine-König <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <dt/dt.h>

#define IMX_NO_PAD_CTL	0x80000000	/* no pin config need */
#define IMX_PAD_SION	0x40000000	/* set SION */

#define IOMUXC_CONFIG_SION	(1 << 4)

struct padinfo {
	const char *padname;
	off_t swmux_regoffset;
	uint32_t swmux_reset_default;
	uint32_t swmux_writeable_mask;
	off_t swpad_regoffset;
	uint32_t swpad_reset_default;
	uint32_t swpad_writeable_mask;
};

struct socinfo {
	const struct padinfo *padinfo;
	size_t size_padinfo;
};

static const struct padinfo imx25_iomux_padinfo[] = {
	{
		.padname = "A10",
		.swmux_regoffset = 0x008,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = -1,
	}, {
		.padname = "A13",
		.swmux_regoffset = 0x00c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x22c,
		.swpad_reset_default = 0x80,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "A14",
		.swmux_regoffset = 0x010,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x230,
		.swpad_reset_default = 0x80,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "A15",
		.swmux_regoffset = 0x014,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x234,
		.swpad_reset_default = 0x80,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "A16",
		.swmux_regoffset = 0x018,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = -1,
	}, {
		.padname = "A17",
		.swmux_regoffset = 0x01c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x238,
		.swpad_reset_default = 0x00,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "A18",
		.swmux_regoffset = 0x020,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x23c,
		.swpad_reset_default = 0x00,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "A19",
		.swmux_regoffset = 0x024,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x240,
		.swpad_reset_default = 0x00,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "A20",
		.swmux_regoffset = 0x028,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x244,
		.swpad_reset_default = 0x00,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "A21",
		.swmux_regoffset = 0x02c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x248,
		.swpad_reset_default = 0x00,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "A22",
		.swmux_regoffset = 0x030,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = -1,
	}, {
		.padname = "A23",
		.swmux_regoffset = 0x034,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x24c,
		.swpad_reset_default = 0x00,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "A24",
		.swmux_regoffset = 0x038,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x250,
		.swpad_reset_default = 0x00,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "A25",
		.swmux_regoffset = 0x03c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x254,
		.swpad_reset_default = 0x00,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "EB0",
		.swmux_regoffset = 0x040,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x258,
		.swpad_reset_default = 0x00,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "EB1",
		.swmux_regoffset = 0x044,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x25c,
		.swpad_reset_default = 0x00,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "OE",
		.swmux_regoffset = 0x048,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x260,
		.swpad_reset_default = 0x00,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "CS0",
		.swmux_regoffset = 0x04c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = -1,
	}, {
		.padname = "CS1",
		.swmux_regoffset = 0x050,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = -1,
	}, {
		.padname = "CS4",
		.swmux_regoffset = 0x054,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x264,
		.swpad_reset_default = 0x2001,
		.swpad_writeable_mask = 0x20b1,
	}, {
		.padname = "CS5",
		.swmux_regoffset = 0x058,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x268,
		.swpad_reset_default = 0x2001,
		.swpad_writeable_mask = 0x21b1,
	}, {
		.padname = "NF_CE0",
		.swmux_regoffset = 0x05c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x26c,
		.swpad_reset_default = 0x0001,
		.swpad_writeable_mask = 0x81,
	}, {
		.padname = "ECB",
		.swmux_regoffset = 0x060,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x270,
		.swpad_reset_default = 0x2e80,
		.swpad_writeable_mask = 0x2180,
	}, {
		.padname = "LBA",
		.swmux_regoffset = 0x064,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x274,
		.swpad_reset_default = 0x0000,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "BCLK",
		.swmux_regoffset = 0x068,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = -1,
	}, {
		.padname = "RW",
		.swmux_regoffset = 0x06c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x278,
		.swpad_reset_default = 0x0000,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "NFWE_B",
		.swmux_regoffset = 0x070,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = -1,
	}, {
		.padname = "NFRE_B",
		.swmux_regoffset = 0x074,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = -1,
	}, {
		.padname = "NFALE",
		.swmux_regoffset = 0x078,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = -1,
	}, {
		.padname = "NFCLE",
		.swmux_regoffset = 0x07c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = -1,
	}, {
		.padname = "NFWP_B",
		.swmux_regoffset = 0x080,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = -1,
	}, {
		.padname = "NFRB",
		.swmux_regoffset = 0x084,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x27c,
		.swpad_reset_default = 0x00e0,
		.swpad_writeable_mask = 0x80,
	}, {
		.padname = "D15",
		.swmux_regoffset = 0x088,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x280,
		.swpad_reset_default = 0x00a1,
		.swpad_writeable_mask = 0x01f1,
	}, {
		.padname = "D14",
		.swmux_regoffset = 0x08c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x284,
		.swpad_reset_default = 0x00a1,
		.swpad_writeable_mask = 0x01f1,
	}, {
		.padname = "D13",
		.swmux_regoffset = 0x090,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x288,
		.swpad_reset_default = 0x00a1,
		.swpad_writeable_mask = 0x01f1,
	}, {
		.padname = "D12",
		.swmux_regoffset = 0x094,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x28c,
		.swpad_reset_default = 0x00a1,
		.swpad_writeable_mask = 0x01f1,
	}, {
		.padname = "D11",
		.swmux_regoffset = 0x098,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x290,
		/*
		 * specified as 0xa1 in IMX25RM; IMX25CEC says
		 * "100 kOhm Pull-Up" which would correspond to 0xe1.
		 */
		.swpad_reset_default = 0x0021,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "D10",
		.swmux_regoffset = 0x09c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x294,
		.swpad_reset_default = 0x00a1,
		.swpad_writeable_mask = 0x01f1,
	}, {
		.padname = "D9",
		.swmux_regoffset = 0x0a0,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x298,
		.swpad_reset_default = 0x00a1,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "D8",
		.swmux_regoffset = 0x0a4,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x29c,
		.swpad_reset_default = 0x00a1,
		.swpad_writeable_mask = 0x01f1,
	}, {
		.padname = "D7",
		.swmux_regoffset = 0x0a8,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x2a0,
		.swpad_reset_default = 0x0080,
		.swpad_writeable_mask = 0x0040,
	}, {
		.padname = "D6",
		.swmux_regoffset = 0x0ac,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x2a4,
		.swpad_reset_default = 0x0080,
		.swpad_writeable_mask = 0x0040,
	}, {
		.padname = "D5",
		.swmux_regoffset = 0x0b0,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x2a8,
		.swpad_reset_default = 0x0080,
		.swpad_writeable_mask = 0x0040,
	}, {
		.padname = "D4",
		.swmux_regoffset = 0x0b4,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x2ac,
		.swpad_reset_default = 0x0080,
		.swpad_writeable_mask = 0x0040,
	}, {
		.padname = "D3",
		.swmux_regoffset = 0x0b8,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x2b0,
		.swpad_reset_default = 0x0080,
		.swpad_writeable_mask = 0x0040,
	}, {
		.padname = "D2",
		.swmux_regoffset = 0x0bc,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x2b4,
		.swpad_reset_default = 0x0080,
		.swpad_writeable_mask = 0x0040,
	}, {
		.padname = "D1",
		.swmux_regoffset = 0x0c0,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x2b8,
		.swpad_reset_default = 0x0000,
		.swpad_writeable_mask = 0x0040,
	}, {
		.padname = "D0",
		.swmux_regoffset = 0x0c4,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = 0x2bc,
		.swpad_reset_default = 0x0080,
		.swpad_writeable_mask = 0x0040,
	}, {
		.padname = "LD0",
		.swmux_regoffset = 0x0c8,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2c0,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f0,
	}, {
		.padname = "LD1",
		.swmux_regoffset = 0x0cc,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2c4,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f0,
	}, {
		.padname = "LD2",
		.swmux_regoffset = 0x0d0,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2c8,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "LD3",
		.swmux_regoffset = 0x0d4,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2cc,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f0,
	}, {
		.padname = "LD4",
		.swmux_regoffset = 0x0d8,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2d0,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "LD5",
		.swmux_regoffset = 0x0dc,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2d4,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "LD6",
		.swmux_regoffset = 0x0e0,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2d8,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "LD7",
		.swmux_regoffset = 0x0e4,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2dc,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "LD8",
		.swmux_regoffset = 0x0e8,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2e0,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "LD9",
		.swmux_regoffset = 0x0ec,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2e4,
		.swpad_reset_default = 0x0160,
		.swpad_writeable_mask = 0x01f1,
	}, {
		.padname = "LD10",
		.swmux_regoffset = 0x0f0,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2e8,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "LD11",
		.swmux_regoffset = 0x0f4,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2ec,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "LD12",
		.swmux_regoffset = 0x0f8,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2f0,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f9,
	}, {
		.padname = "LD13",
		.swmux_regoffset = 0x0fc,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2f4,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f9,
	}, {
		.padname = "LD14",
		.swmux_regoffset = 0x100,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2f8,
		.swpad_reset_default = 0x0020,
		.swpad_writeable_mask = 0x00b8,
	}, {
		.padname = "LD15",
		.swmux_regoffset = 0x104,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x2fc,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f8,
	}, {
		.padname = "HSYNC",
		.swmux_regoffset = 0x108,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x300,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f9,
	}, {
		.padname = "VSYNC",
		.swmux_regoffset = 0x10c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x304,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f9,
	}, {
		.padname = "LSCLK",
		.swmux_regoffset = 0x110,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x308,
		.swpad_reset_default = 0x0061,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "OE_ACD",
		.swmux_regoffset = 0x114,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x30c,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "CONTRAST",
		.swmux_regoffset = 0x118,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x310,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f0,
	}, {
		.padname = "PWM",
		.swmux_regoffset = 0x11c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x314,
		.swpad_reset_default = 0x00c0,
		.swpad_writeable_mask = 0x00f6,
	}, {
		.padname = "CSI_D2",
		.swmux_regoffset = 0x120,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x318,
		.swpad_reset_default = 0x00a1,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "CSI_D3",
		.swmux_regoffset = 0x124,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x31c,
		.swpad_reset_default = 0x00a0,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "CSI_D4",
		.swmux_regoffset = 0x128,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x320,
		.swpad_reset_default = 0x01a1,
		.swpad_writeable_mask = 0x01f1,
	}, {
		.padname = "CSI_D5",
		.swmux_regoffset = 0x12c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x324,
		.swpad_reset_default = 0x00a0,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "CSI_D6",
		.swmux_regoffset = 0x130,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x328,
		.swpad_reset_default = 0x00a0,
		.swpad_writeable_mask = 0x00f9,
	}, {
		.padname = "CSI_D7",
		.swmux_regoffset = 0x134,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x32c,
		.swpad_reset_default = 0x01a0,
		.swpad_writeable_mask = 0x01f9,
	}, {
		.padname = "CSI_D8",
		.swmux_regoffset = 0x138,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x330,
		.swpad_reset_default = 0x00a0,
		.swpad_writeable_mask = 0x00f9,
	}, {
		.padname = "CSI_D9",
		.swmux_regoffset = 0x13c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x334,
		.swpad_reset_default = 0x00a0,
		.swpad_writeable_mask = 0x00f9,
	}, {
		.padname = "CSI_MCLK",
		.swmux_regoffset = 0x140,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x338,
		.swpad_reset_default = 0x0061,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "CSI_VSYNC",
		.swmux_regoffset = 0x144,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x33c,
		.swpad_reset_default = 0x00a0,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "CSI_HSYNC",
		.swmux_regoffset = 0x148,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x340,
		.swpad_reset_default = 0x00a0,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "CSI_PIXCLK",
		.swmux_regoffset = 0x14c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x344,
		.swpad_reset_default = 0x01a0,
		.swpad_writeable_mask = 0x01f1,
	}, {
		.padname = "I2C1_CLK",
		.swmux_regoffset = 0x150,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x348,
		.swpad_reset_default = 0x00e8,
		.swpad_writeable_mask = 0x00be,
	}, {
		.padname = "I2C1_DAT",
		.swmux_regoffset = 0x154,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x34c,
		.swpad_reset_default = 0x00e8,
		.swpad_writeable_mask = 0x00be,
	}, {
		.padname = "CSPI1_MOSI",
		.swmux_regoffset = 0x158,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x350,
		.swpad_reset_default = 0x00e0,
		.swpad_writeable_mask = 0x00b1,
	}, {
		.padname = "CSPI1_MISO",
		.swmux_regoffset = 0x15c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x354,
		.swpad_reset_default = 0x00a0,
		.swpad_writeable_mask = 0x00b1,
	}, {
		.padname = "CSPI1_SS0",
		.swmux_regoffset = 0x160,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x358,
		.swpad_reset_default = 0x00e0,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "CSPI1_SS1",
		.swmux_regoffset = 0x164,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x35c,
		.swpad_reset_default = 0x00e0,
		.swpad_writeable_mask = 0x00b9,
	}, {
		.padname = "CSPI1_SCLK",
		.swmux_regoffset = 0x168,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x360,
		.swpad_reset_default = 0x00e0,
		.swpad_writeable_mask = 0x00b1,
	}, {
		.padname = "CSPI1_RDY",
		.swmux_regoffset = 0x16c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x364,
		.swpad_reset_default = 0x00e0,
		.swpad_writeable_mask = 0x00b1,
	}, {
		.padname = "UART1_RXD",
		.swmux_regoffset = 0x170,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x368,
		.swpad_reset_default = 0x00a0,
		.swpad_writeable_mask = 0x00b0,
	}, {
		.padname = "UART1_TXD",
		.swmux_regoffset = 0x174,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x36c,
		.swpad_reset_default = 0x0020,
		.swpad_writeable_mask = 0x00b0,
	}, {
		.padname = "UART1_RTS",
		.swmux_regoffset = 0x178,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x370,
		.swpad_reset_default = 0x00e0,
		.swpad_writeable_mask = 0x00f0,
	}, {
		.padname = "UART1_CTS",
		.swmux_regoffset = 0x17c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x374,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f0,
	}, {
		.padname = "UART2_RXD",
		.swmux_regoffset = 0x180,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x378,
		.swpad_reset_default = 0x00e0,
		.swpad_writeable_mask = 0x00f0,
	}, {
		.padname = "UART2_TXD",
		.swmux_regoffset = 0x184,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x37c,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "UART2_RTS",
		.swmux_regoffset = 0x188,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x380,
		.swpad_reset_default = 0x00e1,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "UART2_CTS",
		.swmux_regoffset = 0x18c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x384,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "SD1_CMD",
		.swmux_regoffset = 0x190,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x388,
		.swpad_reset_default = 0x00d1,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "SD1_CLK",
		.swmux_regoffset = 0x194,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x38c,
		.swpad_reset_default = 0x00d1,
		.swpad_writeable_mask = 0x01f1,
	}, {
		.padname = "SD1_DATA0",
		.swmux_regoffset = 0x198,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x390,
		.swpad_reset_default = 0x00d1,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "SD1_DATA1",
		.swmux_regoffset = 0x19c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x394,
		.swpad_reset_default = 0x00d1,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "SD1_DATA2",
		.swmux_regoffset = 0x1a0,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x398,
		.swpad_reset_default = 0x00d1,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "SD1_DATA3",
		.swmux_regoffset = 0x1a4,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x39c,
		.swpad_reset_default = 0x00d1,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "KPP_ROW0",
		.swmux_regoffset = 0x1a8,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3a0,
		.swpad_reset_default = 0x00e0,
		.swpad_writeable_mask = 0x00b8,
	}, {
		.padname = "KPP_ROW1",
		.swmux_regoffset = 0x1ac,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3a4,
		.swpad_reset_default = 0x00e0,
		.swpad_writeable_mask = 0x00b8,
	}, {
		.padname = "KPP_ROW2",
		.swmux_regoffset = 0x1b0,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3a8,
		.swpad_reset_default = 0x00e0,
		.swpad_writeable_mask = 0x00f8,
	}, {
		.padname = "KPP_ROW3",
		.swmux_regoffset = 0x1b4,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3ac,
		.swpad_reset_default = 0x00e0,
		.swpad_writeable_mask = 0x00f8,
	}, {
		.padname = "KPP_COL0",
		.swmux_regoffset = 0x1b8,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3b0,
		.swpad_reset_default = 0x00a8,
		.swpad_writeable_mask = 0x00b8,
	}, {
		.padname = "KPP_COL1",
		.swmux_regoffset = 0x1bc,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3b4,
		.swpad_reset_default = 0x00a8,
		.swpad_writeable_mask = 0x00b8,
	}, {
		.padname = "KPP_COL2",
		.swmux_regoffset = 0x1c0,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3b8,
		.swpad_reset_default = 0x00a8,
		.swpad_writeable_mask = 0x00b8,
	}, {
		.padname = "KPP_COL3",
		.swmux_regoffset = 0x1c4,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3bc,
		.swpad_reset_default = 0x00a8,
		.swpad_writeable_mask = 0x00b8,
	}, {
		.padname = "FEC_MDC",
		.swmux_regoffset = 0x1c8,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3c0,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "FEC_MDIO",
		.swmux_regoffset = 0x1cc,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3c4,
		.swpad_reset_default = 0x01f0,
		.swpad_writeable_mask = 0x01f1,
	}, {
		.padname = "FEC_TDATA0",
		.swmux_regoffset = 0x1d0,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3c8,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "FEC_TDATA1",
		.swmux_regoffset = 0x1d4,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3cc,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f1,
	}, {
		.padname = "FEC_TX_EN",
		.swmux_regoffset = 0x1d8,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3d0,
		.swpad_reset_default = 0x0060,
		.swpad_writeable_mask = 0x00f9,
	}, {
		.padname = "FEC_RDATA0",
		.swmux_regoffset = 0x1dc,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3d4,
		.swpad_reset_default = 0x00c1,
		.swpad_writeable_mask = 0x00f9,
	}, {
		.padname = "FEC_RDATA1",
		.swmux_regoffset = 0x1e0,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3d8,
		.swpad_reset_default = 0x00c0,
		.swpad_writeable_mask = 0x00f9,
	}, {
		.padname = "FEC_RX_DV",
		.swmux_regoffset = 0x1e4,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3dc,
		.swpad_reset_default = 0x00c0,
		.swpad_writeable_mask = 0x00f9,
	}, {
		.padname = "FEC_TX_CLK",
		.swmux_regoffset = 0x1e8,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3e0,
		.swpad_reset_default = 0x00c0,
		.swpad_writeable_mask = 0x01f1,
	}, {
		.padname = "RTCK",
		.swmux_regoffset = 0x1ec,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3e4,
		.swpad_reset_default = 0x0062,
		.swpad_writeable_mask = 0x00ff,
	}, {
		.padname = "TDO",
		.swpad_regoffset = 0x3e8,
		.swpad_reset_default = 0x0002,
		.swpad_writeable_mask = 0x0006,
	}, {
		.padname = "DE_B",
		.swmux_regoffset = 0x1f0,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3ec,
		.swpad_reset_default = 0x00d0,
		.swpad_writeable_mask = 0x0006,
	}, {
		.padname = "GPIO_A",
		.swmux_regoffset = 0x1f4,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3f0,
		/* IMX25CE: no configuration after reset */
		.swpad_reset_default = 0x00c0,
		.swpad_writeable_mask = 0x00fe,
	}, {
		.padname = "GPIO_B",
		.swmux_regoffset = 0x1f8,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3f4,
		.swpad_reset_default = 0x00c0,
		.swpad_writeable_mask = 0x00fe,
	}, {
		.padname = "GPIO_C",
		.swmux_regoffset = 0x1fc,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3f8,
		.swpad_reset_default = 0x00c0,
		.swpad_writeable_mask = 0x00fe,
	}, {
		.padname = "GPIO_D",
		.swmux_regoffset = 0x200,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x3fc,
		.swpad_reset_default = 0x0020,
		.swpad_writeable_mask = 0x00be,
	}, {
		.padname = "GPIO_E",
		.swmux_regoffset = 0x204,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x400,
		.swpad_reset_default = 0x00e8,
		.swpad_writeable_mask = 0x00be,
	}, {
		.padname = "GPIO_F",
		.swmux_regoffset = 0x208,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x404,
		.swpad_reset_default = 0x0020,
		.swpad_writeable_mask = 0x00b6,
	}, {
		.padname = "EXT_ARMCLK",
		.swmux_regoffset = 0x20c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = -1,
	}, {
		.padname = "UPLL_BYPCLK",
		.swmux_regoffset = 0x210,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = -1,
	}, {
		.padname = "VSTBY_REQ",
		.swmux_regoffset = 0x214,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x408,
		.swpad_reset_default = 0x0000,
		.swpad_writeable_mask = 0x0086,
	}, {
		.padname = "VSTBY_ACK",
		.swmux_regoffset = 0x218,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x40c,
		.swpad_reset_default = 0x0080,
		.swpad_writeable_mask = 0x00b6,
	}, {
		.padname = "POWER_FAIL",
		.swmux_regoffset = 0x21c,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x410,
		.swpad_reset_default = 0x00c0,
		.swpad_writeable_mask = 0x00b6,
	}, {
		.padname = "CLKO",
		.swmux_regoffset = 0x220,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000017,
		.swpad_regoffset = 0x414,
		.swpad_reset_default = 0x0004,
		.swpad_writeable_mask = 0x0006,
	}, {
		.padname = "BOOT_MODE0",
		.swmux_regoffset = 0x224,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = -1,
	}, {
		.padname = "BOOT_MODE1",
		.swmux_regoffset = 0x228,
		.swmux_reset_default = 0x00000000,
		.swmux_writeable_mask = 0x00000007,
		.swpad_regoffset = -1,
	},
};

static const struct socinfo imx25_socinfo = {
	.padinfo = imx25_iomux_padinfo,
	.size_padinfo = ARRAY_SIZE(imx25_iomux_padinfo),
};

static const struct padinfo *find_padinfo(off_t swmux_regoffset,
					  const struct socinfo *socinfo)
{
	size_t i;
	for (i = 0; i < socinfo->size_padinfo; ++i) {
		if (socinfo->padinfo[i].swmux_regoffset == swmux_regoffset)
			return &socinfo->padinfo[i];
	}

	return NULL;
}

static int parse_function(struct device_node *funcnode,
			  const struct socinfo *socinfo)
{
	const __be32 *list;
	int size, i;

	list = of_get_property(funcnode, "fsl,pins", &size);
	if (!list) {
		fprintf(stderr, "no fsl,pins property in node %s\n", funcnode->full_name);
		return -EINVAL;
	}

	if (!size || size % 24) {
		fprintf(stderr, "fsl,pins invalid in node %s\n", funcnode->full_name);
		return -EINVAL;
	}

	for (i = 0; i < size / 24; ++i) {
		uint32_t mux_reg = be32_to_cpu(*list++);
		off_t conf_reg;
		uint32_t mux_mode;
		uint32_t input_reg;
		uint32_t input_val;
		uint32_t config;
		const struct padinfo *padinfo;

		conf_reg = be32_to_cpu(*list++);
		if (!conf_reg)
			conf_reg = -1;

		input_reg = be32_to_cpu(*list++);
		(void)input_reg;
		mux_mode = be32_to_cpu(*list++);
		input_val = be32_to_cpu(*list++);
		(void)input_val;
		config = be32_to_cpu(*list++);
		if (config & IMX_PAD_SION)
			mux_mode |= IOMUXC_CONFIG_SION;
		config = config & ~IMX_PAD_SION;

		/* sanity checks */
		padinfo = find_padinfo(mux_reg, socinfo);
		if (!padinfo) {
			fprintf(stderr, "unknown pad (muxreg: %lx)\n",
				(unsigned long)mux_reg);
			continue;
		}

		if (conf_reg != padinfo->swpad_regoffset)
			printf("E: wrong offset for SW_PAD register (%s)\n",
			       padinfo->padname);

		if (conf_reg == -1) {
			if (config != 0x80000000)
				printf("E: config value without config register (%s)\n",
				       padinfo->padname);
		} else {
			if (config == 0x80000000) {
				printf("S: explicitly use 0x%08x as config value for %s\n",
				       padinfo->swpad_reset_default &
				       padinfo->swpad_writeable_mask,
				       padinfo->padname);
			} else if (config & ~padinfo->swpad_writeable_mask) {
				printf("E: config value specified for reserved bit (%s)\n",
				       padinfo->padname);
			}
		}
	}
	return 0;
}

struct iomux_id_t {
	const char *compatible;
	const struct socinfo *socinfo;
};

static const struct iomux_id_t iomux_id[] = {
	{
		.compatible = "fsl,imx25-iomuxc",
		.socinfo = &imx25_socinfo,
	},
};

int main(int argc, const char *argv[])
{
	void *fdt;
	struct device_node *root, *np;
	size_t i;

	if (argc < 2) {
		fprintf(stderr, "No filename given\n");
		return EXIT_FAILURE;
	}

	fdt = read_file(argv[1], NULL);
	if (!fdt) {
		fprintf(stderr, "failed to read dtb\n");
		return EXIT_FAILURE;
	}

	root = of_unflatten_dtb(fdt);
	if (IS_ERR(root)) {
		fprintf(stderr, "failed to unflatten device tree (%ld)\n", PTR_ERR(root));
		return EXIT_FAILURE;
	}
	of_set_root_node(root);

	for (i = 0; i < ARRAY_SIZE(iomux_id); ++i) {
		for_each_compatible_node(np, NULL, iomux_id[i].compatible) {
			struct device_node *npc;

			pr_debug("Found iomuxc %s\n", np->full_name);

			for_each_child_of_node(np, npc) {
				if (of_property_read_bool(npc, "fsl,pins")) {
					pr_debug("Found function node %s\n", npc->full_name);
					parse_function(npc, iomux_id[i].socinfo);
				} else {
					struct device_node *npcc;

					for_each_child_of_node(npc, npcc)
						parse_function(npcc,
							       iomux_id[i].socinfo);
				}
			}
		}
	}

	return EXIT_SUCCESS;
}
