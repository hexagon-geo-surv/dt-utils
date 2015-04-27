#ifndef __DT_COMMON_H
#define __DT_COMMON_H

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include <mtd/mtd-abi.h>

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#ifdef DEBUG
#define pr_debug(fmt, arg...)       fprintf(stderr, fmt, ##arg)
#else
#define pr_debug(fmt, arg...)
#endif

#define pr_err(fmt, arg...)       fprintf(stderr, fmt, ##arg)
#define dev_err(dev, fmt, arg...) pr_err(fmt, ##arg)
#define dev_warn(dev, fmt, arg...) pr_err(fmt, ##arg)
#define dev_info(dev, fmt, arg...) pr_err(fmt, ##arg)
#define dev_dbg(dev, fmt, arg...) pr_debug(fmt, ##arg)

static inline void *xzalloc(size_t size)
{
	return calloc(1, size);
}

/*
 * Kernel pointers have redundant information, so we can use a
 * scheme where we can return either an error code or a dentry
 * pointer with the same return value.
 *
 * This should be a per-architecture thing, to allow different
 * error and pointer decisions.
 */
#define MAX_ERRNO	4095

#ifndef __ASSEMBLY__

#define IS_ERR_VALUE(x) ((x) >= (unsigned long)-MAX_ERRNO)

static inline void *ERR_PTR(long error)
{
	return (void *) error;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static inline long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

static inline long IS_ERR_OR_NULL(const void *ptr)
{
	return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}

/**
 * ERR_CAST - Explicitly cast an error-valued pointer to another pointer type
 * @ptr: The pointer to cast.
 *
 * Explicitly cast an error-valued pointer to another pointer type in such a
 * way as to make it clear that's what's going on.
 */
static inline void *ERR_CAST(const void *ptr)
{
	/* cast away the const */
	return (void *) ptr;
}

/**
 * strlcpy - Copy a %NUL terminated string into a sized buffer
 * @dest: Where to copy the string to
 * @src: Where to copy the string from
 * @size: size of destination buffer
 *
 * Compatible with *BSD: the result is always a valid
 * NUL-terminated string that fits in the buffer (unless,
 * of course, the buffer size is zero). It does not pad
 * out the result like strncpy() does.
 */
static inline size_t strlcpy(char *dest, const char *src, size_t size)
{
	size_t ret = strlen(src);

	if (size) {
		size_t len = (ret >= size) ? size - 1 : ret;
		memcpy(dest, src, len);
		dest[len] = '\0';
	}
	return ret;
}

/* Like strncpy but make sure the resulting string is always 0 terminated. */
static inline char * safe_strncpy(char *dst, const char *src, size_t size)
{
	if (!size) return dst;
	dst[--size] = '\0';
	return strncpy(dst, src, size);
}

static inline char *xstrdup(const char *s)
{
	char *p = strdup(s);

	if (!p)
		exit(EXIT_FAILURE);

	return p;
}

static inline int erase(int fd, size_t count, loff_t offset)
{
	struct erase_info_user erase = {
		.start = offset,
		.length = count,
	};

	return ioctl(fd, MEMERASE, &erase);
}

/*
 * read_full - read from filedescriptor
 *
 * Like read, but this function only returns less bytes than
 * requested when the end of file is reached.
 */
static inline int read_full(int fd, void *buf, size_t size)
{
	size_t insize = size;
	int now;
	int total = 0;

	while (size) {
		now = read(fd, buf, size);
		if (now == 0)
			return total;
		if (now < 0)
			return now;
		total += now;
		size -= now;
		buf += now;
	}

	return insize;
}

static inline void *read_file(const char *filename, size_t *size)
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

	if (read_full(fd, buf, s.st_size) < s.st_size)
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


/*
 * write_full - write to filedescriptor
 *
 * Like write, but guarantees to write the full buffer out, else
 * it returns with an error.
 */
static inline int write_full(int fd, void *buf, size_t size)
{
	size_t insize = size;
	int now;

	while (size) {
		now = write(fd, buf, size);
		if (now <= 0)
			return now;
		size -= now;
		buf += now;
	}

	return insize;
}


#define MAX_DRIVER_NAME		32
#define DEVICE_ID_SINGLE	-1

struct device_d {
	char name[MAX_DRIVER_NAME];
	int id;
	struct device_node *device_node;
};

static inline struct param_d *dev_add_param_int(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, const char *format, void *priv)
{
	return NULL;
}

static inline struct param_d *dev_add_param_enum(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, const char **names, int max, void *priv)

{
	return NULL;
}

static inline struct param_d *dev_add_param_bool(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, void *priv)
{
	return NULL;
}

static inline struct param_d *dev_add_param_mac(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		uint8_t *mac, void *priv)
{
	return NULL;
}

struct driver_d;

static inline int register_driver(struct driver_d *d)
{
	return 0;
}

static inline int register_device(struct device_d *d)
{
	return 0;
}
static inline int unregister_device(struct device_d *d)
{
	return 0;
}

#define cpu_to_be32 __cpu_to_be32
#define be32_to_cpu __be32_to_cpu

#define ALIGN(x, a)		__ALIGN_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))

#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))

#define __maybe_unused			__attribute__((unused))

#endif

uint32_t crc32(uint32_t crc, const void *_buf, unsigned int len);
uint32_t crc32_no_comp(uint32_t crc, const void *_buf, unsigned int len);

#endif /* __DT_COMMON_H */
