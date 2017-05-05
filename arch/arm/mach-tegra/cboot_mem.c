/*
 * Copyright (c) 2016-2017, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <fdt_support.h>
#include <fdtdec.h>
#include <asm/arch/tegra.h>

DECLARE_GLOBAL_DATA_PTR;

extern unsigned long cboot_boot_x0;

/*
 * A parsed version of /memory/reg from the DTB that is passed to U-Boot in x0.
 *
 * We only support up to two banks since that's all the binary  bootloader
 * ever sets. We assume bank 0 is RAM below 4G and bank 1 is RAM  above 4G.
 * This is all a fairly safe assumption, since the L4T kernel makes  the same
 * assumptions, so the bootloader is unlikely to change.
 *
 * This is written to before relocation, and hence cannot be in .bss, since
 * .bss overlaps the DTB that's appended to the U-Boot binary. The initializer
 * forces this into .data and avoids this issue. This also has the nice side-
 * effect of the content being valid after relocation.
 */
static struct {
	u64 start;
	u64 size;
} ram_banks[CONFIG_NR_DRAM_BANKS] = {{1}};

u64 SZ_4G = 0x100000000;

int dram_init(void)
{
	unsigned int na, ns;
	const void *cboot_blob = (void *)cboot_boot_x0;
	int node, len, i;
	const u32 *prop;

	memset(ram_banks, 0, sizeof(ram_banks));

	na = fdtdec_get_uint(cboot_blob, 0, "#address-cells", 2);
	ns = fdtdec_get_uint(cboot_blob, 0, "#size-cells", 2);

	node = fdt_path_offset(cboot_blob, "/memory");
	if (node < 0) {
		error("Can't find /memory node in cboot DTB");
		hang();
	}
	prop = fdt_getprop(cboot_blob, node, "reg", &len);
	if (!prop) {
		error("Can't find /memory/reg property in cboot DTB");
		hang();
	}

	/* Calculate the true # of base/size pairs to read */
	len /= 4;		/* Convert bytes to number of cells */
	len /= (na + ns);	/* Convert cells to number of banks */

	if (len > ARRAY_SIZE(ram_banks))
		len = ARRAY_SIZE(ram_banks);

	gd->ram_size = 0;
	for (i = 0; i < len; i++) {
		ram_banks[i].start = of_read_number(prop, na);
		prop += na;
		ram_banks[i].size = of_read_number(prop, ns);
		prop += ns;
		gd->ram_size += ram_banks[i].size;

		debug("%s: ram_banks[%d], start = 0x%08lX, size = 0x%08lX\n",
		      __func__, i, (ulong)ram_banks[i].start,
		      (ulong)ram_banks[i].size);

	}

	/* Bank 0 s/b under 4GB, bank 1 above 4GB, other banks don't care */
	if ((ram_banks[0].start + ram_banks[0].size) > SZ_4G)
		debug("%s: Bank 0 size exceeds 32-bits/4GB!\n", __func__);

	return 0;
}


void dram_init_banksize(void)
{
	int i;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		gd->bd->bi_dram[i].start = ram_banks[i].start;
		gd->bd->bi_dram[i].size = ram_banks[i].size;
	}

#ifdef CONFIG_PCI
	gd->pci_ram_top = gd->bd->bi_dram[0].start + gd->bd->bi_dram[0].size;
#endif
}

ulong board_get_usable_ram_top(ulong total_size)
{
	ulong ram_top = ram_banks[0].start + ram_banks[0].size;

	debug("%s: ram_banks[0], start = 0x%08lX, size = 0x%08lX\n", __func__,
	      (ulong)ram_banks[0].start, (ulong)ram_banks[0].size);
	debug("%s: ram_top = 0x%08lX\n", __func__, ram_top);

	/*
	 * Set a < 4GB 'RAM top' if bank[0] exceeds 4GB. Otherwise U-Boot
	 * tries to use or relocate to memory >4GB and will hang.
	 */
	if (ram_top > SZ_4G) {
		debug("%s: Bank 0 size exceeds 32-bits/4GB! adjusting..\n",
		       __func__);
		/* Use 4GB-2M as the RAM top */
		ram_top = ((SZ_4G - 1) & ~(SZ_2M - 1));
	}

	debug("%s: returning ram_top = 0x%08lX\n", __func__, ram_top);
	return ram_top;
}
