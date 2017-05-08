/*
The MIT License (MIT)

Copyright (c) 2014 Michael Lelli

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <time.h> //#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


#include <gctypes.h>

typedef u8 __u8;
typedef u16 __u16;
typedef u32 __u32;
typedef u64 __u64;

typedef s16 __s16;
typedef s32 __s32;


#include "input.h"

#include "usb.h"
#include "usb.c.h"


#define STATE_NORMAL   0x10
#define STATE_WAVEBIRD 0x20

#define MAX_FF_EVENTS 4

const int BUTTON_OFFSET_VALUES[16] = {
   BTN_START,
   BTN_TR2,
   BTN_TR,
   BTN_TL,
   -1,
   -1,
   -1,
   -1,
   BTN_SOUTH,
   BTN_WEST,
   BTN_EAST,
   BTN_NORTH,
   BTN_DPAD_LEFT,
   BTN_DPAD_RIGHT,
   BTN_DPAD_DOWN,
   BTN_DPAD_UP,
};

const int AXIS_OFFSET_VALUES[6] = {
   ABS_X,
   ABS_Y,
   ABS_RX,
   ABS_RY,
   ABS_Z,
   ABS_RZ
};


struct ports
{
   bool connected;
   bool extra_power;
   //int uinput;
   unsigned char type;
   uint16_t buttons;
   uint8_t axis[6];
};

struct adapter
{
   volatile bool quitting;
   usb_device_entry* device;
   s32 fd;
   unsigned char rumble[5];
   struct ports controllers[4];
   struct adapter *next;
};


static struct adapter adapters;

static struct adapter ata; //Adapter Thread adapter

volatile bool addedAdapter = false;

volatile u32 errorCode = 0;
volatile u32 usbError = 0;


static unsigned char connected_type(unsigned char status)
{
   unsigned char type = status & (STATE_NORMAL | STATE_WAVEBIRD);
   switch (type)
   {
      case STATE_NORMAL:
      case STATE_WAVEBIRD:
         return type;
      default:
         return 0;
   }
}


static __attribute__((noinline)) void handle_payload(int i, struct ports *port, unsigned char *payload/*, struct timespec *current_time*/)
{
   unsigned char status = payload[0];
   unsigned char type = connected_type(status);

   if (type != 0 && !port->connected)
   {
      port->type = type;
	  port->connected = true;
   }
   else if (type == 0 && port->connected)
   {
      port->connected = false;
   }

   if (!port->connected)
      return;

   port->extra_power = ((status & 0x04) != 0);

   if (type != port->type)
   {
      port->type = type;
   }

   input_event events[12+6+1];// 12 + 6 + 1 // buttons + axis + syn event
   //memset(&events, 0, sizeof(events));
   int n;
   for (n = 0; n < sizeof(events); n++)
   {
		char* ptr = (char*)events;
		ptr += n;
		*ptr = 0;
   }
   int e_count = 0;

   uint16_t btns = (uint16_t) payload[1] << 8 | (uint16_t) payload[2];
   

   int j;
   for (j = 0; j < 16; j++)
   {
      if (BUTTON_OFFSET_VALUES[j] == -1)
         continue;

      uint16_t mask = (1 << j);
      uint16_t pressed = btns & mask;

      if ((port->buttons & mask) != pressed)
      {
         events[e_count].type = EV_KEY;
         events[e_count].code = BUTTON_OFFSET_VALUES[j];
         events[e_count].value = (pressed == 0) ? 0 : 1;
         e_count++;
         port->buttons &= ~mask;
         port->buttons |= pressed;
      }
   }

   for (j = 0; j < 6; j++)
   {
      unsigned char value = payload[j+3];

      if (AXIS_OFFSET_VALUES[j] == ABS_Y || AXIS_OFFSET_VALUES[j] == ABS_RY)
         value ^= 0xFF; // flip from 0 - 255 to 255 - 0

      if (port->axis[j] != value)
      {
         events[e_count].type = EV_ABS;
         events[e_count].code = AXIS_OFFSET_VALUES[j];
         events[e_count].value = value;
         e_count++;
         port->axis[j] = value;
      }
   }
}

//We don't want these optimized out
//#pragma GCC optimize 3
static __attribute__((used)) u32 adapter_getType(u32 chan)
{
	//No adapter
	if (!addedAdapter)
		return 0;
	
	//Plugged
	if (ata.controllers[chan].type == STATE_NORMAL
		|| ata.controllers[chan].type == STATE_WAVEBIRD)
		return 0x09000000;
		
	//Unplugged
	return 0;
}


static __attribute__((used)) int adapter_thread(s32 chan, void* buf)
{
   __asm__ volatile ("nop\n\tnop\n\tnop\n\tnop");
   
   if (!addedAdapter) return 0;
	
   //if (data == NULL) return NULL;
   
   //struct adapter *a = (struct adapter *)data;
   struct adapter *a = &ata;
   
   //while (!a->quitting)
   if (!a->quitting)
   {
      unsigned char payload[37] ATTRIBUTE_ALIGN(32);
      int usbret = USB_ReadIntrMsg(a->fd, USB_ENDPOINT_IN, sizeof(payload), payload);
      
      if (usbret != 37 || payload[0] != 0x21)
         //continue;
         return 0;//Apparently this is an error return value
      
      unsigned char *controller = &payload[1];

      //unsigned char rumble[5] ATTRIBUTE_ALIGN(32) = {0x11, 0, 0, 0, 0};
      //struct timespec current_time ATTRIBUTE_ALIGN(32) = {0};
      //clock_gettime(&current_time);
      int i;
      for (i = 0; i < 4; i++, controller += 9)
      {
         handle_payload(i, &a->controllers[i], controller);
         
         //rumble[i+1] = 0;
         
      }
      
      //Write button state
      *p1BtnAddr = a->controllers[0].buttons;

      /*if (memcmp(rumble, a->rumble, sizeof(rumble)) != 0)
      {
         memcpy(a->rumble, rumble, sizeof(rumble));
         USB_WriteIntrMsg(a->fd, USB_ENDPOINT_OUT, sizeof(a->rumble), a->rumble);
      }*/
   }

   __asm__ volatile ("lis 14,0x2000\n\tmtcr 14\n\tlis 14,0");
   return 1;// Apparently this means !error
}
//#pragma GCC optimize 3


static u32 add_adapter(usb_device_entry* dev)
{
   struct adapter *a = &ata;//(struct adapter *)calloc(1, sizeof(struct adapter));
   if (a == NULL)
   {
      return 0xeeeeeeee;
   }
   a->device = dev;
   
   if (IUSB_OpenDeviceIds("oh0", 0x057e, 0x0337, &a->fd) == 0)
   {
      return 4;
   }

   unsigned char payload[1] ATTRIBUTE_ALIGN(32) = { 0x13 };
   usbError = USB_WriteIntrMsg(a->fd, USB_ENDPOINT_OUT, sizeof(payload), payload);
   if (usbError < 0)
   {
		return usbError;
   }

   struct adapter *old_head = adapters.next;
   adapters.next = a;
   a->next = old_head;
   
   return 6;
   //LWP_CreateThread(&a->thread, adapter_thread, a, NULL, 0, 0);
}

//Keeps compiler from complaining about unused functions
static void dummy_use_functions()
{
	char dummy[2];
   	adapter_thread(0, dummy);
   	adapter_getType(0);
}

void _start()
{
   __asm__("mr 3,3");
   errorCode = 1;

   //So I know this actually executed (debug)
   *dbg1TeamMatch = 0x38600000;
   //USB_Initialize();// unnecessary?
   

   while (!addedAdapter) // Enter main loop
   {
	    usb_device_entry devices[2];
		u8 count = 0;
		int i;
		
		memset(devices, 0, sizeof(usb_device_entry) * 2);
   
		USB_GetDeviceList(devices, 2, 0, &count);
		for (i = 0; i < count; i++)
		{
			if (devices[i].vid == 0x057e && devices[i].pid == 0x0337 && !addedAdapter)
			{
				if(add_adapter(&devices[i]) == 6)
					addedAdapter = true;
			}
		}
   }
   
   if (0) dummy_use_functions();
   
   __asm__ volatile ("mr 4,4");
}
