#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <common.h>
#include <dt/dt.h>

int main(int argc, char *argv[])
{
	void *fdt;
	struct device_node *root;
	const char *dtbfile = NULL;

	if (argc > 1)
		dtbfile = argv[1];

	if (dtbfile) {
		fdt = read_file(dtbfile, NULL);
		if (!fdt) {
			fprintf(stderr, "Could not read %s: %s\n", dtbfile, strerror(errno));
			exit(1);
		}

		root = of_unflatten_dtb(NULL, fdt);
	} else {
		root = of_read_proc_devicetree();
	}

	if (IS_ERR(root)) {
		fprintf(stderr, "Could not unflatten dtb: %s\n", strerror(-PTR_ERR(root)));
		exit(1);
	}

	printf("/dts-v1/;\n/");

	of_print_nodes(root, 0);

	exit(0);
}
