/*
** Copyright (C) 2022 CNflysky. All rights reserved.
** Kernel DRM driver for Multiple Panels in DSI interface.
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

struct alientek_panel_priv {
	struct udevice *reg;
	struct udevice *backlight;
	struct gpio_desc reset;
};

static const struct display_timing w280bf036i_timing = {
    .pixelclock.typ   = 22572000,   /* 22.5 MHz */

    .hactive.typ      = 480,
    .hfront_porch.typ = 30,
    .hsync_len.typ    = 10,
    .hback_porch.typ  = 30,

    .vactive.typ      = 640,
    .vfront_porch.typ = 20,
    .vsync_len.typ    = 4,
    .vback_porch.typ  = 20,

    /*.flags = DISPLAY_FLAGS_DE_LOW |
             DISPLAY_FLAGS_HSYNC_LOW |
             DISPLAY_FLAGS_VSYNC_LOW,*/
};

static inline int panel_dsi_write(struct mipi_dsi_device *dsi, const void *seq,
                                  size_t len) {
  return mipi_dsi_dcs_write_buffer(dsi, seq, len);
}

#define panel_command(dsi, seq...)          \
  {                                         \
    const uint8_t d[] = {seq};              \
    panel_dsi_write(dsi, d, ARRAY_SIZE(d)); \
  }

static void w280bf036i_init_sequence(struct mipi_dsi_device *dsi) {
  // Command2 BK3 Selection: Enable the BK function of Command2
  panel_command(dsi, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x13);
  // Unknown
  panel_command(dsi, 0xEF, 0x08);
  // Command2 BK0 Selection: Disable the BK function of Command2
  panel_command(dsi, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x10);
  // Display Line Setting
  panel_command(dsi, 0xC0, 0x4f, 0x00);
  // Porch Control
  panel_command(dsi, 0xC1, 0x10, 0x0c);
  // Inversion selection & Frame Rate Control
  panel_command(dsi, 0xC2, 0x07, 0x14);
  // Unknown
  panel_command(dsi, 0xCC, 0x10);
  // Positive Voltage Gamma Control
  panel_command(dsi, 0xB0, 0x0a, 0x18, 0x1e, 0x12, 0x16,
				     0x0c, 0x0e, 0x0d, 0x0c, 0x29, 0x06, 0x14,
				     0x13, 0x29, 0x33, 0x1c);
  // Negative Voltage Gamma Control
  panel_command(dsi, 0xB1, 0x0a, 0x19, 0x21, 0x0a, 0x0c,
				     0x00, 0x0c, 0x03, 0x03, 0x23, 0x01, 0x0e,
				     0x0c, 0x27, 0x2b, 0x1c);

  // Command2 BK1 Selection: Enable the BK function of Command2
  panel_command(dsi, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11);
  // Vop Amplitude setting
  panel_command(dsi, 0xB0, 0x5d);
  // VCOM amplitude setting
  panel_command(dsi, 0xB1, 0x61);
  // VGH Voltage setting
  panel_command(dsi, 0xB2, 0x84);
  // TEST Command Setting
  panel_command(dsi, 0xB3, 0x80);
  // VGL Voltage setting
  panel_command(dsi, 0xB5, 0x4d);
  // Power Control 1
  panel_command(dsi, 0xB7, 0x85);
  // Power Control 2
  panel_command(dsi, 0xB8, 0x20);
  // Source pre_drive timing set1
  panel_command(dsi, 0xC1, 0x78);
  // Source EQ2 Setting
  panel_command(dsi, 0xC2, 0x78);
  // MIPI Setting 1
  panel_command(dsi, 0xD0, 0x88);
  // GIP Code
  panel_command(dsi, 0xE0, 0x00, 0x00, 0x02);
  panel_command(dsi, 0xE1, 0x06, 0xa0, 0x08, 0xa0, 0x05,
				     0xa0, 0x07, 0xa0, 0x00, 0x44, 0x44);
  panel_command(dsi, 0xE2, 0x20, 0x20, 0x44, 0x44, 0x96,
				     0xa0, 0x00, 0x00, 0x96, 0xa0, 0x00, 0x00);
  panel_command(dsi, 0xE3, 0x00, 0x00, 0x22, 0x22);
  panel_command(dsi, 0xE4, 0x44, 0x44);
  panel_command(dsi, 0xE5, 0x0d, 0x91, 0xa0, 0xa0, 0x0f,
				     0x93, 0xa0, 0xa0, 0x09, 0x8d, 0xa0, 0xa0,
				     0x0b, 0x8f, 0xa0, 0xa0);
  panel_command(dsi, 0xE6, 0x00, 0x00, 0x22, 0x22);
  panel_command(dsi, 0xE7, 0x44, 0x44);
  panel_command(dsi, 0xE8, 0x0c, 0x90, 0xa0, 0xa0, 0x0e,
				     0x92, 0xa0, 0xa0, 0x08, 0x8c, 0xa0, 0xa0,
				     0x0a, 0x8e, 0xa0, 0xa0);
  panel_command(dsi, 0xE9, 0x36, 0x00);
  panel_command(dsi, 0xEB, 0x00, 0x01, 0xe4, 0xe4, 0x44,
				     0x88, 0x40);
  panel_command(dsi, 0xED, 0xff, 0x45, 0x67, 0xfa, 0x01,
				     0x2b, 0xcf, 0xff, 0xff, 0xfc, 0xb2, 0x10,
				     0xaf, 0x76, 0x54, 0xff);
  panel_command(dsi, 0xEF, 0x10, 0x0d, 0x04, 0x08, 0x3f,
				     0x1f);
  // disable Command2
  panel_command(dsi, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x00);
  u8 value = MIPI_DSI_DCS_TEAR_MODE_VBLANK;
  mipi_dsi_dcs_write(dsi, MIPI_DCS_SET_TEAR_ON, &value,
				 sizeof(value));
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
	//dm_gpio_set_value(&priv->reset, false);
	//mdelay(20);
	//dm_gpio_set_value(&priv->reset, true);
	//mdelay(20);
	//dm_gpio_set_value(&priv->reset, false);
	//mdelay(120);
	
	mipi_dsi_dcs_soft_reset(device);
	mdelay(120);
	w280bf036i_init_sequence(device);

	ret = mipi_dsi_dcs_exit_sleep_mode(device);
	if (ret)
		return ret;

	mdelay(120);

	ret = mipi_dsi_dcs_set_display_on(device);
	if (ret)
		return ret;

	mdelay(20);

	ret = backlight_enable(priv->backlight);
	if (ret)
		return ret;
	
	//printf("mipi 1 lane alientek_panel_enable_backlight()\n");
	return 0;
}

static int alientek_panel_get_display_timing(struct udevice *dev,
					    struct display_timing *timings)
{
	memcpy(timings, &w280bf036i_timing, sizeof(*timings));
	//printf("mipi 1 lane get timing\n");
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
	
	//printf("mipi 1 lane alientek_panel_of_to_plat()\n");
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
	//dm_gpio_set_value(&priv->reset, false);
	//mdelay(20);
	// dm_gpio_set_value(&priv->reset, true);
	// mdelay(20);
	// dm_gpio_set_value(&priv->reset, false);
	// mdelay(120);
		
	/* fill characteristics of DSI data link */
	plat->lanes = 1;
	plat->format = MIPI_DSI_FMT_RGB888;
	plat->mode_flags = MIPI_DSI_MODE_VIDEO |
			   MIPI_DSI_MODE_VIDEO_BURST |
			   MIPI_DSI_MODE_LPM |
			   MIPI_DSI_MODE_EOT_PACKET;
	
	//printf("mipi 1 lane alientek_panel_probe()\n");
	return 0;
}

static const struct panel_ops alientek_panel_ops = {
	.enable_backlight = alientek_panel_enable_backlight,
	.get_display_timing = alientek_panel_get_display_timing,
};

static const struct udevice_id dsi_panel_of_match[] = {
    {.compatible = "wlk,w280bf036i"},
    {}
};

U_BOOT_DRIVER(alientek_panel_w280bf036i) = {
	.name			  = "alientek_panel_w280bf036i",
	.id			  = UCLASS_PANEL,
	.of_match		  = dsi_panel_of_match,
	.ops			  = &alientek_panel_ops,
	.of_to_plat	  = alientek_panel_of_to_plat,
	.probe			  = alientek_panel_probe,
	.plat_auto	= sizeof(struct mipi_dsi_panel_plat),
	.priv_auto	= sizeof(struct alientek_panel_priv),
};

