/*
 *  Copyright (C) 2010-2017 NVIDIA CORPORATION.
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

typedef int iter_envitem(void *blob_dst, char *item, void *param);

static iter_envitem fdt_iter_del_node;
static int fdt_iter_del_node(void *blob_dst, char *node_path, void *unused_param)
{
	int ofs;

	ofs = fdt_path_offset(blob_dst, node_path);
	/* Node doesn't exist -> property can't exist -> it's removed! */
	if (ofs == -FDT_ERR_NOTFOUND)
		return 0;
	if (ofs < 0) {
		error("DT node %s lookup failure; can't del node\n", node_path);
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
		error("Can't del prop %s; missing /", prop_path);
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
			error("DT node %s lookup failure; can't del prop %s\n",
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

	items = getenv(env_varname);
	if (!items) {
		log_debug("%s: No env var %s\n", __func__, env_varname);
		return 0;
	}

	items = strdup(items);
	if (!items) {
		error("strdup(%s) failed", env_varname);
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
