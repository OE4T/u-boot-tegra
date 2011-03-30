/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <common.h>
#include <config.h>
#include <i2c.h>
#include "slb9635_i2c/ifx_auto.h"

/* api function pointer for different version chip */
static struct {
	int (*open)(void);
	int (*close)(void);
	int (*sendrecv)(const uint8_t *sendbuf, size_t sbuf_size,
		uint8_t *recvbuf, size_t *rbuf_len);
} _tpm_instance = {0};


int tis_init(void)
{
#ifdef CONFIG_TPM_SLB9635_I2C_V03
	/* prototype version chip detection */
	if (tpm_init_v03() == 0) {
		printf("I2C addr(x1A) : v03 prototype\n");
		_tpm_instance.open     = tpm_open_v03;
		_tpm_instance.close    = tpm_close_v03;
		_tpm_instance.sendrecv = tpm_sendrecv_v03;
		return 0;
	}
#endif

#ifdef CONFIG_TPM_SLB9635_I2C
	/* firmware virsion > v05 : production */
	if (tpm_init_v05() == 0) {
		printf("I2C addr(x20) : v05 engineering/production\n");
		_tpm_instance.open     = tpm_open_v05;
		_tpm_instance.close    = tpm_close_v05;
		_tpm_instance.sendrecv = tpm_sendrecv_v05;
		return 0;
	}
#endif
	_tpm_instance.open     = NULL;
	_tpm_instance.close    = NULL;
	_tpm_instance.sendrecv = NULL;
	return -1;
}

int tis_open(void)
{
	if (_tpm_instance.open)
		return (*_tpm_instance.open)();
	return -1;
}

int tis_close(void)
{
	if (_tpm_instance.close)
		return (*_tpm_instance.close)();
	return -1;
}

int tis_sendrecv(const uint8_t *sendbuf, size_t sbuf_size,
	uint8_t *recvbuf, size_t *rbuf_len)
{
	if (_tpm_instance.sendrecv)
		return (*_tpm_instance.sendrecv)(sendbuf, sbuf_size, recvbuf,
			rbuf_len);
	return -1;
}

