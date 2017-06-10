#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

int main()
{
    FILE* xml = fopen("wii-gc-adapter.xml", "r");
    FILE* gct = fopen("wii-gc-adapter.gct", "w");
    
    if (!xml)
    {
    	printf("Error: Cannot open wii-gc-adapter.xml\n");
    	return 1;
    }
    
    if (!gct)
    {
        printf("Error: Cannon open wii-gc-adapter.gct for writing\n");
        return 1;
    }
    
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
            }
        }
    }
    fclose(xml);
    fclose(gct);
}