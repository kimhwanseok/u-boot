#include <common.h>
#include <command.h>
#include <movi.h>
#include <mmc.h>
#include <part.h>


#ifdef DEBUG_CMD_MOVI
#define dbg(x...)	printf(x)
#else
#define dbg(x...)	do { } while (0)
#endif

extern unsigned int OmPin;

raw_area_t raw_area_control;

int init_raw_area_table (block_dev_desc_t * dev_desc)
{
	struct mmc *host = find_mmc_device(dev_desc->dev);
	
	/* when last block does not have raw_area definition. */
	if (raw_area_control.magic_number != MAGIC_NUMBER_MOVI) {
		int  i = 0;
		member_t *image;
		u32 capacity;

		if(host->high_capacity) {
			capacity = host->capacity;
			}
		else {
			capacity = host->capacity * (host->read_bl_len / MOVI_BLKSIZE);
		}

		dbg("Warning: cannot find the raw area table(%p) %08x\n",
			&raw_area_control, raw_area_control.magic_number);
		
		/* add magic number */
		raw_area_control.magic_number = MAGIC_NUMBER_MOVI;

		/* init raw_area will be 16MB */
		raw_area_control.start_blk = 16*1024*1024/MOVI_BLKSIZE;
		raw_area_control.total_blk = capacity;
		raw_area_control.next_raw_area = 0;
		strcpy(raw_area_control.description, "initial raw table");

		image = raw_area_control.image;

		/* image 1 should be bl1 */
		image[1].start_blk = (eFUSE_SIZE/MOVI_BLKSIZE);
		image[1].used_blk = MOVI_BL1_BLKCNT;
		image[1].size = SS_SIZE;
		image[1].attribute = 0x1;
		strcpy(image[1].description, "u-boot parted");
		dbg("bl1: %d\n", image[1].start_blk);

		/* image 2 should be environment */
		image[2].start_blk = image[1].start_blk + MOVI_BL1_BLKCNT;
		image[2].used_blk = MOVI_ENV_BLKCNT;
		image[2].size = CONFIG_ENV_SIZE;
		image[2].attribute = 0x10;
		strcpy(image[2].description, "environment");
		dbg("env: %d\n", image[2].start_blk);

		if (OmPin == BOOT_EMMC441 || OmPin == BOOT_EMMC43) {
			/* image 3 should be u-boot */
			image[3].start_blk = 0;
#ifdef CONFIG_SECURE
			image[3].used_blk = MOVI_UBOOT_BLKCNT + MOVI_BL1_BLKCNT 
								+ MOVI_BL1_BLKCNT;
			image[3].size = PART_SIZE_UBOOT + PART_SIZE_BL1 + PART_SIZE_BL1;
#else
			image[3].used_blk = MOVI_UBOOT_BLKCNT + MOVI_BL1_BLKCNT;
			image[3].size = PART_SIZE_UBOOT + PART_SIZE_BL1;
#endif
			image[3].size = PART_SIZE_UBOOT + PART_SIZE_BL1;
			image[3].attribute = 0x2;
			strcpy(image[3].description, "u-boot");
			dbg("bl2: %d\n", image[3].start_blk);
		} else {
			/* image 3 should be u-boot */
			image[3].start_blk = image[2].start_blk + MOVI_ENV_BLKCNT;
			image[3].used_blk = MOVI_BL2_BLKCNT;
			image[3].size = PART_SIZE_BL;
			image[3].attribute = 0x2;
			strcpy(image[3].description, "u-boot");
			dbg("bl2: %d\n", image[3].start_blk);
		}

		/* image 4 should be kernel */
		image[4].start_blk = image[3].start_blk + MOVI_BL2_BLKCNT;
		image[4].used_blk = MOVI_ZIMAGE_BLKCNT;
		image[4].size = PART_SIZE_KERNEL;
		image[4].attribute = 0x4;
		strcpy(image[4].description, "kernel");
		dbg("knl: %d\n", image[4].start_blk);

		/* image 5 should be RFS */
		image[5].start_blk = image[4].start_blk + MOVI_ZIMAGE_BLKCNT;
		image[5].used_blk = MOVI_ROOTFS_BLKCNT;
		image[5].size = PART_SIZE_ROOTFS;
		image[5].attribute = 0x8;
		strcpy(image[5].description, "rfs");
		dbg("rfs: %d\n", image[5].start_blk);

		for (i=6; i<15; i++) {
			raw_area_control.image[i].start_blk = 0;
			raw_area_control.image[i].used_blk = 0;
		}
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
	
	case 'f':
		if (argc != 4)
			goto usage;
		attribute = 0x0;
		addr = simple_strtoul(argv[3], NULL, 16);
		break;
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
	
	mmc = find_mmc_device(0);

//	init_raw_area_table();

	/* firmware BL1 r/w */
	if (attribute == 0x0) {
		/* on write case we should write BL1 1st. */
		for (i=0, image = raw_area_control.image; i<15; i++) {
			if (image[i].attribute == attribute)
				break;
		}
		start_blk = image[i].start_blk;
		blkcnt = image[i].used_blk;
		printf("%s FWBL1 .. %ld, %ld ", rw ? "writing":"reading",
				start_blk, blkcnt);
		sprintf(run_cmd,"mmc %s 0 0x%lx 0x%lx 0x%lx",
				rw ? "write":"read",
				addr, start_blk, blkcnt);
		run_command(run_cmd, 0);
		printf("completed\n");
		return 1;
	}
	
	/* u-boot r/w */
	if (attribute == 0x2) {
#if defined (CONFIG_EMMC)
		/* on write case we should write BL2 1st. */
		if (rw) {
			start_blk = raw_area_control.image[1].start_blk;
			blkcnt = raw_area_control.image[1].used_blk;
			printf("Writing BL1 to sector %ld (%ld sectors).. ",
					start_blk, blkcnt);
			movi_calc_checksum_bl1(addr);
		}
		
		for (i=0, image = raw_area_control.image; i<15; i++) {
			if (image[i].attribute == attribute)
				break;
		}
		start_blk = image[i].start_blk;
		blkcnt = image[i].used_blk;
		printf("%s bootloader.. %ld, %ld ", rw ? "writing":"reading",
				start_blk, blkcnt);
		sprintf(run_cmd,"mmc %s 0 0x%lx 0x%lx 0x%lx",
				rw ? "write":"read",
				addr, start_blk, blkcnt);
		run_command(run_cmd, 0);
		printf("completed\n");
		return 1;		
		
#else
		/* on write case we should write BL2 1st. */
		if (rw) {
			start_blk = raw_area_control.image[1].start_blk;
			blkcnt = raw_area_control.image[1].used_blk;
			printf("Writing BL1 to sector %ld (%ld sectors).. ",
					start_blk, blkcnt);
			movi_write_bl1(addr);
		}

		for (i=0, image = raw_area_control.image; i<15; i++) {
			if (image[i].attribute == attribute)
				break;
				
		}
		start_blk = image[i].start_blk;
		blkcnt = image[i].used_blk;
		printf("%s bootloader.. %ld, %ld ", rw ? "writing":"reading",
				start_blk, blkcnt);
		sprintf(run_cmd,"mmc %s 0 0x%lx 0x%lx 0x%lx",
				rw ? "write":"read",
				addr, start_blk, blkcnt);
		run_command(run_cmd, 0);
		printf("completed\n");
		return 1;
#endif		
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
		blkcnt = rfs_size/MOVI_BLKSIZE +
			((rfs_size&(MOVI_BLKSIZE-1)) ? 1 : 0);
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
		"movi write {fwbl1 | u-boot | kernel} {addr} - Write data to sd/mmc\n"
		"movi read  rootfs {addr} [bytes(hex)] - Read rootfs data from sd/mmc by size\n"
		"movi write rootfs {addr} [bytes(hex)] - Write rootfs data to sd/mmc by size\n"
		"movi read  {sector#} {bytes(hex)} {addr} - instead of this, you can use \"mmc read\"\n"
		"movi write {sector#} {bytes(hex)} {addr} - instead of this, you can use \"mmc write\"\n"
	  );
