/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/uclass-internal.h>
#include <malloc.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <fsl_esdhc.h>
#include <mmc.h>
#include <asm/arch/imx8mq_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/clock.h>
#include <spl.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>

#if defined(CONFIG_FSL_QSPI)
#include <spi.h>
#include <spi_flash.h>
#endif

// #include "../common/pfuze.h"
#include "../common/tqc_bb.h"
#include "../common/tqc_eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_FSL_QSPI) && defined(CONFIG_DM_SPI)

static int board_qspi_init(void)
{
	struct udevice *bus;
	struct uclass *uc;
	int count = 0;
	int ret;

	set_clk_qspi();

	ret = uclass_get(UCLASS_SPI, &uc);
	if (ret)
		return ret;

	uclass_foreach_dev(bus, uc) {
		/* init SPI controllers */
		printf("SPI%d:   ", count);
		count++;

		ret = device_probe(bus);
		if (ret == -ENODEV) {	/* No such device. */
			puts("SPI not available.\n");
			continue;
		}

		if (ret) {		/* Other error. */
			printf("probe failed, error %d\n", ret);
			continue;
		}

		puts("\n");
	}

	return 0;
}
#else
static inline int board_qspi_init(void) { return 0; }
#endif

int board_early_init_f(void)
{
	return tqc_bb_board_early_init_f();
}

#ifdef CONFIG_BOARD_POSTCLK_INIT
int board_postclk_init(void)
{
	/* TODO */
	return 0;
}
#endif

int dram_init(void)
{
	/* rom_pointer[1] contains the size of TEE occupies */
	if (rom_pointer[1])
		gd->ram_size = PHYS_SDRAM_SIZE - rom_pointer[1];
	else
		gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
static void tqma8mx_ft_qspi_setup(void *blob, bd_t *bd)
{
	int off;
	int enable_flash = 0;

	if (QSPI_BOOT == get_boot_device()) {
		enable_flash = 1;
	} else {
#if defined(CONFIG_FSL_QSPI)
		unsigned int bus = CONFIG_SF_DEFAULT_BUS;
		unsigned int cs = CONFIG_SF_DEFAULT_CS;
		unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
		unsigned int mode = CONFIG_SF_DEFAULT_MODE;
#ifdef CONFIG_DM_SPI_FLASH
		struct udevice *new, *bus_dev;
		int ret;

		/* Remove the old device, otherwise probe will just be a nop */
		ret = spi_find_bus_and_cs(bus, cs, &bus_dev, &new);
		if (!ret) {
			device_remove(new, DM_REMOVE_NORMAL);
		}
		ret = spi_flash_probe_bus_cs(bus, cs, speed, mode, &new);
		if (!ret) {
			device_remove(new, DM_REMOVE_NORMAL);
			enable_flash = 1;
		}
#else
		struct spi_flash *new;

		new = spi_flash_probe(bus, cs, speed, mode);
		if (new) {
			spi_flash_free(new);
			enable_flash = 1;
		}
#endif
#endif
	}
	off = fdt_node_offset_by_compatible(blob, -1, "fsl,imx7d-qspi");
	if (off >= 0)
		fdt_set_node_status(blob, off, (enable_flash) ?
				    FDT_STATUS_OKAY : FDT_STATUS_DISABLED,
				    0);
}

int ft_board_setup(void *blob, bd_t *bd)
{
	tqma8mx_ft_qspi_setup(blob, bd);

	return tqc_bb_ft_board_setup(blob, bd);
}
#endif

int board_init(void)
{
	board_qspi_init();

	return tqc_bb_board_init();
}

static const char *tqma8mx_get_boardname(void)
{
	switch (get_cpu_type()) {
	case MXC_CPU_IMX8MD:
		return "TQMa8MD";
		break;
	case MXC_CPU_IMX8MQ:
		return "TQMa8MQ";
		break;
	case MXC_CPU_IMX8MQL:
		return "TQMa8MQL";
		break;
	default:
		return "??";
	};
	return "UNKNOWN";
}

int board_late_init(void)
{
#if !defined(CONFIG_SPL_BUILD)
	struct tqc_eeprom_data eeprom;
	char sstring[0x41];

	if (!tqc_read_eeprom_at(0, 0x53, 1, 0, &eeprom)) {
		tqc_parse_eeprom_id(&eeprom, sstring, ARRAY_SIZE(sstring));
		if (0 == strncmp(sstring, "TQMa8M", 5))
			env_set("boardtype", sstring);
		if (0 == tqc_parse_eeprom_serial(&eeprom, sstring,
						 ARRAY_SIZE(sstring)))
			env_set("serial#", sstring);
		else
			env_set("serial#", "???");

		tqc_show_eeprom(&eeprom, "TQMa8M");
	} else {
		puts("EEPROM: read error\n");
	}
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", tqc_bb_get_boardname());
	env_set("board_rev", tqma8mx_get_boardname());
#endif

	return tqc_bb_board_late_init();
}

int checkboard(void)
{
	printf("Board: %s on a %s\n", tqma8mx_get_boardname(),
	       tqc_bb_get_boardname());
	return 0;
}
