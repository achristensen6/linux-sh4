/*
 * arch/sh/boards/mach-h225hdk/setup.c
 *
 * Copyright (C) 2010 STMicroelectronics Limited
 * Author: Stuart Menefy (stuart.menefy@st.com)
 * Author: Srinivas.Kandagatla (srinivas.kandagatla@st.com)
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 *
 * STMicroelectronics STx7141 H225 Reference Board support.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/phy.h>
#include <linux/lirc.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/tm1668.h>
#include <linux/leds.h>
#include <linux/input.h>
#include <linux/stm/emi.h>
#include <linux/stm/platform.h>
#include <linux/stm/stx7141.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/spi/flash.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/partitions.h>
#include <asm/irq-ilc.h>
#include <asm/irl.h>
#include <sound/stm.h>

#define H2257141_PIO_PHY_RESET0 stm_gpio(5, 3)
#define H2257141_PIO_PHY_RESET1 stm_gpio(15, 7)

static void __init h225hdk_setup(char **cmdline_p)
{
	printk(KERN_INFO "STMicroelectronics STx7141 H255 Reference Board initialisation\n");

	stx7141_early_device_init();

	/* For "STi7141-H225 Wave V1.1A" Board,  Smart card cable needs to be
	 * unplugged if you want to control console */
	stx7141_configure_asc(2, &(struct stx7141_asc_config) {
			.is_console = 1, });

}

/* NOR Flash */
static struct mtd_partition h225hdk_nor_flash_parts[] = {
	{
		.name = "Boot firmware",
		.size = 0x00060000,
		.offset = 0x00000000,
	}, {
		.name = "Kernel",
		.size = 0x00300000,
		.offset = 0x00060000,
	}, {
		.name = "Root FS",
		.size = MTDPART_SIZ_FULL,
		.offset = 0x00360000,
	}
};

static struct platform_device h225hdk_nor_flash = {
	.name		= "physmap-flash",
	.id		= -1,
	.num_resources	= 1,
	.resource	= (struct resource[]) {
		STM_PLAT_RESOURCE_MEM(0, 64*1024*1024),
	},
	.dev.platform_data = &(struct physmap_flash_data) {
		.width		= 2,
		.nr_parts	= ARRAY_SIZE(h225hdk_nor_flash_parts),
		.parts		= h225hdk_nor_flash_parts,
	},
};


/* NAND Flash */
static struct mtd_partition h225hdk_nand_flash_parts[] = {
	{
		.name   = "NAND root",
		.offset = 0,
		.size   = 0x00800000
	}, {
		.name   = "NAND home",
		.offset = MTDPART_OFS_APPEND,
		.size   = MTDPART_SIZ_FULL
	},
};

static struct stm_nand_bank_data h225hdk_nand_flash_data = {
	.csn		= 0,
	.nr_partitions	= ARRAY_SIZE(h225hdk_nand_flash_parts),
	.partitions	= h225hdk_nand_flash_parts,
	.options	= NAND_NO_AUTOINCR | NAND_USE_FLASH_BBT,
	.timing_data = &(struct stm_nand_timing_data) {
		.sig_setup      = 10,           /* times in ns */
		.sig_hold       = 10,
		.CE_deassert    = 0,
		.WE_to_RBn      = 100,
		.wr_on          = 10,
		.wr_off         = 30,
		.rd_on          = 10,
		.rd_off         = 30,
		.chip_delay     = 40,           /* in us */
	},
	.emi_withinbankoffset	= 0,
};
/* FrontPanel */
static struct platform_device h225hdk_leds = {
	.name = "leds-gpio",
	.id = 0,
	.dev.platform_data = &(struct gpio_led_platform_data) {
		.num_leds = 2,
		.leds = (struct gpio_led[]) {
			{
				.name = "FP red",
				.gpio = stm_gpio(16, 1),
				.active_low = 1,
			}, {
				.name = "FP green",
				.default_trigger = "heartbeat",
				.gpio = stm_gpio(16, 2),
				.active_low = 1,
			},
		},
	},
};

static struct tm1668_key h225hdk_front_panel_keys[] = {
	{ 0x00001000, KEY_UP, "Up (SWF2)" },
	{ 0x00800000, KEY_DOWN, "Down (SWF7)" },
	{ 0x00008000, KEY_LEFT, "Left (SWF6)" },
	{ 0x00000010, KEY_RIGHT, "Right (SWF5)" },
	{ 0x00000080, KEY_ENTER, "Enter (SWF1)" },
	{ 0x00100000, KEY_ESC, "Escape (SWF4)" },
};

static struct tm1668_character h225hdk_front_panel_characters[] = {
	TM1668_7_SEG_HEX_DIGITS,
	TM1668_7_SEG_HEX_DIGITS_WITH_DOT,
	TM1668_7_SEG_SEGMENTS,
};

static struct platform_device h225hdk_front_panel = {
	.name = "tm1668",
	.id = -1,
	.dev.platform_data = &(struct tm1668_platform_data) {
		.gpio_dio = stm_gpio(6, 2),
		.gpio_sclk = stm_gpio(6, 3),
		.gpio_stb = stm_gpio(6, 7),
		.config = tm1668_config_6_digits_12_segments,

		.keys_num = ARRAY_SIZE(h225hdk_front_panel_keys),
		.keys = h225hdk_front_panel_keys,
		.keys_poll_period = DIV_ROUND_UP(HZ, 5),

		.brightness = 8,
		.characters_num = ARRAY_SIZE(h225hdk_front_panel_characters),
		.characters = h225hdk_front_panel_characters,
		.text = "7141",
	},
};


static int h225hdk_phy_reset0(void *bus)
{
	printk(KERN_DEBUG "Resetting PHY0\n");
	gpio_set_value(H2257141_PIO_PHY_RESET0, 1);
	udelay(10);
	gpio_set_value(H2257141_PIO_PHY_RESET0, 0);
	mdelay(50);
	gpio_set_value(H2257141_PIO_PHY_RESET0, 1);
	udelay(10);

	return 1;
}

static int h225hdk_phy_reset1(void *bus)
{
	printk(KERN_DEBUG "Resetting PHY1\n");
	gpio_set_value(H2257141_PIO_PHY_RESET1, 1);
	udelay(10);
	gpio_set_value(H2257141_PIO_PHY_RESET1, 0);
	mdelay(50);
	gpio_set_value(H2257141_PIO_PHY_RESET1, 1);
	udelay(10);

	return 1;
}

#ifdef CONFIG_SH_ST_H2257141_STMMAC0
#define STMMAC0_PHY_ADDR 1
static int stmmac0_phy_irqs[PHY_MAX_ADDR] = {
                [STMMAC0_PHY_ADDR] = ILC_IRQ(43),
};
static struct stmmac_mdio_bus_data stmmac0_mdio_bus = {
        .bus_id = 0,
        .phy_reset = h225hdk_phy_reset1,
        .phy_mask = 0,
        .irqs = stmmac0_phy_irqs,
};
#endif

static struct stmmac_mdio_bus_data stmmac1_mdio_bus = {
        .bus_id = 1,
        .phy_reset = h225hdk_phy_reset0,
        .phy_mask = 0,
};

#ifdef CONFIG_SND

#define STV6417_REG_COUNT	(9)
static int h225hdk_scart_av_init(struct i2c_client *client, void *priv)
{
	/* Refer to STV6417/18 AN001 for More details on register values */
	char cmd[STV6417_REG_COUNT * 2] = { 0x00, 0x00,
			0x01, 0x09, /* Audio Enable */
			0x02, 0x11, /* Encoder input for TV & VCR SCART */
			0x03, 0x00,
			0x04, 0xa0, /* Bi-directional Y/C connections ??*/
			0x05, 0x81, /* Video encoder DC-coupled */
			0x06, 0x00,
			0x07, 0x00, /* Detection Disabled */
			0x08, 0x00  /* All I/0 out of standby */
			};
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg[STV6417_REG_COUNT];
	int i;

	for (i = 0 ; i < STV6417_REG_COUNT ; i++) {
		msg[i].addr = client->addr;
		msg[i].flags = client->flags & I2C_M_TEN;
		msg[i].len = sizeof(char) * 2;
		msg[i].buf = &cmd[i*2];
	}
	return i2c_transfer(adap, &msg[0], STV6417_REG_COUNT);
}

static int h225hdk_comp_av_init(struct i2c_client *client, void *priv)
{
	char cmd[] = { 0x0, 0x00 };
	int cmd_len = sizeof(cmd);
	i2c_master_send(client, cmd, cmd_len);
	cmd[0] = 0x1;
	cmd[1] = 0x0;
	i2c_master_send(client, cmd, cmd_len);
	return 0;
}

static struct i2c_board_info h225hdk_av_ctrl[] = {
	/* Component Audio/Video outputs control */
	{
		I2C_BOARD_INFO("snd_conv_i2c", 0x4a),
		.type = "STV6440",
		.platform_data = &(struct snd_stm_conv_i2c_info)
		{
			.group = "Analog Output",
			.source_bus_id = "snd_pcm_player.1",
			.channel_from = 0,
			.channel_to = 1,
			.format = SND_STM_FORMAT__I2S |
					SND_STM_FORMAT__SUBFRAME_32_BITS,
			.oversampling = 256,
			.mute_supported = 1,
			.mute_cmd = (char []){ 0x00, 0x80 },
			.mute_cmd_len = 2,
			.init = h225hdk_comp_av_init,
			.enable_supported = 1,
			.enable_cmd = (char []){ 0x00, 0x00 },
			.enable_cmd_len = 2,
			.disable_cmd = (char []){ 0x00, 0x80 },
			.disable_cmd_len = 2,
		},
	},
	/* Audio/Video on SCART outputs control */
	{
		I2C_BOARD_INFO("snd_conv_i2c", 0x4b),
		.type = "STV6417",
		.platform_data = &(struct snd_stm_conv_i2c_info)
		{
			.group = "Analog Output",
			.source_bus_id = "snd_pcm_player.1",
			.channel_from = 0,
			.channel_to = 1,
			.format = SND_STM_FORMAT__I2S |
					SND_STM_FORMAT__SUBFRAME_32_BITS,
			.oversampling = 256,
			.init = h225hdk_scart_av_init,
			.enable_supported = 1,
			.enable_cmd = (char []){ 0x01, 0x09 },
			.enable_cmd_len = 2,
			.disable_cmd = (char []){ 0x01, 0x00 },
			.disable_cmd_len = 2,
		},
	},
};
#endif

/* GPIO based SPI */
static struct platform_device h225hdk_spi_gpio_device = {
	.name		= "spi_gpio",
	.id		= 0,
	.num_resources  = 0,
	.dev		= {
			.platform_data = &(struct spi_gpio_platform_data) {
				.sck = stm_gpio(2, 0),
				.mosi = stm_gpio(2, 1),
				.miso = stm_gpio(2, 2),
				.num_chipselect = 1,
			},
	},
};

static struct platform_device *h225hdk_devices[] __initdata = {
	&h225hdk_leds,
	&h225hdk_front_panel,
	&h225hdk_spi_gpio_device,
};

static int __init h225hdk_device_init(void)
{
	struct sysconf_field *sc;
	u32 boot_mode;

	/* Configure FLASH devices */
	sc = sysconf_claim(SYS_STA, 1, 16, 17, "boot_mode");
	boot_mode = sysconf_read(sc);
	BUG_ON(boot_mode > 0x1);
	switch (boot_mode) {
	case 0x0:
		/* Boot-from-NOR: */
		pr_info("Configuring FLASH for boot-from-NOR\n");
		h225hdk_nor_flash.resource[0].start = 0x00000000;
		h225hdk_nor_flash.resource[0].end = emi_bank_base(1) - 1;
		/* EMI_notCSC */
		h225hdk_nand_flash_data.csn = 1;
		break;
	case 0x1:
		/* Boot-from-NAND */
		pr_info("Configuring FLASH for boot-from-NAND\n");
		h225hdk_nor_flash.resource[0].start = emi_bank_base(1);
		h225hdk_nor_flash.resource[0].end = emi_bank_base(2) - 1;
		/* EMI_notCSC */
		h225hdk_nand_flash_data.csn = 0;
		break;
	default:
		pr_info("Invalid boot mode.\n");
	}

	platform_device_register(&h225hdk_nor_flash);

	stx7141_configure_nand(&(struct stm_nand_config) {
					.driver = stm_nand_flex,
					.nr_banks = 1,
					.banks = &h225hdk_nand_flash_data,
					.rbn.flex_connected = 1,});

	stx7141_configure_ssc_i2c(2);
	stx7141_configure_ssc_i2c(3);
	stx7141_configure_ssc_i2c(4);
	stx7141_configure_ssc_i2c(5);
	stx7141_configure_ssc_i2c(6);

	stx7141_configure_usb(0, &(struct stx7141_usb_config) {
		.ovrcur_mode = stx7141_usb_ovrcur_active_low,
		.pwr_enabled = 1 });

	stx7141_configure_usb(1, &(struct stx7141_usb_config) {
		.ovrcur_mode = stx7141_usb_ovrcur_active_low,
		.pwr_enabled = 1 });

         /* The two usb1 only ports are broken on the hardware - missing the
          * pull-down resisitors , which causes them to report spurious
          * device connection. Unfortunately we must therefore disable them */
//	stx7141_configure_usb(2, &(struct stx7141_usb_config) {
//		.ovrcur_mode = stx7141_usb_ovrcur_active_low,
//		.pwr_enabled = 1 });

//	stx7141_configure_usb(3, &(struct stx7141_usb_config) {
//		.ovrcur_mode = stx7141_usb_ovrcur_active_low,
//		.pwr_enabled = 1 });

	if (cpu_data->cut_major > 1)
		stx7141_configure_sata();

	gpio_request(H2257141_PIO_PHY_RESET0, "eth_phy_reset0");
	gpio_direction_output(H2257141_PIO_PHY_RESET0, 1);
	gpio_request(H2257141_PIO_PHY_RESET1, "eth_phy_reset1");
	gpio_direction_output(H2257141_PIO_PHY_RESET1, 1);

#ifdef CONFIG_SH_ST_H2257141_STMMAC0
	/* Configure GMII0 MDINT for active low */
	set_irq_type(ILC_IRQ(43), IRQ_TYPE_LEVEL_LOW);
	stx7141_configure_ethernet(0, &(struct stx7141_ethernet_config) {
		.mode = stx7141_ethernet_mode_mii,
		.phy_bus = 0,
		.phy_addr = STMMAC0_PHY_ADDR,
		.mdio_bus_data = &stmmac0_mdio_bus,
	});
#endif
	stx7141_configure_ethernet(1, &(struct stx7141_ethernet_config) {
		.mode = stx7141_ethernet_mode_gmii,
		.phy_bus = 1,
		.phy_addr = 9,
		.mdio_bus_data = &stmmac1_mdio_bus,
	});


	stx7141_configure_lirc(&(struct stx7141_lirc_config) {
			.rx_mode = stx7141_lirc_rx_disabled,
			.tx_enabled = 1,
			.tx_od_enabled = 1 });

#ifdef CONFIG_SND_STM
	/* Configure Audio */
	i2c_register_board_info(4, h225hdk_av_ctrl,
				ARRAY_SIZE(h225hdk_av_ctrl));

	stx7141_configure_audio(&(struct stx7141_audio_config) {
					pcm_player_1_output_enabled = 1,
					spdif_player_output_enabled = 1,
					pcm_reader_0_input_enabled  = 0,
					pcm_player_1_output_enabled = 0 });

#endif

	return platform_add_devices(h225hdk_devices,
					ARRAY_SIZE(h225hdk_devices));
}
arch_initcall(h225hdk_device_init);

static void __iomem *h225hdk_ioport_map(unsigned long port, unsigned int size)
{
	/*
	 * No IO ports on this device, but to allow safe probing pick
	 * somewhere safe to redirect all reads and writes.
	 */
	return (void __iomem *)CCN_PVR;
}

static void __init h225hdk_init_irq(void)
{
}

struct sh_machine_vector mv_h225hdk __initmv = {
	.mv_name		= "h225hdk",
	.mv_setup		= h225hdk_setup,
	.mv_nr_irqs		= NR_IRQS,
	.mv_init_irq		= h225hdk_init_irq,
	.mv_ioport_map		= h225hdk_ioport_map,
};
