// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 */

#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/leds-mtk.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/slab.h>
#include <linux/string.h>

/****************************************************************************
 * variables
 ***************************************************************************/
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME " %s(%d) :" fmt, __func__, __LINE__

struct mtk_leds_info;

static int led_level_set(struct led_classdev *led_cdev,
					 enum led_brightness brightness);

struct led_debug_info {
	unsigned long long current_t;
	unsigned long long last_t;
	char buffer[4096];
	int count;
};

struct led_desp {
	int index;
	char name[16];
};

struct leds_desp_info {
	int lens;
	struct led_desp *leds[0];
};

struct mtk_led_data {
	struct led_desp desp;
	struct led_conf_info	conf;
	int last_brightness;
	int hw_brightness;
	int last_hw_brightness;
	struct mtk_leds_info	*parent;
	struct led_debug_info debug;
};

struct mtk_leds_info {
	struct device *dev;
	int			nums;
	struct mtk_led_data leds[0];
};
extern int mtkfb_set_backlight_level(unsigned int level);

static DEFINE_MUTEX(leds_mutex);
struct leds_desp_info *leds_info;
static BLOCKING_NOTIFIER_HEAD(mtk_leds_chain_head);

int mtk_leds_register_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&mtk_leds_chain_head, nb);
}
EXPORT_SYMBOL_GPL(mtk_leds_register_notifier);

int mtk_leds_unregister_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&mtk_leds_chain_head, nb);
}
EXPORT_SYMBOL_GPL(mtk_leds_unregister_notifier);

int mtk_leds_call_notifier(unsigned long action, void *data)
{
	return blocking_notifier_call_chain(&mtk_leds_chain_head, action, data);
}
EXPORT_SYMBOL_GPL(mtk_leds_call_notifier);

static int  __maybe_unused call_notifier(int event, struct mtk_led_data *led_dat)
{
	int err;

	err = mtk_leds_call_notifier(event, &led_dat->conf);
	if (err)
		pr_info("notifier_call_chain error\n");
	return err;
}

/****************************************************************************
 * DEBUG MACROS
 ***************************************************************************/

static void led_debug_log(struct mtk_led_data *s_led,
		int level, int mappingLevel)
{
	unsigned long cur_time_mod = 0;
	unsigned long long cur_time_display = 0;
	int ret = 0;

	s_led->debug.current_t = sched_clock();
	cur_time_display = s_led->debug.current_t;
	do_div(cur_time_display, 1000000);
	cur_time_mod = do_div(cur_time_display, 1000);

	ret = snprintf(s_led->debug.buffer + strlen(s_led->debug.buffer),
		4095 - strlen(s_led->debug.buffer),
		"T:%lld.%ld,L:%d L:%d map:%d    ",
		cur_time_display, cur_time_mod,
		s_led->conf.cdev.brightness, level, mappingLevel);

	s_led->debug.count++;

	if (ret < 0 || ret >= 4096) {
		pr_info("print log error!");
		s_led->debug.count = 5;
	}

	if (level == 0 || s_led->debug.count >= 5 ||
		(s_led->debug.current_t - s_led->debug.last_t) > 1000000000) {
		pr_info("%s", s_led->debug.buffer);
		s_led->debug.count = 0;
		s_led->debug.buffer[strlen("[Light] Set directly ") +
			strlen(s_led->conf.cdev.name)] = '\0';
	}

	s_led->debug.last_t = sched_clock();
}

static int getLedDespIndex(char *name)
{
	int i = 0;

	while (i < leds_info->lens) {
		if (!strcmp(name, leds_info->leds[i]->name))
			return i;
		i++;
	}
	return -1;
}

/****************************************************************************
 * driver functions
 ***************************************************************************/

static int brightness_maptolevel(struct led_conf_info *led_dat, int brightness)
{
	return (((led_dat->max_hw_brightness) * brightness
				+ ((led_dat->cdev.max_brightness) / 2))
				/ (led_dat->cdev.max_brightness));
}

static int led_level_disp_set(struct mtk_led_data *led_dat,
				int brightness)
{

	brightness = min(brightness, led_dat->conf.limit_hw_brightness);
	if (brightness == led_dat->hw_brightness)
		return 0;

	mtkfb_set_backlight_level(brightness);
	led_dat->hw_brightness = brightness;

	pr_debug("set brightness by disp: %d", brightness);

	return 0;
}

int mt_leds_brightness_set(char *name, int level)
{
	struct mtk_led_data *led_dat;
	int index;
	int hw_brightness = 0;

	index = getLedDespIndex(name);
	if (index < 0) {
		pr_notice("can not find leds by led_desp %s", name);
		return -1;
	}
	led_dat = container_of(leds_info->leds[index],
		struct mtk_led_data, desp);

	led_dat->conf.aal_enable = 1;

	hw_brightness = brightness_maptolevel(&led_dat->conf, led_dat->last_brightness);

	if (level > hw_brightness){
		pr_notice("set brightness error %d, %d", level, hw_brightness);
		return -1;
	}

	led_level_disp_set(led_dat, level);
	led_dat->last_hw_brightness = level;

	return 0;
}
EXPORT_SYMBOL(mt_leds_brightness_set);

static int led_level_set(struct led_classdev *led_cdev,
					  enum led_brightness brightness)
{
	int trans_level = 0;

	struct led_conf_info *led_conf =
		container_of(led_cdev, struct led_conf_info, cdev);
	struct mtk_led_data *led_dat =
		container_of(led_conf, struct mtk_led_data, conf);

	if (led_dat->last_brightness == brightness)
		return 0;

	led_dat->last_brightness = brightness;

	trans_level = brightness_maptolevel(led_conf, brightness);

	led_debug_log(led_dat, brightness, trans_level);

	call_notifier(LED_BRIGHTNESS_CHANGED, led_dat);
	if (!led_conf->aal_enable) {
		led_level_disp_set(led_dat, trans_level);
		led_dat->last_hw_brightness = trans_level;
	}

	return 0;

}


/****************************************************************************
 * add API for temperature control
 ***************************************************************************/

int setMaxBrightness(char *name, int percent, bool enable)
{
	struct mtk_led_data *led_dat;
	int max_l = 0, index = -1, limit_l = 0, cur_l = 0;

	index = getLedDespIndex(name);
	if (index < 0) {
		pr_notice("can not find leds by led_desp %s", name);
		return -1;
	}
	led_dat = container_of(leds_info->leds[index],
		struct mtk_led_data, desp);

	max_l = led_dat->conf.max_hw_brightness;
	limit_l = (percent * max_l) / 100;
	pr_info("before: name: %s, percent : %d, limit_l : %d, enable: %d",
		leds_info->leds[index]->name, percent, limit_l, enable);
	if (enable) {
		led_dat->conf.limit_hw_brightness = limit_l;
		cur_l = min(led_dat->last_hw_brightness, limit_l);
	} else if (!enable) {
		led_dat->conf.limit_hw_brightness = max_l;
		cur_l = led_dat->last_hw_brightness;
	}

	if (led_dat->conf.cdev.brightness != 0)
		led_level_disp_set(led_dat, cur_l);

	pr_info("after: name: %s, cur_l : %d, max_brightness : %d",
		led_dat->conf.cdev.name, cur_l, led_dat->conf.limit_hw_brightness);
	return 0;

}
EXPORT_SYMBOL(setMaxBrightness);

static int led_data_init(struct device *dev, struct mtk_led_data *s_led)
{
	int ret;

	s_led->conf.cdev.flags = LED_CORE_SUSPENDRESUME;
	s_led->conf.cdev.brightness_set_blocking = led_level_set;
	s_led->last_brightness = s_led->conf.cdev.max_brightness;
	s_led->hw_brightness = s_led->conf.max_hw_brightness;
	s_led->last_hw_brightness = s_led->conf.max_hw_brightness;
	ret = devm_led_classdev_register(dev, &(s_led->conf.cdev));
	if (ret < 0) {
		pr_notice("led class register fail!");
		return ret;
	}
	pr_info("%s devm_led_classdev_register ok! ", s_led->conf.cdev.name);

	ret = snprintf(s_led->debug.buffer + strlen(s_led->debug.buffer),
		4095 - strlen(s_led->debug.buffer),
		"[Light] Set %s directly ", s_led->conf.cdev.name);

	if (ret < 0 || ret >= 4096)
		pr_info("print log init error!");

	return 0;
}

static int mtk_leds_parse_dt(struct device *dev,
		struct mtk_leds_info *m_leds)
{
	struct device_node *child;
	struct mtk_led_data *s_led;
	int ret = 0, num = 0, level = 0;
	const char *state;

	if (!dev->of_node) {
		pr_notice("Error load dts: node not exist!\n");
		return ret;
	}

	for_each_available_child_of_node(dev->of_node, child) {

		s_led = &(m_leds->leds[num]);
		ret = of_property_read_string(child, "label",
			&(s_led->conf.cdev.name));
		if (ret) {
			pr_info("Fail to read label property");
			goto out_led_dt;
		}
		ret = of_property_read_string(child, "default-trigger",
			&(s_led->conf.cdev.default_trigger ));
		if (ret) {
			pr_info("Fail to read default-trigger property");
			s_led->conf.cdev.default_trigger = NULL;
		}
		ret = of_property_read_u32(child,
			"max-brightness", &(s_led->conf.cdev.max_brightness));
		if (ret) {
			pr_info("No max-brightness, use default value 255");
 			s_led->conf.cdev.max_brightness = 255;
		}
		ret = of_property_read_u32(child,
			"max-hw-brightness", &(s_led->conf.max_hw_brightness));
		if (ret) {
			pr_info("No max-hw-brightness, use default value 1023");
			s_led->conf.max_hw_brightness = 1023;
		}
		s_led->conf.limit_hw_brightness = s_led->conf.max_hw_brightness;
		ret = of_property_read_string(child, "default-state", &state);
		if (!ret) {
			if (!strcmp(state, "half"))
				level = s_led->conf.cdev.max_brightness / 2;
			else if (!strcmp(state, "on"))
				level = s_led->conf.cdev.max_brightness;
			else
				level = 0;
		} else {
			level = s_led->conf.cdev.max_brightness;
		}
		s_led->conf.aal_enable = 0;
		pr_info("parse %s(%d) leds dt: %d, %d, %d\n",
			s_led->conf.cdev.name, num,
			s_led->conf.cdev.max_brightness,
			s_led->conf.max_hw_brightness,
			s_led->conf.aal_enable);
		s_led->desp.index = num;
		strncpy(s_led->desp.name, s_led->conf.cdev.name,
			strlen(s_led->conf.cdev.name));
		s_led->desp.index = num;
		leds_info->leds[num] = &s_led->desp;
		s_led->conf.cdev.brightness = level;
		ret = led_data_init(dev, s_led);
		if (ret)
			goto out_led_dt;
		led_level_set(&s_led->conf.cdev, level);
		num++;
	}
	m_leds->nums = num;
	pr_info("load dts ok!");
	return ret;
out_led_dt:
	pr_notice("Error load dts node!\n");
	of_node_put(child);
	return ret;
}


/****************************************************************************
 * driver functions
 ***************************************************************************/

static int mtk_leds_probe(struct platform_device *pdev)
{

	struct device *dev = &pdev->dev;
	struct mtk_leds_info *m_leds;
	int ret, nums;

	pr_info("probe begain +++");

	nums = of_get_child_count(dev->of_node);
	pr_info("Load dts node nums: %d", nums);
	m_leds = devm_kzalloc(dev, (sizeof(struct mtk_leds_info) +
		(sizeof(struct mtk_led_data) * (nums))), GFP_KERNEL);
	if (!m_leds)
		goto err;

	leds_info = devm_kzalloc(dev, (sizeof(struct leds_desp_info) +
		sizeof(struct led_desp *) * (nums)),
		GFP_KERNEL);
	leds_info->lens = nums;
	if (!leds_info) {
		ret = -ENOMEM;
		goto err;
	}

	platform_set_drvdata(pdev, m_leds);
	m_leds->dev = dev;
	ret = mtk_leds_parse_dt(&(pdev->dev), m_leds);
	if (ret) {
		pr_notice("Failed to parse devicetree!\n");
		goto err;
	}

	pr_info("probe end ---");
	return 0;
 err:
	pr_notice("Failed to probe!\n");
	ret = -ENOMEM;
	return ret;
}

static int mtk_leds_remove(struct platform_device *pdev)
{
	int i;
	struct mtk_leds_info *m_leds = dev_get_platdata(&pdev->dev);

	if (!m_leds)
		return 0;
	for (i = 0; i < m_leds->nums; i++) {
		if (!m_leds->leds[i].parent)
			continue;
		led_classdev_unregister(&m_leds->leds[i].conf.cdev);
		m_leds->leds[i].parent = NULL;
	}
	kfree(m_leds);
	m_leds = NULL;

	return 0;
}

static void mtk_leds_shutdown(struct platform_device *pdev)
{
	int i;
	struct mtk_leds_info *m_leds = dev_get_platdata(&pdev->dev);

	pr_info("Turn off backlight\n");

	for (i = 0; m_leds && i < m_leds->nums; i++) {
		if (!&(m_leds->leds[i]))
			continue;

		led_level_disp_set(&(m_leds->leds[i]), 0);
		call_notifier(LED_STATUS_SHUTDOWN, &(m_leds->leds[i]));
	}

}

static const struct of_device_id of_mtk_disp_leds_match[] = {
	{ .compatible = "mediatek,disp-leds", },
	{},
};
MODULE_DEVICE_TABLE(of, of_mtk_disp_leds_match);

static struct platform_driver mtk_disp_leds_driver = {
	.driver = {
		   .name = "mtk-disp-leds",
		   .owner = THIS_MODULE,
		   .of_match_table = of_mtk_disp_leds_match,
		   },
	.probe = mtk_leds_probe,
	.remove = mtk_leds_remove,
	.shutdown = mtk_leds_shutdown,

};

static int __init mtk_leds_init(void)
{
	int ret;

	pr_info("Leds init\n");
	ret = platform_driver_register(&mtk_disp_leds_driver);

	if (ret) {
		pr_info("driver register error: %d\n", ret);
		return ret;
	}

	return ret;
}

static void __exit mtk_leds_exit(void)
{
	platform_driver_unregister(&mtk_disp_leds_driver);
}

/* delay leds init, for (1)display has delayed to use clock upstream.
 * (2)to fix repeat switch battary and power supply caused BL KE issue,
 * battary calling bl .shutdown whitch need to call display
 * function and they not yet probe.
 */
late_initcall(mtk_leds_init);
module_exit(mtk_leds_exit);

MODULE_AUTHOR("Mediatek Corporation");
MODULE_DESCRIPTION("MTK Disp Backlight Driver");
MODULE_LICENSE("GPL");


