#ifndef _CRYPTO_AESCTR_H_
#define _CRYPTO_AESCTR_H_

#include <stddef.h>
#include <stdint.h>

#include <openssl/aes.h>

/**
 * crypto_aesctr_init(key, nonce):
 * Prepare to encrypt/decrypt data with AES in CTR mode, using the provided
 * expanded key and nonce.  The key provided must remain valid for the
 * lifetime of the stream.
 */
struct crypto_aesctr * crypto_aesctr_init(const AES_KEY *, uint64_t);

/**
 * crypto_aesctr_stream(stream, inbuf, outbuf, buflen):
 * Generate the next ${buflen} bytes of the AES-CTR stream and xor them with
 * bytes from ${inbuf}, writing the result into ${outbuf}.  If the buffers
 * ${inbuf} and ${outbuf} overlap, they must be identical.
 */
void crypto_aesctr_stream(struct crypto_aesctr *, const uint8_t *,
    uint8_t *, size_t);

/**
 * crypto_aesctr_free(stream):
 * Free the provided stream object.
 */
void crypto_aesctr_free(struct crypto_aesctr *);

/**
 * crypto_aesctr_buf(key, nonce, inbuf, outbuf, buflen):
 * Equivalent to init(key, nonce); stream(inbuf, outbuf, buflen); free.
 */
void crypto_aesctr_buf(const AES_KEY *, uint64_t,
    const uint8_t *, uint8_t *, size_t);

#endif /* !_CRYPTO_AESCTR_H_ */