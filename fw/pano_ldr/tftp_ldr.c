/*
 *  Copyright (C) 2022  Skip Hansen
 * 
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms and conditions of the GNU General Public License,
 *  version 2, as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Dirk Ziegelmeier <dziegel@gmx.de>
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "lwip/apps/tftp_client.h"
// #include "lwip/apps/tftp_server.h"
#include "tftp_ldr.h"
#include "spi_drv.h"

// #define DEBUG_LOGGING         1
// #define VERBOSE_DEBUG_LOGGING 1
#include "log.h"

#define INVALID_FLASH_ADR     0xffffffff

static void *tftp_open(const char *p,const char *Mode,u8_t bWrite)
{
   LWIP_UNUSED_ARG(Mode);
   return NULL;
}

static void tftp_close(void* handle)
{
   tftp_ldr_internal *p = (tftp_ldr_internal *) handle;
   LOG("%d bytes transfered\n",p->BytesTransfered);
   if(p->Error == TFTP_IN_PROGRESS) {
      p->Error = TFTP_OK;
   }
}

// tftp put
static int tftp_read(void *handle,void *Buf, int Len)
{
   tftp_ldr_internal *p = (tftp_ldr_internal *) handle;
   int Ret;
   LOG("Called, Len %d\n",Len);

   if(p->TransferType == TFTP_TYPE_SAVE) {
      uint32_t Adr = p->FlashAdr + p->BytesTransfered;
      uint32_t Bytes2Read = Len;

      if(p->BytesTransfered + Len > p->SendLen) {
         Len = p->SendLen - p->BytesTransfered;
      }

      spi_read(p->FlashAdr + p->BytesTransfered,(uint8_t *) Buf,Len);
      p->BytesTransfered += Len;
      Ret = Len;
   }
   else {
      ELOG("Internal err\n");
      Ret = -1;
   }

   LOG("Returning %d\n",Ret);
   return Ret;
}

static int tftp_write(void *handle, struct pbuf *pBuf)
{
   tftp_ldr_internal *p = (tftp_ldr_internal *) handle;
   uint32_t Adr = p->FlashAdr + p->BytesTransfered;

   int Ret = 0;  // assume the best
   u16_t TotalLen = pBuf->tot_len;
   u16_t Len = 0;
   u16_t BufLen;

   while(Ret == 0 && Len < TotalLen) {
      BufLen = pBuf->len;
      switch(p->TransferType) {
         case TFTP_TYPE_RAM:
            if((p->BytesTransfered + TotalLen) > p->MaxBytes ) {
               p->Error = TFTP_ERR_BUF_TOO_SMALL;
               break;
            }
            memcpy(&p->Ram[p->BytesTransfered],pBuf->payload,BufLen);
            break;

         case TFTP_TYPE_FLASH: {
            const FlashInfo_t *pInfo = spi_get_flashinfo();
            uint32_t EraseSize = pInfo->EraseSize;
            uint32_t LastAdr = Adr + BufLen - 1;
            uint32_t EraseAdr;
            uint32_t EraseEnd = LastAdr - (LastAdr % EraseSize);

            if(p->bAutoErase && p->LastEraseAdr != EraseEnd) {
               if(p->LastEraseAdr == INVALID_FLASH_ADR) {
                  p->LastEraseAdr = Adr - (Adr % EraseSize);
               }
               else {
                  p->LastEraseAdr += EraseSize;
               }
               LOG("erase 0x%x\n",p->LastEraseAdr);
               spi_erase(p->LastEraseAdr,pInfo->EraseSize);
            }

            LOG("flash %d @ 0x%x\n",BufLen,Adr);
            spi_write(Adr,pBuf->payload,BufLen);
            break;
         }

         case TFTP_TYPE_COMPARE:
            LOG("comparing %d bytes\n",BufLen);
            if(BufLen > p->MaxBytes) {
               p->Error = TFTP_ERR_BUF_TOO_SMALL;
               break;
            }
            spi_read(Adr,p->Ram,BufLen);
            if(memcmp(p->Ram,pBuf->payload,BufLen) != 0) {
               int i;
               char *cp = pBuf->payload;
            // Find the exact point of failure
               for(i = 0; i < BufLen; i++) {
                  if(p->Ram[i] != *cp++) {
                     break;
                  }
                  p->BytesTransfered++;
               }

               LOG("Compare failed\n");
               p->Error = TFTP_ERR_COMPARE_FAIL;
            }
            break;

         default:
            p->Error = TFTP_ERR_INTERNAL;
            break;
      }

      if(p->Error != TFTP_IN_PROGRESS) {
         Ret = -1;
      }
      else {
         pBuf = pBuf->next;
         p->BytesTransfered += BufLen;
         Len += BufLen;
      }
   }

   LOG("Returning %d\n",Ret);

   return Ret;
}

/* For TFTP client only */
static void tftp_error(void *handle,int err,const char *Msg,int size)
{
   tftp_ldr_internal *p = (tftp_ldr_internal *) handle;

   ELOG("err %d: %s\n",err,Msg);
   if(size > MAX_ERR_MSG_LEN) {
      size = MAX_ERR_MSG_LEN;
   }
   memcpy(p->ErrMsg,Msg,size);
   p->ErrMsg[MAX_ERR_MSG_LEN] = 0;
   p->Error = TFTP_ERR_FAILED;
}

static const struct tftp_context tftp = {
   tftp_open,
   tftp_close,
   tftp_read,
   tftp_write,
   tftp_error
};

err_t ldr_tftp_init(tftp_ldr_internal *p)
{
   err_t Err;
   const char *Filename;
   const char *TransferTypes[] = { "read", "flash","compar","sav" };

   do {
      if(p == NULL || p->ServerIP.addr == 0 || p->Filename == NULL ||
         p->TransferType < 0 || p->TransferType > TFTP_TYPE_LAST)
      {
         Err = ERR_VAL;
         break;
      }
      p->Error = TFTP_OK;
      p->LastEraseAdr = INVALID_FLASH_ADR;
      p->BytesTransfered = 0;
      p->LastProgress = 0;
      p->ErrMsg[0] = 0;

      if((Err = tftp_init_client(&tftp)) != ERR_OK) {
         ELOG("tftp_init_client failed %d\n",Err);
         break;
      }
      Filename = (const char *) p->Filename;
      LOG("%sing %s @ 0x%x\n",
          TransferTypes[p->TransferType - 1],Filename,p->FlashAdr);

      if(p->TransferType == TFTP_TYPE_SAVE) {
         Err = tftp_put(p,&p->ServerIP,TFTP_PORT,Filename,TFTP_MODE_OCTET);
      }
      else {
         Err = tftp_get(p,&p->ServerIP,TFTP_PORT,Filename,TFTP_MODE_OCTET);
      }
      if(Err != ERR_OK) {
         ELOG("failed %d\n",Err);
         break;
      }
      p->Error = TFTP_IN_PROGRESS;
   } while(false);

   return Err;
}

