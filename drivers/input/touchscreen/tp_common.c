#include <linux/input/tp_common.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>

bool capacitive_keys_enabled;
struct kobject *touchpanel_kobj;

#define TS_ENABLE_FOPS(type)                                                   \
	int tp_common_set_##type##_ops(struct tp_common_ops *ops)              \
	{                                                                      \
		static struct kobj_attribute kattr =                           \
			__ATTR(type, (S_IWUSR | S_IRUGO), NULL, NULL);         \
		kattr.show = ops->show;                                        \
		kattr.store = ops->store;                                      \
		return sysfs_create_file(touchpanel_kobj, &kattr.attr);        \
	}

TS_ENABLE_FOPS(capacitive_keys)
TS_ENABLE_FOPS(double_tap)
TS_ENABLE_FOPS(reversed_keys)

#define tpdir "touchpanel"

#define d_tap "touchpanel/double_tap_enable"
#define tp_dt "/touchpanel/double_tap"
#define tp_g "touchpanel/gesture_enable"
#define tpg "tp_gesture"
#define tpdir_fts "devices/platform/soc/a98000.i2c/i2c-3/3-0038/fts_gesture_mode"

static int __init tp_common_init(void)
{
	char *driver_path;
	static struct proc_dir_entry *tp_dir;
	static struct proc_dir_entry *tp_oos;
	int ret = 0;

	tp_dir = proc_mkdir(tpdir, NULL);
	driver_path = kzalloc(PATH_MAX, GFP_KERNEL);
	if (!driver_path) {
		pr_err("%s: failed to allocate memory\n", __func__);
		ret = -ENOMEM;
	}

	sprintf(driver_path, "/sys%s", tp_dt);

	pr_err("%s: driver_path=%s\n", __func__, driver_path);

	tp_oos = proc_symlink(d_tap, NULL, driver_path);

	if (!tp_oos) {
		ret = -ENOMEM;
	}
	kfree(driver_path);
	tp_oos = proc_symlink(tp_g, NULL, "double_tap_enable");
	if (!tp_oos)
		ret = -ENOMEM;
	touchpanel_kobj = kobject_create_and_add("touchpanel", NULL);
	if (!touchpanel_kobj)
		return -ENOMEM;

	return 0;
}

core_initcall(tp_common_init);
