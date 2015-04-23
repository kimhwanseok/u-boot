#include <common.h>
#include <s5pc210.h>
#include <asm/io.h>
#include <mmc.h>
#include <s3c_hsmmc.h>


#ifdef CONFIG_S5P_MSHC
extern void mshci_fifo_deinit(struct mshci_host *host);
extern struct mmc *find_mmc_device(int dev_num);
#endif

void clear_hsmmc_clock_div(void)
{
	/* set sdmmc clk src as mpll */
	u32 tmp;
#ifdef CONFIG_S5P_MSHC
	struct mmc *mmc = find_mmc_device(0);

	mshci_fifo_deinit(mmc);
#endif

	tmp = CLK_SRC_FSYS & ~(0x000fffff);
	CLK_SRC_FSYS = tmp | 0x00066666;


	CLK_DIV_FSYS1 = 0x00080008;
	CLK_DIV_FSYS2 = 0x00080008;
	CLK_DIV_FSYS3 = 0x4;


}

void set_hsmmc_pre_ratio(uint clock)
{
	u32 div;
	u32 tmp;

	/* XXX: we assume that clock is between 40MHz and 50MHz */
	if(clock <= 400000)
		div = 127;
	else if (clock <= 20000000)
		div = 3;
	else if (clock <= 26000000)
		div = 1;
	else
		div = 0;

#ifdef USE_MMC0
	tmp = CLK_DIV_FSYS1 & ~(0x0000ff00);
	CLK_DIV_FSYS1 = tmp | (div << 8);
#endif

#ifdef USE_MMC2
	tmp = CLK_DIV_FSYS2 & ~(0x0000ff00);
	CLK_DIV_FSYS2 = tmp | (div << 8);
#endif

}

void setup_hsmmc_clock(void)
{
	u32 tmp;
	u32 clock;
	u32 i;


#ifdef USE_MMC0
	/* MMC0 clock src = SCLKMPLL */
	tmp = CLK_SRC_FSYS & ~(0x0000000f);
	CLK_SRC_FSYS = tmp | 0x00000006;

	/* MMC0 clock div */
	tmp = CLK_DIV_FSYS1 & ~(0x0000000f);
	clock = get_MPLL_CLK()/1000000;
	for(i=0 ; i<=0xf ; i++)
	{
		if((clock / (i+1)) <= 50) {
			CLK_DIV_FSYS1 = tmp | i<<0;
			break;
		}
	}
#endif	

#ifdef USE_MMC1
#endif	

#ifdef USE_MMC2
	/* MMC2 clock src = SCLKMPLL */
	tmp = CLK_SRC_FSYS & ~(0x00000f00);
	CLK_SRC_FSYS = tmp | 0x00000600;

	/* MMC2 clock div */
	tmp = CLK_DIV_FSYS2 & ~(0x0000000f);
	clock = get_MPLL_CLK()/1000000;
	for(i=0 ; i<=0xf; i++)
	{
		if((clock /(i+1)) <= 50) {
			CLK_DIV_FSYS2 = tmp | i<<0;
			break;
		}
	}
#endif

#ifdef USE_MMC3
#endif

#ifdef USE_MMC4
	/* MMC4 clock src = SCLKMPLL */
	tmp = CLK_SRC_FSYS & ~(0x000f0000);
	CLK_SRC_FSYS = tmp | 0x00060000;
	/* MMC4 clock div */
	tmp = CLK_DIV_FSYS3 & ~(0x0000ff0f);
	clock = get_MPLL_CLK()/1000000;
	for(i=0 ; i<=0xf; i++)	{
		if((clock /(i+1)) <= 160) {
			CLK_DIV_FSYS3 = tmp | (i<<0);
			break;
		}
	}
#endif

}

/*
 * this will set the GPIO for hsmmc ch0
 * GPG0[0:6] = CLK, CMD, CDn, DAT[0:3]
 */
void setup_hsmmc_cfg_gpio(void)
{
	ulong reg;

#ifdef USE_MMC0
	writel(0x02222222, 0x11000040);
	writel(0x00003FFC, 0x11000048);
	writel(0x00003FFF, 0x1100004c);
	writel(0x03333000, 0x11000060);
	writel(0x00003FC0, 0x11000068);
	writel(0x00003FC0, 0x1100006c);	
#endif

#ifdef USE_MMC1
#endif

#ifdef USE_MMC2
	writel(0x02222222, 0x11000080);
	writel(0x00003FF0, 0x11000088);
	writel(0x00003FFF, 0x1100008C);
#endif

#ifdef USE_MMC3
#endif

#ifdef USE_MMC4
	writel(0x03333333, 0x11000040);
	writel(0x00003FF0, 0x11000048);
	writel(0x00002AAA, 0x1100004C);
	writel(0x04444000, 0x11000060);
	writel(0x00003FC0, 0x11000068);
	writel(0x00002AAA, 0x1100006C);
#endif

}


void setup_sdhci0_cfg_card(struct sdhci_host *host)
{
	u32 ctrl2;
	u32 ctrl3;

	/* don't need to alter anything acording to card-type */
	writel(S3C64XX_SDHCI_CONTROL4_DRIVE_9mA, host->ioaddr + S3C64XX_SDHCI_CONTROL4);

	ctrl2 = readl(host->ioaddr + S3C_SDHCI_CONTROL2);

	ctrl2 |= (S3C64XX_SDHCI_CTRL2_ENSTAASYNCCLR |
		S3C64XX_SDHCI_CTRL2_ENCMDCNFMSK |
		S3C_SDHCI_CTRL2_DFCNT_NONE |
		S3C_SDHCI_CTRL2_ENCLKOUTHOLD);

	printf("host->clock: %d\n", host->clock);

	if (0 < host->clock && host->clock < 25000000) {
		/* Feedback Delay Disable */
		ctrl2 &= ~(S3C_SDHCI_CTRL2_ENFBCLKTX |
			S3C_SDHCI_CTRL2_ENFBCLKRX);
		ctrl3 = 0;
	} else if (25000000 <= host->clock && host->clock <= 52000000) {
		/* Feedback Delay Disable */
		ctrl2 &= ~(S3C_SDHCI_CTRL2_ENFBCLKTX |
			S3C_SDHCI_CTRL2_ENFBCLKRX);

		/* Feedback Delay Rx Enable */
		ctrl2 |= (S3C_SDHCI_CTRL2_ENFBCLKRX);
		ctrl3 = (15 << 1 | 7 << 1);
	} else
		printf("This CLOCK is Not Support: %d\n", host->clock);

	writel(ctrl2, host->ioaddr + S3C_SDHCI_CONTROL2);
	writel(ctrl3, host->ioaddr + S3C_SDHCI_CONTROL3);
}
