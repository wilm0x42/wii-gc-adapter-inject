/*-------------------------------------------------------------

usb.c -- USB lowlevel

Copyright (C) 2008
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)
tueidj

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/


//Yo, as per the copyright notice above, I should
//mention that this is a MODIFIED version of this code!
//(DUN DUN DUUUUUNNNNN!)
//  -Wilm


/*  Note: There are 3 types of USB interfaces here, the early ones
 *  (V0: /dev/usb/oh0 and /dev/usb/oh1) and two later ones (V5: /dev/usb/ven
 *  and /dev/usb/hid) which are similar but have some small
 *  differences. There is also an earlier version of /dev/usb/hid (V4)
 *  found in IOSes 37,61,56,etc. and /dev/usb/msc found in IOS 57.
 *  These interfaces aren't implemented here and you may find some
 *  devices don't show up if you're running under those IOSes.
 */

#if defined(HW_RVL)

#include "functionPointers.h"
#include <ogc/ipc.h>
//#include <ogc/machine/asm.h>
//#include <ogc/machine/processor.h>//*/

#include "usb.h"

#define USB_HEAPSIZE                        0x4000 //16384

#define USBV0_IOCTL_CTRLMSG                      0
#define USBV0_IOCTL_BLKMSG                       1
#define USBV0_IOCTL_INTRMSG                      2
#define USBV0_IOCTL_SUSPENDDEV                   5
#define USBV0_IOCTL_RESUMEDEV                    6
#define USBV0_IOCTL_ISOMSG                       9
#define USBV0_IOCTL_GETDEVLIST                  12
#define USBV0_IOCTL_DEVREMOVALHOOK              26
#define USBV0_IOCTL_DEVINSERTHOOK               27
#define USBV0_IOCTL_DEVICECLASSCHANGE           28

#define USBV4_IOCTL_GETVERSION                   6 // returns 0x40001

#define USBV5_IOCTL_GETVERSION                   0 // should return 0x50001
#define USBV5_IOCTL_GETDEVICECHANGE              1
#define USBV5_IOCTL_SHUTDOWN                     2
#define USBV5_IOCTL_GETDEVPARAMS                 3
#define USBV5_IOCTL_ATTACHFINISH                 6
#define USBV5_IOCTL_SETALTERNATE                 7
#define USBV5_IOCTL_SUSPEND_RESUME              16
#define USBV5_IOCTL_CANCELENDPOINT              17
#define USBV5_IOCTL_CTRLMSG                     18
#define USBV5_IOCTL_INTRMSG                     19
#define USBV5_IOCTL_ISOMSG                      20
#define USBV5_IOCTL_BULKMSG                     21
#define USBV5_IOCTL_MSC_READWRITE_ASYNC         32 /* unimplemented */
#define USBV5_IOCTL_MSC_READ_ASYNC              33 /* unimplemented */
#define USBV5_IOCTL_MSC_WRITE_ASYNC             34 /* unimplemented */
#define USBV5_IOCTL_MSC_READWRITE               35 /* unimplemented */
#define USBV5_IOCTL_MSC_RESET                   36 /* unimplemented */

#define USB_MAX_DEVICES                         32


static s32 hIdBuf = -1;
static s32* hId = &hIdBuf;//(s32*)0x8059ed80;
static const char __oh0_path[] ATTRIBUTE_ALIGN(32) = "/dev/usb/oh0";
//static const char __ven_path[] ATTRIBUTE_ALIGN(32) = "/dev/usb/ven";
//static const char __hid_path[] ATTRIBUTE_ALIGN(32) = "/dev/usb/hid";

/*typedef struct _usb_cb_list {
	usbcallback cb;
	void *userdata;
	union {
		s32 device_id;
		struct _usb_cb_list *next;
	};
} _usb_cb_list;*/

/*struct _usbv5_host {
	usb_device_entry attached_devices[USB_MAX_DEVICES];
	_usb_cb_list remove_cb[USB_MAX_DEVICES];
	s32 fd;
	_usb_cb_list *device_change_notify;
};*/

//static struct _usbv5_host* ven_host = NULL;
//static struct _usbv5_host* hid_host = NULL;

struct _usb_msg {
	s32 fd;
	u32 heap_buffers;
	union {
		struct {
			u8 bmRequestType;
			u8 bmRequest;
			u16 wValue;
			u16 wIndex;
			u16 wLength;
			void *rpData;
		} ctrl;

		struct {
			void *rpData;
			u16 wLength;
			u8 pad[4];
			u8 bEndpoint;
		} bulk;

		struct {
			void *rpData;
			u16 wLength;
			u8 bEndpoint;
		} intr;

		struct {
			void *rpData;
			void *rpPacketSizes;
			u8 bPackets;
			u8 bEndpoint;
		} iso;

		struct {
			u16 pid;
			u16 vid;
		} notify;

		u8 class;
		u32 hid_intr_dir;

		u32 align_pad[4]; // pad to 24 bytes
	};
	usbcallback cb;
	void *userdata;
	ioctlv vec[7];
};

static inline s32 __usb_interrupt_bulk_message(s32 device_id,u8 ioctl,u8 bEndpoint,u16 wLength,void *rpData,usbcallback cb,void *userdata)
{
	s32 ret = IPC_ENOMEM;
	struct _usb_msg *msg;

	if(((s32)rpData%32)!=0) return IPC_EINVAL;
	if(wLength && !rpData) return IPC_EINVAL;
	if(!wLength && rpData) return IPC_EINVAL;

	msg = (struct _usb_msg*)iosAlloc(*hId,sizeof(struct _usb_msg));
	if(msg==NULL) return IPC_ENOMEM;

	memset(msg, 0, sizeof(struct _usb_msg));

	msg->fd = device_id;
	msg->cb = cb;
	msg->userdata = userdata;

	//if (device_id>=0 && device_id<0x20) {
		u8 *pEndP = NULL;
		u16 *pLength = NULL;

		pEndP = (u8*)iosAlloc(*hId,32);
		if(pEndP==NULL) goto done;
		*pEndP = bEndpoint;

		pLength = (u16*)iosAlloc(*hId,32);
		if(pLength==NULL) goto done;
		*pLength = wLength;

		msg->vec[0].data = pEndP;
		msg->vec[0].len = sizeof(u8);
		msg->vec[1].data = pLength;
		msg->vec[1].len = sizeof(u16);
		msg->vec[2].data = rpData;
		msg->vec[2].len = wLength;

		msg->heap_buffers = 2;

		if (cb==NULL)
			ret = IOS_Ioctlv(device_id,ioctl,2,1,msg->vec);
		//else
		//	return IOS_IoctlvAsync(device_id,ioctl,2,1,msg->vec,__usbv0_messageCB,msg);

done:
		if(pEndP!=NULL) iosFree(*hId,pEndP);
		if(pLength!=NULL) iosFree(*hId,pLength);

	/*} else {
		u8 endpoint_dir = !!(bEndpoint&USB_ENDPOINT_IN);
		s32 fd = (device_id<0) ? ven_host->fd : hid_host->fd;

		if (ioctl == USBV0_IOCTL_INTRMSG) {
			// HID does this a little bit differently
			if (device_id>=0)
				msg->hid_intr_dir = !endpoint_dir;
			else {
				msg->intr.rpData = rpData;
				msg->intr.wLength = wLength;
				msg->intr.bEndpoint = bEndpoint;
			}
			ioctl = USBV5_IOCTL_INTRMSG;
		} else {
			msg->bulk.rpData = rpData;
			msg->bulk.wLength = wLength;
			msg->bulk.bEndpoint = bEndpoint;
			ioctl = USBV5_IOCTL_BULKMSG;
		}

		msg->vec[0].data = msg;
		msg->vec[0].len = 64;
		msg->vec[1].data = rpData;
		msg->vec[1].len = wLength;

		if (cb==NULL)
			ret = IOS_Ioctlv(fd, ioctl, 2-endpoint_dir, endpoint_dir, msg->vec);
		//else
		//	return IOS_IoctlvAsync(fd, ioctl, 2-endpoint_dir, endpoint_dir, msg->vec, __usbv5_messageCB, msg);
	}*/

	if(msg!=NULL) iosFree(*hId,msg);

	return ret;
}

s32 USB_Initialize()
{
	if(*hId==-1) *hId = iosCreateHeap(IPC_GetBufferLo(), USB_HEAPSIZE);
	if(*hId<0) return IPC_ENOMEM;

	/*if (ven_host==NULL) {
		s32 ven_fd = IOS_Open(__ven_path, IPC_OPEN_NONE);
		if (ven_fd>=0) {
			ven_host = (struct _usbv5_host*)iosAlloc(*hId, sizeof(*ven_host));
			if (ven_host==NULL) {
				IOS_Close(ven_fd);
				return IPC_ENOMEM;
			}
			memset(ven_host, 0, sizeof(*ven_host));
			ven_host->fd = ven_fd;

			u32 *ven_ver = (u32*)iosAlloc(*hId, 0x20);
			if (ven_ver==NULL) goto mem_error;
			if (IOS_Ioctl(ven_fd, USBV5_IOCTL_GETVERSION, NULL, 0, ven_ver, 0x20)==0 && ven_ver[0]==0x50001);
				IOS_IoctlAsync(ven_fd, USBV5_IOCTL_GETDEVICECHANGE, NULL, 0, ven_host->attached_devices, 0x180, __usbv5_devicechangeCB, ven_host);
			else {
				// wrong ven version
				IOS_Close(ven_fd);
				iosFree(*hId, ven_host);
				ven_host = NULL;
			}

			iosFree(*hId, ven_ver);
		}
	}

	if (hid_host==NULL) {
		s32 hid_fd = IOS_Open(__hid_path, IPC_OPEN_NONE);
		if (hid_fd>=0) {
			hid_host = (struct _usbv5_host*)iosAlloc(*hId, sizeof(*hid_host));
			if (hid_host==NULL) {
				IOS_Close(hid_fd);
				goto mem_error;
			}
			memset(hid_host, 0, sizeof(*hid_host));
			hid_host->fd = hid_fd;

			u32 *hid_ver = (u32*)iosAlloc(*hId, 0x20);
			if (hid_ver==NULL) goto mem_error;
			// have to call the USB4 version first, to be safe
			if (IOS_Ioctl(hid_fd, USBV4_IOCTL_GETVERSION, NULL, 0, NULL, 0)==0x40001  || \
					IOS_Ioctl(hid_fd, USBV5_IOCTL_GETVERSION, NULL, 0, hid_ver, 0x20) || hid_ver[0]!=0x50001) {
				// wrong hid version
				IOS_Close(hid_fd);
				iosFree(*hId, hid_host);
				hid_host = NULL;
			} else
				IOS_IoctlAsync(hid_fd, USBV5_IOCTL_GETDEVICECHANGE, NULL, 0, hid_host->attached_devices, 0x180, __usbv5_devicechangeCB, hid_host);

			iosFree(*hId, hid_ver);
		}
	}*/

	return IPC_OK;

//mem_error:
	//USB_Deinitialize();
	//return IPC_ENOMEM;
}

/*s32 USB_Deinitialize()
{
	if (hid_host) {
		if (hid_host->fd>=0) {
			IOS_Ioctl(hid_host->fd, USBV5_IOCTL_SHUTDOWN, NULL, 0, NULL, 0);
			IOS_Close(hid_host->fd);
		}
		iosFree(hId, hid_host);
		hid_host = NULL;
	}

	if (ven_host) {
		if (ven_host->fd>=0) {
			IOS_Ioctl(ven_host->fd, USBV5_IOCTL_SHUTDOWN, NULL, 0, NULL, 0);
			IOS_Close(ven_host->fd);
		}
		iosFree(hId, ven_host);
		ven_host = NULL;
	}

	return IPC_OK;
}*/

s32 USB_ReadIntrMsg(s32 fd,u8 bEndpoint,u16 wLength,void *rpData)
{
	return __usb_interrupt_bulk_message(fd,USBV0_IOCTL_INTRMSG,bEndpoint,wLength,rpData,NULL,NULL);
}

s32 USB_WriteIntrMsg(s32 fd,u8 bEndpoint,u16 wLength,void *rpData)
{
	return __usb_interrupt_bulk_message(fd,USBV0_IOCTL_INTRMSG,bEndpoint,wLength,rpData,NULL,NULL);
}

s32 USB_GetDeviceList(usb_device_entry *descr_buffer,u8 num_descr,u8 interface_class,u8 *cnt_descr)
{
	int i;
	u8 cntdevs=0;

	//if (ven_host==NULL) {
		s32 fd;
		u32 *buf = (u32*)iosAlloc(*hId, num_descr<<3);
		if (buf==NULL) return IPC_ENOMEM;

		fd = IOS_Open(__oh0_path,IPC_OPEN_NONE);
		if (fd<0) {
			iosFree(*hId, buf);
			return fd;
		}

		cntdevs = 0;
		
		//i = IOS_IoctlvFormat(hId,fd,USBV0_IOCTL_GETDEVLIST,"bb:dd",num_descr,interface_class,&cntdevs,sizeof(cntdevs),buf,(num_descr<<3));
		ioctlv getDeviceListIoctlv[4] = {{&num_descr, 1},
										 {&interface_class, 1},
										 {&cntdevs, sizeof(cntdevs)},
										 {buf, (num_descr<<3)}};
		i = IOS_Ioctlv(fd, USBV0_IOCTL_GETDEVLIST, 2, 2, getDeviceListIoctlv);
		
		if (cnt_descr) *cnt_descr = cntdevs;

		while (cntdevs--) {
			descr_buffer[cntdevs].device_id = 0;
			descr_buffer[cntdevs].vid = (u16)(buf[cntdevs*2+1]>>16);
			descr_buffer[cntdevs].pid = (u16)buf[cntdevs*2+1];
		}

		IOS_Close(fd);
		iosFree(*hId, buf);
		return i;
	//}

	// for ven_host, we can only exclude usb_hid class devices
	/*if (interface_class != USB_CLASS_HID && ven_host) {
		i=0;
		while (cntdevs<num_descr && ven_host->attached_devices[i].device_id) {
			descr_buffer[cntdevs++] = ven_host->attached_devices[i++];
			if (i>=32) break;
		}
	}*/

	/*if ((!interface_class || interface_class==USB_CLASS_HID) && hid_host) {
		i=0;
		while (cntdevs<num_descr && hid_host->attached_devices[i].device_id) {
			descr_buffer[cntdevs++] = hid_host->attached_devices[i++];
			if (i>32) break;
		}
	}*/

	if (cnt_descr) *cnt_descr = cntdevs;

	return IPC_OK;
}

#endif /* defined(HW_RVL) */

