// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc.
 */
#include <net/genetlink.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/module.h>

#include "fps_cooler.h"
#include "fpsgo_common.h"
#include "fstb.h"

#define EARA_MAX_COUNT 10
#define EARA_PROC_NAME_LEN 16
#define NETLINK_FPS 29
#define TAG "FPS_COOLER"
#define EARA_ENABLE                 _IOW('g', 1, int)

struct _EARA_THRM_PACKAGE {
	__s32 request;
	__s32 pair_pid[EARA_MAX_COUNT];
	__u64 pair_bufid[EARA_MAX_COUNT];
	__s32 pair_tfps[EARA_MAX_COUNT];
	__s32 pair_diff[EARA_MAX_COUNT];
	char proc_name[EARA_MAX_COUNT][EARA_PROC_NAME_LEN];
};

static int eara_enable;
static DEFINE_MUTEX(pre_lock);
static struct sock *fps_cooler_nl_sk;
static int eara_pid = -1;

static void set_tfps_diff(int max_cnt, int *pid, unsigned long long *buf_id, int *tfps, int *diff)
{
	int i;

	mutex_lock(&pre_lock);

	if (!eara_enable) {
		mutex_unlock(&pre_lock);
		return;
	}

	for (i = 0; i < max_cnt; i++) {
		if (pid[i] == 0)
			break;
		pr_debug(TAG "Set %d %llu: %d\n", pid[i], buf_id[i], diff[i]);
		eara2fstb_tfps_mdiff(pid[i], buf_id[i], diff[i], tfps[i]);
	}

	mutex_unlock(&pre_lock);
}

static void switch_eara(int enable)
{
	mutex_lock(&pre_lock);
	eara_enable = enable;
	eara_pid = task_pid_nr(current);
	mutex_unlock(&pre_lock);

}

int pre_change_single_event(int pid, unsigned long long bufID,
			int target_fps)
{
	struct _EARA_THRM_PACKAGE change_msg;
	int ret = 0;

	mutex_lock(&pre_lock);
	if (!eara_enable) {
		mutex_unlock(&pre_lock);
		return -1;
	}
	mutex_unlock(&pre_lock);

	memset(&change_msg, 0, sizeof(struct _EARA_THRM_PACKAGE));
	change_msg.request = 1;
	change_msg.pair_pid[0] = pid;
	change_msg.pair_bufid[0] = bufID;
	change_msg.pair_tfps[0] = target_fps;
	ret = eara_nl_send_to_user((void *)&change_msg, sizeof(struct _EARA_THRM_PACKAGE));

	return ret;
}

int pre_change_event(void)
{
	struct _EARA_THRM_PACKAGE change_msg;
	int ret = 0;

	mutex_lock(&pre_lock);
	if (!eara_enable) {
		mutex_unlock(&pre_lock);
		return -1;
	}
	mutex_unlock(&pre_lock);
	memset(&change_msg, 0, sizeof(struct _EARA_THRM_PACKAGE));
	eara2fstb_get_tfps(EARA_MAX_COUNT, change_msg.pair_pid, change_msg.pair_bufid, change_msg.pair_tfps,
			change_msg.proc_name);
	ret = eara_nl_send_to_user((void *)&change_msg, sizeof(struct _EARA_THRM_PACKAGE));

	return ret;
}

int eara_nl_send_to_user(void *buf, int size)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;

	int len = NLMSG_SPACE(size);
	void *data;
	int ret;

	if (fps_cooler_nl_sk == NULL)
		return -1;

	skb = alloc_skb(len, GFP_ATOMIC);
	if (!skb)
		return -1;
	nlh = nlmsg_put(skb, 0, 0, NLMSG_DONE, size+1, 0);
	data = NLMSG_DATA(nlh);
	memcpy(data, buf, size);
	NETLINK_CB(skb).portid = 0; /* from kernel */
	NETLINK_CB(skb).dst_group = 0; /* unicast */

	pr_debug(TAG "Netlink_unicast size=%d\n", size);

	ret = netlink_unicast(fps_cooler_nl_sk, skb, eara_pid, MSG_DONTWAIT);
	if (ret < 0) {
		pr_debug(TAG "Send to pid %d failed %d\n", eara_pid, ret);
		return -1;
	}
	pr_debug(TAG "Netlink_unicast- ret=%d\n", ret);
	return 0;

}

static void eara_nl_data_handler(struct sk_buff *skb)
{
	u32 pid;
	kuid_t uid;
	int seq;
	void *data;
	struct nlmsghdr *nlh;
	struct _EARA_THRM_PACKAGE *change_msg;
	//int size = 0;

	nlh = (struct nlmsghdr *)skb->data;
	pid = NETLINK_CREDS(skb)->pid;
	uid = NETLINK_CREDS(skb)->uid;
	seq = nlh->nlmsg_seq;

	/*tsta_dprintk(
	 *"[ta_nl_data_handler] recv skb from user space uid:%d pid:%d seq:%d\n"
	 * ,uid, pid, seq);
	 */
	data = NLMSG_DATA(nlh);
	change_msg = (struct _EARA_THRM_PACKAGE *) NLMSG_DATA(nlh);
	set_tfps_diff(EARA_MAX_COUNT, change_msg->pair_pid,
			change_msg->pair_bufid, change_msg->pair_tfps, change_msg->pair_diff);
}


int eara_netlink_init(void)
{
	/*add by willcai for the userspace  to kernelspace*/
	struct netlink_kernel_cfg cfg = {
		.input  = eara_nl_data_handler,
	};

	fps_cooler_nl_sk = NULL;
	fps_cooler_nl_sk = netlink_kernel_create(&init_net, NETLINK_FPS, &cfg);

	pr_debug(TAG "netlink_kernel_create protol= %d\n", NETLINK_FPS);

	if (fps_cooler_nl_sk == NULL) {
		pr_debug(TAG "netlink_kernel_create fail\n");
		return -1;
	}

	return 0;
}

static int fps_cooler_show(struct seq_file *m, void *v)
{
	return 0;
}

static int fps_cooler_open(struct inode *inode, struct file *file)
{
	return single_open(file, fps_cooler_show, inode->i_private);
}

static int fps_cooler_release(struct inode *inode, struct file *file)
{
	return single_release(inode, file);
}

static long fps_cooler_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	ssize_t ret = 0;

	switch (cmd) {
	case EARA_ENABLE:
		switch_eara(arg);
		break;
	default:
		pr_debug(TAG "%s %d: unknown cmd %x\n",
				__FILE__, __LINE__, cmd);
		ret = -1;
	}
	return ret;
}

static const struct proc_ops fps_cooler_Fops = {
	.proc_compat_ioctl = fps_cooler_ioctl,
	.proc_open = fps_cooler_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = fps_cooler_release,
};

static int init_proc(void)
{
	struct proc_dir_entry *pe;
	struct proc_dir_entry *parent;
	int ret_val = 0;


	pr_debug(TAG"init proc ioctl\n");

	parent = proc_mkdir("fps_cooler", NULL);

	pe = proc_create("control", 0664, parent, &fps_cooler_Fops);
	if (!pe) {
		pr_debug(TAG"%s failed with %d\n",
				"Creating file node ",
				ret_val);
		ret_val = -ENOMEM;
	}
	pr_debug(TAG"init proc ioctl done\n");

	return ret_val;
}

void __exit eara_thrm_pre_exit(void)
{
	eara_pre_change_fp = NULL;
	eara_pre_change_single_fp = NULL;
}

int __init eara_thrm_pre_init(void)
{
	eara_pre_change_fp = pre_change_event;
	eara_pre_change_single_fp =  pre_change_single_event;
	init_proc();
	eara_netlink_init();
	return 0;
}

module_init(eara_thrm_pre_init);
module_exit(eara_thrm_pre_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek FPS COOLER");
MODULE_AUTHOR("MediaTek Inc.");
