// extradnf.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <vector>
#include <boost/filesystem.hpp>

#include "zlib/zlib.h"
#pragma comment(lib, "zlib.lib")

#include "libpng/png.h"

void extract_npk(const char* file, bool only_img);
void extract_img(const char* file, const char* npk_file);
void extract_img_npk(FILE* fp, int offset, const char* file, const char* npk_file);
void extract_image(const char* file);
void convert_to_png(const char* file_name, int width, int height, int type, unsigned char* data, int size);

#pragma warning(disable:4996)

#define ARGB_1555 0x0e
#define ARGB_4444 0x0f
#define ARGB_8888 0x10
#define ARGB_NONE 0x11

#define COMPRESS_ZLIB 0x06
#define COMPRESS_NONE 0x05

struct NPK_Header
{
    char flag[16]; // 文件标识 "NeoplePack_Bill"
    int count;     // 包内文件的数目
};

struct NPK_Index
{
    unsigned int offset;  // 文件的包内偏移量
    unsigned int size;    // 文件的大小
    char name[256];// 文件名
};

char decord_flag[256] = "puchikon@neople dungeon and fighter DNF";

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
	int max_height;   // 整图的高度，有此数据是为了对齐精灵
};

int _tmain(int argc, _TCHAR* argv[])
{
	// 构造解密文件名用的flag
	int len = strlen(decord_flag);
	for (int i = len; i < 256; ++i) {
		
		if ((i - len) % 3 == 0) {
			decord_flag[i] = 'D';
		} else if ((i - len) % 3 == 1) {
			decord_flag[i] = 'N';
		} else if ((i - len) % 3 == 2) {
			decord_flag[i] = 'F';
		}
	}
	decord_flag[sizeof(decord_flag) - 1] = 0;

	boost::filesystem3::path fsPath("npk" ,boost::filesystem3::native);

    boost::filesystem3::recursive_directory_iterator beg_iter(fsPath);
    boost::filesystem3::recursive_directory_iterator end_iter;
    for (; beg_iter != end_iter; ++beg_iter) {
        if (boost::filesystem3::is_directory(*beg_iter)) {
            continue;
        } else {
			extract_npk(beg_iter->path().string().c_str(), false);
        }
    }

	system("PAUSE");
	return 0;
}

void extract_npk(const char* file, bool only_img)
{
	printf("extacting %s...\n", file);
	FILE* fp = fopen(file, "rb");
	if (!fp) {
		printf("error %s not found!!", file);
	}
	NPK_Header header;
	fread(header.flag, 16, 1, fp);
	fread(&header.count, 4, 1, fp);

	std::vector<NPK_Index> all_file_index;
	for (int i = 0; i < header.count; ++i) {
		NPK_Index index;
		fread(&index.offset, 4, 1, fp);
		fread(&index.size, 4, 1, fp);

		char temp[256] = {0};
		fread(temp, 256, 1, fp);
		for (int i = 0; i < 256; ++i) {
			index.name[i] = temp[i] ^ decord_flag[i];
		}
		all_file_index.push_back(index);
	}

	for (std::vector<NPK_Index>::const_iterator itr = all_file_index.begin();
		itr != all_file_index.end(); ++itr ) {
		NPK_Index index = *itr;

		fseek(fp, index.offset, SEEK_SET);

		if (only_img) {
			char* temp = new char[index.size];
			memset(temp, 0, index.size);
			fread(temp, index.size, 1, fp);

			// 创建文件夹
			boost::filesystem3::path file_path(index.name);
			file_path.remove_filename();
			boost::filesystem3::create_directories(file_path);

			// 写img文件
			FILE* fpw = fopen(index.name, "wb");
			fwrite(temp, index.size, 1, fpw);
			fclose(fpw);
			delete[] temp;
		} else {
			// 解压img文件
			extract_img_npk(fp, index.offset, index.name, file);
		}
	}

	fclose(fp);
}

void test_zlib()
{
		// test zlib
	 //原始数据 
     const unsigned char strSrc[]="hello world!/n/\
aaaaa bbbbb ccccc ddddd aaaaa bbbbb ccccc ddddd中文测试 中文测试/\
aaaaa bbbbb ccccc ddddd aaaaa bbbbb ccccc ddddd中文测试 中文测试/\
aaaaa bbbbb ccccc ddddd aaaaa bbbbb ccccc ddddd中文测试 中文测试/\
aaaaa bbbbb ccccc ddddd aaaaa bbbbb ccccc ddddd中文测试 中文测试";

     unsigned char buf[1024]={0},strDst[1024]={0};
     unsigned long srcLen=sizeof (strSrc),bufLen=sizeof (buf),dstLen=sizeof (strDst);

     //压缩 
      compress(buf,&bufLen,strSrc,srcLen);
      printf("After Compressed Length:%d\n",bufLen);
     //解压缩 
      int xrxr = uncompress(strDst,&dstLen,buf,bufLen);
      printf("UnCompressed String:%s\n",strDst);
}

// 传入一个文件指针，进行解压，可以是npk文件，offset为img文件在npk文件中的偏移
// 如果传入的是img文件，那么偏移为0，最后的file参数仅用于记录log和创建文件夹。与文件解压无关
void extract_img_npk(FILE* fp, int offset, const char* file, const char* npk_file)
{
	printf("        extacting %s...\n", file);
	NImgF_Header header;
	fread(header.flag, 16, 1, fp);
	fread(&header.index_size, 4, 1, fp);
	fread(&header.unknown1, 4, 1, fp);
	fread(&header.unknown2, 4, 1, fp);
	fread(&header.index_count, 4, 1, fp);
	if (stricmp(header.flag, "Neople Img File") != 0) {
		printf("error flag %s  in file %s!!", header.flag, file);
		return;
	}

	std::vector<NImgF_Index> all_file_index;
	for (int i = 0; i < header.index_count; ++i) {
		NImgF_Index index;
		fread(&index.dwType, 4, 1, fp);
		fread(&index.dwCompress, 4, 1, fp);

		// 占位文件只包含这两个数据
		if (index.dwType == ARGB_NONE) {
			all_file_index.push_back(index);
			continue;
		}

		fread(&index.width, 4, 1, fp);
		fread(&index.height, 4, 1, fp);
		fread(&index.size, 4, 1, fp);
		fread(&index.key_x, 4, 1, fp);
		fread(&index.key_y, 4, 1, fp);
		fread(&index.max_width, 4, 1, fp);
		fread(&index.max_height, 4, 1, fp);
		all_file_index.push_back(index);
	}

	const int buffer_size = 1024 * 1024 * 3;
	unsigned char* temp_file_data = new unsigned char[buffer_size];
	unsigned char* temp_zlib_data = new unsigned char[buffer_size];
	
	// 跳过文件头和索引表
	fseek(fp, offset + header.index_size + 32, SEEK_SET);

	for (std::vector<NImgF_Index>::const_iterator itr = all_file_index.begin();
		itr != all_file_index.end(); ++itr) {
		NImgF_Index index = *itr;
	//	printf("%d  type:%d   compress:%d \n", itr - all_file_index.begin(), index.dwType, index.dwCompress);

		// 占位文件，无实际资源
		if (index.dwType == ARGB_NONE) {
			continue;
		}

		int size = index.size;

		// 未压缩情况下，index.size代表的是转换为8888所占内存大小。实际读取数据要根据当前格式进行转换
		if (index.dwCompress == COMPRESS_NONE) {
			if (index.dwType == ARGB_8888) {
				size = index.size;
			} else if (index.dwType == ARGB_1555 || index.dwType == ARGB_4444) {
				size = index.size / 2;
			}
		}

		memset(temp_file_data, 0, buffer_size);
		fread(temp_file_data, size, 1, fp);

		unsigned long zlib_len = buffer_size;

		memset(temp_zlib_data, 0, buffer_size);

		if (index.dwCompress == COMPRESS_ZLIB) {
			// zlib压缩
			int ret = uncompress(temp_zlib_data, &zlib_len, temp_file_data, size); //uncompress(temp_zlib_data, &zlib_len, temp, index.size);
			if (ret != Z_OK) {
				printf("compress %s %d error!\n", file, itr - all_file_index.begin());
				continue;;
			}
		} else if (index.dwCompress == COMPRESS_NONE) {
			// 未压缩
			memcpy(temp_zlib_data, temp_file_data, size);
		} else {
			printf("error unkown compress type: %d  in file %s.\n", index.dwCompress, file);
		}	

		// 创建文件夹
		std::string file_path_noextern(file);
		file_path_noextern = file_path_noextern.substr(0, file_path_noextern.find_last_of('.'));
		boost::filesystem3::path file_path(file_path_noextern);
		boost::filesystem3::create_directories(file_path);

		char file_name[256] = {0};
		std::string file_name_last = file_path.filename().string();
		_snprintf(file_name, sizeof(file_name) - 1, "%s/%s_%d.png", file_path_noextern.c_str(), file_name_last.c_str(), itr - all_file_index.begin());
		convert_to_png(file_name, index.width, index.height, index.dwType, temp_zlib_data, zlib_len);

		_snprintf(file_name, sizeof(file_name) - 1, "%s/%s_%d.txt", file_path_noextern.c_str(), file_name_last.c_str(), itr - all_file_index.begin());
		FILE* fpinfo = fopen(file_name, "w");
		char info_line[256] = {0};
		_snprintf(info_line, sizeof(info_line) - 1, "npk=%s\nimg=%s\n", npk_file, file);
		fputs(info_line, fpinfo);
		_snprintf(info_line, sizeof(info_line) - 1, "x=%d\ny=%d\n", index.key_x, index.key_y);
		fputs(info_line, fpinfo);
		_snprintf(info_line, sizeof(info_line) - 1, "width=%d\nheight=%d\n", index.width, index.height);
		fputs(info_line, fpinfo);
		_snprintf(info_line, sizeof(info_line) - 1, "max_width=%d\nmax_height=%d\n", index.max_width, index.max_height);
		fputs(info_line, fpinfo);

		fclose(fpinfo);
	}

	delete temp_file_data;
	delete temp_zlib_data;
}

void extract_img(const char* file, const char* npk_file)
{
	FILE* fp = fopen(file, "rb");
	if (!fp) {
		printf("error %s not found!!", file);
	}
	
	extract_img_npk(fp, 0, file, npk_file);
	fclose(fp);
}

void convert_to_png(const char* file_name, int width, int height, int type, unsigned char* data, int size)
{
	png_structp png_ptr;  
    png_infop info_ptr;   
    png_bytep * row_pointers;  

    /* create file */  
    FILE *fp = fopen(file_name, "wb");  
    if (!fp) {  
        printf("[write_png_file] File %s could not be opened for writing", file_name);  
        return;  
    } 

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);  
  
    if (!png_ptr) {  
        printf("[write_png_file] png_create_write_struct failed");  
        return;  
    }

	info_ptr = png_create_info_struct(png_ptr);  
    if (!info_ptr)  
    {  
        printf("[write_png_file] png_create_info_struct failed");  
        return;  
    }  
    if (setjmp(png_jmpbuf(png_ptr)))  
    {  
        printf("[write_png_file] Error during init_io");  
        return;  
    }  
    png_init_io(png_ptr, fp);  

    /* write header */  
    if (setjmp(png_jmpbuf(png_ptr)))  
    {  
        printf("[write_png_file] Error during writing header");  
        return;  
    }  

    png_set_IHDR(png_ptr, info_ptr, width, height,  
        8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,  
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);  
  
    png_write_info(png_ptr, info_ptr);  
  
    /* write bytes */  
    if (setjmp(png_jmpbuf(png_ptr)))  
    {  
        printf("[write_png_file] Error during writing bytes");  
        return;  
    }

    row_pointers = (png_bytep*)malloc(height*sizeof(png_bytep));  
    for(int i = 0; i < height; i++)  
    {  
        row_pointers[i] = (png_bytep)malloc(sizeof(unsigned char)* 4 * width);  
        for(int j = 0; j < width; ++j)  
        {  
           
			// png is rgba
			switch (type)
			{
				case ARGB_1555://1555
					row_pointers[i][j * 4 + 0] = ((data[i * width * 2 + j * 2 + 1] & 127) >> 2) << 3;   // red  
					row_pointers[i][j * 4 + 1] = (((data[i * width * 2 + j * 2 + 1] & 0x0003) << 3) | ((data[i * width * 2 + j * 2] >> 5) & 0x0007)) << 3; // green  
					row_pointers[i][j * 4 + 2] = (data[i * width * 2 + j * 2] & 0x003f) << 3; // blue 
					row_pointers[i][j * 4 + 3] = (data[i * width * 2 + j * 2 + 1] >> 7) == 0 ? 0 : 255; // alpha
					break;
				case ARGB_4444://4444
					row_pointers[i][j * 4 + 0] = (data[i * width * 2 + j * 2 + 1] & 0x0f) << 4;   // red  
					row_pointers[i][j * 4 + 1] = ((data[i * width * 2 + j * 2 + 0] & 0xf0) >> 4) << 4; // green  
					row_pointers[i][j * 4 + 2] = (data[i * width * 2 + j * 2 + 0] & 0x0f) << 4;; // blue  
					row_pointers[i][j * 4 + 3] = ((data[i * width * 2 + j * 2 + 1] & 0xf0) >> 4) << 4; // alpha
					break;
				case ARGB_8888://8888
					row_pointers[i][j * 4 + 0] = data[i * width * 4 + j * 4 + 2]; // red
					row_pointers[i][j * 4 + 1] = data[i * width * 4 + j * 4 + 1]; // green
					row_pointers[i][j * 4 + 2] = data[i * width * 4 + j * 4 + 0]; // blue
					row_pointers[i][j * 4 + 3] = data[i * width * 4 + j * 4 + 3]; // alpha
					break;
				case ARGB_NONE:// 占位，无图片资源
					break;
				default:
					printf("error known type:%d\n", type);
					break;
			}
        }  
    }  
    png_write_image(png_ptr, row_pointers);  
  
    /* end write */  
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("[write_png_file] Error during end of write");  
        return;  
    }  
    png_write_end(png_ptr, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);
  
    /* cleanup heap allocation */  
    for (int j=0; j < height; j++)  
        free(row_pointers[j]);  
    free(row_pointers);  
  
    fclose(fp);  
}