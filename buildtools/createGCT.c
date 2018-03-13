#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

int main()
{
    FILE* xml = fopen("wii-gc-adapter.xml", "r");
    FILE* txt = fopen("wii-gc-adapter-gct.txt", "w");
    
    if (!xml)
    {
    	printf("Error: Cannot open wii-gc-adapter.xml\n");
    	return 1;
    }
    
    if (!txt)
    {
        printf("Error: Cannot open wii-gc-adapter-gct.txt for writing\n");
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
                
                geckocode &= 0x01FFFFFF;
                geckocode |= 0x04000000;
                geckocode = htonl(geckocode);
                
                fprintf(txt, "%08X %08X\n", ntohl(geckocode), value);
            }
            if (strstr(line, "valuefile=\""))
            {
            	char filenameIn[80];
                sscanf(strstr(line, "valuefile=\""), "valuefile=\"%s\"", filenameIn);
                
                filenameIn[strlen(filenameIn)-1] = 0; // Remove trailing "
                char* filename = filenameIn+1; // Add 1 to filename to avoid the leading /
                
                FILE* inFile = fopen(filename, "r");
                if (!inFile)
                {
                	printf("Error: Could not open file %s\n", filename);
                	continue;
                }
                
                fseek(inFile, 0, SEEK_END);
                int fsize = ftell(inFile);
                fseek(inFile, 0, SEEK_SET);
                
                if (fsize % 4 != 0)
                {
                	printf("Error: size of file %s wasn't a multiple of 4\n", filename);
                	fclose(inFile);
                	continue;
                }
                
                char* fileBuf = (char*)malloc(fsize);
                if (!fileBuf)
                {
                	printf("Error: couldn't allocate memory for %s\n", filename);
                	fclose(inFile);
                }
                
                memset(fileBuf, 0, fsize);
                fread(fileBuf, 1, fsize, inFile);
                
                unsigned int geckocode = offset;
                unsigned int outsize = htonl(fsize);
                
                geckocode &= 0x01FFFFFF;
                geckocode |= 0x06000000;
                geckocode = htonl(geckocode);
                
                fprintf(txt, "%08X %08X\n", ntohl(geckocode), ntohl(outsize));
                int n;
                for (n = 0; n < fsize; n += 8)
                {
                	fprintf(txt, "%08X %08X\n", htonl(*(unsigned int*)(fileBuf+n)), htonl(*(unsigned int*)(fileBuf+n+4)));
                }
                
                fclose(inFile);
                free(fileBuf);
            }
        }
    }
    
    fclose(xml);
    fclose(txt);
}
