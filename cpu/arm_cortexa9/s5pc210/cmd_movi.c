#include <common.h>

#include <command.h>
#include <movi.h>
#include <nand.h>
#include <s5pc210.h>


#define CFG_ENV_SIZE 0x200
#if defined(CONFIG_S5PC110)
#include <secure.h>
#define	SECURE_KEY_ADDRESS	(0xD0037580)
#define KERNEL_ADDRESS		(0x30008000)
extern int Check_IntegrityOfImage (
	SecureBoot_CTX	*sbContext,
	unsigned char	*osImage,
	int		osImageLen,
	unsigned char	*osSignedData,
	int		osSignedDataLen );
#endif

#ifdef DEBUG_CMD_MOVI
#define dbg(x...)       printf(x)
#else
#define dbg(x...)       do { } while (0)
#endif


extern unsigned int OmPin;

#include <mmc.h>
#include <part.h>

raw_area_t raw_area_control;
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
int init_raw_area_table (block_dev_desc_t * dev_desc)
{
	struct mmc *host = find_mmc_device(dev_desc->dev);
	
	/* when last block does not have raw_area definition. */
	if (raw_area_control.magic_number != MAGIC_NUMBER_MOVI) {
		int i = 0;
		member_t *image;
		u32 capacity;
	
		if (host->high_capacity) {
			capacity = host->capacity;
		} else {
			capacity = host->capacity * (host->read_bl_len / MOVI_BLKSIZE);			
		}		
		dev_desc->block_read(dev_desc->dev,
			capacity - MOVI_MBR_BLKCNT - 1,
			1, &raw_area_control);
		if (raw_area_control.magic_number == MAGIC_NUMBER_MOVI) {
			return 0;
		}

		dbg("Warning: cannot find the raw area table(%p) %08x\n",
			&raw_area_control, raw_area_control.magic_number);
		/* add magic number */
		raw_area_control.magic_number = MAGIC_NUMBER_MOVI;

		/* init raw_area will be 16MB */
		raw_area_control.start_blk = MOVI_RAW_BLKCNT;
		raw_area_control.total_blk = capacity;
		raw_area_control.next_raw_area = 0;
		strcpy(raw_area_control.description, "initial raw table");

		image = raw_area_control.image;

		/* image 0 should be fwbl1 */
		if (INF_REG3_REG == BOOT_EMMC441 || INF_REG3_REG == BOOT_EMMC43)
		{
			image[0].start_blk = 0;
		}
		else
		{
			image[0].start_blk = MOVI_MBR_BLKCNT;
		}
		image[0].used_blk = MOVI_FWBL1_BLKCNT;
		image[0].size = PART_SIZE_FWBL1;
		image[0].attribute = 0x0;
		strcpy(image[0].description, "fwbl1");
		dbg("fwbl1: %d\n", image[0].start_blk);

		/* image 1 should be bl1 (non-secure boot) or bl2 (secure boot) */
#if defined(CONFIG_SECURE)
		image[1].start_blk = image[0].start_blk + image[0].used_blk;
		image[1].used_blk = MOVI_BL2_BLKCNT;
		image[1].size = PART_SIZE_BL2;
		image[1].attribute = 0x3;
#else
		/* BL1 will be first image in non-secure case */
		image[1].start_blk = image[0].start_blk;
		image[1].used_blk = MOVI_BL1_BLKCNT;
		image[1].size = PART_SIZE_BL1;
		image[1].attribute = 0x1;
#endif
		strcpy(image[1].description, "u-boot parted");
		dbg("bl1: %d\n", image[1].start_blk);

		/* image 2 should be u-boot */
		image[2].start_blk = image[1].start_blk + image[1].used_blk;
		image[2].used_blk = MOVI_UBOOT_BLKCNT;
		image[2].size = PART_SIZE_UBOOT;
		image[2].attribute = 0x2;
		strcpy(image[2].description, "bootloader");
		dbg("bl2: %d\n", image[2].start_blk);

		image[3].start_blk = (544*1024)/MOVI_BLKSIZE;
		image[3].used_blk = MOVI_ENV_BLKCNT;
		image[3].size = PART_SIZE_ENV;
		image[3].attribute = 0x10;
		strcpy(image[3].description, "environment");
		dbg("env: %d\n", image[3].start_blk);

		/* image 4 should be kernel */
		image[4].start_blk = image[3].start_blk + image[3].used_blk;
		image[4].used_blk = MOVI_ZIMAGE_BLKCNT;
		image[4].size = PART_SIZE_KERNEL;
		image[4].attribute = 0x4;
		strcpy(image[4].description, "kernel");
		dbg("knl: %d\n", image[4].start_blk);

		/* image 5 should be RFS */
		image[5].start_blk = image[4].start_blk + image[4].used_blk;
		image[5].used_blk = RAW_AREA_SIZE/MOVI_BLKSIZE - image[5].start_blk;
		image[5].size = image[5].used_blk * MOVI_BLKSIZE;
		image[5].attribute = 0x8;
		strcpy(image[5].description, "ramdisk");
		dbg("rfs: %d\n", image[5].start_blk);

		/* image 6 should be disk */
		image[6].start_blk = RAW_AREA_SIZE/MOVI_BLKSIZE;
		image[6].size = capacity-RAW_AREA_SIZE;
		image[6].used_blk = image[6].size/MOVI_BLKSIZE;
		image[6].attribute = 0xff;
		strcpy(image[6].description, "disk");
		dbg("rfs: %d\n", image[5].start_blk);

		for (i=7; i<15; i++) {
			raw_area_control.image[i].start_blk = 0;
			raw_area_control.image[i].used_blk = 0;
		}
	}
}

int get_raw_area_info(char *name, u64 *start, u64 *size)
{
	int	i;
	member_t	*image;

	image = raw_area_control.image;
	for (i=0; i<15; i++) {
		if ( strncmp(image[i].description, name, 16) == 0 )
			break;
	}
	*start = image[i].start_blk * MOVI_BLKSIZE;
	*size = image[i].size;
	return (i == 15) ? -1 : i;
}

void check_emmc_open(void)
{
	char run_cmd[100];
	if (INF_REG3_REG == BOOT_EMMC441 || INF_REG3_REG == BOOT_EMMC43)
	{
		/* switch mmc to boot partition */
		sprintf(run_cmd,"emmc open 0");
		run_command(run_cmd, 0);
	}
}

void check_emmc_close(void)
{
	char run_cmd[100];
	if (INF_REG3_REG == BOOT_EMMC441 || INF_REG3_REG == BOOT_EMMC43)
	{
		/* switch mmc to boot partition */
		sprintf(run_cmd,"emmc close 0");
		run_command(run_cmd, 0);
	}
}

int do_movi(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	char *cmd;
	ulong addr, start_blk, blkcnt;
	uint rfs_size;
	char run_cmd[100];
	uint rw = 0, attribute = 0;
	int i;
	member_t *image;
	struct mmc *mmc;
	int dev_num = 0;

#if defined(CONFIG_SECURE)
	volatile u32 * pub_key;
	int secure_booting;
	ulong rv;
#endif

	cmd = argv[1];
	switch (cmd[0]) {
	case 'i':
		raw_area_control.magic_number = 0;
		run_command("mmcinfo", 0);
		return 1;
	case 'r':
		rw = 0;	/* read case */
		break;
	case 'w':
		rw = 1; /* write case */
		break;
	default:
		goto usage;
	}

	cmd = argv[2];
	switch (cmd[0]) {
#if defined(CONFIG_SECURE)
	case 'f':
		if (argc != 4)
			goto usage;
		attribute = 0x0;
		addr = simple_strtoul(argv[3], NULL, 16);
		break;
#endif
	case 'u':
		if (argc != 4)
			goto usage;
		attribute = 0x2;
		addr = simple_strtoul(argv[3], NULL, 16);
		break;
	case 'k':
		if (argc != 4)
			goto usage;
		attribute = 0x4;
		addr = simple_strtoul(argv[3], NULL, 16);
		break;
	case 'r':
		if (argc != 5)
			goto usage;
		attribute = 0x8;
		addr = simple_strtoul(argv[3], NULL, 16);
		break;
	default:
		goto usage;
	}

	mmc = find_mmc_device(dev_num);

#if defined(CONFIG_SECURE)
	/* firmware BL1 r/w */
	if (attribute == 0x0) {
		/* on write case we should write BL1 1st. */
		for (i=0, image = raw_area_control.image; i<15; i++) {
			if (image[i].attribute == attribute)
				break;
		}
		start_blk = image[i].start_blk;
		blkcnt = image[i].used_blk;
		check_emmc_open();
		printf("%s FWBL1 .. %ld, %ld ", rw ? "writing":"reading",
				start_blk, blkcnt);
		sprintf(run_cmd,"mmc %s 0 0x%lx 0x%lx 0x%lx",
				rw ? "write":"read",
				addr, start_blk, blkcnt);
		run_command(run_cmd, 0);
		check_emmc_close();
		printf("completed\n");
		return 1;
	}
#endif

	/* u-boot r/w */
	if (attribute == 0x2) {
		check_emmc_open();
		/* on write case we should write BL2 1st. */
		if (rw) {
			start_blk = raw_area_control.image[1].start_blk;
			blkcnt = raw_area_control.image[1].used_blk;
			printf("Writing BL1 to sector %ld (%ld sectors)..\n",
					start_blk, blkcnt);
			movi_write_bl1(addr);
		}
		for (i=0, image = raw_area_control.image; i<15; i++) {
			if (image[i].attribute == attribute)
				break;
		}
		start_blk = image[i].start_blk;
		blkcnt = image[i].used_blk;
		printf("%s bootloader.. %ld, %ld\n", rw ? "writing":"reading",
				start_blk, blkcnt);
#if	0
		sprintf(run_cmd,"mmc %s 0 0x%lx 0x%lx 0x%lx",
				rw ? "write":"read",
				addr, start_blk, blkcnt);
		run_command(run_cmd, 0);
#else
 		movi_write(start_blk, blkcnt, addr);

#endif
		check_emmc_close();
		printf("completed\n");
		return 1;
	}
	
	/* kernel r/w */
	if (attribute == 0x4) {
		for (i=0, image = raw_area_control.image; i<15; i++) {
			if (image[i].attribute == attribute)
				break;
		}
		start_blk = image[i].start_blk;
		blkcnt = image[i].used_blk;
		printf("%s kernel.. %ld, %ld ", rw ? "writing" : "reading",
		       start_blk, blkcnt);
		sprintf(run_cmd, "mmc %s 0 0x%lx 0x%lx 0x%lx",
			rw ? "write" : "read", addr, start_blk, blkcnt);
		run_command(run_cmd, 0);
		printf("completed\n");
		return 1;
	}

	/* root file system r/w */
	if (attribute == 0x8) {
		rfs_size = simple_strtoul(argv[4], NULL, 16);
		for (i=0, image = raw_area_control.image; i<15; i++) {
			if (image[i].attribute == attribute)
				break;
		}
		start_blk = image[i].start_blk;
		blkcnt = (rfs_size+MOVI_BLKSIZE-1)/MOVI_BLKSIZE;
		image[i].used_blk = blkcnt;
		printf("%s RFS.. %ld, %ld ", rw ? "writing":"reading",
				start_blk, blkcnt);
		sprintf(run_cmd,"mmc %s 0 0x%lx 0x%lx 0x%lx",
				rw ? "write":"read",
				addr, start_blk, blkcnt);
		run_command(run_cmd, 0);
		printf("completed\n");
		return 1;
	}

	return 1;

usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return -1;
}
U_BOOT_CMD(
	movi,	5,	0,	do_movi,
	"movi\t- sd/mmc r/w sub system for SMDK board\n",
	"init - Initialize moviNAND and show card info\n"
	"movi read  {u-boot | kernel} {addr} - Read data from sd/mmc\n"
#if defined(CONFIG_SECURE)
	"movi write {fwbl1 | u-boot | kernel} {addr} - Write data to sd/mmc\n"
#else
	"movi write {u-boot | kernel} {addr} - Write data to sd/mmc\n"
#endif
	"movi read  rootfs {addr} [bytes(hex)] - Read rootfs data from sd/mmc by size\n"
	"movi write rootfs {addr} [bytes(hex)] - Write rootfs data to sd/mmc by size\n"
	"movi read  {sector#} {bytes(hex)} {addr} - instead of this, you can use \"mmc read\"\n"
	"movi write {sector#} {bytes(hex)} {addr} - instead of this, you can use \"mmc write\"\n"
);
