#ifndef __MOVI_H__
#define __MOVI_H__


#define MAGIC_NUMBER_MOVI	(0x24564236)

#define IRAM_SIZE			(128 * 1024)

#define eFUSE_SIZE		(1 * 512)	// 512 Byte eFuse, 512 Byte reserved

#define MOVI_BLKSIZE		(1<<9) /* 512 bytes */

/* partition information */
/*
* SD/MMC memory map
	NonSecure	Secure
-----------------------------------
Raw	MBR (512)	MBR (512)
Area	BL1 (16k)	FWBL1 (8k)
	uBoot (512k)	BL2+Sig. (16k)
	reserved(16k-512)	uBoot (512k)
544k			reserved (8k-512)
---->	Env. (16k)	Env. (16k)
	Kernel (4096k)	Kernel (4096k)
16M	Ramdisk (1024k)	Ramdisk (1024k)
----->	reserved		reserved
	Partition2		Partition2
	Partition3		Partition3
	Partition4		Partition4
	Partition1		Partition1


* eMMC memory map
	NonSecure	Secure
-----------------------------------
Boot	BL1 (16k)	FWBL1 (8k)
Area	uBoot (512k)	BL2+Sig. (16k)
			uBoot (512k)

User	MBR (512)	MBR (512)
Area	reserved		reserved
---->	Env. (16k)	Env. (16k)
544k	Kernel (4096k)	Kernel (4096k)
16M	Ramdisk (1024k)	Ramdisk (1024k)
----->	reserved		reserved
	Partition2		Partition2
	Partition3		Partition3
	Partition4		Partition4
	Partition1		Partition1
*/
#define RAW_AREA_SIZE		(16 * 1024 * 1024)	// total 16MB, RAW
#define PART_SIZE_MBR		(512)			// 512KB, MBR (Master Boot Record)
#define PART_SIZE_FWBL1		(8 * 1024)		// 8KB, FWBL1
#define PART_SIZE_BL1		(16 * 1024)		// 16KB, BL1 (None Sequre)
#define PART_SIZE_BL2		(14 * 1024 + 2 * 1024)	// 16KB, BL2 + 2K signature(Secure Only)
#define PART_SIZE_UBOOT		(512 * 1024)		// 512KB, U-BOOT
#define PART_SIZE_ENV		CONFIG_ENV_SIZE		// 16KB, ENV AREA
#define PART_SIZE_KERNEL	(5 * 1024 * 1024)	// 5MB
#define PART_SIZE_ROOTFS	(RAW_AREA_SIZE -(PART_SIZE_BL1+PART_SIZE_UBOOT+(16*1024)+PART_SIZE_ENV+PART_SIZE_KERNEL))

#define MOVI_RAW_BLKCNT	(RAW_AREA_SIZE / MOVI_BLKSIZE)		/* 16MB */
#define MOVI_MBR_BLKCNT	(PART_SIZE_MBR / MOVI_BLKSIZE)		/* 512 */
#define MOVI_FWBL1_BLKCNT	(PART_SIZE_FWBL1 / MOVI_BLKSIZE)		/* 8KB, Secure */
#define MOVI_BL1_BLKCNT	(PART_SIZE_BL1 / MOVI_BLKSIZE)		/* 16KB */
#define MOVI_BL2_BLKCNT	(PART_SIZE_BL2 / MOVI_BLKSIZE)		/* 14KB + 2KB, Secure */
#define MOVI_UBOOT_BLKCNT	(PART_SIZE_UBOOT / MOVI_BLKSIZE)	/* 512KB */
#define MOVI_ENV_BLKCNT	(CONFIG_ENV_SIZE / MOVI_BLKSIZE)	/* 16KB */
#define MOVI_ZIMAGE_BLKCNT	(PART_SIZE_KERNEL / MOVI_BLKSIZE)	/* 4MB */
#define MOVI_ROOTFS_BLKCNT	(PART_SIZE_ROOTFS / MOVI_BLKSIZE)

#define MOVI_IRAM_BLKCNT	(IRAM_SIZE / MOVI_BLKSIZE)		/* 16KB */
#define MOVI_UBOOT_POS_SECURE		(MOVI_MBR_BLKCNT + MOVI_FWBL1_BLKCNT + MOVI_BL2_BLKCNT)
#define MOVI_UBOOT_POS_NONSECURE	(MOVI_MBR_BLKCNT + MOVI_BL1_BLKCNT)


/*
 *
 * start_blk: start block number for image
 * used_blk: blocks occupied by image
 * size: image size in bytes
 * attribute: attributes of image
 *            0x1: u-boot parted (BL1, non-secure)
 *            0x2: u-boot (BL2, non-secure)
 *
 *            0x0: Firmware BL1 (secure)
 *            0x3: u-boot parted (BL2, secure)
 *            0x2: u-boot (BL3, secure)
 *
 *            0x4: kernel
 *            0x8: root file system
 *            0x10: environment area
 *            0x20: reserved
 *            0xFF: user disk partition
 * description: description for image
 * by scsuh
 */
typedef struct member {
	uint start_blk;
	uint used_blk;
	u64 size;
	uint attribute; /* attribute of image */
	char description[16];
} member_t; /* 32 bytes */

/*
 * magic_number: 0x24564236
 * start_blk: start block number for raw area
 * total_blk: total block number of card
 * next_raw_area: add next raw_area structure
 * description: description for raw_area
 * image: several image that is controlled by raw_area structure
 * by scsuh
 */
typedef struct raw_area {
	uint magic_number; /* to identify itself */
	uint start_blk; /* compare with PT on coherency test */
	uint total_blk;
	uint next_raw_area; /* should be sector number */
	char description[16];
	member_t image[15];
} raw_area_t; /* 512 bytes */

#endif /*__MOVI_H__*/
