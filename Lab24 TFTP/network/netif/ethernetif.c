/**
 * @file
 * Ethernet Interface for ComputerSystems to lwIP
 *
 */




#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include "netif/etharp.h"
#include "netif/ppp_oe.h"

#include <lwip/dhcp.h>
#include <init.h>

#include <stdio.h>
#include <string.h>

#if ENABLE_ETHER

/* Define these to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 't'
#define IFNAME2 'h'

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
struct ethernetif {
  struct eth_addr *ethaddr;
  /* Add whatever per-interface state that is needed here. */
};

/* Forward declarations. */
void  ethernetif_input(struct netif *netif);
#if ENABLE_NETWORK
extern err_t ethernetif_init(struct netif *netif);
extern void ethernetif_input(struct netif *netif);

struct netif Netif;
static ip_addr_t Ipaddr, Nmask, Gw;

extern int NetUp;
int NetStart(char *command);
#endif

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void
low_level_init(struct netif *netif)
{

   if (!LanGetMAC())
  {
    puts("Ethernet device not found");
    return;
  }


  /* Set MAC hardware address and length */
  memcpy(netif->hwaddr, LanGetMAC(), ETHARP_HWADDR_LEN);
  netif->hwaddr_len = ETHARP_HWADDR_LEN;


  /* maximum transfer unit */
  netif->mtu = 1500;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

  /* Do whatever else is needed to initialize interface. */
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
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
  struct pbuf *q;

//  initiate transfer();
#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

  /* Send the data from the pbuf to the interface, one pbuf at a
     time. The size of the data in each pbuf is kept in the ->len
     variable. */
  for(q = p; q != NULL; q = q->next)
  {
    LanDeviceSendFrame(q->payload, q->len);
  }

//  signal that packet should be sent();

#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
struct ethernetif EthernetIf;

err_t
ethernetif_init(struct netif *netif)
{
//  struct ethernetif *ethernetif;

  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "asynos";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->state = /*NULL;*/&EthernetIf;//ethernetif;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  netif->name[2] = IFNAME2;
  netif->name[3] = '\0';
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
#if ENABLE_NETWORK
  netif->output = etharp_output;
#endif
  netif->linkoutput = low_level_output;

  EthernetIf.ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

#if ENABLE_NETWORK

int NetIn(u8 *frame, int frameLength)
{
  struct pbuf *p;

  //Allocate a new protocol buffer to hold this frame
  p = pbuf_alloc(PBUF_RAW, frameLength, PBUF_POOL);
  if (p != NULL)
  {
    struct eth_hdr ethhdr;

    p->payload += 2; // Move 2 bytes before 6 byte Ethernet header
                     // so IP packet will be 4 byte aligned
    memcpy (p->payload, frame /*+ 10*/, frameLength); // copy to new buffer
                                                    // skipping Rx status

    // Copy the (unaligned) Ethernet header from the payload
    memcpy(&ethhdr, p->payload, sizeof(struct eth_hdr));

    // Assign protocol buffer length
    p->len = p->tot_len = frameLength;

    // TODO - put on a queue and create TCP/IP stack to process?

    //Process the Ethernet frame based on the Ethernet type field
    switch (htons(ethhdr.type))
    {
      // IP or ARP packet
      case ETHTYPE_IP:
      case ETHTYPE_ARP:
        // send pbuf to TCP/IP stack which is required to free the pbuf
        if (Netif.input(p, &Netif) != ERR_OK)
        {
          printf("ethernetif_input: IP input error\n");
        }
        break;

      // otherwise if no stack found for the frame then free it
      default:
#if 0
        putchar('d');
        putbyte(ethhdr.dest.addr[0]); putbyte(ethhdr.dest.addr[1]);
        putbyte(ethhdr.dest.addr[2]); putbyte(ethhdr.dest.addr[3]);
        putbyte(ethhdr.dest.addr[4]); putbyte(ethhdr.dest.addr[5]);
        putchar('s');
        putbyte(ethhdr.src.addr[0]); putbyte(ethhdr.src.addr[1]);
        putbyte(ethhdr.src.addr[2]); putbyte(ethhdr.src.addr[3]);
        putbyte(ethhdr.src.addr[4]); putbyte(ethhdr.src.addr[5]);
        putchar('t'); putu32(ethhdr.type);
        putchar('l'); putu32(p->len);
        puts(" - Ethernet frame type unknown");
#endif
        pbuf_free(p);
        break;
    }
  }
  else
    puts("Could not allocate pbuffer, frame discarded");
  return 0;
}

int NetStart(char *command)
{
  if (!UsbUp)
  {
    puts("USB required for network, use 'usb' command to enable.");
    return TASK_FINISHED;
  }

  if (!NetUp)
  {
    /* initialize the TCP/IP stack */
    puts("Ethernet detected, bringing up IPv4 network...");
    lwip_init();

    /*need delay/wait */
#if !LWIP_DHCP
    puts("IPv4 stack initialize static. ");

    /* Configure a static IP address */
    IP4_ADDR(&Ipaddr, 192,168,1,202);
    IP4_ADDR(&Nmask, 255,255,255,0);
    IP4_ADDR(&Gw, 192,168,1,1);

    /* Add the Ethernet interface and set it
     *
     *  as the default network */
    netif_set_default(netif_add(&Netif, &Ipaddr, &Nmask,
                          &Gw, NULL, ethernetif_init, ethernet_input));

    /* Enable the network interface (Ethernet) */
    netif_set_up(&Netif);
#else
    puts("IPv4 stack initialize DHCP. ");

     /* Configure an empty IP address */
    IP4_ADDR(&Ipaddr, 0,0,0,0);
    IP4_ADDR(&Nmask, 0,0,0,0);
    IP4_ADDR(&Gw, 0,0,0,0);

    bzero(&Netif, sizeof(struct netif));

    /* Add empty IP address to Ethernet interface as default. */
    if (netif_add(&Netif, &Ipaddr, &Nmask,
                  &Gw, NULL, ethernetif_init, ethernet_input))
    {
      netif_set_default(&Netif);
      netif_set_up(&Netif);
    }
#endif /* LWIP_DHCP */

    /* Initialize asynchronous receive for inbound frames */
    if (LanReceiveAsync())
      return TASK_FINISHED;

    /* Start DHCP and enable the network interface (Ethernet) */
#if LWIP_DHCP
    dhcp_start(&Netif);
    puts("Network up: Asking DHCP server for address");
#else
    puts("Network up: Static IPv4 address 192.168.1.202");
#endif
    NetUp = TRUE;
  }
  else
    puts("Network already up");
  return TASK_FINISHED;
}
#endif /* ENABLE_NETWORK */

#endif /* ENABLE_ETHER */
