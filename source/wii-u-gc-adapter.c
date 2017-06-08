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

static struct adapter ata; //Adapter Thread Adapter

volatile bool addedAdapter = false;

volatile u32 adapterThreadError = 0xff;


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

   /*input_event events[12+6+1];// 12 + 6 + 1 // buttons + axis + syn event
   memset(&events, 0, sizeof(events));
   int n;
   for (n = 0; n < sizeof(events); n++)
   {
		char* ptr = (char*)events;
		ptr += n;
		*ptr = 0;
   }
   int e_count = 0;*/

   uint16_t btns = (uint16_t) payload[1] << 8 | (uint16_t) payload[2];
   uint16_t outBtns = 0;

   int j;
   for (j = 0; j < 16; j++)
   {
      uint16_t mask = (1 << j);
      uint16_t pressed = btns & mask;

      if (pressed)
      {
      	switch (j)
      	{
      		case 13://BTN_DPAD_LEFT:
      			outBtns |= 0x0001;
      			break;
      		case 12://BTN_DPAD_RIGHT:
      			outBtns |= 0x0002;
      			break;
      		case 14://BTN_DPAD_DOWN:
      			outBtns |= 0x0004;
      			break;
      		case 15://BTN_DPAD_UP:
      			outBtns |= 0x0008;
      			break;
      		case 1://BTN_TR2:
      			outBtns |= 0x0010;
      			break;
      		case 2://BTN_TR:
      			outBtns |= 0x0020;
      			break;
      		case 3://BTN_TL:
      			outBtns |= 0x0040;
      			break;
      		case 8://BTN_SOUTH:
      			outBtns |= 0x0100;
      			break;
      		case 9://BTN_WEST:
      			outBtns |= 0x0200;
      			break;
      		case 10://BTN_EAST:
      			outBtns |= 0x0400;
      			break;
      		case 11://BTN_NORTH:
      			outBtns |= 0x0800;
      			break;
      		case 0://BTN_START:
      			outBtns |= 0x1000;
      			break;
      	}
      }
   }
   
   //Write button state
   port->buttons = outBtns;

   for (j = 0; j < 6; j++)
   {
      unsigned char value = payload[j+3];

      //if (AXIS_OFFSET_VALUES[j] == ABS_Y || AXIS_OFFSET_VALUES[j] == ABS_RY)
         //value ^= 0xFF; // flip from 0 - 255 to 255 - 0

      port->axis[j] = value;
   }
}

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

static __attribute__((used)) u32 adapter_getStatus(u32 chan)
{
	if (!addedAdapter)
		return 0x08;//SI_ERROR_NO_RESPONSE
	
	if (!ata.controllers[chan].connected)
		return 0x08;//SI_ERROR_NO_RESPONSE
	
	return 0;
}

static __attribute__((used)) int adapter_getResponse(u32 chan, void* buf)
{
	struct adapter *a = &ata;
	if (!a) return 0;//False for "Failure"

	memset(buf, 0, 8);
    *((uint16_t*)buf) = a->controllers[0].buttons;
    ((s8*)buf)[4] = a->controllers[0].axis[2];//RX
    ((s8*)buf)[5] = a->controllers[0].axis[3];//RY
    ((s8*)buf)[6] = a->controllers[0].axis[0];//LX
    ((s8*)buf)[7] = a->controllers[0].axis[1];//LY
    ((s8*)buf)[2] = a->controllers[0].axis[5];//TR
    ((s8*)buf)[3] = a->controllers[0].axis[4];//TL
    
    return 1;//True for "Tremendous Successs"
}

//AKA adapter_isChanBusy(u32 chan)
static __attribute__((used)) int adapter_thread(u32 chan)
{
   adapterThreadError = 0;

   if (!addedAdapter)
   {
   		adapterThreadError |= 1;
   		return 1;//"Channel is busy. Try again later."
   }
   
   //struct adapter *a = (struct adapter *)data;
   struct adapter *a = &ata;
   
   if (!a->quitting)
   {
      unsigned char payload[37] ATTRIBUTE_ALIGN(32);
      int usbret = USB_ReadIntrMsg(a->fd, USB_ENDPOINT_IN, sizeof(payload), payload);
      if (usbret != 37 || payload[0] != 0x21)
      {
      	 adapterThreadError |= 2;
         return 1;//"Nope, this channel is busy"
      }
      
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

      /*if (memcmp(rumble, a->rumble, sizeof(rumble)) != 0)
      {
         memcpy(a->rumble, rumble, sizeof(rumble));
         USB_WriteIntrMsg(a->fd, USB_ENDPOINT_OUT, sizeof(a->rumble), a->rumble);
      }*/
   }
   
   if (!a->controllers[chan].connected)
   		return 1;//"Sorry, this channel is busy right now."
   		
   return 0;//"No, this channel isn't busy at this time."
}


static u32 add_adapter(usb_device_entry* dev)
{
   //(struct adapter *)calloc(1, sizeof(struct adapter));
   struct adapter *a = &ata;
   /*if (a == NULL)
   {
      return IPC_ENOMEM;
   }*/
   a->device = dev;
   
   //NOTE: We might want to use our own function, shown below.
   //Reason being, IUSB_OpenDeviceIds uses the game's
   //preexisting USB heap, and not our dedicated one.
   a->fd = IUSB_OpenDeviceIds("oh0", 0x057e, 0x0337, &a->fd);
   if (a->fd < 0)
   {
      return a->fd;
   }
   
   /*char* devPath = (char*)iosAlloc(*hId, 32);
   char pathStr[] = "/dev/usb/oh0/57e/337";
   memset(devPath, 0, 32);
   int n;
   for (n = 0; n < sizeof(pathStr); n++)
   {
   		devPath[n] = pathStr[n];
   }
   a->fd = IOS_Open(devPath, 1|2);//Read | Write
   iosFree(*hId, devPath);
   if (a->fd < 0) return a->fd;*/

   unsigned char payload[1] ATTRIBUTE_ALIGN(32) = {0x13};
   
   int usbret = USB_WriteIntrMsg(a->fd, USB_ENDPOINT_OUT, sizeof(payload), payload);
   if (usbret < 0)
		return usbret;

   struct adapter *old_head = adapters.next;
   adapters.next = a;
   a->next = old_head;
   
   return 0;
}

//Keeps GCC from complaining about "unused" functions
static void dummy_use_functions()
{
   	adapter_thread(0);
   	adapter_getType(0);
}

void _start()
{
   //To mark beginning of our code here
   __asm__("mr 3,3");
   
   //Disable usb log
   *enableUsbLog = 1;

   //Allow me to check for successful
   //code execution on console (without USB gecko)
   *dbg1TeamMatch = 0x38600000;
   
   //Initialize heap
   USB_Initialize();
   

   //Until we've found and successfully
   //initialized the adapter
   while (!addedAdapter)
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
				if(!add_adapter(&devices[i]))
					addedAdapter = true;
			}
		}
   }
   
   //Keeps GCC from complaining about
   //"unused" functions
   if (0) dummy_use_functions();
   
   //To mark the end of our code here
   __asm__ volatile ("mr 4,4");
}
