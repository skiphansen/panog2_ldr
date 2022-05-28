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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include "gpio_defs.h"
#include "timer.h"
#include "pano_io.h"
#include "eth_io.h"
#include "spi_lite.h"
#include "spi_drv.h"
#include "cmd_parser.h"

/* lwIP core includes */
#include "lwip/opt.h"

#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/api.h"

#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/dns.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/snmp.h"

#include "lwip/apps/tftp_client.h"

/* lwIP netif includes */
#include "lwip/etharp.h"
#include "netif/ethernet.h"
#include "tftp_ldr.h"
// #define DEBUG_LOGGING         1
// #define VERBOSE_DEBUG_LOGGING 1
#include "log.h"

// How often to update LEDs in milliseconds
#define LED_BLINK_RATE     250

typedef enum {
   TAG_AUTO_BOOT = 449220, // A random number of no significance
   TAG_AUTO_BOOT_ADR,
   TAG_TFTP_SRV
} Tag_t;


#define NET_PRINT_BUF_LEN  120
typedef struct {
   char PrintBuf[NET_PRINT_BUF_LEN];
   int Len;
   int BytesQueued;
   int BytesSent;
   int BytesOnLine;
} NetPrintInternal_t;

typedef struct {
   uint16_t Type;
   uint16_t Register;
   uint32_t Value;
} PanoInfoBlock_t;

// --- The following are in network order to save some code space ---
#define INFO_TYPE_CFG_WR   0x10        // network order
#define INFO_TYPE_WR       0x40        // network order

#define CFG_REG_BRD_ID     0x0000
#define CFG_REG_CHIP_ID    0x0100
#define CFG_REG_MAC_MSB    0x1000
#define CFG_REG_MAC_LSB    0x1100
// ---

#define BOARD_ID_G1        0x00050000
#define BOARD_ID_G1_PLUS   0x10010001
#define BOARD_ID_G2_B      0x08010000
#define BOARD_ID_G2_C      0x08010002
#define BOARD_ID_DZ22_2    0x08011000

#define GOLDEN_IMAGE_ADR   0x40000

#define AUTOBOOT_OFF       0
#define AUTOBOOT_ON        1
#define AUTOBOOT_DELAYED   2

// How long to wait for button press in milliseconds
#define AUTO_BOOT_DELAY    3000

NetPrintInternal_t gNetPrint;
struct tcp_pcb *gTcpCon;

#define REG_WR(reg, wr_data)       *((volatile uint32_t *)(reg)) = (wr_data)
#define REG_RD(reg)                *((volatile uint32_t *)(reg))
err_t TcpRecv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void TcpError(void *arg, err_t err);
err_t TcpPoll(void *arg, struct tcp_pcb *tpcb);
err_t TcpSent(void *arg, struct tcp_pcb *tpcb, u16_t len);

#define MAX_ETH_FRAME_LEN     1518
#define RX_BUF_LEN            80

int gRxCount;
uint8_t gRxBuf[RX_BUF_LEN];

uint8_t gOurMac[] = {MAC_ADR};
struct netif gNetif;
struct tcp_pcb *gTCP_pcb;
bool gSendWelcome;
bool gInputReady;
tftp_ldr_internal gTftp;
char gTemp[1024];
uint32_t gEthStatus;
int gAutoBoot;
uint32_t gAutoBootAdr = GOLDEN_IMAGE_ADR;
uint32_t gBoardID;
const char DelayedStr[] = "delayed";

void ClearRxFifo(void);
err_t pano_netif_init(struct netif *netif);
void init_default_netif(void);
void pano_netif_poll(void);
void netif_init(void);
err_t pano_netif_output(struct netif *netif, struct pbuf *p);
void TcpInit(void);
err_t TcpAccept(void *arg, struct tcp_pcb *newpcb, err_t err);
void NetPuts(char *String);
void NetWaitBufEmptyInternal(const char *funct,int Line);
#define NetWaitBufEmpty(x) NetWaitBufEmptyInternal(__FUNCTION__,__LINE__)
int NetDumpHex(void *Data,int Len,bool bWithAdr,int Adr);
int NetPrintFillPcb(struct tcp_pcb *tpcb);
void SendPrompt(void);
void UpdateLEDs(void);
int GetAdrAndLen(char **pCmdline,uint32_t *pAdr,uint32_t *pLen);
int GetTftpTransferVals(char **pCmdline,TransferType_t Type);
bool CheckEmpty(uint32_t Adr,uint32_t PageSize,uint32_t EraseSize);
int TftpTranserWait(tftp_ldr_internal *p);
int FlashInternal(char *CmdLine,bool bAutoErase);
uint32_t ParseDataBlock(void);
void AddBoardInfo(Tag_t Tag,size_t Len,void *TagData);
int ButtonPressed(void);
void ReBoot(uint32_t SpiAdr);

const char gVerStr[] = "Pano LDR v0.02 compiled " __DATE__ " " __TIME__ "\r\n";

int AutoBootCmd(char *CmdLine);
int AutoEraseCmd(char *CmdLine);
int BootAdrCmd(char *CmdLine);
int FlashCmd(char *CmdLine);
int RebootCmd(char *CmdLine);
int DumpCmd(char *CmdLine);
int EraseCmd(char *CmdLine);
int MapCmd(char *CmdLine);
int SaveCmd(char *CmdLine);
int TftpCmd(char *CmdLine);
int VerifyCmd(char *CmdLine);

const CommandTable_t gCmdTable[] = {
   { "autoboot ", "[ on | off | delayed ]",NULL,AutoBootCmd},
   { "autoerase", "[ on | off]",NULL,AutoEraseCmd},
   { "bootadr  ",  "<flash adr>",NULL,BootAdrCmd},
   { "dump     ",  "<flash adr> <length>",NULL,DumpCmd},
   { "erase    ",  "<start adr> <end adr>",NULL,EraseCmd},
   { "flash    ",  "<filename> <flash adr>",NULL,FlashCmd},
   { "map      ",  "Display blank regions in flash",NULL,MapCmd},
   { "reboot   ",  "<flash adr>",NULL,RebootCmd},
   { "save     ",  "<filename> <flash adr> <length>",NULL,SaveCmd},
   { "tftp     ",  "<IP adr of tftp server>",NULL,TftpCmd},
   { "verify   ",  "<filename> <flash adr>",NULL,VerifyCmd},
   { "?", NULL, NULL, HelpCmd},
   { "help",NULL, NULL, HelpCmd},
   { NULL }  // end of table
};

//-----------------------------------------------------------------
// main
//-----------------------------------------------------------------
int main(int argc, char *argv[])
{
   int i;
   unsigned char Buf[256];
   int Id = 0;
   uint32_t Temp;
   uint32_t NewEthStatus;
   uint8_t Byte;
   uint16_t Count;
   bool bHaveIP = false;
   bool bRanTest = false;

   gTftp.bAutoErase = true;
   ALOG_R(gVerStr);
   CmdParserInit(gCmdTable,NetPrintf);

// Set LED GPIO's to output
   Temp = REG_RD(GPIO_BASE + GPIO_DIRECTION);
   Temp |= GPIO_BIT_RED_LED|GPIO_BIT_GREEN_LED|GPIO_BIT_BLUE_LED;
   REG_WR(GPIO_BASE + GPIO_DIRECTION,Temp);
   REG_WR(GPIO_BASE + GPIO_OUTPUT,0);

   spi_init(CONFIG_SPILITE_BASE);
   spi_chip_init();

   ParseDataBlock();
   if(gAutoBoot == AUTOBOOT_ON && !ButtonPressed()) {
   // auto boot now
      ReBoot(gAutoBootAdr);
   }
   else if(gAutoBoot == AUTOBOOT_DELAYED) {
      while(!ButtonPressed()) {
         if(timer_now() > AUTO_BOOT_DELAY) {
            ReBoot(gAutoBootAdr);
         }
      }
   }

   lwip_init();
   init_default_netif();
   TcpInit();
   ClearRxFifo();

   for(; ; ) {
      UpdateLEDs();
      pano_netif_poll();
      NewEthStatus = ETH_STATUS & (ETH_STATUS_LINK_UP | ETH_STATUS_LINK_SPEED);
      if(gEthStatus != NewEthStatus) {
         VLOG_R("Ethernet Status: 0x%x\n",ETH_STATUS);
         gEthStatus = NewEthStatus;
         ALOG_R("Ethernet link is %s",
                (gEthStatus & ETH_STATUS_LINK_UP) ? "up" : "down");
         if(gEthStatus & ETH_STATUS_LINK_UP) {
            ALOG_R(", ");
            switch(gEthStatus & ETH_STATUS_LINK_SPEED) {
               case SPEED_1000MBPS:
                  ALOG_R("1000BaseT");
                  break;

               case SPEED_100MBPS:
                  ALOG_R("100BaseT");
                  break;

               case SPEED_10MBPS:
                  ALOG_R("10BaseT");
                  break;

               case SPEED_UNSPECIFIED:
                  ALOG_R("?");
                  break;

               default:
                  ALOG_R("WTF?");
                  break;
            }
            netif_set_link_up(&gNetif);
         }
         else {
            netif_set_link_down(&gNetif);
         }
         ALOG_R("\n");
      }

      if(!bHaveIP && (gEthStatus & ETH_STATUS_LINK_UP)) {
         if(dhcp_supplied_address(&gNetif)) {
            bHaveIP = true;
            ALOG_R("IP address assigned %s\n",ip4addr_ntoa(&gNetif.ip_addr));
         }
      }

      if(gSendWelcome) {
         gSendWelcome = false;
         NetPrintf(gVerStr);
         SendPrompt();
      }
      if(gInputReady) {
         gInputReady = false;
         LOG("Parsing command '%s'\n",gRxBuf);
         ParseCmd(gRxBuf);
         gRxCount = 0;
         SendPrompt();
      }
   }

   return 0;
}

void ClearRxFifo()
{
   int i;
   uint8_t Byte;

   if(!(ETH_STATUS & ETH_STATUS_RXEMPTY)) {
      VLOG("Clearing Rx FIFO\n");
      for(i = 0; i < 2048; i++) {
         if(ETH_STATUS & ETH_STATUS_RXEMPTY) {
            break;
         }
         Byte = ETH_RX();
      }
      VLOG("FIFO %scleared after %d reads\n",i == 2048 ? "not " : "",i);
   }
}

void lwip_pano_assert(const char *msg, int line, const char *file);
void lwip_pano_assert(const char *msg, int line, const char *file)
{
   ALOG_R("Assertion \"%s\" failed %s#%d\n",msg,file,line);
   for( ; ; );
}


/**
 * @ingroup sys_time
 * Returns the current time in milliseconds,
 * may be the same as sys_jiffies or at least based on it.
 * Don't care for wraparound, this is only used for time diffs.
 * Not implementing this function means you cannot use some modules (e.g. TCP
 * timestamps, internal timeouts for NO_SYS==1).
 */
uint32_t sys_now(void);
uint32_t sys_now(void)
{
   static uint32_t LastTimerNow;
   static uint32_t UptimeMillsecs;
   uint32_t TimerNow;

// timer_now is millisecionds, but it's based on the CPU clock so it wraps
// pretty quickly (14 minutes @ 50 Mhz) so use it to update a local up time

   TimerNow = (uint32_t) timer_now();
   UptimeMillsecs += TimerNow - LastTimerNow;
   LastTimerNow = TimerNow;

   return UptimeMillsecs;
}


unsigned int lwip_port_rand(void)
{
  return (unsigned int) rand();
}

err_t pano_netif_init(struct netif *netif)
{
   VLOG("%s: called\n",__FUNCTION__);

   netif->linkoutput = pano_netif_output;
   netif->output     = etharp_output;
   netif->mtu        = 1500;
   netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
   MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 100000000);

   memcpy(netif->hwaddr,gOurMac,ETH_HWADDR_LEN);
   netif->hwaddr_len = ETH_HWADDR_LEN;

   return ERR_OK;
}


void init_default_netif()
{
   err_t Err;

   do {
      if(netif_add(&gNetif,IP4_ADDR_ANY,IP4_ADDR_ANY,IP4_ADDR_ANY,NULL,pano_netif_init,netif_input) == NULL) {
         ELOG("netif_add failed\n");
         break;
      }
      gNetif.name[0] = 'e';
      gNetif.name[1] = 't';
      gNetif.hostname = "pano_ldr";
      netif_set_default(&gNetif);
      netif_set_up(&gNetif);
      if((Err = dhcp_start(&gNetif)) != ERR_OK) {
         ELOG("dhcp_start failed: %d\n",Err);
         break;
      }
   } while(false);
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
err_t pano_netif_output(struct netif *netif, struct pbuf *p)
{
   uint8_t *cp = (uint8_t *) p->payload;
   u16_t TxLen = p->tot_len;
   u16_t ThisBufLen = p->len;
   struct pbuf *pNext = p->next;
   int i;

#ifdef LOG_RAW_ETH
   VLOG("called tot_len: %d, len: %d: \n",p->tot_len,p->len);
   VLOG_HEX(p->payload,p->len);
#endif

   ETH_TX = (uint8_t) ((TxLen >> 8) & 0xff);
   ETH_TX = (uint8_t) (TxLen & 0xff);
   for(i = 0; i < TxLen; i++) {
      if(ThisBufLen == 0) {
         p = p->next;
         if(p == NULL) {
            ELOG("Error - Ran out of buffers before data\n");
            break;
         }
         ThisBufLen = p->len;
         LOG("Next buf in chain tot_len: %d, len: %d: \n",p->tot_len,p->len);
         LOG_HEX(p->payload,p->len);
         cp = (uint8_t *) p->payload;
      }
      ETH_TX = *cp++;
      ThisBufLen--;
   }

   return ERR_OK;
}

void pano_netif_poll()
{
   struct pbuf *p = NULL;
   u16_t len;
   ssize_t readlen;
   uint16_t Count;
   uint8_t *cp;
   int i;

   if(!(ETH_STATUS & ETH_STATUS_RXEMPTY)) do {
      Count = (ETH_RX() << 8) + ETH_RX();
      if(Count == 0 || Count > MAX_ETH_FRAME_LEN) {
         ELOG("Invalid rx frame length %d\n",Count);
         ClearRxFifo();
      }
      len = (u16_t)Count;
      MIB2_STATS_NETIF_ADD(&gNetif,ifinoctets,len);

      /* We allocate a pbuf chain of pbufs from the pool. */
      p = pbuf_alloc(PBUF_RAW,len,PBUF_POOL);
      if(p != NULL) {
         cp = (uint8_t *) p->payload;
         *cp = ETH_RX();
         cp++;
         for(i = 1; i < Count; i++) {
            *cp++ = ETH_RX();
         }
         cp = (uint8_t *) p->payload;

#ifdef LOG_RAW_ETH
         LOG_R("Rx #%d: Read %d (0x%x) bytes from Rx Fifo:\n",gRxCount,Count,Count);
         LOG_HEX(p->payload,p->len);
#endif
         if(gNetif.input(p,&gNetif) != ERR_OK) {
           LWIP_DEBUGF(NETIF_DEBUG,("%s: netif input error\n",__FUNCTION__));
           pbuf_free(p);
         }
      }
      else {
         /* drop packet(); */
         MIB2_STATS_NETIF_INC(gNetif,ifindiscards);
         LWIP_DEBUGF(NETIF_DEBUG,("%s: could not allocate pbuf\n",__FUNCTION__));
         for(i = 0; i < Count; i++) {
            (void) ETH_RX();
         }
      }
   } while(false);
   /* Cyclic lwIP timers check */
   sys_check_timeouts();

}

void TcpInit()
{
   err_t err;
   if((gTCP_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY)) != NULL) {

     if((err = tcp_bind(gTCP_pcb,IP_ANY_TYPE,23)) == ERR_OK) {
       gTCP_pcb = tcp_listen_with_backlog(gTCP_pcb,1);
       tcp_accept(gTCP_pcb,TcpAccept);
     }
     else {
        ELOG("tcp_bind failed: %d\n",err);
     }
   }
   else {
     ELOG("tcp_new_ip_type failed\n");
   }
}

err_t TcpAccept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
   err_t ret_err = ERR_VAL;
   err_t Err;
   struct tcpecho_raw_state *es;

   VLOG("Called, err: %d, newpcb: %p\n",err,newpcb);
   if(err == ERR_OK && newpcb != NULL) {
      if(gTcpCon != NULL) {
         ELOG("gTcpCon not NULL\n");
      }
      gTcpCon = newpcb;
      tcp_arg(newpcb,NULL);
      tcp_recv(newpcb,TcpRecv);
      tcp_err(newpcb,TcpError);
      tcp_poll(newpcb,TcpPoll,0);
      tcp_sent(newpcb,TcpSent);
   // Reset everything for a new connection
      gNetPrint.BytesSent = gNetPrint.Len = gNetPrint.BytesQueued = 0;
      gInputReady = false;
      gSendWelcome = true;
      gRxCount = 0;
      if(gTftp.ServerIP.addr == 0) {
      // Assume tftp server is on the same host as the incoming telnet connection
         gTftp.ServerIP = newpcb->remote_ip;
      }
      ret_err = ERR_OK;
   }

   return ret_err;
}

err_t TcpRecv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
   uint8_t *cp;
   int i;

   VLOG("called err: %d, p: %p\n",err,p);
   if(p != NULL) {
      VLOG("called err: %d, pbuf->tot_len: %d, pbuf->len: %d: \n",err,
           p->tot_len,p->len);
      VLOG_HEX(p->payload,p->len);
      cp = (uint8_t *) p->payload;
      for(i = 0; i < p->len; i++) {
         if(gInputReady) {
         // The last hasn't been processed yet
            break;
         }
         if(*cp == 0xff) {
         // Skip telnet command
            cp += 3;
            i += 2;
            if(i + 1 > p->len) {
               ELOG("Internal error\n");
            }
         }
         else {
            if(*cp == '\b' || *cp == 127) {
            // back space or delete
               if(gRxCount > 0) {
                  gRxCount--;
                  NetPuts("\b \b"); // Send back space, space, back space
               }
               else {
                  NetPuts("\a"); // beep
               }
            }
            else if(*cp == '\r') {
               gInputReady = true;
               gRxBuf[gRxCount] = 0;
            }
            else {
               gRxBuf[gRxCount] = *cp++;
            }

            if(gRxCount < sizeof(gRxBuf) - 1) {
               gRxCount++;
            }
         }
      }
      tcp_recved(tpcb, p->len);
      pbuf_free(p);

      VLOG("tcp_sndbuf: %d\n",tcp_sndbuf(tpcb));
   }
   else {
      LOG("connection closed\n");
      gTcpCon = NULL;
   }
   return ERR_OK;
}

void TcpError(void *arg, err_t err)
{
   ELOG("Called, err: %d\n",err);
}

err_t TcpPoll(void *arg, struct tcp_pcb *tpcb)
{
   err_t Err;

   VLOG("tpcb: %p\n",tpcb);
   NetPrintFillPcb(tpcb);
   return ERR_OK;
}

err_t TcpSent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
   err_t Err;
   NetPrintInternal_t *p = &gNetPrint;

   VLOG("len %d",len);

   gNetPrint.BytesSent += len;
   if(p->BytesSent == p->Len) {
      p->BytesSent = p->Len = p->BytesQueued = 0;
      VLOG_R(" done");
   }
   else {
   // Send some more
      NetPrintFillPcb(tpcb);
   }
   VLOG_R("\n");
}

int NetPrintf(const char *Format, ...)
{
   NetPrintInternal_t *p = &gNetPrint;
   va_list Args;

   NetWaitBufEmpty();
   va_start(Args,Format);

   p->Len = vsnprintf(p->PrintBuf,sizeof(p->PrintBuf),Format,Args);
   LOG("Sending:\n---\n%s\n",p->PrintBuf);
   LOG_R("\n---\n");
   NetPrintFillPcb(gTcpCon);
}

void NetPuts(char *String)
{
   NetPrintInternal_t *p = &gNetPrint;
   char *cp = &p->PrintBuf[p->Len];

   while(gTcpCon != NULL && *String) {
      if(p->Len >= sizeof(p->PrintBuf)) {
      // Flush buffer
         NetPrintFillPcb(gTcpCon);
         NetWaitBufEmpty();
         cp = p->PrintBuf;
      }
      *cp++ = *String++;
      p->Len++;
   }
}

void NetWaitBufEmptyInternal(const char *funct,int Line)
{
   NetPrintInternal_t *p = &gNetPrint;
   if(gTcpCon != NULL && p->Len != p->BytesSent) {
      // LOG_R("%s#%d: Waiting Len %d, BytesSent %d\n",funct,Line,p->Len,p->BytesSent);
      while(gTcpCon != NULL && p->Len != p->BytesSent) {
      // Can't while until the previous buffer has been sent.
         pano_netif_poll();
      }
      VLOG("Finished waiting\n");
   }
}

int NetDumpHex(void *Data,int Len,bool bWithAdr,int Adr)
{
   NetPrintInternal_t *p = &gNetPrint;
   int i;
   char Hex[9];
   unsigned char *cp = Data;

   if(Len == 0) {
   // Dump complete
      if(p->BytesOnLine > 0) {
         NetPuts("\n");
      }
      p->BytesOnLine = 0;
      NetPrintFillPcb(gTcpCon);
   }
   else {
      for(i = 0; i < Len; i++) {
         if(p->BytesOnLine == 0) {
            if(bWithAdr) {
               sprintf(Hex,"%08x",i + Adr);
               NetPuts(Hex);
               NetPuts("  ");
            }
         }
         else {
            NetPuts(" ");
         }

         sprintf(Hex,"%02x",*cp++);
         NetPuts(Hex);

         if(++p->BytesOnLine == 16) {
            NetPuts("\n");
            p->BytesOnLine = 0;
         }
      }
   }
}


int NetPrintFillPcb(struct tcp_pcb *tpcb)
{
   err_t Err;
   NetPrintInternal_t *p = &gNetPrint;
   int Byte2Write;
   int CanSend = tcp_sndbuf(tpcb);

   if(gTcpCon != NULL && (Byte2Write = p->Len - p->BytesQueued) > 0) {
   // Have data to send
      if(Byte2Write > CanSend) {
         LOG("Reduced Byte2Write from %d to %d\n",Byte2Write,CanSend);
         Byte2Write = CanSend;
      }

      VLOG("Calling tcp_write BytesSent %d, Byte2Write %d\n",
           p->BytesSent,Byte2Write);
      Err = tcp_write(tpcb,&p->PrintBuf[p->BytesSent],Byte2Write,
                      TCP_WRITE_FLAG_COPY);
      if(Err != ERR_OK) {
         ELOG("tcp_write failed - %d\n",Err);
      }
      else {
         p->BytesQueued += Byte2Write;
         if((Err = tcp_output(tpcb)) != ERR_OK) {
            ELOG("tcp_output failed - %d\n",Err);
         }
      }
   }
   return 0;
}

void SendPrompt()
{
   NetPrintf("ldr> ");
}

/* 
   Ethernet link down: flashing red
   Ethernet link up, no IP address: flashing blue
   IP address assigned: flashing green
   User connected: solid green
*/
void UpdateLEDs()
{
   static t_time LastUpdate;
   t_time Now = timer_now();
   static uint32_t Led = GPIO_LED_BITS;

   if((Now - LastUpdate) > LED_BLINK_RATE) {
      LastUpdate = Now;
      if(gTcpCon != NULL) {
      // User connected: solid green
         REG_WR(GPIO_BASE + GPIO_OUTPUT_CLR,Led);
         Led = GPIO_BIT_GREEN_LED;
      }
      else if(Led != 0) {
      // Turn off last LED
         REG_WR(GPIO_BASE + GPIO_OUTPUT_CLR,Led);
         Led = 0;
      }
      else if(!(gEthStatus & ETH_STATUS_LINK_UP)) {
      // Ethernet link down, flashing red
         Led = GPIO_BIT_RED_LED;
      }
      else if(!dhcp_supplied_address(&gNetif)) {
      // Ethernet link up, no IP address: flashing blue
         Led = GPIO_BIT_BLUE_LED;
      }
      else {
         Led = GPIO_BIT_GREEN_LED;
      }

      if(Led != 0) {
      // Turn on LED
         REG_WR(GPIO_BASE + GPIO_OUTPUT_SET,Led);
      }
   }
}

int DumpCmd(char *CmdLine)
{
   int Ret = RESULT_BAD_ARG;
   uint32_t Adr;
   uint32_t Len;
   uint32_t Bytes2Read;
   uint32_t BytesRead = 0;
   char *cp = CmdLine;
   
   do {
      if((Ret = GetAdrAndLen(&cp,&Adr,&Len)) != RESULT_OK) {
         break;
      }
      while(gTcpCon != NULL && BytesRead < Len) {
         Bytes2Read = Len;
         if(Bytes2Read > sizeof(gTemp)) {
            Bytes2Read = sizeof(gTemp);
         }
         spi_read(Adr,gTemp,Bytes2Read);
         NetDumpHex(gTemp,Bytes2Read,true,Adr);
         Adr += Bytes2Read;
         BytesRead += Bytes2Read;
      }
   } while(false);
   NetDumpHex(NULL,0,false,0);
   return Ret;
}

int EraseCmd(char *CmdLine)
{
   int Ret;
   uint32_t Adr;
   uint32_t EndAdr;
   uint32_t Len;
   uint32_t BytesErased;
   const FlashInfo_t *pInfo = spi_get_flashinfo();
   uint32_t EraseSize = pInfo->EraseSize;
   char *cp = CmdLine;
   
   do {
      if((Ret = GetAdrAndLen(&cp,&Adr,NULL)) != RESULT_OK) {
         break;
      }
      if((Ret = GetAdrAndLen(&cp,&EndAdr,NULL)) != RESULT_OK) {
         break;
      }

      Len = EndAdr - Adr + 1;
      if((Adr % EraseSize) != 0) {
         Ret = RESULT_ERR;
         NetPrintf("Error - address not at erase boundary\n");
         break;
      }
      if((Len % EraseSize) != 0) {
         Ret = RESULT_ERR;
         NetPrintf("Error - length not a multiple of the erase size\n");
         break;
      }
      LOG("Calling spi_erase Adr 0x%x Len 0x%x\n",Adr,Len);
      while(Adr < EndAdr) {
         spi_erase(Adr,EraseSize);
         NetPrintf(".");
         Adr += EraseSize;
      }
      NetPrintf("\nErased %dK\n",Len / 1024);
   } while(false);

   return Ret;
}

// <filename> <flash adr>
int FlashCmd(char *CmdLine)
{
   int Ret = RESULT_BAD_ARG;  // Assume the worse
   err_t Err;
   char *cp = CmdLine;
   tftp_ldr_internal *p = &gTftp;

   do {
      if((Ret = GetTftpTransferVals(&cp,TFTP_TYPE_FLASH)) != RESULT_OK) {
         break;
      }
      if((Err = ldr_tftp_init(p)) != ERR_OK) {
         NetPrintf("tftp transfer failed %d\n",Err);
         Ret = RESULT_ERR;
         break;
      }

      Ret = TftpTranserWait(p);
      if(Ret == RESULT_OK) {
         NetPrintf("\nFlashed %d bytes\n",p->BytesTransfered);
      }
   } while(false);

   return Ret;
}

// <filename> <flash adr> <length>
int SaveCmd(char *CmdLine)
{
   int Ret = RESULT_BAD_ARG;  // Assume the worse
   err_t Err;
   char *cp = CmdLine;
   tftp_ldr_internal *p = &gTftp;

   do {
      if((Ret = GetTftpTransferVals(&cp,TFTP_TYPE_SAVE)) != RESULT_OK) {
         break;
      }
      if((Err = ldr_tftp_init(p)) != ERR_OK) {
         NetPrintf("tftp transfer failed %d\n",Err);
         Ret = RESULT_ERR;
         break;
      }

      Ret = TftpTranserWait(p);
      if(Ret == RESULT_OK) {
         NetPrintf("\nSaved %d bytes\n",p->BytesTransfered);
      }
   } while(false);

   return Ret;
}

int VerifyCmd(char *CmdLine)
{
   int Ret;
   err_t Err;
   tftp_ldr_internal *p = &gTftp;

   do {
      if((Ret = GetTftpTransferVals(&CmdLine,TFTP_TYPE_COMPARE)) != RESULT_OK) {
         break;
      }
      if((Err = ldr_tftp_init(p))!= ERR_OK) {
         NetPrintf("tftp transfer failed %d:%d\n",Err,gTftp.Error);
      }
      Ret = TftpTranserWait(p);
      if(Ret == RESULT_OK) {
         NetPrintf("%d bytes verified\n",p->BytesTransfered);
      }
   } while(false);

   return Ret;
}


int TftpCmd(char *CmdLine)
{
   int Ret = RESULT_OK;
   ip_addr_t ServerIP;

   if(!*CmdLine) {
      if(gTftp.ServerIP.addr != 0) {
      // Address set, display it
         NetPrintf("%s\n",ip4addr_ntoa(&gTftp.ServerIP));
      }
      else {
      // display usage
         Ret = RESULT_USAGE;
      }
   }
   else if(!ipaddr_aton(CmdLine,&ServerIP)) {
      Ret = RESULT_BAD_ARG;
   }
   else {
      AddBoardInfo(TAG_TFTP_SRV,sizeof(ServerIP),&ServerIP);
   }

   return Ret;
}

int MapCmd(char *CmdLine)
{
   uint32_t Adr = 0;
   uint32_t LastAdr = 0;
   uint32_t FlashSize;
   uint32_t PageSize;
   uint32_t EraseSize;
   const FlashInfo_t *pFlashInfo = spi_get_flashinfo();
   int Ret = RESULT_OK;
   char *pEmpty;
   bool bWasEmpty;
   bool bEmpty;
   bool bPrintStart = true;
   
   do {
      if(pFlashInfo == NULL) {
         Ret = RESULT_INTERNAL_ERR;
         break;
      }
      FlashSize = pFlashInfo->FlashSize;
      PageSize = pFlashInfo->PageSize;
      EraseSize = pFlashInfo->EraseSize;
      pEmpty = &gTemp[PageSize];
      memset(pEmpty,0xff,PageSize);

      spi_read(Adr,gTemp,PageSize);
      bEmpty = CheckEmpty(Adr,PageSize,EraseSize);
      bWasEmpty = bEmpty;

      NetPrintf("%s: %d MB, %d KB sectors\n",
                pFlashInfo->Desc,
                pFlashInfo->FlashSize /(1024 * 1024),
                pFlashInfo->EraseSize / 1024);

      while(Adr < FlashSize) {
         if(bPrintStart) {
            bPrintStart = false;
            NetPrintf("0x%06x -> ",Adr);
            NetWaitBufEmpty();
         }
         Adr += EraseSize;
         bEmpty = CheckEmpty(Adr,PageSize,EraseSize);
         if(bWasEmpty == bEmpty && Adr == FlashSize) {
         // force display if entire flash is erased or in use
            bEmpty = !bEmpty;
         }
         if(bWasEmpty != bEmpty) {
         // End of region
            bWasEmpty = bEmpty;
            bPrintStart = true;
            NetPrintf("0x%06x %4d K %sempty\n",Adr-1,(Adr - LastAdr) / 1024,
                      !bWasEmpty ? "" : "not ");
            LastAdr = Adr;
         }
      }
   } while(false);
   return Ret;
}

void ReBoot(uint32_t SpiAdr)
{
   LOG("0x%x\n",SpiAdr);

   REG_WR(GPIO_BASE + BOOT_SPI_ADR,SpiAdr);
   REG_WR(GPIO_BASE + REBOOT_ADR,1);
   REG_WR(GPIO_BASE + REBOOT_ADR,0);
}

int GetOnOff(char *CmdLine,bool *pOnOff)
{
   int Ret = RESULT_OK;

   *pOnOff = false;
   if(strcasecmp(CmdLine,"on") == 0) {
      *pOnOff = true;
   }
   else if(strcasecmp(CmdLine,"off") != 0) {
      Ret = RESULT_BAD_ARG;
   }

   return Ret;
}

int AutoBootCmd(char *CmdLine)
{
   int Ret = RESULT_BAD_ARG;  // assume the worse
   int AutoBoot = 0;
   bool On;
   char *cp;
   const char *Mode[] = { "off","on",DelayedStr};

   do {
      if(!*CmdLine) {
         Ret = RESULT_OK;
         break;
      }
      if(strcasecmp(CmdLine,DelayedStr) == 0) {
         AutoBoot = AUTOBOOT_DELAYED;
      }
      else if(GetOnOff(CmdLine,(bool *) &AutoBoot)) {
         break;
      }
      Ret = RESULT_OK;
      AddBoardInfo(TAG_AUTO_BOOT,sizeof(int),&AutoBoot);
   } while(false);

   if(Ret == RESULT_OK) {
      NetPrintf("Autoboot %s\n",Mode[gAutoBoot]);
   }

   return Ret;
}

int AutoEraseCmd(char *CmdLine)
{
   int Ret = RESULT_BAD_ARG;  // assume the worse
   bool On = false;

   do {
      if(!*CmdLine) {
         Ret = RESULT_OK;
         break;
      }

      if(GetOnOff(CmdLine,&On)) {
         break;
      }
      Ret = RESULT_OK;
      gTftp.bAutoErase = On;
   } while(false);

   if(Ret == RESULT_OK) {
      NetPrintf("AutoErase o%s\n",gTftp.bAutoErase ? "n":"ff");
   }

   return Ret;
}

int BootAdrCmd(char *CmdLine)
{
   int Ret = RESULT_OK;
   int On;
   uint32_t AutoBootAdr;

   if(!*CmdLine) {
      NetPrintf("Autoboot @0x%x\n",gAutoBootAdr);
   }
   else if(ConvertValue(&CmdLine,&AutoBootAdr)) {
      Ret = RESULT_BAD_ARG;
   }
   else {
      AddBoardInfo(TAG_AUTO_BOOT_ADR,sizeof(AutoBootAdr),&AutoBootAdr);
   }
   return Ret;
}


int RebootCmd(char *CmdLine)
{
   int Ret;
   uint32_t Adr;
   char *cp = CmdLine;
   
   if((Ret = GetAdrAndLen(&cp,&Adr,NULL)) == RESULT_OK) {
      NetPrintf("Booting bitstream @ 0x%x\n");
      NetWaitBufEmpty();
      ReBoot(Adr);
   }

   return Ret;
}


int GetAdrAndLen(char **pCmdline,uint32_t *pAdr,uint32_t *pLen)
{
   int Ret = RESULT_BAD_ARG;
   const FlashInfo_t *pFlashInfo = spi_get_flashinfo();
   uint32_t FlashSize;

   do {
      if(pFlashInfo == NULL) {
         Ret = RESULT_INTERNAL_ERR;
         break;
      }
      FlashSize = pFlashInfo->FlashSize;

      if(ConvertValue(pCmdline,pAdr)) {
         break;
      }
      if(*pAdr > FlashSize) {
         NetPrintf("Error 0x%x is past end of flash (0x%x).\n",*pAdr,FlashSize);
         break;
      }

      if(pLen == NULL) {
      // Len not requested
         Ret = RESULT_OK;
         break;
      }

      if(ConvertValue(pCmdline,pLen)) {
         break;
      }

      if((*pAdr + *pLen) > FlashSize) {
         *pLen = FlashSize - *pAdr;
         NetPrintf("Note: Length reduced to 0x%x to stay within flash\n",*pLen);
      }
      Ret = RESULT_OK;
   } while(false);

   return Ret;
}

// Parse command line: <filename> <flash adr>
// Saving filename and flash adr in gTftp
int GetTftpTransferVals(char **pCmdLine,TransferType_t Type)
{
   int Ret = RESULT_BAD_ARG;
   char *cp = *pCmdLine;
   tftp_ldr_internal *p = &gTftp;

   do {
      if(!*cp) {
      // No arguments given
         Ret = RESULT_USAGE;
         break;
      }

      if(p->ServerIP.addr == 0) {
         NetPrintf("Error - TFTP server address not set\n");
         Ret = RESULT_ERR;
         break;
      }

      cp = Skip2Space(cp);
      *cp++ = 0;

      if(strlen(*pCmdLine) > MAX_FILENAME_LEN) {
         NetPrintf("Filename too long, max %d characters\n",MAX_FILENAME_LEN);
         Ret = RESULT_ERR;
         break;
      }
      strcpy(p->Filename,*pCmdLine);
      if(Ret = GetAdrAndLen(&cp,&gTftp.FlashAdr,NULL)) {
         break;
      }

      if(Type == TFTP_TYPE_SAVE) {
         if(ConvertValue(&cp,&gTftp.SendLen)) {
            break;
         }
      }
      p->MaxBytes = sizeof(gTemp);
      p->Ram = gTemp;
      p->TransferType = Type;
      Ret = RESULT_OK;
   } while(false);

   return Ret;
}

bool CheckEmpty(uint32_t Adr,uint32_t PageSize,uint32_t EraseSize)
{
   bool Ret = true;
   int Pages2Check = EraseSize / PageSize;
   char *pEmpty = &gTemp[PageSize];
   int i;

   for(i = 0; i < Pages2Check; i++) {
      memset(gTemp,0xaa,PageSize);
      spi_read(Adr,gTemp,PageSize);
      if(memcmp(gTemp,pEmpty,PageSize) != 0) {
      // not empty
         Ret = false;
         break;
      }
      Adr += PageSize;
   }

   return Ret;
}

int TftpTranserWait(tftp_ldr_internal *p)
{
   int Ret = RESULT_OK;
   int BytesOnLine = 0;

   while(p->Error == TFTP_IN_PROGRESS) {
      pano_netif_poll();
      if((p->BytesTransfered - p->LastProgress) > PROGRESS_SIZE) {
         p->LastProgress += PROGRESS_SIZE;
         if(BytesOnLine++ > 64) {
            BytesOnLine = 0;
            NetPrintf("\n");
         }
         NetPrintf(".");
      }
   }
   if(BytesOnLine > 0) {
      BytesOnLine = 0;
      NetPrintf("\n");
   }
   tftp_cleanup();
   if(p->ErrMsg[0]) {
      NetPrintf("%s\n",p->ErrMsg);
      Ret = RESULT_ERR;
   }
   else if(p->Error == TFTP_ERR_COMPARE_FAIL) {
      NetPrintf("Compare failed @ 0x%0lx\n",p->FlashAdr + p->BytesTransfered);
   }
   else if(p->Error != TFTP_OK) {
      NetPrintf("Failed %d\n",p->Error);
      Ret = RESULT_ERR;
   }

   return Ret;
}

// read Pano data block into gTemp, return flash adr of first free word
uint32_t ParseDataBlock()
{
   uint32_t DataPageOffset;
   uint32_t *pU32 = (uint32_t *) gTemp;
   uint32_t Tag;
   uint32_t ValueLen;
   void *pValue;
   const FlashInfo_t *p = spi_get_flashinfo();
   uint32_t Ret = 0;

   do {
      DataPageOffset = p->FlashSize - p->EraseSize;
      spi_read(DataPageOffset,gTemp,sizeof(gTemp));

// <32 bit tag> <32 bit length> <variable length data>...<tag><len><data>...
      while((Tag = *pU32) != 0xffffffff) {
         pU32++;
         ValueLen = *pU32++;
         pValue = pU32;
         LOG("Tag %ld Len %ld\n",Tag,ValueLen);
         if((ValueLen & 0x3) != 0) {
         // round up to prevent unaligned accesses
            ValueLen += 4 - (ValueLen & 0x3);
         }
      // Skip to the next tag
         pU32 = (uint32_t *) (((uint8_t *) pU32) + ValueLen); 

         if(((char *) pU32) - gTemp > sizeof(gTemp) - sizeof(uint32_t)) {
         // Invalid length, ignore block
            pU32 = NULL;
            break;
         }

         switch(Tag) {
            case TAG_AUTO_BOOT:
               gAutoBoot = *((int *) pValue);
               break;

            case TAG_AUTO_BOOT_ADR:
               gAutoBootAdr = *((uint32_t *) pValue);
               break;

            case TAG_TFTP_SRV:
               gTftp.ServerIP.addr = *((uint32_t *) pValue);
               break;

            default:
               break;
         }
      }
   } while(false);

   if(pU32 != NULL) {
      Ret = DataPageOffset + ((char *) pU32 - gTemp);
   }
   else {
   // Not a valid data block, is it empty?
      if(CheckEmpty(DataPageOffset,p->PageSize,p->EraseSize)) {
      // block is empty, use it
         Ret = DataPageOffset;
      }
   }

   return Ret;
}

void AddBoardInfo(Tag_t Tag,size_t Len,void *TagData)
{
   uint32_t *pFirstUnused;
   uint32_t *pU32;
   uint32_t BytesAvailable;
   uint32_t WriteOffset = ParseDataBlock();
   uint32_t BufOffset;
   uint32_t EntryLen = (sizeof(uint32_t) * 2) + Len;
   const FlashInfo_t *p = spi_get_flashinfo();

   if(WriteOffset != 0) {
      BufOffset = WriteOffset - (p->FlashSize - p->EraseSize);
      pFirstUnused = pU32 = (uint32_t *) &gTemp[BufOffset];
      BytesAvailable = sizeof(gTemp) - BufOffset;
      BytesAvailable -= EntryLen;
      if(BytesAvailable >= sizeof(uint32_t)) {
      // The new data fits, write it
         *pU32++ = (uint32_t) Tag;
         *pU32++ = (uint32_t) Len;
         memcpy(pU32,TagData,Len);
         // LOG_HEX(gTemp,WriteOffset + EntryLen);
         LOG("Write %d bytes of data @ 0x%x\n",EntryLen,WriteOffset);
         spi_write(WriteOffset,(uint8_t *)pFirstUnused,EntryLen);
         ParseDataBlock(); // reload with updated data
      }
   }
}

int ButtonPressed()
{
   return (REG_RD(GPIO_BASE + GPIO_INPUT) & GPIO_BIT_PANO_BUTTON) ? 0 : 1;
}

