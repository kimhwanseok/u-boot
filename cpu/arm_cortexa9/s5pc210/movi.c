#include <common.h>
#include <movi.h>
#include <asm/io.h>
#include <mmc.h>

extern raw_area_t raw_area_control;

typedef u32 (*copy_sd_mmc_to_mem) \
	(u32 start_block, u32 block_count, u32* dest_addr);

typedef u32 (*copy_emmc_to_mem) \
	(u32 block_size, u32 *buffer);

typedef u32 (*copy_emmc441_to_mem) \
	(u32 uRxWMark,u32 block_size, u32 *buffer);

typedef u32 (*emmc441_endboot_op)();


//#define CopyMMC4_3toMem(a,b,c,d)        (((bool(*)(bool, unsigned int, unsigned int*, int))(*((unsigned int *)0xD0037F9C)))(a,b,c,d))

void movi_uboot_copy(void)
{
#ifdef CONFIG_EVT1
	copy_sd_mmc_to_mem copy_bl2 = (copy_sd_mmc_to_mem)*(u32 *)(0x02020030);
#else
	copy_sd_mmc_to_mem copy_bl2 = (copy_sd_mmc_to_mem)(0x00002488);
#endif

#ifdef CONFIG_SECURE
	copy_bl2(MOVI_UBOOT_POS_SECURE, MOVI_UBOOT_BLKCNT, CFG_PHY_UBOOT_BASE);
#else
	copy_bl2(MOVI_UBOOT_POS_NONSECURE, MOVI_UBOOT_BLKCNT, CFG_PHY_UBOOT_BASE);
#endif
}

void emmc_uboot_copy(void)
{
	int i;
	char *ptemp;
	
#ifdef CONFIG_EVT1
	copy_emmc_to_mem copy_bl2 = (copy_emmc_to_mem)*(u32 *)(0x0202003c);
#else
	copy_emmc_to_mem copy_bl2 = (copy_emmc_to_mem)(0x00003268);//LoadBL2FromEmmc43Ch0ByDMA
#endif

	copy_bl2(MOVI_UBOOT_BLKCNT, CFG_PHY_UBOOT_BASE);
}

void end_bootop(void)
{
#ifdef CONFIG_EVT1
	emmc441_endboot_op irom_end_bootop = (emmc441_endboot_op)*(u32 *)(0x02020048);	//MSH_EndBootOp_eMMC
#else
	emmc441_endboot_op irom_end_bootop = (emmc441_endboot_op)*(u32 *)(0x000082c8);	//MSH_EndBootOp_eMMC
#endif
	irom_end_bootop();
}

void emmc441_uboot_copy(void)
{
	int i;
	char *ptemp;

#ifdef CONFIG_EVT1
	copy_emmc441_to_mem copy_bl2 = (copy_emmc441_to_mem)*(u32 *)(0x02020044);	//MSH_ReadFromFIFO_eMMC
#else
	copy_emmc441_to_mem copy_bl2 = (copy_emmc441_to_mem)*(u32 *)(0x00007974);	//MSH_ReadFromFIFO_eMMC
#endif

	copy_bl2(0x10, MOVI_UBOOT_BLKCNT, CFG_PHY_UBOOT_BASE); 

	/* stop bootop */
	end_bootop();
}

void check_emmc_open(void);
void check_emmc_close(void);

void movi_write_env(ulong addr)
{
	movi_write(raw_area_control.image[3].start_blk, 
		raw_area_control.image[3].used_blk, addr);
}

void movi_read_env(ulong addr)
{
	movi_read(raw_area_control.image[3].start_blk,
		raw_area_control.image[3].used_blk, addr);
}

#ifdef CONFIG_SECURE
void movi_write_bl1(ulong addr)
{
	int	i;
	ulong	checksum=0;
	u8	*src;
	ulong	tmp;

	src = (u8 *) malloc(raw_area_control.image[1].size);
	memset(src,0,raw_area_control.image[1].size);
	for(i = 0;i < (14 * 1024) - 4;i++)
	{
		src[i]=*(u8*)addr++;
		checksum += src[i];
	}
	*(ulong*)(src+i) = checksum;
			
	movi_write(raw_area_control.image[1].start_blk, 
		raw_area_control.image[1].used_blk, src);
	free(src);
}
#else
void movi_write_bl1(ulong addr)
{
	int	i;
	ulong	checksum=0;
	u8	*src;

	src = (u8 *) malloc(raw_area_control.image[1].size);
	memset(src,0,raw_area_control.image[1].size);

	for(i = 0;i < 16368;i++)
	{
		src[i+16]=*(u8*)addr++;
		checksum += src[i+16];
	}

	*(ulong*)src = 0x1f;
	*(ulong*)(src+4) = checksum;

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

	for(i=1;i<16;i++)
	{
		src[i] ^= src[i-1];
	}

	movi_write(raw_area_control.image[1].start_blk,
		raw_area_control.image[1].used_blk, src);
	free(src);
}
#endif

