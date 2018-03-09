#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

int main()
{
    FILE* xml = fopen("wii-gc-adapter.xml", "r");
    FILE* gct = fopen("wii-gc-adapter.gct", "w");
    FILE* txt = fopen("wii-gc-adapter-gct.txt", "w");
    
    if (!xml)
    {
    	printf("Error: Cannot open wii-gc-adapter.xml\n");
    	return 1;
    }
    
    if (!gct)
    {
        printf("Error: Cannot open wii-gc-adapter.gct for writing\n");
        return 1;
    }
    
    unsigned int code_begin = htonl(0x00D0C0DE);
    fwrite(&code_begin, 4, 1, gct);
    fwrite(&code_begin, 4, 1, gct);
    
    char line[256];
    while (!feof(xml))
    {
        fgets(line, 256, xml);
        const char* lineStart;
        if ((lineStart = strstr(line, "<memory")))
        {
            unsigned int offset;
            unsigned int value;
            
            if (!strstr(line, "offset=\""))
                continue;
            sscanf(strstr(line, "offset=\""), "offset=\"%x\"", &offset);
            
            if (strstr(line, "value=\""))
            {
                sscanf(strstr(line, "value=\""), "value=\"%x\"", &value);
                
                unsigned int geckocode = offset;
                unsigned int outvalue = htonl(value);
                
                geckocode &= 0x01FFFFFF;
                geckocode |= 0x04000000;
                geckocode = htonl(geckocode);
                
                fwrite(&geckocode, 4, 1, gct);
                fwrite(&outvalue, 4, 1, gct);
                
                fprintf(txt, "%08X %08X\n", ntohl(geckocode), value);
            }
        }
    }
    unsigned int codes_end1 = htonl(0xe0000000);//Full terminator 
    unsigned int codes_end2 = htonl(0x80008000);
    unsigned int codes_end3 = htonl(0xf0000000);//End of codes
    unsigned int codes_end4 = htonl(0x00000000);
    fwrite(&codes_end1, 4, 1, gct);
    fwrite(&codes_end2, 4, 1, gct);
    fwrite(&codes_end3, 4, 1, gct);
    fwrite(&codes_end4, 4, 1, gct);
    
    fclose(xml);
    fclose(gct);
    fclose(txt);
}
