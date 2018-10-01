#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>

int main()
{
	FILE* txt = fopen("../wii-gca-inject.txt", "r");
	if (!txt)
	{
		printf("Failed to open input file\n");
		return 1;
	}
	FILE* gct = fopen("wii-gca-inject.gct", "w");
	if (!gct)
	{
		printf("Failed to open output file\n");
		return 1;
	}
	
	char linebuf[64];
	uint32_t word1, word2;
		
	fgets(linebuf, 64, txt); // Eat the first line
	
	word1 = htonl(0x00D0C0DE);
	word2 = htonl(0x00D0C0DE);
	fwrite(&word1, 4, 1, gct);
	fwrite(&word2, 4, 1, gct);
	
	while (!feof(txt))
	{
		
		fgets(linebuf, 64, txt);
		sscanf(linebuf, "%X %X\n", &word1, &word2);
				
		word1 = htonl(word1);
		word2 = htonl(word2);
		
		fwrite(&word1, 4, 1, gct);
		fwrite(&word2, 4, 1, gct);
	}
	
	word1 = htonl(0xFF000000);
	word2 = htonl(0x00000000);
	fwrite(&word1, 4, 1, gct);
	fwrite(&word2, 4, 1, gct);
}