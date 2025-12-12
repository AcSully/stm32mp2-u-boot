// SPDX-License-Identifier: GPL-2.0
/*
 *	Copyright (C) 2024 ALIENTEK - All Rights Reserved
 *  ALIENTEK MIPI LCD : 5.5 inch 720x1280, 1080x1920 and 10.1 inch 800x1280
 *  Alientek MIPI LCD : 5.5 inch use IC HX839X and 10.1 inch use IC ili9881
 */

#include <common.h>
#include <backlight.h>
#include <dm.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <asm/gpio.h>
#include <dm/device_compat.h>
#include <linux/delay.h>
#include <power/regulator.h>

/* User Define command set */
#define UD_SETADDRESSMODE	0x36 /* Set address mode */
#define UD_SETSEQUENCE		0xB0 /* Set sequence */
#define UD_SETPOWER			0xB1 /* Set power */
#define UD_SETDISP			0xB2 /* Set display related register */
#define UD_SETCYC			0xB4 /* Set display waveform cycles */
#define UD_SETVCOM			0xB6 /* Set VCOM voltage */
#define UD_SETTE			0xB7 /* Set internal TE function */
#define UD_SETSENSOR		0xB8 /* Set temperature sensor */
#define UD_SETEXTC			0xB9 /* Set extension command */
#define UD_SETMIPI			0xBA /* Set MIPI control */
#define UD_SETOTP			0xBB /* Set OTP */
#define UD_SETREGBANK		0xBD /* Set register bank */
#define UD_SETDGCLUT		0xC1 /* Set DGC LUT */
#define UD_SETID			0xC3 /* Set ID */
#define UD_SETDDB			0xC4 /* Set DDB */
#define UD_SETCABC			0xC9 /* Set CABC control */
#define UD_SETCABCGAIN		0xCA
#define UD_SETPANEL			0xCC
#define UD_SETOFFSET		0xD2
#define UD_SETGIP0			0xD3 /* Set GIP Option0 */
#define UD_SETGIP1			0xD5 /* Set GIP Option1 */
#define UD_SETGIP2			0xD6 /* Set GIP Option2 */
#define UD_SETGIP3			0xD8 /* Set GIP Option2 */
#define UD_SETGPO			0xD9
#define UD_SETSCALING		0xDD
#define UD_SETIDLE			0xDF
#define UD_SETGAMMA			0xE0 /* Set gamma curve related setting */
#define UD_SETCHEMODE_DYN	0xE4
#define UD_SETCHE			0xE5
#define UD_SETCESEL			0xE6 /* Enable color enhance */
#define UD_SET_SP_CMD		0xE9
#define UD_SETREADINDEX		0xFE /* Set SPI Read Index */
#define UD_GETSPIREAD		0xFF /* SPI Read Command Data */

struct alientek_panel_priv {
	struct udevice *reg;
	struct udevice *backlight;
	struct gpio_desc reset;
};

#if 0
static const struct drm_display_mode atk_mipi_dsi_5x5_720x1280_mode = {
	//ALIENTEK MIPI LCD 720x1280	
	.clock 			= 61000,				/* LCD 像素时钟 */
	.hdisplay 		= 720,					/* LCD X轴像素个数 */
	.hsync_start 	= 720 + 50,				/* LCD X轴+hfp像素个数 */
	.hsync_end 		= 720 + 50 + 8,			/* LCD X轴+hfp+hspw像素个数 */
	.htotal 		= 720 + 50 + 8 + 52,	/* LCD X轴+hfp+hspw+hbp像素个数 */
	.vdisplay 		= 1280,					/* LCD Y轴像素个数 */
	.vsync_start 	= 1280 + 16,			/* LCD Y轴+vfp像素个数 */
	.vsync_end 		= 1280 + 16 + 6,		/* LCD Y轴+vfp+vspw像素个数 */
	.vtotal 		= 1280 + 16 + 6 + 15,	/* LCD Y轴+vfp+vspw+vbp像素个数 */
	.flags 			= DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
};
#endif

static const struct display_timing atk_mipi_dsi_5x5_720x1280_timing = {
    .pixelclock.typ   = 61000000,   /* 61 MHz */
    .hactive.typ      = 720,
    .hfront_porch.typ = 50,
    .hsync_len.typ    = 8,
    .hback_porch.typ  = 52,
    .vactive.typ      = 1280,
    .vfront_porch.typ = 16,
    .vsync_len.typ    = 6,
    .vback_porch.typ  = 15,
};

static void hx839x_dcs_write_buf(struct udevice *dev, const void *data,
				  size_t len)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	int ret;

	ret = mipi_dsi_dcs_write_buffer(device, data, len);
	if (ret < 0)
		dev_err(dev, "MIPI DSI DCS write buffer failed: %d\n", ret);
}

#define dcs_write_seq(ctx, seq...)				\
({								\
	static const u8 d[] = { seq };				\
								\
	hx839x_dcs_write_buf(ctx, d, ARRAY_SIZE(d));		\
})

static void hx839x_init_sequence_720p(struct udevice *dev)
{
	u8 mipi_data[] = {UD_SETMIPI, 0x60, 0x03, 0x68, 0x6B, 0xB2, 0xC0};

	dcs_write_seq(dev, UD_SETADDRESSMODE, 0x01);

	dcs_write_seq(dev, UD_SETEXTC, 0xFF, 0x83, 0x94);

	/* SETMIPI */
	//mipi_data[1] = 0x60 | (dsi->lanes - 1);
	mipi_data[1] = 0x60 | (4 - 1);
	hx839x_dcs_write_buf(dev, mipi_data, ARRAY_SIZE(mipi_data));

	dcs_write_seq(dev, UD_SETPOWER, 0x48, 0x12, 0x72, 0x09, 0x32, 0x54,
		      0x71, 0x71, 0x57, 0x47);

	dcs_write_seq(dev, UD_SETDISP, 0x00, 0x80, 0x64, 0x0C, 0x0D, 0x2F);

	dcs_write_seq(dev, UD_SETCYC, 0x73, 0x74, 0x73, 0x74, 0x73, 0x74, 0x01,
		      0x0C, 0x86, 0x75, 0x00, 0x3F, 0x73, 0x74, 0x73, 0x74,
		      0x73, 0x74, 0x01, 0x0C, 0x86);

	dcs_write_seq(dev, UD_SETGIP0, 0x00, 0x00, 0x07, 0x07, 0x40, 0x07, 0x0C,
		      0x00, 0x08, 0x10, 0x08, 0x00, 0x08, 0x54, 0x15, 0x0A,
		      0x05, 0x0A, 0x02, 0x15, 0x06, 0x05, 0x06, 0x47, 0x44,
		      0x0A, 0x0A, 0x4B, 0x10, 0x07, 0x07, 0x0C, 0x40);

	dcs_write_seq(dev, UD_SETGIP1, 0x1C, 0x1C, 0x1D, 0x1D, 0x00, 0x01, 0x02,
		      0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
		      0x24, 0x25, 0x18, 0x18, 0x26, 0x27, 0x18, 0x18, 0x18,
		      0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		      0x18, 0x18, 0x18, 0x18, 0x20, 0x21, 0x18, 0x18, 0x18,
		      0x18);

	dcs_write_seq(dev, UD_SETGIP2, 0x1C, 0x1C, 0x1D, 0x1D, 0x07, 0x06, 0x05,
		      0x04, 0x03, 0x02, 0x01, 0x00, 0x0B, 0x0A, 0x09, 0x08,
		      0x21, 0x20, 0x18, 0x18, 0x27, 0x26, 0x18, 0x18, 0x18,
		      0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		      0x18, 0x18, 0x18, 0x18, 0x25, 0x24, 0x18, 0x18, 0x18,
		      0x18);

	dcs_write_seq(dev, UD_SETVCOM, 0x6E, 0x6E);

	dcs_write_seq(dev, UD_SETGAMMA, 0x00, 0x0A, 0x15, 0x1B, 0x1E, 0x21,
		      0x24, 0x22, 0x47, 0x56, 0x65, 0x66, 0x6E, 0x82, 0x88,
		      0x8B, 0x9A, 0x9D, 0x98, 0xA8, 0xB9, 0x5D, 0x5C, 0x61,
		      0x66, 0x6A, 0x6F, 0x7F, 0x7F, 0x00, 0x0A, 0x15, 0x1B,
		      0x1E, 0x21, 0x24, 0x22, 0x47, 0x56, 0x65, 0x65, 0x6E,
		      0x81, 0x87, 0x8B, 0x98, 0x9D, 0x99, 0xA8, 0xBA, 0x5D,
		      0x5D, 0x62, 0x67, 0x6B, 0x72, 0x7F, 0x7F);
	dcs_write_seq(dev, 0xC0, 0x1F, 0x31);
	dcs_write_seq(dev, UD_SETPANEL, 0x03);
	dcs_write_seq(dev, 0xD4, 0x02);
	dcs_write_seq(dev, UD_SETREGBANK, 0x02);
	dcs_write_seq(dev, 0xD8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		      0xFF, 0xFF, 0xFF, 0xFF);
	dcs_write_seq(dev, UD_SETREGBANK, 0x00);
	dcs_write_seq(dev, UD_SETREGBANK, 0x01);
	dcs_write_seq(dev, UD_SETPOWER, 0x00);
	dcs_write_seq(dev, UD_SETREGBANK, 0x00);
	dcs_write_seq(dev, 0xBF, 0x40, 0x81, 0x50, 0x00, 0x1A, 0xFC, 0x01);
	dcs_write_seq(dev, 0xC6, 0xED);
}

static int alientek_panel_enable_backlight(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	struct alientek_panel_priv *priv = dev_get_priv(dev);
	int ret;

	ret = mipi_dsi_attach(device);
	if (ret < 0)
		return ret;

	hx839x_init_sequence_720p(dev);

	ret = mipi_dsi_dcs_exit_sleep_mode(device);
	if (ret)
		return ret;

	mdelay(125);

	ret = mipi_dsi_dcs_set_display_on(device);
	if (ret)
		return ret;

	mdelay(20);

	ret = backlight_enable(priv->backlight);
	if (ret)
		return ret;

	return 0;
}

static int alientek_panel_get_display_timing(struct udevice *dev,
					    struct display_timing *timings)
{
	memcpy(timings, &atk_mipi_dsi_5x5_720x1280_timing, sizeof(*timings));

	return 0;
}

static int alientek_panel_of_to_plat(struct udevice *dev)
{
	struct alientek_panel_priv *priv = dev_get_priv(dev);
	int ret;

	if (CONFIG_IS_ENABLED(DM_REGULATOR)) {
		ret =  device_get_supply_regulator(dev, "power-supply",
						   &priv->reg);
		if (ret && ret != -ENOENT) {
			dev_err(dev, "Warning: cannot get power supply\n");
			return ret;
		}
	}

	ret = gpio_request_by_name(dev, "reset-gpios", 0, &priv->reset,
				   GPIOD_IS_OUT);
	if (ret) {
		dev_err(dev, "Warning: cannot get reset GPIO\n");
		if (ret != -ENOENT)
			return ret;
	}

	ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev,
					   "backlight", &priv->backlight);
	if (ret) {
		dev_err(dev, "Cannot get backlight: ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static int alientek_panel_probe(struct udevice *dev)
{
	struct alientek_panel_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	int ret;

	if (CONFIG_IS_ENABLED(DM_REGULATOR) && priv->reg) {
		ret = regulator_set_enable(priv->reg, true);
		if (ret)
			return ret;
	}

	/* reset panel */
	dm_gpio_set_value(&priv->reset, true);
	mdelay(1);
	dm_gpio_set_value(&priv->reset, false);
	mdelay(10);

	/* fill characteristics of DSI data link */
	plat->lanes = 4;
	plat->format = MIPI_DSI_FMT_RGB888;
	plat->mode_flags = MIPI_DSI_MODE_VIDEO |
			   MIPI_DSI_MODE_VIDEO_BURST |
			   MIPI_DSI_MODE_LPM |
			   MIPI_DSI_MODE_EOT_PACKET;

	return 0;
}

static const struct panel_ops alientek_panel_ops = {
	.enable_backlight = alientek_panel_enable_backlight,
	.get_display_timing = alientek_panel_get_display_timing,
};

static const struct udevice_id alientek_panel_ids[] = {
	{ .compatible = "alientek,mipi-lcd" },
	{ }
};

U_BOOT_DRIVER(alientek_panel) = {
	.name			  = "alientek_panel",
	.id			  = UCLASS_PANEL,
	.of_match		  = alientek_panel_ids,
	.ops			  = &alientek_panel_ops,
	.of_to_plat	  = alientek_panel_of_to_plat,
	.probe			  = alientek_panel_probe,
	.plat_auto	= sizeof(struct mipi_dsi_panel_plat),
	.priv_auto	= sizeof(struct alientek_panel_priv),
};
