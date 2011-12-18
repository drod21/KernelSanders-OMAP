/*
 * omap-mcpdm.c  --  OMAP ALSA SoC DAI driver using McPDM port
 *
 * Copyright (C) 2009 Texas Instruments
 *
 * Author: Misael Lopez Cruz <x0052729@ti.com>
 * Contact: Jorge Eduardo Candelaria <x0107209@ti.com>
 *          Margarita Olaya <magi.olaya@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <plat/dma.h>
#include <plat/omap_hwmod.h>
#include <plat/mcpdm.h>
#include "../../../arch/arm/mach-omap2/cm1_44xx.h"
#include "omap-mcpdm.h"
#include "omap-pcm.h"
#if defined(CONFIG_SND_OMAP_SOC_ABE_DSP) ||\
	defined(CONFIG_SND_OMAP_SOC_ABE_DSP_MODULE)
#include "omap-abe-dsp.h"
#include "abe/abe_main.h"
#include "abe/port_mgr.h"
#endif

#define MCPDM_LEGACY_DAI_DL1	0
#define MCPDM_LEGACY_DAI_UL1	1
#define MCPDM_ABE_DAI_DL1		2
#define MCPDM_ABE_DAI_DL2		3
#define MCPDM_ABE_DAI_VIB		4
#define MCPDM_ABE_DAI_UL1		5

#define CLKCTRL_MODULEMODE_MASK 0x0003
#define CLKCTRL_MODULEMODE_DISABLED 0x0000
#define CLKCTRL_MODULEMODE_ENABLED 0x0002

struct omap_mcpdm {
	struct device *dev;
	unsigned long phys_base;
	void __iomem *io_base;
	int irq;

	struct mutex mutex;
	struct omap_mcpdm_platform_data *pdata;
	struct completion irq_completion;
	struct delayed_work esd_work;
	struct abe *abe;
	struct omap_abe_port *dl_port;
	struct omap_abe_port *ul_port;

	u32 *reg_cache;

	/* channel data */
	u32 dn_channels;
	u32 up_channels;
	int active;
	int abe_mode;

	/* DC offset */
	unsigned long dl1_offset;
	unsigned long dl2_offset;
};

/*
 * Stream DMA parameters
 */
static struct omap_pcm_dma_data omap_mcpdm_dai_dma_params[] = {
	{
		.name = "Audio playback",
		.dma_req = OMAP44XX_DMA_MCPDM_DL,
		.data_type = OMAP_DMA_DATA_TYPE_S32,
		.sync_mode = OMAP_DMA_SYNC_PACKET,
		.packet_size = 16,
		.port_addr = OMAP44XX_MCPDM_L3_BASE + MCPDM_DN_DATA,
	},
	{
		.name = "Audio capture",
		.dma_req = OMAP44XX_DMA_MCPDM_UP,
		.data_type = OMAP_DMA_DATA_TYPE_S32,
		.sync_mode = OMAP_DMA_SYNC_PACKET,
		.packet_size = 16,
		.port_addr = OMAP44XX_MCPDM_L3_BASE + MCPDM_UP_DATA,
	},
};

static inline void omap_mcpdm_write(struct omap_mcpdm *mcpdm,
		u16 reg, u32 val)
{
	__raw_writel(val, mcpdm->io_base + reg);
}

static inline int omap_mcpdm_read(struct omap_mcpdm *mcpdm, u16 reg)
{
	return __raw_readl(mcpdm->io_base + reg);
}

static inline void omap_mcpdm_write_cache(struct omap_mcpdm *mcpdm,
		u16 reg, u32 val)
{
	mcpdm->reg_cache[reg / sizeof(u32)] = val;
}

static inline int omap_mcpdm_read_cache(struct omap_mcpdm *mcpdm, u16 reg)
{
	return mcpdm->reg_cache[reg / sizeof(u32)];
}

#ifdef DEBUG
static void omap_mcpdm_reg_dump(struct omap_mcpdm *mcpdm)
{
	dev_dbg(mcpdm->dev, "***********************\n");
	dev_dbg(mcpdm->dev, "IRQSTATUS_RAW:  0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_IRQSTATUS_RAW));
	dev_dbg(mcpdm->dev, "IRQSTATUS:  0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_IRQSTATUS));
	dev_dbg(mcpdm->dev, "IRQENABLE_SET:  0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_IRQENABLE_SET));
	dev_dbg(mcpdm->dev, "IRQENABLE_CLR:  0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_IRQENABLE_CLR));
	dev_dbg(mcpdm->dev, "IRQWAKE_EN: 0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_IRQWAKE_EN));
	dev_dbg(mcpdm->dev, "DMAENABLE_SET: 0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_DMAENABLE_SET));
	dev_dbg(mcpdm->dev, "DMAENABLE_CLR:  0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_DMAENABLE_CLR));
	dev_dbg(mcpdm->dev, "DMAWAKEEN:  0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_DMAWAKEEN));
	dev_dbg(mcpdm->dev, "CTRL:  0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_CTRL));
	dev_dbg(mcpdm->dev, "DN_DATA:  0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_DN_DATA));
	dev_dbg(mcpdm->dev, "UP_DATA: 0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_UP_DATA));
	dev_dbg(mcpdm->dev, "FIFO_CTRL_DN: 0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_FIFO_CTRL_DN));
	dev_dbg(mcpdm->dev, "FIFO_CTRL_UP:  0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_FIFO_CTRL_UP));
	dev_dbg(mcpdm->dev, "DN_OFFSET:  0x%04x\n",
			omap_mcpdm_read(mcpdm, MCPDM_DN_OFFSET));
	dev_dbg(mcpdm->dev, "***********************\n");
}
#else
static void omap_mcpdm_reg_dump(struct omap_mcpdm *mcpdm) {}
#endif

/*
 * Enables the transfer through the PDM interface to/from the Phoenix
 * codec by enabling the corresponding UP and DN channels.
 */
static void omap_mcpdm_start(struct omap_mcpdm *mcpdm)
{
	u32 ctrl = omap_mcpdm_read(mcpdm, MCPDM_CTRL);

	ctrl |= (SW_UP_RST | SW_DN_RST);
	omap_mcpdm_write(mcpdm, MCPDM_CTRL, ctrl);
	ctrl |= (mcpdm->up_channels | mcpdm->dn_channels);
	omap_mcpdm_write(mcpdm, MCPDM_CTRL, ctrl);
	ctrl &= ~(SW_UP_RST | SW_DN_RST);
	omap_mcpdm_write(mcpdm, MCPDM_CTRL, ctrl);
}

/*
 * Disables the transfer through the PDM interface to/from the Phoenix
 * codec by disabling the corresponding UP and DN channels.
 */
static void omap_mcpdm_stop(struct omap_mcpdm *mcpdm)
{
	u32 ctrl = omap_mcpdm_read(mcpdm, MCPDM_CTRL);

	ctrl |= (SW_UP_RST | SW_DN_RST);
	omap_mcpdm_write(mcpdm, MCPDM_CTRL, ctrl);
	ctrl &= ~(mcpdm->up_channels | mcpdm->dn_channels);
	omap_mcpdm_write(mcpdm, MCPDM_CTRL, ctrl);
	ctrl &= ~(SW_UP_RST | SW_DN_RST);
	omap_mcpdm_write(mcpdm, MCPDM_CTRL, ctrl);
}

/*
 * Is the physical McPDM interface active.
 */
static inline int omap_mcpdm_active(struct omap_mcpdm *mcpdm)
{
	return omap_mcpdm_read(mcpdm, MCPDM_CTRL) & (PDM_DN_MASK | PDM_UP_MASK);
}

/*
 * Configures McPDM uplink/downlink for audio recording/playback
 * This function should be called before omap_mcpdm_start.
 */
static void omap_mcpdm_open(struct omap_mcpdm *mcpdm)
{
	/* Enable irq request generation */
	omap_mcpdm_write(mcpdm, MCPDM_IRQENABLE_SET,
			MCPDM_UP_IRQ_EMPTY | MCPDM_UP_IRQ_FULL |
			MCPDM_DN_IRQ_EMPTY | MCPDM_DN_IRQ_FULL);

	/* Configure uplink threshold */
	omap_mcpdm_write(mcpdm, MCPDM_FIFO_CTRL_UP, 2);
	omap_mcpdm_write(mcpdm, MCPDM_FIFO_CTRL_DN, 2);

	/* Configure DMA controller */
	omap_mcpdm_write(mcpdm, MCPDM_DMAENABLE_SET,
			DMA_UP_ENABLE | DMA_DN_ENABLE);
}

/*
 * Cleans McPDM uplink/downlink configuration.
 * This function should be called when the stream is closed.
 */
static void omap_mcpdm_close(struct omap_mcpdm *mcpdm)
{
	/* Disable irq request generation */
	omap_mcpdm_write(mcpdm, MCPDM_IRQENABLE_CLR,
			MCPDM_UP_IRQ_EMPTY | MCPDM_UP_IRQ_FULL |
			MCPDM_DN_IRQ_EMPTY | MCPDM_DN_IRQ_FULL);

	/* Disable DMA request generation */
	omap_mcpdm_write(mcpdm, MCPDM_DMAENABLE_CLR,
			DMA_UP_ENABLE | DMA_DN_ENABLE);
}

static irqreturn_t omap_mcpdm_irq_handler(int irq, void *dev_id)
{
	struct omap_mcpdm *mcpdm = dev_id;
	int irq_status;

	irq_status = omap_mcpdm_read(mcpdm, MCPDM_IRQSTATUS);

	/* Acknowledge irq event */
	omap_mcpdm_write(mcpdm, MCPDM_IRQSTATUS, irq_status);

	if (irq & MCPDM_DN_IRQ_FULL)
		dev_err(mcpdm->dev, "DN FIFO error %x\n", irq_status);

	if (irq & MCPDM_DN_IRQ_EMPTY)
		dev_err(mcpdm->dev, "DN FIFO error %x\n", irq_status);

	if (irq & MCPDM_DN_IRQ)
		dev_dbg(mcpdm->dev, "DN write request\n");

	if (irq & MCPDM_UP_IRQ_FULL)
		dev_err(mcpdm->dev, "UP FIFO error %x\n", irq_status);

	if (irq & MCPDM_UP_IRQ_EMPTY)
		dev_err(mcpdm->dev, "UP FIFO error %x\n", irq_status);

	if (irq & MCPDM_UP_IRQ)
		dev_dbg(mcpdm->dev, "UP write request\n");

	return IRQ_HANDLED;
}

/* Enable/disable DC offset cancelation for the analog
 * headset path (PDM channels 1 and 2).
 */
static void omap_mcpdm_set_offset(struct omap_mcpdm *mcpdm)
{
	int offset;

	if (mcpdm->dl1_offset > DN_OFST_MAX) {
		dev_err(mcpdm->dev, "DC DL1 offset out of range\n");
		return;
	}

	if (mcpdm->dl2_offset > DN_OFST_MAX) {
		dev_err(mcpdm->dev, "DC DL2 offset out of range\n");
		return;
	}

	offset = (mcpdm->dl1_offset << DN_OFST_RX1) |
			(mcpdm->dl2_offset << DN_OFST_RX2);

	/* offset cancellation for channel 1 */
	if (mcpdm->dl1_offset)
		offset |= DN_OFST_RX1_EN;
	else
		offset &= ~DN_OFST_RX1_EN;

	/* offset cancellation for channel 2 */
	if (mcpdm->dl2_offset)
		offset |= DN_OFST_RX2_EN;
	else
		offset &= ~DN_OFST_RX2_EN;

	omap_mcpdm_write(mcpdm, MCPDM_DN_OFFSET, offset);
}

static ssize_t mcpdm_dl1_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct omap_mcpdm *mcpdm = dev_get_drvdata(dev);

	return sprintf(buf, "%ld\n", mcpdm->dl1_offset);
}

static ssize_t mcpdm_dl1_set(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct omap_mcpdm *mcpdm = dev_get_drvdata(dev);
	int ret;
	unsigned long value;

	ret = strict_strtol(buf, 10, &value);
	if (ret)
		return ret;

	if (value > DN_OFST_MAX)
		return -EINVAL;

	mcpdm->dl1_offset = value;
	return count;
}

static ssize_t mcpdm_dl2_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct omap_mcpdm *mcpdm = dev_get_drvdata(dev);

	return sprintf(buf, "%ld\n", mcpdm->dl2_offset);
}

static ssize_t mcpdm_dl2_set(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct omap_mcpdm *mcpdm = dev_get_drvdata(dev);
	int ret;
	unsigned long value;

	ret = strict_strtol(buf, 10, &value);
	if (ret)
		return ret;

	if (value > DN_OFST_MAX)
		return -EINVAL;

	mcpdm->dl2_offset = value;
	return count;
}

static DEVICE_ATTR(dl1, 0644, mcpdm_dl1_show, mcpdm_dl1_set);
static DEVICE_ATTR(dl2, 0644, mcpdm_dl2_show, mcpdm_dl2_set);

static int omap_mcpdm_dai_startup(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct omap_mcpdm *mcpdm = snd_soc_dai_get_drvdata(dai);
	u32 ctrl;
	u32 val;
	int err = 0;

	dev_dbg(dai->dev, "%s: active %d\n", __func__, dai->active);

	mutex_lock(&mcpdm->mutex);

	/* nothing to do if already active */
	if (mcpdm->active++)
		goto out;

	if (dai->id >= MCPDM_ABE_DAI_DL1)
		mcpdm->abe_mode = 1;
	else
		mcpdm->abe_mode = 0;

	pm_runtime_get_sync(mcpdm->dev);

	val = __raw_readl(OMAP4430_CM1_ABE_PDM_CLKCTRL);
	if ((val & CLKCTRL_MODULEMODE_MASK) != CLKCTRL_MODULEMODE_ENABLED) {
		WARN(1, "Clock not enabled: PDM_CLKCTRL=0x%x\n", val);
		mcpdm->active--;
		pm_runtime_put_sync(mcpdm->dev);
		err = -ENODEV;
		goto out;
	}

	/* Enable McPDM watch dog for ES above ES 1.0 to avoid saturation */
	if (omap_rev() != OMAP4430_REV_ES1_0) {
		ctrl = omap_mcpdm_read(mcpdm, MCPDM_CTRL);
		omap_mcpdm_write(mcpdm, MCPDM_CTRL, ctrl | WD_EN);
	}

	omap_mcpdm_set_offset(mcpdm);
	omap_mcpdm_open(mcpdm);
	schedule_delayed_work(&mcpdm->esd_work, msecs_to_jiffies(250));
out:
	mutex_unlock(&mcpdm->mutex);
	return err;
}

static void omap_mcpdm_dai_shutdown(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	struct omap_mcpdm *mcpdm = snd_soc_dai_get_drvdata(dai);

	dev_dbg(dai->dev, "%s: active %d\n", __func__, dai->active);

	mutex_lock(&mcpdm->mutex);

	if (--mcpdm->active)
		goto out;

	if (mcpdm->abe_mode) {
		if (omap_mcpdm_active(mcpdm)) {
			omap_abe_port_disable(mcpdm->abe, mcpdm->dl_port);
			omap_abe_port_disable(mcpdm->abe, mcpdm->ul_port);
			udelay(250);
			abe_remove_opp_req(mcpdm->dev);
			omap_mcpdm_stop(mcpdm);
		}
	} else {
		omap_mcpdm_stop(mcpdm);
	}

	cancel_delayed_work_sync(&mcpdm->esd_work);
	omap_mcpdm_close(mcpdm);

	pm_runtime_put_sync(mcpdm->dev);

out:
	mutex_unlock(&mcpdm->mutex);
}

static int omap_mcpdm_dai_hw_params(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params,
				    struct snd_soc_dai *dai)
{
	struct omap_mcpdm *mcpdm = snd_soc_dai_get_drvdata(dai);
	int stream = substream->stream;
	int channels, link_mask = 0;

	snd_soc_dai_set_dma_data(dai, substream,
				 &omap_mcpdm_dai_dma_params[stream]);

	/* ABE DAIs have fixed channels */
	if (mcpdm->abe_mode) {
		mcpdm->dn_channels = PDM_DN_MASK | PDM_CMD_MASK;
		mcpdm->up_channels = PDM_UP1_EN | PDM_UP2_EN;
		return 0;
	}

	channels = params_channels(params);
	switch (channels) {
	case 4:
		if (stream == SNDRV_PCM_STREAM_CAPTURE)
			/* up to 2 channels for capture */
			return -EINVAL;
		link_mask |= 1 << 3;
	case 3:
		if (stream == SNDRV_PCM_STREAM_CAPTURE)
			/* up to 2 channels for capture */
			return -EINVAL;
		link_mask |= 1 << 2;
	case 2:
		link_mask |= 1 << 1;
	case 1:
		link_mask |= 1 << 0;
		break;
	default:
		/* unsupported number of channels */
		return -EINVAL;
	}

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		/* Downlink channels */
		mcpdm->dn_channels = (link_mask << 3) & (PDM_DN_MASK | PDM_CMD_MASK);
	else
		/* Uplink channels */
		mcpdm->up_channels = link_mask & (PDM_UP_MASK | PDM_STATUS_MASK);

	return 0;
}

static int omap_mcpdm_prepare(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct omap_mcpdm *mcpdm = snd_soc_dai_get_drvdata(dai);

	mutex_lock(&mcpdm->mutex);

	if (omap_mcpdm_active(mcpdm))
		goto out;

	if (mcpdm->abe_mode) {
		/* Check if ABE McPDM is already started */
		if (omap_abe_port_is_enabled(mcpdm->abe, mcpdm->ul_port) ||
		    omap_abe_port_is_enabled(mcpdm->abe, mcpdm->dl_port))
			goto out;

		/* PDM tasks require ABE OPP 50 */
		abe_add_opp_req(mcpdm->dev, ABE_OPP_50);

		/* start ATC before McPDM IP */
		omap_abe_port_enable(mcpdm->abe, mcpdm->dl_port);
		omap_abe_port_enable(mcpdm->abe, mcpdm->ul_port);

		/* wait 250us for ABE tick */
		udelay(250);
	}

	omap_mcpdm_start(mcpdm);

out:
	mutex_unlock(&mcpdm->mutex);
	return 0;
}

static int omap_mcpdm_dai_trigger(struct snd_pcm_substream *substream,
				  int cmd, struct snd_soc_dai *dai)
{
	struct omap_mcpdm *mcpdm = snd_soc_dai_get_drvdata(dai);

	dev_dbg(dai->dev, "cmd %d\n", cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		break;
	default:
		break;
	}
	omap_mcpdm_reg_dump(mcpdm);
	return 0;
}

static void mcpdm_esd_work(struct work_struct *work)
{
	struct omap_mcpdm *mcpdm = container_of(work, struct omap_mcpdm,
						esd_work.work);

	if (omap_mcpdm_read(mcpdm, MCPDM_STATUS)) {
		if (mcpdm->abe_mode) {
			omap_abe_port_disable(mcpdm->abe, mcpdm->dl_port);
			omap_abe_port_disable(mcpdm->abe, mcpdm->ul_port);
			udelay(250);
		}
		omap_mcpdm_stop(mcpdm);

		if (mcpdm->abe_mode) {
			omap_abe_port_enable(mcpdm->abe, mcpdm->dl_port);
			omap_abe_port_enable(mcpdm->abe, mcpdm->ul_port);
			udelay(250);
		}
		omap_mcpdm_start(mcpdm);
	}
	schedule_delayed_work(&mcpdm->esd_work, msecs_to_jiffies(250));
}

static struct snd_soc_dai_ops omap_mcpdm_dai_ops = {
	.startup	= omap_mcpdm_dai_startup,
	.shutdown	= omap_mcpdm_dai_shutdown,
	.hw_params	= omap_mcpdm_dai_hw_params,
	.prepare	= omap_mcpdm_prepare,
	.trigger	= omap_mcpdm_dai_trigger,
};

#ifdef CONFIG_PM
static int omap_mcpdm_suspend(struct snd_soc_dai *dai)
{
	struct omap_mcpdm *mcpdm = snd_soc_dai_get_drvdata(dai);

	/* save context only if we are streaming */
	if (!mcpdm->active)
		return 0;

	omap_mcpdm_write_cache(mcpdm, MCPDM_DN_OFFSET,
		omap_mcpdm_read(mcpdm, MCPDM_DN_OFFSET));
	omap_mcpdm_write_cache(mcpdm, MCPDM_IRQENABLE_SET,
		omap_mcpdm_read(mcpdm, MCPDM_IRQENABLE_SET));
	omap_mcpdm_write_cache(mcpdm, MCPDM_DMAENABLE_SET,
		omap_mcpdm_read(mcpdm, MCPDM_DMAENABLE_SET));
	omap_mcpdm_write_cache(mcpdm, MCPDM_FIFO_CTRL_DN,
		omap_mcpdm_read(mcpdm, MCPDM_FIFO_CTRL_DN));
	omap_mcpdm_write_cache(mcpdm, MCPDM_FIFO_CTRL_UP,
		omap_mcpdm_read(mcpdm, MCPDM_FIFO_CTRL_UP));
	omap_mcpdm_write_cache(mcpdm, MCPDM_CTRL,
		omap_mcpdm_read(mcpdm, MCPDM_CTRL));

	pm_runtime_put_sync(mcpdm->dev);

	return 0;
}

static int omap_mcpdm_resume(struct snd_soc_dai *dai)
{
	struct omap_mcpdm *mcpdm = snd_soc_dai_get_drvdata(dai);
	struct omap_mcpdm_platform_data *pdata = mcpdm->pdata;

	/* restore context only if we were streaming */
	if (!mcpdm->active)
		return 0;

	if (!pdata->was_context_lost(mcpdm->dev))
		return 0;

	pm_runtime_get_sync(mcpdm->dev);

	/* restore from reg cache */
	omap_mcpdm_write(mcpdm, MCPDM_DN_OFFSET,
		omap_mcpdm_read_cache(mcpdm, MCPDM_DN_OFFSET));
	omap_mcpdm_write(mcpdm, MCPDM_IRQENABLE_SET,
		omap_mcpdm_read_cache(mcpdm, MCPDM_IRQENABLE_SET));
	omap_mcpdm_write(mcpdm, MCPDM_DMAENABLE_SET,
		omap_mcpdm_read_cache(mcpdm, MCPDM_DMAENABLE_SET));
	omap_mcpdm_write(mcpdm, MCPDM_FIFO_CTRL_DN,
		omap_mcpdm_read_cache(mcpdm, MCPDM_FIFO_CTRL_DN));
	omap_mcpdm_write(mcpdm, MCPDM_FIFO_CTRL_UP,
		omap_mcpdm_read_cache(mcpdm, MCPDM_FIFO_CTRL_UP));
	omap_mcpdm_write(mcpdm, MCPDM_CTRL,
		omap_mcpdm_read_cache(mcpdm, MCPDM_CTRL));

	return 0;
}
#else
#define omap_mcpdm_suspend	NULL
#define omap_mcpdm_resume	NULL
#endif

static int omap_mcpdm_probe(struct snd_soc_dai *dai)
{
	struct omap_mcpdm *mcpdm = snd_soc_dai_get_drvdata(dai);
	int ret;

	pm_runtime_enable(mcpdm->dev);

	/* Disable lines while request is ongoing */
	pm_runtime_get_sync(mcpdm->dev);
	omap_mcpdm_write(mcpdm, MCPDM_CTRL, 0x00);

	ret = request_irq(mcpdm->irq, omap_mcpdm_irq_handler,
				0, "McPDM", (void *)mcpdm);
	if (ret)
		dev_err(mcpdm->dev, "Request for McPDM IRQ failed\n");

	pm_runtime_put_sync(mcpdm->dev);
	return ret;
}

static int omap_mcpdm_remove(struct snd_soc_dai *dai)
{
	struct omap_mcpdm *mcpdm = snd_soc_dai_get_drvdata(dai);

	free_irq(mcpdm->irq, (void *)mcpdm);
	pm_runtime_disable(mcpdm->dev);

	return 0;
}

#define OMAP_MCPDM_RATES	(SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)
#define OMAP_MCPDM_FORMATS	SNDRV_PCM_FMTBIT_S32_LE

static struct snd_soc_dai_driver omap_mcpdm_dai[] = {
{
	.name = "mcpdm-dl",
	.id	= MCPDM_LEGACY_DAI_DL1,
	.probe = omap_mcpdm_probe,
	.remove = omap_mcpdm_remove,
	.probe_order = SND_SOC_COMP_ORDER_LATE,
	.remove_order = SND_SOC_COMP_ORDER_EARLY,
	.suspend = omap_mcpdm_suspend,
	.resume = omap_mcpdm_resume,
	.playback = {
		.channels_min = 1,
		.channels_max = 4,
		.rates = OMAP_MCPDM_RATES,
		.formats = OMAP_MCPDM_FORMATS,
	},
	.ops = &omap_mcpdm_dai_ops,
},
{
	.name = "mcpdm-ul",
	.id	= MCPDM_LEGACY_DAI_UL1,
	.probe_order = SND_SOC_COMP_ORDER_LATE,
	.remove_order = SND_SOC_COMP_ORDER_EARLY,
	.suspend = omap_mcpdm_suspend,
	.resume = omap_mcpdm_resume,
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = OMAP_MCPDM_RATES,
		.formats = OMAP_MCPDM_FORMATS,
	},
	.ops = &omap_mcpdm_dai_ops,
},
#if defined(CONFIG_SND_OMAP_SOC_ABE_DSP) ||\
	defined(CONFIG_SND_OMAP_SOC_ABE_DSP_MODULE)
{
	.name = "mcpdm-dl1",
	.id	= MCPDM_ABE_DAI_DL1,
	.probe_order = SND_SOC_COMP_ORDER_LATE,
	.remove_order = SND_SOC_COMP_ORDER_EARLY,
	.suspend = omap_mcpdm_suspend,
	.resume = omap_mcpdm_resume,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = OMAP_MCPDM_RATES,
		.formats = OMAP_MCPDM_FORMATS,
	},
	.ops = &omap_mcpdm_dai_ops,
},
{
	.name = "mcpdm-dl2",
	.id	= MCPDM_ABE_DAI_DL2,
	.probe_order = SND_SOC_COMP_ORDER_LATE,
	.remove_order = SND_SOC_COMP_ORDER_EARLY,
	.suspend = omap_mcpdm_suspend,
	.resume = omap_mcpdm_resume,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = OMAP_MCPDM_RATES,
		.formats = OMAP_MCPDM_FORMATS,
	},
	.ops = &omap_mcpdm_dai_ops,
},
{
	.name = "mcpdm-vib",
	.id	= MCPDM_ABE_DAI_VIB,
	.probe_order = SND_SOC_COMP_ORDER_LATE,
	.remove_order = SND_SOC_COMP_ORDER_EARLY,
	.suspend = omap_mcpdm_suspend,
	.resume = omap_mcpdm_resume,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = OMAP_MCPDM_RATES,
		.formats = OMAP_MCPDM_FORMATS,
	},
	.ops = &omap_mcpdm_dai_ops,
},
{
	.name = "mcpdm-ul1",
	.id	= MCPDM_ABE_DAI_UL1,
	.probe_order = SND_SOC_COMP_ORDER_LATE,
	.remove_order = SND_SOC_COMP_ORDER_EARLY,
	.suspend = omap_mcpdm_suspend,
	.resume = omap_mcpdm_resume,
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = OMAP_MCPDM_RATES,
		.formats = OMAP_MCPDM_FORMATS,
	},
	.ops = &omap_mcpdm_dai_ops,
},
#endif
 };

static __devinit int asoc_mcpdm_probe(struct platform_device *pdev)
{
	struct omap_mcpdm_platform_data *pdata = pdev->dev.platform_data;
	struct omap_mcpdm *mcpdm;
	struct resource *res;
	int ret = 0, err;

	mcpdm = kzalloc(sizeof(struct omap_mcpdm), GFP_KERNEL);
	if (!mcpdm)
		return -ENOMEM;

	platform_set_drvdata(pdev, mcpdm);

	mutex_init(&mcpdm->mutex);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "no resource\n");
		goto err_res;
	}

	mcpdm->io_base = ioremap(res->start, resource_size(res));
	if (!mcpdm->io_base) {
		ret = -ENOMEM;
		goto err_iomap;
	}

	mcpdm->reg_cache = kzalloc(resource_size(res), GFP_KERNEL);
	if (!mcpdm->reg_cache) {
		ret = -ENOMEM;
		goto err_cache;
	}

	mcpdm->irq = platform_get_irq(pdev, 0);
	if (mcpdm->irq < 0) {
		ret = mcpdm->irq;
		goto err_irq;
	}

	mcpdm->dev = &pdev->dev;
	mcpdm->pdata = pdata;

	/* DL1 and DL2 DC offset values will be different for each device */
	mcpdm->dl1_offset = DN_OFST_MAX >> 1;
	mcpdm->dl2_offset = DN_OFST_MAX >> 1;
	err = device_create_file(mcpdm->dev, &dev_attr_dl1);
	if (err < 0)
		dev_err(mcpdm->dev,"failed to DL1 DC offset sysfs: %d\n", err);
	err = device_create_file(mcpdm->dev, &dev_attr_dl2);
	if (err < 0)
		dev_err(mcpdm->dev,"failed to DL2 DC offset sysfs: %d\n", err);

#if defined(CONFIG_SND_OMAP_SOC_ABE_DSP) ||\
	defined(CONFIG_SND_OMAP_SOC_ABE_DSP_MODULE)

	mcpdm->abe = omap_abe_port_mgr_get();
	if (!mcpdm->abe)
		goto err_irq;

	mcpdm->ul_port = omap_abe_port_open(mcpdm->abe, OMAP_ABE_BE_PORT_PDM_UL1);
	if (!mcpdm->ul_port)
		goto err_ul;

	mcpdm->dl_port = omap_abe_port_open(mcpdm->abe, OMAP_ABE_BE_PORT_PDM_DL1);
	if (!mcpdm->dl_port)
		goto err_dl;
#endif

	INIT_DELAYED_WORK(&mcpdm->esd_work, mcpdm_esd_work);

	ret = snd_soc_register_dais(&pdev->dev, omap_mcpdm_dai,
			ARRAY_SIZE(omap_mcpdm_dai));
	if (ret < 0)
		goto err_dai;

	return 0;

err_dai:
#if defined(CONFIG_SND_OMAP_SOC_ABE_DSP) ||\
	defined(CONFIG_SND_OMAP_SOC_ABE_DSP_MODULE)
	omap_abe_port_close(mcpdm->abe, mcpdm->dl_port);
err_dl:
	omap_abe_port_close(mcpdm->abe, mcpdm->ul_port);
err_ul:
	omap_abe_port_mgr_put(mcpdm->abe);
#endif
err_irq:
	kfree(mcpdm->reg_cache);
err_cache:
	iounmap(mcpdm->io_base);
err_iomap:
	release_mem_region(res->start, resource_size(res));
err_res:
	kfree(mcpdm);
	return ret;
}

static int __devexit asoc_mcpdm_remove(struct platform_device *pdev)
{
	struct omap_mcpdm *mcpdm = platform_get_drvdata(pdev);
	struct resource *res;

	snd_soc_unregister_dais(&pdev->dev, ARRAY_SIZE(omap_mcpdm_dai));

	device_remove_file(&pdev->dev, &dev_attr_dl1);
	device_remove_file(&pdev->dev, &dev_attr_dl2);

#if defined(CONFIG_SND_OMAP_SOC_ABE_DSP) ||\
	defined(CONFIG_SND_OMAP_SOC_ABE_DSP_MODULE)
	omap_abe_port_close(mcpdm->abe, mcpdm->dl_port);
	omap_abe_port_close(mcpdm->abe, mcpdm->ul_port);
	omap_abe_port_mgr_put(mcpdm->abe);
#endif

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	iounmap(mcpdm->io_base);
	kfree(mcpdm->reg_cache);
	kfree(mcpdm);
	return 0;
}

static struct platform_driver asoc_mcpdm_driver = {
	.driver = {
			.name = "omap-mcpdm",
			.owner = THIS_MODULE,
	},

	.probe = asoc_mcpdm_probe,
	.remove = __devexit_p(asoc_mcpdm_remove),
};

static int __init snd_omap_mcpdm_init(void)
{
	return platform_driver_register(&asoc_mcpdm_driver);
}
module_init(snd_omap_mcpdm_init);

static void __exit snd_omap_mcpdm_exit(void)
{
	platform_driver_unregister(&asoc_mcpdm_driver);
}
module_exit(snd_omap_mcpdm_exit);

MODULE_AUTHOR("Misael Lopez Cruz <x0052729@ti.com>");
MODULE_DESCRIPTION("OMAP PDM SoC Interface");
MODULE_LICENSE("GPL");
