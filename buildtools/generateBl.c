#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define BIT(x) (1<<(x))
#define GETBIT(x, b) (x & BIT(b))


int main(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("-- generateBl by wilm -- (Built %s %s)\n"
               "Usage: generateBl <origin address> <target address> [-nolink]\n"
               " Pass -nolink to generate a b instruction, rather than bl\n", __DATE__, __TIME__);
        return 0;
    }

    const char* fromStr = argv[1];
    const char* toStr   = argv[2];
    
    bool link = true;
    
    for (int a = 1; a < argc; a++)
    {
        if (!strcmp(argv[a], "-nolink"))
            link = false;
    }
    
    unsigned int from = 0, to = 0, difference = 0, target = 0;
    unsigned int ret = 0x48000000; //Opcode (0x12)
    
    sscanf(fromStr, "%x", &from);
    sscanf(toStr, "%x", &to);
    
    if (to > from)
    {
        difference = to - from;
    }
    else
    {
        difference = from - to;
        
        difference = (~difference)+1; //Two's complement
    }
    
    target = difference;
    target &= 0x3FFFFFC; //Bits 6-29 (supposedly)
    
    ret |= target;
    if (link)
        ret |= 1; //Link bit
    
    printf("%X\n", ret);
    return 0;
}