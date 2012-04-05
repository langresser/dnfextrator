// extradnf.cpp : Defines the entry point for the console application.
//
#pragma warning(disable:4996)
#include "stdafx.h"
#include <vector>

struct NImgF_Header
{
	char flag[16]; // 文件标石"Neople Img File"
	int index_size;	// 索引表大小，以字节为单位
	int unknown1;
	int unknown2;
	int index_count;// 索引表数目
};

struct NImgF_Index
{
	unsigned int dwType; //目前已知的类型有 0x0E(1555格式) 0x0F(4444格式) 0x10(8888格式) 0x11(不包含任何数据，可能是指内容同上一帧)
	unsigned int dwCompress; // 目前已知的类型有 0x06(zlib压缩) 0x05(未压缩)
	int width;        // 宽度
	int height;       // 高度
	int size;         // 压缩时size为压缩后大小，未压缩时size为转换成8888格式时占用的内存大小
	int key_x;        // X关键点，当前图片在整图中的X坐标
	int key_y;        // Y关键点，当前图片在整图中的Y坐标
	int max_width;    // 整图的宽度
	int max_height;   // 整图的高度，有此数据是为了对其精灵
};

int _tmain(int argc, _TCHAR* argv[])
{
	FILE* fp = fopen("1-dodge.img", "rb");
	NImgF_Header header;
	fread(header.flag, 16, 1, fp);
	fread(&header.index_size, 4, 1, fp);
	fread(&header.unknown1, 4, 1, fp);
	fread(&header.unknown2, 4, 1, fp);
	fread(&header.index_count, 4, 1, fp);

	std::vector<NImgF_Index> all_file_index;
	for (int i = 0; i < header.index_count; ++i) {
		NImgF_Index index;
		fread(&index.dwType, 4, 1, fp);
		fread(&index.dwCompress, 4, 1, fp);
		fread(&index.width, 4, 1, fp);
		fread(&index.height, 4, 1, fp);
		fread(&index.size, 4, 1, fp);
		fread(&index.key_x, 4, 1, fp);
		fread(&index.key_y, 4, 1, fp);
		fread(&index.max_width, 4, 1, fp);
		fread(&index.max_height, 4, 1, fp);
		all_file_index.push_back(index);
	}

	NImgF_Index _1stIndex = all_file_index.front();
	fseek(fp, header.index_size, SEEK_SET);
	char* data = new char[_1stIndex.size];
	fread(data, _1stIndex.size, 1, fp);

	FILE* fpw = fopen("111.zip", "wb");
	fwrite(data, _1stIndex.size, 1, fpw);
	fclose(fpw);
	fclose(fp);
	return 0;
}

