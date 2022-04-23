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

#include "lwip/apps/tftp_client.h"
#include "lwip/apps/tftp_server.h"
#include "tftp_ldr.h"

#include <string.h>

static void *tftp_open(const char *p,const char *Mode,u8_t bWrite)
{
   LWIP_UNUSED_ARG(Mode);
   return NULL;
}

static void tftp_close(void* handle)
{
   tftp_ldr_internal *p = (tftp_ldr_internal *) handle;
   LOG("%d bytes transfered\n",p->BytesTransfered);
}

// tftp put
static int tftp_read(void *handle,void *Buf, int Len)
{
   ELOG("I shouldn't have been called!\n");
   return -1;
}

static int tftp_write(void *handle, struct pbuf *pBuf)
{
   tftp_ldr_internal *p = (tftp_ldr_internal *) handle;
   int Ret = 0;  // assume the best
   u16_t TotalLen = pBuf->tot_len;
   u16_t Len = 0;
   u16_t BufLen;

   if((p->BytesTransfered + TotalLen) > p->MaxBytes ) {
      ELOG("Buffer overflow\n");
      Ret = -1;
   }

   while(Ret == 0 && Len < TotalLen) {
      BufLen = pBuf->len;
      switch(p->TransferType) {
         case TFTP_TYPE_RAM:
            memcpy(&p->Ram[p->BytesTransfered],pBuf->payload,BufLen);
            p->BytesTransfered += BufLen;
            Len += BufLen;
            break;

         case TFTP_TYPE_FLASH:
         default:
            ELOG("Invalid TransferType %d\n",p->TransferType);
            break;
      }
      pBuf = pBuf->next;
   }

   return Ret;
}

/* For TFTP client only */
static void tftp_error(void *handle,int err,const char *Msg,int size)
{
   ELOG("err %d\n");
   LOG_HEX((void *)Msg,size);
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
   const char *Filename = (const char *)p->Filename;

   do {
      if((Err = tftp_init_client(&tftp)) != ERR_OK) {
         ELOG("tftp_init_client failed %d\n",Err);
         break;
      }
      Err = tftp_get(p,&p->ServerIP,TFTP_PORT,Filename,TFTP_MODE_OCTET);
      if(Err != ERR_OK) {
         ELOG("tftp_get failed %d\n",Err);
         break;
      }
   } while(false);

   return Err;
}

