#ifndef _TFTP_LDR_H_
#define _TFTP_LDR_H_

#define DEBUG_LOGGING         1
#define VERBOSE_DEBUG_LOGGING 1
#include "log.h"

typedef enum {
   TFTP_TYPE_RAM = 1,
   TFTP_TYPE_FLASH,
   TFTP_TYPE_COMPARE,
} TransferType_t;
#define TFTP_TYPE_LAST  TFTP_TYPE_COMPARE


typedef enum {
   TFTP_OK = 0,
   TFTP_ERR_BUF_TOO_SMALL,
   TFTP_ERR_COMPARE_FAIL,
   TFTP_IN_PROGRESS,
   TFTP_ERR_FAILED,
   TFTP_ERR_INTERNAL
} TransferResult_t;

#define MAX_FILENAME_LEN   32
#define MAX_ERR_MSG_LEN    32
typedef struct {
   ip_addr_t ServerIP;
   char Filename[MAX_FILENAME_LEN + 1];
   char ErrMsg[MAX_ERR_MSG_LEN + 1];
   int BytesTransfered;
   int MaxBytes;
   TransferType_t TransferType;
   char *Ram;
   uint32_t FlashAdr;
   TransferResult_t Error;
} tftp_ldr_internal;

err_t ldr_tftp_init(tftp_ldr_internal *p);

#endif // _TFTP_LDR_H_
