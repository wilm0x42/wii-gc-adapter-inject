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

#include <time.h>
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
   usb_device_entry* device;
   s32 fd;
   unsigned char rumble[5] ATTRIBUTE_ALIGN(32);
   bool rumbleChanged;
   struct ports controllers[4];
   struct adapter *next;
};


static struct adapter ata; //Adapter Thread Adapter

volatile bool addedAdapter = false;


void look_for_adapter();


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


static __attribute__((noinline)) void handle_payload(int i, struct ports *port, unsigned char *payload)
{
   unsigned char status = payload[0];
   unsigned char type = connected_type(status);
   bool plugChanged = false;

   if (type != 0 && !port->connected)
   {
      port->type = type;
	  port->connected = true;
	  plugChanged = true;
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
      			outBtns |= 0x0002;
      			break;
      		case 12://BTN_DPAD_RIGHT:
      			outBtns |= 0x0001;
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
   //The purpose of this bit is unknown. (So YACGD says)
   //But, it seems to be set under normal circumstances,
   //so we'll just set it for the sake of correctness.
   outBtns |= 0x0080;
   
   port->buttons = outBtns;

   for (j = 0; j < 6; j++)
   {
   	  unsigned char value = payload[j+3];
   	  
   	  if (AXIS_OFFSET_VALUES[j] == ABS_Z || AXIS_OFFSET_VALUES[j] == ABS_RZ)
   	  {
   	  	value = ((value >= 35)? (value - 35) : 0);
   	  }
   	  
      port->axis[j] = value;
   }
   
   if (plugChanged)
   {
   		int off = (i*12)+2;
   		int n;
   		for (n = 0; n < 6; n++)
   		{
   			pad_origin[off + n] = port->axis[n];
   		}
   		*((uint16_t*)pad_origin) = port->buttons;
   		PAD_UpdateOrigin(i);
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
	
	//The return value can really be anything without bit 0x08 set,
	//but this is what's returned when using dolphin's internal
	//gamecube adapter support.
	return 0x00000020;
}

static __attribute__((used)) int adapter_getResponse(u32 chan, void* buf)
{
	if (!addedAdapter)
		return 0;//False for "Failure"

	memset(buf, 0, 8);
    *((uint16_t*)buf) = ata.controllers[chan].buttons;
    ((s8*)buf)[4] = ata.controllers[chan].axis[2];//RX
    ((s8*)buf)[5] = ata.controllers[chan].axis[3];//RY
    ((s8*)buf)[6] = ata.controllers[chan].axis[4];//LX
    ((s8*)buf)[7] = ata.controllers[chan].axis[5];//LY
    ((s8*)buf)[2] = ata.controllers[chan].axis[0];//TL
    ((s8*)buf)[3] = ata.controllers[chan].axis[1];//TR
    
    return 1;//True for "Tremendous Successs"
}

static __attribute__((used)) int adapter_isChanBusy(u32 chan)
{
	if (!addedAdapter)
		return 1;//Busy, not ready yet
		
	if (ata.controllers[chan].connected)
		return 0;//Not busy
		
	return 1;//Otherwise... busy
}

static __attribute__((used)) void adapter_controlMotor(s32 chan, u32 cmd)
{
	if (chan < 0 || chan > 3 || !addedAdapter)
		return;
		
	ata.rumble[chan + 1] = cmd;
	ata.rumbleChanged = true;
}

static s32 dummyUsbCB(s32 result, void* dummy)
{
	return 0;
}

//This previously replaced SI_IsChanBusy
//Now it replaces the instruction "addi r3, r3, 208"
//So all return values should be (inputNum + 208), no exceptions.
//(Pun intended)
static __attribute__((used)) u32 adapter_thread(int inputNum)
{
   u32 ret = inputNum + 208;

   if (!addedAdapter)
   {
   		//Hotplugging
   		look_for_adapter();
   		return ret;
   }
   
   struct adapter *a = &ata;
   
   unsigned char payload[37] ATTRIBUTE_ALIGN(32);
   int usbret = USB_ReadIntrMsg(a->fd, USB_ENDPOINT_IN, sizeof(payload), payload);
   if (usbret != 37 || payload[0] != 0x21)
	  return ret;
  
   unsigned char *controller = &payload[1];
   
   int i;
   for (i = 0; i < 4; i++, controller += 9)
   {
      handle_payload(i, &a->controllers[i], controller);
   }
 
   if (a->rumbleChanged)
   {
      if (USB_WriteIntrMsgAsync(a->fd, USB_ENDPOINT_OUT, sizeof(a->rumble), a->rumble, dummyUsbCB, NULL) >= 0)
      	a->rumbleChanged = false;
   }
   
   return ret;
}


static s32 adapter_removal_cb(s32 result, void *dummy)
{
	addedAdapter = false;
	return 0;
}

static u32 add_adapter(usb_device_entry* dev)
{
   struct adapter *a = &ata;
   memset(a, 0, sizeof(struct adapter));
   a->device = dev;
   a->rumble[0] = 0x11;
   
   //If we ever ditch IUSB_OpenDeviceIds again, here's the code for that:
   /*
	   char* devPath = (char*)iosAlloc(*hId, 32);
	   char pathStr[] = "/dev/usb/oh0/57e/337";
	   memset(devPath, 0, 32);
	   int n;
	   for (n = 0; n < sizeof(pathStr); n++)
	   {
	   		devPath[n] = pathStr[n];
	   }
	   a->fd = IOS_Open(devPath, 1|2);//Read | Write
	   iosFree(*hId, devPath);
   */
   //a->fd = IUSB_OpenDeviceIds("oh0", 0x057e, 0x0337, &a->fd);
   //if (a->fd < 0) return a->fd;
   if (IUSB_OpenDeviceIds("oh0", 0x057e, 0x0337, &a->fd) < 0) return a->fd;
   

   unsigned char payload[1] ATTRIBUTE_ALIGN(32) = {0x13};
   
   int usbret = USB_WriteIntrMsg(a->fd, USB_ENDPOINT_OUT, sizeof(payload), payload);
   if (usbret < 0)
		return usbret;
   
   //Add a callback that'll tell us when the adapter is removed
   //                   26 == USBV0_IOCTL_DEVREMOVALHOOK
   IOS_IoctlAsync(a->fd,26,NULL,0,NULL,0,adapter_removal_cb,NULL);
   
   return 0;
}

void look_for_adapter()
{
	if (!addedAdapter)
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
}

void _start()
{
   //Disable usb log
   *enableUsbLog = 1;
   
   //Initialize heap
   USB_Initialize();
   
   addedAdapter = false;
}
