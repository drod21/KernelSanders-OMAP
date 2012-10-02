/* Copyright (c) 2012, Will Tisdale <willtisdale@gmail.com>. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/*
 * Simple earlysuspend hook to disable OMAP44xx secondary core
 * on early_suspend and enable it on late_resume.
 * Although TI doesn't recommend hotplugging automatically as it
 * is expensive, hotplugging occasionally under these limited
 * circumstances will be ultimately beneficial to power saving
 * as the CPUs will not need to agree to enter OSWR which means
 * it is used more frequently by the remaining online CPU which
 * saves power.
 */

#include <linux/kernel.h>
#include <linux/cpu.h>
#include <linux/earlysuspend.h>
#include <linux/workqueue.h>

struct delayed_work earlysuspend44xx_suspend_work;
struct delayed_work earlysuspend44xx_resume_work;

static void earlysuspend44xx_suspend(struct work_struct *work)
{
	if (likely(cpu_online(1)))
		cpu_down(1);
}

static void earlysuspend44xx_resume(struct work_struct *work)
{
	if (likely(!cpu_online(1)))
		cpu_up(1);
}

static void earlysuspend44xx_early_suspend(struct early_suspend *handler)
{
	schedule_delayed_work_on(0, &earlysuspend44xx_suspend_work, HZ);
}

static void earlysuspend44xx_late_resume(struct early_suspend *handler)
{
	schedule_delayed_work_on(0, &earlysuspend44xx_resume_work, HZ);
}

static struct early_suspend earlysuspend44xx_handler = {
	.suspend = earlysuspend44xx_early_suspend,
	.resume = earlysuspend44xx_late_resume,
};

static int __init earlysuspend44xx_init(void)
{
	INIT_DELAYED_WORK_DEFERRABLE(&earlysuspend44xx_suspend_work, earlysuspend44xx_suspend);
	INIT_DELAYED_WORK_DEFERRABLE(&earlysuspend44xx_resume_work, earlysuspend44xx_resume);
	register_early_suspend(&earlysuspend44xx_handler);
	return 0;
}

late_initcall(earlysuspend44xx_init);
