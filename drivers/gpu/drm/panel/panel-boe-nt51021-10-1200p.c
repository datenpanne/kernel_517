// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2022 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct boe_nt51021_10_1200p {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator *lcd_vsp;  // as power supply
	struct regulator *lcd_vsn;  // as power supply
        //struct regulator *lcdanalogvcc ; // (vdd) lcd_vci (3v3/2v85?)
        struct regulator *lcd_iovcc ; // 32 (vddio) lcdio_vcc 1v8
        //struct regulator *lcdbias;  // vled; 97
        struct regulator *backlight;  // bkl_gpio; // 109
	struct gpio_desc *reset_gpio;

	bool prepared;
	bool enabled;
};

#define HW_NT51021_VND_MIPI 0x8f
#define HW_NT51021_VND_INDEX0 0x83
#define HW_NT51021_VND_INDEX1 0x84
#define HW_NT51021_VND_GOP 0x8c
#define HW_NT51021_VND_3DUMMY 0xcd
#define HW_NT51021_VND_DCH_C0 0xc0
#define HW_NT51021_VND_DCH_C8 0xc8
#define HW_NT51021_VND_TERMRESIST 0x97
#define HW_NT51021_VND_TESTMODE1 0x85
#define HW_NT51021_VND_TESTMODE2 0x86
#define HW_NT51021_VND_TESTMODE3 0x9c
#define HW_NT51021_VND_HSYNC 0x8b

#define HW_NT51021_SET_DISPLAY_BRIGHTNESS 0x9f
#define HW_NT51021_VND_A1 0xa1
#define HW_NT51021_VND_A2 0xa2
#define HW_NT51021_VND_A3 0xa3
#define HW_NT51021_VND_A4 0xa4
#define HW_NT51021_VND_A5 0xa5
#define HW_NT51021_VND_A6 0xa6
#define HW_NT51021_VND_A7 0xa7
#define HW_NT51021_VND_A8 0xa8
#define HW_NT51021_VND_A9 0xa9
#define HW_NT51021_VND_AA 0xaa
#define HW_NT51021_VND_AD 0xAD
#define HW_NT51021_VND_B4 0xb4
#define HW_NT51021_VND_B5 0xb5
#define HW_NT51021_VND_B6 0xb6
#define HW_NT51021_VND_C0 0xC0
#define HW_NT51021_VND_9A 0x9a
#define HW_NT51021_VND_91 0x91
#define HW_NT51021_VND_96 0x96
#define HW_NT51021_VND_99 0x99
#define HW_NT51021_VND_9F 0x9f
#define HW_NT51021_CABC_MODE 0x90 // 0xc0 off | 0x00 on
#define HW_NT51021_CABC_STILL_DIMMING 0x95 // 0xB0 off | 0x60 on
#define HW_NT51021_CABC_MOVING_DIMMING 0x94 // 0x18

static inline
struct boe_nt51021_10_1200p *to_boe_nt51021_10_1200p(struct drm_panel *panel)
{
	return container_of(panel, struct boe_nt51021_10_1200p, panel);
}

#define dsi_generic_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_generic_write(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)


#define dsi_dcs_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_dcs_write_buffer(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)

static void boe_nt51021_10_1200p_reset(struct boe_nt51021_10_1200p *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(1000, 2000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	msleep(20);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(30);
}

static void boe_nt51021_10_1200p_pwr_en(struct boe_nt51021_10_1200p *ctx, int enabled)
{
	int ret;

	ret = regulator_enable(ctx->lcd_vsp);
	if (ret < 0)
		return ret;
	ret = regulator_enable(ctx->lcd_vsn);
	if (ret < 0)
		return ret;
	ret = regulator_enable(ctx->backlight);
	if (ret < 0)
		return ret;
	ret = regulator_enable(ctx->lcd_iovcc);
	if (ret < 0)
		return ret;
	/*ret = regulator_enable(ctx->vsn);
	if (ret < 0)
		return ret;*/
}

static int boe_nt51021_10_1200p_init(struct boe_nt51021_10_1200p *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	dsi_dcs_write_seq(dsi, HW_NT51021_VND_MIPI, 0xa5); // MIPI enable command interface

	/*ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_MIPI, (u8[]){ 0xa5 }, 1);
	if (ret < 0)
		return ret; // i2c --> MIPI*/
        msleep(5);
	ret = mipi_dsi_dcs_soft_reset(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to do Software Reset: %d\n", ret);
		return ret;
	}
	msleep(20);
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_MIPI, 0xa5); // MIPI enable command interface
    msleep(5);
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_INDEX0, 0x00); // command page 0
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_INDEX1, 0x00); // command page 0
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_GOP, 0x80); // GIP unlock OTP
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_3DUMMY, 0x6c); // 3Dummy
	//dsi_dcs_write_seq(dsi, HW_NT51021_VND_DCH_C0, 0x8b); // GCH
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_DCH_C8, 0xf0); // GCH DC improves high temperature interlaced display
	dsi_dcs_write_seq(dsi, HW_NT51021_SET_DISPLAY_BRIGHTNESS, 0x00); // backlight control?
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_TERMRESIST, 0x00); // matching terminal resistance 100 ohms
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_HSYNC, 0x10); // H-sync unlick OTP
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_A9, 0x20); // ajust the hold time H-sync high level/enable TP_SYNC

	dsi_dcs_write_seq(dsi, HW_NT51021_VND_INDEX0, 0xaa); // command page 1
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_INDEX1, 0x11); // command page 1
    dsi_dcs_write_seq(dsi, HW_NT51021_VND_A9, 0x4b); // IC MIPI Rx drive current capability gear 85%
    dsi_dcs_write_seq(dsi, HW_NT51021_VND_TESTMODE1, 0x04); // test mode 1
    dsi_dcs_write_seq(dsi, HW_NT51021_VND_TESTMODE2, 0x08); // test mode 2
    dsi_dcs_write_seq(dsi, HW_NT51021_VND_TESTMODE3, 0x10); // test mode 3

	dsi_dcs_write_seq(dsi, HW_NT51021_VND_INDEX0, 0xbb); // command page 2
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_INDEX1, 0x22); // command page 2
	dsi_dcs_write_seq(dsi, HW_NT51021_CABC_MODE, 0xC0); // cabc off

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
    }
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_MIPI, 0x00); //exit MIPI Command interface
	msleep(5);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}

	return 0;
}

static int boe_nt51021_10_1200p_on(struct boe_nt51021_10_1200p *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;
	
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_INDEX0, 0x00); // command page 0
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_INDEX1, 0x00); // command page 0
	dsi_dcs_write_seq(dsi, 0x29, 0x00);
	msleep(20);

	return 0;
}

static int boe_nt51021_10_1200p_off(struct boe_nt51021_10_1200p *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;
	
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_INDEX0, 0x00); // command page 0
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_INDEX1, 0x00); // command page 0
	dsi_dcs_write_seq(dsi, 0x28, 0x00);
	msleep(20);
	dsi_dcs_write_seq(dsi, 0x10, 0x00);
	msleep(100);

	return 0;
}

static int boe_nt51021_10_1200p_prepare(struct drm_panel *panel)
{
	struct boe_nt51021_10_1200p *ctx = to_boe_nt51021_10_1200p(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

        boe_nt51021_10_1200p_pwr_en(ctx, 1);

        boe_nt51021_10_1200p_reset(ctx);

	ret = boe_nt51021_10_1200p_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
		return ret;
	}

	ctx->prepared = true;
	return 0;
}

static int boe_nt51021_10_1200p_enable(struct drm_panel *panel)
{
	if (ctx->enabled)
		return 0;

	msleep(130);

	ctx->enabled = true;
	return 0;
}

static int boe_nt51021_10_1200p_unprepare(struct drm_panel *panel)
{
	struct boe_nt51021_10_1200p *ctx = to_boe_nt51021_10_1200p(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->prepared)
		return 0;

	ret = boe_nt51021_10_1200p_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
        boe_nt51021_10_1200p_pwr_en(ctx, 1);

	ctx->prepared = false;
	return 0;
}

static int boe_nt51021_10_1200p_disable(struct drm_panel *panel)
{
	if (!ctx->enabled)
		return 0;

	msleep(130);

	ctx->enabled = false;
	return 0;
}

static const struct drm_display_mode boe_nt51021_10_1200p_mode = {
	.clock = (1200 + 64 + 4 + 36) * (1920 + 104 + 2 + 24) * 60 / 1000,
	.hdisplay = 1200,
	.hsync_start = 1200 + 64,
	.hsync_end = 1200 + 64 + 4,
	.htotal = 1200 + 64 + 4 + 36,
	.vdisplay = 1920,
	.vsync_start = 1920 + 104,
	.vsync_end = 1920 + 104 + 2,
	.vtotal = 1920 + 104 + 2 + 24,
	.width_mm = 135,
	.height_mm = 217,
};

static int boe_nt51021_10_1200p_get_modes(struct drm_panel *panel,
					  struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &boe_nt51021_10_1200p_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs boe_nt51021_10_1200p_panel_funcs = {
	.prepare = boe_nt51021_10_1200p_prepare,
	.unprepare = boe_nt51021_10_1200p_unprepare,
	.enable = boe_nt51021_10_1200p_enable,
	.disable = boe_nt51021_10_1200p_disable,
	.get_modes = boe_nt51021_10_1200p_get_modes,
};

/*static int boe_nt51021_10_1200p_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness(dsi, brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

static const struct backlight_ops boe_nt51021_10_1200p_bl_ops = {
	.update_status = boe_nt51021_10_1200p_bl_update_status,
};*/

static int hw_mipi_dsi_dcs_set_level(struct mipi_dsi_device *dsi,
					u16 brightness)
{
	/*struct hw_nt51021 *ctx;
	struct device *dev = &dsi->dev;*/
	u8 payload[2] = { brightness & 0xff, brightness >> 8 };
	ssize_t err;
    int ret;

	//hw_nt51021_bkl_en(&ctx->dsi);
	ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_MIPI, (u8[]){ 0xa5 }, 1);
	if (ret < 0)
		return ret; // MIPI enable command
	ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_INDEX0, (u8[]){ 0x00 }, 1);
	if (ret < 0)
		return ret; //hw boe
	ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_INDEX1, (u8[]){ 0x00 }, 1);
	if (ret < 0)
		return ret;

	err = mipi_dsi_dcs_write(dsi, HW_NT51021_SET_DISPLAY_BRIGHTNESS,
				 payload, sizeof(payload));
      /*err = mipi_dsi_dcs_write(dsi, hw_nt51021_bkl_en,
				 payload, sizeof(payload));*/
	if (err < 0)
		return err;

	return 0;
}

static int boe_nt51021_10_1200p_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	struct boe_nt51021_10_1200p *ctx = mipi_dsi_get_drvdata(dsi);
	struct device *dev = &ctx->dsi->dev;
	//u16 level = backlight_get_brightness(bl);
	u16 brightness = bl->props.brightness;
	int ret;

    //gpiod_set_value_cansleep(ctx->blen_gpio, !!brightness);
    //mutex_lock(&ctx->mutex);
    //hw_nt51021_bias_en(ctx, 1);

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM; // HS_Mode

	/*ret = hw_nt51021_bkl_cmd(ctx);
	if (ret)
		return ret;*/

    ret = hw_mipi_dsi_dcs_set_level(dsi, brightness);
	if (ret < 0)
		return ret;

	/*dev_dbg(dev, "set brightness %d\n", brightness);
	ret = mipi_dsi_dcs_write(dsi, HW_NT51021_SET_DISPLAY_BRIGHTNESS,
				 brightness,
				 sizeof(brightness));*/
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;
    //mutex_unlock(&ctx->mutex);

	return 0;
}

static const struct backlight_ops boe_nt51021_10_1200p_bl_ops = {
	.update_status = boe_nt51021_10_1200p_bl_update_status,
};

static struct backlight_device *
boe_nt51021_10_1200p_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 128,
		.max_brightness = 255,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &boe_nt51021_10_1200p_bl_ops, &props);
}

static int boe_nt51021_10_1200p_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct boe_nt51021_10_1200p *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
		
	ctx->lcd_vsp = devm_regulator_get(dev, "vsp");
	if (IS_ERR(ctx->lcd_vsp))
		return PTR_ERR(ctx->lcd_vsp);

	ctx->lcd_vsn = devm_regulator_get(dev, "vsn");
	if (IS_ERR(ctx->lcd_vsn))
		return PTR_ERR(ctx->lcd_vsn);

	ctx->lcd_iovcc = devm_regulator_get(dev, "lcdiovcc");
	if (IS_ERR(ctx->lcd_iovcc))
		return PTR_ERR(ctx->lcd_iovcc);

	ctx->backlight = devm_regulator_get(dev, "backlight");
	if (IS_ERR(ctx->backlight))
		return PTR_ERR(ctx->backlight);
	/*ctx->supplies[0].supply = "vcc";
	ctx->supplies[1].supply = "vdd";
	ctx->supplies[2].supply = "bl_en";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies),
				      ctx->supplies);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to get regulators\n");*/

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_NO_EOT_PACKET |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS;

	drm_panel_init(&ctx->panel, dev, &boe_nt51021_10_1200p_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ctx->panel.backlight = boe_nt51021_10_1200p_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		drm_panel_remove(&ctx->panel);
		return ret;
	}

	return 0;
}

static int boe_nt51021_10_1200p_remove(struct mipi_dsi_device *dsi)
{
	struct boe_nt51021_10_1200p *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id boe_nt51021_10_1200p_of_match[] = {
	{ .compatible = "boe,nt51021-10-1200p" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, boe_nt51021_10_1200p_of_match);

static struct mipi_dsi_driver boe_nt51021_10_1200p_driver = {
	.probe = boe_nt51021_10_1200p_probe,
	.remove = boe_nt51021_10_1200p_remove,
	.driver = {
		.name = "panel-boe-nt51021-10-1200p",
		.of_match_table = boe_nt51021_10_1200p_of_match,
	},
};
module_mipi_dsi_driver(boe_nt51021_10_1200p_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for BOE_NT51021_10_1200P_VIDEO");
MODULE_LICENSE("GPL v2");
