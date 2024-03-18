/*...................................................................*/
/*                                                                   */
/*   Module:  massstorage.c                                          */
/*   Version: 2023.0                                                 */
/*   Purpose: USB Mass Storage Device driver                         */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                Copyright 2023, Sean Lawless                      */
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
#include <board.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <usb/device.h>
#include <usb/request.h>

#if ENABLE_USB_STORAGE

// USB Mass Storage block definitions
#define BLOCK_SIZE   512
#define BLOCK_MASK   (BLOCK_SIZE - 1)
#define BLOCK_SHIFT  9
#define MAX_OFFSET   0x1FFFFFFFFFF

// USB Mass Storage configuration state machine
#define STATE_INQUIRY         0
#define STATE_TEST_START      1
#define STATE_TEST_END        2
#define STATE_READ_CAPACITY   3
#define STATE_FINISHED        4

// USB Mass Storage command state machine
#define STATE_CBW_TRANSFER    0
#define STATE_DATA_TRANSFER   1
#define STATE_CSW_TRANSFER    2
#define STATE_CMD_FINISHED    3

// USB Mass Storage reset state machine
#define STATE_RESET_START     0
#define STATE_RESET_ENDPOINT1 1
#define STATE_RESET_ENDPOINT2 2
#define STATE_RESET_FINISHED  3

// USB MS Command Block Wrapper (CBW)
#define CBW_SIGNATURE      0x43425355
#define CBW_FLAGS_DATA_IN  0x80
#define CBW_LUN            0

// USB MS Command Status Wrapper (CSW)
#define CSW_SIGNATURE          0x53425355
#define CSW_STATUS_PASSED      0x00
#define CSW_STATUS_FAILED      0x01
#define CSW_STATUS_PHASE_ERROR 0x02

// SCSI
#define SCSI_CONTROL    0x00
#define SCSI_OP_INQUIRY   0x12
#define SCSI_PDT_DIRECT_ACCESS_BLOCK  0x00 // >= SBC-2 command set
#define SCSI_PDT_DIRECT_ACCESS_RBC  0x0E   // RBC command set
#define SCSI_REQUEST_SENSE    0x03
#define SCSI_OP_TEST_UNIT_READY   0x00
#define SCSI_OP_READ_CAPACITY10   0x25
#define SCSI_OP_READ    0x28
#define SCSI_OP_WRITE   0x2A
#define SCSI_WRITE_FUA    0x08

typedef struct MassStorageDevice
{
  Device device;
  Endpoint *endpointIn;
  Endpoint *endpointOut;
  Endpoint EndpointIn;
  Endpoint EndpointOut;
  Request *urb;
  int configurationState;
  int resetState;

  // Command state variables
  int commandState;
  int commandIn;
  void *commandBuffer;
  int commandBufferLen;
  void (*commandComplete)
       (void *urb, void *param, void *context);
  void (*operationCallback)(u8 *buffer, int buffLen, void *opPayload);
  void *operationPayload;

  // Device variables
  u32 tag;
  u32 blockCount;
  u64 offset;
}
MassStorageDevice;

// Command Block Wrapper (CBW)
typedef struct CommandBlockWrapper
{
  u32 signature, tag,
      dataTransferLength;   // number of bytes
  u8 flags, lun : 4, reserved1 : 4,
     CBLength  : 5, // length of control block in bytes
     reserved2 : 3;
  u8 CB[16];
}
__attribute__((packed)) CommandBlockWrapper;

// Command Status Wrapper (CSW)
typedef struct CommandStatusWrapper
{
  u32 signature,
      tag,
      dataResidue;    // difference in amount of data processed
  u8 status;
}
__attribute__((packed)) CommandStatusWrapper;

// SCSI Generic Command Set
//   All multi byte fields are big endian and need conversion

typedef struct SCSIInquiry
{
  u8 OperationCode,
     LogicalUnitNumberEVPD,
     PageCode,
     Reserved,
     AllocationLength,
     Control;
}
__attribute__((packed)) SCSIInquiry;

typedef struct SCSIInquiryResponse
{
  u8 PeripheralDeviceType  : 5,
      PeripheralQualifier : 3,    // 0: device is connected to this LUN
      DeviceTypeModifier : 7,
      RMB : 1, // 1: removable media
      ANSIApprovedVersion : 3,
      ECMAVersion : 3,
      ISOVersion : 2,
      Reserved1,
      AdditionalLength,
      Reserved2[3],
      VendorIdentification[8],
      ProductIdentification[16],
      ProductRevisionLevel[4];
}
__attribute__((packed)) SCSIInquiryResponse;

typedef struct SCSITestUnitReady
{
  u8 OperationCode;
  u32 Reserved;
  u8 Control;
}
__attribute__((packed)) SCSITestUnitReady;

typedef struct SCSIRequestSense
{
  u8 OperationCode;
  u8 DescriptorFormat  : 1,    // set to 0
      Reserved1   : 7;
  u16  Reserved2;
  u8 AllocationLength;
  u8 Control;
}
__attribute__((packed)) SCSIRequestSense;

typedef struct SCSIRequestSenseResponse7x
{
  u8 ResponseCode    : 7,
      Valid     : 1;
  u8 Obsolete;
  u8 SenseKey    : 4,
      Reserved    : 1,
      ILI     : 1,
      EOM     : 1,
      FileMark    : 1;
  u32 Information;
  u8 AdditionalSenseLength;
  u32 CommandSpecificInformation;
  u8 AdditionalSenseCode;
  u8 AdditionalSenseCodeQualifier;
  u8 FieldReplaceableUnitCode;
  u8 SenseKeySpecificHigh  : 7,
      SKSV      : 1;
  u16 SenseKeySpecificLow;
}
__attribute__((packed)) SCSIRequestSenseResponse7x;

typedef struct SCSIReadCapacity10
{
  u8 OperationCode;
  u8 Obsolete    : 1,
      Reserved1   : 7;
  u32  LogicalBlockAddress;
  u16  Reserved2;
  u8 PartialMediumIndicator  : 1,
      Reserved3   : 7;
  u8 Control;
}
__attribute__((packed)) SCSIReadCapacity10;

typedef struct SCSIReadCapacityResponse
{
  u32  ReturnedLogicalBlockAddress;
  u32  BlockLengthInBytes;
}
__attribute__((packed)) SCSIReadCapacityResponse;

typedef struct SCSIRead10
{
  u8 OperationCode,
      Reserved1;
  u32  LogicalBlockAddress;
  u8 Reserved2;
  u16  TransferLength; // block count
  u8 Control;
}
__attribute__((packed)) SCSIRead10;

typedef struct SCSIWrite10
{
  u8 OperationCode, Flags;
  u32 LogicalBlockAddress;
  u8 Reserved;
  u16 TransferLength; // block count
  u8 Control;
}
__attribute__((packed)) SCSIWrite10;

MassStorageDevice MassStorage;
MassStorageDevice *MS1;

// little endian to big endian integer converter functions
inline static u16 le2be16(u16 value)
{
  return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
}

inline static u32 le2be32(u32 value)
{
  return ((value & 0x000000FF) << 24) | ((value & 0x0000FF00) << 8) |
         ((value & 0x00FF0000) >> 8) | ((value & 0xFF000000) >> 24);
}

static void command_complete(void *urb, void *param, void *context)
{
  Endpoint *endpoint = param;
  MassStorageDevice *massStorage = (MassStorageDevice *)endpoint->device;
  static CommandStatusWrapper CSW;

  // Clear last request before sending a new one
  if (urb)
    FreeRequest(urb);

//  printf("  cmd complete state %d\n",
//         massStorage->commandState);


  if (massStorage->commandState == STATE_DATA_TRANSFER)
  {
    int nResult = 0;

    // Skip data transfer if zero bytes
    if (massStorage->commandBufferLen <= 0)
    {
      massStorage->commandState++;
      goto csw;
    }

    nResult = HostEndpointTransfer(massStorage->device.host,
               massStorage->commandIn ? massStorage->endpointIn : massStorage->endpointOut,
               massStorage->commandBuffer, massStorage->commandBufferLen, command_complete);
    if (nResult < 0)
    {
      puts("Data transfer failed");
      massStorage->commandState = 0;
      return;
    }
  }
  else if (massStorage->commandState == STATE_CSW_TRANSFER)
  {
csw:
    if (HostEndpointTransfer(massStorage->device.host,
         massStorage->endpointIn, &CSW, sizeof(CSW), command_complete))
    {
      puts("CSW transfer failed");
      massStorage->commandState = 0;
      return;
    }
  }
  else if (massStorage->commandState == STATE_CMD_FINISHED)
  {
    // Verify the resulting CSW data from previous state
    if (CSW.signature != CSW_SIGNATURE)
    {
      puts("invalid CSW signature");
      massStorage->commandState = 0;
      return;
    }

    if (CSW.tag != massStorage->tag)
    {
      puts("CSW tag incorrect");
      massStorage->commandState = 0;
      return;
    }

    // Ignore CSW data residue for now, pass to command complete
/*
    if (CSW.dataResidue != 0)
    {
      puts("Data residue > 0");
      massStorage->commandState = 0;
      return;
    }
*/
    // Ignore CSW status, but pass resulting CSW to command complete

    // Reset the command state and invoke the command complete callback
    massStorage->commandState = 0;
    massStorage->commandComplete(NULL, massStorage, &CSW);
    return;
  }

  ++massStorage->commandState;
  return;
}

static int command(MassStorageDevice *massStorage, void *cmd,
                   u32 cmdLen, void *buffer, u32 bufLen,
                   int isInEndpoint, void (complete)(void *urb,
                                            void *param, void *context))
{
  assert(massStorage != 0);

  if (massStorage->commandState == STATE_CBW_TRANSFER)
  {
    assert(cmd != 0);
    assert(6 <= cmdLen && cmdLen <= 16);
    assert(bufLen == 0 || buffer != 0);

    CommandBlockWrapper CBW;
    memset(&CBW, 0, sizeof(CBW));

    CBW.signature = CBW_SIGNATURE;
    CBW.tag = ++massStorage->tag;
    CBW.dataTransferLength = bufLen;
    CBW.flags = isInEndpoint ? CBW_FLAGS_DATA_IN : 0;
    CBW.lun = CBW_LUN;
    CBW.CBLength = (u8)cmdLen;

    memcpy(CBW.CB, cmd, cmdLen);

    assert(massStorage->device.host != 0);

    // Save the command state: end callback, isIn and buffer
    massStorage->commandComplete = complete;
    massStorage->commandIn = isInEndpoint;
    massStorage->commandBuffer = buffer;
    massStorage->commandBufferLen = bufLen;

    // Start the asynchronous endpoint transfer
    if (HostEndpointTransfer(massStorage->device.host, massStorage->endpointOut,
                             &CBW, sizeof(CBW), command_complete) < 0)
    {
      puts("MS CBW transfer failed to start");
      massStorage->commandState = 0;
      return -1;
    }
    ++massStorage->commandState;
  }
  else
  {
    puts("USB mass storage command already in progress");
    return -2;
  }
  return 0;
}

/*...................................................................*/
/* configure_complete: State machine to configure a keyboard         */
/*                                                                   */
/*      Inputs: urb is USB Request Buffer (URB)                      */
/*              param is the keyboard device                         */
/*              context is unused                                    */
/*...................................................................*/
static void configure_complete(void *urb, void *param, void *context)
{
  static SCSIInquiryResponse InquiryResponse;
  static SCSIInquiry Inquiry;
  static SCSITestUnitReady TestUnitReady;
  static SCSIReadCapacity10 ReadCapacity;
  static SCSIReadCapacityResponse ReadCapacityResponse;
  static SCSIRequestSense RequestSense;
  static SCSIRequestSenseResponse7x RequestSenseResponse7x;
  MassStorageDevice *massStorage = (MassStorageDevice *)param;
  CommandStatusWrapper *csw = context;

  if (csw->status != CSW_STATUS_PASSED)
  {
    --massStorage->configurationState;
    printf("  csw status 'not passed', retrying state %d.\n",
           massStorage->configurationState);
  }

  // Clear last request before sending a new one
  if (urb)
    FreeRequest(urb);
  massStorage->commandComplete = NULL;

//  printf("MS configure complete state %d\n",
//         massStorage->configurationState);

  if (massStorage->configurationState == STATE_INQUIRY)
  {
    Inquiry.OperationCode   = SCSI_OP_INQUIRY;
    Inquiry.LogicalUnitNumberEVPD = 0;
    Inquiry.PageCode      = 0;
    Inquiry.Reserved      = 0;
    Inquiry.AllocationLength    = sizeof (SCSIInquiryResponse);
    Inquiry.Control     = SCSI_CONTROL;

    massStorage->commandComplete = configure_complete;
    if (command(massStorage, &Inquiry, sizeof(Inquiry),
                &InquiryResponse, sizeof(InquiryResponse),
                TRUE, configure_complete) < 0)
    {
      puts("Device does not respond");
      return;
    }

  }
  else if (massStorage->configurationState == STATE_TEST_START)
  {

    if (InquiryResponse.PeripheralDeviceType != SCSI_PDT_DIRECT_ACCESS_BLOCK)
    {
      printf("Unsupported device type: 0x%02X\n", (unsigned) InquiryResponse.PeripheralDeviceType);
      return;
    }

    TestUnitReady.OperationCode = SCSI_OP_TEST_UNIT_READY;
    TestUnitReady.Reserved  = 0;
    TestUnitReady.Control = SCSI_CONTROL;

    if (command(massStorage, &TestUnitReady,
            sizeof(TestUnitReady), 0, 0, FALSE, configure_complete) < 0)
    {
      puts("Test unit failed");
      return;
    }
  }
  else if (massStorage->configurationState == STATE_TEST_END)
  {
    RequestSense.OperationCode = SCSI_REQUEST_SENSE;
    RequestSense.DescriptorFormat = 0;
    RequestSense.Reserved1 = 0;
    RequestSense.Reserved2 = 0;
    RequestSense.AllocationLength = sizeof(SCSIRequestSenseResponse7x);
    RequestSense.Control = SCSI_CONTROL;

    if (command(massStorage, &RequestSense, sizeof(RequestSense),
                &RequestSenseResponse7x, sizeof(RequestSenseResponse7x),
                TRUE, configure_complete) < 0)
    {
      puts("Request sense failed");
      return;
    }
  }
  else if (massStorage->configurationState == STATE_READ_CAPACITY)
  {
    usleep(100 * MICROS_PER_MILLISECOND);

    ReadCapacity.OperationCode    = SCSI_OP_READ_CAPACITY10;
    ReadCapacity.Obsolete   = 0;
    ReadCapacity.Reserved1    = 0;
    ReadCapacity.LogicalBlockAddress  = 0;
    ReadCapacity.Reserved2    = 0;
    ReadCapacity.PartialMediumIndicator = 0;
    ReadCapacity.Reserved3    = 0;
    ReadCapacity.Control    = SCSI_CONTROL;

    if (command(massStorage, &ReadCapacity, sizeof(ReadCapacity),
                &ReadCapacityResponse, sizeof(ReadCapacityResponse),
                TRUE, configure_complete) < 0)
    {
      puts("Read capacity failed");
      return;
    }
  }
  else if (massStorage->configurationState == STATE_FINISHED)
  {
    u32 blockSize = le2be32(ReadCapacityResponse.BlockLengthInBytes);
    if (blockSize != BLOCK_SIZE)
    {
      printf("Unsupported block size: %u\n", blockSize);
      return;
    }

    massStorage->blockCount = le2be32(ReadCapacityResponse.ReturnedLogicalBlockAddress);
    if (massStorage->blockCount == (u32) -1)
    {
      puts("Unsupported disk size > 2TB");
      return;
    }

    massStorage->blockCount++;

    printf("Capacity is %u MByte\n", (massStorage->blockCount * BLOCK_SIZE) / 0x100000);

    // If registered complete function assigned by hub, call it
    if (massStorage->device.complete)
    {
      massStorage->device.complete(NULL, massStorage->device.comp_dev, NULL);
      massStorage->device.complete = NULL;
    }

    MS1 = &MassStorage;
    return;
  }

  // Advance to the next configuration state
  ++massStorage->configurationState;
  return;
}

static int configure(Device *device, void (complete)
              (void *urb, void *param, void *context), void *param)
{
  MassStorageDevice *massStorage = (MassStorageDevice *)device;
  InterfaceDescriptor *interfaceDesc;
  int count = 0;
  assert (massStorage != 0);

  ConfigurationDescriptor *confDesc =
    (ConfigurationDescriptor *)DeviceGetDescriptor(massStorage->device.configParser, DESCRIPTOR_CONFIGURATION);
  if ((confDesc == 0) || (confDesc->numInterfaces <  1))
  {
    puts("USB MS configuration descriptor error");
    return FALSE;
  }

  // Read and parse all interface descriptors
  while ((interfaceDesc =
         (InterfaceDescriptor *)DeviceGetDescriptor(
             massStorage->device.configParser, DESCRIPTOR_INTERFACE)) != 0)
  {
    ++count;
    if ((interfaceDesc == 0)|| (interfaceDesc->interfaceNumber != 0) ||
        (interfaceDesc->alternateSetting != 0) ||
        (interfaceDesc->numEndpoints < 2) ||
        // Mass Storage Class
        (interfaceDesc->interfaceClass != 8) ||
        // SCSI Transparent Command Set
        (interfaceDesc->interfaceSubClass != 6) ||
        // Bulk-Only Transport
        (interfaceDesc->interfaceProtocol != 0x50)
       )
    {
      printf("Incompatible interface 1 (%d) (%x:%x:%x:%x:%x:%x) for USB MS\n",
             confDesc->numInterfaces, interfaceDesc->interfaceNumber, interfaceDesc->alternateSetting,
             interfaceDesc->numEndpoints, interfaceDesc->interfaceClass,
             interfaceDesc->interfaceSubClass, interfaceDesc->interfaceProtocol);
    }
    else
      break;
  }

  if (!interfaceDesc)
  {
    puts("USB MS bulk interface not found");
    return FALSE;
  }

  const EndpointDescriptor *endpointDesc;

  // Loop on all endpoint descriptors
  while ((endpointDesc = (EndpointDescriptor *)
      DeviceGetDescriptor(massStorage->device.configParser,
                                          DESCRIPTOR_ENDPOINT)) != 0)
  {
    // If endpoint is bulk (0x02), try to use it
    if ((endpointDesc->attributes & 0x3F) == 0x02)
    {
      // If input (0x80) (and bulk) use it
      if ((endpointDesc->endpointAddress & 0x80) == 0x80)
      {
        // Return failure if in endpoint already assigned
        if (massStorage->endpointIn != 0)
        {
          puts("Input endpoint for USB MS not zero, incompatible");
          return FALSE;
        }

        // Configure in endpoint to this endpoint descriptor
        massStorage->endpointIn = (Endpoint *)&massStorage->EndpointIn;
        EndpointFromDescr(massStorage->endpointIn,
                          &massStorage->device, endpointDesc);
      }

      // Otherwise output (and bulk) so use it
      else
      {
        // Return failure if out endpoint already assigned
        if (massStorage->endpointOut != 0)
        {
          puts("Output endpoint for USB MS not zero, incompatible");
          return FALSE;
        }

        // Configure out endpoint to this endpoint descriptor
        massStorage->endpointOut =(Endpoint *)&massStorage->EndpointOut;
        EndpointFromDescr(massStorage->endpointOut,
                          &massStorage->device, endpointDesc);
      }
    }
  }

  // If in or out endpoint not assigned, return error
  if ((massStorage->endpointIn  == 0) ||
      (massStorage->endpointOut == 0))
  {
    puts("Input and/or endpoint for USB MS not set");
    return FALSE;
  }

  // Otherwise begin device configuration sequence (asynchronous)
  if (!DeviceConfigure(&massStorage->device, configure_complete,
                       massStorage))
  {
    puts("Cannot set configuration");
    return FALSE;
  }

  // Return mass storage configuration started successfully
  return TRUE;
}


static void reset_complete(void *urb, void *param, void *context)
{
  MassStorageDevice *massStorage = (MassStorageDevice *)param;

  if (massStorage->resetState == STATE_RESET_ENDPOINT1)
  {
    if (HostEndpointControlMessage(massStorage->device.host,
          massStorage->device.endpoint0, 0x02, 1, 0, 1, 0, 0,
          reset_complete, massStorage) < 0)
    {
      puts("Cannot clear halt on endpoint 1");
      return;
    }
  }
  else if (massStorage->resetState == STATE_RESET_ENDPOINT2)
  {
    if (HostEndpointControlMessage(massStorage->device.host,
          massStorage->device.endpoint0, 0x02, 1, 0, 2, 0, 0,
          reset_complete, massStorage) < 0)
    {
      puts("Cannot clear halt on endpoint 2");
      return;
    }
  }
  else if (massStorage->resetState == STATE_RESET_FINISHED)
  {
    EndpointResetPID(massStorage->endpointIn);
    EndpointResetPID(massStorage->endpointOut);
    return;
  }

  ++massStorage->resetState;
  return;
}

static int reset(MassStorageDevice *massStorage)
{
  assert(massStorage != 0);
  assert(massStorage->device.host != 0);

  puts("-Reset MS device");

  if (HostEndpointControlMessage(massStorage->device.host,
        massStorage->device.endpoint0, 0x21, 0xFF, 0, 0x00, 0, 0,
        reset_complete, massStorage) < 0)
  {
    puts("Cannot reset device");
    return -1;
  }

  massStorage->resetState = STATE_RESET_ENDPOINT1;
  return 0;
}

/*...................................................................*/
/* op_complete: User level operation callback for mass storage       */
/*                                                                   */
/*      Inputs: urb is USB Request Buffer (URB)                      */
/*              param is the keyboard device                         */
/*              context is resulting Command Status Wrapper (CSW)    */
/*...................................................................*/
static void op_complete(void *urb, void *param, void *context)
{
  MassStorageDevice *massStorage = (MassStorageDevice *)param;
  CommandStatusWrapper *csw = context;

  // Clear last request before sending a new one
  if (urb)
    FreeRequest(urb);
  massStorage->commandComplete = NULL;

  if (csw->status != CSW_STATUS_PASSED)
  {
    printf("  csw status 'not passed', read failed %d.\n",
           csw->status);
    massStorage->commandBufferLen = -csw->status; // return failure
  }

  // If there is data left over, reduce the amount
  if ((csw->dataResidue > 0) &&
      (massStorage->commandBufferLen >= csw->dataResidue))
    massStorage->commandBufferLen -= csw->dataResidue;

  // Invoke the user callback if configured
  if (massStorage->operationCallback)
    massStorage->operationCallback(massStorage->commandBuffer,
                                   massStorage->commandBufferLen,
                                   massStorage->operationPayload);

  // Clear the buffer for next user command
  massStorage->commandBuffer = NULL;
  massStorage->commandBufferLen = 0;

  // Increment the offset to the next block for the next read
  massStorage->offset += BLOCK_SIZE;
}

static int read(MassStorageDevice *device, void *buffer, unsigned count)
{
  u32 blockAddress = (u32)(device->offset >> BLOCK_SHIFT);
  u16 transferLength = (u16) (count >> BLOCK_SHIFT);
  assert(device != 0);
  assert (buffer != 0);

  if (((device->offset & BLOCK_MASK) != 0))// ||
//      (device->offset > MAX_OFFSET))
  {
    printf("Offset %d invalid (... & %x, > %x)", device->offset,
           BLOCK_MASK, MAX_OFFSET);
    return -1;
  }

  if ((count & BLOCK_MASK) != 0)
  {
    printf("Count %d invalid (... & %x)", count, BLOCK_MASK);
    return -1;
  }

//  printf("read %u/0x%X/%u", blockAddress, (uintptr_t)buffer,
//         (u32)transferLength);

  SCSIRead10 SCSIRead;
  SCSIRead.OperationCode = SCSI_OP_READ;
  SCSIRead.Reserved1    = 0;
  SCSIRead.LogicalBlockAddress = le2be32(blockAddress);
  SCSIRead.Reserved2 = 0;
  SCSIRead.TransferLength = le2be16(transferLength);
  SCSIRead.Control = SCSI_CONTROL;

  if (command(device, &SCSIRead, sizeof SCSIRead, buffer, count, TRUE,
              op_complete) < 0)
  {
    puts("MS read failed");
    return -1;
  }

  return count;
}

/*...................................................................*/
/* write_complete: State machine to write from mass storage          */
/*                                                                   */
/*      Inputs: urb is USB Request Buffer (URB)                      */
/*              param is the keyboard device                         */
/*              context is unused                                    */
/*...................................................................*/
/*static void write_complete(void *urb, void *param, void *context)
{
  MassStorageDevice *massStorage = (MassStorageDevice *)param;
  CommandStatusWrapper *csw = context;

  // Clear last request before sending a new one
  if (urb)
    FreeRequest(urb);
  massStorage->commandComplete = NULL;

  if (csw->status != CSW_STATUS_PASSED)
  {
    printf("  csw status 'not passed', write failed %d.\n",
           csw->status);
    massStorage->commandBufferLen = -csw->status; // return failure
  }

  // If there is data left over, reduce the amount written
  if ((csw->dataResidue > 0) &&
      (massStorage->commandBufferLen >= csw->dataResidue))
    massStorage->commandBufferLen -= csw->dataResidue;

  // Invoke the user callback if configured
  if (massStorage->operationCallback)
    massStorage->operationCallback(massStorage->commandBuffer,
                                   massStorage->commandBufferLen);

  // Clear the buffer for next user command
  massStorage->commandBuffer = NULL;
  massStorage->commandBufferLen = 0;

  // Increment the offset to the next block for the next read
  massStorage->offset += BLOCK_SIZE;
}
*/

static int write(MassStorageDevice *device, const void *buffer,
                 u32 count)
{
  assert(device != 0);
  assert(buffer != 0);

  if (((device->offset & BLOCK_MASK) != 0))// ||
//      (device->offset > MAX_OFFSET))
  {
    printf("Offset %d invalid (... & %x, > %x)", device->offset,
           BLOCK_MASK, MAX_OFFSET);
    return -1;
  }
  u32 blockAddress = (u32)(device->offset >> BLOCK_SHIFT);

  if ((count & BLOCK_MASK) != 0)
  {
    printf("Count %d invalid (... & %x)", count, BLOCK_MASK);
    return -1;
  }
  u16 transferLength = (u16)(count >> BLOCK_SHIFT);

//  printf("write %u/0x%X/%u", blockAddress, (uintptr_t)buffer,
//         (u32)transferLength);

  SCSIWrite10 SCSIWrite;
  SCSIWrite.OperationCode = SCSI_OP_WRITE;
  SCSIWrite.Flags = SCSI_WRITE_FUA;
  SCSIWrite.LogicalBlockAddress = le2be32 (blockAddress);
  SCSIWrite.Reserved = 0;
  SCSIWrite.TransferLength = le2be16(transferLength);
  SCSIWrite.Control = SCSI_CONTROL;

  if (command(device, &SCSIWrite, sizeof SCSIWrite, (void *)buffer,
              count, FALSE, op_complete) < 0)
  {
    puts("Write command failed to initiate");
    return -1;
  }

  return count;
}

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

void *MassStorageAttach(Device *device)
{
  MassStorageDevice *massStorage = &MassStorage;

  DeviceCopy(&massStorage->device, device);
  massStorage->device.configure = configure;

  massStorage->configurationState = 0;
  massStorage->commandState = 0;

  massStorage->endpointIn = 0;
  massStorage->endpointOut = 0;
  massStorage->tag = 0;
  massStorage->blockCount = 0;
  massStorage->offset = 0;
  return massStorage;
}

void MassStorageRelease(MassStorageDevice *device)
{
  assert(device != 0);

  if (device->endpointOut != 0)
  {
    EndpointRelease(device->endpointOut);
    free(device->endpointOut);
    device->endpointOut =  0;
  }

  if (device->endpointIn != 0)
  {
    EndpointRelease(device->endpointIn);
    free(device->endpointIn);
    device->endpointIn =  0;
  }

  DeviceRelease(&device->device);
}

int MassStorageRead(void *buffer, u32 count,
                    void (callback)(u8 *buffer, int buffLen,
                                    void *payload),
                    void *payload)
{
  MassStorageDevice *device = MS1;
  assert(device != 0);

  device->operationCallback = callback;
  device->operationPayload = payload;
  return read(device, buffer, count);
}

int MassStorageWrite(const void *buffer, u32 count,
                           void (callback)(u8 *buffer, int buffLen,
                                           void *payload),
                           void *payload)
{
  MassStorageDevice *device = MS1;
  assert(device != 0);

  device->operationCallback = callback;
  device->operationPayload = payload;
  return write(device, buffer, count);
}

int MassStorageReset()
{
  MassStorageDevice *device = MS1;
  assert(device != 0);

  return reset(device);
}

u64 MassStorageSeek(u64 offset)
{
  MassStorageDevice *device = MS1;

  assert(device != 0);

  if (((offset & BLOCK_MASK) != 0))// || (offset > MAX_OFFSET))
  {
    printf("Offset %x invalid (... & %x, > %x)", offset,
           BLOCK_MASK, MAX_OFFSET);
    return 0;
  }

  device->offset = offset;
  return device->offset;
}

u64 MassStorageOffset()
{
  MassStorageDevice *device = MS1;

  assert(device != 0);

  return device->offset;
}

u32 MassStorageBlockCapacity()
{
  MassStorageDevice *device = MS1;

  assert(device != 0);

  return device->blockCount;
}

u32 MassStorageBlockSize()
{
  return BLOCK_SIZE;
}
#endif /* ENABLE_USE_STORAGE */
