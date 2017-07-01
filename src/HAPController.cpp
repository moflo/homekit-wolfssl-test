//
//  HAPController.cpp
//  homekit-wolfssl
//
//  Created by d. nye on 6/30/17.
//  Copyright © 2017 Mobile Flow LLC. All rights reserved.
//

#include "HAPController.h"


int
getChallenge(uint8_t **salt,uint16_t *salt_len, uint8_t **key, uint16_t *key_len)
{
    Srp cli, srv;

    int r = 0;
    byte username[] = "alice";
    word32 usernameSz = 5;

    byte password[] = "password123";
    word32 passwordSz = 11;

    if (!r) r = wc_SrpInit(&cli, SRP_TYPE_SHA512, SRP_CLIENT_SIDE);
    if (!r) r = wc_SrpSetUsername(&cli, username, usernameSz);
    
    /* loading N, g and salt in advance to generate the verifier. */
    byte salt_3072[] = {
        0xBE, 0xB2, 0x53, 0x79, 0xD1, 0xA8, 0x58, 0x1E, 0xB5, 0xA7, 0x27, 0x67, 0x3A, 0x24,
        0x41, 0xEE
    };

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
    
    byte verifier[80];
    word32 v_size = sizeof(verifier);

    
    if (!r) r = wc_SrpSetParams(&cli, N,    sizeof(N),
                                g,    sizeof(g),
                                salt_3072, sizeof(salt_3072));
    if (!r) r = wc_SrpSetPassword(&cli, password, passwordSz);
    if (!r) r = wc_SrpGetVerifier(&cli, verifier, &v_size);
    
    *key = (uint8_t *)verifier;
    key_len = (uint16_t *)&v_size;
    
    return r;
    
}


int
verifySession(uint8_t **serverKeyProof, uint16_t *proof_len, uint8_t *clientPublicKey, uint16_t client_key_len, uint8_t *clientKeyProof, uint16_t client_proof_len)
{

    return 0;
    
}
