#ifndef __WAVREADER_HPP__
#define __WAVREADER_HPP__

#include <fstream>
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <vector>


std::vector<float> readWAV(const char fileName[]) {
    FILE *fp = nullptr;
    char type[4];
    // typedef unsigned int uint32_t;
    uint32_t fileSize;			//Size of file in bytes - 8
	uint32_t chunk1Size;		//Size of format chunk in bytes
	uint32_t WAVsampleRate;		//Samplerate of file
	uint32_t avgBytesPerSec;	//Number of bytes per second (sampleRate * blockAlign)
	uint32_t chunk2Size;		//Size of data chunk in bytes
	short formatType;			//Format type. This program will only support 1 and 3
	short channels;				//Number of channels
	short blockAlign;			//Number of bytes in a frame (bytes per sample in ALL CHANNELS)			
	short bitsPerSample;		//Bit depth. This program will only support 8, 16, 24, and 32 bit depth  default is 16bit

    std::cout << "Begin Reading: " << fileName << std::endl;
    fp = fopen(fileName, "r");
    if (!fp) {
        std::cerr << "file open is wrong! " << std::endl;
        return {};
    }

    // Parse through format chunk
    // 解析format chunk

    fread(type, sizeof(char), 4, fp);
    if (!strcmp(type, "RIFF")){  // RIFF 文件标识
        std::cerr << "file is not RIFF! " << std::endl;
        return {};  // return empty vector
    }
    fread(&fileSize, sizeof(uint32_t), 1, fp);  // 文件总数据长度
    fread(type, sizeof(char), 4, fp);  // 文件类型标识
    if (!strcmp(type, "WAVE"))
	{        
        std::cerr << "file is not WAVE! " << std::endl;
		return {};
	}
	fread(type, sizeof(char), 4, fp);  // 格式快标识
	if (!strcmp(type, "fmt"))
	{
        std::cerr << "file is not fmt ! " << std::endl;
		return {};
	}
    fread(&chunk1Size, sizeof(uint32_t), 1, fp);  // 格式块长度
    fread(&formatType, sizeof(short), 1, fp);  // 编码格式代码，1 -> PCM
    fread(&channels, sizeof(short), 1, fp);  // 声道个数，{1，2}
    fread(&WAVsampleRate, sizeof(uint32_t), 1, fp);  // 采样率
    fread(&avgBytesPerSec, sizeof(uint32_t), 1, fp);  // 数据传输速率， channel * samplerate * bit / 8
    fread(&blockAlign, sizeof(short), 1, fp);  // 数据快对齐单位，采样帧大小， channel * bit / 8
    fread(&bitsPerSample, sizeof(short), 1, fp);  // 采样位数 bit，一般用16bit
    fread(type, sizeof(char), 4, fp);
    if (!strcmp(type, "data")) {
        std::cerr << "file is not data ! " << std::endl;
		return {};
    }
    fread(&chunk2Size, sizeof(uint32_t), 1, fp);  // data size

    // bit 是一位，byte是一字
    int bytesPerSample = bitsPerSample / 8;  // 每个sample有多少个byte
    int samples_count = (chunk2Size) / (blockAlign);

	std::cout << "Chunk 1 Size: " << chunk1Size << std::endl;
	std::cout << "Format Type: " << formatType << std::endl;
	std::cout << "Channels: " << channels << std::endl;
	std::cout << "Sample Rate: " << WAVsampleRate << std::endl;
	std::cout << "Byte Rate: " << avgBytesPerSec << std::endl;
	std::cout << "Block Align: " << blockAlign << std::endl;
	std::cout << "Bits Per Sample: " << bitsPerSample << std::endl;


	std::cout << "Chunk 2 Size: " << chunk2Size << std::endl;
	std::cout << "Samples Count: " << samples_count << std::endl;

    // Parse through data chunk
    // 解码数据部分

    std::vector<float> table;  // 这个看位数 ？ TODO
    // temp param
    float fSamp;
    uint8_t bit8;
    int16_t bit16;
    float bit32;

    if (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 32) {
        std::cout << "Unsupported read bit depth: " << bitsPerSample << std::endl;
            return {};
    }

    // table.push_back(channels); // 返回数据第一位是channel （1）
    if (formatType != 1) {
    std::cerr << "file is not PCM! TODO: Parse through no-PCM wav" << std::endl;
    return {};
    }

    for (int index = 0; index < (channels * samples_count); index++)
		{
			if (bitsPerSample == 8)
			{
				fread(&bit8, bytesPerSample, 1, fp);
				fSamp = ((float)bit8 - 127.5f) / (float)127.5;  // MAX of 2 ^ 8 / 2
				if (fSamp > 1) fSamp = 1;					
				if (fSamp < -1) fSamp = -1;
			}
			else if (bitsPerSample == 16)
			{
				fread(&bit16, bytesPerSample, 1, fp);
				fSamp = ((float)bit16) / (float)32768;  // max 2 ^ 16 / 2
				if (fSamp > 1) fSamp = 1;
				if (fSamp < -1) fSamp = -1;
			}
			else if (bitsPerSample == 32)  // TODO：
			{
				fread(&bit32, bytesPerSample, 1, fp);  // -> 16进制
				fSamp = bit32;
			}
            table.push_back(fSamp);
        }
    fclose(fp);
    std::cout << "Finished" << std::endl;
    return table;
}

#endif