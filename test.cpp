// Basic c++ test bench for testing Particle C code
/*
   g++ test.cpp tlv8.cpp -Wall -ggdb -std=c++0x -I. -O3 -fpermissive
   gcc2minix < a.out >test
   chmod +x test   # may not be necessary depending on your umask
   ./test

   Use online HEXDump tools to debug output: https://hexed.it

*/

#include "src/HAPController.h"


#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

using namespace std;

#define WOLFSSL_SHA512
#define WOLFCRYPT_HAVE_SRP

#include "srp.h"


void print_hex_memory(void *mem, int count) {
  int i;
  unsigned char *p = (unsigned char *)mem;
  for (i=0; i < count; i++) {
    printf("0x%02x ", p[i]);
    if ((i%16==0) && i)
      printf("\n");
  }
  printf("\n");
}

void assertNotEqual(uint8_t mem1, uint8_t mem2, const char * description, int * error_count ) {
    
    if (mem1 != mem2) {
        cout << "Error - " << description << ", got : " << (unsigned)mem1 << endl;
        (*error_count)++;
    }
}

/* for async devices */
static int devId = INVALID_DEVID;

static int generate_random_salt(byte *buf, word32 size)
{
    int ret = -5821;
    WC_RNG rng;
    
    if(NULL == buf || !size)
        return -5822;
    
    if (buf && size && wc_InitRng_ex(&rng, NULL, devId) == 0) {
        ret = wc_RNG_GenerateBlock(&rng, (byte *)buf, size);
        
        wc_FreeRng(&rng);
    }
    
    return ret;
}


int main()
{
    cout << "Starting Test.cpp ...\n\n" << endl;
    
    try {

        int error_count = 0;
        {
            cout << "WoflSSL Test - SRP ..." << endl;
            
            Srp srp;
            
            wc_SrpInit(&srp, SRP_TYPE_SHA512, SRP_SERVER_SIDE);

            wc_SrpTerm(&srp);
            

        }
        {
            cout << "WolfSSL test - SHA512 ..." << endl;
            
            Sha512 sha;
            byte   hash[SHA512_DIGEST_SIZE];
            byte   hashcopy[SHA512_DIGEST_SIZE];
            int    ret;

            typedef struct testVector {
                const char*  input;
                const char*  output;
                size_t inLen;
                size_t outLen;
            } testVector;

            testVector a, b;
            testVector test_sha[2];
            int times = sizeof(test_sha) / sizeof(struct testVector), i;
            
            a.input  = "abc";
            a.output = "\xdd\xaf\x35\xa1\x93\x61\x7a\xba\xcc\x41\x73\x49\xae\x20\x41"
            "\x31\x12\xe6\xfa\x4e\x89\xa9\x7e\xa2\x0a\x9e\xee\xe6\x4b\x55"
            "\xd3\x9a\x21\x92\x99\x2a\x27\x4f\xc1\xa8\x36\xba\x3c\x23\xa3"
            "\xfe\xeb\xbd\x45\x4d\x44\x23\x64\x3c\xe8\x0e\x2a\x9a\xc9\x4f"
            "\xa5\x4c\xa4\x9f";
            a.inLen  = XSTRLEN(a.input);
            a.outLen = SHA512_DIGEST_SIZE;
            
            b.input  = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhi"
            "jklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
            b.output = "\x8e\x95\x9b\x75\xda\xe3\x13\xda\x8c\xf4\xf7\x28\x14\xfc\x14"
            "\x3f\x8f\x77\x79\xc6\xeb\x9f\x7f\xa1\x72\x99\xae\xad\xb6\x88"
            "\x90\x18\x50\x1d\x28\x9e\x49\x00\xf7\xe4\x33\x1b\x99\xde\xc4"
            "\xb5\x43\x3a\xc7\xd3\x29\xee\xb6\xdd\x26\x54\x5e\x96\xe5\x5b"
            "\x87\x4b\xe9\x09";
            b.inLen  = XSTRLEN(b.input);
            b.outLen = SHA512_DIGEST_SIZE;
            
            test_sha[0] = a;
            test_sha[1] = b;


            ret = wc_InitSha512_ex(&sha, NULL, devId);
            if (ret != 0)
                return -2200;
            
            for (i = 0; i < times; ++i) {
                ret = wc_Sha512Update(&sha, (byte*)test_sha[i].input,(word32)test_sha[i].inLen);
                if (ret != 0)
                    return -2210 - i;
                ret = wc_Sha512GetHash(&sha, hashcopy);
                if (ret != 0)
                    return -2220 - i;
                ret = wc_Sha512Final(&sha, hash);
                if (ret != 0)
                    return -2230 - i;
                
                if (XMEMCMP(hash, test_sha[i].output, SHA512_DIGEST_SIZE) != 0)
                    return -2240 - i;
                if (XMEMCMP(hash, hashcopy, SHA512_DIGEST_SIZE) != 0)
                    return -2250 - i;
            }
            
            wc_Sha512Free(&sha);
            
            assertNotEqual(ret, 0, "SHA512 test failed - code ", &error_count);

        }
        
        {

            cout << "WolfSSL test - SRP ..." << endl;

            Srp cli, srv;
            int r;
            
            byte clientPubKey[80]; /* A */
            byte serverPubKey[80]; /* B */
            word32 clientPubKeySz = 80;
            word32 serverPubKeySz = 80;
            byte clientProof[SRP_MAX_DIGEST_SIZE]; /* M1 */
            byte serverProof[SRP_MAX_DIGEST_SIZE]; /* M2 */
            word32 clientProofSz = SRP_MAX_DIGEST_SIZE;
            word32 serverProofSz = SRP_MAX_DIGEST_SIZE;
            
            byte username[] = "user";
            word32 usernameSz = 4;
            
            byte password[] = "password";
            word32 passwordSz = 8;
            
            byte N[] = {
                0xC9, 0x4D, 0x67, 0xEB, 0x5B, 0x1A, 0x23, 0x46, 0xE8, 0xAB, 0x42, 0x2F,
                0xC6, 0xA0, 0xED, 0xAE, 0xDA, 0x8C, 0x7F, 0x89, 0x4C, 0x9E, 0xEE, 0xC4,
                0x2F, 0x9E, 0xD2, 0x50, 0xFD, 0x7F, 0x00, 0x46, 0xE5, 0xAF, 0x2C, 0xF7,
                0x3D, 0x6B, 0x2F, 0xA2, 0x6B, 0xB0, 0x80, 0x33, 0xDA, 0x4D, 0xE3, 0x22,
                0xE1, 0x44, 0xE7, 0xA8, 0xE9, 0xB1, 0x2A, 0x0E, 0x46, 0x37, 0xF6, 0x37,
                0x1F, 0x34, 0xA2, 0x07, 0x1C, 0x4B, 0x38, 0x36, 0xCB, 0xEE, 0xAB, 0x15,
                0x03, 0x44, 0x60, 0xFA, 0xA7, 0xAD, 0xF4, 0x83
            };
            
            byte g[] = {
                0x02
            };
            
            byte salt[10];
            
            byte verifier[80];
            word32 v_size = sizeof(verifier);
            
            /* set as 0's so if second init on srv not called SrpTerm is not on
             * garbage values */
            XMEMSET(&srv, 0, sizeof(Srp));
            XMEMSET(&cli, 0, sizeof(Srp));
            
            /* generating random salt */
            
            r = generate_random_salt(salt, sizeof(salt));
            
            /* client knows username and password.   */
            /* server knows N, g, salt and verifier. */
            
            if (!r) r = wc_SrpInit(&cli, SRP_TYPE_SHA, SRP_CLIENT_SIDE);
            if (!r) r = wc_SrpSetUsername(&cli, username, usernameSz);
            
            /* loading N, g and salt in advance to generate the verifier. */
            
            if (!r) r = wc_SrpSetParams(&cli, N,    sizeof(N),
                                        g,    sizeof(g),
                                        salt, sizeof(salt));
            if (!r) r = wc_SrpSetPassword(&cli, password, passwordSz);
            if (!r) r = wc_SrpGetVerifier(&cli, verifier, &v_size);
            
            /* client sends username to server */
            
            if (!r) r = wc_SrpInit(&srv, SRP_TYPE_SHA, SRP_SERVER_SIDE);
            if (!r) r = wc_SrpSetUsername(&srv, username, usernameSz);
            if (!r) r = wc_SrpSetParams(&srv, N,    sizeof(N),
                                        g,    sizeof(g),
                                        salt, sizeof(salt));
            if (!r) r = wc_SrpSetVerifier(&srv, verifier, v_size);
            if (!r) r = wc_SrpGetPublic(&srv, serverPubKey, &serverPubKeySz);
            
            /* server sends N, g, salt and B to client */
            
            if (!r) r = wc_SrpGetPublic(&cli, clientPubKey, &clientPubKeySz);
            if (!r) r = wc_SrpComputeKey(&cli, clientPubKey, clientPubKeySz,
                                         serverPubKey, serverPubKeySz);
            if (!r) r = wc_SrpGetProof(&cli, clientProof, &clientProofSz);
            
            /* client sends A and M1 to server */
            
            if (!r) r = wc_SrpComputeKey(&srv, clientPubKey, clientPubKeySz,
                                         serverPubKey, serverPubKeySz);
            if (!r) r = wc_SrpVerifyPeersProof(&srv, clientProof, clientProofSz);
            if (!r) r = wc_SrpGetProof(&srv, serverProof, &serverProofSz);
            
            /* server sends M2 to client */
            
            if (!r) r = wc_SrpVerifyPeersProof(&cli, serverProof, serverProofSz);
            
            wc_SrpTerm(&cli);
            wc_SrpTerm(&srv);
            
//            return r;
            assertNotEqual(r, 0, "SRP test failed - code ", &error_count);

        }

        {
            cout << "HomeKit - SRP test ..." << endl;

            uint8_t *s = NULL;
            uint32_t s_len = 0;
            uint8_t *v = NULL;
            uint32_t v_len = 0;
            
            getChallenge(&s,&s_len,&v,&v_len);
            
            assertNotEqual(v_len, 384, "length of v not equal to 128", &error_count);
            
            static const uint8_t srp_3072_v[] = {
                0x9B, 0x5E, 0x06, 0x17, 0x01, 0xEA, 0x7A, 0xEB, 0x39, 0xCF, 0x6E, 0x35, 0x19, 0x65,
                0x5A, 0x85, 0x3C, 0xF9, 0x4C, 0x75, 0xCA, 0xF2, 0x55, 0x5E, 0xF1, 0xFA, 0xF7, 0x59,
                0xBB, 0x79, 0xCB, 0x47, 0x70, 0x14, 0xE0, 0x4A, 0x88, 0xD6, 0x8F, 0xFC, 0x05, 0x32,
                0x38, 0x91, 0xD4, 0xC2, 0x05, 0xB8, 0xDE, 0x81, 0xC2, 0xF2, 0x03, 0xD8, 0xFA, 0xD1,
                0xB2, 0x4D, 0x2C, 0x10, 0x97, 0x37, 0xF1, 0xBE, 0xBB, 0xD7, 0x1F, 0x91, 0x24, 0x47,
                0xC4, 0xA0, 0x3C, 0x26, 0xB9, 0xFA, 0xD8, 0xED, 0xB3, 0xE7, 0x80, 0x77, 0x8E, 0x30,
                0x25, 0x29, 0xED, 0x1E, 0xE1, 0x38, 0xCC, 0xFC, 0x36, 0xD4, 0xBA, 0x31, 0x3C, 0xC4,
                0x8B, 0x14, 0xEA, 0x8C, 0x22, 0xA0, 0x18, 0x6B, 0x22, 0x2E, 0x65, 0x5F, 0x2D, 0xF5,
                0x60, 0x3F, 0xD7, 0x5D, 0xF7, 0x6B, 0x3B, 0x08, 0xFF, 0x89, 0x50, 0x06, 0x9A, 0xDD,
                0x03, 0xA7, 0x54, 0xEE, 0x4A, 0xE8, 0x85, 0x87, 0xCC, 0xE1, 0xBF, 0xDE, 0x36, 0x79,
                0x4D, 0xBA, 0xE4, 0x59, 0x2B, 0x7B, 0x90, 0x4F, 0x44, 0x2B, 0x04, 0x1C, 0xB1, 0x7A,
                0xEB, 0xAD, 0x1E, 0x3A, 0xEB, 0xE3, 0xCB, 0xE9, 0x9D, 0xE6, 0x5F, 0x4B, 0xB1, 0xFA,
                0x00, 0xB0, 0xE7, 0xAF, 0x06, 0x86, 0x3D, 0xB5, 0x3B, 0x02, 0x25, 0x4E, 0xC6, 0x6E,
                0x78, 0x1E, 0x3B, 0x62, 0xA8, 0x21, 0x2C, 0x86, 0xBE, 0xB0, 0xD5, 0x0B, 0x5B, 0xA6,
                0xD0, 0xB4, 0x78, 0xD8, 0xC4, 0xE9, 0xBB, 0xCE, 0xC2, 0x17, 0x65, 0x32, 0x6F, 0xBD,
                0x14, 0x05, 0x8D, 0x2B, 0xBD, 0xE2, 0xC3, 0x30, 0x45, 0xF0, 0x38, 0x73, 0xE5, 0x39,
                0x48, 0xD7, 0x8B, 0x79, 0x4F, 0x07, 0x90, 0xE4, 0x8C, 0x36, 0xAE, 0xD6, 0xE8, 0x80,
                0xF5, 0x57, 0x42, 0x7B, 0x2F, 0xC0, 0x6D, 0xB5, 0xE1, 0xE2, 0xE1, 0xD7, 0xE6, 0x61,
                0xAC, 0x48, 0x2D, 0x18, 0xE5, 0x28, 0xD7, 0x29, 0x5E, 0xF7, 0x43, 0x72, 0x95, 0xFF,
                0x1A, 0x72, 0xD4, 0x02, 0x77, 0x17, 0x13, 0xF1, 0x68, 0x76, 0xDD, 0x05, 0x0A, 0xE5,
                0xB7, 0xAD, 0x53, 0xCC, 0xB9, 0x08, 0x55, 0xC9, 0x39, 0x56, 0x64, 0x83, 0x58, 0xAD,
                0xFD, 0x96, 0x64, 0x22, 0xF5, 0x24, 0x98, 0x73, 0x2D, 0x68, 0xD1, 0xD7, 0xFB, 0xEF,
                0x10, 0xD7, 0x80, 0x34, 0xAB, 0x8D, 0xCB, 0x6F, 0x0F, 0xCF, 0x88, 0x5C, 0xC2, 0xB2,
                0xEA, 0x2C, 0x3E, 0x6A, 0xC8, 0x66, 0x09, 0xEA, 0x05, 0x8A, 0x9D, 0xA8, 0xCC, 0x63,
                0x53, 0x1D, 0xC9, 0x15, 0x41, 0x4D, 0xF5, 0x68, 0xB0, 0x94, 0x82, 0xDD, 0xAC, 0x19,
                0x54, 0xDE, 0xC7, 0xEB, 0x71, 0x4F, 0x6F, 0xF7, 0xD4, 0x4C, 0xD5, 0xB8, 0x6F, 0x6B,
                0xD1, 0x15, 0x81, 0x09, 0x30, 0x63, 0x7C, 0x01, 0xD0, 0xF6, 0x01, 0x3B, 0xC9, 0x74,
                0x0F, 0xA2, 0xC6, 0x33, 0xBA, 0x89
            };
            
            int compareV = memcmp(&srp_3072_v, v, v_len);
            assertNotEqual(compareV, 0, "byte comparison failed", &error_count);
            
            cout << "HomeKit - SRP compute proof test ..." << endl;
            
            // Generate test key & proof
            
            /* client sends username to server */
            Srp srv;
            int r = 0;
            
            byte serverPubKey[384]; /* B */
            word32 serverPubKeySz = 384;
            byte serverProof[SRP_MAX_DIGEST_SIZE]; /* M2 */
            word32 serverProofSz = SRP_MAX_DIGEST_SIZE;
            
            byte username[] = "alice";
            word32 usernameSz = 5;
            
            byte password[] = "password123";
            word32 passwordSz = 11;
            
            byte salt_3072[] = {
                0xBE, 0xB2, 0x53, 0x79, 0xD1, 0xA8, 0x58, 0x1E, 0xB5, 0xA7, 0x27, 0x67, 0x3A, 0x24,
                0x41, 0xEE
            };
            
            byte N[] = {
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F,
                0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34, 0xC4, 0xC6, 0x62, 0x8B,
                0x80, 0xDC, 0x1C, 0xD1, 0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67,
                0xCC, 0x74, 0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22,
                0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD, 0xEF, 0x95,
                0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D,
                0xF2, 0x5F, 0x14, 0x37, 0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51,
                0xC2, 0x45, 0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
                0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF,
                0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED, 0xEE, 0x38, 0x6B, 0xFB,
                0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B,
                0x1F, 0xE6, 0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D,
                0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05, 0x98, 0xDA,
                0x48, 0x36, 0x1C, 0x55, 0xD3, 0x9A, 0x69, 0x16, 0x3F, 0xA8,
                0xFD, 0x24, 0xCF, 0x5F, 0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3,
                0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB,
                0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D, 0x67, 0x0C,
                0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04, 0xF1, 0x74, 0x6C, 0x08,
                0xCA, 0x18, 0x21, 0x7C, 0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36,
                0xCE, 0x3B, 0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03,
                0x9B, 0x27, 0x83, 0xA2, 0xEC, 0x07, 0xA2, 0x8F, 0xB5, 0xC5,
                0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9, 0xDE, 0x2B, 0xCB, 0xF6,
                0x95, 0x58, 0x17, 0x18, 0x39, 0x95, 0x49, 0x7C, 0xEA, 0x95,
                0x6A, 0xE5, 0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
                0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAA, 0xC4, 0x2D, 0xAD, 0x33,
                0x17, 0x0D, 0x04, 0x50, 0x7A, 0x33, 0xA8, 0x55, 0x21, 0xAB,
                0xDF, 0x1C, 0xBA, 0x64, 0xEC, 0xFB, 0x85, 0x04, 0x58, 0xDB,
                0xEF, 0x0A, 0x8A, 0xEA, 0x71, 0x57, 0x5D, 0x06, 0x0C, 0x7D,
                0xB3, 0x97, 0x0F, 0x85, 0xA6, 0xE1, 0xE4, 0xC7, 0xAB, 0xF5,
                0xAE, 0x8C, 0xDB, 0x09, 0x33, 0xD7, 0x1E, 0x8C, 0x94, 0xE0,
                0x4A, 0x25, 0x61, 0x9D, 0xCE, 0xE3, 0xD2, 0x26, 0x1A, 0xD2,
                0xEE, 0x6B, 0xF1, 0x2F, 0xFA, 0x06, 0xD9, 0x8A, 0x08, 0x64,
                0xD8, 0x76, 0x02, 0x73, 0x3E, 0xC8, 0x6A, 0x64, 0x52, 0x1F,
                0x2B, 0x18, 0x17, 0x7B, 0x20, 0x0C, 0xBB, 0xE1, 0x17, 0x57,
                0x7A, 0x61, 0x5D, 0x6C, 0x77, 0x09, 0x88, 0xC0, 0xBA, 0xD9,
                0x46, 0xE2, 0x08, 0xE2, 0x4F, 0xA0, 0x74, 0xE5, 0xAB, 0x31, 
                0x43, 0xDB, 0x5B, 0xFC, 0xE0, 0xFD, 0x10, 0x8E, 0x4B, 0x82, 
                0xD1, 0x20, 0xA9, 0x3A, 0xD2, 0xCA, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF
            };
            
            byte g[] = {
                0x05
            };

            if (!r) r = wc_SrpInit(&srv, SRP_TYPE_SHA512, SRP_SERVER_SIDE);
            if (!r) r = wc_SrpSetUsername(&srv, username, usernameSz);
            if (!r) r = wc_SrpSetParams(&srv, N,    sizeof(N),
                                        g,    sizeof(g),
                                        salt_3072, sizeof(salt_3072));
            byte *verifier = v;
            word32 v_size = v_len;

            if (!r) r = wc_SrpSetVerifier(&srv, verifier, v_size);
            if (!r) r = wc_SrpGetPublic(&srv, serverPubKey, &serverPubKeySz);

            
            uint8_t *p;
            uint16_t p_len;
            
            verifySession(&p,&p_len,s,s_len,serverPubKey,serverPubKeySz);
            
            assertNotEqual(p_len, 64, "length of B proof not equal to 128", &error_count);


            static const uint8_t srp_3027_B[] = {
                0x40, 0xF5, 0x70, 0x88, 0xA4, 0x82, 0xD4, 0xC7, 0x73, 0x33, 0x84, 0xFE, 0x0D, 0x30,
                0x1F, 0xDD, 0xCA, 0x90, 0x80, 0xAD, 0x7D, 0x4F, 0x6F, 0xDF, 0x09, 0xA0, 0x10, 0x06,
                0xC3, 0xCB, 0x6D, 0x56, 0x2E, 0x41, 0x63, 0x9A, 0xE8, 0xFA, 0x21, 0xDE, 0x3B, 0x5D,
                0xBA, 0x75, 0x85, 0xB2, 0x75, 0x58, 0x9B, 0xDB, 0x27, 0x98, 0x63, 0xC5, 0x62, 0x80,
                0x7B, 0x2B, 0x99, 0x08, 0x3C, 0xD1, 0x42, 0x9C, 0xDB, 0xE8, 0x9E, 0x25, 0xBF, 0xBD,
                0x7E, 0x3C, 0xAD, 0x31, 0x73, 0xB2, 0xE3, 0xC5, 0xA0, 0xB1, 0x74, 0xDA, 0x6D, 0x53,
                0x91, 0xE6, 0xA0, 0x6E, 0x46, 0x5F, 0x03, 0x7A, 0x40, 0x06, 0x25, 0x48, 0x39, 0xA5,
                0x6B, 0xF7, 0x6D, 0xA8, 0x4B, 0x1C, 0x94, 0xE0, 0xAE, 0x20, 0x85, 0x76, 0x15, 0x6F,
                0xE5, 0xC1, 0x40, 0xA4, 0xBA, 0x4F, 0xFC, 0x9E, 0x38, 0xC3, 0xB0, 0x7B, 0x88, 0x84,
                0x5F, 0xC6, 0xF7, 0xDD, 0xDA, 0x93, 0x38, 0x1F, 0xE0, 0xCA, 0x60, 0x84, 0xC4, 0xCD,
                0x2D, 0x33, 0x6E, 0x54, 0x51, 0xC4, 0x64, 0xCC, 0xB6, 0xEC, 0x65, 0xE7, 0xD1, 0x6E,
                0x54, 0x8A, 0x27, 0x3E, 0x82, 0x62, 0x84, 0xAF, 0x25, 0x59, 0xB6, 0x26, 0x42, 0x74,
                0x21, 0x59, 0x60, 0xFF, 0xF4, 0x7B, 0xDD, 0x63, 0xD3, 0xAF, 0xF0, 0x64, 0xD6, 0x13,
                0x7A, 0xF7, 0x69, 0x66, 0x1C, 0x9D, 0x4F, 0xEE, 0x47, 0x38, 0x26, 0x03, 0xC8, 0x8E,
                0xAA, 0x09, 0x80, 0x58, 0x1D, 0x07, 0x75, 0x84, 0x61, 0xB7, 0x77, 0xE4, 0x35, 0x6D,
                0xDA, 0x58, 0x35, 0x19, 0x8B, 0x51, 0xFE, 0xEA, 0x30, 0x8D, 0x70, 0xF7, 0x54, 0x50,
                0xB7, 0x16, 0x75, 0xC0, 0x8C, 0x7D, 0x83, 0x02, 0xFD, 0x75, 0x39, 0xDD, 0x1F, 0xF2,
                0xA1, 0x1C, 0xB4, 0x25, 0x8A, 0xA7, 0x0D, 0x23, 0x44, 0x36, 0xAA, 0x42, 0xB6, 0xA0,
                0x61, 0x5F, 0x3F, 0x91, 0x5D, 0x55, 0xCC, 0x3B, 0x96, 0x6B, 0x27, 0x16, 0xB3, 0x6E,
                0x4D, 0x1A, 0x06, 0xCE, 0x5E, 0x5D, 0x2E, 0xA3, 0xBE, 0xE5, 0xA1, 0x27, 0x0E, 0x87,
                0x51, 0xDA, 0x45, 0xB6, 0x0B, 0x99, 0x7B, 0x0F, 0xFD, 0xB0, 0xF9, 0x96, 0x2F, 0xEE,
                0x4F, 0x03, 0xBE, 0xE7, 0x80, 0xBA, 0x0A, 0x84, 0x5B, 0x1D, 0x92, 0x71, 0x42, 0x17,
                0x83, 0xAE, 0x66, 0x01, 0xA6, 0x1E, 0xA2, 0xE3, 0x42, 0xE4, 0xF2, 0xE8, 0xBC, 0x93,
                0x5A, 0x40, 0x9E, 0xAD, 0x19, 0xF2, 0x21, 0xBD, 0x1B, 0x74, 0xE2, 0x96, 0x4D, 0xD1,
                0x9F, 0xC8, 0x45, 0xF6, 0x0E, 0xFC, 0x09, 0x33, 0x8B, 0x60, 0xB6, 0xB2, 0x56, 0xD8, 
                0xCA, 0xC8, 0x89, 0xCC, 0xA3, 0x06, 0xCC, 0x37, 0x0A, 0x0B, 0x18, 0xC8, 0xB8, 0x86, 
                0xE9, 0x5D, 0xA0, 0xAF, 0x52, 0x35, 0xFE, 0xF4, 0x39, 0x30, 0x20, 0xD2, 0xB7, 0xF3, 
                0x05, 0x69, 0x04, 0x75, 0x90, 0x42
            };
            int compareB = memcmp(&srp_3027_B, p, p_len);
            assertNotEqual(compareB, 0, "byte comparison failed", &error_count);

        }
        {
#ifdef HAVE_ED25519
            static INLINE int myEd25519Sign(WOLFSSL* ssl, const byte* in, word32 inSz,
                                            byte* out, word32* outSz, const byte* key, word32 keySz, void* ctx)
            {
                int         ret;
                word32      idx = 0;
                ed25519_key myKey;
                
                
                ret = wc_ed25519_init(&myKey);
                if (ret == 0) {
                    ret = wc_Ed25519PrivateKeyDecode(key, &idx, &myKey, keySz);
                    if (ret == 0)
                        ret = wc_ed25519_sign_msg(in, inSz, out, outSz, &myKey);
                    wc_ed25519_free(&myKey);
                }
                
                return ret;
            }
            
            
            static INLINE int myEd25519Verify(WOLFSSL* ssl, const byte* sig, word32 sigSz,
                                              const byte* msg, word32 msgSz, const byte* key, word32 keySz,
                                              int* result, void* ctx)
            {
                int         ret;
                ed25519_key myKey;
                
                (void)ssl;
                (void)ctx;
                
                ret = wc_ed25519_init(&myKey);
                if (ret == 0) {
                    ret = wc_ed25519_import_public(key, keySz, &myKey);
                    if (ret == 0) {
                        ret = wc_ed25519_verify_msg(sig, sigSz, msg, msgSz, result, &myKey);
                    }
                    wc_ed25519_free(&myKey);
                }
                
                return ret;
            }
#endif /* HAVE_ED25519 */

        }
        {
#if defined(HAVE_SESSION_TICKET) && defined(HAVE_CHACHA) && \
defined(HAVE_POLY1305)
            
#include <wolfssl/wolfcrypt/chacha20_poly1305.h>
            
            typedef struct key_ctx {
                byte name[WOLFSSL_TICKET_NAME_SZ];        /* name for this context */
                byte key[CHACHA20_POLY1305_AEAD_KEYSIZE]; /* cipher key */
            } key_ctx;
            
            static key_ctx myKey_ctx;
            static WC_RNG myKey_rng;
            
            static INLINE int TicketInit(void)
            {
                int ret = wc_InitRng(&myKey_rng);
                if (ret != 0) return ret;
                
                ret = wc_RNG_GenerateBlock(&myKey_rng, myKey_ctx.key, sizeof(myKey_ctx.key));
                if (ret != 0) return ret;
                
                ret = wc_RNG_GenerateBlock(&myKey_rng, myKey_ctx.name,sizeof(myKey_ctx.name));
                if (ret != 0) return ret;
                
                return 0;
            }
            
            static INLINE void TicketCleanup(void)
            {
                wc_FreeRng(&myKey_rng);
            }
            
            static INLINE int myTicketEncCb(WOLFSSL* ssl,
                                            byte key_name[WOLFSSL_TICKET_NAME_SZ],
                                            byte iv[WOLFSSL_TICKET_IV_SZ],
                                            byte mac[WOLFSSL_TICKET_MAC_SZ],
                                            int enc, byte* ticket, int inLen, int* outLen,
                                            void* userCtx)
            {
                (void)ssl;
                (void)userCtx;
                
                int ret;
                word16 sLen = XHTONS(inLen);
                byte aad[WOLFSSL_TICKET_NAME_SZ + WOLFSSL_TICKET_IV_SZ + 2];
                int  aadSz = WOLFSSL_TICKET_NAME_SZ + WOLFSSL_TICKET_IV_SZ + 2;
                byte* tmp = aad;
                
                if (enc) {
                    XMEMCPY(key_name, myKey_ctx.name, WOLFSSL_TICKET_NAME_SZ);
                    
                    ret = wc_RNG_GenerateBlock(&myKey_rng, iv, WOLFSSL_TICKET_IV_SZ);
                    if (ret != 0) return WOLFSSL_TICKET_RET_REJECT;
                    
                    /* build aad from key name, iv, and length */
                    XMEMCPY(tmp, key_name, WOLFSSL_TICKET_NAME_SZ);
                    tmp += WOLFSSL_TICKET_NAME_SZ;
                    XMEMCPY(tmp, iv, WOLFSSL_TICKET_IV_SZ);
                    tmp += WOLFSSL_TICKET_IV_SZ;
                    XMEMCPY(tmp, &sLen, 2);
                    
                    ret = wc_ChaCha20Poly1305_Encrypt(myKey_ctx.key, iv,
                                                      aad, aadSz,
                                                      ticket, inLen,
                                                      ticket,
                                                      mac);
                    if (ret != 0) return WOLFSSL_TICKET_RET_REJECT;
                    *outLen = inLen;  /* no padding in this mode */
                } else {
                    /* decrypt */
                    
                    /* see if we know this key */
                    if (XMEMCMP(key_name, myKey_ctx.name, WOLFSSL_TICKET_NAME_SZ) != 0){
                        printf("client presented unknown ticket key name ");
                        return WOLFSSL_TICKET_RET_FATAL;
                    }
                    
                    /* build aad from key name, iv, and length */
                    XMEMCPY(tmp, key_name, WOLFSSL_TICKET_NAME_SZ);
                    tmp += WOLFSSL_TICKET_NAME_SZ;
                    XMEMCPY(tmp, iv, WOLFSSL_TICKET_IV_SZ);
                    tmp += WOLFSSL_TICKET_IV_SZ;
                    XMEMCPY(tmp, &sLen, 2);
                    
                    ret = wc_ChaCha20Poly1305_Decrypt(myKey_ctx.key, iv,
                                                      aad, aadSz,
                                                      ticket, inLen,
                                                      mac,
                                                      ticket);
                    if (ret != 0) return WOLFSSL_TICKET_RET_REJECT;
                    *outLen = inLen;  /* no padding in this mode */
                }
                
                return WOLFSSL_TICKET_RET_OK;
            }
            
#endif  /* HAVE_SESSION_TICKET && CHACHA20 && POLY1305 */
            
        }
        
        cout << "\n\nError count == " << error_count << endl;
        
        return 1;

    }

    catch (...)
	{
		cout << "unknown exception occured" << endl;
		cout << "!!! TEST VECTORS FAILURE !!!" << endl;
	}

	cout << "end!!! TEST VECTORS FAILURE !!!" << endl;
	return -1;

}


