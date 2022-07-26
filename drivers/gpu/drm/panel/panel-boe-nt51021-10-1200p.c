// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 * Author: Jitao Shi <jitao.shi@mediatek.com>
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_connector.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#include <video/mipi_display.h>

struct panel_desc {
	const struct drm_display_mode *modes;
	unsigned int bpc;

	/**
	 * @width_mm: width of the panel's active display area
	 * @height_mm: height of the panel's active display area
	 */
	struct {
		unsigned int width_mm;
		unsigned int height_mm;
	} size;

	unsigned long mode_flags;
	enum mipi_dsi_pixel_format format;
	const struct panel_init_cmd *init_cmds;
	unsigned int lanes;
	//bool discharge_on_disable;
};

struct boe_panel {
	struct drm_panel base;
	struct mipi_dsi_device *dsi;

	const struct panel_desc *desc;

	enum drm_panel_orientation orientation;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *vdd_gpio;
	struct gpio_desc *vcc_gpio;
	struct gpio_desc *blen_gpio;

	bool prepared;
};

enum dsi_cmd_type {
	INIT_DCS_CMD,
	DELAY_CMD,
};

struct panel_init_cmd {
	enum dsi_cmd_type type;
	size_t len;
	const char *data;
};

#define _INIT_DCS_CMD(...) { \
	.type = INIT_DCS_CMD, \
	.len = sizeof((char[]){__VA_ARGS__}), \
	.data = (char[]){__VA_ARGS__} }

#define _INIT_DELAY_CMD(...) { \
	.type = DELAY_CMD,\
	.len = sizeof((char[]){__VA_ARGS__}), \
	.data = (char[]){__VA_ARGS__} }

static const struct panel_init_cmd boe_init_cmd[] = {
	_INIT_DCS_CMD(0x83, 0x00), // dsi on
	_INIT_DCS_CMD(0x84, 0x00),
	_INIT_DCS_CMD(0x8c, 0x80),
	_INIT_DCS_CMD(0xcd, 0x6c),
	_INIT_DCS_CMD(0xc8, 0xfc),
	_INIT_DCS_CMD(0x9f, 0x00),
	_INIT_DCS_CMD(0x97, 0x00),
	_INIT_DCS_CMD(0x83, 0xbb),
	_INIT_DCS_CMD(0x84, 0x22),
	_INIT_DCS_CMD(0x96, 0x00),
	_INIT_DCS_CMD(0x90, 0xc0),
	_INIT_DCS_CMD(0x91, 0xa0),
	_INIT_DCS_CMD(0x9a, 0x10),
	_INIT_DCS_CMD(0x94, 0x78),
	_INIT_DCS_CMD(0x95, 0xb1),
	_INIT_DCS_CMD(MIPI_DCS_READ_DDB_START, 0xff),
	_INIT_DCS_CMD(MIPI_DCS_READ_PPS_START, 0xfa),
	_INIT_DCS_CMD(0xa3, 0xf3),
	_INIT_DCS_CMD(0xa4, 0xed),
	_INIT_DCS_CMD(0xa5, 0xe7),
	_INIT_DCS_CMD(0xa6, 0xe2),
	_INIT_DCS_CMD(0xa7, 0xdc),
	_INIT_DCS_CMD(MIPI_DCS_READ_DDB_CONTINUE, 0xd7),
	_INIT_DCS_CMD(MIPI_DCS_READ_PPS_CONTINUE, 0xd1),
	_INIT_DCS_CMD(0xaa, 0xcc),
	_INIT_DCS_CMD(0xb4, 0x1c),
	_INIT_DCS_CMD(0xb5, 0x38),
	_INIT_DCS_CMD(0xb6, 0x30),
	_INIT_DCS_CMD(0x83, 0xaa),
	_INIT_DCS_CMD(0x84, 0x11),
	_INIT_DCS_CMD(MIPI_DCS_READ_PPS_CONTINUE, 0x4b),
	_INIT_DCS_CMD(0x85, 0x04),
	_INIT_DCS_CMD(0x86, 0x08),
	_INIT_DCS_CMD(0x9c, 0x10),
	_INIT_DCS_CMD(0x83, 0x00),
	_INIT_DCS_CMD(0x84, 0x00),
	_INIT_DCS_CMD(0x83, 0xbb), // cabc off
	_INIT_DCS_CMD(0x84, 0x22),
	_INIT_DCS_CMD(0x90, 0xC0),
	_INIT_DCS_CMD(0x83, 0xbb), // cabc moving
	_INIT_DCS_CMD(0x84, 0x22),
	_INIT_DCS_CMD(0x90, 0x00),
	_INIT_DCS_CMD(0x83, 0xbb), // cabc still
	_INIT_DCS_CMD(0x84, 0x22),
	_INIT_DCS_CMD(0x90, 0x40),
	_INIT_DCS_CMD(0x83, 0xbb), // cabc ui
	_INIT_DCS_CMD(0x84, 0x22),
	_INIT_DCS_CMD(0x53, 0x2C),
	_INIT_DCS_CMD(0x83, 0xbb), // cabc video
	_INIT_DCS_CMD(0x84, 0x22),
	_INIT_DCS_CMD(0x53, 0x2C),
	_INIT_DCS_CMD(0x10, 0x00), // dsi off
	_INIT_DELAY_CMD(100),
	{},
};

static inline struct boe_panel *to_boe_panel(struct drm_panel *panel)
{
	return container_of(panel, struct boe_panel, base);
}

static void disable_gpios(struct boe_panel *boe)
{
	gpiod_set_value(boe->vdd_gpio, 0);
	gpiod_set_value(boe->vcc_gpio, 0);
    gpiod_set_value(boe->blen_gpio, 0);
    gpiod_set_value(boe->reset_gpio, 0);
}

static void boe_bias_en(struct boe_panel *boe)
{
        gpiod_set_value((boe->vdd_gpio), 1);
		//dev_err(dev, "failed to get vdd_gpio gpio: %d\n", enable);
}

static int boe_panel_init_dcs_cmd(struct boe_panel *boe)
{
	struct mipi_dsi_device *dsi = boe->dsi;
	struct drm_panel *panel = &boe->base;
	int i, err = 0;

	if (boe->desc->init_cmds) {
		const struct panel_init_cmd *init_cmds = boe->desc->init_cmds;

		for (i = 0; init_cmds[i].len != 0; i++) {
			const struct panel_init_cmd *cmd = &init_cmds[i];

			switch (cmd->type) {
			case DELAY_CMD:
				msleep(cmd->data[0]);
				err = 0;
				break;

			case INIT_DCS_CMD:
				err = mipi_dsi_dcs_write(dsi, cmd->data[0],
							 cmd->len <= 1 ? NULL :
							 &cmd->data[1],
							 cmd->len - 1);
				break;

			default:
				err = -EINVAL;
			}

			if (err < 0) {
				dev_err(panel->dev,
					"failed to write command %u\n", i);
				return err;
			}
		}
	}

	return 0;
}

static int boe_panel_enter_sleep_mode(struct boe_panel *boe)
{
	struct mipi_dsi_device *dsi = boe->dsi;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0)
		return ret;

	return 0;
}

static int boe_panel_unprepare(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);
	int ret;

	if (!boe->prepared)
		return 0;

	ret = boe_panel_enter_sleep_mode(boe);
	if (ret < 0) {
		dev_err(panel->dev, "failed to set panel off: %d\n", ret);
		return ret;
	}

    msleep(2);
    disable_gpios(boe);

	boe->prepared = false;

	return 0;
}

static int boe_panel_prepare(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);
	int ret;

	if (boe->prepared)
		return 0;

	gpiod_set_value(boe->reset_gpio, 1);
	usleep_range(1000, 2000);
	gpiod_set_value(boe->reset_gpio, 0);
	usleep_range(1000, 2000);
	gpiod_set_value(boe->reset_gpio, 1);
	usleep_range(6000, 10000);

 	gpiod_set_value(boe->vdd_gpio, 1);
	msleep(5);
	gpiod_set_value(boe->blen_gpio, 1);
	gpiod_set_value(boe->vcc_gpio, 1);
	msleep(500);

	ret = boe_panel_init_dcs_cmd(boe);
	if (ret < 0) {
		dev_err(panel->dev, "failed to init panel: %d\n", ret);
		goto poweroff;
	}

	/* Enabe tearing mode: send TE (tearing effect) at VBLANK */
	ret = mipi_dsi_dcs_set_tear_on(boe->dsi,
				       MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(panel->dev, "failed to enable vblank TE (%d)\n", ret);
		goto poweroff;
	}


	boe->prepared = true;

	return 0;

poweroff:
	disable_gpios(boe);

	return ret;
}

static int boe_panel_enable(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);

	msleep(130);

    boe_bias_en(boe);
	gpiod_set_value(boe->blen_gpio, 1);

	return 0;
}

static const struct drm_display_mode boe_nt51021_10_default_mode = {
	.clock = 160392,
	.hdisplay = 1200,
	.hsync_start = 1200 + 64,
	.hsync_end = 1200 + 64 + 4,
	.htotal = 1200 + 64 + 4 + 36,
	.vdisplay = 1920,
	.vsync_start = 1920 + 104,
	.vsync_end = 1920 + 104 + 2,
	.vtotal = 1920 + 104 + 2 + 24,
	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static const struct panel_desc boe_nt51021_10_desc = {
	.modes = &boe_nt51021_10_default_mode,
	.bpc = 8,
	.size = {
		.width_mm = 135,
		.height_mm = 217,
	},
	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	/*.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
		      MIPI_DSI_MODE_LPM,*/
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_NO_EOT_PACKET |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS,
	.init_cmds = boe_init_cmd,
	//.discharge_on_disable = true,
};

static int boe_panel_get_modes(struct drm_panel *panel,
			       struct drm_connector *connector)
{
	struct boe_panel *boe = to_boe_panel(panel);
	const struct drm_display_mode *m = boe->desc->modes;
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, m);
	if (!mode) {
		dev_err(panel->dev, "failed to add mode %ux%u@%u\n",
			m->hdisplay, m->vdisplay, drm_mode_vrefresh(m));
		return -ENOMEM;
	}

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = boe->desc->size.width_mm;
	connector->display_info.height_mm = boe->desc->size.height_mm;
	connector->display_info.bpc = boe->desc->bpc;
	drm_connector_set_panel_orientation(connector, boe->orientation);

	return 1;
}

static const struct drm_panel_funcs boe_panel_funcs = {
	.unprepare = boe_panel_unprepare,
	.prepare = boe_panel_prepare,
	.enable = boe_panel_enable,
	.get_modes = boe_panel_get_modes,
};

static int boe_panel_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	//struct boe_panel *boe = mipi_dsi_get_drvdata(dsi);
	u16 brightness = bl->props.brightness;
	int ret;

	if (bl->props.power == FB_BLANK_UNBLANK &&
	    bl->props.fb_blank == FB_BLANK_UNBLANK)
        brightness = bl->props.brightness;
    else
		brightness = 0;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness(dsi, brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

static int boe_panel_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness & 0xff;
}

static const struct backlight_ops boe_bl_ops = {
	.update_status = boe_panel_bl_update_status,
	.get_brightness = boe_panel_bl_get_brightness,
};

static struct backlight_device *
boe_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 128,
		.max_brightness = 255,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &boe_bl_ops, &props);
}

static int boe_panel_add(struct boe_panel *boe)
{
	struct device *dev = &boe->dsi->dev;
	struct mipi_dsi_device *dsi = boe->dsi;
	int err;

	boe->vdd_gpio = devm_gpiod_get(dev, "vdd", GPIOD_OUT_HIGH);
	if (IS_ERR(boe->vdd_gpio)) {
		dev_err(dev, "cannot get vled-gpios %ld\n",
			PTR_ERR(boe->vdd_gpio));
		return PTR_ERR(boe->vdd_gpio);
	}

	boe->vcc_gpio = devm_gpiod_get(dev, "vcc", GPIOD_OUT_HIGH);
	if (IS_ERR(boe->vcc_gpio)) {
		dev_err(dev, "cannot get panel-gpios %ld\n",
			PTR_ERR(boe->vcc_gpio));
		return PTR_ERR(boe->vcc_gpio);
	}

	boe->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(boe->reset_gpio)) {
		dev_err(dev, "cannot get reset-gpios %ld\n",
			PTR_ERR(boe->reset_gpio));
		return PTR_ERR(boe->reset_gpio);
	}

	boe->blen_gpio = devm_gpiod_get(dev, "backlight", GPIOD_OUT_LOW);
	if (IS_ERR(boe->blen_gpio)) {
		dev_err(dev, "cannot get backlight-gpios %ld\n",
			PTR_ERR(boe->blen_gpio));
		return PTR_ERR(boe->blen_gpio);
	}

	//gpiod_set_value(boe->reset_gpio, 0);
	//gpiod_set_value(boe->backlight_gpio, 0);

	drm_panel_init(&boe->base, dev, &boe_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);
	err = of_drm_get_panel_orientation(dev->of_node, &boe->orientation);
	if (err < 0) {
		dev_err(dev, "%pOF: failed to get orientation %d\n", dev->of_node, err);
		return err;
	}

	boe->base.backlight = boe_create_backlight(dsi);
	if (IS_ERR(boe->base.backlight))
		return dev_err_probe(dev, PTR_ERR(boe->base.backlight),
				     "Failed to create backlight\n");

	boe->base.funcs = &boe_panel_funcs;
	boe->base.dev = &boe->dsi->dev;

	drm_panel_add(&boe->base);

	return 0;
}

static int boe_panel_probe(struct mipi_dsi_device *dsi)
{
	struct boe_panel *boe;
	int ret;
	const struct panel_desc *desc;

	boe = devm_kzalloc(&dsi->dev, sizeof(*boe), GFP_KERNEL);
	if (!boe)
		return -ENOMEM;

	desc = of_device_get_match_data(&dsi->dev);
	dsi->lanes = desc->lanes;
	dsi->format = desc->format;
	dsi->mode_flags = desc->mode_flags;
	boe->desc = desc;
	boe->dsi = dsi;
	ret = boe_panel_add(boe);
	if (ret < 0)
		return ret;

	mipi_dsi_set_drvdata(dsi, boe);

	ret = mipi_dsi_attach(dsi);
	if (ret)
		drm_panel_remove(&boe->base);

	return ret;
}

static void boe_panel_shutdown(struct mipi_dsi_device *dsi)
{
	struct boe_panel *boe = mipi_dsi_get_drvdata(dsi);

	drm_panel_disable(&boe->base);
	drm_panel_unprepare(&boe->base);
}

static int boe_panel_remove(struct mipi_dsi_device *dsi)
{
	struct boe_panel *boe = mipi_dsi_get_drvdata(dsi);
	int ret;

	boe_panel_shutdown(dsi);

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "failed to detach from DSI host: %d\n", ret);

	if (boe->base.dev)
		drm_panel_remove(&boe->base);

	return 0;
}

static const struct of_device_id boe_of_match[] = {
	{ .compatible = "boe,nt51021-10-1200p",
	  .data = &boe_nt51021_10_desc
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, boe_of_match);

static struct mipi_dsi_driver boe_panel_driver = {
	.driver = {
		.name = "panel-boe-nt51021-10-1200p",
		.of_match_table = boe_of_match,
	},
	.probe = boe_panel_probe,
	.remove = boe_panel_remove,
	.shutdown = boe_panel_shutdown,
};
module_mipi_dsi_driver(boe_panel_driver);

MODULE_AUTHOR("Jitao Shi <jitao.shi@mediatek.com>");
MODULE_DESCRIPTION("DRM driver for BOE_NT51021_10_1200P_VIDEO");
MODULE_LICENSE("GPL v2");
