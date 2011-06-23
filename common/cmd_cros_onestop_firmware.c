/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <command.h>
#include <lcd.h>
#include <malloc.h>
#include <chromeos/common.h>
#include <chromeos/firmware_storage.h>
#include <chromeos/gbb_bmpblk.h>
#include <chromeos/gpio.h>
#include <chromeos/kernel_shared_data.h>
#include <chromeos/load_kernel_helper.h>
#include <chromeos/os_storage.h>
#include <chromeos/power_management.h>
#include <chromeos/vboot_nvstorage_helper.h>

#include <boot_device.h>
#include <bmpblk_header.h>
#include <load_kernel_fw.h>
#include <tlcl_stub.h>
#include <vboot_nvstorage.h>
#include <vboot_struct.h>

#define PREFIX "cros_onestop_firmware: "

#define WAIT_MS_BETWEEN_PROBING	400
#define WAIT_MS_SHOW_ERROR	2000

/* this value has to be different to all of VBNV_RECOVERY_* */
#define REBOOT_TO_CURRENT_MODE	~0U

enum {
	KEY_BOOT_INTERNAL = 0x04, /* ctrl-d */
	KEY_BOOT_EXTERNAL = 0x15, /* ctrl-u */
	KEY_GOTO_RECOVERY = 0x1b, /* escape */
};

static struct internal_state_t {
	firmware_storage_t file;
	VbNvContext nvcxt;
	VbSharedDataHeader *shared;
	void *gbb_data;
	uint64_t boot_flags;
	uint32_t recovery_request;
	ScreenIndex current_screen;
} _state;

/**
 * This loads VbNvContext to internal state. The caller has to initialize
 * internal mmc device first.
 *
 * @return VBNV_RECOVERY_NOT_REQUESTED if it succeeds; recovery reason if it
 *         fails
 */
static uint32_t init_internal_state_nvcontext(void)
{
	if (read_nvcontext(&_state.nvcxt)) {
		VBDEBUG(PREFIX "fail to read nvcontext\n");
		return VBNV_RECOVERY_RO_UNSPECIFIED;
	}
	VBDEBUG(PREFIX "nvcxt: %s\n", nvcontext_to_str(&_state.nvcxt));

	if (VbNvGet(&_state.nvcxt, VBNV_RECOVERY_REQUEST,
				&_state.recovery_request)) {
		VBDEBUG(PREFIX "fail to read recovery request\n");
		return VBNV_RECOVERY_RO_UNSPECIFIED;
	}
	VBDEBUG(PREFIX "recovery_request = 0x%x\n", _state.recovery_request);

	/*
	 * we have to clean recovery request once we have it so that we will
	 * not be trapped in recovery boot
	 */
	if (VbNvSet(&_state.nvcxt, VBNV_RECOVERY_REQUEST,
				VBNV_RECOVERY_NOT_REQUESTED)) {
		VBDEBUG(PREFIX "fail to clear recovery request\n");
		return VBNV_RECOVERY_RO_UNSPECIFIED;
	}
	if (VbNvTeardown(&_state.nvcxt)) {
		VBDEBUG(PREFIX "fail to tear down nvcontext\n");
		return VBNV_RECOVERY_RO_UNSPECIFIED;
	}
	if (_state.nvcxt.raw_changed && write_nvcontext(&_state.nvcxt)) {
		VBDEBUG(PREFIX "fail to write back nvcontext\n");
		return VBNV_RECOVERY_RO_UNSPECIFIED;
	}

	return VBNV_RECOVERY_NOT_REQUESTED;
}

/**
 * This initializes data blob that firmware shares with kernel. The caller has
 * to initialize:
 *   firmware storage device
 *   gbb
 *   nvcontext
 * before calling this function.
 *
 * @return VBNV_RECOVERY_NOT_REQUESTED if it succeeds; recovery reason if it
 *         fails
 */
static uint32_t init_internal_state_ksd(int write_protect_sw, int recovery_sw,
		int developer_sw)
{
	void *ksd = get_last_1mb_of_ram();
	uint8_t frid[ID_LEN], fmap[CONFIG_LENGTH_FMAP];

	if (firmware_storage_read(&_state.file, CONFIG_OFFSET_RO_FRID,
				CONFIG_LENGTH_RO_FRID, frid)) {
		VBDEBUG(PREFIX "read frid fail\n");
		return VBNV_RECOVERY_RO_SHARED_DATA;
	}

	if (firmware_storage_read(&_state.file,
				CONFIG_OFFSET_FMAP, CONFIG_LENGTH_FMAP, fmap)) {
		VBDEBUG(PREFIX "read fmap fail\n");
		return VBNV_RECOVERY_RO_SHARED_DATA;
	}

	if (cros_ksd_init(ksd, frid, fmap, CONFIG_LENGTH_FMAP,
				_state.gbb_data, CONFIG_LENGTH_GBB,
				&_state.nvcxt,
				write_protect_sw, recovery_sw, developer_sw)) {
		VBDEBUG(PREFIX "init kernel shared data fail\n");
		return VBNV_RECOVERY_RO_UNSPECIFIED;
	}

	return VBNV_RECOVERY_NOT_REQUESTED;
}

/**
 * This initializes global variable [_state] and kernel shared data.
 *
 * @return VBNV_RECOVERY_NOT_REQUESTED if it succeeds; recovery reason if it
 *         fails
 */
static uint32_t init_internal_state(void)
{
	int write_protect_sw, recovery_sw, developer_sw;
	uint32_t reason;

	/* fetch gpios at once */
	write_protect_sw = is_firmware_write_protect_gpio_asserted();
	recovery_sw = is_recovery_mode_gpio_asserted();
	developer_sw = is_developer_mode_gpio_asserted();

	memset(&_state, '\0', sizeof(_state));

	/* sad enough, SCREEN_BLANK != 0 */
	_state.current_screen = SCREEN_BLANK;

	/* malloc spaces for buffers */
	_state.gbb_data = malloc(CONFIG_LENGTH_GBB);
	if (!_state.gbb_data) {
		VBDEBUG(PREFIX "malloc gbb_data == NULL\n");
		return VBNV_RECOVERY_RO_UNSPECIFIED;
	}
	_state.shared = malloc(VB_SHARED_DATA_REC_SIZE);
	if (!_state.shared) {
		VBDEBUG(PREFIX "malloc shared == NULL\n");
		return VBNV_RECOVERY_RO_UNSPECIFIED;
	}

	/* open firmware storage device and load gbb */
	if (firmware_storage_open_spi(&_state.file)) {
		VBDEBUG(PREFIX "open_spi fail\n");
		return VBNV_RECOVERY_RO_SHARED_DATA;
	}
	if (firmware_storage_read(&_state.file,
				CONFIG_OFFSET_GBB, CONFIG_LENGTH_GBB,
				_state.gbb_data)) {
		VBDEBUG(PREFIX "fail to read gbb\n");
		return VBNV_RECOVERY_RO_SHARED_DATA;
	}

	/* initialize mmc and load nvcontext */
	if (initialize_mmc_device(MMC_INTERNAL_DEVICE)) {
		VBDEBUG(PREFIX "mmc %d init fail\n", MMC_INTERNAL_DEVICE);
		return VBNV_RECOVERY_RO_UNSPECIFIED;
	}
	reason = init_internal_state_nvcontext();
	if (reason != VBNV_RECOVERY_NOT_REQUESTED) {
		VBDEBUG(PREFIX "fail to load nvcontext\n");
		return reason;
	}

	reason = init_internal_state_ksd(write_protect_sw, recovery_sw,
				developer_sw);
	if (reason != VBNV_RECOVERY_NOT_REQUESTED) {
		VBDEBUG(PREFIX "init ksd fail\n");
		return reason;
	}

	_state.boot_flags = 0;
	if (developer_sw) {
		_state.boot_flags |= BOOT_FLAG_DEVELOPER;
		_state.boot_flags |= BOOT_FLAG_DEV_FIRMWARE;
	}
	if (recovery_sw)
		_state.boot_flags |= BOOT_FLAG_RECOVERY;
	VBDEBUG(PREFIX "boot_flags = 0x%llx\n", _state.boot_flags);

	return VBNV_RECOVERY_NOT_REQUESTED;
}

static uint32_t get_recovery_request(void)
{
	if (_state.boot_flags & BOOT_FLAG_RECOVERY)
		return VBNV_RECOVERY_RO_MANUAL;
	return _state.recovery_request;
}

static int is_developer_boot_requested(void)
{
	return (_state.boot_flags & BOOT_FLAG_DEVELOPER);
}

/* forward declare a (private) struct of vboot_reference */
struct RSAPublicKey;

/*
 * We copy declaration of private functions of vboot_reference here so that we
 * can initialize VbSharedData without actually calling to LoadFirmware.
 */
int VbSharedDataInit(VbSharedDataHeader *header, uint64_t size);
int VbSharedDataSetKernelKey(VbSharedDataHeader *header,
		const VbPublicKey *src);
int VerifyFirmwarePreamble(const VbFirmwarePreambleHeader *preamble,
		uint64_t size, const struct RSAPublicKey *key);
struct RSAPublicKey *PublicKeyToRSA(const VbPublicKey *key);

/**
 * This loads kernel subkey from rewritable preamble A.
 *
 * @param vblock - a buffer large enough for holding verification block A
 * @return VBNV_RECOVERY_NOT_REQUESTED if it succeeds; recovery reason if it
 *         fails
 */
static uint32_t load_kernel_subkey_a(void *vblock)
{
	VbKeyBlockHeader *key_block;
	VbFirmwarePreambleHeader *preamble;
	struct RSAPublicKey *data_key;

	if (firmware_storage_read(&_state.file,
				CONFIG_OFFSET_VBLOCK_A, CONFIG_LENGTH_VBLOCK_A,
				vblock)) {
		VBDEBUG(PREFIX "read verification block fail\n");
		return VBNV_RECOVERY_RO_SHARED_DATA;
	}

	key_block = (VbKeyBlockHeader *)vblock;

	data_key = PublicKeyToRSA(&key_block->data_key);
	if (!data_key) {
		VBDEBUG(PREFIX "parse data key fail\n");
		return VBNV_RECOVERY_RO_SHARED_DATA;
	}

	preamble = (VbFirmwarePreambleHeader *)(vblock +
			key_block->key_block_size);
	if (VerifyFirmwarePreamble(preamble, CONFIG_LENGTH_VBLOCK_A -
				key_block->key_block_size, data_key)) {
		VBDEBUG(PREFIX "verify preamble fail\n");
		return VBNV_RECOVERY_RO_INVALID_RW;
	}

	if (VbSharedDataSetKernelKey(_state.shared, &preamble->kernel_subkey)) {
		VBDEBUG(PREFIX "unable to save kernel subkey\n");
		return VBNV_RECOVERY_RO_INVALID_RW;
	}

	return VBNV_RECOVERY_NOT_REQUESTED;
}

/**
 * This initializes VbSharedData that is used in normal and developer boot.
 *
 * @return VBNV_RECOVERY_NOT_REQUESTED if it succeeds; recovery reason if it
 *         fails
 */
static uint32_t init_vbshared_data(void)
{
	/*
	 * This function is adapted from LoadFirmware(). The differences between
	 * the two include:
	 *
	 *   We always pretend that we "boot" from firmware A, assuming that
	 *   you have the right kernel subkey there. We also fill in respective
	 *   fields with dummy values.
	 *
	 *   We set zero to these fields of VbSharedDataHeader
	 *     fw_version_tpm_start
	 *     fw_version_lowest
	 *     fw_version_tpm
	 *
	 *    We do not call VbGetTimer() because we have conflicting symbols
	 *    when include utility.h.
	 */

	uint32_t reason;
	uint8_t *vblock;

	if (VbSharedDataInit(_state.shared, VB_SHARED_DATA_REC_SIZE)) {
		VBDEBUG(PREFIX "VbSharedDataInit fail\n");
		return VBNV_RECOVERY_RO_SHARED_DATA;
	}

	if (is_developer_boot_requested())
		_state.shared->flags |= VBSD_LF_DEV_SWITCH_ON;

	VbNvSet(&_state.nvcxt, VBNV_TRY_B_COUNT, 0);
	_state.shared->check_fw_a_result = VBSD_LF_CHECK_VALID;
	_state.shared->firmware_index = 0;

	vblock = malloc(CONFIG_LENGTH_VBLOCK_A);
	if (!vblock) {
		VBDEBUG(PREFIX "vblock == NULL\n");
		return VBNV_RECOVERY_RO_UNSPECIFIED;
	}
	reason = load_kernel_subkey_a(vblock);
	if (reason != VBNV_RECOVERY_NOT_REQUESTED)
		VBDEBUG(PREFIX "fail to load kernel subkey A\n");

	free(vblock);
	return reason;
}

static void beep(void)
{
	/* TODO: implement beep */
	VBDEBUG(PREFIX "beep\n");
}

static void clear_screen(void)
{
	if (lcd_clear())
		VBDEBUG(PREFIX "fail to clear screen\n");
	else
		_state.current_screen = SCREEN_BLANK;
}

static void show_screen(ScreenIndex screen)
{
	VBDEBUG(PREFIX "show_screen: %d\n", (int) screen);

	if (_state.current_screen == screen)
		return;

	if (display_screen_in_bmpblk(_state.gbb_data, screen))
		VBDEBUG(PREFIX "fail to display screen\n");
	else
		_state.current_screen = screen;
}

/**
 * This is a boot_kernel wrapper function. It only returns to its caller when
 * it fails to boot kernel, as boot_kernel does.
 *
 * @return recvoery reason or REBOOT_TO_CURRENT_MODE
 */
static uint32_t boot_kernel_helper(void)
{
	int status;

	cros_ksd_dump(get_last_1mb_of_ram());

	status = boot_kernel(_state.boot_flags,
			_state.gbb_data, CONFIG_LENGTH_GBB,
			_state.shared, VB_SHARED_DATA_REC_SIZE,
			&_state.nvcxt);

	switch(status) {
	case LOAD_KERNEL_NOT_FOUND:
		/* no kernel found on device */
		return VBNV_RECOVERY_RW_NO_OS;
	case LOAD_KERNEL_INVALID:
		/* only invalid kernels found on device */
		return VBNV_RECOVERY_RW_INVALID_OS;
	case LOAD_KERNEL_RECOVERY:
		/* internal error; reboot to recovery mode */
		return VBNV_RECOVERY_RW_UNSPECIFIED;
	case LOAD_KERNEL_REBOOT:
		/* internal error; reboot to current mode */
		return REBOOT_TO_CURRENT_MODE;
	}

	return VBNV_RECOVERY_RW_UNSPECIFIED;
}

/**
 * This is the main entry point of recovery boot flow. It never returns to its
 * caller.
 *
 * @param reason - recovery reason
 */
static void recovery_boot(uint32_t reason)
{
	void *ksd = get_last_1mb_of_ram();

	VBDEBUG(PREFIX "recovery boot\n");

	/*
	 * TODO:
	 * 1. write_log()
	 * 2. test_clear_mem_regions()
	 * 3. clear_ram_not_in_use()
	 */

	cros_ksd_set_active_main_firmware(ksd, RECOVERY_FIRMWARE,
			RECOVERY_TYPE);
	cros_ksd_set_recovery_reason(ksd, reason);

	if (!is_developer_boot_requested()) {
		/* wait user unplugging external storage device */
		while (is_any_storage_device_plugged(NOT_BOOT_PROBED_DEVICE)) {
			show_screen(SCREEN_RECOVERY_MODE);
			udelay(WAIT_MS_BETWEEN_PROBING * 1000);
		}
	}

	for (;;) {
		/* Wait for user to plug in SD card or USB storage device */
		while (!is_any_storage_device_plugged(BOOT_PROBED_DEVICE)) {
			show_screen(SCREEN_RECOVERY_MISSING_OS);
			udelay(WAIT_MS_BETWEEN_PROBING * 1000);
		}

		clear_screen();

		/* even if it fails, we simply don't care */
		boot_kernel_helper();

		while (is_any_storage_device_plugged(NOT_BOOT_PROBED_DEVICE)) {
			show_screen(SCREEN_RECOVERY_NO_OS);
			udelay(WAIT_MS_SHOW_ERROR * 1000);
		}
	}

	/* It should never reach here; something terrible happens! */
	cold_reboot();
}

/**
 * This initializes TPM and kernel share data blob (by pretending it boots
 * rewritable firmware A).
 *
 * @return VBNV_RECOVERY_NOT_REQUESTED if it succeeds; recovery reason if it
 *         fails
 */
static uint32_t rewritable_boot_init(void)
{
	void *ksd = get_last_1mb_of_ram();
	uint8_t fwid[ID_LEN];

	/* we pretend we boot from r/w firmware A */
	if (firmware_storage_read(&_state.file, CONFIG_OFFSET_RW_FWID_A,
				CONFIG_LENGTH_RW_FWID_A, fwid)) {
		VBDEBUG(PREFIX "read fwid_a fail\n");
		return VBNV_RECOVERY_RW_SHARED_DATA;
	}

	cros_ksd_set_fwid(ksd, fwid);
	cros_ksd_set_active_main_firmware(ksd, REWRITABLE_FIRMWARE_A,
			is_developer_boot_requested() ?
			DEVELOPER_TYPE : NORMAL_TYPE);

	if (TlclStubInit() != TPM_SUCCESS) {
		VBDEBUG(PREFIX "fail to init tpm\n");
		return VBNV_RECOVERY_RW_TPM_ERROR;
	}

	return VBNV_RECOVERY_NOT_REQUESTED;
}

/**
 * This is the main entry point of developer boot flow.
 *
 * @return VBNV_RECOVERY_NOT_REQUESTED when caller has to boot from internal
 *         storage device; recovery reason when caller has to go to recovery.
 */
static uint32_t developer_boot(void)
{
	ulong start = 0, time = 0, last_time = 0;
	int c, is_after_20_seconds = 0;

	show_screen(SCREEN_DEVELOPER_MODE);

	start = get_timer(0);
	while (1) {
		udelay(100);

		last_time = time;
		time = get_timer(start);
#ifdef DEBUG
		/* only output when time advances at least 1 second */
		if (time / CONFIG_SYS_HZ > last_time / CONFIG_SYS_HZ)
			VBDEBUG(PREFIX "time ~ %d sec\n", time / CONFIG_SYS_HZ);
#endif

		/* beep twice when time > 20 seconds */
		if (!is_after_20_seconds && time > 20 * CONFIG_SYS_HZ) {
			beep(); udelay(500); beep();
			is_after_20_seconds = 1;
			continue;
		}

		/* boot from internal storage after 30 seconds */
		if (time > 30 * CONFIG_SYS_HZ)
			break;

		if (!tstc())
			continue;
		c = getc();
		VBDEBUG(PREFIX "getc() == 0x%x\n", c);

		/* boot from internal storage when user pressed Ctrl-D */
		if (KEY_BOOT_INTERNAL == c)
			break;

		/* load and boot kernel from USB or SD card */
		if (KEY_BOOT_EXTERNAL == c) {
			/* even if boot_kernel_helper fails, we don't care */
			if (is_any_storage_device_plugged(BOOT_PROBED_DEVICE))
				boot_kernel_helper();
			beep();
		}

		if (c == ' ' || c == '\r' || c == '\n' ||
				c == KEY_GOTO_RECOVERY) {
			VBDEBUG(PREFIX "goto recovery\n");
			return VBNV_RECOVERY_RW_DEV_SCREEN;
		}
	}

	return VBNV_RECOVERY_NOT_REQUESTED;
}

/**
 * This is the main entry point of normal boot flow. It never returns
 * VBNV_RECOVERY_NOT_REQUESTED because if it returns, the caller always has to
 * go to recovery (or reboot).
 *
 * @return recovery reason or REBOOT_TO_CURRENT_MODE
 */
static uint32_t normal_boot(void)
{
	VBDEBUG(PREFIX "boot from internal storage\n");

	if (set_bootdev("mmc", MMC_INTERNAL_DEVICE, 0)) {
		VBDEBUG(PREFIX "set_bootdev mmc_internal_device fail\n");
		return VBNV_RECOVERY_RW_NO_OS;
	}

	return boot_kernel_helper();
}

/**
 * This is the main entry point of onestop firmware. If it returns to its
 * caller, the caller should reboot.
 */
static void onestop_boot(void)
{
	uint32_t reason = VBNV_RECOVERY_NOT_REQUESTED;

	/*
	 * At this point, we pretend to be "bootstub firmware" that selects
	 * one of the main firmware (recovery or rewritable A/B).
	 */

	clear_screen();

	reason = init_internal_state();
	if (reason != VBNV_RECOVERY_NOT_REQUESTED) {
		VBDEBUG(PREFIX "fail to init internal state\n");
		recovery_boot(reason);
	}

	reason = get_recovery_request();
	if (reason != VBNV_RECOVERY_NOT_REQUESTED) {
		VBDEBUG(PREFIX "do recovery boot as requested\n");
		recovery_boot(reason);
	}

	/*
	 * At this point, we pretent to select one of rewritable (A versus B)
	 * main firmware. In fact, we always select A here.
	 *
	 * Note: initializing VbShareData is required for developer and normal
	 * boot, but not for recovery boot.
	 */

	reason = init_vbshared_data();
	if (reason != VBNV_RECOVERY_NOT_REQUESTED) {
		VBDEBUG(PREFIX "fail to init VbSharedData\n");
		recovery_boot(reason);
	}

	/*
	 * At this point, we pretend to be "rewritable firmware A". As a
	 * result, all recovery reasons after this point must be one of
	 * VBNV_RECOVERY_RW_*.
	 *
	 * Here we act like a Cr-48 style of rewritable firmware. That is, it
	 * has the capability to do both developer boot and normal boot.
	 */

	reason = rewritable_boot_init();
	if (reason != VBNV_RECOVERY_NOT_REQUESTED) {
		VBDEBUG(PREFIX "fail to pretend booting from r/w fw\n");
		recovery_boot(reason);
	}

	if (is_developer_boot_requested())
		reason = developer_boot();

	/*
	 * If developer boot flow exits normally or is not requested, try
	 * normal boot flow.
	 */
	if (reason == VBNV_RECOVERY_NOT_REQUESTED)
		reason = normal_boot();

	if (reason == REBOOT_TO_CURRENT_MODE)
		cold_reboot();

	/*
	 * If normal boot flow fails, we should reboot to recovery mode. But
	 * here we take a short cut, and just go to recovery boot flow.
	 */
	recovery_boot(reason);
}

int do_cros_onestop_firmware(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	onestop_boot();
	cold_reboot();
	return 0;
}

U_BOOT_CMD(cros_onestop_firmware, 1, 1, do_cros_onestop_firmware,
		"verified boot onestop firmware", NULL);
