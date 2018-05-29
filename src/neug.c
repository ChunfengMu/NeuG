/*
 * neug.c - true random number generation
 *
 * Copyright (C) 2011, 2012, 2013, 2016, 2017, 2018
 *               Free Software Initiative of Japan
 * Author: NIIBE Yutaka <gniibe@fsij.org>
 *
 * This file is a part of NeuG, a True Random Number Generator
 * implementation based on quantization error of ADC (for STM32F103).
 *
 * NeuG is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NeuG is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdint.h>
#include <string.h>

#include <rtthread.h>

#include "neug.h"
#include "sys-neug.h"
#include "adc.h"
#include "tiny_sha2.h"

/* CRC-32/MPGE-2 */
static const uint32_t crc32_rv_table[256] = {
  0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
  0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
  0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
  0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
  0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
  0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
  0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
  0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
  0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
  0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
  0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
  0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
  0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
  0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
  0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
  0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
  0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
  0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
  0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
  0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
  0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
  0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
  0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
  0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
  0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
  0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
  0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
  0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
  0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
  0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
  0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
  0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
  0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
  0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
  0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
  0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
  0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
  0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
  0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

static uint32_t crc;

void crc32_rv_reset (void)
{
  crc = 0xffffffff;
}

void crc32_rv_step (uint32_t v)
{
  crc = crc32_rv_table[(crc ^ (v << 0))  >> 24] ^ (crc << 8);
  crc = crc32_rv_table[(crc ^ (v << 8))  >> 24] ^ (crc << 8);
  crc = crc32_rv_table[(crc ^ (v << 16)) >> 24] ^ (crc << 8);
  crc = crc32_rv_table[(crc ^ (v << 24)) >> 24] ^ (crc << 8);
}

uint32_t crc32_rv_get (void)
{
  return crc;
}
void crc32_rv_stop (void)
{
}

#define SHA256_DIGEST_SIZE  32

#define MODE_CONDITION   0x01

struct rt_mutex mode_mtx;
struct rt_event mode_cond;

static tiny_sha2_context tiny_sha2_ctx;
static uint32_t tiny_sha2_input[64/sizeof(uint32_t)];
static uint32_t tiny_sha2_output[SHA256_DIGEST_SIZE/sizeof (uint32_t)];

/*
 * To be a full entropy source, the requirement is to have N samples
 * for output of 256-bit, where:
 *
 *      N = (256 * 2) / <min-entropy of a sample>
 *
 * For example, N should be more than 103 for min-entropy = 5.0.
 *
 * On the other hand, in the section 6.2 "Full Entropy Source
 * Requirements", it says:
 *
 *     At least twice the block size of the underlying cryptographic
 *     primitive shall be provided as input to the conditioning
 *     function to produce full entropy output.
 *
 * For us, cryptographic primitive is SHA-256 and its blocksize is
 * 512-bit (64-byte), thus, N >= 128.
 *
 * We chose N=140.  Note that we have "additional bits" of 16-byte for
 * last block (feedback from previous output of SHA-256) to feed
 * hash_df function of SHA-256, together with sample data of 140-byte.
 *
 * N=140 corresponds to min-entropy >= 3.68.
 *
 */
#define NUM_NOISE_INPUTS 140

#define EP_ROUND_0 0 /* initial-five-byte and 3-byte, then 56-byte-input */
#define EP_ROUND_1 1 /* 64-byte-input */
#define EP_ROUND_2 2 /* 17-byte-input */
#define EP_ROUND_RAW      3 /* 32-byte-input */
#define EP_ROUND_RAW_DATA 4 /* 32-byte-input */

#define EP_ROUND_0_INPUTS 56
#define EP_ROUND_1_INPUTS 64
#define EP_ROUND_2_INPUTS 17
#define EP_ROUND_RAW_INPUTS 32
#define EP_ROUND_RAW_DATA_INPUTS 32

static uint8_t ep_round;

static void noise_source_continuous_test (uint8_t noise);
static void noise_source_continuous_test_word (uint8_t b0, uint8_t b1,
					       uint8_t b2, uint8_t b3);

/*
 * Hash_df initial string:
 *
 *  Initial five bytes are:
 *    1,          : counter = 1
 *    0, 0, 1, 0  : no_of_bits_returned (in big endian)
 *
 *  Then, three-byte from noise source follows.
 *
 *  One-byte was used in the previous turn, and we have three bytes in
 *  CRC32.
 */
static void ep_fill_initial_string (void)
{
  uint32_t v = crc32_rv_get ();
  uint8_t b1, b2, b3;

  b3 = v >> 24;
  b2 = v >> 16;
  b1 = v >> 8;

  noise_source_continuous_test (b1);
  noise_source_continuous_test (b2);
  noise_source_continuous_test (b3);

  adc_buf[0] = 0x01000001;
  adc_buf[1] = (v & 0xffffff00);
}

static void ep_init (int mode)
{
  if (mode == NEUG_MODE_RAW)
  {
    ep_round = EP_ROUND_RAW;
    adc_start_conversion (0, EP_ROUND_RAW_INPUTS);
  }
  else if (mode == NEUG_MODE_RAW_DATA)
  {
    ep_round = EP_ROUND_RAW_DATA;
    adc_start_conversion (0, EP_ROUND_RAW_DATA_INPUTS / 4);
  }
  else
  {
    ep_round = EP_ROUND_0;
    ep_fill_initial_string ();
    adc_start_conversion (2, EP_ROUND_0_INPUTS);
  }
}

static void ep_fill_wbuf_v (int i, int test, uint32_t v)
{
  if (test)
  {
    uint8_t b0, b1, b2, b3;

    b3 = v >> 24;
    b2 = v >> 16;
    b1 = v >> 8;
    b0 = v;

    noise_source_continuous_test_word (b0, b1, b2, b3);
  }

  tiny_sha2_input[i] = v;
}

/* Here, we assume a little endian architecture.  */
static int ep_process (int mode)
{
  int i, n;
  uint32_t v;

  if (ep_round == EP_ROUND_0)/* wbuf fill (3 + 5 + 56 bytes)*/
  {
    tiny_sha2_starts(&tiny_sha2_ctx, 0);
    tiny_sha2_input[0] = adc_buf[0];
    tiny_sha2_input[1] = adc_buf[1];

    for (i = 0; i < EP_ROUND_0_INPUTS / 4; i++)
    {
      crc32_rv_step (adc_buf[i*4 + 2]);
      crc32_rv_step (adc_buf[i*4 + 3]);
      crc32_rv_step (adc_buf[i*4 + 4]);
      crc32_rv_step (adc_buf[i*4 + 5]);
      v = crc32_rv_get ();

      ep_fill_wbuf_v (i+2, 1, v);
    }

    adc_start_conversion (0, EP_ROUND_1_INPUTS);
    tiny_sha2_update(&tiny_sha2_ctx, (unsigned char *)&tiny_sha2_input[0], 64);

    ep_round++;

    return 0;
  }
  else if (ep_round == EP_ROUND_1)/* wbuf fill 64 bytes */
  {
    for (i = 0; i < EP_ROUND_1_INPUTS / 4; i++)
    {
      crc32_rv_step (adc_buf[i*4]);
      crc32_rv_step (adc_buf[i*4 + 1]);
      crc32_rv_step (adc_buf[i*4 + 2]);
      crc32_rv_step (adc_buf[i*4 + 3]);
      v = crc32_rv_get ();

      ep_fill_wbuf_v (i, 1, v);
    }

    adc_start_conversion (0, EP_ROUND_2_INPUTS + 3);
    tiny_sha2_update(&tiny_sha2_ctx, (unsigned char *)&tiny_sha2_input[0], 64);

    ep_round++;

    return 0;
  }
  else if (ep_round == EP_ROUND_2)/* wbuf fill (17 + 16 bytes) */
  {
    for (i = 0; i < EP_ROUND_2_INPUTS / 4; i++)
    {
      crc32_rv_step (adc_buf[i*4]);
      crc32_rv_step (adc_buf[i*4 + 1]);
      crc32_rv_step (adc_buf[i*4 + 2]);
      crc32_rv_step (adc_buf[i*4 + 3]);
      v = crc32_rv_get ();

      ep_fill_wbuf_v (i, 1, v);
    }

    crc32_rv_step (adc_buf[i*4]);
    crc32_rv_step (adc_buf[i*4 + 1]);
    crc32_rv_step (adc_buf[i*4 + 2]);
    crc32_rv_step (adc_buf[i*4 + 3]);

    v = crc32_rv_get () & 0xff;   /* First byte of CRC32 is used here.  */
    noise_source_continuous_test (v);
    tiny_sha2_input[i] = v;

    ep_init (NEUG_MODE_CONDITIONED); /* The rest three-byte of CRC32 is used here.  */

    n = SHA256_DIGEST_SIZE / 2;
    memcpy (((unsigned char *)&tiny_sha2_input[0]) + EP_ROUND_2_INPUTS, (unsigned char *)&tiny_sha2_output[0], n);
    tiny_sha2_update(&tiny_sha2_ctx, (unsigned char *)&tiny_sha2_input[0], EP_ROUND_2_INPUTS + n);
    tiny_sha2_finish(&tiny_sha2_ctx, (unsigned char *)&tiny_sha2_output[0]);

    return SHA256_DIGEST_SIZE / sizeof (uint32_t);
  }
  else if (ep_round == EP_ROUND_RAW)
  {
    for (i = 0; i < EP_ROUND_RAW_INPUTS / 4; i++)
    {
      crc32_rv_step (adc_buf[i*4]);
      crc32_rv_step (adc_buf[i*4 + 1]);
      crc32_rv_step (adc_buf[i*4 + 2]);
      crc32_rv_step (adc_buf[i*4 + 3]);
      v = crc32_rv_get ();
      ep_fill_wbuf_v (i, 1, v);
    }

    ep_init (mode);

    return EP_ROUND_RAW_INPUTS / 4;
  }
  else if (ep_round == EP_ROUND_RAW_DATA)
  {
    for (i = 0; i < EP_ROUND_RAW_DATA_INPUTS / 4; i++)
    {
      v = adc_buf[i];
      ep_fill_wbuf_v (i, 0, v);
    }

    ep_init (mode);

    return EP_ROUND_RAW_DATA_INPUTS / 4;
  }

  return 0;
}

static const uint32_t *ep_output (int mode)
{
  if (mode)
  {
    return &tiny_sha2_input[0];
  }
  else
  {
    return &tiny_sha2_output[0];
  }
}

#define REPETITION_COUNT           1
#define ADAPTIVE_PROPORTION_64     2
#define ADAPTIVE_PROPORTION_4096   4

uint8_t neug_err_state;

uint16_t neug_err_cnt;
uint16_t neug_err_cnt_rc;
uint16_t neug_err_cnt_p64;
uint16_t neug_err_cnt_p4k;

uint16_t neug_rc_max;
uint16_t neug_p64_max;
uint16_t neug_p4k_max;

static void noise_source_cnt_max_reset (void)
{
  neug_err_cnt = neug_err_cnt_rc = neug_err_cnt_p64 = neug_err_cnt_p4k = 0;
  neug_rc_max = neug_p64_max = neug_p4k_max = 0;
}

static void noise_source_error_reset (void)
{
  neug_err_state = 0;
}

static void noise_source_error (uint32_t err)
{
  neug_err_state |= err;
  neug_err_cnt++;

  if ((err & REPETITION_COUNT))
  {
    neug_err_cnt_rc++;
  }

  if ((err & ADAPTIVE_PROPORTION_64))
  {
    neug_err_cnt_p64++;
  }

  if ((err & ADAPTIVE_PROPORTION_4096))
  {
    neug_err_cnt_p4k++;
  }
}

/*
 * For health tests, we assume that the device noise source has
 * min-entropy >= 4.2.  Observing raw data stream (before CRC-32) has
 * more than 4.2 bit/byte entropy.  When the data stream after CRC-32
 * filter will be less than 4.2 bit/byte entropy, that must be
 * something wrong.  Note that even we observe < 4.2, we still have
 * some margin, since we use NUM_NOISE_INPUTS=140.
 *
 */

/* Cuttoff = 9, when min-entropy = 4.2, W= 2^-30 */
/* ceiling of (1+30/4.2) */
#define REPITITION_COUNT_TEST_CUTOFF 9

static uint8_t rct_a;
static uint8_t rct_b;

static void repetition_count_test (uint8_t sample)
{
  if (rct_a == sample)
  {
    rct_b++;

    if (rct_b >= REPITITION_COUNT_TEST_CUTOFF)
    {
      noise_source_error (REPETITION_COUNT);
    }

    if (rct_b > neug_rc_max)
    {
      neug_rc_max = rct_b;
    }
  }
  else
  {
    rct_a = sample;
    rct_b = 1;
  }
}

static void repetition_count_test_word (uint8_t b0, uint8_t b1,
					uint8_t b2, uint8_t b3)
{
  if (rct_a == b0)
  {
    rct_b++;
  }
  else
  {
    rct_a = b0;
    rct_b = 1;
  }

  if (rct_a == b1)
  {
    rct_b++;
  }
  else
  {
    rct_a = b1;
    rct_b = 1;
  }

  if (rct_a == b2)
  {
    rct_b++;
  }
  else
  {
    rct_a = b2;
    rct_b = 1;
  }

  if (rct_a == b3)
  {
    rct_b++;
  }
  else
  {
    rct_a = b3;
    rct_b = 1;
  }

  if (rct_b >= REPITITION_COUNT_TEST_CUTOFF)
  {
    noise_source_error (REPETITION_COUNT);
  }

  if (rct_b > neug_rc_max)
  {
    neug_rc_max = rct_b;
  }
}

/* Cuttoff = 18, when min-entropy = 4.2, W= 2^-30 */
/* With R, qbinom(1-2^-30,64,2^-4.2) */
#define ADAPTIVE_PROPORTION_64_TEST_CUTOFF 18

static uint8_t ap64t_a;
static uint8_t ap64t_b;
static uint8_t ap64t_s;

static void adaptive_proportion_64_test (uint8_t sample)
{
  if (ap64t_s++ >= 64)
  {
    ap64t_a = sample;
    ap64t_s = 1;
    ap64t_b = 0;
  }
  else
  {
    if (ap64t_a == sample)
    {
      ap64t_b++;

      if (ap64t_b > ADAPTIVE_PROPORTION_64_TEST_CUTOFF)
      {
        noise_source_error (ADAPTIVE_PROPORTION_64);
      }

      if (ap64t_b > neug_p64_max)
      {
        neug_p64_max = ap64t_b;
      }
    }
  }
}

static void adaptive_proportion_64_test_word (uint8_t b0, uint8_t b1,
					      uint8_t b2, uint8_t b3)
{
  if (ap64t_s >= 64)
  {
    ap64t_a = b0;
    ap64t_s = 4;
    ap64t_b = 0;
  }
  else
  {
    ap64t_s += 4;

    if (ap64t_a == b0)
    {
      ap64t_b++;
    }
  }

  if (ap64t_a == b1)
  {
    ap64t_b++;
  }

  if (ap64t_a == b2)
  {
    ap64t_b++;
  }

  if (ap64t_a == b3)
  {
    ap64t_b++;
  }

  if (ap64t_b > ADAPTIVE_PROPORTION_64_TEST_CUTOFF)
  {
    noise_source_error (ADAPTIVE_PROPORTION_64);
  }

  if (ap64t_b > neug_p64_max)
  {
    neug_p64_max = ap64t_b;
  }
}

/* Cuttoff = 315, when min-entropy = 4.2, W= 2^-30 */
/* With R, qbinom(1-2^-30,4096,2^-4.2) */
#define ADAPTIVE_PROPORTION_4096_TEST_CUTOFF 315

static uint8_t ap4096t_a;
static uint16_t ap4096t_b;
static uint16_t ap4096t_s;

static void adaptive_proportion_4096_test (uint8_t sample)
{
  if (ap4096t_s++ >= 4096)
  {
    ap4096t_a = sample;
    ap4096t_s = 1;
    ap4096t_b = 0;
  }
  else
  {
    if (ap4096t_a == sample)
    {
      ap4096t_b++;

      if (ap4096t_b > ADAPTIVE_PROPORTION_4096_TEST_CUTOFF)
      {
        noise_source_error (ADAPTIVE_PROPORTION_4096);
      }

      if (ap4096t_b > neug_p4k_max)
      {
        neug_p4k_max = ap4096t_b;
      }
    }
  }
}

static void adaptive_proportion_4096_test_word (uint8_t b0, uint8_t b1,
						uint8_t b2, uint8_t b3)
{
  if (ap4096t_s >= 4096)
  {
    ap4096t_a = b0;
    ap4096t_s = 4;
    ap4096t_b = 0;
  }
  else
  {
    ap4096t_s += 4;

    if (ap4096t_a == b0)
    {
      ap4096t_b++;
    }
  }

  if (ap4096t_a == b1)
  {
    ap4096t_b++;
  }

  if (ap4096t_a == b2)
	{
	  ap4096t_b++;
  }

  if (ap4096t_a == b3)
  {
    ap4096t_b++;
  }

  if (ap4096t_b > ADAPTIVE_PROPORTION_4096_TEST_CUTOFF)
  {
    noise_source_error (ADAPTIVE_PROPORTION_4096);
  }

  if (ap4096t_b > neug_p4k_max)
  {
    neug_p4k_max = ap4096t_b;
  }
}

static void noise_source_continuous_test (uint8_t noise)
{
  repetition_count_test (noise);
  adaptive_proportion_64_test (noise);
  adaptive_proportion_4096_test (noise);
}

static void noise_source_continuous_test_word (uint8_t b0, uint8_t b1,
					       uint8_t b2, uint8_t b3)
{
  repetition_count_test_word (b0, b1, b2, b3);
  adaptive_proportion_64_test_word (b0, b1, b2, b3);
  adaptive_proportion_4096_test_word (b0, b1, b2, b3);
}

#define RNG_SPACE_AVAILABLE   0x01
#define RNG_DATA_AVAILABLE   0x02

#define RNG_ALL_STATE (RNG_DATA_AVAILABLE|RNG_SPACE_AVAILABLE)

/*
 * Ring buffer, filled by generator, consumed by neug_get routine.
 */
struct rng_rb {
  uint32_t *buf;
  struct rt_mutex m;
  struct rt_event available_state;
  uint8_t head, tail;
  uint8_t size;
  unsigned int full :1;
  unsigned int empty :1;
};

static void rb_init (struct rng_rb *rb, uint32_t *p, uint8_t size)
{
  rb->buf = p;
  rb->size = size;
  rt_mutex_init(&rb->m, "rng_rb_m", RT_IPC_FLAG_FIFO);
  rt_event_init(&rb->available_state, "rng_rb_s", RT_IPC_FLAG_FIFO);
  rb->head = rb->tail = 0;
  rb->full = 0;
  rb->empty = 1;
}

static void rb_add (struct rng_rb *rb, uint32_t v)
{
  rb->buf[rb->tail++] = v;

  if (rb->tail == rb->size)
  {
    rb->tail = 0;
  }

  if (rb->tail == rb->head)
  {
    rb->full = 1;
  }

  rb->empty = 0;
}

static uint32_t rb_del (struct rng_rb *rb)
{
  uint32_t v = rb->buf[rb->head++];

  if (rb->head == rb->size)
  {
    rb->head = 0;
  }

  if (rb->head == rb->tail)
  {
    rb->empty = 1;
  }

  rb->full = 0;

  return v;
}

uint8_t neug_mode;
static int rng_should_terminate;
static rt_thread_t rng_thread;

/**
 * @brief Random number generation thread.
 */
static void rng (void* parameter)
{
  struct rng_rb *rb = (struct rng_rb *)parameter;
  int mode = neug_mode;

  rng_should_terminate = 0;
  rt_mutex_init(&mode_mtx, "rng_mode", RT_IPC_FLAG_FIFO);
  rt_event_init(&mode_cond, "rng_mode", RT_IPC_FLAG_FIFO);

  /* Init ADCs */
  adc_init();

  /* Enable ADCs */
  adc_start ();

  ep_init (mode);

  while (!rng_should_terminate)
  {
    int err;
    int n;

    /* return 0 on success. */
    err = adc_wait_completion ();

    rt_mutex_take(&mode_mtx, RT_WAITING_FOREVER);
    /* if err occur or mode change */
    if (err || mode != neug_mode)
    {
      mode = neug_mode;

      noise_source_cnt_max_reset ();

      /* Discarding data available, re-initiate from the start.  */
      ep_init (mode);

      rt_mutex_release(&mode_mtx);

      /* notify mode condition event */
      rt_event_send(&mode_cond, MODE_CONDITION);

      continue;
    }
    else
    {
      rt_mutex_release(&mode_mtx);
    }

    if ((n = ep_process (mode)) > 0)
    {
      int i;
      const uint32_t *vp;

      /* noise err */
      if (neug_err_state != 0 && 
        (mode == NEUG_MODE_CONDITIONED || mode == NEUG_MODE_RAW))
      {
        /* Don't use the result and do it again.  */
        noise_source_error_reset ();
        continue;
      }

      vp = ep_output (mode);

      rt_mutex_take(&rb->m, RT_WAITING_FOREVER);
      while (rb->full)
      {
        rt_mutex_release(&rb->m);

        /* wait until space available event */
        rt_event_recv(&rb->available_state, RNG_SPACE_AVAILABLE, 
            RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, NULL);

        rt_mutex_take(&rb->m, RT_WAITING_FOREVER);
      }

      for (i = 0; i < n; i++)
      {
        rb_add (rb, *vp++);
        if (rb->full)
        {
          break;
        }
      }

      rt_mutex_release(&rb->m);

      /* notify data available */
      rt_event_send(&rb->available_state, RNG_DATA_AVAILABLE);
    }
  }

  adc_stop();
}

static struct rng_rb the_ring_buffer;

/**
 * @brief Initialize NeuG.
 */
void neug_init (uint32_t *buf, uint8_t size)
{
  const uint32_t *u = (const uint32_t *)unique_device_id ();
  struct rng_rb *rb = &the_ring_buffer;
  int i;

  crc32_rv_reset ();

  /*
   * This initialization ensures that it generates different sequence
   * even if all physical conditions are same.
   */
  for (i = 0; i < 3; i++)
  {
    crc32_rv_step (*u++);
  }

  neug_mode = NEUG_MODE_CONDITIONED;

  rb_init (rb, buf, size);

  rng_thread = rt_thread_create("rng", rng, rb, 
                    2048, RT_THREAD_PRIORITY_MAX -2, 32);

  if (rng_thread != RT_NULL)
  {
    rt_thread_startup(rng_thread);
  }
}

/**
 * @brief  Get random word (32-bit) from NeuG.
 * @detail With NEUG_KICK_FILLING, it wakes up RNG thread.
 *         With NEUG_NO_KICK, it doesn't wake up RNG thread automatically,
 *         it is needed to call neug_kick_filling later.
 */
uint32_t neug_get (int kick)
{
  struct rng_rb *rb = &the_ring_buffer;
  uint32_t v;

  rt_mutex_take(&rb->m, RT_WAITING_FOREVER);
  while (rb->empty)
  {
    rt_mutex_release(&rb->m);

    /* wait until data available */
    rt_event_recv(&rb->available_state, RNG_DATA_AVAILABLE, 
        RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, NULL);

    rt_mutex_take(&rb->m, RT_WAITING_FOREVER);
  }

  v = rb_del (rb);

  rt_mutex_release(&rb->m);

  if (kick)
  {
    /* notify space available event */
    rt_event_send(&rb->available_state, RNG_SPACE_AVAILABLE);
  }

  return v;
}

int neug_get_nonblock (uint32_t *p)
{
  struct rng_rb *rb = &the_ring_buffer;
  int r = 0;

  rt_mutex_take(&rb->m, RT_WAITING_FOREVER);
  if (rb->empty)
  {
    r = -1;
    /* notify space available event */
    rt_event_send(&rb->available_state, RNG_SPACE_AVAILABLE);
  }
  else
  {
    *p = rb_del (rb);
  }

  rt_mutex_release(&rb->m);

  return r;
}

/**
 * @brief  Wakes up RNG thread to generate random numbers.
 */
void neug_kick_filling (void)
{
  struct rng_rb *rb = &the_ring_buffer;

  rt_mutex_take(&rb->m, RT_WAITING_FOREVER);

  if (!rb->full)
  {
    /* notify space available event */
    rt_event_send(&rb->available_state, RNG_SPACE_AVAILABLE);
  }

  rt_mutex_release(&rb->m);
}

void neug_wait_full (void)
{
  struct rng_rb *rb = &the_ring_buffer;

  rt_mutex_take(&rb->m, RT_WAITING_FOREVER);
  while (!rb->full)
  {
    rt_mutex_release(&rb->m);

    /* wait until data available */
    rt_event_recv(&rb->available_state, RNG_DATA_AVAILABLE, 
        RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, NULL);

    rt_mutex_take(&rb->m, RT_WAITING_FOREVER);
  }

  rt_mutex_release(&rb->m);
}

/**
 * @breif Flush random bytes.
 */
void neug_flush (void)
{
  struct rng_rb *rb = &the_ring_buffer;

  rt_mutex_take(&rb->m, RT_WAITING_FOREVER);
  while (!rb->empty)
  {
    (void)rb_del (rb);
  }

  rt_mutex_release(&rb->m);

  /* notify space available event */
  rt_event_send(&rb->available_state, RNG_SPACE_AVAILABLE);
}

void neug_mode_select (uint8_t mode)
{
  if (neug_mode == mode)
  {
    return;
  }

  neug_wait_full ();

  rt_mutex_take(&mode_mtx, RT_WAITING_FOREVER);
  neug_mode = mode;
  neug_flush ();
  rt_mutex_release(&mode_mtx);

  /* wait until mode condition event */
  rt_event_recv(&mode_cond, MODE_CONDITION, 
      RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, NULL);

  neug_wait_full ();
  neug_flush ();
}

int neug_consume_random (void (*proc) (uint32_t, int))
{
  struct rng_rb *rb = &the_ring_buffer;
  int i = 0;
  uint32_t v;

  rt_mutex_take(&rb->m, RT_WAITING_FOREVER);
  while (!rb->empty)
  {

    v = rb_del (rb);
    proc (v, i);
    i++;
  }

  rt_mutex_release(&rb->m);
  /* notify space available event */
  rt_event_send(&rb->available_state, RNG_SPACE_AVAILABLE);

  return i;
}

void neug_fini (void)
{
  rng_should_terminate = 1;
  neug_get (1);
  crc32_rv_stop ();
}

