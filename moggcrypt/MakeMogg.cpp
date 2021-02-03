// moggcrypt-cpp.cpp : Defines the entry point for the console application.

#include <iostream>
#include <cstring>
#include <fstream>

#include "VorbisEncrypter.h"
#include "OggMap.h"
#include "CCallbacks.h"

#define VERSION "1.0.0"

int usage(const char* name) {
	char* usage = "Usage: \n"
    "To encrypt a mogg: %s <input_mogg> -e <output_encrypted_mogg>\n"
    "To create a mogg from an ogg: %s <input_ogg> -m <output_mogg>\n"
    "To do both: %s <input_ogg> -em <output_encrypted_mogg>\n"
		"\n\nVersion " VERSION "\n";
	printf(usage, name, name, name);
	return 0;
}

int fail(const char* msg) {
	printf("Failed!\n\nMessage: %s\n", msg);
	return 1;
}

int encryptOgg(const char* in, const char* out) {
	std::ifstream infile(in, std::ios::in | std::ios::binary);
	if (!infile.is_open()) {
		return fail("Could not open input file");
	}

	std::ofstream outfile(out, std::ios::out | std::ios::binary);
	if (!outfile.is_open()) {
		return fail("Could not open output file");
	}

	try {
		VorbisEncrypter ve(&infile, cppCallbacks);
		char buf[8192];
		size_t read = 0;
		do {
			read = ve.ReadRaw(buf, 1, 8192);
			outfile.write(buf, read);
		} while (read != 0);
	} catch(std::exception& e) {
		printf("Error: %s", e.what());
		return 1;
	}

	return 0;
}

int mapAndEncryptOgg(const char* in, const char* out) {
	std::ifstream infile(in, std::ios::in | std::ios::binary);
	if (!infile.is_open()) {
		return fail("Could not open input file");
	}

	std::ofstream outfile(out, std::ios::out | std::ios::binary);
	if (!outfile.is_open()) {
		return fail("Could not open output file");
	}
	try {
		VorbisEncrypter ve(&infile, 0x10, cppCallbacks);
		char buf[8192];
		size_t read = 0;
		do {
			read = ve.ReadRaw(buf, 1, 8192);
			outfile.write(buf, read);
		} while (read != 0);
	} catch(std::exception& e) {
		printf("Error: %s", e.what());
		return 1;
	}
	return 0;
}


int mapOgg(const char* in, const char* out)
{
	int ret = 0;
	std::ifstream infile(in, std::ios::in | std::ios::binary);
	if (!infile.is_open()) {
		return fail("Could not open input file");
	}

	std::ofstream outfile(out, std::ios::out | std::ios::binary);
	if (!outfile.is_open()) {
		return fail("Could not open output file");
	}

	auto result = OggMap::Create(&infile, cppCallbacks);
	if (std::holds_alternative<std::string>(result)) {
		printf("Error creating OggMap\n%s\n", std::get<std::string>(result).c_str());
		return 1;
	}

	auto& map = std::get<OggMap>(result);
	auto mapData = map.Serialize();

	int oggVersion = 0xA;
	int fileOffset = 8 + mapData.size();
	outfile.write((char*)&oggVersion, sizeof(int));
	outfile.write((char*)&fileOffset, sizeof(int));
	outfile.write(mapData.data(), mapData.size());

	// Copy the audio data
	infile.clear();
	infile.seekg(0);
	size_t read = 0;
	char copyBuf[8192];
	do {
		infile.read(copyBuf, 8192);
		outfile.write(copyBuf, infile.gcount());
	} while (infile.gcount() > 0);

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc != 4 || argv[2][0] != '-') {
		return usage(argv[0]);
	}

	char* infilename = argv[1];
	char* mode = argv[2];
	char* outfilename = argv[3];

	if (!strcmp(mode, "-em")) {
		return mapAndEncryptOgg(infilename, outfilename);
	}

	switch (mode[1]) {
	case 'e':
		return encryptOgg(infilename, outfilename);
		break;
	case 'm':
		return mapOgg(infilename, outfilename);
		break;
	default:
		return usage(argv[0]);
	}
}

