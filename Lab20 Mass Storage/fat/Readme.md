## AsyncFatFS

When paired with a simple asynchronous read-block/write-block API for an SD card, this library provides an 
asynchronous FAT16/FAT32 filesystem for embedded devices. Filesystem operations do not wait for the SD card
to become ready, but instead either provide a callback to notify you of operation completion, or return a
status code that indicates that the operation should be retried later.

A special optional feature of this filesystem is a high-speed contiguous append file mode, provided by the "freefile"
support. In this mode, the largest contiguous free block on the volume is pre-allocated during filesystem 
initialisation into a file called "freespac.e". One file can be created which slowly grows from the beginning 
of this contiguous region, stealing the first part of the freefile's space. Because the freefile is contiguous, 
the FAT entries for the file need never be read. This saves on buffer space and reduces latency during file
extend operations.

### Implementing AsyncFatFS

Compile and link your app against `lib/asyncfatfs.c` and `lib/fat_standard.c`.

If you will not be using the "freefile" feature, remove the define "AFATFS_USE_FREEFILE" in `asyncfatfs.c`.

The header `lib/sdcard.h` describes the functions that you must provide in your app to provide the necessary SD card 
read/write primitives, the minimum are:

```cpp
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
bool sdcard_readBlock(uint32_t blockIndex, uint8_t *buffer, sdcard_operationCompleteCallback_c callback, uint32_t callbackData);

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
sdcardOperationStatus_e sdcard_writeBlock(uint32_t blockIndex, uint8_t *buffer, sdcard_operationCompleteCallback_c callback, uint32_t callbackData);

/**
 * Will be called by afatfs_poll() periodically for the SD card to perform in-progress transfers.
 *
 * Returns true if the card is ready to accept new commands.
 */
bool sdcard_poll();
```

The header `asyncfatfs.h` lists the filesystem functions that AsyncFatFS provides for you. The documentation for these 
is inline in `asyncfatfs.c`.

To start up the filesystem, first call `afatfs_init()`.

You must periodically call `afatfs_poll()` so that the filesystem can perform any queued work. This will in turn call 
`sdcard_poll()` for you, which you can use to perform any pending transactions with the SD card.

You can use the testcases in the `test/` directory as a guide to how to use the filesystem. AsyncFatFS is also used in
Cleanflight / Betaflight's "blackbox" logging system: [filesystem consumer code](https://github.com/betaflight/betaflight/blob/master/src/main/blackbox/blackbox_io.c), 
[SDCard SPI driver code](https://github.com/betaflight/betaflight/blob/master/src/main/drivers/sdcard_spi.c).

You'll notice that since most filesystem operations will fail and ask you to retry when the card is busy, it becomes 
natural to call it using a state-machine from your app's main loop - where you only advance to the next state once the 
current operation succeeds, calling afatfs_poll() in-between so that the filesystem can complete its queued tasks.
