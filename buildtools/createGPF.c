#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char** argv)
{
    int verbose = 0;
    if (argc > 1)
    {
        if (!strcmp(argv[1], "--verbose"))
            verbose = 1;
    }
    if (!verbose) fclose(stderr);

    FILE* xml = fopen("wii-gc-adapter.xml", "r");
    
    if (!xml)
    {
    	printf("Error: Cannot open wii-gc-adapter.xml\n");
    	return 1;
    }
    
    //10KB probably wouldn't (easily) fit into memory anyway, so this should be reasonable.
    char* gpfBuf = (char*)malloc(10240);
    if (!gpfBuf)
    {
    	printf("Error: Failed to allocate buffer\n");
    	return 1;
    }
    memset(gpfBuf, 0, 10240);
    unsigned int gpfSize = 0;
    
    //1 byte for the number of patches (we fill this in later)
    gpfSize++;
    unsigned char numPatches = 0;
    
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
                
                unsigned int outvalue = htonl(value);
                unsigned int outoffset = htonl(offset);
                unsigned int outpatchsize = htonl(4);
                
                memcpy(gpfBuf+gpfSize, &outoffset, 4);
                gpfSize += 4;
                memcpy(gpfBuf+gpfSize, &outpatchsize, 4);
                gpfSize += 4;
                memcpy(gpfBuf+gpfSize, &outvalue, 4);
                gpfSize += 4;
                
                fprintf(stderr, "poke: %x = %x\n", offset, value);
                numPatches++;
            }
            if (strstr(line, "valuefile=\""))
            {
            	char filename[256];
                sscanf(strstr(line, "valuefile=\""), "valuefile=\"%s\"", filename);
                
                //For whatever reason, sscanf leaves in the trailing quote
                filename[strlen(filename)-1] = 0;
                //Also, we don't want the leading slash, if it's there
                if (filename[0] == '/')
                    memmove(filename, &filename[1], strlen(filename));
                
                FILE* fp = fopen(filename, "r");
                if (!fp)
                {
                	printf("Error: Cannot open %s\n", filename);
                	return 1;
                }
                fseek(fp, 0, SEEK_END);
                long int fsize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                
                char* fileBuf = (char*)malloc(fsize);
                if (!fileBuf)
                {
                	printf("Error: Could not allocate buffer for \"%s\"\n", filename);
                	return 1;
                }
                if (fread(fileBuf, fsize, 1, fp) <= 0)
                {
                	printf("Error Failed to read file \"%s\"\n", filename);
                	return 1;
                }
                
                unsigned int outoffset = htonl(offset);
                unsigned int outpatchsize = htonl(fsize);
                
                memcpy(gpfBuf+gpfSize, &outoffset, 4);
                gpfSize += 4;
                memcpy(gpfBuf+gpfSize, &outpatchsize, 4);
                gpfSize += 4;
                memcpy(gpfBuf+gpfSize, fileBuf, fsize);
                gpfSize += fsize;
                
                fprintf(stderr, "file: %x = %s\n", offset, filename);
                numPatches++;
            }
        }
    }
    gpfBuf[0] = numPatches;
    
    FILE* gpf = fopen("wii-gc-adapter.gpf", "w");
    if (!gpf)
    {
        printf("Error: Cannot open wii-gc-adapter.gpf for writing\n");
        return 1;
    }
    if (fwrite(gpfBuf, gpfSize, 1, gpf) <= 0)
    {
    	printf("Error: Failed to write gpf to file\n");
    	return 1;
    }
    
    fclose(xml);
    fclose(gpf);
    
    return 0;
}