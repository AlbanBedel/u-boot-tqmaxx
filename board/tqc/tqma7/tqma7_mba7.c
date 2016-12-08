/*
 * Copyright (C) 2016 TQ Systems
 * Author: Markus Niebel <markus.niebel@tq-group.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx7-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/mxc_i2c.h>
#include <asm/imx-common/spi.h>
#include <asm/io.h>
#include <common.h>
#include <fsl_esdhc.h>
#include <libfdt.h>
#include <linux/sizes.h>
#include <malloc.h>
#include <i2c.h>
#include <miiphy.h>
#include <mmc.h>
#include <netdev.h>

#include "../common/tqc_bb.h"
#include "../common/tqc_eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

#define UART_RX_PAD_CTRL	(PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_PUS_PU100KOHM | \
	PAD_CTL_PUE | PAD_CTL_HYS | PAD_CTL_SRE_SLOW)

#define UART_TX_PAD_CTRL	(PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_PUS_PU100KOHM | \
	PAD_CTL_PUE | PAD_CTL_SRE_SLOW)

#define USDHC_DATA_PAD_CTRL	(PAD_CTL_DSE_3P3V_98OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU47KOHM)

#define USDHC_CMD_PAD_CTRL	(USDHC_DATA_PAD_CTRL)

#define USDHC_CLK_PAD_CTRL	(PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_PUE | PAD_CTL_PUS_PU47KOHM)

/* output of MIC2016 is open drain */
#define USB_OC_PAD_CTRL	(PAD_CTL_PUS_PU47KOHM | \
	PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_HYS | PAD_CTL_SRE_SLOW)

#define GPIO_IN_PAD_CTRL	(PAD_CTL_PUS_PU100KOHM | \
	PAD_CTL_DSE_3P3V_196OHM | PAD_CTL_HYS | PAD_CTL_SRE_SLOW)

#define GPIO_OUT_PAD_CTRL	(PAD_CTL_PUS_PU100KOHM | \
	PAD_CTL_DSE_3P3V_196OHM | PAD_CTL_HYS | PAD_CTL_SRE_FAST)

#define I2C_PAD_CTRL		(PAD_CTL_DSE_3P3V_196OHM | PAD_CTL_SRE_FAST | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU100KOHM)

#define ENET_PAD_CTRL		(PAD_CTL_PUS_PU100KOHM | PAD_CTL_PUE | \
	PAD_CTL_SRE_FAST | PAD_CTL_DSE_3P3V_49OHM)
#define ENET_TX_PAD_CTRL	(ENET_PAD_CTRL)
#define ENET_RX_PAD_CTRL	(ENET_PAD_CTRL | PAD_CTL_HYS)
#define ENET_PAD_CTRL_MDC	(PAD_CTL_DSE_3P3V_196OHM)
#define ENET_PAD_CTRL_MDIO	(PAD_CTL_DSE_3P3V_98OHM)
#define ENET_RST_PAD_CTRL	(PAD_CTL_PUS_PU100KOHM | PAD_CTL_PUE | \
	PAD_CTL_DSE_3P3V_196OHM | PAD_CTL_SRE_FAST)

#define WDOG_PAD_CTRL		(PAD_CTL_PUS_PU5KOHM | PAD_CTL_PUE | \
	PAD_CTL_SRE_FAST | PAD_CTL_DSE_3P3V_196OHM)

static iomux_v3_cfg_t const mba7_fec1_pads[] = {
	MX7D_PAD_ENET1_RGMII_RX_CTL__ENET1_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_RD0__ENET1_RGMII_RD0 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_RD1__ENET1_RGMII_RD1 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_RD2__ENET1_RGMII_RD2 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_RD3__ENET1_RGMII_RD3 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_RXC__ENET1_RGMII_RXC | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_TX_CTL__ENET1_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_TX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_TD0__ENET1_RGMII_TD0 | MUX_PAD_CTRL(ENET_TX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_TD1__ENET1_RGMII_TD1 | MUX_PAD_CTRL(ENET_TX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_TD2__ENET1_RGMII_TD2 | MUX_PAD_CTRL(ENET_TX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_TD3__ENET1_RGMII_TD3 | MUX_PAD_CTRL(ENET_TX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_TXC__ENET1_RGMII_TXC | MUX_PAD_CTRL(ENET_TX_PAD_CTRL),

	MX7D_PAD_GPIO1_IO10__ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL_MDIO),
	MX7D_PAD_GPIO1_IO11__ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL_MDC),
	/* PHY reset */
	MX7D_PAD_ENET1_COL__GPIO7_IO15 | MUX_PAD_CTRL(ENET_RST_PAD_CTRL) |
		MUX_MODE_SION,
	/* INT/PWDN */
	MX7D_PAD_GPIO1_IO09__GPIO1_IO9 | MUX_PAD_CTRL(GPIO_OUT_PAD_CTRL) |
		MUX_MODE_SION,
};

#define ENET1_PHY_RESET_GPIO IMX_GPIO_NR(7, 15)
#define ENET1_PHY_INT_GPIO IMX_GPIO_NR(1, 9)

static iomux_v3_cfg_t const mba7_fec2_pads[] = {
	MX7D_PAD_EPDC_SDCE0__ENET2_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDCLK__ENET2_RGMII_RD0 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDLE__ENET2_RGMII_RD1  | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDOE__ENET2_RGMII_RD2  | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDSHR__ENET2_RGMII_RD3 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE1__ENET2_RGMII_RXC | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_GDRL__ENET2_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_TX_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE2__ENET2_RGMII_TD0 | MUX_PAD_CTRL(ENET_TX_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE3__ENET2_RGMII_TD1 | MUX_PAD_CTRL(ENET_TX_PAD_CTRL),
	MX7D_PAD_EPDC_GDCLK__ENET2_RGMII_TD2 | MUX_PAD_CTRL(ENET_TX_PAD_CTRL),
	MX7D_PAD_EPDC_GDOE__ENET2_RGMII_TD3  | MUX_PAD_CTRL(ENET_TX_PAD_CTRL),
	MX7D_PAD_EPDC_GDSP__ENET2_RGMII_TXC  | MUX_PAD_CTRL(ENET_TX_PAD_CTRL),

	MX7D_PAD_SD2_CD_B__ENET2_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL_MDIO),
	MX7D_PAD_SD2_WP__ENET2_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL_MDC),

	/* PHY reset */
	MX7D_PAD_EPDC_BDR0__GPIO2_IO28 | MUX_PAD_CTRL(ENET_RST_PAD_CTRL) |
		MUX_MODE_SION,
	/* INT/PWDN */
	MX7D_PAD_EPDC_PWR_STAT__GPIO2_IO31 | MUX_PAD_CTRL(GPIO_OUT_PAD_CTRL) |
		MUX_MODE_SION,
};

#define ENET2_PHY_RESET_GPIO IMX_GPIO_NR(2, 28)
#define ENET2_PHY_INT_GPIO IMX_GPIO_NR(2, 31)

static void mba7_setup_iomuxc_enet(void)
{
	imx_iomux_v3_setup_multiple_pads(mba7_fec1_pads,
					 ARRAY_SIZE(mba7_fec1_pads));

	gpio_request(ENET1_PHY_RESET_GPIO, "enet1-phy-rst#");
	gpio_request(ENET1_PHY_INT_GPIO, "enet1-phy-intpwdn");
	gpio_direction_output(ENET1_PHY_INT_GPIO, 1);
	/* Reset PHY */
	gpio_direction_output(ENET1_PHY_RESET_GPIO, 0);
	udelay(100);
	gpio_set_value(ENET1_PHY_RESET_GPIO, 1);
	udelay(500);

	if (is_cpu_type(MXC_CPU_MX7D)) {
		imx_iomux_v3_setup_multiple_pads(mba7_fec2_pads,
						 ARRAY_SIZE(mba7_fec2_pads));
		gpio_request(ENET2_PHY_RESET_GPIO, "enet2-phy-rst#");
		gpio_request(ENET2_PHY_INT_GPIO, "enet2-phy-intpwdn");
		gpio_direction_output(ENET2_PHY_INT_GPIO, 1);
		/* Reset PHY */
		gpio_direction_output(ENET2_PHY_RESET_GPIO, 0);
		udelay(100);
		gpio_set_value(ENET2_PHY_RESET_GPIO, 1);
		udelay(500);
	} else {
		gpio_request(ENET2_PHY_RESET_GPIO, "enet2-reserved0");
		gpio_request(ENET2_PHY_INT_GPIO, "enet2-reserved1");
		gpio_direction_output(ENET2_PHY_INT_GPIO, 0);
		gpio_direction_output(ENET2_PHY_RESET_GPIO, 0);
	}
}

static const uint32_t fec_base[] = {
	ENET_IPS_BASE_ADDR,
	ENET2_IPS_BASE_ADDR
};

static const uint32_t phy_addr[] = {
	TQMA7_ENET1_PHYADDR,
	TQMA7_ENET2_PHYADDR
};

int board_eth_init(bd_t *bis)
{
	int ret;
	int i;
	int count = (is_cpu_type(MXC_CPU_MX7D)) ? ARRAY_SIZE(fec_base) : 1;

	struct mii_dev *bus = NULL;
	struct phy_device *phydev = NULL;

	for (i = 0; i < count; ++i) {
		bus = fec_get_miibus(fec_base[i], i);
		if (!bus)
			goto err_bus;
		/* scan phy */
		phydev = phy_find_by_mask(bus, (0x1 << phy_addr[i]),
					  PHY_INTERFACE_MODE_RGMII_ID);
		if (!phydev)
			goto err_phydev;
		ret = fec_probe(bis, i, fec_base[i], bus, phydev);
		if (ret)
			goto err_fec;
		continue;

err_fec:
		free(phydev);
err_phydev:
		free(bus);
err_bus:
		printf("Error init FEC%d\n", i);
	}

	return 0;
}

static int mba7_setup_fec(int fec_id)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs
		= (struct iomuxc_gpr_base_regs *) IOMUXC_GPR_BASE_ADDR;
	int ret;

	switch (fec_id) {
	case 0:
		/* Use 125M anatop REF_CLK1 for ENET1, clear gpr1[13], gpr1[17]*/
		clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
			(IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_MASK |
			 IOMUXC_GPR_GPR1_GPR_ENET1_CLK_DIR_MASK), 0);
		break;
	case 1:
		if (is_cpu_type(MXC_CPU_MX7S))
			return -ENODEV;

		/* Use 125M anatop REF_CLK2 for ENET2, clear gpr1[14], gpr1[18]*/
		clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
			(IOMUXC_GPR_GPR1_GPR_ENET2_TX_CLK_SEL_MASK |
			 IOMUXC_GPR_GPR1_GPR_ENET2_CLK_DIR_MASK), 0);
		break;
	default:
		printf("FEC%d: unsupported\n", fec_id);
		return -1;
	}

	ret = set_clk_enet(ENET_125MHz);
	if (ret)
		return ret;

	return 0;
}

#define DP83867_DEVADDR         0x1f

/* TODO: needed here until we hav DT init */

#define MBA7_DP83867_RGMII_TX_DELAY_CTRL	0x09 /* 0b1001 = 2,5 ns */
#define MBA7_DP83867_RGMII_RX_DELAY_CTRL	0x09 /* 0b1001 = 2,5 ns */

/* Extended Registers */
#define DP83867_RGMIICTL	0x0032
#define DP83867_RGMIIDCTL	0x0086

/* RGMIIDCTL bits */
#define DP83867_RGMII_TX_CLK_DELAY_SHIFT	4

/* RGMIICTL bits */
#define DP83867_RGMII_TX_CLK_DELAY_EN		BIT(1)
#define DP83867_RGMII_RX_CLK_DELAY_EN		BIT(0)

int board_phy_config(struct phy_device *phydev)
{
	uint32_t val;
	/*
	* TODO: set skew values using phy_read_mmd_indirect from
	* /driver/net/phy/ti - see also LS102xa for details about LED
	* config
	*/
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	/* LED configuration */
	/*
	 * LED1: Link / Activity
	 * LED2: error
	 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x18, 0x0db0);
	/* active low, LED1/2 driven by phy */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x19, 0x1001);


	if (!phydev->drv ||
	    (!phydev->drv->writeext) || (!phydev->drv->readext)) {
		puts("PHY does not have extended functions\n");
		return 0;
	}

	/* set GPIO to out low */
	phydev->drv->writeext(phydev, phydev->addr, DP83867_DEVADDR,
			      0x0171, 0x8888);
	phydev->drv->writeext(phydev, phydev->addr, DP83867_DEVADDR,
			      0x0172, 0x8888);

	val = phydev->drv->readext(phydev, phydev->addr, DP83867_DEVADDR,
				   0x0031);
	val |= 1;
	phydev->drv->writeext(phydev, phydev->addr, DP83867_DEVADDR,
			      0x0031, val);

	val = phy_read(phydev, MDIO_DEVAD_NONE, 0x10);
	val |= 0x2;
	phy_write(phydev, MDIO_DEVAD_NONE, 0x10, val);

	val = phydev->drv->readext(phydev, phydev->addr, DP83867_DEVADDR,
				   DP83867_RGMIICTL);
	val |= (DP83867_RGMII_TX_CLK_DELAY_EN |
		DP83867_RGMII_RX_CLK_DELAY_EN);

	phydev->drv->writeext(phydev, phydev->addr, DP83867_DEVADDR,
			      DP83867_RGMIICTL, val);

	val = ((MBA7_DP83867_RGMII_RX_DELAY_CTRL) |
	       (MBA7_DP83867_RGMII_TX_DELAY_CTRL <<
		DP83867_RGMII_TX_CLK_DELAY_SHIFT));

	phydev->drv->writeext(phydev, phydev->addr, DP83867_DEVADDR,
			      DP83867_RGMIIDCTL, val);

	return 0;
}

iomux_v3_cfg_t const mba7_usb_otg1_pads[] = {
	MX7D_PAD_GPIO1_IO04__USB_OTG1_OC |
		MUX_PAD_CTRL(USB_OC_PAD_CTRL),
	MX7D_PAD_GPIO1_IO05__GPIO1_IO5 |
		MUX_PAD_CTRL(GPIO_OUT_PAD_CTRL),
};

iomux_v3_cfg_t const mba7_usb_otg2_pads[] = {
	MX7D_PAD_GPIO1_IO06__USB_OTG2_OC |
		MUX_PAD_CTRL(USB_OC_PAD_CTRL),
	MX7D_PAD_GPIO1_IO07__GPIO1_IO7 |
		MUX_PAD_CTRL(GPIO_OUT_PAD_CTRL),
};

/*
 * use gpio instead of PWR as log as he ehci driver does not support
 * board specific polarity
 */
#define MBA7_OTG1_PWR_GPIO IMX_GPIO_NR(1, 5)
#define MBA7_OTG2_PWR_GPIO IMX_GPIO_NR(1, 7)

static void mba7_setup_iomux_usb(void)
{
	int ret;

	imx_iomux_v3_setup_multiple_pads(mba7_usb_otg1_pads,
					 ARRAY_SIZE(mba7_usb_otg1_pads));
	ret = gpio_request(MBA7_OTG1_PWR_GPIO, "usb-otg1-pwr");
	if (!ret)
		gpio_direction_output(MBA7_OTG1_PWR_GPIO, 0);

	if (is_cpu_type(MXC_CPU_MX7S))
		return;

	imx_iomux_v3_setup_multiple_pads(mba7_usb_otg2_pads,
					 ARRAY_SIZE(mba7_usb_otg2_pads));
	ret = gpio_request(MBA7_OTG2_PWR_GPIO, "usb-otg2-pwr");
	if (!ret)
		gpio_direction_output(MBA7_OTG2_PWR_GPIO, 0);
}


int board_ehci_hcd_init(int port)
{
	switch (port) {
	case 0:
		break;
	case 1:
		if (is_cpu_type(MXC_CPU_MX7S))
			return -ENODEV;
		break;
	case 2:
		printf("MXC USB port %d not yet supported\n", port);
		return -ENODEV;
		break;
	default:
		return -ENODEV;
	}
	return 0;
}

int board_ehci_power(int port, int on)
{
	switch (port) {
	case 0:
		gpio_set_value(MBA7_OTG1_PWR_GPIO, on);
		break;
	case 1:
		if (is_cpu_type(MXC_CPU_MX7S))
			return -ENODEV;
		gpio_set_value(MBA7_OTG2_PWR_GPIO, on);
		break;
	case 2:
		printf("MXC USB port %d not yet supported\n", port);
		return -ENODEV;
		break;
	default:
		return -ENODEV;
	}
	return 0;
}

static iomux_v3_cfg_t const mba7_uart6_pads[] = {
	NEW_PAD_CTRL(MX7D_PAD_EPDC_DATA08__UART6_DCE_RX, UART_RX_PAD_CTRL),
	NEW_PAD_CTRL(MX7D_PAD_EPDC_DATA09__UART6_DCE_TX, UART_TX_PAD_CTRL),
};

static void mba7_setup_iomuxc_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(mba7_uart6_pads,
					 ARRAY_SIZE(mba7_uart6_pads));
}

static iomux_v3_cfg_t const mba7_usdhc1_pads[] = {
	NEW_PAD_CTRL(MX7D_PAD_SD1_CLK__SD1_CLK,		USDHC_CLK_PAD_CTRL),
	NEW_PAD_CTRL(MX7D_PAD_SD1_CMD__SD1_CMD,		USDHC_CMD_PAD_CTRL),
	NEW_PAD_CTRL(MX7D_PAD_SD1_DATA0__SD1_DATA0,	USDHC_DATA_PAD_CTRL),
	NEW_PAD_CTRL(MX7D_PAD_SD1_DATA1__SD1_DATA1,	USDHC_DATA_PAD_CTRL),
	NEW_PAD_CTRL(MX7D_PAD_SD1_DATA2__SD1_DATA2,	USDHC_DATA_PAD_CTRL),
	NEW_PAD_CTRL(MX7D_PAD_SD1_DATA3__SD1_DATA3,	USDHC_DATA_PAD_CTRL),
	/* CD */
	NEW_PAD_CTRL(MX7D_PAD_SD1_CD_B__GPIO5_IO0,	GPIO_IN_PAD_CTRL),
	/* WP */
	NEW_PAD_CTRL(MX7D_PAD_SD1_WP__GPIO5_IO1,	GPIO_IN_PAD_CTRL),
};

#define USDHC1_CD_GPIO	IMX_GPIO_NR(5, 0)
#define USDHC1_WP_GPIO	IMX_GPIO_NR(5, 1)

int tqc_bb_board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	if (cfg->esdhc_base == USDHC1_BASE_ADDR)
		ret = !gpio_get_value(USDHC1_CD_GPIO);

	return ret;
}

int tqc_bb_board_mmc_getwp(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	if (cfg->esdhc_base == USDHC1_BASE_ADDR)
		ret = gpio_get_value(USDHC1_WP_GPIO);

	return ret;
}

static struct fsl_esdhc_cfg mba7_usdhc_cfg = {
	.esdhc_base = USDHC1_BASE_ADDR,
	.max_bus_width = 4,
};

int tqc_bb_board_mmc_init(bd_t *bis)
{
	imx_iomux_v3_setup_multiple_pads(mba7_usdhc1_pads,
					 ARRAY_SIZE(mba7_usdhc1_pads));
	gpio_request(USDHC1_CD_GPIO, "usdhc1-cd");
	gpio_request(USDHC1_WP_GPIO, "usdhc1-wp");
	gpio_direction_input(USDHC1_CD_GPIO);
	gpio_direction_input(USDHC1_WP_GPIO);

	mba7_usdhc_cfg.sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
	if (fsl_esdhc_initialize(bis, &mba7_usdhc_cfg))
		puts("Warning: failed to initialize SD\n");

	return 0;
}


static struct i2c_pads_info mba7_i2c2_pads = {
/* I2C2: MBa7x */
	.scl = {
		.i2c_mode = NEW_PAD_CTRL(MX7D_PAD_I2C2_SCL__I2C2_SCL,
					 I2C_PAD_CTRL),
		.gpio_mode = NEW_PAD_CTRL(MX7D_PAD_I2C2_SCL__GPIO4_IO10,
					  I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(4, 10)
	},
	.sda = {
		.i2c_mode = NEW_PAD_CTRL(MX7D_PAD_I2C2_SDA__I2C2_SDA,
					 I2C_PAD_CTRL),
		.gpio_mode = NEW_PAD_CTRL(MX7D_PAD_I2C2_SDA__GPIO4_IO11,
					  I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(4, 11)
	}
};

static void mba7_setup_i2c(void)
{
	int ret;

	/*
	 * use logical index for bus, e.g. I2C1 -> 0
	 * warn on error
	 */
	ret = setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &mba7_i2c2_pads);
	if (ret)
		printf("setup I2C2 failed: %d\n", ret);
}

int tqc_bb_board_early_init_f(void)
{
	mba7_setup_iomuxc_uart();

	return 0;
}

int tqc_bb_board_init(void)
{
	mba7_setup_i2c();
	/* do it here - to have reset completed */
	mba7_setup_iomuxc_enet();
	mba7_setup_fec(0);
	/* TODO: only for TQMa7D */
	mba7_setup_fec(1);

	mba7_setup_iomux_usb();

	return 0;
}

static iomux_v3_cfg_t const mba7_wdog_pads[] = {
	NEW_PAD_CTRL(MX7D_PAD_GPIO1_IO00__WDOG1_WDOG_B, WDOG_PAD_CTRL),
};

#define MBA7_PCF8574_I2C_BUS	1
#define MBA7_PCF8574_DEV0_ADDR	0x3c
#define MBA7_PCF8574_DEV0_VAL	0x1f /* 0 - output, 1 - input */
#define MBA7_PCF8574_DEV1_ADDR	0x39
#define MBA7_PCF8574_DEV1_VAL	0xe0 /* 0 - output, 1 - input */

static const uint8_t io_exp[] = {
	MBA7_PCF8574_DEV0_ADDR,
	MBA7_PCF8574_DEV1_ADDR
};

static uint8_t io_dat[] = {
	MBA7_PCF8574_DEV0_VAL,
	MBA7_PCF8574_DEV1_VAL
};

int tqc_bb_board_late_init(void)
{
	int i;
	/*
	* try to get sd card slots in order:
	* eMMC: on Module
	* -> therefore index 0 for bootloader
	* index n in kernel (controller instance 3) -> patches needed for
	* alias indexing
	* SD1: on Mainboard
	* index n in kernel (controller instance 1) -> patches needed for
	* alias indexing
	* we assume to have a kernel patch that will present mmcblk dev
	* indexed like controller devs
	*/
	puts("Boot:\t");
	switch (get_boot_device()) {
	case MMC3_BOOT:
		setenv("boot_dev", "mmc");
		/* eMMC (USDHC3)*/
		setenv("mmcblkdev", "0");
		setenv("mmcdev", "0");
		break;
	case SD2_BOOT:
		setenv("boot_dev", "mmc");
		/* ext SD (USDHC1)*/
		setenv("mmcblkdev", "1");
		setenv("mmcdev", "1");
		break;
	/* TODO: QSPI */
	case QSPI_BOOT:
		setenv("boot_dev", "qspi");
		/* mmcdev is dev index for u-boot */
		setenv("mmcdev", "0");
		/* mmcblkdev is dev index for linux env */
		setenv("mmcblkdev", "0");
		break;
	default:
		puts("unhandled boot device\n");
		setenv("mmcblkdev", "");
		setenv("mmcdev", "");
	}

	imx_iomux_v3_setup_multiple_pads(mba7_wdog_pads,
					 ARRAY_SIZE(mba7_wdog_pads));
	set_wdog_reset((struct wdog_regs *)WDOG1_BASE_ADDR);

	/* There is a HW-BUG on HW.REV.0100
	 * set port-expander default values to avoid unexpected behaviours */
	i2c_set_bus_num(MBA7_PCF8574_I2C_BUS);
	for (i = 0; i < ARRAY_SIZE(io_exp); ++i) {
		if (!i2c_probe(io_exp[i])) {
			/* this device has no addr hence addr length is 0 */
			if (i2c_write(io_exp[i], 0, 0, &io_dat[i], 1))
				printf("PCF8574 write failed\n");
		} else {
			printf("PCF8574 Not found\n");
		}
	}

	return 0;
}

const char *tqc_bb_get_boardname(void)
{
	return "MBa7";
}

int board_mmc_get_env_dev(int devno)
{
	int env_dev;
	/*
	 * eMMC:	USDHC3 -> 0
	 * SD:		USDHC1 -> 1
	 */
	switch (devno) {
	case 2:
		env_dev = 0;
		break;
	case 0:
		env_dev = 1;
		break;
	default:
		printf("mmc boot dev %d not supported, use default",
		       devno);
		env_dev = CONFIG_SYS_MMC_ENV_DEV;
	};

	return env_dev;
}

/*
 * Device Tree Support
 */
#if defined(CONFIG_OF_BOARD_SETUP) && defined(CONFIG_OF_LIBFDT)
void tqc_bb_ft_board_setup(void *blob, bd_t *bd)
{

}
#endif /* defined(CONFIG_OF_BOARD_SETUP) && defined(CONFIG_OF_LIBFDT) */
