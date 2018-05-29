#ifndef  __RANDOM_H__
#define  __RANDOM_H__

#include <stdint.h>
#include <string.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

int random_init (void);

/* 32-byte random bytes */
const uint8_t * random_bytes_get (void);
void random_bytes_free (const uint8_t *p);

/* 8-bytes salt */
void random_get_salt (uint8_t *p);

int random_gen (void *arg, unsigned char *out, size_t out_len);

void random_fini (void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif
