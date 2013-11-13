#include <linux/types.h>

#include <stdio.h>
#include <dt/dt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

void *read_file(const char *filename, size_t *size)
{
	int fd;
	struct stat s;
	void *buf = NULL;
	int ret;

	if (stat(filename, &s))
		return NULL;

	buf = xzalloc(s.st_size + 1);

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		goto err_out;

	if (read(fd, buf, s.st_size) < s.st_size)
		goto err_out1;

	close(fd);

	if (size)
		*size = s.st_size;

	return buf;

err_out1:
	close(fd);
err_out:
	free(buf);

	return NULL;
}

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
