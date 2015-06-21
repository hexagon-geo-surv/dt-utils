/*
 * state.c - state handling tool
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/fs.h>
#include <mtd/mtd-abi.h>

#include <common.h>
#include <digest.h>
#include <dt.h>
#include <fdt.h>
#include <keystore.h>

static int verbose;

struct state_variable;

static int __state_uint8_set(struct state_variable *var, const char *val);
static char *__state_uint8_get(struct state_variable *var);
static int __state_uint32_set(struct state_variable *var, const char *val);
static char *__state_uint32_get(struct state_variable *var);
static int __state_enum32_set(struct state_variable *sv, const char *val);
static char *__state_enum32_get(struct state_variable *var);
static void __state_enum32_info(struct state_variable *var);
static int __state_mac_set(struct state_variable *var, const char *val);
static char *__state_mac_get(struct state_variable *var);
static int __state_string_set(struct state_variable *var, const char *val);
static char *__state_string_get(struct state_variable *var);

#define asprintf(fmt, arg...) barebox_asprintf(fmt, ##arg)

/* ------------------------------------------------------------ */

#define RAW_BACKEND_COPIES 2

struct state_backend;

struct state {
	struct device_d dev;
	struct device_node *root;
	struct list_head variables;
	const char *name;
	struct list_head list;
	struct state_backend *backend;
	uint32_t magic;
	unsigned int dirty;
};

struct state_backend {
	int (*load)(struct state_backend *backend, struct state *state);
	int (*save)(struct state_backend *backend, struct state *state);
	const char *name;
	const char *of_path;
	const char *path;
	struct digest *digest;
};

enum state_variable_type {
	STATE_TYPE_INVALID = 0,
	STATE_TYPE_ENUM,
	STATE_TYPE_U8,
	STATE_TYPE_U32,
	STATE_TYPE_MAC,
	STATE_TYPE_STRING,
};

/* instance of a single variable */
struct state_variable {
	enum state_variable_type type;
	struct list_head list;
	const char *name;
	unsigned int start;
	unsigned int size;
	void *raw;
};

enum state_convert {
	STATE_CONVERT_FROM_NODE,
	STATE_CONVERT_FROM_NODE_CREATE,
	STATE_CONVERT_TO_NODE,
	STATE_CONVERT_FIXUP,
};

/* A variable type (uint32, enum32) */
struct variable_type {
	enum state_variable_type type;
	const char *type_name;
	struct list_head list;
	int (*export)(struct state_variable *, struct device_node *,
			enum state_convert);
	int (*import)(struct state_variable *, struct device_node *);
	struct state_variable *(*create)(struct state *state,
			const char *name, struct device_node *);
	char *(*get)(struct state_variable *);
	int (*set)(struct state_variable *, const char *val);
	void (*info)(struct state_variable *);
};

/* list of all registered state instances */
static LIST_HEAD(state_list);

static int state_set_dirty(struct param_d *p, void *priv)
{
	struct state *state = priv;

	state->dirty = 1;

	return 0;
}

/*
 *  uint32
 */
struct state_uint32 {
	struct state_variable var;
	struct param_d *param;
	struct state *state;
	uint32_t value;
	uint32_t value_default;
};

static int state_var_compare(struct list_head *a, struct list_head *b)
{
	struct state_variable *va = list_entry(a, struct state_variable, list);
	struct state_variable *vb = list_entry(b, struct state_variable, list);

	return va->start < vb->start ? -1 : 1;
}

static void state_add_var(struct state *state, struct state_variable *var)
{
	list_add_sort(&var->list, &state->variables, state_var_compare);
}

static inline struct state_uint32 *to_state_uint32(struct state_variable *s)
{
	return container_of(s, struct state_uint32, var);
}

static int state_uint32_export(struct state_variable *var,
		struct device_node *node, enum state_convert conv)
{
	struct state_uint32 *su32 = to_state_uint32(var);
	int ret;

	if (su32->value_default || conv == STATE_CONVERT_FIXUP) {
		ret = of_property_write_u32(node, "default",
					    su32->value_default);
		if (ret || conv == STATE_CONVERT_FIXUP)
			return ret;
	}

	return of_property_write_u32(node, "value", su32->value);
}

static int state_uint32_import(struct state_variable *sv,
			       struct device_node *node)
{
	struct state_uint32 *su32 = to_state_uint32(sv);

	of_property_read_u32(node, "default", &su32->value_default);
	if (of_property_read_u32(node, "value", &su32->value))
		su32->value = su32->value_default;

	return 0;
}

static int state_uint8_set(struct param_d *p, void *priv)
{
	struct state_uint32 *su32 = priv;
	struct state *state = su32->state;

	if (su32->value > 255)
		return -ERANGE;

	return state_set_dirty(p, state);
}

static struct state_variable *state_uint8_create(struct state *state,
		const char *name, struct device_node *node)
{
	struct state_uint32 *su32;
	struct param_d *param;

	su32 = xzalloc(sizeof(*su32));

	param = dev_add_param_int(&state->dev, name, state_uint8_set,
				  NULL, &su32->value, "%u", su32);
	if (IS_ERR(param)) {
		free(su32);
		return ERR_CAST(param);
	}

	su32->param = param;
	su32->var.size = sizeof(uint8_t);
#ifdef __LITTLE_ENDIAN
	su32->var.raw = &su32->value;
#else
	su32->var.raw = &su32->value + 3;
#endif
	su32->state = state;

	return &su32->var;
}

static struct state_variable *state_uint32_create(struct state *state,
		const char *name, struct device_node *node)
{
	struct state_uint32 *su32;
	struct param_d *param;

	su32 = xzalloc(sizeof(*su32));

	param = dev_add_param_int(&state->dev, name, state_set_dirty,
				  NULL, &su32->value, "%u", state);
	if (IS_ERR(param)) {
		free(su32);
		return ERR_CAST(param);
	}

	su32->param = param;
	su32->var.size = sizeof(uint32_t);
	su32->var.raw = &su32->value;

	return &su32->var;
}

/*
 *  enum32
 */
struct state_enum32 {
	struct state_variable var;
	struct param_d *param;
	uint32_t value;
	uint32_t value_default;
	const char **names;
	int num_names;
};

static inline struct state_enum32 *to_state_enum32(struct state_variable *s)
{
	return container_of(s, struct state_enum32, var);
}

static int state_enum32_export(struct state_variable *var,
		struct device_node *node, enum state_convert conv)
{
	struct state_enum32 *enum32 = to_state_enum32(var);
	int ret, i, len;
	char *prop, *str;

	if (enum32->value_default || conv == STATE_CONVERT_FIXUP) {
		ret = of_property_write_u32(node, "default",
					    enum32->value_default);
		if (ret || conv == STATE_CONVERT_FIXUP)
			return ret;
	}

	ret = of_property_write_u32(node, "value", enum32->value);
	if (ret)
		return ret;

	len = 0;

	for (i = 0; i < enum32->num_names; i++)
		len += strlen(enum32->names[i]) + 1;

	prop = xzalloc(len);
	str = prop;

	for (i = 0; i < enum32->num_names; i++)
		str += sprintf(str, "%s", enum32->names[i]) + 1;

	ret = of_set_property(node, "names", prop, len, 1);

	free(prop);

	return ret;
}

static int state_enum32_import(struct state_variable *sv,
			       struct device_node *node)
{
	struct state_enum32 *enum32 = to_state_enum32(sv);
	int len;
	const __be32 *value, *value_default;

	value = of_get_property(node, "value", &len);
	if (value && len != sizeof(uint32_t))
		return -EINVAL;

	value_default = of_get_property(node, "default", &len);
	if (value_default && len != sizeof(uint32_t))
		return -EINVAL;

	if (value_default)
		enum32->value_default = be32_to_cpu(*value_default);
	if (value)
		enum32->value = be32_to_cpu(*value);
	else
		enum32->value = enum32->value_default;

	return 0;
}

static struct state_variable *state_enum32_create(struct state *state,
		const char *name, struct device_node *node)
{
	struct state_enum32 *enum32;
	int ret, i, num_names;

	enum32 = xzalloc(sizeof(*enum32));

	num_names = of_property_count_strings(node, "names");

	enum32->names = xzalloc(sizeof(char *) * num_names);
	enum32->num_names = num_names;
	enum32->var.size = sizeof(uint32_t);
	enum32->var.raw = &enum32->value;

	for (i = 0; i < num_names; i++) {
		const char *name;

		ret = of_property_read_string_index(node, "names", i, &name);
		if (ret)
			goto out;
		enum32->names[i] = xstrdup(name);
	}

	enum32->param = dev_add_param_enum(&state->dev, name, state_set_dirty,
			NULL, &enum32->value, enum32->names, num_names, state);
	if (IS_ERR(enum32->param)) {
		ret = PTR_ERR(enum32->param);
		goto out;
	}

	return &enum32->var;
out:
	for (i--; i >= 0; i--)
		free((char *)enum32->names[i]);
	free(enum32->names);
	free(enum32);
	return ERR_PTR(ret);
}

/*
 *  MAC address
 */
struct state_mac {
	struct state_variable var;
	struct param_d *param;
	uint8_t value[6];
	uint8_t value_default[6];
};

static inline struct state_mac *to_state_mac(struct state_variable *s)
{
	return container_of(s, struct state_mac, var);
}

static int state_mac_export(struct state_variable *var,
		struct device_node *node, enum state_convert conv)
{
	struct state_mac *mac = to_state_mac(var);
	int ret;

	ret = of_property_write_u8_array(node, "default", mac->value_default,
					 ARRAY_SIZE(mac->value_default));
	if (ret || conv == STATE_CONVERT_FIXUP)
		return ret;

	return of_property_write_u8_array(node, "value", mac->value,
					  ARRAY_SIZE(mac->value));
}

static int state_mac_import(struct state_variable *sv,
			    struct device_node *node)
{
	struct state_mac *mac = to_state_mac(sv);

	of_property_read_u8_array(node, "default", mac->value_default,
				  ARRAY_SIZE(mac->value_default));
	if (of_property_read_u8_array(node, "value", mac->value,
				      ARRAY_SIZE(mac->value)))
		memcpy(mac->value, mac->value_default, ARRAY_SIZE(mac->value));

	return 0;
}

static struct state_variable *state_mac_create(struct state *state,
		const char *name, struct device_node *node)
{
	struct state_mac *mac;
	int ret;

	mac = xzalloc(sizeof(*mac));

	mac->var.size = ARRAY_SIZE(mac->value);
	mac->var.raw = mac->value;

	mac->param = dev_add_param_mac(&state->dev, name, state_set_dirty,
			NULL, mac->value, state);
	if (IS_ERR(mac->param)) {
		ret = PTR_ERR(mac->param);
		goto out;
	}

	return &mac->var;
out:
	free(mac);
	return ERR_PTR(ret);
}

/*
 *  string
 */
struct state_string {
	struct state_variable var;
	struct param_d *param;
	struct state *state;
	char *value;
	const char *value_default;
	char raw[];
};

static inline struct state_string *to_state_string(struct state_variable *s)
{
	return container_of(s, struct state_string, var);
}

static int state_string_export(struct state_variable *var,
		struct device_node *node, enum state_convert conv)
{
	struct state_string *string = to_state_string(var);
	int ret = 0;

	if (string->value_default || conv == STATE_CONVERT_FIXUP) {
		ret = of_set_property(node, "default", string->value_default,
				      strlen(string->value_default) + 1, 1);

		if (ret || conv == STATE_CONVERT_FIXUP)
			return ret;
	}

	if (string->value)
		ret = of_set_property(node, "value", string->value,
				      strlen(string->value) + 1, 1);

	return ret;
}

static int state_string_copy_to_raw(struct state_string *string,
				    const char *src)
{
	size_t len;

	len = strlen(src);
	if (len > string->var.size)
		return -EILSEQ;

	/* copy string and clear remaining contents of buffer */
	memcpy(string->raw, src, len);
	memset(string->raw + len, 0x0, string->var.size - len);

	return 0;
}

static int state_string_import(struct state_variable *sv,
			       struct device_node *node)
{
	struct state_string *string = to_state_string(sv);
	const char *value = NULL;
	size_t len;
	int ret;

	of_property_read_string(node, "default", &string->value_default);
	if (string->value_default) {
		len = strlen(string->value_default);
		if (len > string->var.size)
			return -EILSEQ;
	}

	ret = of_property_read_string(node, "value", &value);
	if (ret)
		value = string->value_default;

	if (value)
		return state_string_copy_to_raw(string, value);

	return 0;
}

static int state_string_set(struct param_d *p, void *priv)
{
	struct state_string *string = priv;
	struct state *state = string->state;
	int ret;

	ret = state_string_copy_to_raw(string, string->value);
	if (ret)
		return ret;

	return state_set_dirty(p, state);
}

static int state_string_get(struct param_d *p, void *priv)
{
	struct state_string *string = priv;

	free(string->value);
	if (string->raw[0])
		string->value = xstrndup(string->raw, string->var.size);
	else
		string->value = xstrdup("");

	return 0;
}

static struct state_variable *state_string_create(struct state *state,
		const char *name, struct device_node *node)
{
	struct state_string *string;
	u32 start_size[2];
	int ret;

	ret = of_property_read_u32_array(node, "reg", start_size,
					 ARRAY_SIZE(start_size));
	if (ret) {
		dev_err(&state->dev,
			"%s: reg property not found\n", name);
		return ERR_PTR(ret);
	}

	/* limit to arbitrary len of 4k */
	if (start_size[1] > 4096)
		return ERR_PTR(-EILSEQ);

	string = xzalloc(sizeof(*string) + start_size[1]);
	string->var.size = start_size[1];
	string->var.raw = &string->raw;
	string->state = state;

	string->param = dev_add_param_string(&state->dev, name, state_string_set,
					     state_string_get, &string->value, string);
	if (IS_ERR(string->param)) {
		ret = PTR_ERR(string->param);
		goto out;
	}

	return &string->var;
out:
	free(string);
	return ERR_PTR(ret);
}

static struct variable_type types[] =  {
	{
		.type = STATE_TYPE_U8,
		.type_name = "uint8",
		.export = state_uint32_export,
		.import = state_uint32_import,
		.create = state_uint8_create,
		.set = __state_uint8_set,
		.get = __state_uint32_get,
	}, {
		.type = STATE_TYPE_U32,
		.type_name = "uint32",
		.export = state_uint32_export,
		.import = state_uint32_import,
		.create = state_uint32_create,
		.set = __state_uint32_set,
		.get = __state_uint32_get,
	}, {
		.type = STATE_TYPE_ENUM,
		.type_name = "enum32",
		.export = state_enum32_export,
		.import = state_enum32_import,
		.create = state_enum32_create,
		.set = __state_enum32_set,
		.get = __state_enum32_get,
		.info = __state_enum32_info,
	}, {
		.type = STATE_TYPE_MAC,
		.type_name = "mac",
		.export = state_mac_export,
		.import = state_mac_import,
		.create = state_mac_create,
		.set = __state_mac_set,
		.get = __state_mac_get,
	}, {
		.type = STATE_TYPE_STRING,
		.type_name = "string",
		.export = state_string_export,
		.import = state_string_import,
		.create = state_string_create,
		.set = __state_string_set,
		.get = __state_string_get,
	},
};

static struct variable_type *state_find_type_by_name(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(types); i++) {
		if (!strcmp(name, types[i].type_name)) {
			return &types[i];
		}
	}

	return NULL;
}

/*
 * Generic state functions
 */

static struct state *state_new(const char *name)
{
	struct state *state;
	int ret;

	state = xzalloc(sizeof(*state));
	safe_strncpy(state->dev.name, name, MAX_DRIVER_NAME);
	state->name = state->dev.name;
	state->dev.id = DEVICE_ID_SINGLE;
	INIT_LIST_HEAD(&state->variables);

	ret = register_device(&state->dev);
	if (ret) {
		free(state);
		return ERR_PTR(ret);
	}

	state->dirty = 1;
	dev_add_param_bool(&state->dev, "dirty", NULL, NULL, &state->dirty,
			   NULL);

	list_add_tail(&state->list, &state_list);

	return state;
}

static struct state_variable *state_find_var(struct state *state,
					     const char *name)
{
	struct state_variable *sv;

	list_for_each_entry(sv, &state->variables, list) {
		if (!strcmp(sv->name, name))
			return sv;
	}

	return ERR_PTR(-ENOENT);
}

static int state_convert_node_variable(struct state *state,
		struct device_node *node, struct device_node *parent,
		const char *parent_name, enum state_convert conv)
{
	const struct variable_type *vtype;
	struct device_node *child;
	struct device_node *new_node = NULL;
	struct state_variable *sv;
	const char *type_name;
	char *short_name, *name, *indexs;
	u32 start_size[2];
	int ret;

	/* strip trailing @<ADDRESS> */
	short_name = xstrdup(node->name);
	indexs = strchr(short_name, '@');
	if (indexs)
		*indexs = 0;

	/* construct full name */
	name = asprintf("%s%s%s",
			parent_name, parent_name[0] ? "." : "", short_name);
	free(short_name);

	if ((conv == STATE_CONVERT_TO_NODE) ||
	    (conv == STATE_CONVERT_FIXUP))
		new_node = of_new_node(parent, node->name);

	for_each_child_of_node(node, child) {
		ret = state_convert_node_variable(state, child, new_node, name,
						  conv);
		if (ret)
			goto out_free;
	}

	/* parents are allowed to have no type */
	ret = of_property_read_string(node, "type", &type_name);
	if (!list_empty(&node->children) && ret == -EINVAL) {
		if (conv == STATE_CONVERT_FIXUP) {
			ret = of_property_write_u32(new_node, "#address-cells", 1);
			if (ret)
				goto out_free;

			ret = of_property_write_u32(new_node, "#size-cells", 1);
			if (ret)
				goto out_free;
		}
		ret = 0;
		goto out_free;
	} else if (ret) {
		goto out_free;
	}

	vtype = state_find_type_by_name(type_name);
	if (!vtype) {
		ret = -ENOENT;
		goto out_free;
	}

	if (conv == STATE_CONVERT_FROM_NODE_CREATE) {
		sv = vtype->create(state, name, node);
		if (IS_ERR(sv)) {
			ret = PTR_ERR(sv);
			dev_err(&state->dev, "failed to create %s: %s\n",
				name, strerror(-ret));
			goto out_free;
		}

		ret = of_property_read_u32_array(node, "reg", start_size,
						 ARRAY_SIZE(start_size));
		if (ret) {
			dev_err(&state->dev,
				"%s: reg property not found\n", name);
			goto out_free;
		}

		if (start_size[1] != sv->size) {
			dev_err(&state->dev,
				"%s: size mismatch: type=%s(size=%u) size=%u\n",
				name, type_name, sv->size, start_size[1]);
			ret = -EOVERFLOW;
			goto out_free;
		}

		sv->name = name;
		sv->start = start_size[0];
		sv->type = vtype->type;
		state_add_var(state, sv);
	} else {
		sv = state_find_var(state, name);
		if (IS_ERR(sv)) {
			/* we ignore this error */
			dev_dbg(&state->dev,
				"no such variable: %s: %s\n",
				name, strerror(-ret));
			ret = 0;
			goto out_free;
		}
		free(name);

		if ((conv == STATE_CONVERT_TO_NODE) ||
		    (conv == STATE_CONVERT_FIXUP)) {
			ret = of_set_property(new_node, "type",
					      vtype->type_name,
					      strlen(vtype->type_name) + 1, 1);
			if (ret)
				goto out;

			start_size[0] = sv->start;
			start_size[1] = sv->size;
			ret = of_property_write_u32_array(new_node, "reg",
							  start_size,
							  ARRAY_SIZE(start_size));
			if (ret)
				goto out;
		}
	}

	if ((conv == STATE_CONVERT_TO_NODE) ||
	    (conv == STATE_CONVERT_FIXUP))
		ret = vtype->export(sv, new_node, conv);
	else
		ret = vtype->import(sv, node);

	if (ret)
		goto out;

	return 0;
out_free:
	free(name);
out:
	return ret;
}

static struct device_node *state_to_node(struct state *state, struct device_node *parent,
					 enum state_convert conv)
{
	struct device_node *child;
	struct device_node *root;
	int ret;

	root = of_new_node(parent, state->root->name);
	ret = of_property_write_u32(root, "magic", state->magic);
	if (ret)
		goto out;

	for_each_child_of_node(state->root, child) {
		ret = state_convert_node_variable(state, child, root, "",
						  conv);
		if (ret)
			goto out;
	}

	return root;
out:
	of_delete_node(root);
	return ERR_PTR(ret);
}

static int state_from_node(struct state *state, struct device_node *node,
			   bool create)
{
	struct device_node *child;
	enum state_convert conv;
	int ret;
	uint32_t magic;

	ret = of_property_read_u32(node, "magic", &magic);
	if (ret)
		return ret;

	if (create) {
		conv = STATE_CONVERT_FROM_NODE_CREATE;
		state->root = node;
		state->magic = magic;
	} else {
		conv = STATE_CONVERT_FROM_NODE;
		if (state->magic && state->magic != magic) {
			dev_err(&state->dev,
					"invalid magic 0x%08x, should be 0x%08x\n",
					magic, state->magic);
			return -EINVAL;
		}
	}

	for_each_child_of_node(node, child) {
		ret = state_convert_node_variable(state, child, NULL, "", conv);
		if (ret)
			return ret;
	}

	/* check for overlapping variables */
	if (create) {
		const struct state_variable *sv;

		/* start with second entry */
		sv = list_first_entry(&state->variables,
				      struct state_variable, list);

		list_for_each_entry_continue(sv, &state->variables, list) {
			const struct state_variable *last_sv;

			last_sv = list_last_entry(&sv->list,
						  struct state_variable, list);
			if ((last_sv->start + last_sv->size - 1) < sv->start)
				continue;

			dev_err(&state->dev,
				"ERROR: Conflicting variable position between: "
				"%s (0x%02x..0x%02x) and %s (0x%02x..0x%02x)\n",
				last_sv->name, last_sv->start,
				last_sv->start + last_sv->size - 1,
				sv->name, sv->start, sv->start + sv->size - 1);

			ret |= -EINVAL;
		}
	}

	return ret;
}

static int of_state_fixup(struct device_node *root, void *ctx)
{
	struct state *state = ctx;
	const char *compatible = "barebox,state";
	struct device_node *new_node, *node, *parent, *backend_node;
	struct property *p;
	int ret;
	phandle phandle;

	node = of_find_node_by_path_from(root, state->root->full_name);
	if (node) {
		/* replace existing node - it will be deleted later */
		parent = node->parent;
	} else {
		char *of_path, *c;

		/* look for parent, remove last '/' from path */
		of_path = xstrdup(state->root->full_name);
		c = strrchr(of_path, '/');
		if (!c)
			return -ENODEV;
		*c = '0';
		parent = of_find_node_by_path(of_path);
		if (!parent)
			parent = root;

		free(of_path);
	}

	/* serialize variable definitions */
	new_node = state_to_node(state, parent, STATE_CONVERT_FIXUP);
	if (IS_ERR(new_node))
		return PTR_ERR(new_node);

	/* compatible */
	p = of_new_property(new_node, "compatible", compatible,
			    strlen(compatible) + 1);
	if (!p) {
		ret = -ENOMEM;
		goto out;
	}

	/* backend-type */
	if (!state->backend) {
		ret = -ENODEV;
		goto out;
	}

	p = of_new_property(new_node, "backend-type", state->backend->name,
			    strlen(state->backend->name) + 1);
	if (!p) {
		ret = -ENOMEM;
		goto out;
	}

	/* backend phandle */
	backend_node = of_find_node_by_path_from(root, state->backend->of_path);
	if (!backend_node) {
		ret = -ENODEV;
		goto out;
	}

	phandle = of_node_create_phandle(backend_node);
	ret = of_property_write_u32(new_node, "backend", phandle);
	if (ret)
		goto out;

	if (state->backend->digest) {
		p = of_new_property(new_node, "algo",
				    digest_name(state->backend->digest),
				    strlen(digest_name(state->backend->digest)) + 1);
		if (!p) {
			ret = -ENOMEM;
			goto out;
		}
	}

	/* address-cells + size-cells */
	ret = of_property_write_u32(new_node, "#address-cells", 1);
	if (ret)
		goto out;

	ret = of_property_write_u32(new_node, "#size-cells", 1);
	if (ret)
		goto out;

	/* delete existing node */
	if (node)
		of_delete_node(node);

	return 0;

 out:
	of_delete_node(new_node);
	return ret;
}

void state_release(struct state *state)
{
	of_unregister_fixup(of_state_fixup, state);
	list_del(&state->list);
	unregister_device(&state->dev);
	free(state);
}

/*
 * state_new_from_node - create a new state instance from a device_node
 *
 * @name	The name of the new state instance
 * @node	The device_node describing the new state instance
 */
struct state *state_new_from_node(const char *name, struct device_node *node)
{
	struct state *state;
	int ret;

	state = state_new(name);
	if (IS_ERR(state))
		return state;

	ret = state_from_node(state, node, 1);
	if (ret) {
		state_release(state);
		return ERR_PTR(ret);
	}

	ret = of_register_fixup(of_state_fixup, state);
	if (ret) {
		state_release(state);
		return ERR_PTR(ret);
	}

	return state;
}

/*
 * state_by_name - find a state instance by name
 *
 * @name	The name of the state instance
 */
struct state *state_by_name(const char *name)
{
	struct state *state;

	list_for_each_entry(state, &state_list, list) {
		if (!strcmp(name, state->name))
			return state;
	}

	return NULL;
}

/*
 * state_by_node - find a state instance by of node
 *
 * @node	The of node of the state intance
 */
struct state *state_by_node(const struct device_node *node)
{
	struct state *state;

	list_for_each_entry(state, &state_list, list) {
		if (state->root == node)
			return state;
	}

	return NULL;
}

int state_get_name(const struct state *state, char const **name)
{
	*name = xstrdup(state->name);

	return 0;
}

/*
 * state_load - load a state from the backing store
 *
 * @state	The state instance to load
 */
int state_load(struct state *state)
{
	int ret;

	if (!state->backend)
		return -ENOSYS;

	ret = state->backend->load(state->backend, state);
	if (ret) {
		dev_warn(&state->dev, "load failed\n");
		state->dirty = 1;
	} else {
		dev_info(&state->dev, "load successful\n");
		state->dirty = 0;
	}

	return ret;
}

/*
 * state_save - save a state to the backing store
 *
 * @state	The state instance to save
 */
int state_save(struct state *state)
{
	int ret;

	if (!state->dirty)
		return 0;

	if (!state->backend)
		return -ENOSYS;

	ret = state->backend->save(state->backend, state);
	if (ret)
		return ret;

	state->dirty = 0;

	return 0;
}

void state_info(void)
{
	struct state *state;

	printf("registered state instances:\n");

	list_for_each_entry(state, &state_list, list) {
		printf("%-20s ", state->name);
		if (state->backend)
			printf("(backend: %s, path: %s)\n",
			       state->backend->name, state->backend->path);
		else
			printf("(no backend)\n");
	}
}

static int mtd_get_meminfo(const char *path, struct mtd_info_user *meminfo)
{
	int fd, ret;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return fd;

	ret = ioctl(fd, MEMGETINFO, meminfo);

	close(fd);

	return ret;
}

/*
 * DTB backend implementation
 */
struct state_backend_dtb {
	struct state_backend backend;
	bool need_erase;
};

static int state_backend_dtb_load(struct state_backend *backend,
				  struct state *state)
{
	struct device_node *root;
	void *fdt;
	int ret;
	size_t len;

	fdt = read_file(backend->path, &len);
	if (!fdt) {
		dev_err(&state->dev, "cannot read %s\n", backend->path);
		return -EINVAL;
	}

	root = of_unflatten_dtb(fdt);

	free(fdt);

	if (IS_ERR(root))
		return PTR_ERR(root);

	ret = state_from_node(state, root, 0);

	return ret;
}

static int state_backend_dtb_save(struct state_backend *backend,
				  struct state *state)
{
	struct state_backend_dtb *backend_dtb = container_of(backend,
			struct state_backend_dtb, backend);
	int ret, fd;
	struct device_node *root;
	struct fdt_header *fdt;

	root = state_to_node(state, NULL, STATE_CONVERT_TO_NODE);
	if (IS_ERR(root))
		return PTR_ERR(root);

	fdt = of_flatten_dtb(root);
	if (!fdt)
		return -EINVAL;

	fd = open(backend->path, O_WRONLY);
	if (fd < 0) {
		ret = fd;
		goto out;
	}

	if (backend_dtb->need_erase) {
		ret = erase(fd, fdt32_to_cpu(fdt->totalsize), 0);
		if (ret) {
			close(fd);
			goto out;
		}
	}

	ret = write_full(fd, fdt, fdt32_to_cpu(fdt->totalsize));

	close(fd);

	if (ret < 0)
		goto out;

	ret = 0;
out:
	free(fdt);
	of_delete_node(root);

	return ret;
}

/*
 * state_backend_dtb_file - create a dtb backend store for a state instance
 *
 * @state	The state instance to work on
 * @path	The path where the state will be stored to
 */
int state_backend_dtb_file(struct state *state, const char *of_path, const char *path)
{
	struct state_backend_dtb *backend_dtb;
	struct state_backend *backend;
	struct mtd_info_user meminfo;
	int ret;

	if (state->backend)
		return -EBUSY;

	backend_dtb = xzalloc(sizeof(*backend_dtb));
	backend = &backend_dtb->backend;

	backend->load = state_backend_dtb_load;
	backend->save = state_backend_dtb_save;
	backend->of_path = xstrdup(of_path);
	backend->path = xstrdup(path);
	backend->name = "dtb";

	state->backend = backend;

	ret = mtd_get_meminfo(backend->path, &meminfo);
	if (!ret && !(meminfo.flags & MTD_NO_ERASE))
		backend_dtb->need_erase = true;

	return 0;
}

/*
 * Raw backend implementation
 */
struct state_backend_raw {
	struct state_backend backend;
	unsigned long size_data; /* The raw data size (without header) */
	unsigned long size_full; /* The size header + raw data + hmac */
	unsigned long stride; /* The stride size in bytes of the copies */
	off_t offset; /* offset in the storage file */
	size_t size; /* size of the storage area */
	int num_copy_read; /* The first successfully read copy */
	bool need_erase;
};

struct backend_raw_header {
	uint32_t magic;
	uint16_t reserved;
	uint16_t data_len;
	uint32_t data_crc;
	uint32_t header_crc;
};

static int backend_raw_load_one(struct state_backend_raw *backend_raw,
		struct state *state, int fd, off_t offset)
{
	uint32_t crc;
	struct state_variable *sv;
	struct backend_raw_header header = {};
	unsigned long max_len;
	int d_len = 0;
	int ret;
	void *buf, *data, *hmac;

	max_len = backend_raw->stride;

	ret = lseek(fd, offset, SEEK_SET);
	if (ret < 0)
		return ret;

	ret = read_full(fd, &header, sizeof(header));
	max_len -= sizeof(header);
	if (ret < 0) {
		dev_err(&state->dev,
			"cannot read header from backend device");
		return ret;
	}

	crc = crc32(0, &header, sizeof(header) - sizeof(uint32_t));
	if (crc != header.header_crc) {
		dev_err(&state->dev,
			"invalid header crc, calculated 0x%08x, found 0x%08x\n",
			crc, header.header_crc);
		return -EINVAL;
	}

	if (state->magic && state->magic != header.magic) {
		dev_err(&state->dev,
			"invalid magic 0x%08x, should be 0x%08x\n",
			header.magic, state->magic);
		return -EINVAL;
	}

	if (backend_raw->backend.digest) {
		d_len = digest_length(backend_raw->backend.digest);
		max_len -= d_len;
	}

	if (header.data_len > max_len) {
		dev_err(&state->dev,
			"invalid data_len %u in header, max is %lu\n",
			header.data_len, max_len);
		return -EINVAL;
	}

	buf = xzalloc(sizeof(header) + header.data_len + d_len);
	data = buf + sizeof(header);
	hmac = data + header.data_len;

	ret = lseek(fd, offset, SEEK_SET);
	if (ret < 0)
		goto out_free;

	ret = read_full(fd, buf, sizeof(header) + header.data_len + d_len);
	if (ret < 0)
		goto out_free;

	crc = crc32(0, data, header.data_len);
	if (crc != header.data_crc) {
		dev_err(&state->dev,
			"invalid crc, calculated 0x%08x, found 0x%08x\n",
			crc, header.data_crc);
		ret = -EINVAL;
		goto out_free;
	}

	if (backend_raw->backend.digest) {
		struct digest *d = backend_raw->backend.digest;

		ret = digest_init(d);
		if (ret)
			goto out_free;

		/* hmac over header and data */
		ret = digest_update(d, buf, sizeof(header) + header.data_len);
		if (ret)
			goto out_free;

		ret = digest_verify(d, hmac);
		if (ret < 0)
			goto out_free;
	}

	list_for_each_entry(sv, &state->variables, list) {
		if (sv->start + sv->size > header.data_len)
			break;
		memcpy(sv->raw, data + sv->start, sv->size);
	}

	free(buf);
	return 0;

 out_free:
	free(buf);
	return ret;
}

static int state_backend_raw_load(struct state_backend *backend,
				  struct state *state)
{
	struct state_backend_raw *backend_raw = container_of(backend,
			struct state_backend_raw, backend);
	int ret = 0, fd, i;

	fd = open(backend->path, O_RDONLY);
	if (fd < 0) {
		dev_err(&state->dev, "cannot open %s\n", backend->path);
		return fd;
	}

	for (i = 0; i < RAW_BACKEND_COPIES; i++) {
		off_t offset = backend_raw->offset + i * backend_raw->stride;

		ret = backend_raw_load_one(backend_raw, state, fd, offset);
		if (!ret) {
			backend_raw->num_copy_read = i;
			dev_dbg(&state->dev,
				"copy %d successfully loaded\n", i);
			break;
		}
	}

	close(fd);

	return ret;
}

static int backend_raw_save_one(struct state_backend_raw *backend_raw,
		struct state *state, int fd, int num, void *buf, size_t size)
{
	int ret;
	off_t offset = backend_raw->offset + num * backend_raw->stride;

	dev_dbg(&state->dev, "%s: 0x%08lx 0x%08zx\n",
			__func__, offset, size);

	ret = lseek(fd, offset, SEEK_SET);
	if (ret < 0)
		return ret;

	if (backend_raw->need_erase) {
		ret = erase(fd, backend_raw->stride, offset);
		if (ret)
			return ret;
	}

	ret = write_full(fd, buf, size);
	if (ret < 0)
		return ret;

	return 0;
}

static int state_backend_raw_save(struct state_backend *backend,
				  struct state *state)
{
	struct state_backend_raw *backend_raw = container_of(backend,
			struct state_backend_raw, backend);
	int ret = 0, fd, i;
	void *buf, *data, *hmac;
	struct backend_raw_header *header;
	struct state_variable *sv;

	buf = xzalloc(backend_raw->size_full);

	header = buf;
	data = buf + sizeof(*header);
	hmac = data + backend_raw->size_data;

	list_for_each_entry(sv, &state->variables, list)
		memcpy(data + sv->start, sv->raw, sv->size);

	header->magic = state->magic;
	header->data_len = backend_raw->size_data;
	header->data_crc = crc32(0, data, backend_raw->size_data);
	header->header_crc = crc32(0, header,
				   sizeof(*header) - sizeof(uint32_t));

	if (backend_raw->backend.digest) {
		struct digest *d = backend_raw->backend.digest;

		ret = digest_init(d);
		if (ret)
			goto out_free;

		/* hmac over header and data */
		ret = digest_update(d, buf, sizeof(*header) + backend_raw->size_data);
		if (ret)
			goto out_free;

		ret = digest_final(d, hmac);
		if (ret < 0)
			goto out_free;
	}

	fd = open(backend->path, O_WRONLY);
	if (fd < 0)
		goto out_free;

	/* save other slots first */
	for (i = 0; i < RAW_BACKEND_COPIES; i++) {
		if (i == backend_raw->num_copy_read)
			continue;

		ret = backend_raw_save_one(backend_raw, state, fd,
					   i, buf, backend_raw->size_full);
		if (ret)
			goto out_close;

	}

	ret = backend_raw_save_one(backend_raw, state, fd,
				   backend_raw->num_copy_read, buf, backend_raw->size_full);
	if (ret)
		goto out_close;

	dev_dbg(&state->dev, "wrote state to %s\n", backend->path);
out_close:
	close(fd);
out_free:
	free(buf);

	return ret;
}

#ifdef __BAREBOX__
#define STAT_GIVES_SIZE(s) (S_ISREG(s.st_mode) || S_ISCHR(s.st_mode))
#define BLKGET_GIVES_SIZE(s) 0
#ifndef BLKGETSIZE64
#define BLKGETSIZE64 -1
#endif
#else
#define STAT_GIVES_SIZE(s) (S_ISREG(s.st_mode))
#define BLKGET_GIVES_SIZE(s) (S_ISBLK(s.st_mode))
#endif

static int state_backend_raw_file_get_size(const char *path, size_t *out_size)
{
	struct mtd_info_user meminfo;
	struct stat s;
	int ret;

	ret = stat(path, &s);
	if (ret)
		return -errno;

	/*
	 * under Linux, stat() gives the size only on regular files
	 * under barebox, it works on char dev, too
	 */
	if (STAT_GIVES_SIZE(s)) {
		*out_size = s.st_size;
		return 0;
	}

	/* this works under Linux on block devs */
	if (BLKGET_GIVES_SIZE(s)) {
		int fd;

		fd = open(path, O_RDONLY);
		if (fd < 0)
			return -errno;

		ret = ioctl(fd, BLKGETSIZE64, out_size);
		close(fd);
		if (!ret)
			return 0;
	}

	/* try mtd next */
	ret = mtd_get_meminfo(path, &meminfo);
	if (!ret) {
		*out_size = meminfo.size;
		return 0;
	}

	return ret;
}

static int state_backend_raw_file_init_digest(struct state *state, struct state_backend_raw *backend_raw)
{
	struct digest *digest;
	const char *algo;
	const unsigned char *key;
	int key_len, ret;

	ret = of_property_read_string(state->root, "algo", &algo);
	if (ret == -EINVAL)	/* -EINVAL == does not exist */
		return 0;
	else if (ret)
		return ret;

	ret = keystore_get_secret(state->name, &key, &key_len);
	if (ret)
		return ret;

	digest = digest_alloc(algo);
	if (!digest) {
		dev_err(&state->dev, "unsupported algo %s\n", algo);
		return -EINVAL;
	}

	ret = digest_set_key(digest, key, key_len);
	if (ret) {
		digest_free(digest);
		return ret;
	}

	backend_raw->backend.digest = digest;
	backend_raw->size_full += digest_length(digest);

	return 0;
}

/*
 * state_backend_raw_file - create a raw file backend store for a state instance
 *
 * @state	The state instance to work on
 * @path	The path where the state will be stored to
 * @offset	The offset in the storage file
 * @size	The maximum size to use in the storage file
 *
 * This backend stores raw binary data from a state instance. The
 * binary data is protected with a magic value which has to match and
 * a crc32 that must be valid.  Two copies are stored, sufficient
 * space must be available.

 * @path can be a path to a device or a regular file. When it's a
 * device @size may be 0. The two copies are spread to different
 * eraseblocks if approriate for this device.
 */
int state_backend_raw_file(struct state *state, const char *of_path,
		const char *path, off_t offset, size_t size)
{
	struct state_backend_raw *backend_raw;
	struct state_backend *backend;
	struct state_variable *sv;
	struct mtd_info_user meminfo;
	size_t path_size = 0;
	int ret;

	if (state->backend)
		return -EBUSY;

	ret = state_backend_raw_file_get_size(path, &path_size);
	if (ret)
		return ret;

	if (size == 0)
		size = path_size;
	else if (offset + size > path_size)
		return -EINVAL;

	backend_raw = xzalloc(sizeof(*backend_raw));
	backend = &backend_raw->backend;

	backend->load = state_backend_raw_load;
	backend->save = state_backend_raw_save;
	backend->of_path = xstrdup(of_path);
	backend->path = xstrdup(path);
	backend->name = "raw";

	sv = list_last_entry(&state->variables, struct state_variable, list);
	backend_raw->size_data = sv->start + sv->size;
	backend_raw->offset = offset;
	backend_raw->size = size;
	backend_raw->size_full = backend_raw->size_data +
		sizeof(struct backend_raw_header);

	state->backend = backend;

	ret = state_backend_raw_file_init_digest(state, backend_raw);
	if (ret)
		return ret;

	ret = mtd_get_meminfo(backend->path, &meminfo);
	if (!ret && !(meminfo.flags & MTD_NO_ERASE)) {
		backend_raw->need_erase = true;
		backend_raw->size_full = ALIGN(backend_raw->size_full,
					       meminfo.writesize);
		backend_raw->stride = ALIGN(backend_raw->size_full,
					    meminfo.erasesize);
		dev_dbg(&state->dev, "is a mtd, adjust stepsize to %ld\n",
			backend_raw->stride);
	} else {
		backend_raw->stride = backend_raw->size_full;
	}

	if (backend_raw->size / backend_raw->stride < RAW_BACKEND_COPIES) {
		dev_err(&state->dev, "not enough space for two copies (%lu each)\n",
			backend_raw->stride);
		ret = -ENOSPC;
		goto err;
	}

	return 0;
err:
	if (backend_raw->backend.digest)
		digest_free(backend_raw->backend.digest);

	free(backend_raw);
	return ret;
}

/* ------------------------------------------------------------ */

#undef asprintf

static struct variable_type *state_find_type(enum state_variable_type type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(types); i++) {
		if (type == types[i].type) {
			return &types[i];
		}
	}

	return NULL;
}

static int __state_uint32_set(struct state_variable *var, const char *val)
{
	struct state_uint32 *su32 = to_state_uint32(var);

	su32->value = strtoul(val, NULL, 0);

	return 0;
}

static int __state_uint8_set(struct state_variable *var, const char *val)
{
	struct state_uint32 *su32 = to_state_uint32(var);
	unsigned long num;

	num = strtoul(val, NULL, 0);
	if (num > UINT8_MAX)
		return -ERANGE;

	su32->value = num;

	return 0;
}

static char *__state_uint32_get(struct state_variable *var)
{
	struct state_uint32 *su32 = to_state_uint32(var);
	char *str;
	int ret;

	ret = asprintf(&str, "%u", su32->value);
	if (ret < 0)
		return ERR_PTR(-ENOMEM);

	return str;
}


static int __state_enum32_set(struct state_variable *sv, const char *val)
{
	struct state_enum32 *enum32 = to_state_enum32(sv);
	int i;

	for (i = 0; i < enum32->num_names; i++) {
		if (!strcmp(enum32->names[i], val)) {
			enum32->value = i;
			return 0;
		}
	}

	return -EINVAL;
}

static char *__state_enum32_get(struct state_variable *var)
{
	struct state_enum32 *enum32 = to_state_enum32(var);
	char *str;
	int ret;

	ret = asprintf(&str, "%s", enum32->names[enum32->value]);
	if (ret < 0)
		return ERR_PTR(-ENOMEM);

	return str;
}

static void __state_enum32_info(struct state_variable *var)
{
	struct state_enum32 *enum32 = to_state_enum32(var);
	int i;

	printf(", values=[");

	for (i = 0; i < enum32->num_names; i++)
		printf("%s%s", enum32->names[i],
				i == enum32->num_names - 1 ? "" : ",");
	printf("]");
}

static int string_to_ethaddr(const char *str, uint8_t enetaddr[6])
{
	int reg;
	char *e;

	if (!str || strlen(str) != 17) {
		memset(enetaddr, 0, 6);
		return -EINVAL;
	}

	if (str[2] != ':' || str[5] != ':' || str[8] != ':' ||
			str[11] != ':' || str[14] != ':')
		return -EINVAL;

	for (reg = 0; reg < 6; ++reg) {
		enetaddr[reg] = strtoul(str, &e, 16);
		str = e + 1;
	}

	return 0;
}

static int __state_mac_set(struct state_variable *var, const char *val)
{
	struct state_mac *mac = to_state_mac(var);
	uint8_t mac_save[6];
	int ret;

	ret = string_to_ethaddr(val, mac_save);
	if (ret)
		return ret;

	memcpy(mac->value, mac_save, ARRAY_SIZE(mac_save));

	return 0;
}

static char *__state_mac_get(struct state_variable *var)
{
	struct state_mac *mac = to_state_mac(var);
	char *str;
	int ret;

	ret = asprintf(&str, "%02x:%02x:%02x:%02x:%02x:%02x",
			mac->value[0], mac->value[1], mac->value[2],
			mac->value[3], mac->value[4], mac->value[5]);
	if (ret < 0)
		return ERR_PTR(-ENOMEM);

	return str;
}

static int __state_string_set(struct state_variable *sv, const char *val)
{
	struct state_string *string = to_state_string(sv);
	int ret;

	ret = state_string_copy_to_raw(string, val);
	if (ret)
		return ret;
	free(string->value);
	string->value = xstrdup(val);

	return 0;
}

static char *__state_string_get(struct state_variable *var)
{
	struct state_string *string = to_state_string(var);
	char *str;
	int ret;

	if (string->raw[0])
		str = strndup(string->raw, string->var.size);
	else
		str = strdup("");

	if (!str)
		return ERR_PTR(-ENOMEM);

	return str;
}

static char *state_get_var(struct state *state, const char *var)
{
	struct state_variable *sv;
	struct variable_type *vtype;

	sv = state_find_var(state, var);
	if (IS_ERR(sv))
		return NULL;

	vtype = state_find_type(sv->type);

	return vtype->get(sv);
}

static int state_set_var(struct state *state, const char *var, const char *val)
{
	struct state_variable *sv;
	struct variable_type *vtype;
	int ret;

	sv = state_find_var(state, var);
	if (IS_ERR(sv))
		return PTR_ERR(sv);

	vtype = state_find_type(sv->type);

	if (!vtype->set)
		return -EPERM;

	ret = vtype->set(sv, val);
	if (ret)
		return ret;

	state->dirty = 1;

	return 0;
}


static struct state *state_get(const char *name)
{
	struct device_node *root, *node, *partition_node;
	char *path;
	struct state *state;
	int ret;
	const char *backend_type = NULL;
	struct state_variable *v;
	char *devpath;
	off_t offset;
	size_t size;

	root = of_read_proc_devicetree();
	if (IS_ERR(root)) {
		fprintf(stderr, "Unable to read devicetree. %s\n",
				strerror(-PTR_ERR(root)));
		return ERR_CAST(root);
	}

	of_set_root_node(root);

	node = of_find_node_by_path_or_alias(root, name);
	if (!node) {
		fprintf(stderr, "no such node: %s\n", name);
		return ERR_PTR(-ENOENT);
	}

	if (verbose > 1) {
		printf("found state node %s:\n", node->full_name);
		of_print_nodes(node, 0);
	}

	state = state_new_from_node(node->name, node);
	if (IS_ERR(state)) {
		fprintf(stderr, "unable to initlialize state: %s\n",
				strerror(PTR_ERR(state)));
		return ERR_CAST(state);
	}

	partition_node = of_parse_phandle(node, "backend", 0);
	if (!partition_node) {
		fprintf(stderr, "cannot find backend node in %s\n", node->full_name);
		exit (1);
	}

	ret = of_get_devicepath(partition_node, &devpath, &offset, &size);
	if (ret) {
		fprintf(stderr, "Cannot find backend path in %s\n", node->full_name);
		return ERR_PTR(ret);
	}

	of_property_read_string(node, "backend-type", &backend_type);
	if (!strcmp(backend_type, "raw"))
		ret = state_backend_raw_file(state, partition_node->full_name, devpath, offset, size);
	else
                fprintf(stderr, "invalid backend type: %s\n", backend_type);

	if (ret) {
		fprintf(stderr, "Cannot initialize backend: %s\n", strerror(-ret));
		return ERR_PTR(ret);
	}

	return state;
}

enum opt {
	OPT_DUMP_SHELL = UCHAR_MAX + 1,
};

static struct option long_options[] = {
	{"get",		required_argument,	0,	'g' },
	{"set",		required_argument,	0,	's' },
	{"name",	required_argument,	0,	'n' },
	{"dump",	no_argument,		0,	'd' },
	{"dump-shell",	no_argument,		0,	OPT_DUMP_SHELL },
	{"init",	no_argument,		0,	'i' },
	{"verbose",	no_argument,		0,	'v' },
	{"help",	no_argument,		0,	'h' },
};

static void usage(char *name)
{
	printf(
"Usage: %s [OPTIONS]\n"
"\n"
"-g, --get <variable>                      get the value of a variable\n"
"-s, --set <variable>=<value>              set the value of a variable\n"
"-n, --name <name>                         specify the state to use (default=\"state\")\n"
"-d, --dump                                dump the state\n"
"--dump-shell                              dump the state suitable for shell sourcing\n"
"-i, --init                                initialize the state (do not load from storage)\n"
"-v, --verbose                             increase verbosity\n"
"--help                                    this help\n",
	name);
}

#define state_for_each_var(state, var) \
	list_for_each_entry(var, &(state)->variables, list)

struct state_set_get {
	char *arg;
	int get;
	struct list_head list;
};

int main(int argc, char *argv[])
{
	struct state *state;
	struct state_variable *v;
	int ret, c, option_index;
	int do_dump = 0, do_dump_shell = 0, do_initialize = 0;
	struct state_set_get *sg;
	struct list_head sg_list;
	char *statename = "/state";

	INIT_LIST_HEAD(&sg_list);

	while (1) {
		c = getopt_long(argc, argv, "hg:s:divn:", long_options, &option_index);
		if (c < 0)
			break;
		switch (c) {
		case 'h':
			usage(argv[0]);
			exit(0);
		case 'g':
			sg = xzalloc(sizeof(*sg));
			sg->get = 1;
			sg->arg = optarg;
			list_add_tail(&sg->list, &sg_list);
			break;
		case 's':
			sg = xzalloc(sizeof(*sg));
			sg->get = 0;
			sg->arg = optarg;
			list_add_tail(&sg->list, &sg_list);
			break;
		case 'd':
			do_dump = 1;
			break;
		case 'i':
			do_initialize = 1;
			break;
		case OPT_DUMP_SHELL:
			do_dump_shell = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 'n':
			statename = optarg;
		}
	}

	state = state_get(statename);
	if (IS_ERR(state))
		exit(1);

	if (!do_initialize) {
		ret = state_load(state);
		if (ret) {
			fprintf(stderr, "Cannot load state: %s\n", strerror(-ret));
			exit(1);
		}
	}

	if (do_dump) {
		state_for_each_var(state, v) {
			struct variable_type *vtype;
			vtype = state_find_type(v->type);
			printf("%s=%s", v->name, vtype->get(v));
			if (verbose) {
				printf(", type=%s", vtype->type_name);
				if (vtype->info)
					vtype->info(v);
			}
			printf("\n");
		}
	}

	if (do_dump_shell) {
		state_for_each_var(state, v) {
			struct variable_type *vtype;
			char *name, *ptr;
			int i;

			/* replace "." by "_" to make it var name shell compatible */
			name = strdup(v->name);
			ptr = name;
			while ((ptr = strchr(ptr, '.')))
				*ptr++ = '_';

			vtype = state_find_type(v->type);
			printf("%s_%s=\"%s\"\n", state->name, name, vtype->get(v));
		}
	}

	list_for_each_entry(sg, &sg_list, list) {
		if (sg->get) {
			char *val = state_get_var(state, sg->arg);
			if (!val) {
				fprintf(stderr, "no such variable: %s\n", sg->arg);
				exit (1);
			}

			printf("%s\n", val);
		} else {
			char *var, *val;

			var = sg->arg;
			val = index(sg->arg, '=');
			if (!val) {
				fprintf(stderr, "usage: -s var=val\n");
				exit (1);
			}
			*val++ = '\0';
			ret = state_set_var(state, var, val);
			if (ret) {
				fprintf(stderr, "Failed to set variable %s to %s: %s\n",
						var, val, strerror(-ret));
				exit(1);
			}
		}
	}

	if (do_initialize || state->dirty) {
		ret = state_save(state);
		if (ret) {
			fprintf(stderr, "Failed to save state: %s\n", strerror(-ret));
			exit(1);
		}
	}

	return 0;
}
