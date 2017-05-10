#ifndef _FUNCTIONPOINTERS_H_
#define _FUNCTIONPOINTERS_H_


#include <errno.h>
#include <gctypes.h>
#include <ogc/ipc.h>


//The address of player one's button inputs
u16* p1BtnAddr = (u16*)0x804de4b0;

//Will enable 1 team matches when set to 0x38600000
//(use for debugging)
u32* dbg1TeamMatch = (u32*)0x8068D534;

u8* enableUsbLog = (u8*)0x805A0CD8;


void* memset(void *__s, int __c, unsigned int num)
{
	int i;
	for (i = 0 ; i < num ; i++) *((char*)__s+i) = __c;
	return __s;
}


s32 (*IOS_Ioctlv)(s32 fd, s32 ioctl, s32 cnt_in, s32 cnt_io, ioctlv *argv) = (void*)0x80213090;//0x80212a40;
//For some unknown reason, it seems that every call to iosAlloc
//is made with r5 set to 0x20... I dunno why, but iosAlloc requires
//that (r5 != NULL). Seemingly all calls to iosAlloc within SSBB just do this.
//Likely alignment boundary.
void* (*_iosAlloc)(s32 hid,s32 size,u32 always0x20WTH) = (void*)0x80213598;
#define iosAlloc(x,y) _iosAlloc(x,y,0x20)
void (*iosFree)(s32 hid,void *ptr) = (void*)0x802137a8;
void* (*IPC_GetBufferLo)() = (void*)0x80211ae8;
//Not sure of arg0
s32 (*iosCreateHeap)(void* bufferLo, s32 size) = (void*)0x80213468;
s32 (*IOS_Open)(const char *filepath,u32 mode) = (void*)0x802123a8;
s32 (*IOS_Close)(s32 fd) = (void*)0x80212588;

//s32 (*printf)(const char* format, ...) = (void*)0x803f861c;

//__save_gpr 803f12ec

// Returns fd, even though this is also filled in at the address of argument 4
// Example:
// 	s32 fd;
// 	if (IUSB_OpenDeviceIds("oh1", 0x57e, 0x305, &fd)) printf("Error\n");
s32 (*IUSB_OpenDeviceIds)(char* device_id,u16 vid,u16 pid,s32 *fd) = (void*)0x80228c30;


//s32 USB_GetDeviceList(usb_device_entry *descr_buffer,u8 num_descr,u8 interface_class,u8 *cnt_descr);


#endif
