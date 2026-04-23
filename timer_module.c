#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>

#define INTERVAL_DEFAULT (30 * HZ)
#define MAX_TIMEOUT_JIFFIES (5 * 60 * HZ)

static struct timer_list my_timer;
static unsigned int interval = INTERVAL_DEFAULT; // Динамическая переменная
static unsigned long start_time; 
static struct kobject *interval_kobj;

static void timer_callback(struct timer_list *t)
{
    unsigned long now = jiffies;
    
    if (time_after(now, start_time + MAX_TIMEOUT_JIFFIES)) {
        pr_info("timer_module: Лимит 5 минут исчерпан. Остановка.\n");
        return;
    }

    unsigned long elapsed_ms = jiffies_to_msecs(now - start_time);
    unsigned int total_sec = elapsed_ms / 1000;
    
    pr_info("timer_module: [%02u:%02u] Hello, timer! Next in %u sec.\n", 
            total_sec / 60, total_sec % 60, READ_ONCE(interval) / HZ);

    // Используем динамический интервал для следующего шага
    mod_timer(&my_timer, jiffies + READ_ONCE(interval));
}

// Чтение интервала из sysfs
static ssize_t interval_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", READ_ONCE(interval) / HZ);
}

// Запись нового интервала через sysfs
static ssize_t interval_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    unsigned long val;
    if (kstrtoul(buf, 10, &val) || val == 0)
        return -EINVAL;
    
    WRITE_ONCE(interval, val * HZ); // Обновляем интервал "на лету"
    return count;
}

static struct kobj_attribute interval_attr = __ATTR(interval, 0664, interval_show, interval_store);

static struct attribute *attrs[] = { &interval_attr.attr, NULL };
static struct attribute_group attr_group = { .attrs = attrs };

static int __init timer_module_init(void)
{
    start_time = jiffies;
    interval_kobj = kobject_create_and_add("timer_module", kernel_kobj);
    if (!interval_kobj) return -ENOMEM;

    if (sysfs_create_group(interval_kobj, &attr_group)) {
        kobject_put(interval_kobj);
        return -ENOMEM;
    }

    timer_setup(&my_timer, timer_callback, 0);
    mod_timer(&my_timer, jiffies + interval);

    pr_info("timer_module: Модуль загружен. Попробуйте: echo 5 > /sys/kernel/timer_module/interval\n");
    return 0;
}

static void __exit timer_module_exit(void)
{
    timer_delete_sync(&my_timer);
    sysfs_remove_group(interval_kobj, &attr_group);
    kobject_put(interval_kobj);
    pr_info("timer_module: Выгружен\n");
}

module_init(timer_module_init);
module_exit(timer_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anton Zubin");
MODULE_DESCRIPTION("Timer module with formatted output and 5min limit");

