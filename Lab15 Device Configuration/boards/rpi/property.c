/*...................................................................*/
/*                                                                   */
/*   Module:  property.c                                             */
/*   Version: 2015.0                                                 */
/*   Purpose: property tag interface                                 */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2015, Sean Lawless                    */
/*                                                                   */
/*                      ALL RIGHTS RESERVED                          */
/*                                                                   */
/* Redistribution and use in source, binary or derived forms, with   */
/* or without modification, are permitted provided that the          */
/* following conditions are met:                                     */
/*                                                                   */
/*  1. Redistributions in any form, including but not limited to     */
/*     source code, binary, or derived works, must include the above */
/*     copyright notice, this list of conditions and the following   */
/*     disclaimer.                                                   */
/*                                                                   */
/*  2. Any change or addition to this copyright notice requires the  */
/*     prior written permission of the above copyright holder.       */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED ''AS IS''. ANY EXPRESS OR IMPLIED       */
/* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES */
/* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE       */
/* DISCLAIMED. IN NO EVENT SHALL ANY AUTHOR AND/OR COPYRIGHT HOLDER  */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/*...................................................................*/
#include <system.h>
#include <board.h>
#include <string.h>
#include <stdio.h>

#if ENABLE_VIDEO || ENABLE_USB

// Thank you to the following sources.
// 
// github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
// www.abbeycat.info/2017/03/06/the-raspberry-pis-vidiocore-iv/
//

// Mailbox register map
#define MAILBOX_BASE    (PERIPHERAL_BASE + 0xB880)
#define MAILBOX_READ      MAILBOX_BASE
#define MAILBOX_STATUS    (MAILBOX_BASE + 0x18)
  #define MAILBOX_STATUS_EMPTY  (1 << 30)
  #define MAILBOX_STATUS_FULL   (1 << 31)
#define MAILBOX_WRITE     (MAILBOX_BASE + 0x20)

// Mailbox channels
#define CHANNEL_PROPERTY_TAGS_OUT     8

// Lan95xx
#define TAG_GET_MAC_ADDRESS           0x00010003

// Display
#define TAG_ALLOCATE_BUFFER           0x00040001
#define TAG_RELEASE_BUFFER            0x00048001
#define TAG_BLANK_SCREEN              0x00040002
#define TAG_GET_PHYSICAL_WIDTH_HEIGHT 0x00040003
#define TAG_SET_PHYSICAL_WIDTH_HEIGHT 0x00048003
#define TAG_GET_VIRTUAL_WIDTH_HEIGHT  0x00040004
#define TAG_SET_VIRTUAL_WIDTH_HEIGHT  0x00048004
#define TAG_GET_BUFFER_DEPTH          0x00040005
#define TAG_SET_BUFFER_DEPTH          0x00048005
#define TAG_GET_BUFFER_PITCH          0x00040008

//Power
#define TAG_SET_POWER_STATE           0x00028001
  #define DEVICE_ID_USB_HCD             3

  #define POWER_STATE_OFF              (0 << 0)
  #define POWER_STATE_ON               (1 << 0)
  #define POWER_STATE_WAIT             (1 << 1)
  #define POWER_STATE_NO_DEVICE        (1 << 1) // response only

// Property buffer codes
#define CODE_REQUEST                  0x00000000
#define CODE_RESPONSE_SUCCESS         0x80000000
#define CODE_RESPONSE_FAILURE         0x80000001

typedef struct PropertyBuffer
{
  u32 bufferSize;
  u32 code;
  u8  property[0];
}
PropertyBuffer;

typedef struct PropertyTag
{
  u32 tagId;
  u32 bufSize;
  u32 code;
}
PropertyTag;

typedef struct PropertyMACAddress
{
  PropertyTag tag;
  u8 address[6];
  u8 padding[2];
}
PropertyMACAddress;

typedef struct PropertyPowerState
{
  PropertyTag tag;
  u32 deviceId;
  u32 state;
}
PropertyPowerState;

// Must include all display property tags in one mailbox transaction
typedef struct PropertyDisplayDimensions
{
  PropertyTag pTag; // Physical
  u32 pWidth;
  u32 pHeight;
  PropertyTag vTag; // Virtual
  u32 vWidth;
  u32 vHeight;
  PropertyTag dTag; // Depth
  u32 depth;
  PropertyTag fTag; // Framebuffer 
  u32 bufferAddr;
  u32 bufferSize;
}
PropertyDisplayDimensions;

// Static local functions

/*...................................................................*/
/* flush: wait for all mailboxes to complete                         */
/*                                                                   */
/*...................................................................*/
void flush(void)
{
  // Read from the mailbox until status is empty
  while (!(REG32(MAILBOX_STATUS) & MAILBOX_STATUS_EMPTY))
    REG32(MAILBOX_READ);
}

/*...................................................................*/
/*      write: Write data to a mailbox channel                       */
/*                                                                   */
/*      Input: channel to write to                                   */
/*             data to write                                         */
/*                                                                   */
/*...................................................................*/
void write(u32 channel, uintptr_t data)
{
  // Read the mailbox status until it is not full
  while (REG32(MAILBOX_STATUS) & MAILBOX_STATUS_FULL) ;

  // Add the channel number before writing the data to mailbox.
  REG32(MAILBOX_WRITE) = channel | data;
}

/*...................................................................*/
/*      read: Read data from a mailbox channel                       */
/*                                                                   */
/*      Input: channel to read from                                  */
/*                                                                   */
/*     Return: data read from channel                                */
/*...................................................................*/
u32 read(u32 channel)
{
  u32 result;

  // Read the mailbox until a result for this channel.
  for (result = REG32(MAILBOX_READ); (result & 0xF) != channel;
       result = REG32(MAILBOX_READ))
  {
    // Read the mailbox status until it is not empty
    while (REG32(MAILBOX_STATUS) & MAILBOX_STATUS_EMPTY);
  }

  // Return the result
  return result;
}

/*...................................................................*/
/* write_read: Write and then read data to/from a mailbox channel    */
/*                                                                   */
/*      Input: channel to read/write from/to                         */
/*             data to write                                         */
/*                                                                   */
/*     Return: data read from channel                                */
/*...................................................................*/
static u32 write_read(u32 channel, uintptr_t data)
{
  u32 result;

  // Flush all before writing to the mailbox
  flush();
  write(channel, data);

  // Read and return mailbox result
  result = read(channel);
  
  // Output error if result of read does not match write channel
  if ((result & 0x0F) != channel)
    puts("Read channel does not match write!");

  // Return the result of read after removing the channel
  return result & ~0x0F;
}

/*...................................................................*/
/* property_get: Write and then read through property tag interface  */
/*                                                                   */
/*      Input: property is a pointer to the data to write/read       */
/*             propertySize is the size of the property              */
/*                                                                   */
/*     Return: zero (0) on success, negative value on error          */
/*...................................................................*/
static int property_get(void *property, u32 propertySize)
{
  // Determine length of property buffer based on the tag size
  u32 bufferSize = sizeof(PropertyBuffer) + propertySize + sizeof(u32);
  // Declare buffer with additional 15 bytes for alignment
  u8 buffer[bufferSize + 15];
  // Adjust pointer so that it is exactly 16 byte aligned
  PropertyBuffer *propBuffer =
                           (PropertyBuffer *)(((u32)buffer + 15) & ~15);
                                      /* __attribute__((aligned(16))) */
  u32 *endTag, bufferAddress;

  // Initialize with size, request code and copy tags
  propBuffer->bufferSize = bufferSize;
  propBuffer->code = CODE_REQUEST;
  memcpy(propBuffer->property, property, propertySize);

  // End the property tag
  endTag = (u32 *)(propBuffer->property + propertySize);
  *endTag = 0;

  // Add the GPU memory base to the address and write the mailbox,
  // reading back the result, which will match the write if success
  bufferAddress = GPU_MEM_BASE | (u32)propBuffer;
  if (write_read(CHANNEL_PROPERTY_TAGS_OUT, bufferAddress) !=
                                                          bufferAddress)
  {
    puts("write_read() failed");
    return -1;
  }

  // Check response and return failure if not success
  if (propBuffer->code != CODE_RESPONSE_SUCCESS)
  {
    puts("Property buffer code is not response");
    return -1;
  }

  // Copy property tag result to tag parameter and return success
  memcpy(property, propBuffer->property, propertySize);
  return 0;
}

// Global functions

#if ENABLE_VIDEO
/*...................................................................*/
/* SetDisplayResolution: Write new framebuffer properties            */
/*                                                                   */
/*      Input: width is screen width (X)                             */
/*             height is screen height (Y)                           */
/*             depth is color depth (bpp)                            */
/*     Output: bufferAddr is a pointer to the framebuffer            */
/*                                                                   */
/*     Return: Framebuffer size on success, zero if error            */
/*...................................................................*/
u32 SetDisplayResolution(u32 *width, u32 *height, u32 *depth,
                         u32 *bufferAddr)
{
  PropertyDisplayDimensions dimensions;

  dimensions.pTag.tagId = TAG_SET_PHYSICAL_WIDTH_HEIGHT;
  dimensions.pTag.bufSize = 8; // 4 bytes each width and height
  dimensions.pTag.code = CODE_REQUEST;
  dimensions.pWidth = *width;
  dimensions.pHeight = *height;
  
  dimensions.vTag.tagId = TAG_SET_VIRTUAL_WIDTH_HEIGHT;
  dimensions.vTag.bufSize = 8; // 4 bytes each width and height
  dimensions.vTag.code = CODE_REQUEST;
  dimensions.vWidth = *width;
  dimensions.vHeight = *height;
  
  dimensions.dTag.tagId = TAG_SET_BUFFER_DEPTH;
  dimensions.dTag.bufSize = 4; // 4 bytes depth
  dimensions.dTag.code = CODE_REQUEST;
  dimensions.depth = *depth;

  dimensions.fTag.tagId = TAG_ALLOCATE_BUFFER;
  dimensions.fTag.bufSize = 8; // 4 bytes each addr and size
  dimensions.fTag.code = CODE_REQUEST;
  dimensions.bufferAddr = 0;
  dimensions.bufferSize = 0;

  // If success then assign the height and width output parameters
  if (property_get(&dimensions, sizeof(dimensions)) == 0)
  {
    // If resolution unspecified or allocation matches
    if (!*width || !*height || ((dimensions.pWidth == *width) &&
        (dimensions.pHeight == *height) &&
        (dimensions.vWidth == *width) &&
        (dimensions.vHeight == *height) &&
        (dimensions.depth == *depth)))
    {
      // Assign width, height, depth and buffer address, returning size
      *width  = dimensions.pWidth;
      *height = dimensions.pHeight;
      *depth = dimensions.depth;
      *bufferAddr = dimensions.bufferAddr;
      return dimensions.bufferSize;
    }

    // Otherwise out of sync so report an error
    else
      puts("ERROR framebuffer not valid\n");
  }

  // Otherwise zero the height and width output parameters
  else
    puts("set display resolution property failed");

  // Return failure
  return 0;
}
#endif

#if ENABLE_USB
// Supported devices are DEVICE_ID_SD_CARD or DEVICE_ID_USB_CARD
int SetUsbPowerStateOn(void)
{
  PropertyPowerState powerState;

  powerState.tag.tagId = TAG_SET_POWER_STATE;
  powerState.tag.bufSize = 8;
  powerState.tag.code = CODE_REQUEST;

  powerState.deviceId = DEVICE_ID_USB_HCD;
  powerState.state = POWER_STATE_ON | POWER_STATE_WAIT;
  if (property_get(&powerState, sizeof(powerState)) ||
      (powerState.state & POWER_STATE_NO_DEVICE) ||
      !(powerState.state & POWER_STATE_ON))
    return -1;

  return 0;
}

int SetUsbPowerStateOff(void)
{
  PropertyPowerState powerState;

  powerState.tag.tagId = TAG_SET_POWER_STATE;
  powerState.tag.bufSize = 8;
  powerState.tag.code = CODE_REQUEST;

  powerState.deviceId = DEVICE_ID_USB_HCD;
  powerState.state = POWER_STATE_OFF;
  if (property_get(&powerState, sizeof(powerState)) ||
      (powerState.state & POWER_STATE_NO_DEVICE) ||
      (powerState.state & POWER_STATE_ON))
    return -1;

  return 0;
}

#if ENABLE_USB_ETHER
int GetMACAddress(u8 buffer[6])
{
  PropertyMACAddress macAddress;

  macAddress.tag.tagId = TAG_GET_MAC_ADDRESS;
  macAddress.tag.bufSize = 8;
  macAddress.tag.code = CODE_REQUEST;

  if (property_get(&macAddress, sizeof(macAddress)))
    return -1;

  memcpy(buffer, macAddress.address, 6);
  return 0;
}
#endif /* ENABLE_USB_ETHER */
#endif /* ENABLE_USB */

#endif /* ENABLE_VIDEO || ENABLE_USB */
