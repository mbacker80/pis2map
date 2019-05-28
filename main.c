#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <conio.h>

#include "head.h"

WSADATA *WsaData;

struct sockaddr_in CONSOLE_Transfer;
int CONSOLE_SOCKET = 0;

char CharToHexL(unsigned char c);
char CharToHexR(unsigned char c);
void ReverseHexStr(char *hx);
char HexChar(int c);
void StrToHex(unsigned char *from, unsigned char *to, int c);
void CopyBytes(unsigned char *from, unsigned char *to, int c);
void CopyLabelText(unsigned char *from, unsigned char *to, int c);
void ReadInputString(char *input_print, char inp[1024]);
void ExtractLabels(unsigned char *pisdata, int lstart, int lcount, int lsize, unsigned char (*labels)[255], unsigned char (*lblAddrs)[8]);
int WriteMAP(char *mapName, unsigned char (*labels)[255], unsigned char (*lblAddrs)[8], int lc);

int main(int argc, char** argv)
{
	int i = 0;
	int i2 = 0;
	FILE * fpis;
	int fpis_sz;
	int foundit = 0;
	
	int s;
	int lblCount;
	int lblSize;
	unsigned char (*labels)[255];
	unsigned char (*lblAddrs)[8];
	
	unsigned char *pisdata;
	unsigned char lblBuffer[255];
	
	
	char rtline[2];
	
	if (argc < 3)
	{
		printf("Input and output files needed.");
	}
	if (argc >= 3)
	{
		
		fpis = fopen(argv[1], "rb");
		if (fpis > 0)
		{
			
			printf("Reading [%s]...\n", argv[1]);
			//====================================== Load pis file
			fseek(fpis, 0, SEEK_END);
			fpis_sz = ftell(fpis);
			fseek(fpis, 0, SEEK_SET);
			
			pisdata = malloc(fpis_sz);
			
			i = 0;
			i2 = 0;
			while (i2 < fpis_sz)
			{
				i = fread(pisdata + i2, 1, fpis_sz - i2, fpis);
				i2 += i;
				if (i <= 0)
					break;
			}
			
			fclose(fpis);
			// 5049531F
			if (((pisdata[0] != 0x50) || (pisdata[1] != 0x49)) || ((pisdata[2] != 0x53) || (pisdata[3] != 0x1F)))
			{
				printf("File not a .pis\n");
				return -1;
			}
			//====================================== Find Labels
			printf("Scanning for label definition...\n");
			
			foundit = 0;
			i = fpis_sz - 1;
			while ((i > 0) && (foundit == 0))
			{
				if ((pisdata[i] == 0x02) && (pisdata[i+1] == 0xDB))
				{
					printf ("Found (Type1) at %08X\n", (i-6));
					foundit = i - 4;
				}
				if ((pisdata[i] == 0x02) && (pisdata[i+1] == 0xD8))
				{
					printf ("Found (Type2) at %08X\n", (i-6));
					foundit = i - 4;
				}
				i--;
			}
			if (foundit <= 0)
			{
				printf("Did not locate anything");
				free(pisdata);
				return -1;
			}
			s = foundit;
			
			lblSize = pisdata[s] + (pisdata[s+1] * 0x100) + (pisdata[s+2] * 0x10000) + (pisdata[s+3] * 0x1000000);
			s += 8;
			lblCount = pisdata[s] + (pisdata[s+1] * 0x100) + (pisdata[s+2] * 0x10000) + (pisdata[s+3] * 0x1000000);
			s += 4;
			printf("Label Count: %08X\n", lblSize, lblCount);
			
			labels = malloc(sizeof(*labels) * lblCount);
			if (labels == NULL) { printf("[1] Allocation failure...\n"); return -1; }
			lblAddrs = malloc(sizeof(*lblAddrs) * lblCount);
			if (lblAddrs == NULL) { printf("[2] Allocation failure...\n"); return -1; }
			
			ExtractLabels(pisdata, s, lblCount, lblSize, labels, lblAddrs);
			
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bExtraction complete\n");
			printf("Writing map file...\n");
			
			i = WriteMAP(argv[2], labels, lblAddrs, lblCount);
			if (i > 0)
			{
				printf("Map file saved successfully\n");
			}
			else
			{
				printf("Map file save error.\n");
			}
			
			
			free(pisdata);
			free(labels);
			free(lblAddrs);
		}
		if (fpis <= 0)
		{
			printf("Error opening %s", argv[1]);
		}
		
	}
	
	//printf("\n\nPress any key to close.");
	//getch();
	
	return 0;
}

void ExtractLabels(unsigned char *pisdata, int lstart, int lcount, int lsize, unsigned char (*labels)[255], unsigned char (*lblAddrs)[8])
{
	int i, s, lblSize, lblCount;
	
	lblSize = lsize;
	lblCount = lcount;
	s = lstart;
	
	for (i = 0; i < lblCount; i++)
	{
		StrToHex(pisdata + s, lblAddrs[i], 4);
		s += 4;
		lblSize = pisdata[s];
		s++;
		CopyLabelText(pisdata + s, labels[i], lblSize);
		s += lblSize;
		
		ReverseHexStr(lblAddrs[i]);
		
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b%i/%i", i, lblCount);
		//printf("(Label) %s: %s\n", lblAddrs[i], labels[i]);
	}
}

int WriteMAP(char *mapName, unsigned char (*labels)[255], unsigned char (*lblAddrs)[8], int lc)
{
	int i, ret;
	FILE * fmap;
	char rtline[2];
	
	rtline[0] = 0x0D;
	rtline[1] = 0x0A;
	
	fmap = fopen(mapName, "wb");
	if (fmap > 0)
	{
		ret = fwrite("# pis2map v1.0", 1, 14, fmap);
		ret = fwrite(rtline, 1, 2, fmap);
		ret = fwrite("# Created by: Gtlcpimp", 1, 22, fmap);
		ret = fwrite(rtline, 1, 2, fmap);
		
		for (i = 0; i < lc; i++)
		{
			ret = fwrite(lblAddrs[i], 1, 8, fmap);
			ret = fwrite(":", 1, 1, fmap);
			ret = fwrite(rtline, 1, 2, fmap);
			ret = fwrite(".code	", 1, 6, fmap);
			ret = fwrite(labels[i], 1, strlen(labels[i]), fmap);
			ret = fwrite(rtline, 1, 2, fmap);
		}
		
		fclose(fmap);
		return 1;
	}
	return 0;
}

void CopyBytes(unsigned char *from, unsigned char *to, int c)
{
	int i;
	for (i = 0; i < c; i++)
		to[i] = from[i];
}
void CopyLabelText(unsigned char *from, unsigned char *to, int c)
{
	int i;
	for (i = 0; i < c; i++)
		to[i] = from[i];
	to[c] = 0x00;
}

void StrToHex(unsigned char *from, unsigned char *to, int c)
{
	int i;
	int i2;
	
	
	i2 = 0;
	for (i = 0; i < c; i++)
	{
		to[i2] = CharToHexL(from[i]);
		to[i2+1] = CharToHexR(from[i]);
		
		i2 += 2;
		to[i2] = 0x00;
	}
}

char CharToHexL(unsigned char c)
{
	unsigned char b1;
	unsigned char b2;
	
	b1 = c;
	b2 = c;
	
	b1 = b1 / 0x10;
	b2 = b2 - (b1 * 0x10);
		
	return HexChar(b1);
	//to[i2+1] = HexChar(b1);
}

char CharToHexR(unsigned char c)
{
	unsigned char b1;
	unsigned char b2;
	
	b1 = c;
	b2 = c;
	
	b1 = b1 / 0x10;
	b2 = b2 - (b1 * 0x10);
		
	//to[i2] = HexChar(b2);
	return HexChar(b2);
}

char HexChar(int c)
{
	if (c == 0){ return 0x30; }
	if (c == 1){ return 0x31; }
	if (c == 2){ return 0x32; }
	if (c == 3){ return 0x33; }
	if (c == 4){ return 0x34; }
	if (c == 5){ return 0x35; }
	if (c == 6){ return 0x36; }
	if (c == 7){ return 0x37; }
	if (c == 8){ return 0x38; }
	if (c == 9){ return 0x39; }
	if (c == 10){ return 0x61; }
	if (c == 11){ return 0x62; }
	if (c == 12){ return 0x63; }
	if (c == 13){ return 0x64; }
	if (c == 14){ return 0x65; }
	if (c == 15){ return 0x66; }
}

void ReverseHexStr(char *hx)
{
	int i, n;
	int l = strlen(hx);
	char *tmp = malloc(l);
	
	n = l - 1;
	for (i = 0; i < l-1; i += 2)
	{
		tmp[i] = hx[n-1];
		tmp[i+1] = hx[n];

		n -= 2;
		tmp[i+2] = 0x00;
	}
	
	for (i = 0; i < l; i++)
		hx[i] = tmp[i];
	
	free(tmp);
}

void ReadInputString(char *input_print, char inp[1024])
{
	int i = 0;
	
	printf("%s", input_print);
	
	while (1)
	{
		char chrIn = _getch();
		
		if (chrIn == 0x0D) { break; }
		if (chrIn != 0x08)
		{
			printf("%c", chrIn);
			inp[i] = chrIn;
			i++;
		}
		else
		{
			if (i > 0)
			{
				printf("%c %c", 0x08, 0x08);
				inp[i] = 0x00;
				i--;
				inp[i] = 0x00;
			}
			else
			{
				inp[0] = 0x00;
			}
		}
	}
}







