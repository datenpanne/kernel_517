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

struct hw_nt51021 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data supplies[3];
	struct gpio_desc *reset_gpio;
	struct gpio_desc *disp_power_backlight;
	struct gpio_desc *disp_power_panel;
	struct gpio_desc *disp_en_gpio_vled;

	bool prepared;
	bool enabled;
};

#define HW_NT51021_VND_A1 0xa1
#define HW_NT51021_VND_A2 0xa2
#define HW_NT51021_VND_A8 0xa8
#define HW_NT51021_VND_A9 0xa9
#define HW_NT51021_INDEX0 0x83
#define HW_NT51021_INDEX1 0x84
#define HW_NT51021_SET_DISPLAY_BRIGHTNESS 0x9f
#define HW_NT51021_CABC_MODE 0x90 // 0xc0 off| 0x00 on
#define HW_NT51021_CABC_STILL_DIMMING 0x95 // 0xB0 off| 0x60 on
#define HW_NT51021_CABC_MOVING_DIMMING 0x94 // 0x18

static inline
struct hw_nt51021 *to_hw_nt51021(struct drm_panel *panel)
{
	return container_of(panel, struct hw_nt51021, panel);
}

#define dsi_dcs_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_dcs_write_buffer(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)

static void hw_nt51021_reset(struct hw_nt51021 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(1000, 2000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	msleep(20);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(30);
}

/*static void hw_nt51021_pwr_en(struct hw_nt51021 *ctx)
{
	//gpiod_set_value_cansleep(ctx->vled_gpio, 1);
	//msleep(5);

	gpiod_set_value_cansleep(ctx->blen_gpio, 1);
	gpiod_set_value_cansleep(ctx->vcc_gpio, 1);
	msleep(500);
}

static void hw_nt51021_pwr_dis(struct hw_nt51021 *ctx)
{
	gpiod_set_value_cansleep(ctx->vled_gpio, 0);
	msleep(5);
	gpiod_set_value_cansleep(ctx->blen_gpio, 0);
	gpiod_set_value_cansleep(ctx->vcc_gpio, 0);
	msleep(500);

	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	msleep(2);
	//regulator_disable(ctx->vsp);
    //regulator_disable(ctx->vsn);
	//usleep_range(5000, 7000);
}*/

static int hw_nt51021_init(struct hw_nt51021 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	/*gpiod_set_value_cansleep(ctx->vcc_gpio, 1);
	gpiod_set_value_cansleep(ctx->blen_gpio, 1);
	msleep(30);*/


	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	dsi_dcs_write_seq(dsi, HW_NT51021_INDEX0, 0x00);
	dsi_dcs_write_seq(dsi, HW_NT51021_INDEX1, 0x00);
	dsi_dcs_write_seq(dsi, 0x8c, 0x80);
	dsi_dcs_write_seq(dsi, 0xcd, 0x6c);
	dsi_dcs_write_seq(dsi, 0xc8, 0xfc);
	dsi_dcs_write_seq(dsi, HW_NT51021_SET_DISPLAY_BRIGHTNESS, 0x00);
	dsi_dcs_write_seq(dsi, 0x97, 0x00);
	dsi_dcs_write_seq(dsi, HW_NT51021_INDEX0, 0xbb);
	dsi_dcs_write_seq(dsi, HW_NT51021_INDEX1, 0x22);
	dsi_dcs_write_seq(dsi, 0x96, 0x00);
	dsi_dcs_write_seq(dsi, HW_NT51021_CABC_MODE, 0xc0);
	dsi_dcs_write_seq(dsi, 0x91, 0xa0);
	dsi_dcs_write_seq(dsi, 0x9a, 0x10);
	dsi_dcs_write_seq(dsi, HW_NT51021_CABC_MOVING_DIMMING, 0x78);
	dsi_dcs_write_seq(dsi, HW_NT51021_CABC_STILL_DIMMING, 0xb1);
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_A1, 0xff);
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_A2, 0xfa);
	dsi_dcs_write_seq(dsi, 0xa3, 0xf3);
	dsi_dcs_write_seq(dsi, 0xa4, 0xed);
	dsi_dcs_write_seq(dsi, 0xa5, 0xe7);
	dsi_dcs_write_seq(dsi, 0xa6, 0xe2);
	dsi_dcs_write_seq(dsi, 0xa7, 0xdc);
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_A8, 0xd7);
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_A9, 0xd1);
	dsi_dcs_write_seq(dsi, 0xaa, 0xcc);
	dsi_dcs_write_seq(dsi, 0xb4, 0x1c);
	dsi_dcs_write_seq(dsi, 0xb5, 0x38);
	dsi_dcs_write_seq(dsi, 0xb6, 0x30);
	dsi_dcs_write_seq(dsi, HW_NT51021_INDEX0, 0xaa);
	dsi_dcs_write_seq(dsi, HW_NT51021_INDEX1, 0x11);
	dsi_dcs_write_seq(dsi, HW_NT51021_VND_A9, 0x4b);
	dsi_dcs_write_seq(dsi, 0x85, 0x04);
	dsi_dcs_write_seq(dsi, 0x86, 0x08);
	dsi_dcs_write_seq(dsi, 0x9c, 0x10);
	dsi_dcs_write_seq(dsi, HW_NT51021_INDEX0, 0x00);
	dsi_dcs_write_seq(dsi, HW_NT51021_INDEX1, 0x00);

	/*ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}*/

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}

	return 0;
}

static int hw_nt51021_on(struct hw_nt51021 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM; // HS_Mode

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}

	return 0;
}

static int hw_nt51021_off(struct hw_nt51021 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM; // HS_Mode

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}
	msleep(50);

	dsi_dcs_write_seq(dsi, 0x10, 0x00);
	msleep(100);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(100);

	//hw_nt51021_pwr_dis(ctx);

	return 0;
}

static int hw_nt51021_cabc_off(struct hw_nt51021 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;

    static char cabc_still_dimming_on[2] = {HW_NT51021_CABC_STILL_DIMMING, 0x60};
    static char cabc_mode_off[2] = {HW_NT51021_CABC_MODE, 0xc0};
    static char chang_page2_index0[2] = {HW_NT51021_INDEX0, 0xBB};
    static char chang_page2_index1[2] = {HW_NT51021_INDEX1, 0x22};
	int ret;

	ret = mipi_dsi_dcs_write_buffer(dsi, chang_page2_index0,
					ARRAY_SIZE(chang_page2_index0));
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dcs_write_buffer(dsi, chang_page2_index1,
					ARRAY_SIZE(chang_page2_index1));
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dcs_write_buffer(dsi, cabc_still_dimming_on,
					ARRAY_SIZE(cabc_still_dimming_on));
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dcs_write_buffer(dsi, cabc_mode_off,
					ARRAY_SIZE(cabc_mode_off));
	if (ret < 0)
		return ret;
	return 0;
}

static int hw_nt51021_prepare(struct drm_panel *panel)
{
	struct hw_nt51021 *ctx = to_hw_nt51021(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}
    mdelay(5);

	gpiod_set_value_cansleep(ctx->disp_power_backlight, 1);
	gpiod_set_value_cansleep(ctx->disp_power_panel, 1);
	msleep(500);

	hw_nt51021_reset(ctx);

	ret = hw_nt51021_init(ctx);

	ret = hw_nt51021_cabc_off(ctx);
	if (ret)
		return ret;

	ret = hw_nt51021_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
		return ret;
	}

	ctx->prepared = true;
	return 0;
}

static int hw_nt51021_enable(struct drm_panel *panel)
//static int hw_nt51021_enable(struct hw_nt51021 *ctx)
{
	struct hw_nt51021 *ctx = to_hw_nt51021(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->enabled)
		return 0;

    ret = hw_nt51021_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to enable panel: %d\n", ret);
		return ret;
	}
	msleep(20);

	gpiod_set_value(ctx->disp_en_gpio_vled, 1);
	msleep(200);

	ctx->enabled = true;

	return 0;
}

static int hw_nt51021_disable(struct drm_panel *panel)
{
	struct hw_nt51021 *ctx = to_hw_nt51021(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->enabled)
		return 0;

	ret = hw_nt51021_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value(ctx->disp_en_gpio_vled, 0);
	msleep(200);

	ctx->enabled = false;
	return 0;
}

static int hw_nt51021_unprepare(struct drm_panel *panel)
{
	struct hw_nt51021 *ctx = to_hw_nt51021(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (!ctx->prepared)
		return 0;

	ret = hw_nt51021_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	ctx->prepared = false;
	return 0;
}

static const struct drm_display_mode hw_nt51021_mode = {
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

static int hw_nt51021_get_modes(struct drm_panel *panel,
					  struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &hw_nt51021_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs hw_nt51021_panel_funcs = {
	.prepare = hw_nt51021_prepare,
	.enable = hw_nt51021_enable,
	.disable = hw_nt51021_disable,
	.unprepare = hw_nt51021_unprepare,
	.get_modes = hw_nt51021_get_modes,
};

static int hw_nt51021_bkl_en(struct hw_nt51021 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;

    static char brightness[2] = {HW_NT51021_SET_DISPLAY_BRIGHTNESS, 0x00};
    static char chang_page0_index0[2] = {HW_NT51021_INDEX0, 0x00};
    static char chang_page0_index1[2] = {HW_NT51021_INDEX1, 0x00};
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM; // HS_Mode

	ret = mipi_dsi_dcs_write_buffer(dsi, chang_page0_index0,
					ARRAY_SIZE(chang_page0_index0));
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dcs_write_buffer(dsi, chang_page0_index1,
					ARRAY_SIZE(chang_page0_index1));
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dcs_write_buffer(dsi, brightness,
					ARRAY_SIZE(brightness));
	if (ret < 0)
		return ret;

	return 0;
}

static int hw_mipi_dsi_dcs_set_display_brightness(struct mipi_dsi_device *dsi,
					u16 brightness)
{
	//struct device *dev = &dsi->dev;
	struct boe_nt51021_10_1200p *ctx;
	u8 payload[2] = { brightness & 0xff, brightness >> 8 };
	ssize_t err;

    //hw_nt51021_bkl(ctx);

	err = mipi_dsi_dcs_write(dsi, HW_NT51021_SET_DISPLAY_BRIGHTNESS,
				 payload, sizeof(payload));
	if (err < 0)
		return err;

	return 0;
}

static int hw_nt51021_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	struct hw_nt51021 *ctx = mipi_dsi_get_drvdata(dsi);
	struct device *dev = &ctx->dsi->dev;
	u16 brightness = backlight_get_brightness(bl);
	//u8 brightness = bl->props.brightness;
	int ret;

    //gpiod_set_value_cansleep(ctx->blen_gpio, !!brightness);

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM; // HS_Mode

	/*ret = hw_nt51021_bias_enable(ctx);
	if (ret)
		return ret;*/

	ret = hw_nt51021_bkl_en(ctx);
	if (ret)
		return ret;
    
	dev_dbg(dev, "set brightness %d\n", brightness);
	ret = mipi_dsi_dcs_write(dsi, HW_NT51021_SET_DISPLAY_BRIGHTNESS,
				 brightness,
				 sizeof(brightness));
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

static int hw_nt51021_bl_get_intensity(struct backlight_device *bl)
{
	return backlight_get_brightness(bl);
}

static const struct backlight_ops hw_nt51021_bl_ops = {
	.update_status = hw_nt51021_bl_update_status,
	.get_brightness = hw_nt51021_bl_get_intensity,
};

static struct backlight_device *
hw_nt51021_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 128,
		.max_brightness = 255,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &hw_nt51021_bl_ops, &props);
}

static int hw_nt51021_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct hw_nt51021 *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->supplies[0].supply = "vsp";
	ctx->supplies[1].supply = "vsn";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies),
				      ctx->supplies);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to get regulators\n");

	ctx->disp_power_panel = devm_gpiod_get(dev, "panel", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->disp_power_panel))
		return dev_err_probe(dev, PTR_ERR(ctx->disp_power_panel),
				     "Failed to get panel-gpios\n");

	ctx->disp_en_gpio_vled = devm_gpiod_get(dev, "enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->disp_en_gpio_vled))
		return dev_err_probe(dev, PTR_ERR(ctx->disp_en_gpio_vled),
				     "Failed to get enable-gpios\n");

	ctx->disp_power_backlight = devm_gpiod_get(dev, "backlight", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->disp_power_backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->disp_power_backlight),
				     "Failed to get backlight-gpios\n");

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

	drm_panel_init(&ctx->panel, dev, &hw_nt51021_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ctx->panel.backlight = hw_nt51021_create_backlight(dsi);
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

static int hw_nt51021_remove(struct mipi_dsi_device *dsi)
{
	struct hw_nt51021 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id hw_nt51021_of_match[] = {
	{ .compatible = "boe,nt51021-10-1200p" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, hw_nt51021_of_match);

static struct mipi_dsi_driver hw_nt51021_driver = {
	.probe = hw_nt51021_probe,
	.remove = hw_nt51021_remove,
	.driver = {
		.name = "panel-boe-nt51021-10-1200p",
		.of_match_table = hw_nt51021_of_match,
	},
};
module_mipi_dsi_driver(hw_nt51021_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for hw_nt51021_VIDEO");
MODULE_LICENSE("GPL v2");
