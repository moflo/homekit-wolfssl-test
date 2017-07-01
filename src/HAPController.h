//
//  HAPController.hpp
//  homekit-wolfssl
//
//  Created by d. nye on 6/30/17.
//  Copyright © 2017 Mobile Flow LLC. All rights reserved.
//

#ifndef HAPController_H
#define HAPController_H

#include <stdlib.h>
#include <string.h>

#define WOLFSSL_SHA512
#define WOLFCRYPT_HAVE_SRP

#include "srp.h"


int getChallenge(uint8_t **salt,uint32_t *salt_len, uint8_t **key, uint32_t *key_len);

int verifySession(uint8_t **serverKeyProof, uint16_t *proof_len,
                  uint8_t *clientPublicKey, uint16_t client_key_len,
                  uint8_t *serverPublicKey, uint16_t server_key_len);


#endif /* HAPController_hpp */
