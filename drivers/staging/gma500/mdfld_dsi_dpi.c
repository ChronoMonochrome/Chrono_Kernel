/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 * jim liu <jim.liu@intel.com>
 * Jackie Li<yaodong.li@intel.com>
 */

#include "mdfld_dsi_dpi.h"
#include "mdfld_output.h"
#include "mdfld_dsi_pkg_sender.h"


static void mdfld_wait_for_HS_DATA_FIFO(struct drm_device *dev, u32 pipe)
{
	u32 gen_fifo_stat_reg = MIPIA_GEN_FIFO_STAT_REG;
	int timeout = 0;

	if (pipe == 2)
		gen_fifo_stat_reg += MIPIC_REG_OFFSET;

	udelay(500);

	/* This will time out after approximately 2+ seconds */
	while ((timeout < 20000) && (REG_READ(gen_fifo_stat_reg) & DSI_FIFO_GEN_HS_DATA_FULL)) {
		udelay(100);
		timeout++;
	}

	if (timeout == 20000)
		dev_warn(dev->dev, "MIPI: HS Data FIFO was never cleared!\n");
}

static void mdfld_wait_for_HS_CTRL_FIFO(struct drm_device *dev, u32 pipe)
{
	u32 gen_fifo_stat_reg = MIPIA_GEN_FIFO_STAT_REG;
	int timeout = 0;

	if (pipe == 2)
		gen_fifo_stat_reg += MIPIC_REG_OFFSET;

	udelay(500);

	/* This will time out after approximately 2+ seconds */
	while ((timeout < 20000) && (REG_READ(gen_fifo_stat_reg) & DSI_FIFO_GEN_HS_CTRL_FULL)) {
		udelay(100);
		timeout++;
	}
	if (timeout == 20000)
		dev_warn(dev->dev, "MIPI: HS CMD FIFO was never cleared!\n");
}

static void mdfld_wait_for_PIPEA_DISABLE(struct drm_device *dev, u32 pipe)
{
        u32 pipeconf_reg = PIPEACONF;
        int timeout = 0;

        if (pipe == 2)
                pipeconf_reg = PIPECCONF;

        udelay(500);

        /* This will time out after approximately 2+ seconds */
        while ((timeout < 20000) && (REG_READ(pipeconf_reg) & 0x40000000)) {
                udelay(100);
                timeout++;
        }

        if (timeout == 20000)
                dev_warn(dev->dev, "MIPI: PIPE was not disabled!\n");
}

static void mdfld_wait_for_DPI_CTRL_FIFO(struct drm_device *dev, u32 pipe)
{
	u32 gen_fifo_stat_reg = MIPIA_GEN_FIFO_STAT_REG;
        int timeout = 0;

	if (pipe == 2)
		gen_fifo_stat_reg += MIPIC_REG_OFFSET;

        udelay(500);

        /* This will time out after approximately 2+ seconds */
        while ((timeout < 20000) && ((REG_READ(gen_fifo_stat_reg) & DPI_FIFO_EMPTY)
                                                        != DPI_FIFO_EMPTY)) {
                udelay(100);
                timeout++;
        }

        if (timeout == 20000)
                dev_warn(dev->dev, "MIPI: DPI FIFO was never cleared!\n");
}

static void mdfld_wait_for_SPL_PKG_SENT(struct drm_device *dev, u32 pipe)
{
	u32 intr_stat_reg = MIPIA_INTR_STAT_REG;
	int timeout = 0;

	if (pipe == 2)
		intr_stat_reg += MIPIC_REG_OFFSET;

        udelay(500);

        /* This will time out after approximately 2+ seconds */
        while ((timeout < 20000) && (!(REG_READ(intr_stat_reg) & DSI_INTR_STATE_SPL_PKG_SENT))) {
                udelay(100);
                timeout++;
        }

        if (timeout == 20000)
                dev_warn(dev->dev, "MIPI: SPL_PKT_SENT_INTERRUPT was not sent successfully!\n");
}


/* ************************************************************************* *\
 * FUNCTION: mdfld_dsi_tpo_ic_init
 *
 * DESCRIPTION:  This function is called only by mrst_dsi_mode_set and
 *               restore_display_registers.  since this function does not
 *               acquire the mutex, it is important that the calling function
 *               does!
\* ************************************************************************* */
void mdfld_dsi_tpo_ic_init(struct mdfld_dsi_config *dsi_config, u32 pipe)
{
	struct drm_device *dev = dsi_config->dev;
	u32 dcsChannelNumber = dsi_config->channel_num;
	u32 gen_data_reg = MIPIA_HS_GEN_DATA_REG; 
	u32 gen_ctrl_reg = MIPIA_HS_GEN_CTRL_REG;
	u32 gen_ctrl_val = GEN_LONG_WRITE;

	dev_warn(dev->dev, "Enter mrst init TPO MIPI display.\n");

	if (pipe == 2) {
		gen_data_reg = HS_GEN_DATA_REG + MIPIC_REG_OFFSET; 
		gen_ctrl_reg = HS_GEN_CTRL_REG + MIPIC_REG_OFFSET;
	}

	gen_ctrl_val |= dcsChannelNumber << DCS_CHANNEL_NUMBER_POS;

	/* Flip page order */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x00008036);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x02 << WORD_COUNTS_POS));

	/* 0xF0 */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x005a5af0);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x03 << WORD_COUNTS_POS));

	/* Write protection key */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x005a5af1);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x03 << WORD_COUNTS_POS));

	/* 0xFC */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x005a5afc);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x03 << WORD_COUNTS_POS));

	/* 0xB7 */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x770000b7);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x00000044);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x05 << WORD_COUNTS_POS));

	/* 0xB6 */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x000a0ab6);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x03 << WORD_COUNTS_POS));

	/* 0xF2 */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x081010f2);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x4a070708);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x000000c5);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x09 << WORD_COUNTS_POS));

	/* 0xF8 */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x024003f8);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x01030a04);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x0e020220);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x00000004);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x0d << WORD_COUNTS_POS));

	/* 0xE2 */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x398fc3e2);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x0000916f);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x06 << WORD_COUNTS_POS));

	/* 0xB0 */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x000000b0);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x02 << WORD_COUNTS_POS));

	/* 0xF4 */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x240242f4);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x78ee2002);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x2a071050);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x507fee10);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x10300710);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x14 << WORD_COUNTS_POS));

	/* 0xBA */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x19fe07ba);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x101c0a31);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x00000010);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x09 << WORD_COUNTS_POS));

	/* 0xBB */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x28ff07bb);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x24280a31);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x00000034);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x09 << WORD_COUNTS_POS));

	/* 0xFB */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x535d05fb);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x1b1a2130);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x221e180e);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x131d2120);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x535d0508);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x1c1a2131);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x231f160d);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x111b2220);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x535c2008);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x1f1d2433);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x2c251a10);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x2c34372d);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x00000023);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x31 << WORD_COUNTS_POS));

	/* 0xFA */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x525c0bfa);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x1c1c232f);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x2623190e);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x18212625);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x545d0d0e);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x1e1d2333);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x26231a10);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x1a222725);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x545d280f);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x21202635);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x31292013);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x31393d33);
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x00000029);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x31 << WORD_COUNTS_POS));

	/* Set DM */
	mdfld_wait_for_HS_DATA_FIFO(dev, pipe);
	REG_WRITE(gen_data_reg, 0x000100f7);
	mdfld_wait_for_HS_CTRL_FIFO(dev, pipe);
	REG_WRITE(gen_ctrl_reg, gen_ctrl_val | (0x03 << WORD_COUNTS_POS));
}

/* ************************************************************************* *\
 * FUNCTION: mdfld_init_TMD_MIPI
 *
 * DESCRIPTION:  This function is called only by mrst_dsi_mode_set and
 *               restore_display_registers.  since this function does not
 *               acquire the mutex, it is important that the calling function
 *               does!
\* ************************************************************************* */

static u32 tmd_cmd_mcap_off[] = {0x000000b2};
static u32 tmd_cmd_enable_lane_switch[] = {0x000101ef};
static u32 tmd_cmd_set_lane_num[] = {0x006360ef};
static u32 tmd_cmd_set_mode[] = {0x000000b3};
static u32 tmd_cmd_set_sync_pulse_mode[] = {0x000961ef};
static u32 tmd_cmd_set_video_mode[] = {0x00000153};
static u32 tmd_cmd_enable_backlight[] = {0x00005ab4};//no auto_bl,need add in furtrue
static u32 tmd_cmd_set_backlight_dimming[] = {0x00000ebd}; 

void mdfld_dsi_tmd_drv_ic_init(struct mdfld_dsi_config *dsi_config, u32 pipe)
{
	struct mdfld_dsi_pkg_sender *sender = mdfld_dsi_get_pkg_sender(dsi_config);

	if(!sender) {
		WARN_ON(1);
		return;
	}

	if(dsi_config->dvr_ic_inited)
		return;

	msleep(3);

	mdfld_dsi_send_gen_long_lp(sender, tmd_cmd_mcap_off, 1, 0);
	mdfld_dsi_send_gen_long_lp(sender, tmd_cmd_enable_lane_switch, 1, 0);
	mdfld_dsi_send_gen_long_lp(sender, tmd_cmd_set_lane_num, 1, 0);
	mdfld_dsi_send_gen_long_lp(sender, tmd_cmd_set_mode, 1, 0);
	mdfld_dsi_send_gen_long_lp(sender, tmd_cmd_set_sync_pulse_mode, 1, 0);
	/*TODO: set page and column here*/
	mdfld_dsi_send_gen_long_lp(sender, tmd_cmd_set_video_mode, 1, 0);
	mdfld_dsi_send_gen_long_lp(sender, tmd_cmd_enable_backlight, 1, 0);
	mdfld_dsi_send_gen_long_lp(sender, tmd_cmd_set_backlight_dimming,1,0);
	dsi_config->dvr_ic_inited = 1;
}

static u16 mdfld_dsi_dpi_to_byte_clock_count(int pixel_clock_count, int num_lane, int bpp)
{
	return (u16)((pixel_clock_count * bpp) / (num_lane * 8)); 
}

/*
 * Calculate the dpi time basing on a given drm mode @mode
 * return 0 on success.
 * FIXME: I was using proposed mode value for calculation, may need to 
 * use crtc mode values later 
 */
int mdfld_dsi_dpi_timing_calculation(struct drm_display_mode *mode, 
									struct mdfld_dsi_dpi_timing *dpi_timing,
									int num_lane, int bpp)
{
	int pclk_hsync, pclk_hfp, pclk_hbp, pclk_hactive;
	int pclk_vsync, pclk_vfp, pclk_vbp, pclk_vactive;
	
	if(!mode || !dpi_timing) {
		DRM_ERROR("Invalid parameter\n");
		return -EINVAL;
	}
	
	pclk_hactive = mode->hdisplay;
	pclk_hfp = mode->hsync_start - mode->hdisplay;
	pclk_hsync = mode->hsync_end - mode->hsync_start;
	pclk_hbp = mode->htotal - mode->hsync_end;
	
	pclk_vactive = mode->vdisplay;
	pclk_vfp = mode->vsync_start - mode->vdisplay;
	pclk_vsync = mode->vsync_end - mode->vsync_start;
	pclk_vbp = mode->vtotal - mode->vsync_end;

#ifdef MIPI_DEBUG_LOG
	printk(KERN_ALERT "[DISPLAY] %s: pclk_hactive = %d\n", __func__, pclk_hactive);
	printk(KERN_ALERT "[DISPLAY] %s: pclk_hfp = %d\n", __func__, pclk_hfp);
	printk(KERN_ALERT "[DISPLAY] %s: pclk_hsync = %d\n", __func__, pclk_hsync);
	printk(KERN_ALERT "[DISPLAY] %s: pclk_hbp = %d\n", __func__, pclk_hbp);
	printk(KERN_ALERT "[DISPLAY] %s: pclk_vactive = %d\n", __func__, pclk_vactive);
	printk(KERN_ALERT "[DISPLAY] %s: pclk_vfp = %d\n", __func__, pclk_vfp);
	printk(KERN_ALERT "[DISPLAY] %s: pclk_vsync = %d\n", __func__, pclk_vsync);
	printk(KERN_ALERT "[DISPLAY] %s: pclk_vbp = %d\n", __func__, pclk_vbp);
#endif
	/*
	 * byte clock counts were calculated by following formula
	 * bclock_count = pclk_count * bpp / num_lane / 8
	 */
	dpi_timing->hsync_count = mdfld_dsi_dpi_to_byte_clock_count(pclk_hsync, num_lane, bpp);
	dpi_timing->hbp_count = mdfld_dsi_dpi_to_byte_clock_count(pclk_hbp, num_lane, bpp);
	dpi_timing->hfp_count = mdfld_dsi_dpi_to_byte_clock_count(pclk_hfp, num_lane, bpp);
	dpi_timing->hactive_count = mdfld_dsi_dpi_to_byte_clock_count(pclk_hactive, num_lane, bpp);
	dpi_timing->vsync_count = mdfld_dsi_dpi_to_byte_clock_count(pclk_vsync, num_lane, bpp);
	dpi_timing->vbp_count = mdfld_dsi_dpi_to_byte_clock_count(pclk_vbp, num_lane, bpp);
	dpi_timing->vfp_count = mdfld_dsi_dpi_to_byte_clock_count(pclk_vfp, num_lane, bpp);

	return 0; 
}

void mdfld_dsi_dpi_controller_init(struct mdfld_dsi_config *dsi_config, int pipe)
{
	struct drm_device *dev = dsi_config->dev;
	u32 reg_offset = pipe ? MIPIC_REG_OFFSET : 0;
	int lane_count = dsi_config->lane_count;
	struct mdfld_dsi_dpi_timing dpi_timing;
	struct drm_display_mode *mode = dsi_config->mode;
	u32 val = 0;
	
	/*un-ready device*/
	REG_WRITE((MIPIA_DEVICE_READY_REG + reg_offset), 0x00000000);
	
	/*init dsi adapter before kicking off*/
	REG_WRITE((MIPIA_CONTROL_REG + reg_offset), 0x00000018);
	
	/*enable all interrupts*/
	REG_WRITE((MIPIA_INTR_EN_REG + reg_offset), 0xffffffff);
	

	/*set up func_prg*/
	val |= lane_count;
	val |= dsi_config->channel_num << DSI_DPI_VIRT_CHANNEL_OFFSET;
		
	switch(dsi_config->bpp) {
	case 16:
		val |= DSI_DPI_COLOR_FORMAT_RGB565;
		break;
	case 18:
		val |= DSI_DPI_COLOR_FORMAT_RGB666;
		break;
	case 24:
		val |= DSI_DPI_COLOR_FORMAT_RGB888;
		break;
	default:
		DRM_ERROR("unsupported color format, bpp = %d\n", dsi_config->bpp);
	}
	REG_WRITE((MIPIA_DSI_FUNC_PRG_REG + reg_offset), val);
	
	REG_WRITE((MIPIA_HS_TX_TIMEOUT_REG + reg_offset), 
			(mode->vtotal * mode->htotal * dsi_config->bpp / (8 * lane_count)) & DSI_HS_TX_TIMEOUT_MASK);
	REG_WRITE((MIPIA_LP_RX_TIMEOUT_REG + reg_offset), 0xffff & DSI_LP_RX_TIMEOUT_MASK);
	
	/*max value: 20 clock cycles of txclkesc*/
	REG_WRITE((MIPIA_TURN_AROUND_TIMEOUT_REG + reg_offset), 0x14 & DSI_TURN_AROUND_TIMEOUT_MASK);
	
	/*min 21 txclkesc, max: ffffh*/
	REG_WRITE((MIPIA_DEVICE_RESET_TIMER_REG + reg_offset), 0xffff & DSI_RESET_TIMER_MASK);

	REG_WRITE((MIPIA_DPI_RESOLUTION_REG + reg_offset), mode->vdisplay << 16 | mode->hdisplay);
	
	/*set DPI timing registers*/
	mdfld_dsi_dpi_timing_calculation(mode, &dpi_timing, dsi_config->lane_count, dsi_config->bpp);
	
	REG_WRITE((MIPIA_HSYNC_COUNT_REG + reg_offset), dpi_timing.hsync_count & DSI_DPI_TIMING_MASK);
	REG_WRITE((MIPIA_HBP_COUNT_REG + reg_offset), dpi_timing.hbp_count & DSI_DPI_TIMING_MASK);
	REG_WRITE((MIPIA_HFP_COUNT_REG + reg_offset), dpi_timing.hfp_count & DSI_DPI_TIMING_MASK);
	REG_WRITE((MIPIA_HACTIVE_COUNT_REG + reg_offset), dpi_timing.hactive_count & DSI_DPI_TIMING_MASK);
	REG_WRITE((MIPIA_VSYNC_COUNT_REG + reg_offset), dpi_timing.vsync_count & DSI_DPI_TIMING_MASK);
	REG_WRITE((MIPIA_VBP_COUNT_REG + reg_offset), dpi_timing.vbp_count & DSI_DPI_TIMING_MASK);
	REG_WRITE((MIPIA_VFP_COUNT_REG + reg_offset), dpi_timing.vfp_count & DSI_DPI_TIMING_MASK);
	
	REG_WRITE((MIPIA_HIGH_LOW_SWITCH_COUNT_REG + reg_offset), 0x46);
	
	/*min: 7d0 max: 4e20*/
	REG_WRITE((MIPIA_INIT_COUNT_REG + reg_offset), 0x000007d0);
	
	/*set up video mode*/
	val = 0;
	val = dsi_config->video_mode | DSI_DPI_COMPLETE_LAST_LINE;
	REG_WRITE((MIPIA_VIDEO_MODE_FORMAT_REG + reg_offset), val);
	
	REG_WRITE((MIPIA_EOT_DISABLE_REG + reg_offset), 0x00000000);
	
	REG_WRITE((MIPIA_LP_BYTECLK_REG + reg_offset), 0x00000004);
	
	/*TODO: figure out how to setup these registers*/
	REG_WRITE((MIPIA_DPHY_PARAM_REG + reg_offset), 0x150c3408);
	
	REG_WRITE((MIPIA_CLK_LANE_SWITCH_TIME_CNT_REG + reg_offset), (0xa << 16) | 0x14);
	/*set device ready*/
	REG_WRITE((MIPIA_DEVICE_READY_REG + reg_offset), 0x00000001);
}

void mdfld_dsi_dpi_turn_on(struct mdfld_dsi_dpi_output *output, int pipe)
{
	struct drm_device *dev = output->dev;
	/* struct drm_psb_private *dev_priv = dev->dev_private; */
	u32 reg_offset = 0;
	
	if(output->panel_on) 
		return;
		
	if(pipe) 
		reg_offset = MIPIC_REG_OFFSET;

	/* clear special packet sent bit */
	if(REG_READ(MIPIA_INTR_STAT_REG + reg_offset) & DSI_INTR_STATE_SPL_PKG_SENT) {
		REG_WRITE((MIPIA_INTR_STAT_REG + reg_offset), DSI_INTR_STATE_SPL_PKG_SENT);
	}
		
	/*send turn on package*/
	REG_WRITE((MIPIA_DPI_CONTROL_REG + reg_offset), DSI_DPI_CTRL_HS_TURN_ON);
	
	/*wait for SPL_PKG_SENT interrupt*/
	mdfld_wait_for_SPL_PKG_SENT(dev, pipe);
	
	if(REG_READ(MIPIA_INTR_STAT_REG + reg_offset) & DSI_INTR_STATE_SPL_PKG_SENT) {
		REG_WRITE((MIPIA_INTR_STAT_REG + reg_offset), DSI_INTR_STATE_SPL_PKG_SENT);
	}

	output->panel_on = 1;

	/* FIXME the following is disabled to WA the X slow start issue for TMD panel */
	/* if(pipe == 2) */
	/* 	dev_priv->dpi_panel_on2 = true; */
	/* else if (pipe == 0) */
	/* 	dev_priv->dpi_panel_on = true; */
}

static void mdfld_dsi_dpi_shut_down(struct mdfld_dsi_dpi_output *output, int pipe)
{
	struct drm_device *dev = output->dev;
	/* struct drm_psb_private *dev_priv = dev->dev_private; */
	u32 reg_offset = 0;
	
	/*if output is on, or mode setting didn't happen, ignore this*/
	if((!output->panel_on) || output->first_boot) {
		output->first_boot = 0; 
		return;
	}
	
	if(pipe)
		reg_offset = MIPIC_REG_OFFSET;

	/* Wait for dpi fifo to empty */
	mdfld_wait_for_DPI_CTRL_FIFO(dev, pipe);

	/* Clear the special packet interrupt bit if set */
	if(REG_READ(MIPIA_INTR_STAT_REG + reg_offset) & DSI_INTR_STATE_SPL_PKG_SENT) {
		REG_WRITE((MIPIA_INTR_STAT_REG + reg_offset), DSI_INTR_STATE_SPL_PKG_SENT);
	}
	
	if(REG_READ(MIPIA_DPI_CONTROL_REG + reg_offset) == DSI_DPI_CTRL_HS_SHUTDOWN) {
		goto shutdown_out;
	}
	
	REG_WRITE((MIPIA_DPI_CONTROL_REG + reg_offset), DSI_DPI_CTRL_HS_SHUTDOWN);

shutdown_out:
	output->panel_on = 0;
	output->first_boot = 0;

	/* FIXME the following is disabled to WA the X slow start issue for TMD panel */
	/* if(pipe == 2) */
	/* 	dev_priv->dpi_panel_on2 = false; */
	/* else if (pipe == 0) */
	/* 	dev_priv->dpi_panel_on = false;	 */
}

void mdfld_dsi_dpi_set_power(struct drm_encoder *encoder, bool on)
{
	struct mdfld_dsi_encoder *dsi_encoder = MDFLD_DSI_ENCODER(encoder);
	struct mdfld_dsi_dpi_output *dpi_output = MDFLD_DSI_DPI_OUTPUT(dsi_encoder);
	struct mdfld_dsi_config *dsi_config = mdfld_dsi_encoder_get_config(dsi_encoder);
	int pipe = mdfld_dsi_encoder_get_pipe(dsi_encoder);
	struct drm_device *dev = dsi_config->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	u32 mipi_reg = MIPI;
	u32 pipeconf_reg = PIPEACONF;
	
	if(pipe) {
		mipi_reg = MIPI_C;
		pipeconf_reg = PIPECCONF;
	}
	
	/* Start up display island if it was shutdown */
	if (!gma_power_begin(dev, true))
		return;

	if(on) {
		if (mdfld_get_panel_type(dev, pipe) == TMD_VID){
 			mdfld_dsi_dpi_turn_on(dpi_output, pipe);
 		} else {
			/*enable mipi port*/
			REG_WRITE(mipi_reg, (REG_READ(mipi_reg) | (1 << 31)));
			REG_READ(mipi_reg);

			mdfld_dsi_dpi_turn_on(dpi_output, pipe);
			mdfld_dsi_tpo_ic_init(dsi_config, pipe);
		}

		if(pipe == 2) {
			dev_priv->dpi_panel_on2 = true;
		}
		else {
			dev_priv->dpi_panel_on  = true;
		}

	} else {
 		if (mdfld_get_panel_type(dev, pipe) == TMD_VID) {
 			mdfld_dsi_dpi_shut_down(dpi_output, pipe);
 		} else {
			mdfld_dsi_dpi_shut_down(dpi_output, pipe);
			/*disable mipi port*/
			REG_WRITE(mipi_reg, (REG_READ(mipi_reg) & ~(1<<31)));
			REG_READ(mipi_reg);
		}

		if(pipe == 2)
			dev_priv->dpi_panel_on2 = false;
		else
			dev_priv->dpi_panel_on  = false;
	}
	gma_power_end(dev);
}

void mdfld_dsi_dpi_dpms(struct drm_encoder *encoder, int mode)
{
	dev_dbg(encoder->dev->dev, "DPMS %s\n",
			(mode == DRM_MODE_DPMS_ON ? "on":"off"));

	if (mode == DRM_MODE_DPMS_ON)
		mdfld_dsi_dpi_set_power(encoder, true);
	else
		mdfld_dsi_dpi_set_power(encoder, false);
}

bool mdfld_dsi_dpi_mode_fixup(struct drm_encoder *encoder,
				     struct drm_display_mode *mode,
				     struct drm_display_mode *adjusted_mode)
{
	struct mdfld_dsi_encoder *dsi_encoder = MDFLD_DSI_ENCODER(encoder);
	struct mdfld_dsi_config *dsi_config = mdfld_dsi_encoder_get_config(dsi_encoder);
	struct drm_display_mode *fixed_mode = dsi_config->fixed_mode;

	if(fixed_mode) {
		adjusted_mode->hdisplay = fixed_mode->hdisplay;
		adjusted_mode->hsync_start = fixed_mode->hsync_start;
		adjusted_mode->hsync_end = fixed_mode->hsync_end;
		adjusted_mode->htotal = fixed_mode->htotal;
		adjusted_mode->vdisplay = fixed_mode->vdisplay;
		adjusted_mode->vsync_start = fixed_mode->vsync_start;
		adjusted_mode->vsync_end = fixed_mode->vsync_end;
		adjusted_mode->vtotal = fixed_mode->vtotal;
		adjusted_mode->clock = fixed_mode->clock;
		drm_mode_set_crtcinfo(adjusted_mode, CRTC_INTERLACE_HALVE_V);
	}
	
	return true;
}

void mdfld_dsi_dpi_prepare(struct drm_encoder *encoder) 
{
	mdfld_dsi_dpi_set_power(encoder, false);
}

void mdfld_dsi_dpi_commit(struct drm_encoder *encoder) 
{
	mdfld_dsi_dpi_set_power(encoder, true);
}

void dsi_debug_MIPI_reg(struct drm_device *dev)
{
	u32 temp_val = 0;

	temp_val = REG_READ(MIPI);
	printk(KERN_ALERT "[DISPLAY] MIPI = %x\n", temp_val);

	/* set the lane speed */
	temp_val = REG_READ(MIPI_CONTROL_REG);
	printk(KERN_ALERT "[DISPLAY] MIPI_CONTROL_REG = %x\n", temp_val);

	/* Enable all the error interrupt */
	temp_val = REG_READ(INTR_EN_REG);
	printk(KERN_ALERT "[DISPLAY] INTR_EN_REG = %x\n", temp_val);
	temp_val = REG_READ(TURN_AROUND_TIMEOUT_REG);
	printk(KERN_ALERT "[DISPLAY] TURN_AROUND_TIMEOUT_REG = %x\n", temp_val);
	temp_val = REG_READ(DEVICE_RESET_REG);
	printk(KERN_ALERT "[DISPLAY] DEVICE_RESET_REG = %x\n", temp_val);
	temp_val = REG_READ(INIT_COUNT_REG);
	printk(KERN_ALERT "[DISPLAY] INIT_COUNT_REG = %x\n", temp_val);

	temp_val = REG_READ(DSI_FUNC_PRG_REG);
	printk(KERN_ALERT "[DISPLAY] DSI_FUNC_PRG_REG = %x\n", temp_val);

	temp_val = REG_READ(DPI_RESOLUTION_REG);
	printk(KERN_ALERT "[DISPLAY] DPI_RESOLUTION_REG = %x\n", temp_val);

	temp_val = REG_READ(VERT_SYNC_PAD_COUNT_REG);
	printk(KERN_ALERT "[DISPLAY] VERT_SYNC_PAD_COUNT_REG = %x\n", temp_val);
	temp_val = REG_READ(VERT_BACK_PORCH_COUNT_REG);
	printk(KERN_ALERT "[DISPLAY] VERT_BACK_PORCH_COUNT_REG = %x\n", temp_val);
	temp_val = REG_READ(VERT_FRONT_PORCH_COUNT_REG);
	printk(KERN_ALERT "[DISPLAY] VERT_FRONT_PORCH_COUNT_REG = %x\n", temp_val);

	temp_val = REG_READ(HORIZ_SYNC_PAD_COUNT_REG);
	printk(KERN_ALERT "[DISPLAY] HORIZ_SYNC_PAD_COUNT_REG = %x\n", temp_val);
	temp_val = REG_READ(HORIZ_BACK_PORCH_COUNT_REG);
	printk(KERN_ALERT "[DISPLAY] HORIZ_BACK_PORCH_COUNT_REG = %x\n", temp_val);
	temp_val = REG_READ(HORIZ_FRONT_PORCH_COUNT_REG);
	printk(KERN_ALERT "[DISPLAY] HORIZ_FRONT_PORCH_COUNT_REG = %x\n", temp_val);
	temp_val = REG_READ(HORIZ_ACTIVE_AREA_COUNT_REG);
	printk(KERN_ALERT "[DISPLAY] HORIZ_ACTIVE_AREA_COUNT_REG = %x\n", temp_val);

	temp_val = REG_READ(VIDEO_FMT_REG);
	printk(KERN_ALERT "[DISPLAY] VIDEO_FMT_REG = %x\n", temp_val);

	temp_val = REG_READ(HS_TX_TIMEOUT_REG);
	printk(KERN_ALERT "[DISPLAY] HS_TX_TIMEOUT_REG = %x\n", temp_val);
	temp_val = REG_READ(LP_RX_TIMEOUT_REG);
	printk(KERN_ALERT "[DISPLAY] LP_RX_TIMEOUT_REG = %x\n", temp_val);

	temp_val = REG_READ(HIGH_LOW_SWITCH_COUNT_REG);
	printk(KERN_ALERT "[DISPLAY] HIGH_LOW_SWITCH_COUNT_REG = %x\n", temp_val);

	temp_val = REG_READ(EOT_DISABLE_REG);
	printk(KERN_ALERT "[DISPLAY] EOT_DISABLE_REG = %x\n", temp_val);

	temp_val = REG_READ(LP_BYTECLK_REG);
	printk(KERN_ALERT "[DISPLAY] LP_BYTECLK_REG = %x\n", temp_val);
	temp_val = REG_READ(MAX_RET_PAK_REG);
	printk(KERN_ALERT "[DISPLAY] MAX_RET_PAK_REG = %x\n", temp_val);
	temp_val = REG_READ(DPI_CONTROL_REG);
	printk(KERN_ALERT "[DISPLAY] DPI_CONTROL_REG = %x\n", temp_val);
	temp_val = REG_READ(DPHY_PARAM_REG);
	printk(KERN_ALERT "[DISPLAY] DPHY_PARAM_REG = %x\n", temp_val);
//	temp_val = REG_READ(PIPEACONF);
//	printk(KERN_INFO "[DISPLAY] PIPEACONF = %x\n", temp_val);
//	temp_val = REG_READ(DSPACNTR);
//	printk(KERN_INFO "[DISPLAY] DSPACNTR = %x\n", temp_val);
}

void mdfld_dsi_dpi_mode_set(struct drm_encoder *encoder,
				   struct drm_display_mode *mode,
				   struct drm_display_mode *adjusted_mode)
{
	struct mdfld_dsi_encoder *dsi_encoder = MDFLD_DSI_ENCODER(encoder);
	struct mdfld_dsi_dpi_output *dpi_output = MDFLD_DSI_DPI_OUTPUT(dsi_encoder);
	struct mdfld_dsi_config *dsi_config = mdfld_dsi_encoder_get_config(dsi_encoder);
	struct drm_device *dev = dsi_config->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	int pipe = mdfld_dsi_encoder_get_pipe(dsi_encoder);
	
	u32 pipeconf_reg = PIPEACONF;
	u32 dspcntr_reg = DSPACNTR;
	u32 mipi_reg = MIPI;
	u32 reg_offset = 0;
	
	u32 pipeconf = dev_priv->pipeconf;
	u32 dspcntr = dev_priv->dspcntr;
	u32 mipi = MIPI_PORT_EN | PASS_FROM_SPHY_TO_AFE | SEL_FLOPPED_HSTX;
	
	dev_dbg(dev->dev, "set mode %dx%d on pipe %d\n",
				mode->hdisplay, mode->vdisplay, pipe);

	if(pipe) {
		pipeconf_reg = PIPECCONF;
		dspcntr_reg = DSPCCNTR;
		mipi_reg = MIPI_C;
		reg_offset = MIPIC_REG_OFFSET;
	} else {
		mipi |= 2;
	}
	
	if (!gma_power_begin(dev, true))
		return;

	/* Set up mipi port FIXME: do at init time */
	REG_WRITE(mipi_reg, mipi);
	REG_READ(mipi_reg);

	/* Set up DSI controller DPI interface*/
	mdfld_dsi_dpi_controller_init(dsi_config, pipe);

	if (mdfld_get_panel_type(dev, pipe) == TMD_VID) {
 		/* init driver ic */
 		mdfld_dsi_tmd_drv_ic_init(dsi_config, pipe);
	} else {
		/*turn on DPI interface*/
		mdfld_dsi_dpi_turn_on(dpi_output, pipe);
	}
	
	/* Set up pipe */
	REG_WRITE(pipeconf_reg, pipeconf);
	REG_READ(pipeconf_reg);
	
	/* Set up display plane */
	REG_WRITE(dspcntr_reg, dspcntr);
	REG_READ(dspcntr_reg);
	
	msleep(20); /* FIXME: this should wait for vblank */
	
	dev_dbg(dev->dev, "State %x, power %d\n",
		REG_READ(MIPIA_INTR_STAT_REG + reg_offset),
		dpi_output->panel_on);

	if (mdfld_get_panel_type(dev, pipe) == TMD_VID) {
		//mdfld_dsi_dpi_turn_on(dpi_output, pipe);
	} else {
		/* init driver ic */
		mdfld_dsi_tpo_ic_init(dsi_config, pipe);
		/*init backlight*/
		mdfld_dsi_brightness_init(dsi_config, pipe);
	}
	
#ifdef MIPI_DEBUG_LOG
	dsi_debug_MIPI_reg(dev);
#endif
	gma_power_end(dev);
}

static int mdfld_dpi_panel_reset(int pipe)
{
	unsigned gpio;
	int ret = 0;
	
	switch(pipe) {
	case 0:
		gpio = 128;
		break;
	case 2:
		gpio = 34;
		break;
	default:
		DRM_ERROR("Invalid output\n");
		return -EINVAL;
	}
	
	ret = gpio_request(gpio, "gfx");
	if(ret) {
		DRM_ERROR("gpio_rqueset failed\n");
		return ret;
	}
	ret = gpio_direction_output(gpio, 1);
	if(ret) {
		DRM_ERROR("gpio_direction_output failed\n");
		goto gpio_error;
	}
	
	gpio_get_value(128);
	
gpio_error:
	if(gpio_is_valid(gpio))
		gpio_free(gpio);
	return ret;
}

/**
 * Exit from DSR
 */
void mdfld_dsi_dpi_exit_idle (struct drm_device *dev, u32 update_src, void *p_surfaceAddr, bool check_hw_on_only)
{
	struct drm_psb_private *dev_priv = dev->dev_private;

	if (!gma_power_begin(dev, true)) {
		DRM_ERROR("hw begin failed\n");
		return;
	}

	/* update the surface base address. */
	if (p_surfaceAddr) {
		REG_WRITE(DSPASURF, *((u32 *)p_surfaceAddr));
#if defined(CONFIG_MDFD_DUAL_MIPI)
		REG_WRITE(DSPCSURF, *((u32 *)p_surfaceAddr));
#endif
	}
	mid_enable_pipe_event(dev_priv, 0);
	psb_enable_pipestat(dev_priv, 0, PIPE_VBLANK_INTERRUPT_ENABLE);
	dev_priv->is_in_idle = false;
	dev_priv->dsr_idle_count = 0;
}

/*
 * Init DSI DPI encoder. 
 * Allocate an mdfld_dsi_encoder and attach it to given @dsi_connector
 * return pointer of newly allocated DPI encoder, NULL on error
 */ 
struct mdfld_dsi_encoder *mdfld_dsi_dpi_init(struct drm_device *dev, 
				struct mdfld_dsi_connector *dsi_connector,
				struct panel_funcs*p_funcs)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct mdfld_dsi_dpi_output *dpi_output = NULL;
	struct mdfld_dsi_config *dsi_config;
	struct drm_connector *connector = NULL;
	struct drm_encoder *encoder = NULL;
	struct drm_display_mode *fixed_mode = NULL;
	int ret;

	if (!dsi_connector) {
		WARN_ON(1);
		return NULL;
	}

	dpi_output = kzalloc(sizeof(struct mdfld_dsi_dpi_output), GFP_KERNEL);
	if(!dpi_output) {
		dev_err(dev->dev, "No memory for dsi_dpi_output\n");
		return NULL;
	}
	/* Panel reset */
	ret = mdfld_dpi_panel_reset(dsi_connector->pipe);
	if(ret) {
		DRM_ERROR("reset panel error\n");
		goto out_err1;
	}
	
	if(dsi_connector->pipe) 
		dpi_output->panel_on = 0;

		dpi_output->panel_on = 0;
	
	
	dpi_output->dev = dev;
	dpi_output->first_boot = 1;
	
	/* Get fixed mode */
	dsi_config = mdfld_dsi_get_config(dsi_connector);
	fixed_mode = dsi_config->fixed_mode;
	
	/* Create drm encoder object */
	connector = &dsi_connector->base.base;
	encoder = &dpi_output->base.base;
	drm_encoder_init(dev,
			encoder,
			p_funcs->encoder_funcs,
			DRM_MODE_ENCODER_MIPI);
	drm_encoder_helper_add(encoder,
				p_funcs->encoder_helper_funcs);
	
	/* Attach to given connector */
	drm_mode_connector_attach_encoder(connector, encoder);
	
	/* Set possible crtcs and clones */
	if(dsi_connector->pipe) {
		encoder->possible_crtcs = (1 << 2);
		encoder->possible_clones = (1 << 1);
	} else {
		encoder->possible_crtcs = (1 << 0);
		encoder->possible_clones = (1 << 0);
	}

	dev_priv->dsr_fb_update = 0;
	dev_priv->dsr_enable = false;
	dev_priv->exit_idle = mdfld_dsi_dpi_exit_idle;
#if defined(CONFIG_MDFLD_DSI_DPU) || defined(CONFIG_MDFLD_DSI_DSR)
	dev_priv->dsr_enable_config = true;
#endif /*CONFIG_MDFLD_DSI_DSR*/

	return &dpi_output->base;
	
out_err1: 
	if(dpi_output)
		kfree(dpi_output);
	return NULL;	
}
