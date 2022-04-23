#ifndef _TFTP_LDR_H_
#define _TFTP_LDR_H_

#define DEBUG_LOGGING         1
// #define VERBOSE_DEBUG_LOGGING 1
#include "log.h"

typedef enum {
   TFTP_TYPE_RAM = 1,
   TFTP_TYPE_FLASH,
} TransferType_t;

#define MAX_FILENAME_LEN   32
typedef struct {
   ip_addr_t ServerIP;
   char Filename[MAX_FILENAME_LEN];
   int BytesTransfered;
   int MaxBytes;
   TransferType_t TransferType;
   char *Ram;
   uint32_t FlashAdr;
} tftp_ldr_internal;

err_t ldr_tftp_init(tftp_ldr_internal *p);

#endif // _TFTP_LDR_H_
