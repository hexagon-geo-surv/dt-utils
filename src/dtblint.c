/* SPDX-License-Identifier: GPL-2.0-only */
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

static const char * const fsl_fec_compatibles[] = {
	"fsl,imx25-fec",
	"fsl,imx27-fec",
	"fsl,imx28-fec",
	"fsl,imx6q-fec",
	"fsl,imx6sx-fec",
	"fsl,mvf600-fec",
};

static void fsl_fec_reset_polarity(void)
{
	size_t i;
	/*
	 * For historical reasons the gpio flag in the phy-reset-gpios property
	 * isn't evaluated and the gpio is assumed to be active low. Inversion
	 * can be accomplished by adding the property phy-reset-active-high.
	 * The gpio flag should match the presence of this property.
	 */
	struct device_node *np;
	int ret;

	for (i = 0; i < ARRAY_SIZE(fsl_fec_compatibles); ++i) {
		for_each_compatible_node(np, NULL, fsl_fec_compatibles[i]) {
			struct of_phandle_args out_args[MAX_PHANDLE_ARGS];

			bool active_high_property =
				of_property_read_bool(np,
						      "phy-reset-active-high");
			bool active_high_gpio_flag;

			ret = of_parse_phandle_with_args(np, "phy-reset-gpios",
							 "#gpio-cells", 0,
							 out_args);
			if (ret < 0)
				continue;

			if (out_args->args_count < 2)
				continue;

			active_high_gpio_flag = out_args->args[1] == 0;

			if (active_high_gpio_flag != active_high_property) {
				printf("E: phy-reset-gpios flags don't match presence of phy-reset-active-high property (%s)\n",
				       np->full_name);
			}
		}
	}
}

static const char * const fsl_pcie_compatibles[] = {
        "fsl,imx6q-pcie",
        "fsl,imx6sx-pcie",
        "fsl,imx6qp-pcie",
        "fsl,imx7d-pcie",
};

static void fsl_pcie_reset_polarity(void)
{
	size_t i;
	/*
	 * For historical reasons the gpio flag in the reset-gpio property
	 * isn't evaluated and the gpio is assumed to be active low. Inversion
	 * can be accomplished by adding the property reset-gpio-active-high.
	 * The gpio flag should match the presence of this property.
	 */
	struct device_node *np;
	int ret;

	for (i = 0; i < ARRAY_SIZE(fsl_pcie_compatibles); ++i) {
		for_each_compatible_node(np, NULL, fsl_pcie_compatibles[i]) {
			struct of_phandle_args out_args[MAX_PHANDLE_ARGS];

			bool active_high_property =
				of_property_read_bool(np,
						      "reset-gpio-active-high");
			bool active_high_gpio_flag;

			ret = of_parse_phandle_with_args(np, "reset-gpio",
							 "#gpio-cells", 0,
							 out_args);
			if (ret < 0)
				continue;

			if (out_args->args_count < 2)
				continue;

			active_high_gpio_flag = out_args->args[1] == 0;

			if (active_high_gpio_flag != active_high_property) {
				printf("E: reset-gpios flags don't match presence of reset-gpio-active-high property (%s)\n",
				       np->full_name);
			}
		}
	}
}

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
	free(fdt);
	if (IS_ERR(root)) {
		fprintf(stderr, "failed to unflatten device tree (%ld)\n",
			PTR_ERR(root));
		return EXIT_FAILURE;
	}
	of_set_root_node(root);

	dtblint_imx_pinmux();
	fsl_fec_reset_polarity();
	fsl_pcie_reset_polarity();
}
