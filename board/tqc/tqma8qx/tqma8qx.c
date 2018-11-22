/*
 * Copyright 2018 TQ Systems GmbH
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <netdev.h>
#include <fsl_ifc.h>
#include <fdt_support.h>
#include <libfdt.h>
#include <environment.h>
#include <fsl_esdhc.h>

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/imx-common/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <dm.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <power-domain.h>
#include <cdns3-uboot.h>

#include "../common/tqc_bb.h"

DECLARE_GLOBAL_DATA_PTR;

int board_early_init_f(void)
{
	tqc_bb_board_early_init_f();

	return 0;
}

static void board_gpio_init(void)
{

}

int checkboard(void)
{
	puts("Board: TQMa8QX\n");

	print_bootinfo();

	/* Note:  After reloc, ipcHndl will no longer be valid.  If handle
	 *        returned by sc_ipc_open matches SC_IPC_CH, use this
	 *        macro (valid after reloc) for subsequent SCI calls.
	 */
	if (gd->arch.ipc_channel_handle != SC_IPC_CH)
		printf("\nSCI error! Invalid handle\n");

#ifdef SCI_FORCE_ABORT
	sc_rpc_msg_t abort_msg;

	puts("Send abort request\n");
	RPC_SIZE(&abort_msg) = 1;
	RPC_SVC(&abort_msg) = SC_RPC_SVC_ABORT;
	sc_ipc_write(SC_IPC_CH, &abort_msg);

	/* Close IPC channel */
	sc_ipc_close(SC_IPC_CH);
#endif /* SCI_FORCE_ABORT */

	return tqc_bb_checkboard();
}

int board_init(void)
{
#ifdef CONFIG_MXC_GPIO
	board_gpio_init();
#endif

	tqc_bb_board_init();

	return 0;
}

void detail_board_ddr_info(void)
{
	puts("\nDDR    ");
}

/*
 * Board specific reset that is system reset.
 */
void reset_cpu(ulong addr)
{
	puts("SCI reboot request");
	sc_pm_reboot(SC_IPC_CH, SC_PM_RESET_TYPE_COLD);
	while (1)
		putc('.');
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return tqc_bb_ft_board_setup(blob, bd);
}
#endif

int board_late_init(void)
{
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	setenv("board_name", "TQMa8QX");
	setenv("board_rev", "iMX8QXP");
#endif

	tqc_bb_board_late_init();

	setenv("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	setenv("sec_boot", "yes");
#endif

	return 0;
}

