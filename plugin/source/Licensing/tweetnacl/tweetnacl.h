#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int crypto_hash_sha512_tweet(unsigned char *out, const unsigned char *m,
                             unsigned long long n);
int crypto_verify_32_tweet(const unsigned char *x, const unsigned char *y);
int crypto_sign_ed25519_tweet_open(unsigned char *m, unsigned long long *mlen,
                                   const unsigned char *sm, unsigned long long n,
                                   const unsigned char *pk);

// Ed25519 detached signature verification convenience function.
// Returns 0 on success, -1 on failure.
int ed25519_verify_detached(const unsigned char sig[64],
                            const unsigned char *msg, unsigned long long msglen,
                            const unsigned char pk[32]);

#ifdef __cplusplus
}
#endif
