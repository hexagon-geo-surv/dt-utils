#ifndef __DT_COMMON_H
#define __DT_COMMON_H

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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

#ifndef ENOTSUPP
#define ENOTSUPP	524
#endif

static inline void *xzalloc(size_t size)
{
	return calloc(1, size);
}

static inline void *xmalloc(size_t size)
{
	return xzalloc(size);
}

#define EXPORT_SYMBOL(sym)
#define EXPORT_SYMBOL_GPL(sym)

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

static inline char *barebox_asprintf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));
static inline char *barebox_asprintf(const char *fmt, ...)
{
	va_list ap;
	char *p;
	int ret;

	va_start(ap, fmt);
	ret = vasprintf(&p, fmt, ap);
	va_end(ap);

	return ret == -1 ? NULL : p;
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

static inline void *memmap(int fd, int flags)
{
	return (void *)-1;
}

static inline int ctrlc (void)
{
	return 0;
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

static inline struct param_d *dev_add_param_string(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		char **value, void *priv)
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

static inline int of_register_fixup(int (*fixup)(struct device_node *, void *),
				    void *context)
{
	return 0;
}

static inline int of_unregister_fixup(int (*fixup)(struct device_node *, void *), void *context)
{
	return 0;
}

#define __define_initcall(level,fn,id) \
static void __attribute__ ((constructor)) __initcall_##id##_##fn() { \
	fn(); \
}

#define core_initcall(fn)		__define_initcall("1",fn,1)
#define postcore_initcall(fn)		__define_initcall("2",fn,2)
#define console_initcall(fn)		__define_initcall("3",fn,3)
#define postconsole_initcall(fn)	__define_initcall("4",fn,4)
#define mem_initcall(fn)		__define_initcall("5",fn,5)
#define mmu_initcall(fn)		__define_initcall("6",fn,6)
#define postmmu_initcall(fn)		__define_initcall("7",fn,7)
#define coredevice_initcall(fn)		__define_initcall("8",fn,8)
#define fs_initcall(fn)			__define_initcall("9",fn,9)
#define device_initcall(fn)		__define_initcall("10",fn,10)
#define crypto_initcall(fn)		__define_initcall("11",fn,11)
#define late_initcall(fn)		__define_initcall("12",fn,12)
#define environment_initcall(fn)	__define_initcall("13",fn,13)
#define postenvironment_initcall(fn)	__define_initcall("14",fn,14)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define cpu_to_be32 __cpu_to_be32
#define be32_to_cpu __be32_to_cpu

#define cpu_to_be64 __cpu_to_be64
#define be64_to_cpu __be64_to_cpu

#define ALIGN(x, a)		__ALIGN_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))

#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))

#define __maybe_unused			__attribute__((unused))

static inline u16 __get_unaligned_be16(const u8 *p)
{
	return p[0] << 8 | p[1];
}

static inline u32 __get_unaligned_be32(const u8 *p)
{
	return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

static inline u64 __get_unaligned_be64(const u8 *p)
{
	return (u64)__get_unaligned_be32(p) << 32 |
	       __get_unaligned_be32(p + 4);
}

static inline void __put_unaligned_be16(u16 val, u8 *p)
{
	*p++ = val >> 8;
	*p++ = val;
}

static inline void __put_unaligned_be32(u32 val, u8 *p)
{
	__put_unaligned_be16(val >> 16, p);
	__put_unaligned_be16(val, p + 2);
}

static inline void __put_unaligned_be64(u64 val, u8 *p)
{
	__put_unaligned_be32(val >> 32, p);
	__put_unaligned_be32(val, p + 4);
}

static inline u16 get_unaligned_be16(const void *p)
{
	return __get_unaligned_be16((const u8 *)p);
}

static inline u32 get_unaligned_be32(const void *p)
{
	return __get_unaligned_be32((const u8 *)p);
}

static inline u64 get_unaligned_be64(const void *p)
{
	return __get_unaligned_be64((const u8 *)p);
}

static inline void put_unaligned_be16(u16 val, void *p)
{
	__put_unaligned_be16(val, p);
}

static inline void put_unaligned_be32(u32 val, void *p)
{
	__put_unaligned_be32(val, p);
}

static inline void put_unaligned_be64(u64 val, void *p)
{
	__put_unaligned_be64(val, p);
}

/**
 * rol32 - rotate a 32-bit value left
 * @word: value to rotate
 * @shift: bits to roll
 */
static inline __u32 rol32(__u32 word, unsigned int shift)
{
	return (word << shift) | (word >> (32 - shift));
}

/**
 * ror32 - rotate a 32-bit value right
 * @word: value to rotate
 * @shift: bits to roll
 */
static inline __u32 ror32(__u32 word, unsigned int shift)
{
	return (word >> shift) | (word << (32 - shift));
}

#define min(x, y) ({				\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })

/*
 * Helper macros to use CONFIG_ options in C expressions. Note that
 * these only work with boolean and tristate options.
 */

/*
 * Getting something that works in C and CPP for an arg that may or may
 * not be defined is tricky.  Here, if we have "#define CONFIG_BOOGER 1"
 * we match on the placeholder define, insert the "0," for arg1 and generate
 * the triplet (0, 1, 0).  Then the last step cherry picks the 2nd arg (a one).
 * When CONFIG_BOOGER is not defined, we generate a (... 1, 0) pair, and when
 * the last step cherry picks the 2nd arg, we get a zero.
 */
#define __ARG_PLACEHOLDER_1 0,
#define config_enabled(cfg) _config_enabled(cfg)
#define _config_enabled(value) __config_enabled(__ARG_PLACEHOLDER_##value)
#define __config_enabled(arg1_or_junk) ___config_enabled(arg1_or_junk 1, 0)
#define ___config_enabled(__ignored, val, ...) val

/*
 * IS_ENABLED(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'y' or 'm',
 * 0 otherwise.
 *
 */
#define IS_ENABLED(option) \
	(config_enabled(option) || config_enabled(option##_MODULE))

/*
 * IS_BUILTIN(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'y', 0
 * otherwise. For boolean options, this is equivalent to
 * IS_ENABLED(CONFIG_FOO).
 */
#define IS_BUILTIN(option) config_enabled(option)

/*
 * IS_MODULE(CONFIG_FOO) evaluates to 1 if CONFIG_FOO is set to 'm', 0
 * otherwise.
 */
#define IS_MODULE(option) config_enabled(option##_MODULE)

#endif

uint32_t crc32(uint32_t crc, const void *_buf, unsigned int len);
uint32_t crc32_no_comp(uint32_t crc, const void *_buf, unsigned int len);

#endif /* __DT_COMMON_H */
