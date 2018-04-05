#ifndef _FUNCTIONPOINTERS_H_
#define _FUNCTIONPOINTERS_H_


#include <errno.h>
#include <gctypes.h>
#include <ogc/ipc.h>


u8* initDone = (u8*)0x805A0CD8;// Explained in wii-u-gamecube-adapter.c

s32* origUsbHeapId = (s32*)0x8059ED80;
u8* pad_origin = (u8*)0x804f6760;


//Not a function POINTER, but since we don't have libc...
void* memset(void *__s, int __c, unsigned int num)
{
	int i;
	for (i = 0 ; i < num ; i++) *((char*)__s+i) = __c;
	return __s;
}


s32 (*IOS_Ioctl)(s32 fd,s32 ioctl,void *buffer_in,s32 len_in,void *buffer_io,s32 len_io) = (void*)0x80212d40;
s32 (*IOS_Ioctlv)(s32 fd, s32 ioctl, s32 cnt_in, s32 cnt_io, ioctlv *argv) = (void*)0x80213090;
s32 (*IOS_IoctlvAsync)(s32 fd,s32 ioctl,s32 cnt_in,s32 cnt_io,ioctlv *argv,ipccallback ipc_cb,void *usrdata) = (void*)0x80212fac;
s32 (*IOS_IoctlAsync)(s32 fd,s32 ioctl,void *buffer_in,s32 len_in,void *buffer_io,s32 len_io,ipccallback ipc_cb,void *usrdata) = (void*)0x80212c08;
//argument 3 is likely the desired alignment boundary,
//but we'll probably always want 0x20, so this
//macro makes it work more as expected
void* (*_iosAlloc)(s32 hid,s32 size,u32 always0x20WTH) = (void*)0x80213598;
#define iosAlloc(x,y) _iosAlloc(x,y,0x20)
void (*iosFree)(s32 hid,void *ptr) = (void*)0x802137a8;
void* (*IPC_GetBufferLo)() = (void*)0x80211ae8;
s32 (*iosCreateHeap)(void* bufferLo, s32 size) = (void*)0x80213468;
s32 (*IOS_Open)(const char *filepath,u32 mode) = (void*)0x802123a8;
s32 (*IOS_Close)(s32 fd) = (void*)0x80212588;
s32 (*IUSB_OpenDeviceIds)(char* device_id,u16 vid,u16 pid,s32 *fd) = (void*)0x80228c30;
void (*PAD_UpdateOrigin)(s32 chan) = (void*)0x802153f8;

#endif
