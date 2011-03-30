/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <config.h>
#include <common.h>
#include <i2c.h>


/* infineon prototype i2c tpm's internal registers
 */
#define INFINEON_TPM_CHIP   (0x1A)
#define REG_RDATA           (0xA4)
#define REG_WDATA           (0xA5)
#define REG_STAT            (0xA7)
/* i2c timeout value and polling period
 */
#define I2CBUSY_WAIT_MS	    (1ul)
#define TIMEOUT_REG_MS      (1000ul)

/* error code follows u-boot standard
 * 0        : success
 * non zero : failed
 */
enum {
	E_TIMEOUT   = -7,
	E_CHECKSUM  = -6,
	E_SIZE      = -5,
	E_MEMORY    = -4,
	E_DATA      = -3,
	E_PARAMETER = -2,
	E_GENERAL   = -1,
	S_OK        = 0,
	E_WARNING   = 1
};

/* tpm chip timeout value on read/write
 * TODO:
 * find the correct timeout when tpm in i2c mode. but since this is the
 * prototype i2c tpm chip, spec is not as clear as lpc tpm chip. just use
 * the maximum wait time as the timeout value.
 */
#define TIMEOUT_READ_MS     (3000ul)
#define TIMEOUT_WRITE_MS    (1000ul)

/* tpm stat register bits
 */
#define TPM_RECEIVE_DATA_AVAILABLE  (1u)
#define TPM_TRANSMIT_FIFO_NOTFULL   (1u << 7)

/* tpm transport and vendor layers
 */
enum {
	TSPT_VER    = 0,
	TSPT_CTRL   = 1,
	TSPT_LEN_H  = 2,
	TSPT_LEN_L  = 3,
	TSPT_CSUM1  = 4,
	VEND_VERS   = 5,
	VEND_CHA    = 6,
	VEND_LEN_0  = 7,
	VEND_LEN_1  = 8,
	VEND_LEN_2  = 9,
	VEND_LEN_3  = 10,
	DATA_START  = 11
};

/* infineon prototype chip specific header
 */
#define VENDOR_HEADER_SIZE        (6)
#define TSPT_HEADER_SIZE          (5)
#define TSPT_TAIL_SIZE            (1)
#define TPM_I2C_HEADER_SIZE       (TSPT_HEADER_SIZE+VENDOR_HEADER_SIZE)
#define PACKED_EXTRA_SIZE         (TPM_I2C_HEADER_SIZE+TSPT_TAIL_SIZE)
#define TPM_TRANSPORT_VER         (0x02)
#define TPM_VENDOR_VERS           (0x01)
#define TPM_VENDOR_CHA_TPM12MSC   (0x0B)
/* infineon prototype chip specific data bit value
 */
#define TPM_TRANSPORT_CTRL_E_BIT  (1u << 5)
#define TPM_TRANSPORT_CTRL_D_BIT  (1u << 2)

/* FIXME: (should modify drivers/i2c/tegra2_i2c.c)
 *    following two functions only exist in i2c_tegra2.c driver
 *    the impl should be what defined in u-boot header file
 *    int i2c_read(chip, addr_ignore, 0, *buffer, length);
 */
extern int i2c_read_data(uchar chip, uchar *buffer, int len);
extern int i2c_write_data(uchar chip, uchar *buffer, int len);

typedef struct s_inf_stat_reg {
	uint8_t status;
	uint8_t len_h;
	uint8_t len_l;
	uint8_t checksum;
} t_inf_stat_reg;


/* TODO: consider implement scatter buffers
 *    it is possible to use scattered buffers for send/recv
 *    but current impl used single buffer on stack
 */
#define HUGE_BUFFER_SIZE 300

#ifdef VBOOT_DEBUG
#define vboot_debug printf
#else
#define vboot_debug debug
#endif /* VBOOT_DEBUG */

#define TPM_CHECK(XACTION) do { \
	int ret_code = XACTION; \
	if (ret_code) { \
		vboot_debug("#FAIL(%d) - infineon_tpm:%d\n", \
			ret_code, __LINE__); \
		return ret_code; \
	} \
} while (0)

/* XOR checksum */
static uint8_t xor_checksum(const uint8_t *array, size_t len)
{
	uint8_t checksum = 0;
	if (len == 0 || array == NULL)
		return 0;
	while (len++) {
		checksum ^= *array++;
	}
	return checksum;
}

static inline void tpm_i2c_debug_dump(const uint8_t *data, int dir_w,
	size_t len)
{
#ifdef VBOOT_DEBUG
	uint32_t i;
	size_t data_len = len - DATA_START - 1;
	printf("%c - transport header %02x %02x %02x %02x %02x\n",
		dir_w ? 'W' : 'R', data[0], data[1], data[2], data[3], data[4]);
	printf("  - vendor header    %02x %02x %02x %02x %02x %02x",
		data[5], data[6], data[7], data[8], data[9], data[10]);
	for (i = 0; i < data_len; i++) {
		if (0 == (i & 0xf)) {
			printf("\n  - payload          ");
		}
		printf("%02x ", data[i + DATA_START]);
	}
	printf("\n  - checksum         %02x\n", data[DATA_START + data_len]);
#endif /* VBOOT_DEBUG */
}

/* 'to' buffer must be 6 bytes more larger than 'from' byte length */
static int tpm_i2c_pack(uint8_t *to, const uint8_t *from, size_t data_len)
{
	size_t transport_length = data_len + VENDOR_HEADER_SIZE;
	if (to == NULL || from == NULL)
		return E_PARAMETER;
	/* infineon prototype chip maximum data length */
	if (transport_length > 0xffff)
		return E_SIZE;

	/* build transport header */
	to[TSPT_VER  ] = TPM_TRANSPORT_VER;          // fixed        : 0x02
	to[TSPT_CTRL ] = TPM_TRANSPORT_CTRL_D_BIT;   // normal data  : 0x04
	to[TSPT_LEN_H] = 0xff & (transport_length >> 8);
	to[TSPT_LEN_L] = 0xff & transport_length;
	to[TSPT_CSUM1] = xor_checksum(to, 4);
	/* build vendor header */
	to[VEND_VERS ] = TPM_VENDOR_VERS;          // fixed : 0x01
	to[VEND_CHA  ] = TPM_VENDOR_CHA_TPM12MSC;  // fixed : 0x0B
	to[VEND_LEN_0] = 0;
	to[VEND_LEN_1] = 0;
	to[VEND_LEN_2] = 0xff & (data_len >> 8);
	to[VEND_LEN_3] = 0xff & data_len;
	/* clone payload body */
	memcpy(to + TPM_I2C_HEADER_SIZE, from, data_len);
	/* calculate tail checksum */
	to[DATA_START + data_len] = xor_checksum(to + VEND_VERS,
		transport_length);
	tpm_i2c_debug_dump(to, 1, DATA_START + data_len + 1);
	return 0;
}

/* verify and extract transport payload */
static int tpm_i2c_unpack(uint8_t *to, const uint8_t *from, size_t * pcopy_size)
{
	size_t transport_len, data_len;

	/* verify pointers
	 */
	if (to == NULL || from == NULL || pcopy_size == NULL)
		return E_PARAMETER;
	/* verify version, header checksum, control errorbit
	 */
	if (from[TSPT_VER   ] != TPM_TRANSPORT_VER)
		return E_DATA;
	if (from[TSPT_CSUM1 ] != xor_checksum(from, 4))
		return E_CHECKSUM;
	if (0 != (from[TSPT_CTRL  ] & TPM_TRANSPORT_CTRL_E_BIT))
		return E_DATA;
	/* verify payload size
	 */
	transport_len = ((size_t)from[TSPT_LEN_H] << 8) +
		(size_t)from[TSPT_LEN_L];
	data_len = ((size_t)from[VEND_LEN_2] << 8) + (size_t)from[VEND_LEN_3];
	if (transport_len != (data_len + VENDOR_HEADER_SIZE))
		return E_SIZE;
	/* verify payload checksum
	 */
	if (xor_checksum(from + TSPT_HEADER_SIZE, VENDOR_HEADER_SIZE +
		data_len) != from[DATA_START + data_len])
		return E_CHECKSUM;
	/* extract payload
	 */
	memcpy(to, from + DATA_START, data_len);
	*pcopy_size = data_len;
	tpm_i2c_debug_dump(from, 0, DATA_START + data_len + 1);
	return 0;
}

static int tpm_i2c_reg(uint8_t *reg)
{
	ulong stime = get_timer(0);
	while (i2c_write_data((uchar)INFINEON_TPM_CHIP, reg, 1)) {
		/* i2c busy, wait 1ms */
		udelay(I2CBUSY_WAIT_MS * 1000);
		if (get_timer(stime) > TIMEOUT_REG_MS)
			return E_TIMEOUT;
	}
	return 0;
}

static int tpm_i2c_read_status(t_inf_stat_reg *preg)
{
	uint8_t s_reg[8];
	uint8_t stat[4];
	s_reg[0] = (uchar)REG_STAT;
	TPM_CHECK(tpm_i2c_reg(s_reg));
	TPM_CHECK(i2c_read_data((uchar)INFINEON_TPM_CHIP, (uchar*)stat, 4));
	if (stat[3] != xor_checksum(stat, 3))
		return E_CHECKSUM;
	if (preg) {
		preg->status    = stat[0];
		preg->len_h     = stat[1];
		preg->len_l     = stat[2];
		preg->checksum  = stat[3];
		return 0;
	}
	return 0;
}

static int tpm_i2c_write_data(const uint8_t *data, size_t len)
{
	uint8_t w_reg[8];
	uint8_t packed_buffer[HUGE_BUFFER_SIZE];
	w_reg[0] = (uchar)REG_WDATA;

	/* check max buffer size
	 */
	if (len > (HUGE_BUFFER_SIZE - TPM_I2C_HEADER_SIZE - 1))
		return E_SIZE;
	/* pack data for transport
	 */
	TPM_CHECK(tpm_i2c_pack(packed_buffer, data, len));
	/* write to fifo
	 */
	TPM_CHECK(tpm_i2c_reg(w_reg));
	TPM_CHECK(i2c_write_data((uchar)INFINEON_TPM_CHIP,
		(uchar*)packed_buffer, len + PACKED_EXTRA_SIZE));
	return 0;
}

static int tpm_i2c_read_data(uint8_t *payload, size_t read_len,
  size_t *payload_len)
{
	uint8_t r_reg[8];
	uint8_t packed_buffer[HUGE_BUFFER_SIZE];
	size_t temp_size = 0;
	r_reg[0] = (uchar)REG_RDATA;

	if (read_len > HUGE_BUFFER_SIZE)
		return E_SIZE;
	/* read data into packed buffer
	 */
	TPM_CHECK(tpm_i2c_reg(r_reg));
	TPM_CHECK(i2c_read_data((uchar)INFINEON_TPM_CHIP,
		(uchar*)packed_buffer, read_len));
	if (payload == NULL)
		return 0;
	/* unpack buffer
	 */
	TPM_CHECK(tpm_i2c_unpack(payload, packed_buffer, payload_len ?
		payload_len : &temp_size));
	return 0;
}

static int tpm_prepare_write(ulong timeout_us)
{
	t_inf_stat_reg st_reg;
	ulong stime = get_timer(0);

	TPM_CHECK(tpm_i2c_read_status(&st_reg));
	if ((st_reg.status & TPM_TRANSMIT_FIFO_NOTFULL) != 0)
		return 0;
	/* when write, flush all data in read buffer first
	 */
	if ((st_reg.status & TPM_RECEIVE_DATA_AVAILABLE) != 0)
		TPM_CHECK(tpm_i2c_read_data(NULL, (size_t)st_reg.len_h * 256 +
			st_reg.len_l, NULL));

	do {
		TPM_CHECK(tpm_i2c_read_status(&st_reg));
	} while ((get_timer(stime) < timeout_us) &&
		(st_reg.status & TPM_TRANSMIT_FIFO_NOTFULL) == 0);

	if ((st_reg.status & TPM_TRANSMIT_FIFO_NOTFULL) == 0)
		return E_TIMEOUT;
	return 0;
}

static int tpm_prepare_read(ulong timeout_us)
{
	t_inf_stat_reg st_reg;
	ulong stime = get_timer(0);

	do {
		TPM_CHECK(tpm_i2c_read_status(&st_reg));
	} while ((get_timer(stime) < timeout_us) &&
		(st_reg.status & TPM_RECEIVE_DATA_AVAILABLE) == 0);

	return (st_reg.status & TPM_RECEIVE_DATA_AVAILABLE) == 0 ?
		E_TIMEOUT : 0;
}

/* TPM API
 */
int tpm_status_v03(int *ready_write, int *ready_read, size_t *p_len)
{
	t_inf_stat_reg reg;

	TPM_CHECK(tpm_i2c_read_status(&reg));
	/* exception case in prototype spec, error
	 */
	if (reg.status == 0xff && reg.len_h == 0xff && reg.len_l == 0xff)
		return E_GENERAL;
	/* verify status checksum
	 */
	if ((reg.status ^ reg.len_h ^ reg.len_l) != reg.checksum)
		return E_CHECKSUM;
	/* data available
	 */
	if (ready_read)
		*ready_read = (reg.status & TPM_RECEIVE_DATA_AVAILABLE) ? 1 : 0;
	/* data length
	 */
	if (p_len) {
		if (*ready_read)
			*p_len = ((size_t)reg.len_h << 8) + (size_t)reg.len_l;
		else
			*p_len = 0;
	}
	/* device writable
	 */
	if (ready_write)
		*ready_write = (reg.status & TPM_TRANSMIT_FIFO_NOTFULL) ? 1 : 0;
	return 0;
}


int tpm_init_v03(void)
{
	int ready_write = 0, ready_read = 0;
	size_t read_len = 0;
	int ret_code;
	#if defined(CONFIG_I2C_MULTI_BUS)
	i2c_set_bus_num(CONFIG_INFINEON_TPM_I2C_BUS);
	#endif
	ret_code = i2c_probe(INFINEON_TPM_CHIP);
	#ifdef VBOOT_DEBUG
	printf("v03 probe  : %s\n", ret_code ? "N/A" : "found");
	#endif /* VBOOT_DEBUG */
	if (ret_code)
		return ret_code;
	ret_code = tpm_status_v03(&ready_write, &ready_read, &read_len);
	#ifdef VBOOT_DEBUG
	printf("status : %d\n\tw[%c] r[%c : %u]\n", ret_code,
	ready_write ? 'y' : 'n',
	ready_read  ? 'y' : 'n', read_len);
	#endif /* VBOOT_DEBUG */
	if (ret_code)
		return ret_code;
	TPM_CHECK(tpm_prepare_write(TIMEOUT_WRITE_MS));
	return 0;
}

int tpm_sendrecv_v03(const uint8_t *sendbuf, size_t buf_size, uint8_t *recvbuf,
	size_t *recv_len)
{
	int ready_write = 0, ready_read = 0;
	size_t read_length = 0;
	TPM_CHECK(tpm_prepare_write(TIMEOUT_WRITE_MS));
	TPM_CHECK(tpm_i2c_write_data(sendbuf, buf_size));
	TPM_CHECK(tpm_prepare_read(TIMEOUT_READ_MS));
	TPM_CHECK(tpm_status_v03(&ready_write, &ready_read, &read_length));
	if (read_length == 0)
		return E_GENERAL;
	if (recv_len) {
		/* *recv_len sets the maximum data can be feched
		 * if *recv_len == 0, means the buffer is large enough to read any tpm
		 * response
		 */
		if (*recv_len != 0 && *recv_len < read_length)
			return E_SIZE;
	}
	if (recvbuf) {
		TPM_CHECK(tpm_i2c_read_data(recvbuf, read_length, NULL));
		/* update recv_len only on success */
		if (recv_len)
			*recv_len = read_length;
	} else {
		/* discard data in read buffer */
		TPM_CHECK(tpm_prepare_write(TIMEOUT_WRITE_MS));
	}
	return 0;
}

