#include <string.h>
#include "core/types.h"
#include "core/errors.h"
#include "exec/exec.h"

/*
 * XOR cipher with an 8-byte key, cycling over the key bytes.
 * The encrypt and decrypt operations are identical (XOR is its own inverse).
 * Compile-time flag TEB2_MODERN swaps to ChaCha20-Poly1305.
 */
static void xor_block(const unsigned char *in, size_t len,
                      const Key8 *key, unsigned char *out)
{
    size_t i;
    for (i = 0; i < len; i++)
        out[i] = in[i] ^ key->k[i % 8];
}

CipherResult vault_encrypt(Bytes plaintext, Key8 key)
{
    CipherResult r;
    memset(&r, 0, sizeof(r));
    if (plaintext.len == 0 || plaintext.len > sizeof(r.data.data)) {
        r.err = ERR_LIMIT;
        return r;
    }
    xor_block(plaintext.data, plaintext.len, &key, r.data.data);
    r.data.len = plaintext.len;
    r.err = ERR_OK;
    return r;
}

CipherResult vault_decrypt(Bytes ciphertext, Key8 key)
{
    CipherResult r;
    memset(&r, 0, sizeof(r));
    if (ciphertext.len == 0 || ciphertext.len > sizeof(r.data.data)) {
        r.err = ERR_LIMIT;
        return r;
    }
    xor_block(ciphertext.data, ciphertext.len, &key, r.data.data);
    r.data.len = ciphertext.len;
    r.err = ERR_OK;
    return r;
}
