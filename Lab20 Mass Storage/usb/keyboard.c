/*...................................................................*/
/*                                                                   */
/*   Module:  keyboard.c                                             */
/*   Version: 2020.0                                                 */
/*   Purpose: USB standard keyboard device driver                    */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2020, Sean Lawless                    */
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
/*                                                                   */
/* The FreeBSD Project, Copyright 1992-2020                          */
/*   Copyright 1992-2020 The FreeBSD Project.                        */
/*   Permission granted under MIT license (Public Domain).           */
/*                                                                   */
/* Embedded Xinu, Copyright (C) 2013.                                */
/*   Permission granted under Creative Commons Zero (Public Domain). */
/*                                                                   */
/* Alex Chadwick (CSUD), Copyright (C) 2012.                         */
/*   Permission granted under MIT license (Public Domain).           */
/*                                                                   */
/* Thanks to Linux/Circle/uspi, and Rene Stange specifically, for    */
/* the quality reference model and runtime register debug output.    */
/*                                                                   */
/* Additional thanks to the Raspberry Pi bare metal forum community. */
/* https://www.raspberrypi.org/forums/viewforum.php?f=72             */
/*...................................................................*/
#include <board.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <usb/hid.h>
#include <usb/request.h>
#include <usb/device.h>

#if ENABLE_USB_HID

/*...................................................................*/
/* Symbole Definitions                                               */
/*...................................................................*/
#define REPORT_SIZE          8
#define LEFT_SHIFT_MODIFIER  2
#define RIGHT_SHIFT_MODIFIER 0x20
#define DEFAULT_MODIFIER     1


/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
typedef struct KeyboardDevice
{
  Device device; // Must be first element in this structure
  Endpoint *reportEndpoint;
  Endpoint *interruptEndpoint;
  Endpoint ReportEndpoint;
  Endpoint InterruptEndpoint;

  void (*key_pressed)(const char ascii);

  Request *urb;
  u8 reportBuffer[REPORT_SIZE];

  u8 interfaceNumber;
  u8 alternateSetting;

  u8 lastPhyCode;
  u32 tmr;

  int capsLock;
}
KeyboardDevice;

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
char KeyMap[][2] =
{
  {  0,  0 }, // 0x00
  {  0,  0 }, // 0x01
  {  0,  0 }, // 0x02
  {  0,  0 }, // 0x03
  {'a', 'A'}, // 0x04
  {'b', 'B'}, // 0x05
  {'c', 'C'}, // 0x06
  {'d', 'D'}, // 0x07
  {'e', 'E'}, // 0x08
  {'f', 'F'}, // 0x09
  {'g', 'G'}, // 0x0A
  {'h', 'H'}, // 0x0B
  {'i', 'I'}, // 0x0C
  {'j', 'J'}, // 0x0D
  {'k', 'K'}, // 0x0E
  {'l', 'L'}, // 0x0F
  {'m', 'M'}, // 0x10
  {'n', 'N'}, // 0x11
  {'o', 'O'}, // 0x12
  {'p', 'P'}, // 0x13
  {'q', 'Q'}, // 0x14
  {'r', 'R'}, // 0x15
  {'s', 'S'}, // 0x16
  {'t', 'T'}, // 0x17
  {'u', 'U'}, // 0x18
  {'v', 'V'}, // 0x19
  {'w', 'W'}, // 0x1A
  {'x', 'X'}, // 0x1B
  {'y', 'Y'}, // 0x1C
  {'z', 'Z'}, // 0x1D
  {'1', '!'}, // 0x1E
  {'2', '@'}, // 0x1F
  {'3', '#'}, // 0x20
  {'4', '$'}, // 0x21
  {'5', '%'}, // 0x22
  {'6', '^'}, // 0x23
  {'7', '&'}, // 0x24
  {'8', '*'}, // 0x25
  {'9', '('}, // 0x26
  {'0', ')'}, // 0x27
  {'\n','\n'},// 0x28
  {'\e','\e'},// 0x29
  {'\b','\b'},// 0x2A
  {'\t','\t'},// 0x2B
  {' ', ' '}, // 0x2C
  {'-', '_'}, // 0x2D
  {'=', '+'}, // 0x2E
  {'[', '{'}, // 0x2F
  {']', '}'}, // 0x30
  {'\\','|'}, // 0x31
  {'#', '~'}, // 0x32
  {';', ':'}, // 0x33
  {'\'','\"'},// 0x34
  {'`', '~'}, // 0x35
  {',', '<'}, // 0x36
  {'.', '>'}, // 0x37
  {'/', '?'}, // 0x38
  {'\1','\1'}, // 0x39 Caps lock
};

KeyboardDevice Keyboard;
KeyboardDevice *KBD1;
extern KeyboardDevice *KBD1;

//static const char *FromUSBKbd;
static int KeyboardInitState = 0;
int KeyboardEnabled = 0;

extern int KeyboardUp(const char *command);
extern int UsbUp;

// Application specific, defined in main.c
extern unsigned int usbKbdCheck(void);
extern char usbKbdGetc(void);
extern void KeyPressedHandler(const char ascii);

/*...................................................................*/
/* Static Local Functions                                            */
/*...................................................................*/

/*...................................................................*/
/* configure_complete: State machine to configure a keyboard         */
/*                                                                   */
/*      Inputs: urb is USB Request Buffer (URB)                      */
/*              param is the keyboard device                         */
/*              context is unused                                    */
/*...................................................................*/
static void configure_complete(void *urb, void *param, void *context)
{
  KeyboardDevice *keyboard = (KeyboardDevice *)param;

  // Clear last request before sending a new one
  if (urb)
    FreeRequest(urb);

  /* Register alternate setting only if set and state is zero */
  if ((keyboard->alternateSetting != 0) && (KeyboardInitState == 0))
  {
    if (HostEndpointControlMessage(keyboard->device.host,
          keyboard->device.endpoint0,
          REQUEST_OUT | REQUEST_TO_INTERFACE, SET_INTERFACE,
          keyboard->alternateSetting,
          keyboard->interfaceNumber, 0, 0,
          configure_complete, keyboard) < 0)
    {
      puts("Cannot set interface");

      return;
    }
    KeyboardInitState = 1;
  }
  else if (KeyboardInitState == 2)
  {
    // Success. Assign global keyboard device and reset state
    KBD1 = keyboard;
    KeyboardInitState = 0;

    // If keyboard specific complete function registered, call it
    if (keyboard->device.complete)
    {
      keyboard->device.complete(NULL, keyboard->device.comp_dev, NULL);
      keyboard->device.complete = NULL;
    }

#if ENABLE_AUTO_START
    // Automatically start the keyboard for the user if configured
    KeyboardUp(NULL);
#endif
  }

  // Otherwise configuration unchanged/ready, set out protocol to boot
  else
  {
    if (HostEndpointControlMessage(keyboard->device.host,
          keyboard->device.endpoint0, REQUEST_OUT | REQUEST_CLASS |
          REQUEST_TO_INTERFACE, SET_PROTOCOL, BOOT_PROTOCOL,
          keyboard->interfaceNumber, 0, 0, configure_complete,
          keyboard) < 0)
    {
      puts("Cannot set boot protocol");

      //reset the state on error
      KeyboardInitState = 0;
      return;
    }

    // Set to final state
    KeyboardInitState = 2;
  }
}

/*...................................................................*/
/*   configure: Start keyboard device configuration                  */
/*                                                                   */
/*      Inputs: device is keyboard device                            */
/*              complete is callback function invoked upon completion*/
/*              param is unused                                      */
/*...................................................................*/
static int configure(Device *device, void (complete)
              (void *urb, void *param, void *context), void *param)
{
  KeyboardDevice *keyboard = (KeyboardDevice *)device;
  ConfigurationDescriptor *confDesc;
  InterfaceDescriptor *interfaceDesc;
  EndpointDescriptor *endpointDesc;

  assert(keyboard != 0);

  confDesc = (ConfigurationDescriptor *)DeviceGetDescriptor(
               keyboard->device.configParser, DESCRIPTOR_CONFIGURATION);
  if (confDesc == 0 || (confDesc->numInterfaces <  1))
  {
    puts("USB keyboard configuration descriptor not valid");
    return FALSE;
  }

  // Read and parse all interface descriptors
  while ((interfaceDesc =
         (InterfaceDescriptor *)DeviceGetDescriptor(
             keyboard->device.configParser, DESCRIPTOR_INTERFACE)) != 0)
  {
    if ((interfaceDesc->numEndpoints <  1) ||
        (interfaceDesc->interfaceClass    != 0x03) || // HID Class
        (interfaceDesc->interfaceSubClass != 0x01) || // Boot Subclass
        (interfaceDesc->interfaceProtocol != 0x01))   // Keyboard
    {
      continue;
    }

    keyboard->interfaceNumber  = interfaceDesc->interfaceNumber;
    keyboard->alternateSetting = interfaceDesc->alternateSetting;

    endpointDesc =
      (EndpointDescriptor *)DeviceGetDescriptor(
                    keyboard->device.configParser, DESCRIPTOR_ENDPOINT);

    if ((endpointDesc == 0) ||
        (endpointDesc->endpointAddress & 0x80) != 0x80) // Input EP
    {
      continue;
    }

    if ((endpointDesc->attributes & 0x3F) == 0x03)  // Interrupt EP
    {
      keyboard->interruptEndpoint = &(keyboard->InterruptEndpoint);
      assert(keyboard->interruptEndpoint != 0);
      EndpointFromDescr(keyboard->interruptEndpoint, &keyboard->device,
                        endpointDesc);
      continue;
    }
    else
    {
      assert (keyboard->reportEndpoint == 0);
      keyboard->reportEndpoint = &(keyboard->ReportEndpoint);
      assert(keyboard->reportEndpoint != 0);
      EndpointFromDescr(keyboard->reportEndpoint, &keyboard->device,
                        endpointDesc);
      continue;
    }

    /* Continue to read all descriptors. */
  }

  /* Return error if no report or interrupt endpoints configured. */
  if ((keyboard->reportEndpoint == 0) &&
      (keyboard->interruptEndpoint == 0))
  {
    puts("USB keyboard endpoint not found");
    return FALSE;
  }

  // Return error if device configuration fails
  if (!DeviceConfigure(&keyboard->device, configure_complete,
                       keyboard))
  {
    puts("Cannot set configuration");
    return FALSE;
  }

  // Return keyboard configuration success.
  return TRUE;
}

#if 0
/*...................................................................*/
/*   register_key_pressed: Start keyboard device configuration                  */
/*                                                                   */
/*      Inputs: device is keyboard device                            */
/*              complete is callback function invoked upon completion*/
/*              param is unused                                      */
/*...................................................................*/
static void register_key_pressed(KeyboardDevice *keyboard,
                                 void (*key_pressed)(const char ascii))
{
  assert(keyboard != 0);
  assert (key_pressed != 0);
  keyboard->key_pressed = key_pressed;
}
#endif

/*...................................................................*/
/* generate_key_event: Generate a key event from a PHY code          */
/*                                                                   */
/*      Inputs: keyboard is keyboard device                          */
/*              phyCode is keyboard device PHY code                  */
/*...................................................................*/
static void generate_key_event(KeyboardDevice *keyboard, u8 phyCode)
{
  assert (keyboard != 0);

  // Modifiers is first byte of the report
  u8 modifiers = keyboard->reportBuffer[0];
  u8 asciiCode;

//  putbyte(phyCode);putchar('.');putbyte(modifiers);

  // If ascii letter and caps lock, set shift modifier
  if ((keyboard->capsLock) && (phyCode <= 0x1D)) // 0x1D is 'z'
    modifiers = 2;

  // Convert Right shift to left shift
  else if (modifiers == RIGHT_SHIFT_MODIFIER)
    modifiers = LEFT_SHIFT_MODIFIER;

  // Otherwise default modifier
  else if (!modifiers)
    modifiers = DEFAULT_MODIFIER;

  // If a valid character use key map to convert to ascii
  if (phyCode <= 0x39)
    asciiCode = KeyMap[phyCode][modifiers - 1];
  else
    return;

  // Toggle capslock if pressed
  if (asciiCode == '\1')
  {
    if (keyboard->capsLock)
      keyboard->capsLock = 0;
    else
      keyboard->capsLock = 1;
    return;
  }

  // Send buffer to registered key pressed handler if present
  if (keyboard->key_pressed != 0)
    (*keyboard->key_pressed)(asciiCode);
}

/*...................................................................*/
/* get_key_event: Retrieve a key event from the keyboard device      */
/*                                                                   */
/*       Input: keyboard is keyboard device                          */
/*                                                                   */
/*     Returns: the current keycode or zero if none                  */
/*...................................................................*/
static u8 get_key_code(KeyboardDevice *keyboard)
{
  unsigned i = 7;
  assert(keyboard != 0);

  for (i = 7; i >= 2; i--)
  {
    u8 keyCode = keyboard->reportBuffer[i];
    if (keyCode != 0)
    {
      return keyCode;
    }
  }

  return 0;
}

/*...................................................................*/
/*  completion: Keyboard URB completion parses keypress events       */
/*                                                                   */
/*       Input: urb is the USB Request Buffer (URB)                  */
/*              param is the keyboard device                         */
/*              context is unused                                    */
/*...................................................................*/
static void completion(void *urb, void *param, void *context)
{
  Request *request = urb;
  KeyboardDevice *keyboard = (KeyboardDevice *)context;

  assert(keyboard != 0);
  assert(request != 0);
  assert(keyboard->urb == request);

  if ((request->status != 0) &&
      (request->resultLen == REPORT_SIZE))
  {
    u8 keyCode = get_key_code(keyboard);

    if (keyCode == keyboard->lastPhyCode)
    {
      keyCode = 0;
    }
    else
    {
      keyboard->lastPhyCode = keyCode;
    }

    if (keyCode != 0)
    {
      generate_key_event(keyboard, keyCode);
    }
    else if (keyboard->tmr != 0)
    {
      keyboard->tmr = 0;
    }
  }

  // Reuse the URB by releasing it
  RequestRelease(keyboard->urb);

  // Reattach to URB, prefering the interrupt endpoint.
  if (keyboard->interruptEndpoint)
    RequestAttach(keyboard->urb, keyboard->interruptEndpoint,
                  keyboard->reportBuffer, REPORT_SIZE, 0);
  else
    RequestAttach(keyboard->urb, keyboard->reportEndpoint,
                  keyboard->reportBuffer, REPORT_SIZE, 0);
  RequestSetCompletionRoutine(keyboard->urb,completion, 0, keyboard);

  // Submit the USB request
  HostSubmitAsyncRequest(keyboard->urb, keyboard->device.host, NULL);
}

/*...................................................................*/
/* start_request: Initiate URB for keypress events                   */
/*                                                                   */
/*       Input: keyboard is the keyboard device                      */
/*                                                                   */
/*     Returns: zero (0)                                             */
/*...................................................................*/
static int start_request(KeyboardDevice *keyboard)
{
  assert(keyboard != 0);

  assert((keyboard->reportEndpoint != 0) ||
         (keyboard->interruptEndpoint != 0));
  assert(keyboard->reportBuffer != 0);

  assert(keyboard->urb == 0);
  keyboard->urb = NewRequest();
  assert(keyboard->urb != 0);
  bzero(keyboard->urb, sizeof(Request));

  /* Prefer the interrupt endpoint. */
  if (keyboard->interruptEndpoint)
    RequestAttach(keyboard->urb, keyboard->interruptEndpoint,
                  keyboard->reportBuffer, REPORT_SIZE, 0);
  else
    RequestAttach(keyboard->urb, keyboard->reportEndpoint,
                  keyboard->reportBuffer, REPORT_SIZE, 0);
  RequestSetCompletionRoutine(keyboard->urb,completion, 0, keyboard);

  HostSubmitAsyncRequest(keyboard->urb, keyboard->device.host, NULL);
  return 0;
}

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* ConsoleEnableKeyboard: Enable the keyboard for video console      */
/*                                                                   */
/*...................................................................*/
void ConsoleEnableKeyboard(void)
{
  /* Extend the console state for keyboard input. */
  ConsoleState.getc = usbKbdGetc;
  ConsoleState.check = usbKbdCheck;
  ConsoleState.puts("USB keyboard capable video console.");
  ConsoleState.puts("'?' for a list of commands");

#if ENABLE_OS
  /* Create the task for the video console. */
  TaskNew(MAX_TASKS - 2, ShellPoll, &ConsoleState);
#endif
}

/*...................................................................*/
/* KeyboardAttach: Attach a device to a keyboard device              */
/*                                                                   */
/*       Input: device is the generic USB device                     */
/*                                                                   */
/*     Returns: void pointer to the new keyboard device              */
/*...................................................................*/
void *KeyboardAttach(Device *device)
{
  KeyboardDevice *keyboard = &Keyboard;

  assert(keyboard != 0);

  DeviceCopy(&keyboard->device, device);
  keyboard->device.configure = configure;

  KeyboardInitState = 0;
  KeyboardEnabled = 0;

  keyboard->capsLock = 0;
  keyboard->reportEndpoint = 0;
  keyboard->key_pressed = 0;
  keyboard->urb = 0;
  keyboard->lastPhyCode = 0;
  keyboard->tmr = 0;

  assert(keyboard->reportBuffer != 0);
  return keyboard;
}

/*...................................................................*/
/* KeyboardRelease: Release a keyboard device                        */
/*                                                                   */
/*       Input: device is the USB keyboard device                    */
/*...................................................................*/
void KeyboardRelease(KeyboardDevice *device)
{
  KeyboardDevice *keyboard = (void *)device;
  assert(keyboard != 0);

  if (keyboard->reportEndpoint != 0)
  {
    EndpointRelease(keyboard->reportEndpoint);
    keyboard->reportEndpoint = 0;
  }
  if (keyboard->interruptEndpoint != 0)
  {
    EndpointRelease(keyboard->interruptEndpoint);
    keyboard->interruptEndpoint = 0;
  }

  DeviceRelease(&keyboard->device);
}

/*...................................................................*/
/*  KeyboardUp: Activate a discovered and configured USB keyboard    */
/*                                                                   */
/*       Input: command is unused                                    */
/*                                                                   */
/*     Returns: TASK_FINISHED as it is a shell command               */
/*...................................................................*/
int KeyboardUp(const char *command)
{
  if (!UsbUp)
    puts("USB not initialized");

  else if (!KBD1)
    puts("Keyboard not present");

  else if (!KeyboardEnabled)
  {
      KeyboardEnabled = TRUE;

      /* Register the key handler and create the new console task */
      KBD1->key_pressed = KeyPressedHandler;//register_key_pressed(KBD1, KeyPressedHandler);

      /* Asynchronous send of USB interrupt request. */
      start_request(KBD1);

#if ENABLE_VIDEO
      /* Start the interactive console if screen is up. */
      if (ScreenUp)
        ConsoleEnableKeyboard();
#endif
  }
  else
    puts("USB keyboard already initialized");

  return TASK_FINISHED;
}

#endif /* ENABLE_USB_HID */
