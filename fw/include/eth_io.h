#ifndef _ETH_IO_H_
#define _ETH_IO_H_

#define ETH_RX_OFFSET         0x0
#define ETH_TX_OFFSET         0x4
#define ETH_STATUS_OFFSET     0x8

#define ETH_STATUS_TXRESET      (1 << 0)
#define ETH_STATUS_RXRESET      (1 << 1)
#define ETH_STATUS_RXEMPTY      (1 << 2)
#define ETH_STATUS_TXFULL       (1 << 3)
#define ETH_STATUS_IE           (1 << 4)
#define ETH_STATUS_LINK_UP      (1 << 5)

#define ETH_LINK_SPEED_SHIFT     6
#define ETH_STATUS_LINK_SPEED   (3 << ETH_LINK_SPEED_SHIFT)

#define ETH_RX()   (*((volatile uint8_t *)(ETH_BASE + ETH_RX_OFFSET)))
#define ETH_TX     *((volatile uint8_t *)(ETH_BASE + ETH_TX_OFFSET))
#define ETH_STATUS *((volatile uint32_t *)(ETH_BASE + ETH_STATUS_OFFSET))

// Speed constants
#define SPEED_1000MBPS      (2 << ETH_LINK_SPEED_SHIFT)
#define SPEED_100MBPS       (1 << ETH_LINK_SPEED_SHIFT)
#define SPEED_10MBPS        (0 << ETH_LINK_SPEED_SHIFT)
#define SPEED_UNSPECIFIED   (3 << ETH_LINK_SPEED_SHIFT)

// Change this if you like, this was copied from the label on one of Skip's G2s
#define MAC_ADR               0x00,0x1c,0x02,0x70,0x1d,0x5d

#endif   // _ETH_IO_H_

