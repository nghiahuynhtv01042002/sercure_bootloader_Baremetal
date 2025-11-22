#ifndef BIG_INT_H
#define BIG_INT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BIGINT_WORD_BITS     (32)
#define BIGINT_WORD_BYTES    (4)
#define BIGINT_MAX_WORDS     (128) // 64 * 32 = 2048 bits

typedef enum {
    BIGINT_OK = 0,
    BIGINT_ERR_NULL = -1,
    BIGINT_ERR_DIV_ZERO = -2,
    BIGINT_ERR_OVERFLOW = -3,
    BIGINT_ERR_INVALID = -4,
} bigIntStatus_t;

typedef struct {
    uint32_t words[BIGINT_MAX_WORDS];
    uint32_t length; 
} bigInt_t;

bigIntStatus_t bigint_zero(bigInt_t *a);
bigIntStatus_t bigint_from_uint32(bigInt_t *a, uint32_t val);
bigIntStatus_t bigint_from_bytes(bigInt_t *a, const uint8_t *bytes, size_t byte_len);
bigIntStatus_t bigint_to_bytes(const bigInt_t *a, uint8_t *bytes, size_t target_len);
bigIntStatus_t bigint_copy(bigInt_t *dst, const bigInt_t *src);
void bigint_normalize(bigInt_t *a);

bool bigint_is_zero(const bigInt_t *a);
int  bigint_compare(const bigInt_t *a, const bigInt_t *b);

bigIntStatus_t bigint_add(bigInt_t *res, const bigInt_t *a, const bigInt_t *b);
bigIntStatus_t bigint_sub(bigInt_t *res, const bigInt_t *a, const bigInt_t *b); // required: a ≥ b
bigIntStatus_t bigint_mul(bigInt_t *res, const bigInt_t *a, const bigInt_t *b);
bigIntStatus_t bigint_divmod(bigInt_t *quot, bigInt_t *rem, const bigInt_t *num, const bigInt_t *den);
bigIntStatus_t bigint_mod(bigInt_t *res, const bigInt_t *a, const bigInt_t *m);

bigIntStatus_t bigint_shift_left(bigInt_t *a, size_t bits);
bigIntStatus_t bigint_shift_right(bigInt_t *a, size_t bits);

bigIntStatus_t bigint_mod_exp(bigInt_t *res, const bigInt_t *base, const bigInt_t *exp, const bigInt_t *mod);

#endif // BIG_INT_H
