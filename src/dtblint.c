/*
 * Copyright (C) 2016 Pengutronix, Uwe Kleine-König <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <dt/dt.h>

#include "dtblint.h"

int main(int argc, const char *argv[])
{
	void *fdt;
	struct device_node *root;

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

	dtblint_imx_pinmux();
}
