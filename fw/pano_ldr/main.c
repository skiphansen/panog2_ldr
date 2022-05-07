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

/* lwIP netif includes */
#include "lwip/etharp.h"
#include "netif/ethernet.h"
#include "tftp_ldr.h"

#define NET_PRINT_BUF_LEN  120
typedef struct {
   char PrintBuf[NET_PRINT_BUF_LEN];
   int Len;
   int BytesQueued;
   int BytesSent;
} NetPrintInternal_t;

NetPrintInternal_t gNetPrint;

#define REG_WR(reg, wr_data)       *((volatile uint32_t *)(reg)) = (wr_data)
#define REG_RD(reg)                *((volatile uint32_t *)(reg))
err_t TcpRecv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void TcpError(void *arg, err_t err);
err_t TcpPoll(void *arg, struct tcp_pcb *tpcb);
err_t TcpSent(void *arg, struct tcp_pcb *tpcb, u16_t len);

#define MAX_ETH_FRAME_LEN     1518
int gRxCount;
uint8_t gRxBuf[MAX_ETH_FRAME_LEN];

uint8_t gOurMac[] = {MAC_ADR};
struct netif gNetif;
struct tcp_pcb *gTCP_pcb;
bool gSendWelcome;
bool gInputReady;
tftp_ldr_internal gTftp;
char gTemp[1024];

bool ButtonJustPressed(void);
void ClearRxFifo(void);
void init_default_netif(void);
void pano_netif_poll(void);
void netif_init(void);
err_t pano_netif_output(struct netif *netif, struct pbuf *p);
void TcpInit(void);
err_t TcpAccept(void *arg, struct tcp_pcb *newpcb, err_t err);
int NetPrintf(const char *Format, ...);
int NetPrintFillPcb(struct tcp_pcb *tpcb);
void SendPrompt(void);

const char gVerStr[] = "Pano LDR v0.01 compiled " __DATE__ " " __TIME__ "\r\n";

int VersionCmd(const char *CmdLine);

const CommandTable_t gCmdTable[] = {
   { "version",  "Display firmware version",NULL,0,VersionCmd},
   { "?", NULL, NULL, CMD_FLAG_HIDE, HelpCmd},
   { "help",NULL, NULL, CMD_FLAG_HIDE, HelpCmd},
   { NULL}  // end of table
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
    uint32_t Led;
    uint32_t EthStatus = 0;
    uint32_t NewEthStatus;
    uint8_t Byte;
    uint16_t Count;
    bool bHaveIP = false;
    bool bRanTest = false;

    ALOG_R(gVerStr);
    CmdParserInit(gCmdTable,printf);

// Set LED GPIO's to output
    Temp = REG_RD(GPIO_BASE + GPIO_DIRECTION);
    Temp |= GPIO_BIT_RED_LED|GPIO_BIT_GREEN_LED|GPIO_BIT_BLUE_LED;
    REG_WR(GPIO_BASE + GPIO_DIRECTION,Temp);
    Led = GPIO_BIT_RED_LED;

    spi_init(CONFIG_SPILITE_BASE);
    spi_chip_init();
#if 0
    {
//       #define TEST_ADR    0x8c0000
       #define TEST_ADR    0x380000
       int i;

       spi_read(TEST_ADR,gTemp,32);
       LOG("Init data:\n");
       LOG_HEX(gTemp,32);

       spi_erase(TEST_ADR, 256*1024);
       spi_read(TEST_ADR,gTemp,32);
       LOG("After erase:\n");
       LOG_HEX(gTemp,32);

       for(i = 0; i < 32; i++) {
          gTemp[i] = (char) i;
       }
       spi_write(TEST_ADR,gTemp,32);

       spi_read(TEST_ADR,gTemp,32);
       LOG("after read:\n");
       LOG_HEX(gTemp,32);
    }
#endif

    CmdParserInit(gCmdTable,NetPrintf);
    lwip_init();
    init_default_netif();
    TcpInit();
    ClearRxFifo();

    for(; ; ) {
       NewEthStatus = ETH_STATUS & (ETH_STATUS_LINK_UP | ETH_STATUS_LINK_SPEED);
       if(EthStatus != NewEthStatus) {
          LOG_R("Ethernet Status: 0x%x\n",ETH_STATUS);
          EthStatus = NewEthStatus;
          ALOG_R("Ethernet link is %s",
                 (EthStatus & ETH_STATUS_LINK_UP) ? "up" : "down");
          if(EthStatus & ETH_STATUS_LINK_UP) {
             ALOG_R(", ");
             switch(EthStatus & ETH_STATUS_LINK_SPEED) {
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

       if(!bHaveIP && (EthStatus & ETH_STATUS_LINK_UP)) {
          if(dhcp_supplied_address(&gNetif)) {
             bHaveIP = true;
             ALOG_R("IP address assigned %s\n",ip4addr_ntoa(&gNetif.ip_addr));
          }
       }

       REG_WR(GPIO_BASE + GPIO_OUTPUT,Led);
       for(i = 0; i < 10; i++) {
          pano_netif_poll();
          timer_sleep(50);
       }
       REG_WR(GPIO_BASE + GPIO_OUTPUT,0);
       for(i = 0; i < 10; i++) {
          pano_netif_poll();
          timer_sleep(50);
       }
       switch(Led) {
          case GPIO_BIT_RED_LED:
             Led = GPIO_BIT_GREEN_LED;
             break;

          case GPIO_BIT_GREEN_LED:
             Led = GPIO_BIT_BLUE_LED;
             break;

          case GPIO_BIT_BLUE_LED:
             Led = GPIO_BIT_RED_LED;
             break;
       }

#if 0
       if(bHaveIP && !bRanTest) {
          bRanTest = true;
          strcpy(gTftp.Filename,"tftp_ldr.h");
          ipaddr_aton("192.168.123.170",&gTftp.ServerIP);
          gTftp.MaxBytes = sizeof(gTemp);
          gTftp.Ram = gTemp;
          gTftp.TransferType = TFTP_TYPE_RAM;
          ldr_tftp_init(&gTftp);
       }
       ButtonJustPressed();
#endif

       if(gSendWelcome) {
          gSendWelcome = false;
          VersionCmd(NULL);
          SendPrompt();
       }
       if(gInputReady) {
          gInputReady = false;
          ParseCmd(gRxBuf);
          gRxCount = 0;
          SendPrompt();
       }
    }

    return 0;
}

bool ButtonJustPressed()
{
   static uint32_t ButtonLast = 3;
   uint32_t Temp;
   int Ret = 0;

   Temp = REG_RD(GPIO_BASE + GPIO_INPUT) & GPIO_BIT_PANO_BUTTON;
   if(ButtonLast != 3 && ButtonLast != Temp) {
      if(Temp == 0) {
         printf("Pano button pressed\n");
         Ret = 1;
      }
 // start golden bitstream
    LOG("Attempting to switch to the golden bit stream\n");
//     REG_WR(GPIO_BASE + GPIO_OUTPUT,0x80);
    REG_WR(GPIO_BASE + BOOT_SPI_ADR,0x040000);
    LOG("Address read back 0x%x\n",REG_RD(GPIO_BASE + BOOT_SPI_ADR));
    REG_WR(GPIO_BASE + REBOOT_ADR,1);
    LOG("Rebooting\n");
    REG_WR(GPIO_BASE + REBOOT_ADR,0);
   }
   ButtonLast = Temp;

   return Ret;
}


void ClearRxFifo()
{
   int i;
   uint8_t Byte;

   if(!(ETH_STATUS & ETH_STATUS_RXEMPTY)) {
      LOG("Clearing Rx FIFO\n");
      for(i = 0; i < 2048; i++) {
         if(ETH_STATUS & ETH_STATUS_RXEMPTY) {
            break;
         }
         gRxBuf[i % MAX_ETH_FRAME_LEN] = ETH_RX();
      }
      LOG("FIFO %scleared after %d reads\n",i == 2048 ? "not " : "",i);
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
   return (uint32_t) timer_now();
}


unsigned int lwip_port_rand(void)
{
  return (unsigned int) rand();
}


err_t pano_netif_init(struct netif *netif);

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
#ifdef MIB2_STATS
         if((*cp & 0x01) == 0) {
           MIB2_STATS_NETIF_INC(&gNetif,ifinucastpkts);
         }
         else {
           MIB2_STATS_NETIF_INC(&gNetif,ifinnucastpkts);
         }
#endif
         cp++;
         for(i = 1; i < Count; i++) {
            *cp++ = ETH_RX();
         }
         cp = (uint8_t *) p->payload;

#ifdef LOG_RAW_ETH
         VLOG_R("Rx #%d: Read %d (0x%x) bytes from Rx Fifo:\n",gRxCount,Count,Count);
         VLOG_HEX(p->payload,p->len);
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
            if(*cp == '\r') {
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

   LOG("Called, tpcb: %p, len: %d\n",tpcb,len);

   gNetPrint.BytesSent += len;
   if(p->BytesSent == p->Len) {
      p->BytesSent = p->Len = p->BytesQueued = 0;
   }
   else {
   // Send some more
      NetPrintFillPcb(tpcb);
   }
}

int VersionCmd(const char *CmdLine)
{
   LOG("Called\n");
   NetPrintf(gVerStr);
}


int NetPrintf(const char *Format, ...)
{
   NetPrintInternal_t *p = &gNetPrint;
   va_list Args;
   struct tcp_pcb *pPcb;

   if(p->Len != p->BytesSent) {
      LOG("Waiting Len %d, BytesSent %d\n",p->Len,p->BytesSent);
      while(p->Len != p->BytesSent) {
      // Can't while until the previous buffer has been sent.
         pano_netif_poll();
      }
      LOG("Finished waiting\n");
   }

   va_start(Args,Format);

   p->Len = vsnprintf(p->PrintBuf,sizeof(p->PrintBuf),Format,Args);
   LOG("Wrote %d bytes to PrintBuf:\n",p->Len);
   LOG_HEX(p->PrintBuf,p->Len);
}

int NetPrintFillPcb(struct tcp_pcb *tpcb)
{
   err_t Err;
   NetPrintInternal_t *p = &gNetPrint;
   int Byte2Write;

   if((Byte2Write = p->Len - p->BytesQueued) > 0) {
   // Have data to send
      if(Byte2Write > tcp_sndbuf(tpcb)) {
         Byte2Write = tcp_sndbuf(tpcb);
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
      }
   }
}

void SendPrompt()
{
   LOG("Called\n");
   NetPrintf("ldr> ");
   LOG("Returning\n");
}

