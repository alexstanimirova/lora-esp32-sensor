#pragma once

#ifndef _LORA_SECRETS_H_
#define _LORA_SECRETS_H_

// Keys required for OTAA activation:

// Application Identifier (u1_t[8]) in lsb format
#define OTAA_APPEUI 0x75, 0x9A, 0x7D, 0xF1, 0x60, 0xF9, 0x81, 0x60

// Application Key (u1_t[16]) in msb format
#define OTAA_APPKEY 0x2F, 0x77, 0xB8, 0x6A, 0x47, 0xCF, 0xB8, 0xF1, 0x94, 0x7C, 0x25, 0xF0, 0x24, 0x29, 0xBD, 0xF1

#endif // _LORA_SECRETS_H_