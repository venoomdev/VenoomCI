/*
 * TEE driver for goodix fingerprint sensor
 * Copyright (C) 2016 Goodix
 * Copyright (C) 2020 XiaoMi, Inc.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt)		KBUILD_MODNAME ": " fmt
#include <linux/clk.h>
#include <linux/compat.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fb.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/irq.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/of_gpio.h>
#include <linux/mdss_io_util.h>
//#include <linux/wakelock.h>
#include <linux/proc_fs.h>
#include <linux/unistd.h>
#include <linux/delay.h>
#include <drm/drm_bridge.h>
#include <linux/msm_drm_notify.h>

#include <linux/platform_device.h>

#include <linux/pm_qos.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <net/netlink.h>
#include <net/sock.h>
#include "gf_spi.h"

#define VER_MAJOR   1
#define VER_MINOR   2
#define PATCH_LEVEL 8
#define FAIL -1

#define WAKELOCK_HOLD_TIME 400 /* in ms */
#define FP_UNLOCK_REJECTION_TIMEOUT (WAKELOCK_HOLD_TIME - 100)
#define GF_SPIDEV_NAME     "goodix,fingerprint"
/*device name after register in character*/
#define GF_DEV_NAME            "goodix_fp"
#define	GF_INPUT_NAME	    "uinput-goodix"/*"goodix_fp" */

#define	CHRD_DRIVER_NAME	"goodix_fp_spi"
#define	CLASS_NAME		    "goodix_fp"

#define PROC_NAME  "hwinfo"

#define N_SPI_MINORS		32	/* ... up to 256 */
static int SPIDEV_MAJOR;

static DECLARE_BITMAP(minors, N_SPI_MINORS);
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
//static struct wake_lock fp_wakelock;
static struct wakeup_source fp_ws;//for kernel 4.9
static struct gf_dev gf;
static int pid = -1;
struct sock *gf_nl_sk = NULL;

extern int fpsensor;
static struct proc_dir_entry *proc_entry;

static inline void sendnlmsg(char *msg)
{
	struct sk_buff *skb_1;
	struct nlmsghdr *nlh;
	int len = NLMSG_SPACE(MAX_MSGSIZE);
	int ret = 0;
	if (!msg || !gf_nl_sk || !pid) {
		return ;
	}
	skb_1 = alloc_skb(len, GFP_KERNEL);
	if (!skb_1) {
		return;
	}
	nlh = nlmsg_put(skb_1, 0, 0, 0, MAX_MSGSIZE, 0);
	NETLINK_CB(skb_1).portid = 0;
	NETLINK_CB(skb_1).dst_group = 0;
	memcpy(NLMSG_DATA(nlh), msg, sizeof(char));
	ret = netlink_unicast(gf_nl_sk, skb_1, pid, MSG_DONTWAIT);
}

static inline void sendnlmsg_tp(struct fp_underscreen_info *msg, int length)
{
	struct sk_buff *skb_1;
	struct nlmsghdr *nlh;
	int len = NLMSG_SPACE(MAX_MSGSIZE);
	int ret = 0;
	if (!msg || !gf_nl_sk || !pid) {
		return ;
	}
	skb_1 = alloc_skb(len, GFP_KERNEL);
	if (!skb_1) {
		return;
	}
	nlh = nlmsg_put(skb_1, 0, 0, 0, length, 0);
	NETLINK_CB(skb_1).portid = 0;
	NETLINK_CB(skb_1).dst_group = 0;
	memcpy(NLMSG_DATA(nlh), msg, length);//core
	ret = netlink_unicast(gf_nl_sk, skb_1, pid, MSG_DONTWAIT);
}

static inline void nl_data_ready(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	char str[100];
	skb = skb_get (__skb);
	if(skb->len >= NLMSG_SPACE(0))
	{
		nlh = nlmsg_hdr(skb);
		memcpy(str, NLMSG_DATA(nlh), sizeof(str));
		pid = nlh->nlmsg_pid;
		kfree_skb(skb);
	}
}

static inline int netlink_init(void)
{
	struct netlink_kernel_cfg netlink_cfg;
	memset(&netlink_cfg, 0, sizeof(struct netlink_kernel_cfg));
	netlink_cfg.groups = 0;
	netlink_cfg.flags = 0;
	netlink_cfg.input = nl_data_ready;
	netlink_cfg.cb_mutex = NULL;
	gf_nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST,
			&netlink_cfg);
	if(!gf_nl_sk){
		return 1;
	}
	return 0;
}
static inline void netlink_exit(void)
{
	if(gf_nl_sk != NULL){
		netlink_kernel_release(gf_nl_sk);
		gf_nl_sk = NULL;
	}
}

static inline int gf_pinctrl_init(struct gf_dev* gf_dev)
{
	int ret = 0;
	struct device *dev = &gf_dev->spi->dev;

	gf_dev->gf_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(gf_dev->gf_pinctrl)) {
		ret = PTR_ERR(gf_dev->gf_pinctrl);
		goto err;
	}
	gf_dev->gpio_state_enable =
		pinctrl_lookup_state(gf_dev->gf_pinctrl, "fp_en_init");
	if (IS_ERR_OR_NULL(gf_dev->gpio_state_enable)) {
		ret = PTR_ERR(gf_dev->gpio_state_enable);
		goto err;
	}
	gf_dev->gpio_state_disable =
		pinctrl_lookup_state(gf_dev->gf_pinctrl, "fp_dis_init");
	if (IS_ERR_OR_NULL(gf_dev->gpio_state_disable)) {
		ret = PTR_ERR(gf_dev->gpio_state_disable);
		goto err;
	}
	return 0;
err:
	gf_dev->gf_pinctrl = NULL;
	gf_dev->gpio_state_enable = NULL;
	gf_dev->gpio_state_disable = NULL;
	return ret;
}

static inline int gf_parse_dts(struct gf_dev* gf_dev)
{
	int rc = 0;
	struct device *dev = &gf_dev->spi->dev;
	struct device_node *np = dev->of_node;

	gf_dev->reset_gpio = of_get_named_gpio(np, "fp-gpio-reset", 0);
	if (gf_dev->reset_gpio < 0) {
		return gf_dev->reset_gpio;
	}

	rc = devm_gpio_request(dev, gf_dev->reset_gpio, "goodix_reset");
	if (rc) {
		goto err_reset;
	}
	gpio_direction_output(gf_dev->reset_gpio, 0);
	
	gf_dev->irq_gpio = of_get_named_gpio(np, "fp-gpio-irq", 0);
	if (gf_dev->irq_gpio < 0) {
		return gf_dev->irq_gpio;
	}

	rc = devm_gpio_request(dev, gf_dev->irq_gpio, "goodix_irq");
	if (rc) {
		goto err_irq;
	}
	gpio_direction_input(gf_dev->irq_gpio);

	return rc;
err_irq:
	devm_gpio_free(dev, gf_dev->irq_gpio);
err_reset:
	devm_gpio_free(dev, gf_dev->reset_gpio);
	return rc;
}

static inline int gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms)
{
	if(gf_dev == NULL) {
		return -1;
	}
	gpio_direction_output(gf_dev->reset_gpio, 1);
	gpio_set_value(gf_dev->reset_gpio, 0);
	mdelay(3);
	gpio_set_value(gf_dev->reset_gpio, 1);
	mdelay(delay_ms);

	return 0;
}

static inline void gf_enable_irq(struct gf_dev *gf_dev)
{
	if (!(gf_dev->irq_enabled)) {
		enable_irq(gf_dev->irq);
		gf_dev->irq_enabled = 1;
	}

}

static inline void gf_disable_irq(struct gf_dev *gf_dev)
{
	if (gf_dev->irq_enabled) {
		gf_dev->irq_enabled = 0;
		disable_irq(gf_dev->irq);
	}
}

static inline irqreturn_t gf_irq(int irq, void *handle)
{
	char msg[2] =  { 0x0 };
	struct gf_dev *gf_dev = &gf;
	//wake_lock_timeout(&fp_wakelock, msecs_to_jiffies(WAKELOCK_HOLD_TIME));
	__pm_wakeup_event(&fp_ws, WAKELOCK_HOLD_TIME);//for kernel 4.9
	msg[0] = GF_NET_EVENT_IRQ;
	sendnlmsg(msg);
	if (gf_dev->device_available == 1) {
		pr_debug("%s:shedule_work\n", __func__);
		gf_dev->wait_finger_down = false;
		schedule_work(&gf_dev->work);
	}
	return IRQ_HANDLED;
}

static inline int irq_setup(struct gf_dev *gf_dev)
{
	int status;
	gf_dev->irq = gpio_to_irq(gf_dev->irq_gpio);
	status = request_threaded_irq(gf_dev->irq, NULL, gf_irq,
			IRQF_TRIGGER_RISING | IRQF_ONESHOT | IRQF_PERF_AFFINE,
			"gf", gf_dev);

	if (status) {
		return status;
	}
	enable_irq_wake(gf_dev->irq);
	gf_dev->irq_enabled = 1;

	return status;
}

static long gf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct gf_dev *gf_dev = &gf;
	int retval = 0;
	u8 netlink_route = NETLINK_TEST;
	struct gf_ioc_chip_info info;

	if (_IOC_TYPE(cmd) != GF_IOC_MAGIC)
		return -ENODEV;

	if (_IOC_DIR(cmd) & _IOC_READ)
		retval = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		retval = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (retval)
		return -EFAULT;

	switch (cmd) {
	case GF_IOC_INIT:
		if (copy_to_user((void __user *)arg, (void *)&netlink_route, sizeof(u8))) {
			pr_err("GF_IOC_INIT failed\n");
			retval = -EFAULT;
			break;
		}
		break;

	case GF_IOC_EXIT:
		break;

	case GF_IOC_DISABLE_IRQ:
		gf_disable_irq(gf_dev);
		break;

	case GF_IOC_ENABLE_IRQ:
		gf_enable_irq(gf_dev);
		break;

	case GF_IOC_RESET:
		gf_hw_reset(gf_dev, 3);
		break;

	case GF_IOC_ENABLE_POWER:
	if (gf_dev->device_available == 0)
		gf_power_on(gf_dev);
		break;

	case GF_IOC_DISABLE_POWER:
		if (gf_dev->device_available == 1)
		gf_power_off(gf_dev);
		break;

	case GF_IOC_ENTER_SLEEP_MODE:
		break;

	case GF_IOC_GET_FW_INFO:
		break;

	case GF_IOC_REMOVE:
		break;

	case GF_IOC_CHIP_INFO:
		if (copy_from_user(&info, (void __user *)arg, sizeof(struct gf_ioc_chip_info))) {
			retval = -EFAULT;
			break;
		}
		break;

	case GF_IOC_AUTHENTICATE_START:
		pr_debug("%s GF_IOC_AUTHENTICATE_START\n", __func__);
		gf_dev->device_available = 1;
		break;

	case GF_IOC_AUTHENTICATE_END:
		pr_debug("%s GF_IOC_AUTHENTICATE_END\n", __func__);
		gf_dev->device_available = 0;
		break;

	default:
		break;
	}

	return retval;
}

#ifdef CONFIG_COMPAT
static inline long gf_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return gf_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif /*CONFIG_COMPAT*/

 static void notification_work(struct work_struct *work)
{
	pr_debug("%s unblank\n", __func__);
	dsi_bridge_interface_enable(FP_UNLOCK_REJECTION_TIMEOUT);
}

static inline int gf_open(struct inode *inode, struct file *filp)
{
	struct gf_dev *gf_dev = &gf;
	int status = -ENXIO;

	mutex_lock(&device_list_lock);

	list_for_each_entry(gf_dev, &device_list, device_entry) {
		if (gf_dev->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}

	if (status == 0) {
		if (status == 0) {
			gf_dev->users++;
			filp->private_data = gf_dev;
			nonseekable_open(inode, filp);
			gf_disable_irq(gf_dev);
		}
	}
	mutex_unlock(&device_list_lock);

	return status;
}

static int proc_show_ver(struct seq_file *file, void *v)
{
	seq_printf(file, "Fingerprint: Goodix\n");
	return 0;
}

static int proc_open(struct inode *inode, struct file *file)
{
	pr_debug("gf3258 proc_opening\n");
	single_open(file, proc_show_ver, NULL);
	return 0;
}

static inline int gf_release(struct inode *inode, struct file *filp)
{
	struct gf_dev *gf_dev = &gf;
	int status = 0;

	mutex_lock(&device_list_lock);
	gf_dev = filp->private_data;
	filp->private_data = NULL;
	gf_dev->users--;
	if (!gf_dev->users) {
		gf_disable_irq(gf_dev);
		gf_dev->device_available = 0;
		gf_power_off(gf_dev);
	}
	mutex_unlock(&device_list_lock);
	return status;
}

static const struct file_operations gf_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = gf_ioctl,
	.compat_ioctl = gf_compat_ioctl,
	.open = gf_open,
	.release = gf_release,
};

static const struct file_operations proc_file_ops = {
	.owner = THIS_MODULE,
	.open = proc_open,
	.read = seq_read,
	.release = single_release,
};

static void set_fingerprintd_nice(int nice)
{
	struct task_struct *p;

	read_lock(&tasklist_lock);
	for_each_process(p) {
		if (strstr(p->comm, "erprint"))
			set_user_nice(p, nice);
	}
	read_unlock(&tasklist_lock);
}

static int goodix_fb_state_chg_callback(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct gf_dev *gf_dev;
	struct fb_event *evdata = data;
	int *blank;
	char msg[2] = { 0x0 };

	if (val != MSM_DRM_EVENT_BLANK && val != MSM_DRM_EARLY_EVENT_BLANK)
		return 0;
	gf_dev = container_of(nb, struct gf_dev, notifier);
	if (evdata && evdata->data && val == MSM_DRM_EVENT_BLANK && gf_dev) {
		blank = evdata->data;
		if (*blank == MSM_DRM_BLANK_UNBLANK) {
				set_fingerprintd_nice(0);
				gf_dev->fb_black = 0;
				msg[0] = GF_NET_EVENT_FB_UNBLACK;
				sendnlmsg(msg);
			}

	}else if(evdata && evdata->data && val == MSM_DRM_EARLY_EVENT_BLANK && gf_dev){
		blank = evdata->data;
			if (*blank == MSM_DRM_BLANK_POWERDOWN) {
				set_fingerprintd_nice(MIN_NICE);
				gf_dev->fb_black = 1;
				gf_dev->wait_finger_down = true;
				msg[0] = GF_NET_EVENT_FB_BLACK;
				sendnlmsg(msg);
			}

	}
	return NOTIFY_OK;
}

static struct notifier_block goodix_noti_block = {
	.notifier_call = goodix_fb_state_chg_callback,
};

static struct class *gf_class;
static int gf_probe(struct platform_device *pdev)
{
	struct gf_dev *gf_dev = &gf;
	int status = -EINVAL;
	unsigned long minor;
	int i;
	pr_info("Macle11 gf probe\n");
	/* Initialize the driver data */
	INIT_LIST_HEAD(&gf_dev->device_entry);
	gf_dev->spi = pdev;
	gf_dev->irq_gpio = -EINVAL;
	gf_dev->reset_gpio = -EINVAL;
	gf_dev->pwr_gpio = -EINVAL;
	gf_dev->device_available = 0;
	gf_dev->fb_black = 0;
	gf_dev->wait_finger_down = false;
	INIT_WORK(&gf_dev->work, notification_work);

	/* If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		gf_dev->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(gf_class, &gf_dev->spi->dev, gf_dev->devt,
				gf_dev, GF_DEV_NAME);
		status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	} else {
		dev_dbg(&gf_dev->spi->dev, "no minor number available!\n");
		status = -ENODEV;
		mutex_unlock(&device_list_lock);
		goto error_hw;
	}

	if (status == 0) {
		set_bit(minor, minors);
		list_add(&gf_dev->device_entry, &device_list);
	} else {
		gf_dev->devt = 0;
		goto error_hw;
	}
	mutex_unlock(&device_list_lock);

	gf_dev->input = input_allocate_device();
	if (gf_dev->input == NULL) {
		status = -ENOMEM;
		goto error_dev;
	}

	gf_dev->input->name = GF_INPUT_NAME;
	status = input_register_device(gf_dev->input);
	if (status) {
		goto error_input;
	}

	gf_dev->notifier = goodix_noti_block;
	msm_drm_register_client(&gf_dev->notifier);

	//wake_lock_init(&fp_wakelock, WAKE_LOCK_SUSPEND, "fp_wakelock");
	wakeup_source_init(&fp_ws, "fp_ws");//for kernel 4.9

	proc_entry = proc_create(PROC_NAME, 0644, NULL, &proc_file_ops);
	if (NULL == proc_entry) {
		return -ENOMEM;
	}

	return status;

error_input:
	if (gf_dev->input != NULL)
		input_free_device(gf_dev->input);
error_dev:
	if (gf_dev->devt != 0) {
		mutex_lock(&device_list_lock);
		list_del(&gf_dev->device_entry);
		device_destroy(gf_class, gf_dev->devt);
		clear_bit(MINOR(gf_dev->devt), minors);
		mutex_unlock(&device_list_lock);
	}
error_hw:
	gf_dev->device_available = 0;

	return status;
}

static inline int gf_remove(struct platform_device *pdev)
{
	struct gf_dev *gf_dev = &gf;

	//wake_lock_destroy(&fp_wakelock);
	wakeup_source_trash(&fp_ws);//for kernel 4.9
	msm_drm_unregister_client(&gf_dev->notifier);
	if (gf_dev->input)
		input_unregister_device(gf_dev->input);
	input_free_device(gf_dev->input);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	list_del(&gf_dev->device_entry);
	device_destroy(gf_class, gf_dev->devt);
	clear_bit(MINOR(gf_dev->devt), minors);
	remove_proc_entry(PROC_NAME, NULL);
	mutex_unlock(&device_list_lock);

	return 0;
}

static const struct of_device_id gx_match_table[] = {
	{ .compatible = GF_SPIDEV_NAME },
	{},
};

static struct platform_driver gf_driver = {
	.driver = {
		.name = GF_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = gx_match_table,
	},
	.probe = gf_probe,
	.remove = gf_remove,
};

static inline int __init gf_init(void)
{
	int status;

	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	if(fpsensor != 2) {
		return FAIL;
	}

	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(SPIDEV_MAJOR, CHRD_DRIVER_NAME, &gf_fops);
	if (status < 0) {
		return status;
	}
	SPIDEV_MAJOR = status;
	gf_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(gf_class)) {
		unregister_chrdev(SPIDEV_MAJOR, gf_driver.driver.name);
		return PTR_ERR(gf_class);
	}
	status = platform_driver_register(&gf_driver);
	if (status < 0) {
		class_destroy(gf_class);
		unregister_chrdev(SPIDEV_MAJOR, gf_driver.driver.name);
	}

	netlink_init();
	return 0;
}
module_init(gf_init);

static inline void __exit gf_exit(void)
{
	netlink_exit();
	platform_driver_unregister(&gf_driver);
	class_destroy(gf_class);
	unregister_chrdev(SPIDEV_MAJOR, gf_driver.driver.name);
}
module_exit(gf_exit);

MODULE_AUTHOR("Jiangtao Yi, <yijiangtao@goodix.com>");
MODULE_AUTHOR("Jandy Gou, <gouqingsong@goodix.com>");
MODULE_DESCRIPTION("goodix fingerprint sensor device driver");
MODULE_LICENSE("GPL");

