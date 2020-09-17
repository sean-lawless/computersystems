/* Derived from Embedded Xinu, Copyright (C) 2013. All rights reserved. */
/*   Used with permission under Creative Commons Zero (Public Domain). */

#ifndef _TFTP_H_
#define _TFTP_H_

#include <system.h>
#include <lwip/api.h>

#define TFTP_OPCODE_RRQ   1
#define TFTP_OPCODE_WRQ   2
#define TFTP_OPCODE_DATA  3
#define TFTP_OPCODE_ACK   4
#define TFTP_OPCODE_ERROR 5

#define TFTP_TRACE(...)

#define TFTP_RECV_THR_STK   NET_THR_STK
#define TFTP_RECV_THR_PRIO  NET_THR_PRIO

/* Maximum number of seconds to wait for a block, other than the first, before
 * aborting the TFTP transfer.  */
#define TFTP_BLOCK_TIMEOUT      (10 * MICROS_PER_SECOND)

/** Maximum number of seconds to wait for the first block before re-sending the
 * RREQ.  */
#define TFTP_INIT_BLOCK_TIMEOUT (MICROS_PER_SECOND * 3)

/** Maximum number of times to send the initial RREQ.  */
#define TFTP_INIT_BLOCK_MAX_RETRIES 5

#define TFTP_BLOCK_SIZE     512

struct tftpPkt
{
    uint16_t opcode;
    union
    {
        struct
        {
            char filename_and_mode[2 + TFTP_BLOCK_SIZE];
        } RRQ;
        struct
        {
            uint16_t block_number;
            uint8_t data[TFTP_BLOCK_SIZE];
        } DATA;
        struct
        {
            uint16_t block_number;
        } ACK;
    };
};

#define TFTP_MAX_PACKET_LEN      516

/**
 * @ingroup tftp
 *
 * Type of a caller-provided callback function that consumes data downloaded by
 * tftpGet().  See tftpGet() for more details.
 */
typedef int (*tftpRecvDataFunc)(const u8 *data, u32 len, void *ctx);

int tftpGet(void *t);

int tftpSendACK(void *conn, u16 block_number);

int tftpSendRRQ(void *conn, const char *filename);

void tftp_init(void);

#endif /* _TFTP_H_ */
