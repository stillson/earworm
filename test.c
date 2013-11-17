/*
test.c - Test harness
Written in 2013 by Daniel Franke <dfoxfranke@gmail.com>

To the extent possible under law, the author(s) have dedicated all
copyright and related and neighboring rights to this software to the
public domain worldwide. This software is distributed without any
warranty.

You should have received a copy of the CC0 Public Domain Dedication
along with this software. If not, see
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#include "aes.h"
#include "core.h"
#include "sha256.h"
#include "phc.h"
#include "util.h"

#define AES_BLOCK_SIZE ((size_t)16)

static int test_be32enc() {
  uint8_t buf[4];

  be32enc(buf, 0x01020304U);
  if(buf[0] != 1 || buf[1] != 2 || buf[2] != 3 || buf[3] != 4)
    return -1;

  be32enc(buf, 0xffefdfcfU);
  if(buf[0] != 0xff || buf[1] != 0xef || buf[2] != 0xdf || buf[3] != 0xcf)
    return -1;

  return 0;
}

static int test_be32dec() {
  uint8_t buf1[] = { 0x01, 0x02, 0x03, 0x04 };
  uint8_t buf2[] = { 0xff, 0xef, 0xdf, 0xcf };
  
  if(be32dec(buf1) != 0x01020304U)
    return -1;

  if(be32dec(buf2) != 0xffefdfcfU)
    return -1;

  return 0;
}

static int test_be64enc() {
  uint8_t buf[8];
  be64enc(buf, 0x0102030405060708ULL);
  if(buf[0] != 1 || buf[1] != 2 || buf[2] != 3 || buf[3] != 4 ||
     buf[4] != 5 || buf[5] != 6 || buf[6] != 7 || buf[7] != 8)
    return -1;

  be64enc(buf, 0xffefdfcfbfaf9f8fULL);
  if(buf[0] != 0xff || buf[1] != 0xef || buf[2] != 0xdf || buf[3] != 0xcf ||
     buf[4] != 0xbf || buf[5] != 0xaf || buf[6] != 0x9f || buf[7] != 0x8f)
    return -1;

  return 0;
}

static int test_be64dec() {
  uint8_t buf1[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
  uint8_t buf2[] = { 0xff, 0xef, 0xdf, 0xcf, 0xbf, 0xaf, 0x9f, 0xaf };

  if(be64dec(buf1) != 0x0102030405060708ULL)
    return -1;
  if(be64dec(buf2) != 0xffefdfcfbfaf9fafULL)
    return -1;

  return 0;
}
  
static int test_SHA256() {
  /* Test vectors from http://www.nsrl.nist.gov/testdata/ */
  static const uint8_t data1[] = {
    0x61, 0x62, 0x63
  };

  static const uint8_t result1[] = {
    0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
    0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
    0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
    0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD
  };

  static const uint8_t data2[] = {
    0x61, 0x62, 0x63, 0x64, 0x62, 0x63, 0x64, 0x65,
    0x63, 0x64, 0x65, 0x66, 0x64, 0x65, 0x66, 0x67,
    0x65, 0x66, 0x67, 0x68, 0x66, 0x67, 0x68, 0x69,
    0x67, 0x68, 0x69, 0x6a, 0x68, 0x69, 0x6a, 0x6b,
    0x69, 0x6a, 0x6b, 0x6c, 0x6a, 0x6b, 0x6c, 0x6d,
    0x6b, 0x6c, 0x6d, 0x6e, 0x6c, 0x6d, 0x6e, 0x6f,
    0x6d, 0x6e, 0x6f, 0x70, 0x6e, 0x6f, 0x70, 0x71
  };

  static const uint8_t result2[] = {
    0x24, 0x8D, 0x6A, 0x61, 0xD2, 0x06, 0x38, 0xB8,
    0xE5, 0xC0, 0x26, 0x93, 0x0C, 0x3E, 0x60, 0x39,
    0xA3, 0x3C, 0xE4, 0x59, 0x64, 0xFF, 0x21, 0x67,
    0xF6, 0xEC, 0xED, 0xD4, 0x19, 0xDB, 0x06, 0xC1
  };

  static uint8_t data3[1000000];

  static const uint8_t result3[] = {
    0xCD, 0xC7, 0x6E, 0x5C, 0x99, 0x14, 0xFB, 0x92,
    0x81, 0xA1, 0xC7, 0xE2, 0x84, 0xD7, 0x3E, 0x67,
    0xF1, 0x80, 0x9A, 0x48, 0xA4, 0x97, 0x20, 0x0E,
    0x04, 0x6D, 0x39, 0xCC, 0xC7, 0x11, 0x2C, 0xD0
  };

  SHA256_CTX ctx;
  uint8_t result[32];

  earworm_SHA256_Init(&ctx);
  earworm_SHA256_Update(&ctx, data1, sizeof data1);
  earworm_SHA256_Final(result, &ctx);
  if(memcmp(result1, result, 32))
    return -1;

  earworm_SHA256_Init(&ctx);
  earworm_SHA256_Update(&ctx, data2, sizeof data2);
  earworm_SHA256_Final(result, &ctx);
  if(memcmp(result2, result, 32))
    return -1;

  if(data3 == NULL)
    return -1;

  memset(data3, 'a', 1000000);

  earworm_SHA256_Init(&ctx);
  earworm_SHA256_Update(&ctx, data3, 1000000);
  earworm_SHA256_Final(result, &ctx);
  if(memcmp(result3, result, 32))
    return -1;

  earworm_SHA256_Init(&ctx);
  earworm_SHA256_Update(&ctx, data3, 500000);
  earworm_SHA256_Update(&ctx, data3 + 500000, 500000);
  earworm_SHA256_Final(result, &ctx);
  if(memcmp(result3, result, 32))
    return -1;
    
  return 0;
}

static int test_HMAC_SHA256() {
  /* Test vectors from RFC4231. */
  static const uint8_t key1[] = {
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
  };

  static const uint8_t data1[] = {
    0x48, 0x69, 0x20, 0x54, 0x68, 0x65, 0x72, 0x65
  };

  static const uint8_t result1[] = {
    0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53, 
    0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b,
    0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7,
    0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7
  };

  static const uint8_t key2[] = {
    0x4a, 0x65, 0x66, 0x65
  };

  static const uint8_t data2[] = {
    0x77, 0x68, 0x61, 0x74, 0x20, 0x64, 0x6f, 0x20,
    0x79, 0x61, 0x20, 0x77, 0x61, 0x6e, 0x74, 0x20,
    0x66, 0x6f, 0x72, 0x20, 0x6e, 0x6f, 0x74, 0x68,
    0x69, 0x6e, 0x67, 0x3f
  };

  static const uint8_t result2[] = {
    0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e,
    0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
    0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83,
    0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43
  };

  static const uint8_t key3[] = {
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa
  };

  static const uint8_t data3[] = {
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd
  };

  static const uint8_t result3[] = {
    0x77, 0x3e, 0xa9, 0x1e, 0x36, 0x80, 0x0e, 0x46,
    0x85, 0x4d, 0xb8, 0xeb, 0xd0, 0x91, 0x81, 0xa7,
    0x29, 0x59, 0x09, 0x8b, 0x3e, 0xf8, 0xc1, 0x22,
    0xd9, 0x63, 0x55, 0x14, 0xce, 0xd5, 0x65, 0xfe
  };

  static const uint8_t key4[] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19
  };

  static const uint8_t data4[] = {
    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 
    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 
    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 
    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 
    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd
  };

  static const uint8_t result4[] = {
    0x82, 0x55, 0x8a, 0x38, 0x9a, 0x44, 0x3c, 0x0e,
    0xa4, 0xcc, 0x81, 0x98, 0x99, 0xf2, 0x08, 0x3a,
    0x85, 0xf0, 0xfa, 0xa3, 0xe5, 0x78, 0xf8, 0x07,
    0x7a, 0x2e, 0x3f, 0xf4, 0x67, 0x29, 0x66, 0x5b
  };    

  HMAC_SHA256_CTX ctx;
  uint8_t result[32];

  earworm_HMAC_SHA256_Init(&ctx, key1, sizeof key1);
  earworm_HMAC_SHA256_Update(&ctx, data1, sizeof data1);
  earworm_HMAC_SHA256_Final(result, &ctx);
  if(memcmp(result1, result, 32))
    return -1;

  earworm_HMAC_SHA256_Init(&ctx, key2, sizeof key2);
  earworm_HMAC_SHA256_Update(&ctx, data2, sizeof data2);
  earworm_HMAC_SHA256_Final(result, &ctx);
  if(memcmp(result2, result, 32))
    return -1;

  earworm_HMAC_SHA256_Init(&ctx, key3, sizeof key3);
  earworm_HMAC_SHA256_Update(&ctx, data3, sizeof data3);
  earworm_HMAC_SHA256_Final(result, &ctx);
  if(memcmp(result3, result, 32))
    return -1;

  earworm_HMAC_SHA256_Init(&ctx, key4, sizeof key4);
  earworm_HMAC_SHA256_Update(&ctx, data4, sizeof data4);
  earworm_HMAC_SHA256_Final(result, &ctx);
  if(memcmp(result4, result, 32))
    return -1;

  earworm_HMAC_SHA256_Init(&ctx, key4, sizeof key4);
  earworm_HMAC_SHA256_Update(&ctx, data4, 25);
  earworm_HMAC_SHA256_Update(&ctx, data4+25, sizeof data4 - 25);
  earworm_HMAC_SHA256_Final(result, &ctx);
  if(memcmp(result4, result, 32))
    return -1;  

  return 0;
}

static int test_PBKDF2_SHA256() {
  /* Test vectors from http://tools.ietf.org/html/draft-josefsson-scrypt-kdf-01#section-10 */
  static const uint8_t secret1[] = {
    0x70, 0x61, 0x73, 0x73, 0x77, 0x64
  };

  static const uint8_t salt1[] = {
    0x73, 0x61, 0x6c, 0x74
  };

  static const uint8_t result1[] = {
    0x55, 0xac, 0x04, 0x6e, 0x56, 0xe3, 0x08, 0x9f,
    0xec, 0x16, 0x91, 0xc2, 0x25, 0x44, 0xb6, 0x05,
    0xf9, 0x41, 0x85, 0x21, 0x6d, 0xde, 0x04, 0x65,
    0xe6, 0x8b, 0x9d, 0x57, 0xc2, 0x0d, 0xac, 0xbc,
    0x49, 0xca, 0x9c, 0xcc, 0xf1, 0x79, 0xb6, 0x45, 
    0x99, 0x16, 0x64, 0xb3, 0x9d, 0x77, 0xef, 0x31,
    0x7c, 0x71, 0xb8, 0x45, 0xb1, 0xe3, 0x0b, 0xd5,
    0x09, 0x11, 0x20, 0x41, 0xd3, 0xa1, 0x97, 0x83
  };

  static const uint8_t secret2[] = {
    0x50, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64
  };

  static const uint8_t salt2[] = {
    0x4e, 0x61, 0x43, 0x6c
  };

  static const uint8_t result2[] = {
    0x4d, 0xdc, 0xd8, 0xf6, 0x0b, 0x98, 0xbe, 0x21,
    0x83, 0x0c, 0xee, 0x5e, 0xf2, 0x27, 0x01, 0xf9,
    0x64, 0x1a, 0x44, 0x18, 0xd0, 0x4c, 0x04, 0x14,
    0xae, 0xff, 0x08, 0x87, 0x6b, 0x34, 0xab, 0x56,
    0xa1, 0xd4, 0x25, 0xa1, 0x22, 0x58, 0x33, 0x54,
    0x9a, 0xdb, 0x84, 0x1b, 0x51, 0xc9, 0xb3, 0x17,
    0x6a, 0x27, 0x2b, 0xde, 0xbb, 0xa1, 0xd0, 0x78,
    0x47, 0x8f, 0x62, 0xb3, 0x97, 0xf3, 0x3c, 0x8d
  };

  uint8_t result[64];

  earworm_PBKDF2_SHA256(secret1, sizeof secret1,
			salt1, sizeof salt1,
			1, result, sizeof result);
  if(memcmp(result1, result, sizeof result))
    return -1;

  earworm_PBKDF2_SHA256(secret2, sizeof secret2,
			salt2, sizeof salt2,
			80000, result, sizeof result);
  if(memcmp(result2, result, sizeof result))
    return -1;

  return 0;
}

static int test_aesenc_round() {
  /* Test vector from http://download-software.intel.com/sites/default/files/article/165683/aes-wp-2012-09-22-v01.pdf */

  static const uint8_t state_input[AES_BLOCK_SIZE] = {
    0x5d, 0x47, 0x53, 0x5d, 0x72, 0x6f, 0x74, 0x63,
    0x65, 0x56, 0x74, 0x73, 0x65, 0x54, 0x5b, 0x7b
  };

  static const uint8_t roundkey[AES_BLOCK_SIZE] = {
    0x5d, 0x6e, 0x6f, 0x72, 0x65, 0x75, 0x47, 0x5b,
    0x29, 0x79, 0x61, 0x68, 0x53, 0x28, 0x69, 0x48
  };

  static const uint8_t expected_result[AES_BLOCK_SIZE] = {
    0x95, 0xe5, 0xd7, 0xde, 0x58, 0x4b, 0x10, 0x8b,
    0xc5, 0xa3, 0xdb, 0x9f, 0x2f, 0x1c, 0x31, 0xa8
  };

  uint8_t state[AES_BLOCK_SIZE];

  memcpy(state, state_input, AES_BLOCK_SIZE);
  earworm_aesenc_round(state, roundkey);
  if(memcmp(state, expected_result, AES_BLOCK_SIZE) == 0)
    return 0;
  else return -1;
}

static int test_aes256enc() {
  /* Test vector from FIPS-197 */
  static const uint8_t plaintext[AES_BLOCK_SIZE] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
  };

  static const uint8_t userkey[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
  };

  static const uint8_t expected_result[AES_BLOCK_SIZE] = {
    0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
    0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89
  };

  uint8_t block[AES_BLOCK_SIZE];
  aeskey_t key;

  earworm_aes256enc_keysetup(userkey, &key);
  
  memcpy(block, plaintext, sizeof block);
  earworm_aes256enc(block, &key);

  if(memcmp(block, expected_result, sizeof block) == 0)
    return 0;
  else return -1;
}

static int run_phs(uint8_t *out, size_t outlen, 
                   const uint8_t *in, size_t inlen,
                   const uint8_t *salt, size_t saltlen,
                   unsigned int t_cost, unsigned int m_cost) {

  struct timeval start, end;
  int in_printable = 1;
  int salt_printable = 1;
  size_t i;

  for(i=0; i<inlen; i++)
    if(in[i] < ' ' || in[i] > '~' || in[i] == '"')
      in_printable = 0;

  for(i=0; i<saltlen; i++)
    if(salt[i] < ' ' || salt[i] > '~' || salt[i] == '"')
      salt_printable = 0;

  printf("PHS(");
  if(in_printable)
    printf("\"%s\", ", in);
  else { 
    for(i=0; i<inlen && i<16; i++)
      printf("%.2hhx", in[i]);
    if(inlen > 16)
      printf("...");
    printf(", ");
  }

  if(salt_printable)
    printf("\"%s\", ", salt);
  else { 
    for(i=0; i<saltlen && i<16; i++)
      printf("%.2hhx", salt[i]);
    if(saltlen > 16)
      printf("...");
    printf(", ");
  }

  printf("%d, %d) = ", t_cost, m_cost);
  fflush(stdout);

  gettimeofday(&start, NULL);

  if(PHS(out, outlen, in, inlen, salt, saltlen, t_cost, m_cost) != 0)
    return -1;

  gettimeofday(&end, NULL);
  
  for(i=0; i<outlen; i++)
    printf("%.2hhx", out[i]);
  
  printf(" (%ldus)\n",
         1000000*(end.tv_sec - start.tv_sec) + 
         end.tv_usec - start.tv_usec);
  return 0;
}

static void* phs_thread(void *thread_no) {
  uint8_t out[16];
  
  struct timeval start, end;
  
  gettimeofday(&start, NULL);
  PHS(out, sizeof out, (uint8_t*)"secret", 6, thread_no, 4, 10000, 16);
  gettimeofday(&end, NULL);
  printf("Thread %d: %ldus\n", *((uint32_t*)thread_no),
         1000000*(end.tv_sec - start.tv_sec) + 
         end.tv_usec - start.tv_usec);

  return NULL;
};
         
static int run_test(char *name, int (*test)()) {
  int result = test();
  printf("%-30s\t%s\n", name, result == 0 ? "PASS" : "FAIL");
  return result;
}

#define check(test) run_test(#test, test)

int main() {
  uint8_t out[16];
  pthread_t thread[16];
  uint32_t thread_no[16] = { 0, 1, 2, 3 ,4, 5, 6, 7, 8,
                             9, 10, 11, 12, 13, 14, 15 };
  int i;

  printf("Verifying known test vectors...\n");
  if(check(test_be32enc) |
     check(test_be32dec) |
     check(test_be64enc) |
     check(test_be64dec) |
     check(test_SHA256) |
     check(test_HMAC_SHA256) |
     check(test_PBKDF2_SHA256) |
     check(test_aesenc_round) |
     check(test_aes256enc)) {
    printf("Some known test vectors failed.\n");
    return 1;
  }

  printf("All known test vectors passed.\nInitializing test arena... ");
  fflush(stdout);
  PHS_initialize_arena(16);
  printf("done\n");

  run_phs(out, sizeof out,
          (const uint8_t*)"secret", 6,
          (const uint8_t*)"salt", 4,
          10000, 16);

  printf("Running 16 threads...\n");


  for(i=0; i < 16; i++)
    pthread_create(&thread[i], NULL, phs_thread, &thread_no[i]);

  for(i=0; i < 16; i++)
    pthread_join(thread[i], NULL);

  return 0;
}
