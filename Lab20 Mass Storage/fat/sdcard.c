#include <system.h>
#include "./sdcard.h"

#if ENABLE_FAT

// Must match masstorage.c BLOCK_SIZE and asysncfatfs.c
// AFATFS_SECTOR_SIZE

#define BLOCK_SIZE  512

/*
typedef struct sdcard_metadata_t {
    uint8_t manufacturerID;
    uint16_t oemID;

    char productName[5];

    uint8_t productRevisionMajor;
    uint8_t productRevisionMinor;
    uint32_t productSerial;

    uint16_t productionYear;
    uint8_t productionMonth;

    uint32_t numBlocks; // Card capacity in 512-byte blocks
} sdcardMetadata_t;

typedef enum {
    SDCARD_BLOCK_OPERATION_READ,
    SDCARD_BLOCK_OPERATION_WRITE,
    SDCARD_BLOCK_OPERATION_ERASE,
} sdcardBlockOperation_e;

typedef enum {
    SDCARD_OPERATION_IN_PROGRESS,
    SDCARD_OPERATION_BUSY,
    SDCARD_OPERATION_SUCCESS,
    SDCARD_OPERATION_FAILURE
} sdcardOperationStatus_e;

typedef void(*sdcard_operationCompleteCallback_c)(sdcardBlockOperation_e operation, uint32_t blockIndex, uint8_t *buffer, uint32_t callbackData);

typedef void(*sdcard_profilerCallback_c)(sdcardBlockOperation_e operation, uint32_t blockIndex, uint32_t duration);
*/

/*
int MassStorageRead(void *buffer, u32 count,
                    void (callback)(u8 *buffer, int buffLen),
                    void *callback_payload);
*/
#define MAX_OPERATIONS 1

typedef struct OpCallback
{
  sdcard_operationCompleteCallback_c callback;
  uint32_t callbackData;
  uint32_t blockIndex;
} OpCallback;

OpCallback Operations[MAX_OPERATIONS];

OpCallback *NewOp()
{
  for (int i = 0; i < MAX_OPERATIONS; ++i)
    if (Operations[i].callback == NULL)
      return &Operations[i];
  return NULL;
}

void ms_readBlockCallback(u8 *buffer, int buffLen, void *opPayload)
{
  OpCallback *op = opPayload;

  op->callback(SDCARD_BLOCK_OPERATION_READ, op->blockIndex, buffer,
               op->callbackData);

  op->callback = NULL;
  op->callbackData = 0;
  op->blockIndex = 0;
}

void ms_writeBlockCallback(u8 *buffer, int buffLen, void *opPayload)
{
  OpCallback *op = opPayload;

  op->callback(SDCARD_BLOCK_OPERATION_WRITE, op->blockIndex, buffer,
               op->callbackData);

  op->callback = NULL;
  op->callbackData = 0;
  op->blockIndex = 0;
}

/**
 * Read the 512-byte block with the given index into the given 512-byte buffer.
 *
 * When the read completes, your callback will be called. If the read was successful, the buffer pointer will be the
 * same buffer you originally passed in, otherwise the buffer will be set to NULL.
 *
 * You must keep the pointer to the buffer valid until the operation completes!
 *
 * Returns:
 *     true - The operation was successfully queued for later completion, your callback will be called later
 *     false - The operation could not be started due to the card being busy (try again later).
 */
bool sdcard_readBlock(uint32_t blockIndex, uint8_t *buffer, sdcard_operationCompleteCallback_c callback, uint32_t callbackData)
{
  OpCallback *op = NewOp();

  if (op == NULL)
    return false;

  op->callback = callback;
  op->callbackData = callbackData;
  op->blockIndex = blockIndex;

  // Seek to the block
  if (MassStorageSeek(blockIndex * BLOCK_SIZE) != blockIndex * BLOCK_SIZE)
    return false;

  // Start the asynchronous read
  return (MassStorageRead(buffer, BLOCK_SIZE, ms_readBlockCallback,
                          (void *)op) < 0) ? false : true;
}


/**
 * Write the 512-byte block from the given buffer into the block with the given index.
 *
 * If the write does not complete immediately, your callback will be called later. If the write was successful, the
 * buffer pointer will be the same buffer you originally passed in, otherwise the buffer will be set to NULL.
 *
 * Returns:
 *     SDCARD_OPERATION_IN_PROGRESS - Your buffer is currently being transmitted to the card and your callback will be
 *                                    called later to report the completion. The buffer pointer must remain valid until
 *                                    that time.
 *     SDCARD_OPERATION_SUCCESS     - Your buffer has been transmitted to the card now.
 *     SDCARD_OPERATION_BUSY        - The card is already busy and cannot accept your write
 *     SDCARD_OPERATION_FAILURE     - Your write was rejected by the card, card will be reset
 */
sdcardOperationStatus_e sdcard_writeBlock(uint32_t blockIndex, uint8_t *buffer, sdcard_operationCompleteCallback_c callback, uint32_t callbackData)
{
  OpCallback *op = NewOp();

  if (op == NULL)
    return false;

  op->callback = callback;
  op->callbackData = callbackData;
  op->blockIndex = blockIndex;

  // Seek to the block
  if (MassStorageSeek(blockIndex * BLOCK_SIZE) != blockIndex * BLOCK_SIZE)
    return false;

  // Start the asynchronous write
  return (MassStorageWrite(buffer, BLOCK_SIZE, ms_writeBlockCallback,
                           (void *)op) < 0) ? false : true;
}

/**
 * Will be called by afatfs_poll() periodically for the SD card to perform in-progress transfers.
 *
 * Returns true if the card is ready to accept new commands.
 */
bool sdcard_poll()
{
  // If another operation available, return true
  for (int i = 0; i < MAX_OPERATIONS; ++i)
    if (Operations[i].callback == NULL)
      return true;
  return false;
}

/**
 * Begin writing a series of consecutive blocks beginning at the given block index. This will allow (but not require)
 * the SD card to pre-erase the number of blocks you specifiy, which can allow the writes to complete faster.
 *
 * Afterwards, just call sdcard_writeBlock() as normal to write those blocks consecutively.
 *
 * The multi-block write will be aborted automatically when writing to a non-consecutive address, or by performing a
 * read. You can abort it manually by calling sdcard_endWriteBlocks().
 *
 * If you don't want to implement this feature, remove the define AFATFS_MIN_MULTIPLE_BLOCK_WRITE_COUNT in asyncfatfs.c.
 *
 * Returns:
 *     SDCARD_OPERATION_SUCCESS     - Multi-block write has been queued
 *     SDCARD_OPERATION_BUSY        - The card is already busy and cannot accept your write
 *     SDCARD_OPERATION_FAILURE     - A fatal error occured, card will be reset
 */
//sdcardOperationStatus_e sdcard_beginWriteBlocks(uint32_t blockIndex, uint32_t blockCount);

/**
 * Abort a multiple-block write early (before all the `blockCount` blocks had been written).
 *
 * If you don't want to implement this feature, remove the define AFATFS_MIN_MULTIPLE_BLOCK_WRITE_COUNT in asyncfatfs.c.
 *
 * Returns:
 *     SDCARD_OPERATION_SUCCESS     - Multi-block write has been cancelled, or no multi-block write was in progress.
 *     SDCARD_OPERATION_BUSY        - The card is busy with another operation and could not cancel the multi-block write.
 */
//sdcardOperationStatus_e sdcard_endWriteBlocks();

/**
 * Only required to be provided when using AFATFS_USE_INTROSPECTIVE_LOGGING.
 */
//void sdcard_setProfilerCallback(sdcard_profilerCallback_c callback);

/*..................................................................*/
/* FatPoll: poll the FAT file system                                */
/*                                                                  */
/* returns: exit error                                              */
/*..................................................................*/
int FatPoll(void *unused)
{
  afatfs_poll();
  return TASK_FINISHED;
}

void FatInit()
{
  afatfs_init();
}

#endif /* ENABLE_FAT */
