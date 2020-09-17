/* Derived from Embedded Xinu, Copyright (C) 2013. All rights reserved. */
/*   Used with permission under Creative Commons Zero (Public Domain). */

#include <system.h>
#include <string.h>
#include <stdio.h>

#if ENABLE_UDP

#include <opt.h>
#include <lwip/tftp.h>
#include <lwip/udp.h>
#include <lwip/ip_addr.h>
#include <board.h>


#define PORT_TFTP  69

#define OK 0
#define SYSERR -1
#define TIMEOUT -2
#define EOF -3

/* Endian conversion macros*/
#if BYTE_ORDER == LITTLE_ENDIAN
#define hs2net(x) (unsigned) ((((x)>>8) &0xff) | (((x) & 0xff)<<8))
#define net2hs(x) hs2net(x)
#define hl2net(x)   ((((x)& 0xff)<<24) | (((x)>>24) & 0xff) | \
    (((x) & 0xff0000)>>8) | (((x) & 0xff00)<<8))
#define net2hl(x) hl2net(x)
#else
#define hs2net(x) (x)
#define net2hs(x) (x)
#define hl2net(x) (x)
#define net2hl(x) (x)
#endif

/* Stress testing--- randomly ignore this percent of valid received data
 * packets.  */
#define TFTP_DROP_PACKET_PERCENT 0

struct tftpcb
{
  char filename[TFTP_BLOCK_SIZE];
  ip_addr_t local_ip;
  ip_addr_t server_ip;
  struct udp_pcb *send_udpdev;
  struct udp_pcb *recv_udpdev;
  u32 num_rreqs_sent;
  u32 block_recv_tries;
  u32 next_block_number;
  int status;
  struct timer block_max_end_timer;// = 0;  /* This value is not used, but
                                  //   gcc fails to detect it.  */
  struct timer block_attempt_timer;
  struct tftpPkt *pkt;
  struct pbuf *out; /* outgoing buffer is recycled */
  char *destination;
};

struct tftpcb TFtpCB;
extern struct netif Netif;
void *Data = NULL;
int Ip1 = 192, Ip2 = 168, Ip3 = 1, Ip4 = 3;

void tftp_init(void)
{
  Ip1 = 192;
  Ip2 = 168;
  Ip3 = 1;
  Ip4 = 3;
  Data = NULL;
  bzero(&TFtpCB, sizeof(struct tftpcb));
}

static void tftp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
    ip_addr_t *addr, u16_t port)
{
  static u16 bnum;

    struct tftpcb *tftp = &TFtpCB;

    if (tftp->next_block_number == 1)
    {
      if (tftp->server_ip.addr != addr->addr)
      {
        TFTP_TRACE("server ip is not equal to from address.");
        return;
      }
      TFTP_TRACE("Connect after first packet from port %d.", port);
      udp_connect(tftp->recv_udpdev, addr, port);
      if (tftp->recv_udpdev->remote_port != port)
      {
        TFTP_TRACE("Send port %d differs from recv port %d.",
                   tftp->recv_udpdev->remote_port, port);
        return;
      }
      bnum = 0;
    }

    /* Loop until file is fully downloaded or an error condition occurs.  The
     * basic idea is that the client receives DATA packets one-by-one, each of
     * which corresponds to the next block of file data, and the client ACK's
     * each one before the server sends the next.  But the actual code below is
     * a bit more complicated as it must handle timeouts, retries, invalid
     * packets, etc.  */
    {
      int retval;
        u16 opcode;
        u16 recv_block_number;
        int wrong_source;
        u16 block_nbytes;
        struct tftpPkt *pkt = p->payload;

        retval = p->len;


        /* Otherwise, 'retval' is the length of the received TFTP packet.  */
        opcode = net2hs(pkt->opcode);
        recv_block_number = net2hs(pkt->DATA.block_number);

        if (retval < 4 || TFTP_OPCODE_DATA != opcode ||
            (recv_block_number != (u16)tftp->next_block_number &&
             (tftp->next_block_number == 1 ||
              recv_block_number != (u16)tftp->next_block_number - 1)))
        {
            /* Check for TFTP ERROR packet  */
            if (/*!wrong_source &&*/ (retval >= 2 && TFTP_OPCODE_ERROR == opcode))
            {
                TFTP_TRACE("Received TFTP ERROR opcode packet; aborting.");
                tftp->status = SYSERR;
                goto out_kill_recv_thread;
            }
            TFTP_TRACE("Received invalid or unexpected packet.");
            TFTP_TRACE("\n\t res %d, rblk %d, nblk %d, op %d\n",
                retval, recv_block_number, tftp->next_block_number,
                opcode);

            /* Ignore the bad packet and try receiving again.  */
            pbuf_free(p);
            return;// TASK_IDLE;//continue;
        }

        /* Received packet is a valid TFTP DATA packet for either the next block
         * or the previous block.  */


    #if TFTP_DROP_PACKET_PERCENT != 0
        /* Stress testing.  */
        if (rand() % 100 < TFTP_DROP_PACKET_PERCENT)
        {
            TFTP_TRACE("WARNING: Ignoring valid TFTP packet for test purposes.");
            pbuf_free(p);
            return;// TASK_IDLE;//continue;
        }
    #endif

        /* Handle receiving the next data block.  */
        block_nbytes = TFTP_BLOCK_SIZE;
        if (recv_block_number == (u16)tftp->next_block_number)
        {
            block_nbytes = retval - 4;
            TFTP_TRACE("Received block %u (%u bytes)",
                       recv_block_number, block_nbytes);

            if (++bnum != recv_block_number)
              printf("TFTP OutOfSync %d != %d\n", bnum - 1,
                     recv_block_number - 1);

//            pkt->DATA.data[block_nbytes] = '\0';
//            printf("%d: %s\r\n", recv_block_number, pkt->DATA.data);
#if ENABLE_BOOTLOADER
            /* Save the block to the memory location chosen */
            memcpy(&tftp->destination[(recv_block_number - 1) *
                   TFTP_BLOCK_SIZE], pkt->DATA.data, block_nbytes);
#endif
            tftp->next_block_number++;
            tftp->block_recv_tries = 0;
        }

        /* Acknowledge the block received.  */
        retval = tftpSendACK(tftp->send_udpdev, recv_block_number);

        /* A TFTP Get transfer is complete when a short data block has been
         * received.   Note that it doesn't really matter from the client's
         * perspective whether the last data block is acknowledged or not;
         * however, the server would like to know so it doesn't keep re-sending
         * the last block.  For this reason we did send the final ACK packet but
         * will ignore failure to send it.  */
        if (block_nbytes < TFTP_BLOCK_SIZE)
        {
            tftp->status = EOF;
            goto out_kill_recv_thread;//break;
        }

        /* Break if sending the ACK failed.  */
        if (SYSERR == retval)
        {
            tftp->status = SYSERR;
            goto out_kill_recv_thread;//break;
        }

        /* Start the block timer again */
        tftp->block_max_end_timer = TimerRegister(TFTP_BLOCK_TIMEOUT);
    }

    pbuf_free(p);
    return;// TASK_IDLE;

    /* Clean up and return.  */
out_kill_recv_thread:

    /* schedule an immediate time out */
    TFTP_TRACE("TFTP command finished");
    pbuf_free(p);
    tftp->block_max_end_timer = TimerRegister(0);
}

int Tftp(char *command)
{
  static int first = 0;
  int status;
  int local_port;
  char *fname, *arg1;
  char *arg2, *ip;

  if (first++ == 0)
    Data = NULL;

  if (Data == NULL)
  {
    {
      fname = NULL;
      ip = NULL;
      arg1 = strchr(command, ' ');
      arg2 = NULL;

      if (arg1)
        arg2 = strchr(&arg1[1], ' ');

      if (arg1 && arg2)
      {
        ip = arg1;
        fname = arg2;
      }
      else if (arg1)
      {
        fname = arg1;
      }

      /* initialize the tftp boot loader request */
      bzero(&TFtpCB, sizeof(struct tftpcb));

      if (fname)
        strcpy(TFtpCB.filename, &fname[1]);
      else
        strcpy(TFtpCB.filename, "kernel7.img"); /* default app name */

      if (ip)
      {
        char *ipnext;

        ip = &ip[1];
        ipnext = strchr(ip, '.');
        if (ipnext)
        {
          ipnext[0] = '\0';
          Ip1 = atoi(ip);
          ip = ipnext;
        }
        ip = &ip[1];
        ipnext = strchr(ip, '.');
        if (ipnext)
        {
          ipnext[0] = '\0';
          Ip2 = atoi(ip);
          ip = ipnext;
        }
        ip = &ip[1];
        ipnext = strchr(ip, '.');
        if (ipnext)
        {
          ipnext[0] = '\0';
          Ip3 = atoi(ip);
          ip = ipnext;
        }
        ip = &ip[1];
        ipnext = strchr(ip, ' ');
        if (ipnext)
          ipnext[0] = '\0';
        Ip4 = atoi(ip);
      }

      IP4_ADDR(&TFtpCB.server_ip, Ip1, Ip2, Ip3, Ip4); /* default app name */
      TFtpCB.send_udpdev = NULL;
      TFtpCB.recv_udpdev = NULL;

      TFtpCB.destination = (void *)_run_location();

      Data = &TFtpCB;
      printf("Download image %s from TFTP server %d.%d.%d.%d...",
             TFtpCB.filename, Ip1, Ip2, Ip3, Ip4);
    }

#ifdef ENABLE_TFTP_TRACE
    TFTP_TRACE("Downloading %s from ", TFtpCB.filename);
//    netaddrprintf(&tftp->server_ip);
    TFTP_TRACE(" using local_ip = ");
//    netaddrprintf(&tftp->local_ip);
    TFTP_TRACE("\n");
#endif

    TFtpCB.send_udpdev = udp_new();//netconn_new(NETCONN_UDP);
    LWIP_ASSERT("tftp->udpdev != NULL", tftp->send_udpdev != NULL);

    /* connect the outgoing UDP socket */
    udp_connect(TFtpCB.send_udpdev, &TFtpCB.server_ip, PORT_TFTP);//    netconn_connect(conn, &tftp->server_ip, 69/*UDP_PORT_TFTP*/);

    /* Begin the download by requesting the file.  */
    status = tftpSendRRQ(TFtpCB.send_udpdev, TFtpCB.filename);
    if (status == SYSERR)
    {
        TFtpCB.status = SYSERR;
        udp_remove(TFtpCB.send_udpdev);
        TFtpCB.send_udpdev = NULL;//(void *)-1;
        TFtpCB.recv_udpdev = NULL;//(void *)-1;
        Data = NULL;
        return TASK_FINISHED;
    }
    TFtpCB.block_max_end_timer = TimerRegister(TFTP_INIT_BLOCK_TIMEOUT);
    TFtpCB.num_rreqs_sent = 1;
    TFtpCB.next_block_number = 1;
    TFtpCB.block_recv_tries = 0;

    local_port = TFtpCB.send_udpdev->local_port;

    udp_remove(TFtpCB.send_udpdev);
    TFtpCB.send_udpdev = NULL;

    /* register the tftp_recv function as callback for this socket */
    TFtpCB.recv_udpdev = udp_new();
    LWIP_ASSERT("tftp->recv_udpdev != NULL", TFtpCB.recv_udpdev != NULL);
    TFTP_TRACE("TFTP receiver on local port %d\n", local_port);
    if (udp_bind(TFtpCB.recv_udpdev, IP_ADDR_ANY, local_port) != ERR_OK)
    {
      TFTP_TRACE("udp bind failed for TFTP receiver\n");
      TFtpCB.status = SYSERR;
      udp_remove(TFtpCB.recv_udpdev);
      TFtpCB.send_udpdev = NULL;//(void *)-1;
      TFtpCB.recv_udpdev = NULL;//(void *)-1;
      Data = NULL;
      return TASK_FINISHED;
    }

    /* change to new socket for sending and register receive callback */
    TFtpCB.send_udpdev = TFtpCB.recv_udpdev;
    udp_recv(TFtpCB.recv_udpdev, tftp_recv, &Netif);
  }

  status = tftpGet(Data);
  if (status == TASK_FINISHED)
  {
    if (TFtpCB.status == EOF)
      puts("transfer complete");
    else if (TFtpCB.status == TIMEOUT)
      puts("transfer timeout");
    else
      puts("transfer failed");
    Data = NULL;
  }
  return status;
}

/**
 * Send a TFTP ACK (Acknowledge) packet over a UDP connection to the TFTP
 * server.  For TFTP Get transfers, this informs the TFTP server that a given
 * data packet (having a specific block number) has been received.  Not intended
 * to be used outside of the TFTP code.
 *
 * @param udpdev
 *      Device descriptor for the open UDP device.
 * @param block_number
 *      Block number to acknowledge.
 *
 * @return
 *      OK if packet sent successfully; SYSERR otherwise.
 */
int tftpSendACK(void *conn, u16 block_number)
{
  struct tftpPkt *pkt;
  struct pbuf *buf;
//    struct udp *udpptr = &udptab[udpdev];

    TFTP_TRACE("ACK block %u", block_number);

  buf = pbuf_alloc(PBUF_TRANSPORT, 4/*sizeof(struct tftpPkt)*/,
                   PBUF_RAM);
  if (buf)
  {
    pkt = (struct tftpPkt *)buf->payload;
    pkt->opcode = hs2net(TFTP_OPCODE_ACK);
    pkt->ACK.block_number = hs2net(block_number);
    buf->len = 4;
    udp_sendto_if(conn, buf, &TFtpCB.server_ip,
                  TFtpCB.send_udpdev->remote_port, &Netif);
    pbuf_free(buf);
    return OK;
  }
  return SYSERR;
}

/**
 * Send a TFTP RRQ (Read Request) packet over a UDP connection to the TFTP
 * server.  This instructs the TFTP server to begin sending the contents of the
 * specified file.  Not intended to be used outside of the TFTP code.
 *
 * @param udpdev
 *      Device descriptor for the open UDP device.
 * @param filename
 *      Name of the file to request.
 *
 * @return
 *      OK if packet sent successfully; SYSERR otherwise.
 */
int tftpSendRRQ(void *conn, const char *filename)
{
  u8 *p;
  u32 filenamelen;
  u32 pktlen;
  struct tftpPkt *pkt;
  struct pbuf *buf;

  /* Do sanity check on filename.  */
  filenamelen = strnlen(filename, 256);
  if (0 == filenamelen || 256 == filenamelen)
  {
      TFTP_TRACE("Filename is invalid.");
      return SYSERR;
  }

  TFTP_TRACE("RRQ \"%s\" (mode: octet) fnamelen %d", filename,
             filenamelen);

  /* Write the resulting packet to the UDP device.  */
  pktlen = 2 + filenamelen + 1 + 6;//p - (u8 *)&pkt;


  buf = pbuf_alloc(PBUF_TRANSPORT, pktlen, PBUF_RAM);
  if (buf)
  {
    pkt = (struct tftpPkt *)buf->payload;

    /* Set TFTP opcode to RRQ (Read Request).  */
    pkt->opcode = hs2net(TFTP_OPCODE_RRQ);

    /* Set up filename and mode.  */
    p = pkt->RRQ.filename_and_mode;
    memcpy(p, filename, filenamelen + 1);
    TFTP_TRACE("p (fname) is: %s\n", p);

    memcpy(&p[filenamelen + 1], "octet", 6);
    TFTP_TRACE("p (type) is: %s\n", &p[filenamelen + 1]);

    buf->len = pktlen;
    udp_sendto_if(conn, buf/*TFtpCB.out*/, &TFtpCB.server_ip, PORT_TFTP,
                  &Netif);
    pbuf_free(buf);
    return OK;
  }
  return SYSERR;
}

extern int NetUp;

int tftpGet(void *t)
{
  struct tftpcb *tftp = t;
  int retval, local_port;

  if (!NetUp)
  {
      TFTP_TRACE("Network down");
      return TASK_FINISHED;
  }

  /* Make sure the required parameters have been specified.  */
  if ((tftp == NULL) || ('\0' == tftp->filename[0]))
  {
      TFTP_TRACE("Invalid parameter.");
      return TASK_FINISHED;
  }

  if ((TimerRemaining(&tftp->block_max_end_timer) == 0) &&
      (tftp->send_udpdev != NULL))
  {
    if (tftp->status)
    {
      udp_remove(tftp->send_udpdev);
      tftp->send_udpdev = NULL;
      tftp->recv_udpdev = NULL;
      Data = NULL;
      return TASK_FINISHED;
    }

    /* Timeout was reached.  */
    TFTP_TRACE("Timeout on block %u", tftp->next_block_number);

    /* If the client is still waiting for the very first reply from the
     * server, don't fail on the first timeout; instead wait until the
     * client has had the chance to re-send the RRQ a few times.  */
    if (tftp->next_block_number == 1 &&
        tftp->num_rreqs_sent < TFTP_INIT_BLOCK_MAX_RETRIES)
    {
      if (tftp->send_udpdev == NULL)
        tftp->send_udpdev = udp_new();

      TFTP_TRACE("Trying RRQ again (try %u of %u)",
                 tftp->num_rreqs_sent + 1, TFTP_INIT_BLOCK_MAX_RETRIES);
      udp_connect(tftp->send_udpdev, &tftp->server_ip, PORT_TFTP);
      retval = tftpSendRRQ(tftp->send_udpdev, tftp->filename);
      if (retval == SYSERR)
      {
        udp_remove(tftp->send_udpdev);
        tftp->send_udpdev = NULL;
        tftp->recv_udpdev = NULL;
        Data = NULL;
        return TASK_FINISHED;
      }
      tftp->block_recv_tries = 0;
      tftp->num_rreqs_sent++;
      tftp->block_max_end_timer = TimerRegister(
                                  TFTP_INIT_BLOCK_TIMEOUT);
      local_port = tftp->send_udpdev->local_port;
      udp_remove(tftp->send_udpdev);
      tftp->send_udpdev = NULL;

      /* register the tftp_recv function as callback for this socket */
      if (tftp->recv_udpdev)
        udp_remove(tftp->recv_udpdev);

      tftp->recv_udpdev = udp_new();
      LWIP_ASSERT("tftp->recv_udpdev != NULL", tftp->recv_udpdev != NULL);
      TFTP_TRACE("TFTP receiver on local port %d\n", local_port);
      if (udp_bind(tftp->recv_udpdev, IP_ADDR_ANY, local_port) != ERR_OK)
      {
        TFTP_TRACE("udp bind failed for TFTP receiver\n");
        tftp->status = SYSERR;
        udp_remove(TFtpCB.recv_udpdev);
        tftp->send_udpdev = NULL;
        tftp->recv_udpdev = NULL;
        Data = NULL;
        return TASK_FINISHED;
      }

      tftp->send_udpdev = tftp->recv_udpdev;
      udp_recv(tftp->recv_udpdev, tftp_recv, &Netif);
      return TASK_IDLE;
    }
    else if (tftp->num_rreqs_sent >= TFTP_INIT_BLOCK_MAX_RETRIES)
    {
      udp_disconnect(tftp->recv_udpdev);
      udp_remove(tftp->recv_udpdev);
      tftp->send_udpdev = NULL;
      tftp->recv_udpdev = NULL;
      return TASK_FINISHED;
    }
  }

  return TASK_IDLE;
}

#endif /* ENABLE_NETWORK */
