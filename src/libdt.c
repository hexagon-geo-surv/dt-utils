/*
 * of.c - basic devicetree functions
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * based on Linux devicetree support
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <stdio.h>
#include <linux/types.h>
#include <asm/byteorder.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libudev.h>
#include <dt.h>

static int is_printable_string(const void *data, int len)
{
	const char *s = data;

	/* zero length is not */
	if (len == 0)
		return 0;

	/* must terminate with zero */
	if (s[len - 1] != '\0')
		return 0;

	/* printable or a null byte (concatenated strings) */
	while (((*s == '\0') || isprint(*s)) && (len > 0)) {
		/*
		 * If we see a null, there are three possibilities:
		 * 1) If len == 1, it is the end of the string, printable
		 * 2) Next character also a null, not printable.
		 * 3) Next character not a null, continue to check.
		 */
		if (s[0] == '\0') {
			if (len == 1)
				return 1;
			if (s[1] == '\0')
				return 0;
		}
		s++;
		len--;
	}

	/* Not the null termination, or not done yet: not printable */
	if (*s != '\0' || (len != 0))
		return 0;

	return 1;
}

/*
 * Print the property in the best format, a heuristic guess.  Print as
 * a string, concatenated strings, a byte, word, double word, or (if all
 * else fails) it is printed as a stream of bytes.
 */
void of_print_property(const void *data, int len)
{
	int j;

	/* no data, don't print */
	if (len == 0)
		return;

	/*
	 * It is a string, but it may have multiple strings (embedded '\0's).
	 */
	if (is_printable_string(data, len)) {
		printf("\"");
		j = 0;
		while (j < len) {
			if (j > 0)
				printf("\", \"");
			printf(data);
			j    += strlen(data) + 1;
			data += strlen(data) + 1;
		}
		printf("\"");
		return;
	}

	if ((len % 4) == 0) {
		const uint32_t *p;

		printf("<");
		for (j = 0, p = data; j < len/4; j ++)
			printf("0x%x%s", __be32_to_cpu(p[j]), j < (len/4 - 1) ? " " : "");
		printf(">");
	} else { /* anything else... hexdump */
		const uint8_t *s;

		printf("[");
		for (j = 0, s = data; j < len; j++)
			printf("%02x%s", s[j], j < len - 1 ? " " : "");
		printf("]");
	}
}

/*
 * Iterate over all nodes of a tree. As a devicetree does not
 * have a dedicated list head, the start node (usually the root
 * node) will not be iterated over.
 */
static inline struct device_node *of_next_node(struct device_node *node)
{
	struct device_node *next;

	next = list_first_entry(&node->list, struct device_node, list);

	return next->parent ? next : NULL;
}

#define of_tree_for_each_node_from(node, from) \
	for (node = of_next_node(from); node; node = of_next_node(node))

/**
 * struct alias_prop - Alias property in 'aliases' node
 * @link:	List node to link the structure in aliases_lookup list
 * @alias:	Alias property name
 * @np:		Pointer to device_node that the alias stands for
 * @id:		Index value from end of alias name
 * @stem:	Alias string without the index
 *
 * The structure represents one alias property of 'aliases' node as
 * an entry in aliases_lookup list.
 */
struct alias_prop {
	struct list_head link;
	const char *alias;
	struct device_node *np;
	int id;
	char stem[0];
};

static LIST_HEAD(aliases_lookup);

struct device_node *root_node;

struct device_node *of_aliases;

#define OF_ROOT_NODE_SIZE_CELLS_DEFAULT 1
#define OF_ROOT_NODE_ADDR_CELLS_DEFAULT 1

int of_n_addr_cells(struct device_node *np)
{
	const __be32 *ip;

	do {
		if (np->parent)
			np = np->parent;
		ip = of_get_property(np, "#address-cells", NULL);
		if (ip)
			return __be32_to_cpup(ip);
	} while (np->parent);
	/* No #address-cells property for the root node */
	return OF_ROOT_NODE_ADDR_CELLS_DEFAULT;
}

int of_n_size_cells(struct device_node *np)
{
	const __be32 *ip;

	do {
		if (np->parent)
			np = np->parent;
		ip = of_get_property(np, "#size-cells", NULL);
		if (ip)
			return __be32_to_cpup(ip);
	} while (np->parent);
	/* No #size-cells property for the root node */
	return OF_ROOT_NODE_SIZE_CELLS_DEFAULT;
}

struct property *of_find_property(const struct device_node *np,
				  const char *name, int *lenp)
{
	struct property *pp;

	if (!np)
		return NULL;

	list_for_each_entry(pp, &np->properties, list)
		if (of_prop_cmp(pp->name, name) == 0) {
			if (lenp)
				*lenp = pp->length;
			return pp;
		}

	return NULL;
}

static void of_alias_add(struct alias_prop *ap, struct device_node *np,
			 int id, const char *stem, int stem_len)
{
	ap->np = np;
	ap->id = id;
	strncpy(ap->stem, stem, stem_len);
	ap->stem[stem_len] = 0;
	list_add_tail(&ap->link, &aliases_lookup);
	pr_debug("adding DT alias:%s: stem=%s id=%i node=%s\n",
		 ap->alias, ap->stem, ap->id, np->full_name);
}

/**
 * of_alias_scan - Scan all properties of 'aliases' node
 *
 * The function scans all the properties of 'aliases' node and populates
 * the global lookup table with the properties.  It returns the
 * number of alias_prop found, or error code in error case.
 */
void of_alias_scan(void)
{
	struct property *pp;
	struct alias_prop *app, *tmp;

	list_for_each_entry_safe(app, tmp, &aliases_lookup, link)
		free(app);

	INIT_LIST_HEAD(&aliases_lookup);

	if (!root_node)
		return;

	of_aliases = of_find_node_by_path("/aliases");
	if (!of_aliases)
		return;

	list_for_each_entry(pp, &of_aliases->properties, list) {
		const char *start = pp->name;
		const char *end = start + strlen(start);
		struct device_node *np;
		struct alias_prop *ap;
		int id, len;

		/* Skip those we do not want to proceed */
		if (!of_prop_cmp(pp->name, "name") ||
		    !of_prop_cmp(pp->name, "phandle") ||
		    !of_prop_cmp(pp->name, "linux,phandle"))
			continue;

		np = of_find_node_by_path(pp->value);
		if (!np)
			continue;

		/* walk the alias backwards to extract the id and work out
		 * the 'stem' string */
		while (isdigit(*(end-1)) && end > start)
			end--;
		len = end - start;

		id = strtol(end, 0, 10);
		if (id < 0)
			continue;

		/* Allocate an alias_prop with enough space for the stem */
		ap = xzalloc(sizeof(*ap) + len + 1);
		if (!ap)
			continue;
		ap->alias = start;
		of_alias_add(ap, np, id, start, len);
	}
}

/**
 * of_alias_get_id - Get alias id for the given device_node
 * @np:		Pointer to the given device_node
 * @stem:	Alias stem of the given device_node
 *
 * The function travels the lookup table to get alias id for the given
 * device_node and alias stem.  It returns the alias id if find it.
 */
int of_alias_get_id(struct device_node *np, const char *stem)
{
	struct alias_prop *app;
	int id = -ENODEV;

	list_for_each_entry(app, &aliases_lookup, link) {
		if (of_node_cmp(app->stem, stem) != 0)
			continue;

		if (np == app->np) {
			id = app->id;
			break;
		}
	}

	return id;
}

const char *of_alias_get(struct device_node *np)
{
	struct property *pp;

	list_for_each_entry(pp, &of_aliases->properties, list) {
		if (!of_node_cmp(np->full_name, pp->value))
			return pp->name;
	}

	return NULL;
}

/*
 * of_find_node_by_alias - Find a node given an alias name
 * @root:    the root node of the tree. If NULL, use internal tree
 * @alias:   the alias name to find
 */
struct device_node *of_find_node_by_alias(struct device_node *root, const char *alias)
{
	struct device_node *aliasnp;
	int ret;
	const char *path;

	if (!root)
		root = root_node;

	aliasnp = of_find_node_by_path_from(root, "/aliases");
	if (!aliasnp)
		return NULL;

	ret = of_property_read_string(aliasnp, alias, &path);
	if (ret)
		return NULL;

	return of_find_node_by_path_from(root, path);
}

/*
 * of_find_node_by_phandle - Find a node given a phandle
 * @handle:    phandle of the node to find
 */
struct device_node *of_find_node_by_phandle(phandle phandle)
{
	struct device_node *node;

	of_tree_for_each_node_from(node, root_node)
		if (node->phandle == phandle)
			return node;

	return NULL;
}

/*
 * of_get_tree_max_phandle - Find the maximum phandle of a tree
 * @root:    root node of the tree to search in. If NULL use the
 *           internal tree.
 */
phandle of_get_tree_max_phandle(struct device_node *root)
{
	struct device_node *n;
	phandle max;

	if (!root)
		root = root_node;

	if (!root)
		return 0;

	max = root->phandle;

	of_tree_for_each_node_from(n, root) {
		if (n->phandle > max)
			max = n->phandle;
	}

	return max;
}

/*
 * of_node_create_phandle - create a phandle for a node
 * @node:    The node to create a phandle in
 *
 * returns the new phandle or the existing phandle if the node
 * already has a phandle.
 */
phandle of_node_create_phandle(struct device_node *node)
{
	phandle p;
	struct device_node *root;

	if (node->phandle)
		return node->phandle;

	root = of_find_root_node(node);

	p = of_get_tree_max_phandle(root) + 1;

	node->phandle = p;

	p = __cpu_to_be32(p);

	of_set_property(node, "phandle", &p, sizeof(p), 1);

	return node->phandle;
}

/*
 * Find a property with a given name for a given node
 * and return the value.
 */
const void *of_get_property(const struct device_node *np, const char *name,
			 int *lenp)
{
	struct property *pp = of_find_property(np, name, lenp);

	return pp ? pp->value : NULL;
}

/** Checks if the given "compat" string matches one of the strings in
 * the device's "compatible" property
 */
int of_device_is_compatible(const struct device_node *device,
		const char *compat)
{
	const char *cp;
	int cplen, l;

	cp = of_get_property(device, "compatible", &cplen);
	if (cp == NULL)
		return 0;
	while (cplen > 0) {
		if (of_compat_cmp(cp, compat, strlen(compat)) == 0)
			return 1;
		l = strlen(cp) + 1;
		cp += l;
		cplen -= l;
	}

	return 0;
}

/**
 *	of_find_node_by_name - Find a node by its "name" property
 *	@from:	The node to start searching from or NULL, the node
 *		you pass will not be searched, only the next one
 *		will; typically, you pass what the previous call
 *		returned.
 *	@name:	The name string to match against
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_by_name(struct device_node *from,
	const char *name)
{
	struct device_node *np;

	if (!from)
		from = root_node;

	of_tree_for_each_node_from(np, from)
		if (np->name && !of_node_cmp(np->name, name))
			return np;

	return NULL;
}

/**
 *	of_find_node_by_type - Find a node by its "device_type" property
 *	@from:  The node to start searching from, or NULL to start searching
 *		the entire device tree. The node you pass will not be
 *		searched, only the next one will; typically, you pass
 *		what the previous call returned.
 *	@type:  The type string to match against.
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_by_type(struct device_node *from,
		const char *type)
{
	struct device_node *np;
	const char *device_type;
	int ret;

	if (!from)
		from = root_node;

	of_tree_for_each_node_from(np, from) {
		ret = of_property_read_string(np, "device_type", &device_type);
		if (!ret && !of_node_cmp(device_type, type))
			return np;
	}
	return NULL;
}

/**
 *	of_find_compatible_node - Find a node based on type and one of the
 *                                tokens in its "compatible" property
 *	@from:		The node to start searching from or NULL, the node
 *			you pass will not be searched, only the next one
 *			will; typically, you pass what the previous call
 *			returned.
 *	@type:		The type string to match "device_type" or NULL to ignore
 *                      (currently always ignored in barebox)
 *	@compatible:	The string to match to one of the tokens in the device
 *			"compatible" list.
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_compatible_node(struct device_node *from,
	const char *type, const char *compatible)
{
	struct device_node *np;

	if (!from)
		from = root_node;

	of_tree_for_each_node_from(np, from)
		if (of_device_is_compatible(np, compatible))
			return np;

	return NULL;
}

/**
 *	of_find_node_with_property - Find a node which has a property with
 *                                   the given name.
 *	@from:		The node to start searching from or NULL, the node
 *			you pass will not be searched, only the next one
 *			will; typically, you pass what the previous call
 *			returned.
 *	@prop_name:	The name of the property to look for.
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_with_property(struct device_node *from,
	const char *prop_name)
{
	struct device_node *np;

	if (!from)
		from = root_node;

	of_tree_for_each_node_from(np, from) {
		struct property *pp = of_find_property(np, prop_name, NULL);
		if (pp)
			return np;
	}

	return NULL;
}

/**
 * of_match_node - Tell if an device_node has a matching of_match structure
 *      @matches:       array of of device match structures to search in
 *      @node:          the of device structure to match against
 *
 *      Low level utility function used by device matching.
 */
const struct of_device_id *of_match_node(const struct of_device_id *matches,
					 const struct device_node *node)
{
	if (!matches || !node)
		return NULL;

	while (matches->compatible) {
		if (of_device_is_compatible(node, matches->compatible) == 1)
			return matches;
		matches++;
	}

	return NULL;
}

/**
 *	of_find_matching_node_and_match - Find a node based on an of_device_id
 *					  match table.
 *	@from:		The node to start searching from or NULL, the node
 *			you pass will not be searched, only the next one
 *			will; typically, you pass what the previous call
 *			returned.
 *	@matches:	array of of device match structures to search in
 *	@match		Updated to point at the matches entry which matched
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_matching_node_and_match(struct device_node *from,
					const struct of_device_id *matches,
					const struct of_device_id **match)
{
	struct device_node *np;

	if (match)
		*match = NULL;

	if (!from)
		from = root_node;

	of_tree_for_each_node_from(np, from) {
		const struct of_device_id *m = of_match_node(matches, np);
		if (m) {
			if (match)
				*match = m;
			return np;
		}
	}

	return NULL;
}

/**
 * of_find_property_value_of_size
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @len:	requested length of property value
 *
 * Search for a property in a device node and valid the requested size.
 * Returns the property value on success, -EINVAL if the property does not
 *  exist, -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 */
static void *of_find_property_value_of_size(const struct device_node *np,
			const char *propname, uint32_t len)
{
	struct property *prop = of_find_property(np, propname, NULL);

	if (!prop)
		return ERR_PTR(-EINVAL);
	if (!prop->value)
		return ERR_PTR(-ENODATA);
	if (len > prop->length)
		return ERR_PTR(-EOVERFLOW);

	return prop->value;
}

/**
 * of_property_read_u32_index - Find and read a uint32_t from a multi-value property.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @index:	index of the uint32_t in the list of values
 * @out_value:	pointer to return value, modified only if no error.
 *
 * Search for a property in a device node and read nth 32-bit value from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * The out_value is modified only if a valid uint32_t value can be decoded.
 */
int of_property_read_u32_index(const struct device_node *np,
				       const char *propname,
				       uint32_t index, uint32_t *out_value)
{
	const uint32_t *val = of_find_property_value_of_size(np, propname,
					((index + 1) * sizeof(*out_value)));

	if (IS_ERR(val))
		return PTR_ERR(val);

	*out_value = __be32_to_cpup(((__be32 *)val) + index);
	return 0;
}

/**
 * of_property_read_u8_array - Find and read an array of uint8_t from a property.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_value:	pointer to return value, modified only if return value is 0.
 * @sz:		number of array elements to read
 *
 * Search for a property in a device node and read 8-bit value(s) from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * dts entry of array should be like:
 *	property = /bits/ 8 <0x50 0x60 0x70>;
 *
 * The out_value is modified only if a valid uint8_t value can be decoded.
 */
int of_property_read_u8_array(const struct device_node *np,
			const char *propname, uint8_t *out_values, size_t sz)
{
	const uint8_t *val = of_find_property_value_of_size(np, propname,
						(sz * sizeof(*out_values)));

	if (IS_ERR(val))
		return PTR_ERR(val);

	while (sz--)
		*out_values++ = *val++;
	return 0;
}

/**
 * of_property_read_u16_array - Find and read an array of uint16_t from a property.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_value:	pointer to return value, modified only if return value is 0.
 * @sz:		number of array elements to read
 *
 * Search for a property in a device node and read 16-bit value(s) from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * dts entry of array should be like:
 *	property = /bits/ 16 <0x5000 0x6000 0x7000>;
 *
 * The out_value is modified only if a valid uint16_t value can be decoded.
 */
int of_property_read_u16_array(const struct device_node *np,
			const char *propname, uint16_t *out_values, size_t sz)
{
	const __be16 *val = of_find_property_value_of_size(np, propname,
						(sz * sizeof(*out_values)));

	if (IS_ERR(val))
		return PTR_ERR(val);

	while (sz--)
		*out_values++ = __be16_to_cpup(val++);
	return 0;
}

/**
 * of_property_read_u32_array - Find and read an array of 32 bit integers
 * from a property.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_value:	pointer to return value, modified only if return value is 0.
 * @sz:		number of array elements to read
 *
 * Search for a property in a device node and read 32-bit value(s) from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * The out_value is modified only if a valid uint32_t value can be decoded.
 */
int of_property_read_u32_array(const struct device_node *np,
			       const char *propname, uint32_t *out_values,
			       size_t sz)
{
	const __be32 *val = of_find_property_value_of_size(np, propname,
						(sz * sizeof(*out_values)));

	if (IS_ERR(val))
		return PTR_ERR(val);

	while (sz--)
		*out_values++ = __be32_to_cpup(val++);
	return 0;
}

/**
 * of_property_read_u64 - Find and read a 64 bit integer from a property
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_value:	pointer to return value, modified only if return value is 0.
 *
 * Search for a property in a device node and read a 64-bit value from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * The out_value is modified only if a valid uint64_t value can be decoded.
 */
int of_property_read_u64(const struct device_node *np, const char *propname,
			 uint64_t *out_value)
{
	const __be32 *val = of_find_property_value_of_size(np, propname,
						sizeof(*out_value));

	if (IS_ERR(val))
		return PTR_ERR(val);

	*out_value = of_read_number(val, 2);
	return 0;
}

/**
 * of_property_read_string - Find and read a string from a property
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_string:	pointer to null terminated return string, modified only if
 *		return value is 0.
 *
 * Search for a property in a device tree node and retrieve a null
 * terminated string value (pointer to data, not a copy). Returns 0 on
 * success, -EINVAL if the property does not exist, -ENODATA if property
 * does not have a value, and -EILSEQ if the string is not null-terminated
 * within the length of the property data.
 *
 * The out_string pointer is modified only if a valid string can be decoded.
 */
int of_property_read_string(struct device_node *np, const char *propname,
				const char **out_string)
{
	struct property *prop = of_find_property(np, propname, NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;
	if (strnlen(prop->value, prop->length) >= prop->length)
		return -EILSEQ;
	*out_string = prop->value;
	return 0;
}

/**
 * of_property_read_string_index - Find and read a string from a multiple
 * strings property.
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @index:	index of the string in the list of strings
 * @out_string:	pointer to null terminated return string, modified only if
 *		return value is 0.
 *
 * Search for a property in a device tree node and retrieve a null
 * terminated string value (pointer to data, not a copy) in the list of strings
 * contained in that property.
 * Returns 0 on success, -EINVAL if the property does not exist, -ENODATA if
 * property does not have a value, and -EILSEQ if the string is not
 * null-terminated within the length of the property data.
 *
 * The out_string pointer is modified only if a valid string can be decoded.
 */
int of_property_read_string_index(struct device_node *np, const char *propname,
				  int index, const char **output)
{
	struct property *prop = of_find_property(np, propname, NULL);
	int i = 0;
	size_t l = 0, total = 0;
	const char *p;

	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;
	if (strnlen(prop->value, prop->length) >= prop->length)
		return -EILSEQ;

	p = prop->value;

	for (i = 0; total < prop->length; total += l, p += l) {
		l = strlen(p) + 1;
		if (i++ == index) {
			*output = p;
			return 0;
		}
	}
	return -ENODATA;
}

/**
 * of_property_match_string() - Find string in a list and return index
 * @np: pointer to node containing string list property
 * @propname: string list property name
 * @string: pointer to string to search for in string list
 *
 * This function searches a string list property and returns the index
 * of a specific string value.
 */
int of_property_match_string(struct device_node *np, const char *propname,
			     const char *string)
{
	struct property *prop = of_find_property(np, propname, NULL);
	size_t l;
	int i;
	const char *p, *end;

	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;

	p = prop->value;
	end = p + prop->length;

	for (i = 0; p < end; i++, p += l) {
		l = strlen(p) + 1;
		if (p + l > end)
			return -EILSEQ;
		pr_debug("comparing %s with %s\n", string, p);
		if (strcmp(string, p) == 0)
			return i; /* Found it; return index */
	}
	return -ENODATA;
}

/**
 * of_property_count_strings - Find and return the number of strings from a
 * multiple strings property.
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 *
 * Search for a property in a device tree node and retrieve the number of null
 * terminated string contain in it. Returns the number of strings on
 * success, -EINVAL if the property does not exist, -ENODATA if property
 * does not have a value, and -EILSEQ if the string is not null-terminated
 * within the length of the property data.
 */
int of_property_count_strings(struct device_node *np, const char *propname)
{
	struct property *prop = of_find_property(np, propname, NULL);
	int i = 0;
	size_t l = 0, total = 0;
	const char *p;

	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;
	if (strnlen(prop->value, prop->length) >= prop->length)
		return -EILSEQ;

	p = prop->value;

	for (i = 0; total < prop->length; total += l, p += l, i++)
		l = strlen(p) + 1;

	return i;
}

const __be32 *of_prop_next_u32(struct property *prop, const __be32 *cur,
			uint32_t *pu)
{
	const void *curv = cur;

	if (!prop)
		return NULL;

	if (!cur) {
		curv = prop->value;
		goto out_val;
	}

	curv += sizeof(*cur);
	if (curv >= prop->value + prop->length)
		return NULL;

out_val:
	*pu = __be32_to_cpup(curv);
	return curv;
}

const char *of_prop_next_string(struct property *prop, const char *cur)
{
	const void *curv = cur;

	if (!prop)
		return NULL;

	if (!cur)
		return prop->value;

	curv += strlen(cur) + 1;
	if (curv >= prop->value + prop->length)
		return NULL;

	return curv;
}

/**
 * of_property_write_bool - Create/Delete empty (bool) property.
 *
 * @np:		device node from which the property is to be set.
 * @propname:	name of the property to be set.
 *
 * Search for a property in a device node and create or delete the property.
 * If the property already exists and write value is false, the property is
 * deleted. If write value is true and the property does not exist, it is
 * created. Returns 0 on success, -ENOMEM if the property or array
 * of elements cannot be created.
 */
int of_property_write_bool(struct device_node *np, const char *propname,
			   const bool value)
{
	struct property *prop = of_find_property(np, propname, NULL);

	if (!value) {
		if (prop)
			of_delete_property(prop);
		return 0;
	}

	if (!prop)
		prop = of_new_property(np, propname, NULL, 0);
	if (!prop)
		return -ENOMEM;

	return 0;
}

/**
 * of_property_write_u8_array - Write an array of uint8_t to a property. If
 * the property does not exist, it will be created and appended to the given
 * device node.
 *
 * @np:		device node to which the property value is to be written.
 * @propname:	name of the property to be written.
 * @values:	pointer to array elements to write.
 * @sz:		number of array elements to write.
 *
 * Search for a property in a device node and write 8-bit value(s) to
 * it. If the property does not exist, it will be created and appended to
 * the device node. Returns 0 on success, -ENOMEM if the property or array
 * of elements cannot be created.
 */
int of_property_write_u8_array(struct device_node *np,
			       const char *propname, const uint8_t *values,
			       size_t sz)
{
	struct property *prop = of_find_property(np, propname, NULL);
	uint8_t *val;

	if (prop)
		of_delete_property(prop);

	prop = of_new_property(np, propname, NULL, sizeof(*val) * sz);
	if (!prop)
		return -ENOMEM;

	val = prop->value;
	while (sz--)
		*val++ = *values++;

	return 0;
}

/**
 * of_property_write_u16_array - Write an array of uint16_t to a property. If
 * the property does not exist, it will be created and appended to the given
 * device node.
 *
 * @np:		device node to which the property value is to be written.
 * @propname:	name of the property to be written.
 * @values:	pointer to array elements to write.
 * @sz:		number of array elements to write.
 *
 * Search for a property in a device node and write 16-bit value(s) to
 * it. If the property does not exist, it will be created and appended to
 * the device node. Returns 0 on success, -ENOMEM if the property or array
 * of elements cannot be created.
 */
int of_property_write_u16_array(struct device_node *np,
				const char *propname, const uint16_t *values,
				size_t sz)
{
	struct property *prop = of_find_property(np, propname, NULL);
	__be16 *val;

	if (prop)
		of_delete_property(prop);

	prop = of_new_property(np, propname, NULL, sizeof(*val) * sz);
	if (!prop)
		return -ENOMEM;

	val = prop->value;
	while (sz--)
		*val++ = __cpu_to_be16(*values++);

	return 0;
}

/**
 * of_property_write_u32_array - Write an array of uint32_t to a property. If
 * the property does not exist, it will be created and appended to the given
 * device node.
 *
 * @np:		device node to which the property value is to be written.
 * @propname:	name of the property to be written.
 * @values:	pointer to array elements to write.
 * @sz:		number of array elements to write.
 *
 * Search for a property in a device node and write 32-bit value(s) to
 * it. If the property does not exist, it will be created and appended to
 * the device node. Returns 0 on success, -ENOMEM if the property or array
 * of elements cannot be created.
 */
int of_property_write_u32_array(struct device_node *np,
				const char *propname, const uint32_t *values,
				size_t sz)
{
	struct property *prop = of_find_property(np, propname, NULL);
	__be32 *val;

	if (prop)
		of_delete_property(prop);

	prop = of_new_property(np, propname, NULL, sizeof(*val) * sz);
	if (!prop)
		return -ENOMEM;

	val = prop->value;
	while (sz--)
		*val++ = __cpu_to_be32(*values++);

	return 0;
}

/**
 * of_property_write_u64_array - Write an array of uint64_t to a property. If
 * the property does not exist, it will be created and appended to the given
 * device node.
 *
 * @np:		device node to which the property value is to be written.
 * @propname:	name of the property to be written.
 * @values:	pointer to array elements to write.
 * @sz:		number of array elements to write.
 *
 * Search for a property in a device node and write 64-bit value(s) to
 * it. If the property does not exist, it will be created and appended to
 * the device node. Returns 0 on success, -ENOMEM if the property or array
 * of elements cannot be created.
 */
int of_property_write_u64_array(struct device_node *np,
				const char *propname, const uint64_t *values,
				size_t sz)
{
	struct property *prop = of_find_property(np, propname, NULL);
	__be32 *val;

	if (prop)
		of_delete_property(prop);

	prop = of_new_property(np, propname, NULL, 2 * sizeof(*val) * sz);
	if (!prop)
		return -ENOMEM;

	val = prop->value;
	while (sz--) {
		of_write_number(val, *values++, 2);
		val += 2;
	}

	return 0;
}

/**
 * of_parse_phandle - Resolve a phandle property to a device_node pointer
 * @np: Pointer to device node holding phandle property
 * @phandle_name: Name of property holding a phandle value
 * @index: For properties holding a table of phandles, this is the index into
 *         the table
 *
 * Returns the device_node pointer found or NULL.
 */
struct device_node *of_parse_phandle(const struct device_node *np,
				     const char *phandle_name, int index)
{
	const __be32 *phandle;
	int size;

	phandle = of_get_property(np, phandle_name, &size);
	if ((!phandle) || (size < sizeof(*phandle) * (index + 1)))
		return NULL;

	return of_find_node_by_phandle(__be32_to_cpup(phandle + index));
}

/**
 * of_parse_phandle_with_args() - Find a node pointed by phandle in a list
 * @np:		pointer to a device tree node containing a list
 * @list_name:	property name that contains a list
 * @cells_name:	property name that specifies phandles' arguments count
 * @index:	index of a phandle to parse out
 * @out_args:	optional pointer to output arguments structure (will be filled)
 *
 * This function is useful to parse lists of phandles and their arguments.
 * Returns 0 on success and fills out_args, on error returns appropriate
 * errno value.
 *
 * Example:
 *
 * phandle1: node1 {
 * 	#list-cells = <2>;
 * }
 *
 * phandle2: node2 {
 * 	#list-cells = <1>;
 * }
 *
 * node3 {
 * 	list = <&phandle1 1 2 &phandle2 3>;
 * }
 *
 * To get a device_node of the `node2' node you may call this:
 * of_parse_phandle_with_args(node3, "list", "#list-cells", 1, &args);
 */
static int __of_parse_phandle_with_args(const struct device_node *np,
					const char *list_name,
					const char *cells_name, int index,
					struct of_phandle_args *out_args)
{
	const __be32 *list, *list_end;
	int rc = 0, size, cur_index = 0;
	uint32_t count = 0;
	struct device_node *node = NULL;
	phandle phandle;

	/* Retrieve the phandle list property */
	list = of_get_property(np, list_name, &size);
	if (!list)
		return -ENOENT;
	list_end = list + size / sizeof(*list);

	/* Loop over the phandles until all the requested entry is found */
	while (list < list_end) {
		rc = -EINVAL;
		count = 0;

		/*
		 * If phandle is 0, then it is an empty entry with no
		 * arguments.  Skip forward to the next entry.
		 */
		phandle = __be32_to_cpup(list++);
		if (phandle) {
			/*
			 * Find the provider node and parse the #*-cells
			 * property to determine the argument length
			 */
			node = of_find_node_by_phandle(phandle);
			if (!node) {
				pr_err("%s: could not find phandle\n",
					 np->full_name);
				goto err;
			}
			if (of_property_read_u32(node, cells_name, &count)) {
				pr_err("%s: could not get %s for %s\n",
					 np->full_name, cells_name,
					 node->full_name);
				goto err;
			}

			/*
			 * Make sure that the arguments actually fit in the
			 * remaining property data length
			 */
			if (list + count > list_end) {
				pr_err("%s: arguments longer than property\n",
					 np->full_name);
				goto err;
			}
		}

		/*
		 * All of the error cases above bail out of the loop, so at
		 * this point, the parsing is successful. If the requested
		 * index matches, then fill the out_args structure and return,
		 * or return -ENOENT for an empty entry.
		 */
		rc = -ENOENT;
		if (cur_index == index) {
			if (!phandle)
				goto err;

			if (out_args) {
				int i;
				if (count > MAX_PHANDLE_ARGS)
					count = MAX_PHANDLE_ARGS;
				out_args->np = node;
				out_args->args_count = count;
				for (i = 0; i < count; i++)
					out_args->args[i] =
						__be32_to_cpup(list++);
			}

			/* Found it! return success */
			return 0;
		}

		node = NULL;
		list += count;
		cur_index++;
	}

	/*
	 * Unlock node before returning result; will be one of:
	 * -ENOENT : index is for empty phandle
	 * -EINVAL : parsing error on data
	 * [1..n]  : Number of phandle (count mode; when index = -1)
	 */
	rc = index < 0 ? cur_index : -ENOENT;
 err:
	return rc;
}

int of_parse_phandle_with_args(const struct device_node *np,
		const char *list_name, const char *cells_name, int index,
		struct of_phandle_args *out_args)
{
	if (index < 0)
		return -EINVAL;
	return __of_parse_phandle_with_args(np, list_name, cells_name,
					index, out_args);
}

/**
 * of_count_phandle_with_args() - Find the number of phandles references in a property
 * @np:		pointer to a device tree node containing a list
 * @list_name:	property name that contains a list
 * @cells_name:	property name that specifies phandles' arguments count
 *
 * Returns the number of phandle + argument tuples within a property. It
 * is a typical pattern to encode a list of phandle and variable
 * arguments into a single property. The number of arguments is encoded
 * by a property in the phandle-target node. For example, a gpios
 * property would contain a list of GPIO specifies consisting of a
 * phandle and 1 or more arguments. The number of arguments are
 * determined by the #gpio-cells property in the node pointed to by the
 * phandle.
 */
int of_count_phandle_with_args(const struct device_node *np,
			const char *list_name, const char *cells_name)
{
	return __of_parse_phandle_with_args(np, list_name, cells_name,
					-1, NULL);
}

/**
 * of_machine_is_compatible - Test root of device tree for a given compatible value
 * @compat: compatible string to look for in root node's compatible property.
 *
 * Returns true if the root node has the given value in its
 * compatible property.
 */
int of_machine_is_compatible(const char *compat)
{
	if (!root_node)
		return 0;

	return of_device_is_compatible(root_node, compat);
}

/**
 *	of_find_node_by_path_from - Find a node matching a full OF path
 *      relative to a given root node.
 *	@path:	The full path to match
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_by_path_from(struct device_node *from,
					const char *path)
{
	char *slash, *p, *freep;

	if (!from)
		from = root_node;

	if (!from || !path || *path != '/')
		return NULL;

	path++;

	freep = p = strdup(path);

	while (1) {
		if (!*p)
			goto out;

		slash = strchr(p, '/');
		if (slash)
			*slash = 0;

		from = of_get_child_by_name(from, p);
		if (!from)
			goto out;

		if (!slash)
			goto out;

		p = slash + 1;
	}
out:
	free(freep);

	return from;
}

/**
 *	of_find_node_by_path - Find a node matching a full OF path
 *	@path:	The full path to match
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_by_path(const char *path)
{
	return of_find_node_by_path_from(root_node, path);
}

/**
 *	of_find_node_by_path_or_alias - Find a node matching a full OF path
 *	or an alias
 *	@root:	The root node. If NULL the internal tree is used
 *	@str:	the full path or alias
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_by_path_or_alias(struct device_node *root,
		const char *str)
{
	if (*str ==  '/')
		return of_find_node_by_path_from(root, str);
	else
		return of_find_node_by_alias(root, str);

}

/**
 * of_modalias_node - Lookup appropriate modalias for a device node
 * @node:	pointer to a device tree node
 * @modalias:	Pointer to buffer that modalias value will be copied into
 * @len:	Length of modalias value
 *
 * Based on the value of the compatible property, this routine will attempt
 * to choose an appropriate modalias value for a particular device tree node.
 * It does this by stripping the manufacturer prefix (as delimited by a ',')
 * from the first entry in the compatible list property.
 *
 * This routine returns 0 on success, <0 on failure.
 */
int of_modalias_node(struct device_node *node, char *modalias, int len)
{
	const char *compatible, *p;
	int cplen;

	compatible = of_get_property(node, "compatible", &cplen);
	if (!compatible || strlen(compatible) > cplen)
		return -ENODEV;
	p = strchr(compatible, ',');
	strlcpy(modalias, p ? p + 1 : compatible, len);
	return 0;
}

struct device_node *of_get_root_node(void)
{
	return root_node;
}

int of_set_root_node(struct device_node *node)
{
	if (node && root_node)
		return -EBUSY;

	root_node = node;

	of_alias_scan();

	return 0;
}

/**
 *  of_device_is_available - check if a device is available for use
 *
 *  @device: Node to check for availability
 *
 *  Returns 1 if the status property is absent or set to "okay" or "ok",
 *  0 otherwise
 */
int of_device_is_available(const struct device_node *device)
{
	const char *status;
	int statlen;

	status = of_get_property(device, "status", &statlen);
	if (status == NULL)
		return 1;

	if (statlen > 0) {
		if (!strcmp(status, "okay") || !strcmp(status, "ok"))
			return 1;
	}

	return 0;
}

/**
 *	of_get_parent - Get a node's parent if any
 *	@node:	Node to get parent
 *
 *	Returns a pointer to the parent node or NULL if already at root.
 */
struct device_node *of_get_parent(const struct device_node *node)
{
	return (!node) ? NULL : node->parent;
}

/**
 *	of_get_next_available_child - Find the next available child node
 *	@node:	parent node
 *	@prev:	previous child of the parent node, or NULL to get first
 *
 *      This function is like of_get_next_child(), except that it
 *      automatically skips any disabled nodes (i.e. status = "disabled").
 */
struct device_node *of_get_next_available_child(const struct device_node *node,
	struct device_node *prev)
{
	prev = list_prepare_entry(prev, &node->children, parent_list);
	list_for_each_entry_continue(prev, &node->children, parent_list)
		if (of_device_is_available(prev))
			return prev;
	return NULL;
}

/**
 *	of_get_child_count - Count child nodes of given parent node
 *	@parent:	parent node
 *
 *      Returns the number of child nodes or -EINVAL on NULL parent node.
 */
int of_get_child_count(const struct device_node *parent)
{
	struct device_node *child;
	int num = 0;

	if (!parent)
		return -EINVAL;

	for_each_child_of_node(parent, child)
		num++;

	return num;
}

/**
 *	of_get_available_child_count - Count available child nodes of given
 *      parent node
 *	@parent:	parent node
 *
 *      Returns the number of available child nodes or -EINVAL on NULL parent
 *      node.
 */
int of_get_available_child_count(const struct device_node *parent)
{
	struct device_node *child;
	int num = 0;

	if (!parent)
		return -EINVAL;

	for_each_child_of_node(parent, child)
		if (of_device_is_available(child))
			num++;

	return num;
}

/**
 *	of_get_child_by_name - Find the child node by name for a given parent
 *	@node:	parent node
 *	@name:	child name to look for.
 *
 *      This function looks for child node for given matching name
 *
 *	Returns a node pointer if found or NULL.
 */
struct device_node *of_get_child_by_name(const struct device_node *node,
				const char *name)
{
	struct device_node *child;

	for_each_child_of_node(node, child)
		if (child->name && (of_node_cmp(child->name, name) == 0))
			return child;

	return NULL;
}

void of_print_nodes(struct device_node *node, int indent)
{
	struct device_node *n;
	struct property *p;
	int i;

	if (!node)
		return;

	for (i = 0; i < indent; i++)
		printf("\t");

	printf("%s%s\n", node->name, node->name ? " {" : "{");

	list_for_each_entry(p, &node->properties, list) {
		for (i = 0; i < indent + 1; i++)
			printf("\t");
		printf("%s", p->name);
		if (p->length) {
			printf(" = ");
			of_print_property(p->value, p->length);
		}
		printf(";\n");
	}

	list_for_each_entry(n, &node->children, parent_list) {
		of_print_nodes(n, indent + 1);
	}

	for (i = 0; i < indent; i++)
		printf("\t");
	printf("};\n");
}

struct device_node *of_new_node(struct device_node *parent, const char *name)
{
	struct device_node *node;
	int ret;

	node = xzalloc(sizeof(*node));
	node->parent = parent;
	if (parent)
		list_add_tail(&node->parent_list, &parent->children);

	INIT_LIST_HEAD(&node->children);
	INIT_LIST_HEAD(&node->properties);

	if (parent) {
		node->name = strdup(name);
		ret = asprintf(&node->full_name, "%s/%s", node->parent->full_name, name);
		if (ret < 0)
			return NULL;
		list_add(&node->list, &parent->list);
	} else {
		node->name = strdup("");
		node->full_name = strdup("");
		INIT_LIST_HEAD(&node->list);
	}

	return node;
}

struct property *of_new_property(struct device_node *node, const char *name,
		const void *data, int len)
{
	struct property *prop;

	prop = xzalloc(sizeof(*prop));
	prop->name = strdup(name);
	if (!prop->name) {
		free(prop);
		return NULL;
	}

	prop->length = len;
	prop->value = xzalloc(len);

	if (data)
		memcpy(prop->value, data, len);

	list_add_tail(&prop->list, &node->properties);

	return prop;
}

void of_delete_property(struct property *pp)
{
	if (!pp)
		return;

	list_del(&pp->list);

	free(pp->name);
	free(pp->value);
	free(pp);
}

/**
 * of_set_property - create a property for a given node
 * @node - the node
 * @name - the name of the property
 * @val - the value for the property
 * @len - the length of the properties value
 * @create - if true, the property is created if not existing already
 */
int of_set_property(struct device_node *np, const char *name, const void *val, int len,
		int create)
{
	struct property *pp = of_find_property(np, name, NULL);

	if (!np)
		return -ENOENT;

	if (!pp && !create)
		return -ENOENT;

	of_delete_property(pp);

	pp = of_new_property(np, name, val, len);
	if (!pp)
		return -ENOMEM;

	return 0;
}

struct device_node *of_chosen;
const char *of_model;

const char *of_get_model(void)
{
	return of_model;
}

const struct of_device_id of_default_bus_match_table[] = {
	{
		.compatible = "simple-bus",
	}, {
		/* sentinel */
	}
};

/**
 * of_create_node - create a new node including its parents
 * @path - the nodepath to create
 */
struct device_node *of_create_node(struct device_node *root, const char *path)
{
	char *slash, *p, *freep;
	struct device_node *tmp, *dn = root;

	if (*path != '/')
		return NULL;

	path++;

	p = freep = strdup(path);

	while (1) {
		if (!*p)
			goto out;

		slash = strchr(p, '/');
		if (slash)
			*slash = 0;

		tmp = of_get_child_by_name(dn, p);
		if (tmp)
			dn = tmp;
		else
			dn = of_new_node(dn, p);

		if (!dn)
			goto out;

		if (!slash)
			goto out;

		p = slash + 1;
	}
out:
	free(freep);

	return dn;
}

void of_delete_node(struct device_node *node)
{
	struct device_node *n, *nt;
	struct property *p, *pt;

	if (!node)
		return;

	list_for_each_entry_safe(p, pt, &node->properties, list)
		of_delete_property(p);

	list_for_each_entry_safe(n, nt, &node->children, parent_list)
		of_delete_node(n);

	if (node->parent) {
		list_del(&node->parent_list);
		list_del(&node->list);
	}

	free(node->name);
	free(node->full_name);
	free(node);

	if (node == root_node)
		of_set_root_node(NULL);
}

/**
 * of_device_enable - enable a devicenode device
 * @node - the node to enable
 *
 * This deletes the status property of a devicenode effectively
 * enabling the device.
 */
int of_device_enable(struct device_node *node)
{
	struct property *pp;

	pp = of_find_property(node, "status", NULL);
	if (!pp)
		return 0;

	of_delete_property(pp);

	return 0;
}

/**
 * of_device_enable_path - enable a devicenode
 * @path - the nodepath to enable
 *
 * wrapper around of_device_enable taking the nodepath as argument
 */
int of_device_enable_path(const char *path)
{
	struct device_node *node;

	node = of_find_node_by_path(path);
	if (!node)
		return -ENODEV;

	return of_device_enable(node);
}

/**
 * of_device_enable - disable a devicenode device
 * @node - the node to disable
 *
 * This sets the status of a devicenode to "disabled"
 */
int of_device_disable(struct device_node *node)
{
	return of_set_property(node, "status", "disabled", sizeof("disabled"), 1);
}

/**
 * of_device_disable_path - disable a devicenode
 * @path - the nodepath to disable
 *
 * wrapper around of_device_disable taking the nodepath as argument
 */
int of_device_disable_path(const char *path)
{
	struct device_node *node;

	node = of_find_node_by_path(path);
	if (!node)
		return -ENODEV;

	return of_device_disable(node);
}

int scan_proc_dir(struct device_node *node, const char *path)
{
	DIR *dir;
	struct dirent *dirent;
	struct stat s;
	int ret;
	void *buf;

	dir = opendir(path);
	if (!dir)
		return -errno;

	while (1) {
		char *cur;

		dirent = readdir(dir);
		if (!dirent)
			break;

		if (dirent->d_name[0] == '.')
			continue;

		ret = asprintf(&cur, "%s/%s", path, dirent->d_name);
		if (ret < 0)
			return -ENOMEM;

		ret = stat(cur, &s);
		if (ret)
			return -errno;

		if (S_ISREG(s.st_mode)) {
			int fd;

			fd = open(cur, O_RDONLY);
			if (fd < 0)
				return -errno;

			buf = xzalloc(s.st_size);
			ret = read(fd, buf, s.st_size);
			if (ret < 0)
				return -errno;
			close(fd);

			of_new_property(node, dirent->d_name, buf, s.st_size);
		}

		if (S_ISDIR(s.st_mode)) {
			struct device_node *new;
			new = of_new_node(node, dirent->d_name);
			scan_proc_dir(new, cur);
		}

		free(cur);
	}

	closedir(dir);

	return 0;
}

struct device_node *of_read_proc_devicetree(void)
{
	struct device_node *root;
	int ret;

	root = of_new_node(NULL, NULL);

	ret = scan_proc_dir(root, "/sys/firmware/devicetree/base");
	if (!ret)
		return root;

	ret = scan_proc_dir(root, "/proc/device-tree");
	if (!ret)
		return root;

	of_delete_node(root);
	return ERR_PTR(ret);
}

struct udev_device *of_find_device_by_node_path(const char *of_full_path)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;
	
	udev = udev_new();
	if (!udev) {
		fprintf(stderr, "Can't create udev\n");
		return NULL;
	}
	
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_property(enumerate, "OF_FULLNAME", of_full_path);
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;
		
		/*
		 * Get the filename of the /sys entry for the device
		 * and create a udev_device object (dev) representing it
		 */
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);

		goto out;
	}

	dev = NULL;
out:
	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	return dev;       
}

int of_parse_partition_from_path(struct of_path *op, const char *name)
{
	struct device_node *node;

	for_each_child_of_node(op->node, node) {
		const char *partname;
		int len;

		partname = of_get_property(node, "label", &len);
		if (!strcmp(partname, name)) {
			const __be32 *reg;
			int a_cells, s_cells;

			reg = of_get_property(node, "reg", &len);
			if (!reg)
				continue;

			a_cells = of_n_addr_cells(node);
			s_cells = of_n_size_cells(node);

			op->offset = of_read_number(reg, a_cells);
			op->size = of_read_number(reg + a_cells, s_cells);

			return 0;
		}
	}

	return -EINVAL;
}

struct udev_device *device_find_partition(struct udev_device *dev, const char *name,
					  const char *sysattr)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *part;

	udev = udev_new();
	if (!udev) {
		fprintf(stderr, "Can't create udev\n");
		return NULL;
	}

	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_parent(enumerate, dev);
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path, *partname;
		path = udev_list_entry_get_name(dev_list_entry);
		part = udev_device_new_from_syspath(udev, path);
		partname = udev_device_get_sysattr_value(part, sysattr);
		if (!partname)
			continue;
		if (!strcmp(partname, name))
			return part;
	}

	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	return NULL;
}

struct udev_device *device_find_devnode(struct udev_device *dev)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *devnode;

	udev = udev_new();
	if (!udev) {
		fprintf(stderr, "Can't create udev\n");
		return NULL;
	}

	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_parent(enumerate, dev);
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;

		path = udev_list_entry_get_name(dev_list_entry);
		devnode = udev_device_new_from_syspath(udev, path);

		if (!udev_device_get_devnode(devnode))
			continue;
		else
			return devnode;
	}

	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	return NULL;
}

struct of_path_type {
	const char *name;
	int (*parse)(struct of_path *op, const char *str);
};

/**
 * of_path_type_partname - find a partition based on physical device and
 *                         partition name
 * @op: of_path context
 * @name: the partition name to find
 */
static int of_path_type_partname(struct of_path *op, const char *name)
{
	struct udev_device *part;
	struct stat s;
	int ret;
	struct device_node *node;

	if (!op->dev)
		return -EINVAL;

	part = device_find_partition(op->dev, name, "name");
	if (part) {
		if (udev_device_get_devnode(part) != NULL) {
			op->devpath = strdup(udev_device_get_devnode(part));
			pr_debug("%s: found part '%s'\n", __func__, name);
			ret = of_parse_partition_from_path(op, name);
		} else {
			pr_debug("%s: '%s' not found\n", __func__, name);
			ret = -EINVAL;
		}
		return ret;
	}

	part = device_find_partition(op->dev, name, "partition");
	if (part) {
		if (udev_device_get_devnode(part) != NULL) {
			op->devpath = strdup(udev_device_get_devnode(part));
			pr_debug("%s: found part '%s'\n", __func__, name);
			ret = of_parse_partition_from_path(op, name);
		} else {
			pr_debug("%s: '%s' not found\n", __func__, name);
			ret = -EINVAL;
		}
		return ret;
	}

	part = device_find_devnode(op->dev);
	if (part) {
		if (udev_device_get_devnode(part) != NULL) {
			op->devpath = strdup(udev_device_get_devnode(part));

			if (!op->devpath)
				return -EINVAL;

			ret = stat(op->devpath, &s);
			if (ret)
				return -errno;

			ret = of_parse_partition_from_path(op, name);
		} else {
			pr_debug("%s: '%s' not found\n", __func__, name);
			ret = -EINVAL;
		}
		return ret;
	}

	ret = asprintf(&op->devpath, "%s/eeprom", udev_device_get_syspath(op->dev));
	if (ret < 0)
		return -ENOMEM;

	ret = stat(op->devpath, &s);
	if (ret)
		return -errno;

	of_parse_partition_from_path(op, name);

	return 0;
}

static struct of_path_type of_path_types[] = {
	{
		.name = "partname",
		.parse = of_path_type_partname,
	},
};
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static int of_path_parse_one(struct of_path *op, const char *str)
{
	int i, ret;
	char *name, *desc;

	pr_debug("parsing: %s\n", str);

	name = strdup(str);
	desc = strchr(name, ':');
	if (!desc) {
		free(name);
		return -EINVAL;
	}

	*desc = 0;
	desc++;

	for (i = 0; i < ARRAY_SIZE(of_path_types); i++) {
		if (!strcmp(of_path_types[i].name, name)) {
			ret = of_path_types[i].parse(op, desc);
			goto out;
		}
	}

	ret = -EINVAL;
out:
	free(name);

	return ret;
}

/**
 * of_find_path - translate a path description in the devicetree to a device node
 *                path
 *
 * @node: the node containing the property with the path description
 * @propname: the property name of the path description
 * @outpath: if this function returns 0 outpath will contain the path belonging
 *           to the input path description. Must be freed with free().
 *
 * pathes in the devicetree have the form of a multistring property. The first
 * string contains the full path to the physical device containing the path.
 * The remaining strings have the form "<type>:<options>". Currently supported
 * for <type> are:
 *
 * partname:<partname> - find a partition by its partition name. For mtd
 *                       partitions this is the label. For DOS partitions
 *                       this is the number beginning with 0.
 *
 * examples:
 *
 * device-path = &mmc0, "partname:0";
 * device-path = &norflash, "partname:barebox-environment";
 */
int of_find_path(struct device_node *node, const char *propname, struct of_path *op)
{
	struct device_node *rnode;
	const char *path, *str;
	int i, len, ret;

	memset(op, 0, sizeof(*op));

	path = of_get_property(node, propname, &len);
	if (!path)
		return -EINVAL;

	rnode = of_find_node_by_path(path);
	if (!rnode)
		return -ENODEV;

	op->node = rnode;

	op->dev = of_find_device_by_node_path(rnode->full_name);
	if (!op->dev)
		return -ENODEV;

	i = 1;

	while (1) {
		ret = of_property_read_string_index(node, propname, i++, &str);
		if (ret)
			break;

		ret = of_path_parse_one(op, str);
		if (ret)
			return ret;
	}

	if (!op->devpath)
		return -ENOENT;

	pr_debug("%s: devpath: %s ofs: 0x%08llx size: 0x%08lx\n",
			__func__, op->devpath, op->offset, op->size);

	return 0;
}
