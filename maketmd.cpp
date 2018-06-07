/*---------------------------------------------------------------------------------

maketmd.cpp -- TMD Creator for DSiWare Homebrew

Copyright (C) 2018
 Przemyslaw Skryjomski (Tuxality)

Big thanks to:
 Apache Thunder

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

---------------------------------------------------------------------------------*/

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <string>
#include <openssl/sha.h>

#ifdef _WIN32
uint32_t __bswap_32(uint32_t value) {
	return _byteswap_ulong(value);
}
#else
#include <byteswap.h>
#endif	// _WIN32

#include "version.h"

#define TMD_SIZE        0x208
#define SHA_BUFFER_SIZE 0x200

void tmd_create(uint8_t* tmd, std::fstream& app) {
	// Phase 1 - offset 0x18C (Title ID, first part)
	{
		app.seekg(0x234, app.beg);
		
		uint32_t value;
		app.read((char*)&value, 4);
		value = __bswap_32(value);
		
		memcpy(tmd + 0x18C, &value, 4);
	}
	
	// Phase 2 - offset 0x190 (Title ID, second part)
	{
		// We can take this also from 0x230, but reversed
		app.seekg(0x0C, app.beg);
		app.read((char*)&tmd[0x190], 4);
	}
	
	// Phase 3 - offset 0x198 (Group ID = '01')
	{
		app.seekg(0x10, app.beg);
		app.read((char*)&tmd[0x198], 2);
	}
	
	// Phase 4 - offset 0x1AA (fill-in 0x80 value, 0x10 times)
	{
		for(size_t i = 0; i<0x10; i++) {
			tmd[0x1AA + i] = 0x80;
		}
	}
	
	// Phase 5 - offset 0x1DE (number of contents = 1)
	{
		tmd[0x1DE] = 0x00;
		tmd[0x1DF] = 0x01;
	}
	
	// Phase 6 - offset 0x1EA (type of content = 1)
	{
		tmd[0x1EA] = 0x00;
		tmd[0x1EB] = 0x01;
	}
	
	// Phase 7 - offset, 0x1EC (file size, 8B)
	{
		app.seekg(0, app.end);
		uint32_t size = app.tellg();
		size = __bswap_32(size);
		
		// We only use 4B for size as for now
		memcpy((tmd + 0x1F0), &size, sizeof(uint32_t));
	}
	
	// Phase 8 - offset, 0x1F4 (SHA1 sum, 20B)
	{
		// Makes use of OpenSSL library
		app.seekg(0, app.beg);
		
		uint8_t buffer[SHA_BUFFER_SIZE] = { 0 };
		uint32_t buffer_read = 0;
		
		SHA_CTX ctx;
		SHA1_Init(&ctx);
		
		do {
			app.read((char*)&buffer[0], SHA_BUFFER_SIZE);
			buffer_read = app.gcount();
			
			SHA1_Update(&ctx, buffer, buffer_read);
		} while(buffer_read == SHA_BUFFER_SIZE);
		
		SHA1_Final(buffer, &ctx);
		
		// Store SHA1 sum
		memcpy((tmd + 0x1F4), buffer, SHA_DIGEST_LENGTH);
	}
}

int main(int argc, char* argv[]) {
	printf("TMD Creator for DSiWare Homebrew %s - %s\n", TMD_CREATOR_VER, TMD_CREATOR_DATE);
	printf("by Przemyslaw Skryjomski (Tuxality)\n");
	
	if(argc < 2) {
		printf("\nUsage: %s file.app <file.tmd>\n", argv[0]);
		return 1;
	}
	
	// APP file (input)
	std::fstream app(argv[1], std::ios::in | std::ios::binary);
	
	if(!app.is_open()) {
		printf("Error at opening %s for reading.\n", argv[1]);
		return 1;
	}

	// TMD file (output)
	std::string tmdPath = "title.tmd";
	if(argc > 2) {
		tmdPath = argv[2];
	}

	std::fstream tmd(tmdPath, std::ios::out | std::ios::binary);
	
	if(!tmd.is_open()) {
		printf("Error at opening %s for writing.\n", argv[2]);
		return 1;
	}
	
	// Allocate memory for TMD
	uint8_t* tmd_template = new uint8_t[TMD_SIZE](); // zeroed
	
	// Prepare TMD template then write to file
	tmd_create(tmd_template, app);
	tmd.write((const char*)(&tmd_template[0]), TMD_SIZE);
	
	// Free allocated memory for TMD
	delete[] tmd_template;
	
	// This is done in dtor, but we additionally flush tmd.
	app.close();
	tmd.flush();
	tmd.close();
	
	return 0;
}
