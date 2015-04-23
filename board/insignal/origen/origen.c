/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <s5pc210.h>
#include <asm/io.h>

int check_bootmode(void);
unsigned int OmPin;
int boot_mode;

DECLARE_GLOBAL_DATA_PTR;
extern int nr_dram_banks;
unsigned int dmc_density = 0xFFFFFFFF;

static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n" "bne 1b":"=r" (loops):"0"(loops));
}

int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd->bd->bi_arch_number = MACH_TYPE;
	gd->bd->bi_boot_params = (PHYS_SDRAM_1+0x100);

	return 0;
}

int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	nr_dram_banks = 4;
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
	gd->bd->bi_dram[2].start = PHYS_SDRAM_5;
	gd->bd->bi_dram[2].size = PHYS_SDRAM_5_SIZE;
	gd->bd->bi_dram[3].start = PHYS_SDRAM_6;
	gd->bd->bi_dram[3].size = PHYS_SDRAM_6_SIZE;

	return 0;
}

#ifdef BOARD_LATE_INIT
int board_late_init (void)
{
	int ret = check_bootmode();
	if ((ret == BOOT_MMCSD || ret == BOOT_EMMC441 || ret == BOOT_EMMC43 )
				&& boot_mode == 0) {
		printf("board_late_init\n");
		}

	return 0;
}
#endif

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	printf("Board:	%s\n", CONFIG_DEVICE_STRING);
	return (0);
}
#endif

#ifdef CONFIG_ENABLE_MMU
ulong virt_to_phy_smdkc210(ulong addr)
{
	if ((0xc0000000 <= addr) && (addr < 0xd0000000))
		return (addr - 0xc0000000 + 0x40000000);
	else;
		//printf("The input address don't need "\
		//	"a virtual-to-physical translation : %08lx\n", addr);

	return addr;
}
#endif

int check_bootmode(void)
{
	OmPin = INF_REG3_REG = BOOT_MMCSD;

	printf("\n\nChecking Boot Mode ...");

	printf("OMPIN : %d\n", OmPin);

	if(OmPin == BOOT_ONENAND) {
		printf(" OneNand\n");
	} else if (OmPin == BOOT_NAND) {
		printf(" NAND\n");
	} else if (OmPin == BOOT_MMCSD) {
		printf(" SDMMC\n");
	} else if (OmPin == BOOT_EMMC43) {
		printf(" EMMC4.3\n");
	} else if (OmPin == BOOT_EMMC441) {
		printf(" EMMC4.41\n");
	}

	return OmPin;
}

#if defined(CONFIG_CMD_NAND) && defined(CFG_NAND_LEGACY)
#include <linux/mtd/nand.h>
extern struct nand_chip nand_dev_desc[CFG_MAX_NAND_DEVICE];
void nand_init(void)
{
	nand_probe(CFG_NAND_BASE);
        if (nand_dev_desc[0].ChipID != NAND_ChipID_UNKNOWN) {
                print_size(nand_dev_desc[0].totlen, "\n");
        }
}
#endif
