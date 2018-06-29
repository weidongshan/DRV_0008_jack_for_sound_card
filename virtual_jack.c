
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/switch.h>
#include <linux/input.h>

static struct input_dev *g_virtual_input;

static ssize_t input_test_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	long code;
	long val;
	char *endp;

	/* ����ַ���ǰ�溬�з�����, simple_strtol���ܴ��� */
	while ((*buf == ' ') || (*buf == '\t'))
		buf++;

	code = simple_strtol(buf, &endp, 0);

	/* ����ַ���ǰ�溬�з�����, simple_strtol���ܴ��� */
	while ((*endp == ' ') || (*endp == '\t'))
		endp++;
	val  = simple_strtol(endp, NULL, 0);

	printk("emulate to report EV_SW: 0x%x 0x%x\n", code, val);
	input_event(g_virtual_input, EV_SW, code, val);
	input_sync(g_virtual_input);

	return count;
}

static DEVICE_ATTR(test_input, S_IRUGO | S_IWUSR, NULL, input_test_store);

static int register_input_device_for_jack(void)
{
	int err;
	
	/* ����input_dev */
	g_virtual_input = input_allocate_device();

	/* ���� */
	/* 2.1 �ܲ��������¼� */
	set_bit(EV_SYN, g_virtual_input->evbit);
	set_bit(EV_SW, g_virtual_input->evbit);


	/* 2.2 �ܲ��������¼��е���Щ */
	/* headset = ��Ͳ + MIC = SW_HEADPHONE_INSERT + SW_MICROPHONE_INSERT
	 *    ͬʱ�ϱ� SW_HEADPHONE_INSERT �� SW_MICROPHONE_INSERT, �ͱ�ʾheadset
	 *    Ϊ�˼�, ����androidϵͳֻ�ϱ�SW_MICROPHONE_INSERTҲ��ʾheadset
	 */
	set_bit(SW_HEADPHONE_INSERT, g_virtual_input->swbit);
	set_bit(SW_MICROPHONE_INSERT, g_virtual_input->swbit);
	set_bit(SW_LINEOUT_INSERT, g_virtual_input->swbit);

	/* 2.3 ��Щ�¼��ķ�Χ */

	g_virtual_input->name = "alsa_switch"; /* ����Ҫ */

	/* ע�� */
	err = input_register_device(g_virtual_input);
	if (err) {
		input_free_device(g_virtual_input);	
		printk("input_register_device for virtual jack err!\n");
		return err;
	}

	/* ����/sys/class/input/inputX/test_input�ļ�
	 *   ����ִ�����������������ģ�����Ķ���:  
	 *       �����ϱ�headset����: echo 4 1 > /sys/class/input/inputX/test_input  
	 *       �����ϱ�headsetȡ��: echo 4 0 > /sys/class/input/inputX/test_input  
	 */
	err = device_create_file(&g_virtual_input->dev, &dev_attr_test_input);
	if (err) {
		printk("device_create_file for test_input err!\n");
		input_unregister_device(g_virtual_input);
		input_free_device(g_virtual_input); 
		return err;
	}

	return 0;
}

static void unregister_input_device_for_jack(void)
{
	device_remove_file(&g_virtual_input->dev, &dev_attr_test_input);

	input_unregister_device(g_virtual_input);
	input_free_device(g_virtual_input);	
}


/**************************************************************************************************************/

static struct switch_dev g_virtual_switch;

static ssize_t state_test_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	long val;

	val = simple_strtol(buf, NULL, 0);

	printk("emulate to report swtich state: 0x%x\n", val);
	switch_set_state(&g_virtual_switch, val);

	return count;
}

static DEVICE_ATTR(test_state, S_IRUGO | S_IWUSR, NULL, state_test_store);

static int register_switch_device_for_jack(void)
{
	int err;
	
	g_virtual_switch.name = "h2w";
	err = switch_dev_register(&g_virtual_switch);
	if (err) {
		printk("switch_dev_register h2w err!\n");
		return err;
	}

	/* ����/sys/class/switch/h2w/test_state�ļ�
	 *   ����ִ�����������������ģ�����Ķ���:  
	 *       �����ϱ�headset����: echo 1 > /sys/class/switch/h2w/test_state
	 *       �����ϱ�headsetȡ��: echo 0 > /sys/class/switch/h2w/test_state
	 */
	err = device_create_file(g_virtual_switch.dev, &dev_attr_test_state);
	if (err) {
		printk("device_create_file test err!\n");
		switch_dev_unregister(&g_virtual_switch);
		return err;
	}

	return 0;
}

static void unregister_switch_device_for_jack(void)
{
	device_remove_file(g_virtual_switch.dev, &dev_attr_test_state);
	switch_dev_unregister(&g_virtual_switch);
}

/**************************************************************************************************************/

static int __init virtual_jack_init(void)
{
	int err;
	
	err = register_input_device_for_jack();
	err = register_switch_device_for_jack();
	
	return 0;
}

static void __exit virtual_jack_exit(void)
{
	unregister_input_device_for_jack();
	unregister_switch_device_for_jack();
}

module_init(virtual_jack_init);
module_exit(virtual_jack_exit);

MODULE_AUTHOR("weidongshan@qq.com");
MODULE_DESCRIPTION("Virutal jack driver for sound card");
MODULE_LICENSE("GPL");

