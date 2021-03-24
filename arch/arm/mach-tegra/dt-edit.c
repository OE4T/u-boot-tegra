/*
 *  Copyright (C) 2010-2021 NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#define LOG_CATEGORY LOGC_ARCH
#include <stdlib.h>
#include <common.h>
#include <fdt_support.h>
#include <log.h>
#include <fdtdec.h>
#include <env.h>
#include "dt-edit.h"

#define fdt_for_each_property(fdt, prop, parent)		\
	for (prop = fdt_first_property_offset(fdt, parent);	\
	     prop >= 0;						\
	     prop = fdt_next_property_offset(fdt, prop))
typedef int iter_envitem(void *blob_dst, char *item, void *param);

static int fdt_copy_node_content(void *blob_src, int ofs_src, void *blob_dst,
		int ofs_dst, int indent)
{
	int ofs_src_child, ofs_dst_child;

	/*
	 * FIXME: This doesn't remove properties or nodes in the destination
	 * that are not present in the source. For the nodes we care about
	 * right now, this is not an issue.
	 */

	/* Don't copy a node's contents if the phandles don't match */

	debug("%s: Getting phandles ....\n", __func__);

	u32 src_phandle = fdt_get_phandle(blob_src, ofs_src);
	if (!src_phandle) {
		debug("%s: No phandle in this node, copying content ...\n",
		      __func__);
	} else {
		debug("%s: src phandle = %X\n", __func__, src_phandle);

		u32 dst_phandle = fdt_get_phandle(blob_dst, ofs_dst);
		debug("%s: dst phandle = %X\n", __func__, dst_phandle);


		if (src_phandle != dst_phandle) {
			const char *node_name = fdt_get_name(blob_src, ofs_src,
							     NULL);

			printf("WARNING: phandle mismatch was found in node ");
			printf("'%s'\n. It will not be updated in the "
			       "destination DTB.\n", node_name);
			return FDT_ERR_NOTFOUND;	/* skip this node */
		}
	}

	fdt_for_each_property(blob_src, ofs_src_child, ofs_src) {
		const void *prop;
		const char *name;
		int len, ret;

		prop = fdt_getprop_by_offset(blob_src, ofs_src_child, &name,
					     &len);
		log_debug("%s: %*scopy prop: %s\n", __func__, indent, "", name);

		ret = fdt_setprop(blob_dst, ofs_dst, name, prop, len);
		if (ret < 0) {
			pr_err("Can't copy DT prop %s\n", name);
			return ret;
		}
	}

	fdt_for_each_subnode(ofs_src_child, blob_src, ofs_src) {
		const char *name;

		name = fdt_get_name(blob_src, ofs_src_child, NULL);
		log_debug("%s: %*scopy node: %s\n", __func__, indent, "", name);

		ofs_dst_child = fdt_subnode_offset(blob_dst, ofs_dst, name);
		if (ofs_dst_child < 0) {
			log_debug("%s: %*s(creating it in dst)\n", __func__,
				  indent, "");
			ofs_dst_child = fdt_add_subnode(blob_dst, ofs_dst,
							name);
			if (ofs_dst_child < 0) {
				pr_err("Can't copy DT node %s\n", name);
				return ofs_dst_child;
			}
		}

		fdt_copy_node_content(blob_src, ofs_src_child, blob_dst,
				      ofs_dst_child, indent + 2);
	}

	return 0;
}

static int fdt_add_path(void *blob, const char *path)
{
	char *pcopy, *tmp, *node;
	int ofs_parent, ofs_child, ret;

	if (path[0] != '/') {
		pr_err("Can't add path %s; missing leading /", path);
		return -1;
	}
	path++;
	if (!*path) {
		log_debug("%s: path points at DT root!", __func__);
		return 0;
	}

	pcopy = strdup(path);
	if (!pcopy) {
		pr_err("strdup() failed");
		return -1;
	}

	tmp = pcopy;
	ofs_parent = 0;
	while (true) {
		node = strsep(&tmp, "/");
		if (!node)
			break;
		log_debug("%s: node=%s\n", __func__, node);
		ofs_child = fdt_subnode_offset(blob, ofs_parent, node);
		if (ofs_child < 0)
			ofs_child = fdt_add_subnode(blob, ofs_parent, node);
		if (ofs_child < 0) {
			pr_err("Can't create DT node %s\n", node);
			ret = ofs_child;
			goto out;
		}
		ofs_parent = ofs_child;
	}
	ret = ofs_parent;

out:
	free(pcopy);

	return ret;
}

static iter_envitem fdt_iter_copy_prop;
static int fdt_iter_copy_prop(void *blob_dst, char *prop_path, void *blob_src)
{
	char *prop_name, *node_path;
	const void *prop;
	int ofs_src, ofs_dst, len, ret;

	prop_name = strrchr(prop_path, '/');
	if (!prop_name) {
		pr_err("Can't copy prop %s; missing /", prop_path);
		return -1;
	}
	*prop_name = 0;
	prop_name++;
	node_path = prop_path;

	if (*node_path) {
		ofs_src = fdt_path_offset(blob_src, node_path);
		if (ofs_src < 0) {
			pr_err("DT node %s missing in source; can't copy %s\n",
			      node_path, prop_name);
			return -1;
		}

		ofs_dst = fdt_path_offset(blob_dst, node_path);
		if (ofs_src < 0) {
			pr_err("DT node %s missing in dest; can't copy prop %s\n",
			      node_path, prop_name);
			return -1;
		}
	} else {
		ofs_src = 0;
		ofs_dst = 0;
	}

	prop = fdt_getprop(blob_src, ofs_src, prop_name, &len);
	if (!prop) {
		pr_err("DT property %s/%s missing in source; can't copy\n",
		      node_path, prop_name);
		return -1;
	}

	ret = fdt_setprop(blob_dst, ofs_dst, prop_name, prop, len);
	if (ret < 0) {
		pr_err("Can't set DT prop %s/%s\n", node_path, prop_name);
		return ret;
	}

	return 0;
}

static iter_envitem fdt_iter_copy_node;
static int fdt_iter_copy_node(void *blob_dst, char *path, void *blob_src)
{
	int ofs_dst, ofs_src;
	int ret;

	ofs_dst = fdt_add_path(blob_dst, path);
	if (ofs_dst < 0) {
		pr_err("Can't find/create dest DT node %s to copy\n", path);
		return ofs_dst;
	}

	if (!fdtdec_get_is_enabled(blob_dst, ofs_dst)) {
		log_debug("%s: DT node %s disabled in dest; skipping copy\n",
			  __func__, path);
		return 0;
	}

	ofs_src = fdt_path_offset(blob_src, path);
	if (ofs_src < 0) {
		pr_err("DT node %s missing in source; can't copy\n", path);
		return 0;
	}

	ret = fdt_copy_node_content(blob_src, ofs_src, blob_dst,
				    ofs_dst, 2);
	if (ret < 0)
		return ret;

	return 0;
}

static iter_envitem fdt_iter_del_node;
static int fdt_iter_del_node(void *blob_dst, char *node_path, void *unused_param)
{
	int ofs;

	ofs = fdt_path_offset(blob_dst, node_path);
	/* Node doesn't exist -> property can't exist -> it's removed! */
	if (ofs == -FDT_ERR_NOTFOUND)
		return 0;
	if (ofs < 0) {
		pr_err("DT node %s lookup failure; can't del node\n", node_path);
		return ofs;
	}

	return fdt_del_node(blob_dst, ofs);
}

static iter_envitem fdt_iter_del_prop;
static int fdt_iter_del_prop(void *blob_dst, char *prop_path, void *unused_param)
{
	char *prop_name, *node_path;
	int ofs, ret;

	prop_name = strrchr(prop_path, '/');
	if (!prop_name) {
		pr_err("Can't del prop %s; missing /", prop_path);
		return -1;
	}
	*prop_name = 0;
	prop_name++;
	node_path = prop_path;

	if (*node_path) {
		ofs = fdt_path_offset(blob_dst, node_path);
		/* Node doesn't exist -> property can't exist -> it's removed! */
		if (ofs == -FDT_ERR_NOTFOUND)
			return 0;
		if (ofs < 0) {
			pr_err("DT node %s lookup failure; can't del prop %s\n",
			      node_path, prop_name);
			return ofs;
		}
	} else {
		ofs = 0;
	}

	ret = fdt_delprop(blob_dst, ofs, prop_name);
	/* Property doesn't exist -> it's already removed! */
	if (ret == -FDT_ERR_NOTFOUND)
		return 0;
	return ret;
}

static int fdt_iter_envlist(iter_envitem *func, void *blob_dst, const char *env_varname, void *param)
{
	char *items, *tmp, *item;
	int ret;

	items = env_get(env_varname);
	if (!items) {
		log_debug("%s: No env var %s\n", __func__, env_varname);
		return 0;
	}

	items = strdup(items);
	if (!items) {
		pr_err("strdup(%s) failed", env_varname);
		return -1;
	}

	tmp = items;
	while (true) {
		item = strsep(&tmp, ":");
		if (!item)
			break;
		log_debug("%s: item: %s\n", __func__, item);
		ret = func(blob_dst, item, param);
		if (ret < 0) {
			ret = -1;
			goto out;
		}
	}

	ret = 0;

out:
	free(items);

	return ret;
}

__weak void *fdt_copy_get_blob_src_default(void)
{
	return NULL;
}

static void *no_self_copy(void *blob_src, void *blob_dst)
{
	if (blob_src == blob_dst)
		return NULL;
	return blob_src;
}

static void *fdt_get_copy_blob_src(void *blob_dst)
{
	char *src_addr_s;

	src_addr_s = env_get("fdt_copy_src_addr");
	if (!src_addr_s)
		return no_self_copy(fdt_copy_get_blob_src_default(), blob_dst);
	return no_self_copy((void *)simple_strtoul(src_addr_s, NULL, 16),
			    blob_dst);
}

int fdt_copy_env_nodelist(void *blob_dst)
{
	void *blob_src;

	log_debug("%s:\n", __func__);

	blob_src = fdt_get_copy_blob_src(blob_dst);
	if (!blob_src) {
		log_debug("%s: No source DT\n", __func__);
		return 0;
	}

	return fdt_iter_envlist(fdt_iter_copy_node, blob_dst, "fdt_copy_node_paths", blob_src);
}

int fdt_copy_env_proplist(void *blob_dst)
{
	void *blob_src;

	log_debug("%s:\n", __func__);

	blob_src = fdt_get_copy_blob_src(blob_dst);
	if (!blob_src) {
		log_debug("%s: No source DT\n", __func__);
		return 0;
	}

	return fdt_iter_envlist(fdt_iter_copy_prop, blob_dst, "fdt_copy_prop_paths", blob_src);
}

int fdt_del_env_nodelist(void *blob_dst)
{
	log_debug("%s:\n", __func__);

	return fdt_iter_envlist(fdt_iter_del_node, blob_dst, "fdt_del_node_paths", NULL);
}

int fdt_del_env_proplist(void *blob_dst)
{
	log_debug("%s:\n", __func__);

	return fdt_iter_envlist(fdt_iter_del_prop, blob_dst, "fdt_del_prop_paths", NULL);
}
