#include <iostream>
#include <Windows.h>
#include <string>

#pragma warning(disable:4996)

// FSD data that contians information about all of the assets (not the asset data itself).
unsigned char fsdData[0x28000] = {};

DWORD bytesread = 0;
DWORD bytesread_asset = 0;

// Asset count.
int items = 0;

//File handle for Hydro.fsd
HANDLE fh = NULL;

// These tables of values are used in the asset decode process.
const DWORD table1[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x2, 0x4, 0x6, 0x8, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x2, 0x4, 0x6, 0x8};
const DWORD table2[] = {0x00000007, 0x00000008, 0x00000009,
0x0000000A, 0x0000000B, 0x0000000C, 0x0000000D,
0x0000000E, 0x00000010, 0x00000011, 0x00000013,
0x00000015, 0x00000017, 0x00000019, 0x0000001C,
0x0000001F, 0x00000022, 0x00000025, 0x00000029,
0x0000002D ,0x00000032, 0x00000037, 0x0000003C,
0x00000042 ,0x00000049 ,0x00000050, 0x00000058,
0x00000061, 0x0000006B, 0x00000076, 0x00000082,
0x0000008F, 0x0000009D ,0x000000AD, 0x000000BE,
0x000000D1, 0x000000E6, 0x000000FD, 0x00000117,
0x00000133 ,0x00000151, 0x00000173, 0x00000198,
0x000001C1 ,0x000001EE ,0x00000220, 0x00000256,
0x00000292, 0x000002D4 ,0x0000031C, 0x0000036C,
0x000003C3, 0x00000424, 0x0000048E, 0x00000502,
0x00000583 ,0x00000610, 0x000006AB, 0x00000756,
0x00000812 ,0x000008E0 ,0x000009C3, 0x00000ABD,
0x00000BD0 ,0x00000CFF, 0x00000E4C, 0x00000FBA,
0x0000114C, 0x00001307, 0x000014EE, 0x00001706,
0x00001954 ,0x00001BDC, 0x00001EA5, 0x000021B6,
0x00002515, 0x000028CA, 0x00002CDF, 0x0000315B,
0x0000364B ,0x00003BB9, 0x000041B2, 0x00004844,
0x00004F7E, 0x00005771, 0x0000602F ,0x000069CE,
0x00007462, 0x00007FFF, 0x00006277 ,0x775C3A68,
0x756D7661, 0x5C636973, 0x63617274 ,0x3230256B,
0x73652E64 ,0x00000066, 0xB0ADAABA, 0xDEB4B7BC,
0x179288D6, 0xDEB2937F ,0x00000025 ,0x00000027,
0x00000026 ,0x00000028, 0x00000010, 0x00000020,
0x00000001 ,0x3F800000 ,0x3F800000, 0x3F800000,
0x00000005 ,0x00000002, 0x00000001 ,0x00000009,
0x00000006 ,0x00000002 ,0x00000004, 0x00000001,
0x00000001, 0x00000002 ,0x00000003, 0x00000004,
0x00000005, 0x00000006 ,0x00000007 ,0x00000008,
0xFFFFFFFF,
};

struct WAVHeader {
	char chunkID[4] = { 'R', 'I', 'F', 'F' };
	uint32_t chunkSize;
	char format[4] = { 'W', 'A', 'V', 'E' };
	char subchunk1ID[4] = { 'f', 'm', 't', ' ' };
	uint32_t subchunk1Size;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
	char Subchunk2ID[4] = { 'd', 'a', 't', 'a' };
	uint32_t Subchunk2Size;
};

// Asset types from hydro.fsd
// TODO: Add the rest of the assets later after analyzing and understanding what they exactly are.
enum ASSET_TYPE
{
	ESF, //Sound File.
	EGF //Image File.
};

// Use this function to get the encoded value of a string.
// Can be used later to check if an asset's encoded string matches the result you got.
DWORD Encode_String(const char* String)
{
	int counter = 0;
	unsigned char chr = NULL;
	int v3 = 0;
	DWORD ret = 0;
	DWORD chr2 = 0;

	chr = String[0];

	while (chr != NULL)
	{
		chr2 = (DWORD)chr;
		for (int i = 0; i < v3; i++)
			chr2 *= 2;
		v3 += 8;
		ret += chr2;
		if (v3 > 0x18)
			v3 = 0;
		chr = String[counter + 1];
		counter++;
	}
	return ret + counter;
}

// This function will return the index where the asset is located at.
// "Name" is the full path to the asset file with all letters capitalized.
// If the function fails, -1 will be returned. Else it returns the correct index.
int Find_Encoded_String(const char* Name)
{
	for (int i = 0; i < items - 1; ++i)
	{
		DWORD string_hash = *(DWORD*)((fsdData + 0x4) + 0x10 * i);
		if (Encode_String(Name) == string_hash)
			return i;
	}
	return -1;
}

// This function is used to decode the encoded data for the asset.
// The "Data" parameter is the encoded data.
// Specify the exact size as well for the "size" parameter.
void Decode_Asset(void* Data, int size)
{
	DWORD v1 = table2[0]; //EBP
	bool v2 = false; // EBX
	DWORD v3 = v1; // ESP+0x18
	int v4 = 0; //ESP+0x14
	char* v5 = (char*)Data; //ESP+0x1C
	char* v6 = new char[size + 1]; // ESP+0x10
	char* v6_2 = v6; // ESP+0x10
	memcpy(v6, v5, size);

	if (v2)
		v6 = (char*)Data + 1;

	int v7 = size / 2; //ESP+0x24

	if (v7 < 0)
		return;

	int v8 = 0; //ECX
	int v9 = 0; //ESI
	int v10 = 0; //EDI

	while (v7 > 0)
	{

		if (!v2)
		{
			v8 = *v6;
			v4 = *v6;
			v6++;
			v8 /= (unsigned int)pow(2, 4);
		}
		else
			v8 = v4;

		v8 &= 0xF;
		v9 += table1[v8];
		bool v2_c = v2;
		v2 = !v2;

		if (v9 < 0)
			v9 = 0;
		else if (v9 > 0x58)
			v9 = 0x58;

		int v8_c = v8; //EAX
		int v1_c = v1; //EDX
		v8 &= 7;
		v8_c &= 8;
		v1_c /= (unsigned int)pow(2, 3);

		if ((BYTE)v8 & 4)
			v1_c += v1;

		if ((BYTE)v8 & 2)
		{
			v1 /= 2;
			v1_c += v1;
			v1 = v3;
		}

		if ((BYTE)v8 & 1)
		{
			v1 /= (int)pow(2, 2);
			v1_c += v1;
		}

		if (!v8_c)
			v10 += v1_c;
		else
			v10 -= v1_c;

		if (v10 > 0x7FFF)
			v10 = 0x7FFF;
		else if (v10 < (int)0xFFFF8000)
			v10 = 0xFFFF8000;

		v1 = table2[v9];
		*(WORD*)v5 = (WORD)v10;
		v5 += 2;
		v7--;
		v3 = v1;
	}
	delete[] v6_2;
}

// This function will unpack/extract the asset in directory Name specified at index Index and which Type it should be.
// The "Name" parameter can be a full path like "C:\\Hydro Thunder\\MyFolder\\file.wav".
// Alternatively, you can choose a path from the current directory like "MyFolder\\file.wav".
// Index is the asset index, as in which one you want to extract.
// Type is what the asset type is. You can look in the enum values for a list of them. There will be comments that explains what the enum value represents.
// If the function success, it will return true. Otherwise it returns false.
bool Unpack_Asset(const char* Name, int Index, ASSET_TYPE Type)
{
	// Information about asset data:
	// Offset 0x0 = file type/asset type (ESF, EGF, ERM, EDL...)
	// EDL is also a sound file?
	// Offset 0x4 = Size
	// Actual data starts from 0x8

	char type_n_size[8] = {};
	bool ret = false;

	DWORD asset_offset = *(DWORD*)((fsdData + 0x8) + Index * 0x10); //Get the asset data offset in Hydro.fsd.
	if (SetFilePointer(fh, asset_offset, NULL, 0) != INVALID_SET_FILE_POINTER)
	{
		if (ReadFile(fh, &type_n_size, 8, &bytesread_asset, NULL))
		{
			switch (Type)
			{
			case ESF:
			{
				if (!strncmp(type_n_size, "ESF\x8", 4)) //Must be a sound file type.
				{
					DWORD size = *(DWORD*)(type_n_size + 0x4);
					size = ((size & 0xFFFFFFF));

					char* databuf = new char[size + 1];

					SetFilePointer(fh, asset_offset + 0x8, NULL, 0); //Set file offset at the actual data of the asset.
					if (ReadFile(fh, databuf, size, &bytesread_asset, NULL))
					{
						Decode_Asset(databuf, size);
						HANDLE fh2 = CreateFile(Name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						if (fh2 != INVALID_HANDLE_VALUE)
						{
							// Setup the waveform header for write.
							WAVHeader wav_header = {}; 
							wav_header.Subchunk2Size = size;
							wav_header.chunkSize = size + 36;
							wav_header.subchunk1Size = 16;
							wav_header.audioFormat = 1;
							wav_header.numChannels = 1;
							wav_header.sampleRate = 11100;
							wav_header.bitsPerSample = 16;
							wav_header.byteRate = (wav_header.sampleRate * wav_header.numChannels * wav_header.bitsPerSample) / 8;
							wav_header.blockAlign = (wav_header.numChannels * wav_header.bitsPerSample) / 8;
							// Setup the waveform header for write.

							DWORD temp = 0;
							WriteFile(fh2, &wav_header, size, &temp, NULL);
							SetFilePointer(fh2, sizeof(WAVHeader), NULL, 0);
							WriteFile(fh2, databuf, size, &temp, NULL);
							CloseHandle(fh2);
							ret = true;
						}
						else
						{
							if (GetLastError() == 3)
							{
								std::string tempbuf{ Name };

								size_t index = tempbuf.find_last_of('\\');

								if (index != tempbuf.npos)
									tempbuf.erase(index);

								CreateDirectory(tempbuf.c_str(), NULL);

								// Attempt to create the file again.
								HANDLE fh2 = CreateFile(Name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
								if (fh2 != INVALID_HANDLE_VALUE)
								{
									// Setup the waveform header for write.
									WAVHeader wav_header = {};
									wav_header.Subchunk2Size = size;
									wav_header.chunkSize = size + 36;
									wav_header.subchunk1Size = 16;
									wav_header.audioFormat = 1;
									wav_header.numChannels = 1;
									wav_header.sampleRate = 11100;
									wav_header.bitsPerSample = 16;
									wav_header.byteRate = (wav_header.sampleRate * wav_header.numChannels * wav_header.bitsPerSample) / 8;
									wav_header.blockAlign = (wav_header.numChannels * wav_header.bitsPerSample) / 8;
									// Setup the waveform header for write.

									DWORD temp = 0;
									WriteFile(fh2, &wav_header, size, &temp, NULL);
									SetFilePointer(fh2, sizeof(WAVHeader), NULL, 0);
									WriteFile(fh2, databuf, size, &temp, NULL);
									CloseHandle(fh2);
									ret = true;
								}
							}
						}
					}
					delete[] databuf;
				}
			}
			break;
			case EGF:
			{
				// TODO: 
				// Add code that properly exports the texture as an image file where an image viewer program can display it.
			}
			break;
			}

		}
	}
	return ret;
}

int main()
{
	// Read Hydro.fsd from the same directory.
	fh = CreateFile("Hydro.fsd", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh != INVALID_HANDLE_VALUE)
	{
		if (ReadFile(fh, fsdData, sizeof(fsdData), &bytesread, NULL)) // Read the file.
		{
			if (bytesread > 3 && !strncmp((char*)fsdData, "FSD\x2", 4)) // Check if the bytes read from the file is bigger than 3 and that the file is an FSD archive.
			{
				int itemcounter = 0;

				while (true)
				{
					DWORD string_hash = *(DWORD*)((fsdData + 0x4) + 0x10 * itemcounter); // Get the encoded string of the asset and check if it's not NULL.
					if (!string_hash) // If it's NULL, then we end the loop because there are no more assets.
						break;
					itemcounter++;
				}
				items = itemcounter;

				if (items > 0)
				{
					// This code was part of the original game. Not sure if it's really necessary, but it's here just in case.
					for (int i = 0; i < items - 1; i++)
					{
						DWORD string_hash = *(DWORD*)((fsdData + 0x4) + 0x10 * i);

						for (int i2 = 0; i2 < items - 1 - i; ++i2)
						{
							if (*(DWORD*)((fsdData + 0x4) + 0x10 * (i2 + i)) < string_hash)
							{
								MessageBox(NULL, "Bad String Encoding.", "ERROR", MB_ICONERROR);
								CloseHandle(fh);
								return -1;
							}
						}
					}

					int counter = 0;
					for (int i = 0; i < items - 1; ++i)
					{
						// Extract all track audios.

						char tempbuf[35] = {};
						char tempbuf2[35] = {};
						sprintf(tempbuf, "wavmusic\\track%d.wav", counter);
						sprintf(tempbuf2, "H:\\WAVMUSIC\\TRACK%d.ESF", counter);

						int in = Find_Encoded_String(tempbuf2);

						if (in != -1)
						{
							counter++;
							Unpack_Asset(tempbuf, in, ASSET_TYPE::ESF);
						}
						else
							counter++;
					}

					counter = 0;
					for (int i = 0; i < items - 1; ++i)
					{
						//Extract all sounds.

						char tempbuf[25] = {};
						char tempbuf2[25] = {};
						sprintf(tempbuf, "sound\\%d.wav", counter);
						sprintf(tempbuf2, "H:\\SOUND\\%d.ESF", counter);

						int in = Find_Encoded_String(tempbuf2);

						if (in != -1)
						{
							counter++;
							Unpack_Asset(tempbuf, in, ASSET_TYPE::ESF);
						}
						else
							counter++;
					}

				}
				else
					MessageBox(NULL, "The archive does not contain any assets.", "ERROR", MB_ICONERROR);
			}
			else
				MessageBox(NULL, "Hydro.fsd archive check failed.", "ERROR", MB_ICONERROR);
		}
		else
			MessageBox(NULL, "Failed to read data from Hydro.fsd.", "ERROR", MB_ICONERROR);

		CloseHandle(fh);
	}
	else
		MessageBox(NULL, "Failed to find/open Hydro.fsd.", "ERROR", MB_ICONERROR);
}