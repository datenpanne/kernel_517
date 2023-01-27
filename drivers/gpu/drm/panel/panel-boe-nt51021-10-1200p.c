// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 * Author: Jitao Shi <jitao.shi@mediatek.com>
 */

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_connector.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
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
	bool discharge_on_disable;
};

struct boe_panel {
	struct drm_panel base;
	struct mipi_dsi_device *dsi;

	const struct panel_desc *desc;

	enum drm_panel_orientation orientation;
	struct regulator *pp3300;
	struct regulator *pp1800;
	struct regulator *volregN;
	struct regulator *volregP;
	struct gpio_desc *reset_gpio;

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
	_INIT_DCS_CMD(0x8f, 0xa5),
	_INIT_DCS_CMD(0x83, 0x00),
	_INIT_DCS_CMD(0x84, 0x00),
	_INIT_DCS_CMD(0x8c, 0x80),
	_INIT_DCS_CMD(0xcd, 0x6c),
	_INIT_DCS_CMD(0xc8, 0xfc),
	_INIT_DCS_CMD(0x97, 0x00),
	_INIT_DCS_CMD(0x8b, 0x10),
	_INIT_DCS_CMD(0xa9, 0x20),
	_INIT_DCS_CMD(0x83, 0xaa),
	_INIT_DCS_CMD(0x84, 0x11),
	_INIT_DCS_CMD(0xa9, 0x4b),
	_INIT_DCS_CMD(0x85, 0x04),
	_INIT_DCS_CMD(0x86, 0x08),
	_INIT_DCS_CMD(0x9c, 0x10),
	_INIT_DCS_CMD(0x11, 0x00),
	_INIT_DCS_CMD(0x8f, 0x00),
	{},
};

static inline struct boe_panel *to_boe_panel(struct drm_panel *panel)
{
	return container_of(panel, struct boe_panel, base);
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

	msleep(150);

	if (boe->desc->discharge_on_disable) {
		regulator_disable(boe->volregN);
		regulator_disable(boe->volregP);
		usleep_range(5000, 7000);
		gpiod_set_value(boe->reset_gpio, 0);
		usleep_range(5000, 7000);
		regulator_disable(boe->pp1800);
		regulator_disable(boe->pp3300);
	} else {
		gpiod_set_value(boe->reset_gpio, 0);
		usleep_range(1000, 2000);
		regulator_disable(boe->volregN);
		regulator_disable(boe->volregP);
		usleep_range(5000, 7000);
		regulator_disable(boe->pp1800);
		regulator_disable(boe->pp3300);
	}

	boe->prepared = false;

	return 0;
}

static int boe_panel_prepare(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);
	int ret;

	if (boe->prepared)
		return 0;

	gpiod_set_value(boe->reset_gpio, 0);
	usleep_range(1000, 1500);

	ret = regulator_enable(boe->pp3300);
	if (ret < 0)
		return ret;

	ret = regulator_enable(boe->pp1800);
	if (ret < 0)
		return ret;

	usleep_range(3000, 5000);

	ret = regulator_enable(boe->volregP);
	if (ret < 0)
		goto poweroff1v8;
	ret = regulator_enable(boe->volregN);
	if (ret < 0)
		goto poweroffvolregP;

	usleep_range(10000, 11000);

	gpiod_set_value(boe->reset_gpio, 1);
	usleep_range(1000, 2000);
	gpiod_set_value(boe->reset_gpio, 0);
	usleep_range(1000, 2000);
	gpiod_set_value(boe->reset_gpio, 1);
	usleep_range(6000, 10000);

	ret = boe_panel_init_dcs_cmd(boe);
	if (ret < 0) {
		dev_err(panel->dev, "failed to init panel: %d\n", ret);
		goto poweroff;
	}

	boe->prepared = true;

	return 0;

poweroff:
	regulator_disable(boe->volregN);
poweroffvolregP:
	regulator_disable(boe->volregP);
poweroff1v8:
	usleep_range(5000, 7000);
	regulator_disable(boe->pp1800);
	gpiod_set_value(boe->reset_gpio, 0);

	return ret;
}

static int boe_panel_enable(struct drm_panel *panel)
{
	msleep(130);
	return 0;
}

static const struct drm_display_mode boe_nt51921_default_mode = {
	.clock = (1200 + 64 + 4 + 36) * (1920 + 104 + 2 + 24) * 60 / 1000,
	.hdisplay = 1200,
	.hsync_start = 1200 + 64,
	.hsync_end = 1200 + 64 + 4,
	.htotal = 1200 + 64 + 4 + 36,
	.vdisplay = 1920,
	.vsync_start = 1920 + 1046,
	.vsync_end = 1920 + 104 + 2,
	.vtotal = 1920 + 104 + 2 + 24,
	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static const struct panel_desc boe_nt51921_desc = {
	.modes = &boe_nt51921_default_mode,
	.bpc = 8,
	.size = {
		.width_mm = 135,
		.height_mm = 217,
	},
	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO
			| MIPI_DSI_MODE_VIDEO_HSE
			| MIPI_DSI_CLOCK_NON_CONTINUOUS
			| MIPI_DSI_MODE_VIDEO_BURST,
	.init_cmds = boe_init_cmd,
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

static int boe_panel_add(struct boe_panel *boe)
{
	struct device *dev = &boe->dsi->dev;
	int err;

	boe->volregP = devm_regulator_get(dev, "volregP");
	if (IS_ERR(boe->volregP))
		return PTR_ERR(boe->volregP);

	boe->volregN = devm_regulator_get(dev, "volregN");
	if (IS_ERR(boe->volregN))
		return PTR_ERR(boe->volregN);

	boe->pp3300 = devm_regulator_get(dev, "pp3300");
	if (IS_ERR(boe->pp3300))
		return PTR_ERR(boe->pp3300);

	boe->pp1800 = devm_regulator_get(dev, "pp1800");
	if (IS_ERR(boe->pp1800))
		return PTR_ERR(boe->pp1800);

	boe->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(boe->reset_gpio)) {
		dev_err(dev, "cannot get reset-gpios %ld\n",
			PTR_ERR(boe->reset_gpio));
		return PTR_ERR(boe->reset_gpio);
	}

	gpiod_set_value(boe->reset_gpio, 0);

	drm_panel_init(&boe->base, dev, &boe_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);
	err = of_drm_get_panel_orientation(dev->of_node, &boe->orientation);
	if (err < 0) {
		dev_err(dev, "%pOF: failed to get orientation %d\n", dev->of_node, err);
		return err;
	}

	err = drm_panel_of_backlight(&boe->base);
	if (err)
		return err;

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
	  .data = &boe_nt51921_desc
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
MODULE_DESCRIPTION("BOE tv101wum-nl6 1200x1920 video mode panel driver");
MODULE_LICENSE("GPL v2");
