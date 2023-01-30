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

/*static const char * const regulator_names[] = {
	"vddio",
	"vsp",
	"vsn",
};

static unsigned long const regulator_enable_loads[] = {
	100000,
	200,
	40,
};

static unsigned long const regulator_disable_loads[] = {
	100,
	0,
	0,
};*/

struct hw_nt51021 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	//struct regulator_bulk_data supplies[ARRAY_SIZE(regulator_names)];
	struct regulator_bulk_data supplies[6];
	struct gpio_desc *reset_gpio;
	/*struct gpio_desc *vci_gpio; //vdd?
	struct gpio_desc *iovcc_gpio; //vddio?
	struct gpio_desc *backlight_gpio; //vled*/
	/*struct gpio_desc *disp_power_backlight;
	struct gpio_desc *disp_power_panel;
	struct gpio_desc *disp_en_gpio_vled;*/

	struct mutex mutex;

    int hw_led_en_flag;
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
struct hw_nt51021 *to_hw_nt51021(struct drm_panel *panel)
{
	return container_of(panel, struct hw_nt51021, panel);
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

static void hw_nt51021_reset(struct hw_nt51021 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(50);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	msleep(50);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(200);
}

/*static void hw_nt51021_pwr_en(struct hw_nt51021 *ctx, int enabled)
{
	//gpiod_set_value_cansleep(ctx->vled_gpio, 1);
	//msleep(5);

	gpiod_set_value_cansleep(ctx->disp_power_backlight, 1);
	gpiod_set_value_cansleep(ctx->disp_power_panel, 1);
	msleep(500);
}

static void hw_nt51021_bias_en(struct hw_nt51021 *ctx, int enabled)
{
	//struct mipi_dsi_device *dsi = ctx->dsi;
	//struct device *dev = &dsi->dev;
	//int ret=1;
    //int hw_led_en_flag;

	gpiod_set_value(ctx->disp_en_gpio_vled, 1);
	//msleep(5);

	if (enabled) {
        ctx-> hw_led_en_flag = 1;
    } else {
        ctx-> hw_led_en_flag = 0;
    }
}*/

static int nt51021_esd_check(struct hw_nt51021 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	u8 addr, val1, val2;
	int ret = -1;

	//dsi->mode_flags &= ~MIPI_DSI_MODE_LPM; // HS_Mode

	addr = 0x81;
	val1 = 0x00;

	ret = mipi_dsi_dcs_read(dsi, addr, &val1, 1);
	//ret = mipi_dsi_dcs_read(dsi, addr, &val1, 1);

	addr = 0x82;
	val2 = 0x00;

	ret = mipi_dsi_dcs_read(dsi, addr, &val2, 1);
	if (ret < 0) {
		return ret;
	}

	if ((val1 == 0x00) && (val2 == 0x00))
		ret = 0;
	else if ((val1 == 0x10) && (val2 == 0x80))
		ret = 0;
	else if ((val1 == 0x40) && (val2 == 0x00))
		ret = 0;
	else if ((val1 == 0x00) && (val2 == 0x02))
		ret = 0;
	else {
		ret = 1;
		dev_info(dev, "MIPI status: 0x%02x, 0x%02x", val1, val2);
	}

    return ret;
}

/*static void hw_nt51021_pwr_dis(struct hw_nt51021 *ctx)
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

	dsi_generic_write_seq(dsi, HW_NT51021_VND_MIPI, 0xa5); // MIPI enable command interface

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
	dsi_generic_write_seq(dsi, HW_NT51021_VND_MIPI, 0xa5); // MIPI enable command interface
    msleep(5);
	dsi_generic_write_seq(dsi, HW_NT51021_VND_INDEX0, 0x00); // command page 0
	dsi_generic_write_seq(dsi, HW_NT51021_VND_INDEX1, 0x00); // command page 0
	dsi_generic_write_seq(dsi, HW_NT51021_VND_GOP, 0x80); // GIP unlock OTP
	dsi_generic_write_seq(dsi, HW_NT51021_VND_3DUMMY, 0x6c); // 3Dummy
	//dsi_generic_write_seq(dsi, HW_NT51021_VND_DCH_C0, 0x8b); // GCH
	dsi_generic_write_seq(dsi, HW_NT51021_VND_DCH_C8, 0xf0); // GCH DC improves high temperature interlaced display
	dsi_generic_write_seq(dsi, HW_NT51021_SET_DISPLAY_BRIGHTNESS, 0x00); // backlight control?
	dsi_generic_write_seq(dsi, HW_NT51021_VND_TERMRESIST, 0x00); // matching terminal resistance 100 ohms
	dsi_generic_write_seq(dsi, HW_NT51021_VND_HSYNC, 0x10); // H-sync unlick OTP
	dsi_generic_write_seq(dsi, HW_NT51021_VND_A9, 0x20); // ajust the hold time H-sync high level/enable TP_SYNC

	dsi_generic_write_seq(dsi, HW_NT51021_VND_INDEX0, 0xaa); // command page 1
	dsi_generic_write_seq(dsi, HW_NT51021_VND_INDEX1, 0x11); // command page 1
    dsi_generic_write_seq(dsi, HW_NT51021_VND_A9, 0x4b); // IC MIPI Rx drive current capability gear 85%
    dsi_generic_write_seq(dsi, HW_NT51021_VND_TESTMODE1, 0x04); // test mode 1
    dsi_generic_write_seq(dsi, HW_NT51021_VND_TESTMODE2, 0x08); // test mode 2
    dsi_generic_write_seq(dsi, HW_NT51021_VND_TESTMODE3, 0x10); // test mode 3

	dsi_generic_write_seq(dsi, HW_NT51021_VND_INDEX0, 0xbb); // command page 2
	dsi_generic_write_seq(dsi, HW_NT51021_VND_INDEX1, 0x22); // command page 2
	dsi_generic_write_seq(dsi, HW_NT51021_CABC_MODE, 0xC0); // cabc off

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
    }
	dsi_generic_write_seq(dsi, HW_NT51021_VND_MIPI, 0x00); //exit MIPI Command interface
	msleep(5);
    /*ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_MIPI, (u8[]){ 0xa5 }, 1);
	if (ret < 0)
		return ret; // MIPI enable command
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_INDEX0, (u8[]){ 0x00 }, 1);
	if (ret < 0)
		return ret; //hw boe
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_INDEX1, (u8[]){ 0x00 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_GOP, (u8[]){ 0x80 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_3DUMMY, (u8[]){ 0x6c }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_DCH_C8, (u8[]){ 0xfc }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_SET_DISPLAY_BRIGHTNESS, (u8[]){ 0x00 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_TERMRESIST, (u8[]){ 0x00 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_INDEX0, (u8[]){ 0xbb }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_INDEX1, (u8[]){ 0x22 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_96, (u8[]){ 0x00 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_CABC_MODE, (u8[]){ 0xc0 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_91, (u8[]){ 0xa0 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_9A, (u8[]){ 0x10 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_CABC_MOVING_DIMMING, (u8[]){ 0x78 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_CABC_STILL_DIMMING, (u8[]){ 0xb1 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_A1, (u8[]){ 0xff }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_A2, (u8[]){ 0xfa }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_A3, (u8[]){ 0xf3 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_A4, (u8[]){ 0xed }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_A5, (u8[]){ 0xe7 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_A6, (u8[]){ 0xe2 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_A7, (u8[]){ 0xdc }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_A8, (u8[]){ 0xd7 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_A9, (u8[]){ 0xd1 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_AA, (u8[]){ 0xcc }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_B4, (u8[]){ 0x1c }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_B5, (u8[]){ 0x38 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_B6, (u8[]){ 0x30 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_INDEX0, (u8[]){ 0xaa }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_INDEX1, (u8[]){ 0x11 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_A9, (u8[]){ 0x4b }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_TESTMODE1, (u8[]){ 0x04 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_TESTMODE2, (u8[]){ 0x08 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_TESTMODE3, (u8[]){ 0x10 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_INDEX0, (u8[]){ 0x00 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_INDEX1, (u8[]){ 0x00 }, 1);
	if (ret < 0)
		return ret;

    ret = mipi_dsi_dcs_write(dsi, 0x11, (u8[]){ 0x00 }, 1);
	if (ret < 0)
		return ret;
	msleep(120);*/

/*	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}*/

	/*ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_MIPI, (u8[]){ 0x00 }, 1); //exit MIPI Command
	if (ret < 0)
		return ret;*/

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	return 0;
}

static int hw_nt51021_panel_on(struct hw_nt51021 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	//dsi->mode_flags = 0;
	//dsi->mode_flags &= ~MIPI_DSI_MODE_LPM; // HS_Mode
	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

    /*ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_MIPI, (u8[]){ 0xa5 }, 1);
	if (ret < 0)
		return ret;*/

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}
	msleep(100);

	return 0;
}

static int hw_nt51021_on(struct hw_nt51021 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret, i;

	/*for (i = 0; i < ARRAY_SIZE(ctx->supplies); i++) {
		ret = regulator_set_load(ctx->supplies[i].consumer,
					regulator_enable_loads[i]);
		if (ret)
			return ret;
	}*/

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		return ret;
	msleep(30);

    //hw_nt51021_pwr_en(ctx, 1);

	/*gpiod_set_value_cansleep(ctx->disp_power_backlight, 1);
	//msleep(40);
	gpiod_set_value_cansleep(ctx->disp_power_panel, 1);
	msleep(40);*/

	hw_nt51021_reset(ctx);

	return 0;
}

static int hw_nt51021_off(struct hw_nt51021 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret = 0;
    int i;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM; // HS_Mode

    /*ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_MIPI, (u8[]){ 0xa5 }, 1);
	if (ret < 0)
		return ret;*/

	dsi_generic_write_seq(dsi, HW_NT51021_VND_MIPI, 0xa5); // MIPI enable command interface
	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}
	msleep(50);

	/*ret = mipi_dsi_dcs_write(dsi, 0x10, (u8[]){ 0x00);
	msleep(100);*/

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(100);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

    //hw_nt51021_pwr_en(ctx, 0);
	/*gpiod_set_value_cansleep(ctx->disp_power_backlight, 0);
	msleep(40);
	gpiod_set_value_cansleep(ctx->disp_power_panel, 0);
	msleep(40);*/

	/*for (i = 0; i < ARRAY_SIZE(ctx->supplies); i++) {
		ret = regulator_set_load(ctx->supplies[i].consumer,
				regulator_disable_loads[i]);
		if (ret) {
			dev_err(dev, "regulator_set_load failed %d\n", ret);
			return ret;
		}
	}*/

	ret = regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret) {
		dev_err(dev, "regulator_bulk_disable failed %d\n", ret);
	}
	return ret;
}

static int hw_nt51021_cabc_off(struct hw_nt51021 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;

    //static char cabc_still_dimming_on[2] = {HW_NT51021_CABC_STILL_DIMMING, 0x60};
    const u8 cabc_mode_off[2] = {HW_NT51021_CABC_MODE, 0xc0};
    const u8 chang_page2_index0[2] = {HW_NT51021_VND_INDEX0, 0xBB};
    const u8 chang_page2_index1[2] = {HW_NT51021_VND_INDEX1, 0x22};
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM; // HS_Mode

    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_MIPI, (u8[]){ 0xa5 }, 1);
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dcs_write_buffer(dsi, chang_page2_index0,
					ARRAY_SIZE(chang_page2_index0));
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dcs_write_buffer(dsi, chang_page2_index1,
					ARRAY_SIZE(chang_page2_index1));
	if (ret < 0)
		return ret;

	/*ret = mipi_dsi_dcs_write_buffer(dsi, cabc_still_dimming_on,
					ARRAY_SIZE(cabc_still_dimming_on));
	if (ret < 0)
		return ret;*/

	ret = mipi_dsi_dcs_write_buffer(dsi, cabc_mode_off,
					ARRAY_SIZE(cabc_mode_off));
	if (ret < 0)
		return ret;
	return 0;
}

static int hw_nt51021_cabc_still(struct hw_nt51021 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
    int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM; // HS_Mode

    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_MIPI, (u8[]){ 0xa5 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_INDEX0, (u8[]){ 0xbb }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_VND_INDEX1, (u8[]){ 0x22 }, 1);
	if (ret < 0)
		return ret;
    ret = mipi_dsi_dcs_write(dsi, HW_NT51021_CABC_MODE, (u8[]){ 0x40 }, 1);
	if (ret < 0)
		return ret;

    return 0;
}

static int hw_nt51021_prepare(struct drm_panel *panel)
{
	struct hw_nt51021 *ctx = to_hw_nt51021(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

    ret = hw_nt51021_on(ctx);
	if (ret < 0)
		return ret;

    //dsi->mode_flags |= MIPI_DSI_MODE_LPM;
	//mutex_lock(&ctx->mutex);
	ret = hw_nt51021_init(ctx);

	/*ret = hw_nt51021_cabc_off(ctx); //pele?
	if (ret)
		return ret;*/

    //hw_nt51021_cabc_off(ctx);

    nt51021_esd_check(ctx);
	msleep(20);

    hw_nt51021_cabc_still(ctx);

	ret = hw_nt51021_panel_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
		return ret;
	}
	//mutex_unlock(&ctx->mutex);

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

	msleep(40);

	if (ctx->enabled)
		return 0;

	//gpiod_set_value(ctx->disp_en_gpio_vled, 1);
	//msleep(200);

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

    //gpiod_set_value(ctx->disp_en_gpio_vled, 0);
	//msleep(200);

	ctx->enabled = false;
	return 0;
}

static int hw_nt51021_unprepare(struct drm_panel *panel)
{
	struct hw_nt51021 *ctx = to_hw_nt51021(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret, i;

	if (!ctx->prepared)
		return 0;
	//mutex_lock(&ctx->mutex);
	//dsi->mode_flags = 0;

	ret = hw_nt51021_off(ctx);
	if (ret < 0)
		dev_err(dev, "power_off failed ret = %d\n", ret);
	//mutex_unlock(&ctx->mutex);

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
	struct hw_nt51021 *ctx = to_hw_nt51021(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
    struct device *dev = &dsi->dev;
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

/*static int hw_nt51021_bkl_cmd(struct hw_nt51021 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;

    u8 pwm_led[2] = {HW_NT51021_SET_DISPLAY_BRIGHTNESS, 0x00};
    u8 chang_page0_index0[2] = {HW_NT51021_VND_INDEX0, 0x00};
    u8 chang_page0_index1[2] = {HW_NT51021_VND_INDEX1, 0x00};
    int ret;

	//dsi->mode_flags &= ~MIPI_DSI_MODE_LPM; // HS_Mode
    dsi_dcs_write_seq(dsi, HW_NT51021_VND_MIPI, 0xa5);

	ret = mipi_dsi_dcs_write_buffer(dsi, chang_page0_index0,
					ARRAY_SIZE(chang_page0_index0));
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dcs_write_buffer(dsi, chang_page0_index1,
					ARRAY_SIZE(chang_page0_index1));
	if (ret < 0)
		return ret;

	ret = mipi_dsi_dcs_write_buffer(dsi, pwm_led,
					ARRAY_SIZE(pwm_led));
	if (ret < 0)
		return ret;

	return 0;
}*/

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

static int hw_nt51021_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	struct hw_nt51021 *ctx = mipi_dsi_get_drvdata(dsi);
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

/*static int hw_nt51021_bl_get_intensity(struct backlight_device *bl)
{
	return backlight_get_brightness(bl);
}*/

static const struct backlight_ops hw_nt51021_bl_ops = {
	.update_status = hw_nt51021_bl_update_status,
	//.get_brightness = hw_nt51021_bl_get_intensity,
};

static struct backlight_device *
hw_nt51021_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct backlight_properties props;

	memset(&props, 0, sizeof(props));
    props.type = BACKLIGHT_RAW,
    props.brightness = 128,
    props.max_brightness = 255;

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &hw_nt51021_bl_ops, &props);
}

static int hw_nt51021_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct hw_nt51021 *ctx;
	int ret, i;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	/*for (i = 0; i < ARRAY_SIZE(ctx->supplies); i++)
		ctx->supplies[i].supply = regulator_names[i];

	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies),
				      ctx->supplies);
	if (ret < 0)
		return ret;*/

	//mutex_init(&ctx->mutex);

	ctx->supplies[0].supply = "lcdiovcc";
	ctx->supplies[1].supply = "lcdanalogvcc";
	ctx->supplies[2].supply = "Biasvol";
	ctx->supplies[3].supply = "vsp";
	ctx->supplies[4].supply = "vsn";	
	ctx->supplies[5].supply = "backlight";
	
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies),
				      ctx->supplies);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to get regulators\n");

	if (ret < 0)
		return ret;
	ret = regulator_set_voltage(ctx->supplies[2].consumer,
				    5400000, 5400000);
	if (ret)
		return ret;
	ret = regulator_set_voltage(ctx->supplies[3].consumer,
				    5400000, 5400000);
	if (ret)
		return ret;
	ret = regulator_set_voltage(ctx->supplies[4].consumer,
				    5400000, 5400000);
	if (ret)
		return ret;

	/*ctx->disp_power_panel = devm_gpiod_get(dev, "panel", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->disp_power_panel))
		return dev_err_probe(dev, PTR_ERR(ctx->disp_power_panel),
				     "Failed to get panel-gpios\n");

	ctx->disp_en_gpio_vled = devm_gpiod_get(dev, "vled", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->disp_en_gpio_vled))
		return dev_err_probe(dev, PTR_ERR(ctx->disp_en_gpio_vled),
				     "Failed to get enable-gpios\n");

	ctx->disp_power_backlight = devm_gpiod_get(dev, "backlight", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->disp_power_backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->disp_power_backlight),
				     "Failed to get backlight-gpios\n");*/

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
