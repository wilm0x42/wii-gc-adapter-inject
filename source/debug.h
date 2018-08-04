#ifndef DEBUG_H
#define DEBUG_H

#ifdef BUILD_DEBUG

#define DEBUG_CHECKPOINT(which,error) debug_send(which, error)


u32 (*SI_Transfer)(s32 chan, void *out, u32 out_len, void *in, u32 in_len, void* cb, u32 us_delay) = (void*)0x801e6de8;


void emptyCallback(s32 chan, u32 type)
{
}

void debug_send(u32 which, u32 error)
{
	char outBuf[8];
	
	memcpy(outBuf, &which, 4);
	memcpy(outBuf+4, &error, 4);
	
	SI_Transfer(0, outBuf, 8, NULL, 0, emptyCallback, 0);
}

#else
#define DEBUG_CHECKPOINT(which,error)
#endif // BUILD_DEBUG

#endif