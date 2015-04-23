#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

void usage(char* file)
{
    printf("Usage: %s input-file output-file\n", file);
}

int get_filesize(char* file)
{
    FILE* fp = fopen(file, "rb");
    int size = 0;

    if (fp) {
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fclose(fp);
    }

    return size;
}

int write_bl1(char* srcfile, FILE* dstfp)
{
    int i;
    unsigned long checksum = 0;
    unsigned char *src;
    int size = 0;
    FILE* srcfp;

    size = get_filesize(srcfile);

    src = (unsigned char*)malloc(size);
    memset(src, 0, size);

    srcfp = fopen(srcfile, "rb");
    for (i = 0 ; i < 16368 ; i++) {
        fread(&src[i+16], 1, 1, srcfp);
        checksum += src[i+16];
    }
    fclose(srcfp);

    *(unsigned long*)src = 0x1F;
    *(unsigned long*)(src + 4) = checksum;

	src[ 0] ^= 0x53;
	src[ 1] ^= 0x35;
	src[ 2] ^= 0x50;
	src[ 3] ^= 0x43;
	src[ 4] ^= 0x32;
	src[ 5] ^= 0x31;
	src[ 6] ^= 0x30;
	src[ 7] ^= 0x20;
	src[ 8] ^= 0x48;
	src[ 9] ^= 0x45;
	src[10] ^= 0x41;
	src[11] ^= 0x44;
	src[12] ^= 0x45;
	src[13] ^= 0x52;
	src[14] ^= 0x20;
	src[15] ^= 0x20;

	for (i=1;i<16;i++) {
		src[i] ^= src[i-1];
	}

    fwrite(src, 1, 16384, dstfp);
    return 0;
}

int main(int argc, char* argv[])
{
    FILE* dstfp;

    if (argc != 3 || access(argv[1], F_OK)) {
        usage(argv[0]);
        return 0;
    }

    dstfp = fopen(argv[2], "wb");
    if (dstfp) {
        write_bl1(argv[1], dstfp);
        fclose(dstfp);
    }

    return 0;
}

