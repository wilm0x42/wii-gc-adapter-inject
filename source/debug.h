#ifndef DEBUG_H
#define DEBUG_H

#ifdef BUILD_DEBUG

#define DEBUG_CHECKPOINT(which,error) debug_send(which, error)
#define DEBUG_DISPLAY(val, yoffset) debug_display(val, yoffset)


static bool debug_initDone = false;

static u32 permaValue = 0;

u32 (*SI_Transfer)(s32 chan, void *out, u32 out_len, void *in, u32 in_len, void* cb, u32 us_delay) = (void*)0x801e6de8;


void emptyCallback(s32 chan, u32 type)
{
}

void debug_send(u32 which, u32 error)
{
	if (!debug_initDone)
		return;

	char outBuf[8] ATTRIBUTE_ALIGN(32);
	
	memcpy(outBuf, &which, 4);
	memcpy(outBuf+4, &error, 4);
	
	SI_Transfer(0, outBuf, 8, NULL, 0, emptyCallback, 0);
}

#define _SHIFTL(v, s, w)	\
    ((u32) (((u32)(v) & ((0x01 << (w)) - 1)) << (s)))
#define _SHIFTR(v, s, w)	\
    ((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))

void debug_display(u32 val, int yoffset)
{
	if (!debug_initDone)
	{
		return;
	}

	u8 r = 255;
	u8 g = 255;
	u8 b = 255;
	u8 a = 255;
	
	u32 regval;
	
	u16 x = 0;
	u16 y = 0;
	u16 i = 0;

	for (i = 0; i < 32; i++)
	{
		if (val & (1 << i))
		{
			r = 255;
			g = 0;
			b = 255;
		}
		else
		{
			r = 0;
			g = 255;
			b = 0;
		}
		
		for (x = (i*8) + 10; x < (i*8)+10 + 8; x++)
		{
			for (y = yoffset+10; y < yoffset+10 + 8; y++)
			{
				regval = 0xc8000000|(_SHIFTL(x,2,10));
				regval = (regval&~0x3FF000)|(_SHIFTL(y,12,10));
				*(u32*)regval = _SHIFTL(a,24,8)|_SHIFTL(r,16,8)|_SHIFTL(g,8,8)|(b&0xff);
			}
		}
	}
}

void debug_init()
{
	// This lets the debug monitor know it's now talking
	// to this debugging code, rather than normal controller code
	char outBuf[1] ATTRIBUTE_ALIGN(32);
	outBuf[0] = 0x42;
	SI_Transfer(0, outBuf, 1, NULL, 0, emptyCallback, 0);
	
	debug_initDone = true;
}

#else
#define DEBUG_CHECKPOINT(which,error)
#define DEBUG_DISPLAY(val, yoffset)
#endif // BUILD_DEBUG

#endif