/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <stdlib.h>
#include <common.h>
#include <linux/ctype.h>
#include <fdt_support.h>
#include <fdtdec.h>
#include <asm/arch/tegra.h>

extern unsigned long nvtboot_boot_x0;

char *strstrip(char *s)
{
    size_t size;
    char *end;

    size = strlen(s);

    if (!size)
        return s;

    end = s + size - 1;
    while (end >= s && isblank(*end))
        end--;
    *(end + 1) = '\0';

    while (*s && isblank(*s))
        s++;

    return s;
}

static int set_fdt_addr(void)
{
	int ret;

	ret = setenv_hex("fdt_addr", nvtboot_boot_x0);
	if (ret) {
		printf("Failed to set fdt_addr to point at DTB: %d\n", ret);
		return ret;
	}

	return 0;
}

#if defined(CONFIG_TEGRA186)
/*
 * Attempt to use /chosen/nvidia,ether-mac in the nvtboot DTB to U-Boot's
 * ethaddr environment variable if possible.
 */
static int set_ethaddr_from_nvtboot(void)
{
	const void *nvtboot_blob = (void *)nvtboot_boot_x0;
	int ret, node, len;
	const u32 *prop;

	/* Already a valid address in the environment? If so, keep it */
	if (getenv("ethaddr"))
		return 0;

	node = fdt_path_offset(nvtboot_blob, "/chosen");
	if (node < 0) {
		printf("Can't find /chosen node in nvtboot DTB\n");
		return node;
	}
	prop = fdt_getprop(nvtboot_blob, node, "nvidia,ether-mac", &len);
	if (!prop) {
		printf("Can't find nvidia,ether-mac property in nvtboot DTB\n");
		return -ENOENT;
	}

	ret = setenv("ethaddr", (void *)prop);
	if (ret) {
		printf("Failed to set ethaddr from nvtboot DTB: %d\n", ret);
		return ret;
	}

	return 0;
}
#endif	/* T186 */

#if defined(CONFIG_TEGRA210)
static int parse_bootargs(void)
{
	const void *nvtboot_blob = (void *)nvtboot_boot_x0;
	const void *prop;
	const char *start;
	char *bargs, *s;
	char *target;
	int node, len;

	/*
	 * Set some env vars based on the bootargs passed in
	 * the DTB by the previous bootloader (pointer in reg x0)
	 */

	debug("%s: nvtboot_blob = %p\n", __func__, nvtboot_blob);

	node = fdt_path_offset(nvtboot_blob, "/chosen");
	if (node < 0) {
		error("Can't find /chosen node in nvtboot DTB");
		return node;
	}
	debug("%s: found 'chosen' node: %d\n", __func__, node);

	prop = fdt_getprop(nvtboot_blob, node, "bootargs", &len);
	if (!prop) {
		error("Can't find /chosen/bootargs property in nvtboot DTB");
		return -ENOENT;
	}
	debug("%s: found 'bootargs' property, len =%d\n",  __func__, len);

	bargs = strdup((char *)prop);
	debug("%s: bootargs = %s\n", __func__, bargs);

	/* Get nvdumper_reserved from bootargs */
	target = "nvdumper_reserved";
	start = bargs;

	s = strstr(start, target);
	if (s) {
		debug("%s: Found %s in bootargs.\n", __func__, target);

		/* remove 'nvdumper_reserved=' portion */
		start = strtok(s, "=");
		/* get actual hex address portion */
		start = strtok(NULL, " ");
		debug("%s: %s=%s\n", __func__, target, start);
		setenv("nvdumper_reserved", start);
	} else {				/* s == NULL */
		debug("%s: Failed to find %s in bootargs!\n", __func__, target);
	}

	/* Re-allocate bargs since strtok above has modified it */
	free(bargs);
	bargs = strdup((char *)prop);

	/* Get lp0_vec from bootargs */
	target = "lp0_vec";
	start = bargs;

	s = strstr(start, target);
	if (s) {
		debug("%s: Found %s in bootargs.\n", __func__, target);

		/* remove 'lp0_vec=' portion */
		start = strtok(s, "=");
		/* get actual hex address portion */
		start = strtok(NULL, " ");
		debug("%s: %s=%s\n", __func__, target, start);
		setenv("lp0_vec", start);
	} else {				/* s == NULL */
		debug("%s: Failed to find %s in bootargs!\n", __func__, target);
	}

	free(bargs);
	return 0;
}
#endif	/* T210 */

static int set_cbootargs(void)
{
	const void *nvtboot_blob = (void *)nvtboot_boot_x0;
	const void *prop;
	char *bargs, *s;
	int node, len, ret = 0;

	/*
	 * Save the bootargs passed in the DTB by the previous bootloader
	 * (CBoot) to the env. (pointer in reg x0)
	 */

	debug("%s: nvtboot_blob = %p\n", __func__, nvtboot_blob);

	node = fdt_path_offset(nvtboot_blob, "/chosen");
	if (node < 0) {
		error("Can't find /chosen node in nvtboot DTB");
		return node;
	}
	debug("%s: found 'chosen' node: %d\n", __func__, node);

	prop = fdt_getprop(nvtboot_blob, node, "bootargs", &len);
	if (!prop) {
		error("Can't find /chosen/bootargs property in nvtboot DTB");
		return -ENOENT;
	}
	debug("%s: found 'bootargs' property, len =%d\n",  __func__, len);

	/* CBoot seems to add trailing whitespace - strip it here */
	s = strdup((char *)prop);
	bargs = strstrip(s);
	debug("%s: bootargs = %s!\n", __func__, bargs);

        /* Set cbootargs to env for later use by extlinux files */
	ret = setenv("cbootargs", bargs);
	if (ret)
		printf("Failed to set cbootargs from cboot DTB: %d\n", ret);

	free(s);
	return ret;
}

int nvtboot_init_late(void)
{
	/*
	 * Ignore errors here; the value may not be used depending on
	 * extlinux.conf or boot script content.
	 */
	set_fdt_addr();
#if defined(CONFIG_TEGRA186)
	/* Ignore errors here; not all cases care about Ethernet addresses */
	set_ethaddr_from_nvtboot();
#endif
#if defined(CONFIG_TEGRA210)
	/* Handle lp0_vec and nvdumper env vars */
	parse_bootargs();
#endif
	/* Save CBoot bootargs to env */
	set_cbootargs();

	return 0;
}
