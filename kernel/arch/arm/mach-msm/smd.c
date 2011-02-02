/* arch/arm/mach-msm/smd.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/termios.h>
#include <linux/ctype.h>
#include <mach/msm_smd.h>
#include <mach/msm_iomap.h>
#include <mach/system.h>

#ifdef CONFIG_FIH_FXX
/* FIH, Debbie, 2009/12/07 { */
/* add for F917 */
#include <mach/gpio.h>
/* FIH, Debbie, 2009/12/07 } */
#endif

#include "smd_private.h"
#include "proc_comm.h"
#include "modem_notifier.h"

#if defined(CONFIG_ARCH_QSD8X50) || defined(CONFIG_ARCH_MSM8X60)
#define CONFIG_QDSP6 1
#endif

#define MODULE_NAME "msm_smd"
#define SMEM_VERSION 0x000B
#define SMD_VERSION 0x00020000

enum {
	MSM_SMD_DEBUG = 1U << 0,
	MSM_SMSM_DEBUG = 1U << 1,
	MSM_SMD_INFO = 1U << 2,
	MSM_SMSM_INFO = 1U << 3,
};

struct smsm_shared_info {
	uint32_t *state;
	uint32_t *intr_mask;
	uint32_t *intr_mux;
};

static struct smsm_shared_info smsm_info;

#define SMSM_STATE_ADDR(entry)           (smsm_info.state + entry)
#define SMSM_INTR_MASK_ADDR(entry, host) (smsm_info.intr_mask + \
					  entry * SMSM_NUM_HOSTS + host)
#define SMSM_INTR_MUX_ADDR(entry)        (smsm_info.intr_mux + entry)

/* Internal definitions which are not exported in some targets */
enum {
	SMSM_Q6_I = 2,
};

enum {
	SMSM_APPS_DEM_I = 3,
};

enum {
	SMD_APPS_QDSP_I = 1,
	SMD_MODEM_QDSP_I = 2
};

static int msm_smd_debug_mask;
module_param_named(debug_mask, msm_smd_debug_mask,
		   int, S_IRUGO | S_IWUSR | S_IWGRP);

#if defined(CONFIG_MSM_SMD_DEBUG)
#define SMD_DBG(x...) do {				\
		if (msm_smd_debug_mask & MSM_SMD_DEBUG) \
			printk(KERN_DEBUG x);		\
	} while (0)

#define SMSM_DBG(x...) do {					\
		if (msm_smd_debug_mask & MSM_SMSM_DEBUG)	\
			printk(KERN_DEBUG x);			\
	} while (0)

#define SMD_INFO(x...) do {			 	\
		if (msm_smd_debug_mask & MSM_SMD_INFO)	\
			printk(KERN_INFO x);		\
	} while (0)

#define SMSM_INFO(x...) do {				\
		if (msm_smd_debug_mask & MSM_SMSM_INFO) \
			printk(KERN_INFO x);		\
	} while (0)
#else
#define SMD_DBG(x...) do { } while (0)
#define SMSM_DBG(x...) do { } while (0)
#define SMD_INFO(x...) do { } while (0)
#define SMSM_INFO(x...) do { } while (0)
#endif

static unsigned last_heap_free = 0xffffffff;

#if defined(CONFIG_ARCH_MSM7X30)
#define MSM_TRIG_A2M_SMD_INT   (writel(1 << 0, MSM_GCC_BASE + 0x8))
#define MSM_TRIG_A2Q6_SMD_INT  (writel(1 << 8, MSM_GCC_BASE + 0x8))
#define MSM_TRIG_A2M_SMSM_INT  (writel(1 << 5, MSM_GCC_BASE + 0x8))
#define MSM_TRIG_A2Q6_SMSM_INT (writel(1 << 8, MSM_GCC_BASE + 0x8))
#elif defined(CONFIG_ARCH_MSM8X60)
#define MSM_TRIG_A2M_SMD_INT   (writel(1 << 3, MSM_GCC_BASE + 0x8))
#define MSM_TRIG_A2Q6_SMD_INT  (writel(1 << 15, MSM_GCC_BASE + 0x8))
#define MSM_TRIG_A2M_SMSM_INT  (writel(1 << 4, MSM_GCC_BASE + 0x8))
#define MSM_TRIG_A2Q6_SMSM_INT (writel(1 << 15, MSM_GCC_BASE + 0x8))
#else
#define MSM_TRIG_A2M_SMD_INT   (writel(1, MSM_CSR_BASE + 0x400 + (0) * 4))
#define MSM_TRIG_A2Q6_SMD_INT  (writel(1, MSM_CSR_BASE + 0x400 + (8) * 4))
#define MSM_TRIG_A2M_SMSM_INT  (writel(1, MSM_CSR_BASE + 0x400 + (5) * 4))
#define MSM_TRIG_A2Q6_SMSM_INT (writel(1, MSM_CSR_BASE + 0x400 + (8) * 4))
#endif

#define SMD_LOOPBACK_CID 100

static LIST_HEAD(smd_ch_list_loopback);

static void notify_other_smsm(uint32_t smsm_entry, uint32_t notify_mask)
{
	uint32_t mux_val;

	/* older protocol don't use smsm_intr_mask,
	   but still communicates with modem */
	if (!smsm_info.intr_mask ||
	    (readl(SMSM_INTR_MASK_ADDR(smsm_entry, SMSM_MODEM)) & notify_mask))
		MSM_TRIG_A2M_SMSM_INT;

	if (smsm_info.intr_mask &&
	    (readl(SMSM_INTR_MASK_ADDR(smsm_entry, SMSM_Q6_I)) & notify_mask)) {
		if (smsm_info.intr_mux) {
			mux_val = readl(SMSM_INTR_MUX_ADDR(SMEM_APPS_Q6_SMSM));
			mux_val++;
			writel(mux_val, SMSM_INTR_MUX_ADDR(SMEM_APPS_Q6_SMSM));
		}

		MSM_TRIG_A2Q6_SMSM_INT;
	}
}

static inline void notify_modem_smd(void)
{
	MSM_TRIG_A2M_SMD_INT;
}

static inline void notify_dsp_smd(void)
{
	MSM_TRIG_A2Q6_SMD_INT;
}

void smd_diag(void)
{
	char *x;
	int size;

	x = smem_find(ID_DIAG_ERR_MSG, SZ_DIAG_ERR_MSG);
	if (x != 0) {
		x[SZ_DIAG_ERR_MSG - 1] = 0;
		SMD_INFO("smem: DIAG '%s'\n", x);
	}

	x = smem_get_entry(SMEM_ERR_CRASH_LOG, &size);
	if (x != 0) {
		x[size - 1] = 0;
		pr_err("smem: CRASH LOG\n'%s'\n", x);
	}
}


static void handle_modem_crash(void)
{
	pr_err("ARM9 has CRASHED\n");
	smd_diag();

	/* hard reboot if possible FIXME
	if (msm_reset_hook)
		msm_reset_hook();
	*/

	/* in this case the modem or watchdog should reboot us */
	for (;;)
		;
}

int smsm_check_for_modem_crash(void)
{
	/* if the modem's not ready yet, we have to hope for the best */
	if (!smsm_info.state)
		return 0;

	if (readl(SMSM_STATE_ADDR(SMSM_MODEM_STATE)) & SMSM_RESET) {
		handle_modem_crash();
		return -1;
	}
	return 0;
}

/* the spinlock is used to synchronize between the
 * irq handler and code that mutates the channel
 * list or fiddles with channel state
 */
static DEFINE_SPINLOCK(smd_lock);
DEFINE_SPINLOCK(smem_lock);

/* the mutex is used during open() and close()
 * operations to avoid races while creating or
 * destroying smd_channel structures
 */
static DEFINE_MUTEX(smd_creation_mutex);

static int smd_initialized;

struct smd_shared_v1 {
	struct smd_half_channel ch0;
	unsigned char data0[SMD_BUF_SIZE];
	struct smd_half_channel ch1;
	unsigned char data1[SMD_BUF_SIZE];
};

struct smd_shared_v2 {
	struct smd_half_channel ch0;
	struct smd_half_channel ch1;
};

struct smd_channel {
	volatile struct smd_half_channel *send;
	volatile struct smd_half_channel *recv;
	unsigned char *send_data;
	unsigned char *recv_data;
	unsigned fifo_size;
	unsigned fifo_mask;
	struct list_head ch_list;

	unsigned current_packet;
	unsigned n;
	void *priv;
	void (*notify)(void *priv, unsigned flags);

	int (*read)(smd_channel_t *ch, void *data, int len);
	int (*write)(smd_channel_t *ch, const void *data, int len);
	int (*read_avail)(smd_channel_t *ch);
	int (*write_avail)(smd_channel_t *ch);
	int (*read_from_cb)(smd_channel_t *ch, void *data, int len);

	void (*update_state)(smd_channel_t *ch);
	unsigned last_state;
	void (*notify_other_cpu)(void);

	char name[20];
	struct platform_device pdev;
	unsigned type;
};

static LIST_HEAD(smd_ch_closed_list);
static LIST_HEAD(smd_ch_list_modem);
static LIST_HEAD(smd_ch_list_dsp);

static unsigned char smd_ch_allocated[64];
static struct work_struct probe_work;

static int smd_alloc_channel(struct smd_alloc_elm *alloc_elm);

static void smd_channel_probe_worker(struct work_struct *work)
{
	struct smd_alloc_elm *shared;
	unsigned n;

	shared = smem_find(ID_CH_ALLOC_TBL, sizeof(*shared) * 64);

	if (!shared) {
		pr_err("%s: allocation table not initialized\n", __func__);
		return;
	}

	for (n = 0; n < 64; n++) {
		if (smd_ch_allocated[n])
			continue;

		/* channel should be allocated only if APPS
		   processor is involved */
		if (SMD_CHANNEL_TYPE(shared[n].type) == SMD_MODEM_QDSP_I)
			continue;
		if (!shared[n].ref_count)
			continue;
		if (!shared[n].name[0])
			continue;

		if (!smd_alloc_channel(&shared[n]))
			smd_ch_allocated[n] = 1;
		else
			SMD_INFO("Probe skipping ch %d, not allocated \n", n);
	}
}

/* how many bytes are available for reading */
static int smd_stream_read_avail(struct smd_channel *ch)
{
	return (ch->recv->head - ch->recv->tail) & ch->fifo_mask;
}

/* how many bytes we are free to write */
static int smd_stream_write_avail(struct smd_channel *ch)
{
	return ch->fifo_mask -
		((ch->send->head - ch->send->tail) & ch->fifo_mask);
}

static int smd_packet_read_avail(struct smd_channel *ch)
{
	if (ch->current_packet) {
		int n = smd_stream_read_avail(ch);
		if (n > ch->current_packet)
			n = ch->current_packet;
		return n;
	} else {
		return 0;
	}
}

static int smd_packet_write_avail(struct smd_channel *ch)
{
	int n = smd_stream_write_avail(ch);
	return n > SMD_HEADER_SIZE ? n - SMD_HEADER_SIZE : 0;
}

static int ch_is_open(struct smd_channel *ch)
{
	return (ch->recv->state == SMD_SS_OPENED ||
		ch->recv->state == SMD_SS_FLUSHING)
		&& (ch->send->state == SMD_SS_OPENED);
}

/* provide a pointer and length to readable data in the fifo */
static unsigned ch_read_buffer(struct smd_channel *ch, void **ptr)
{
	unsigned head = ch->recv->head;
	unsigned tail = ch->recv->tail;
	*ptr = (void *) (ch->recv_data + tail);

	if (tail <= head)
		return head - tail;
	else
		return ch->fifo_size - tail;
}

/* advance the fifo read pointer after data from ch_read_buffer is consumed */
static void ch_read_done(struct smd_channel *ch, unsigned count)
{
	BUG_ON(count > smd_stream_read_avail(ch));
	ch->recv->tail = (ch->recv->tail + count) & ch->fifo_mask;
	ch->send->fTAIL = 1;
}

/* basic read interface to ch_read_{buffer,done} used
 * by smd_*_read() and update_packet_state()
 * will read-and-discard if the _data pointer is null
 */
static int ch_read(struct smd_channel *ch, void *_data, int len)
{
	void *ptr;
	unsigned n;
	unsigned char *data = _data;
	int orig_len = len;

	while (len > 0) {
		n = ch_read_buffer(ch, &ptr);
		if (n == 0)
			break;

		if (n > len)
			n = len;
		if (_data)
			memcpy(data, ptr, n);

		data += n;
		len -= n;
		ch_read_done(ch, n);
	}

	return orig_len - len;
}

static void update_stream_state(struct smd_channel *ch)
{
	/* streams have no special state requiring updating */
}

static void update_packet_state(struct smd_channel *ch)
{
	unsigned hdr[5];
	int r;

	/* can't do anything if we're in the middle of a packet */
	while (ch->current_packet == 0) {
		/* discard 0 length packets if any */

		/* don't bother unless we can get the full header */
		if (smd_stream_read_avail(ch) < SMD_HEADER_SIZE)
			return;

		r = ch_read(ch, hdr, SMD_HEADER_SIZE);
		BUG_ON(r != SMD_HEADER_SIZE);

		ch->current_packet = hdr[0];
	}
}

/* provide a pointer and length to next free space in the fifo */
static unsigned ch_write_buffer(struct smd_channel *ch, void **ptr)
{
	unsigned head = ch->send->head;
	unsigned tail = ch->send->tail;
	*ptr = (void *) (ch->send_data + head);

	if (head < tail) {
		return tail - head - 1;
	} else {
		if (tail == 0)
			return ch->fifo_size - head - 1;
		else
			return ch->fifo_size - head;
	}
}

/* advace the fifo write pointer after freespace
 * from ch_write_buffer is filled
 */
static void ch_write_done(struct smd_channel *ch, unsigned count)
{
	BUG_ON(count > smd_stream_write_avail(ch));
	ch->send->head = (ch->send->head + count) & ch->fifo_mask;
	ch->send->fHEAD = 1;
}

static void ch_set_state(struct smd_channel *ch, unsigned n)
{
	if (n == SMD_SS_OPENED) {
		ch->send->fDSR = 1;
		ch->send->fCTS = 1;
		ch->send->fCD = 1;
	} else {
		ch->send->fDSR = 0;
		ch->send->fCTS = 0;
		ch->send->fCD = 0;
	}
	ch->send->state = n;
	ch->send->fSTATE = 1;
	ch->notify_other_cpu();
}

static void do_smd_probe(void)
{
	struct smem_shared *shared = (void *) MSM_SHARED_RAM_BASE;
	if (shared->heap_info.free_offset != last_heap_free) {
		last_heap_free = shared->heap_info.free_offset;
		schedule_work(&probe_work);
	}
}

static void smd_state_change(struct smd_channel *ch,
			     unsigned last, unsigned next)
{
	ch->last_state = next;

	SMD_INFO("SMD: ch %d %d -> %d\n", ch->n, last, next);

	switch (next) {
	case SMD_SS_OPENING:
		if (ch->send->state == SMD_SS_CLOSING ||
		    ch->send->state == SMD_SS_CLOSED) {
			ch->recv->tail = 0;
			ch->send->head = 0;
			ch_set_state(ch, SMD_SS_OPENING);
		}
		break;
	case SMD_SS_OPENED:
		if (ch->send->state == SMD_SS_OPENING) {
			ch_set_state(ch, SMD_SS_OPENED);
			ch->notify(ch->priv, SMD_EVENT_OPEN);
		}
		break;
	case SMD_SS_FLUSHING:
	case SMD_SS_RESET:
		/* we should force them to close? */
		break;
	case SMD_SS_CLOSED:
		if (ch->send->state == SMD_SS_OPENED) {
			ch_set_state(ch, SMD_SS_CLOSING);
			ch->notify(ch->priv, SMD_EVENT_CLOSE);
		}
		break;
	}
}

static void handle_smd_irq(struct list_head *list, void (*notify)(void))
{
	unsigned long flags;
	struct smd_channel *ch;
	int do_notify = 0;
	unsigned ch_flags;
	unsigned tmp;

	spin_lock_irqsave(&smd_lock, flags);
	list_for_each_entry(ch, list, ch_list) {
		ch_flags = 0;
		if (ch_is_open(ch)) {
			if (ch->recv->fHEAD) {
				ch->recv->fHEAD = 0;
				ch_flags |= 1;
				do_notify |= 1;
			}
			if (ch->recv->fTAIL) {
				ch->recv->fTAIL = 0;
				ch_flags |= 2;
				do_notify |= 1;
			}
			if (ch->recv->fSTATE) {
				ch->recv->fSTATE = 0;
				ch_flags |= 4;
				do_notify |= 1;
			}
		}
		tmp = ch->recv->state;
		if (tmp != ch->last_state)
			smd_state_change(ch, ch->last_state, tmp);
		if (ch_flags) {
			ch->update_state(ch);
			ch->notify(ch->priv, SMD_EVENT_DATA);
		}
	}
	if (do_notify)
		notify();
	spin_unlock_irqrestore(&smd_lock, flags);
	do_smd_probe();
}

static irqreturn_t smd_modem_irq_handler(int irq, void *data)
{
	handle_smd_irq(&smd_ch_list_modem, notify_modem_smd);
	return IRQ_HANDLED;
}

#if defined(CONFIG_QDSP6)
static irqreturn_t smd_dsp_irq_handler(int irq, void *data)
{
	handle_smd_irq(&smd_ch_list_dsp, notify_dsp_smd);
	return IRQ_HANDLED;
}
#endif

static void smd_fake_irq_handler(unsigned long arg)
{
	handle_smd_irq(&smd_ch_list_modem, notify_modem_smd);
	handle_smd_irq(&smd_ch_list_dsp, notify_dsp_smd);
}

static DECLARE_TASKLET(smd_fake_irq_tasklet, smd_fake_irq_handler, 0);

static inline int smd_need_int(struct smd_channel *ch)
{
	if (ch_is_open(ch)) {
		if (ch->recv->fHEAD || ch->recv->fTAIL || ch->recv->fSTATE)
			return 1;
		if (ch->recv->state != ch->last_state)
			return 1;
	}
	return 0;
}

void smd_sleep_exit(void)
{
	unsigned long flags;
	struct smd_channel *ch;
	int need_int = 0;

	spin_lock_irqsave(&smd_lock, flags);
	list_for_each_entry(ch, &smd_ch_list_modem, ch_list) {
		if (smd_need_int(ch)) {
			need_int = 1;
			break;
		}
	}
	list_for_each_entry(ch, &smd_ch_list_dsp, ch_list) {
		if (smd_need_int(ch)) {
			need_int = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&smd_lock, flags);
	do_smd_probe();

	if (need_int) {
		SMD_DBG("smd_sleep_exit need interrupt\n");
		tasklet_schedule(&smd_fake_irq_tasklet);
	}
}

static int smd_is_packet(struct smd_alloc_elm *alloc_elm)
{
	if (SMD_XFER_TYPE(alloc_elm->type) == 1)
		return 0;
	else if (SMD_XFER_TYPE(alloc_elm->type) == 2)
		return 1;

	/* for cases where xfer type is 0 */
	if (!strncmp(alloc_elm->name, "DAL", 3))
		return 0;

	if (alloc_elm->cid > 4 || alloc_elm->cid == 1)
		return 1;
	else
		return 0;
}

static int smd_stream_write(smd_channel_t *ch, const void *_data, int len)
{
	void *ptr;
	const unsigned char *buf = _data;
	unsigned xfer;
	int orig_len = len;

	SMD_DBG("smd_stream_write() %d -> ch%d\n", len, ch->n);
	if (len < 0)
		return -EINVAL;
	else if (len == 0)
		return 0;

	while ((xfer = ch_write_buffer(ch, &ptr)) != 0) {
		if (!ch_is_open(ch))
			break;
		if (xfer > len)
			xfer = len;
		memcpy(ptr, buf, xfer);
		ch_write_done(ch, xfer);
		len -= xfer;
		buf += xfer;
		if (len == 0)
			break;
	}

	if (orig_len - len)
		ch->notify_other_cpu();

	return orig_len - len;
}

static int smd_packet_write(smd_channel_t *ch, const void *_data, int len)
{
	int ret;
	unsigned hdr[5];

	SMD_DBG("smd_packet_write() %d -> ch%d\n", len, ch->n);
	if (len < 0)
		return -EINVAL;
	else if (len == 0)
		return 0;

	if (smd_stream_write_avail(ch) < (len + SMD_HEADER_SIZE))
		return -ENOMEM;

	hdr[0] = len;
	hdr[1] = hdr[2] = hdr[3] = hdr[4] = 0;


	ret = smd_stream_write(ch, hdr, sizeof(hdr));
	if (ret < 0 || ret != sizeof(hdr)) {
		SMD_DBG("%s failed to write pkt header: "
			"%d returned\n", __func__, ret);
		return -1;
	}


	ret = smd_stream_write(ch, _data, len);
	if (ret < 0 || ret != len) {
		SMD_DBG("%s failed to write pkt data: "
			"%d returned\n", __func__, ret);
		return ret;
	}

	return len;
}

static int smd_stream_read(smd_channel_t *ch, void *data, int len)
{
	int r;

	if (len < 0)
		return -EINVAL;

	r = ch_read(ch, data, len);
	if (r > 0)
		ch->notify_other_cpu();

	return r;
}

static int smd_packet_read(smd_channel_t *ch, void *data, int len)
{
	unsigned long flags;
	int r;

	if (len < 0)
		return -EINVAL;

	if (len > ch->current_packet)
		len = ch->current_packet;

	r = ch_read(ch, data, len);
	if (r > 0)
		ch->notify_other_cpu();

	spin_lock_irqsave(&smd_lock, flags);
	ch->current_packet -= r;
	update_packet_state(ch);
	spin_unlock_irqrestore(&smd_lock, flags);

	return r;
}

static int smd_packet_read_from_cb(smd_channel_t *ch, void *data, int len)
{
	int r;

	if (len < 0)
		return -EINVAL;

	if (len > ch->current_packet)
		len = ch->current_packet;

	r = ch_read(ch, data, len);
	if (r > 0)
		ch->notify_other_cpu();

	ch->current_packet -= r;
	update_packet_state(ch);

	return r;
}

static int smd_alloc_v2(struct smd_channel *ch)
{
	struct smd_shared_v2 *shared2;
	void *buffer;
	unsigned buffer_sz;

	shared2 = smem_alloc(SMEM_SMD_BASE_ID + ch->n, sizeof(*shared2));
	if (!shared2) {
		SMD_INFO("smem_alloc failed ch=%d\n", ch->n);
		return -1;
	}
	buffer = smem_get_entry(SMEM_SMD_FIFO_BASE_ID + ch->n, &buffer_sz);
	if (!buffer) {
		SMD_INFO("smem_get_entry failed \n");
		return -1;
	}

	/* buffer must be a power-of-two size */
	if (buffer_sz & (buffer_sz - 1))
		return -1;

	buffer_sz /= 2;
	ch->send = &shared2->ch0;
	ch->recv = &shared2->ch1;
	ch->send_data = buffer;
	ch->recv_data = buffer + buffer_sz;
	ch->fifo_size = buffer_sz;
	return 0;
}

static int smd_alloc_v1(struct smd_channel *ch)
{
	struct smd_shared_v1 *shared1;
	shared1 = smem_alloc(ID_SMD_CHANNELS + ch->n, sizeof(*shared1));
	if (!shared1) {
		pr_err("smd_alloc_channel() cid %d does not exist\n", ch->n);
		return -1;
	}
	ch->send = &shared1->ch0;
	ch->recv = &shared1->ch1;
	ch->send_data = shared1->data0;
	ch->recv_data = shared1->data1;
	ch->fifo_size = SMD_BUF_SIZE;
	return 0;
}

static int smd_alloc_channel(struct smd_alloc_elm *alloc_elm)
{
	struct smd_channel *ch;

	ch = kzalloc(sizeof(struct smd_channel), GFP_KERNEL);
	if (ch == 0) {
		pr_err("smd_alloc_channel() out of memory\n");
		return -1;
	}
	ch->n = alloc_elm->cid;

	if (smd_alloc_v2(ch) && smd_alloc_v1(ch)) {
		kfree(ch);
		return -1;
	}

	ch->fifo_mask = ch->fifo_size - 1;
	ch->type = SMD_CHANNEL_TYPE(alloc_elm->type);

	if (ch->type == SMD_APPS_MODEM)
		ch->notify_other_cpu = notify_modem_smd;
	else
		ch->notify_other_cpu = notify_dsp_smd;

	if (smd_is_packet(alloc_elm)) {
		ch->read = smd_packet_read;
		ch->write = smd_packet_write;
		ch->read_avail = smd_packet_read_avail;
		ch->write_avail = smd_packet_write_avail;
		ch->update_state = update_packet_state;
		ch->read_from_cb = smd_packet_read_from_cb;
	} else {
		ch->read = smd_stream_read;
		ch->write = smd_stream_write;
		ch->read_avail = smd_stream_read_avail;
		ch->write_avail = smd_stream_write_avail;
		ch->update_state = update_stream_state;
		ch->read_from_cb = smd_stream_read;
	}

	memcpy(ch->name, alloc_elm->name, 20);
	ch->name[19] = 0;

	ch->pdev.name = ch->name;
	ch->pdev.id = ch->type;

	SMD_INFO("smd_alloc_channel() '%s' cid=%d\n",
		 ch->name, ch->n);

	mutex_lock(&smd_creation_mutex);
	list_add(&ch->ch_list, &smd_ch_closed_list);
	mutex_unlock(&smd_creation_mutex);

	platform_device_register(&ch->pdev);
	return 0;
}

static inline void notify_loopback_smd(void)
{
	unsigned long flags;
	struct smd_channel *ch;

	spin_lock_irqsave(&smd_lock, flags);
	list_for_each_entry(ch, &smd_ch_list_loopback, ch_list) {
		ch->notify(ch->priv, SMD_EVENT_DATA);
	}
	spin_unlock_irqrestore(&smd_lock, flags);
}

static int smd_alloc_loopback_channel(void)
{
	static struct smd_half_channel smd_loopback_ctl;
	static char smd_loopback_data[SMD_BUF_SIZE];
	struct smd_channel *ch;

	ch = kzalloc(sizeof(struct smd_channel), GFP_KERNEL);
	if (ch == 0) {
		pr_err("%s: out of memory\n", __func__);
		return -1;
	}
	ch->n = SMD_LOOPBACK_CID;

	ch->send = &smd_loopback_ctl;
	ch->recv = &smd_loopback_ctl;
	ch->send_data = smd_loopback_data;
	ch->recv_data = smd_loopback_data;
	ch->fifo_size = SMD_BUF_SIZE;

	ch->fifo_mask = ch->fifo_size - 1;
	ch->type = SMD_LOOPBACK_TYPE;
	ch->notify_other_cpu = notify_loopback_smd;

	ch->read = smd_stream_read;
	ch->write = smd_stream_write;
	ch->read_avail = smd_stream_read_avail;
	ch->write_avail = smd_stream_write_avail;
	ch->update_state = update_stream_state;
	ch->read_from_cb = smd_stream_read;

	memset(ch->name, 0, 20);
	memcpy(ch->name, "local_loopback", 14);

	ch->pdev.name = ch->name;
	ch->pdev.id = ch->type;

	SMD_INFO("%s: '%s' cid=%d\n", __func__, ch->name, ch->n);

	mutex_lock(&smd_creation_mutex);
	list_add(&ch->ch_list, &smd_ch_closed_list);
	mutex_unlock(&smd_creation_mutex);

	platform_device_register(&ch->pdev);
	return 0;
}

static void do_nothing_notify(void *priv, unsigned flags)
{
}

struct smd_channel *smd_get_channel(const char *name, uint32_t type)
{
	struct smd_channel *ch;

	mutex_lock(&smd_creation_mutex);
	list_for_each_entry(ch, &smd_ch_closed_list, ch_list) {
		if (!strcmp(name, ch->name) &&
			(type == ch->type)) {
			list_del(&ch->ch_list);
			mutex_unlock(&smd_creation_mutex);
			return ch;
		}
	}
	mutex_unlock(&smd_creation_mutex);

	return NULL;
}

int smd_named_open_on_edge(const char *name, uint32_t edge,
			   smd_channel_t **_ch,
			   void *priv, void (*notify)(void *, unsigned))
{
	struct smd_channel *ch;
	unsigned long flags;

	if (smd_initialized == 0) {
		SMD_INFO("smd_open() before smd_init()\n");
		return -ENODEV;
	}

	SMD_DBG("smd_open('%s', %p, %p)\n", name, priv, notify);

	ch = smd_get_channel(name, edge);
	if (!ch)
		return -ENODEV;

	if (notify == 0)
		notify = do_nothing_notify;

	ch->notify = notify;
	ch->current_packet = 0;
	ch->last_state = SMD_SS_CLOSED;
	ch->priv = priv;

	if (edge == SMD_LOOPBACK_TYPE) {
		ch->last_state = SMD_SS_OPENED;
		ch->send->state = SMD_SS_OPENED;
		ch->send->fDSR = 1;
		ch->send->fCTS = 1;
		ch->send->fCD = 1;
	}

	*_ch = ch;

	SMD_DBG("smd_open: opening '%s'\n", ch->name);

	spin_lock_irqsave(&smd_lock, flags);
	if (SMD_CHANNEL_TYPE(ch->type) == SMD_APPS_MODEM)
		list_add(&ch->ch_list, &smd_ch_list_modem);
	else if (SMD_CHANNEL_TYPE(ch->type) == SMD_APPS_QDSP_I)
		list_add(&ch->ch_list, &smd_ch_list_dsp);
	else
		list_add(&ch->ch_list, &smd_ch_list_loopback);

	SMD_DBG("%s: opening ch %d\n", __func__, ch->n);

	if (edge != SMD_LOOPBACK_TYPE)
		smd_state_change(ch, ch->last_state, SMD_SS_OPENING);

	spin_unlock_irqrestore(&smd_lock, flags);

	return 0;
}
EXPORT_SYMBOL(smd_named_open_on_edge);


int smd_open(const char *name, smd_channel_t **_ch,
	     void *priv, void (*notify)(void *, unsigned))
{
	return smd_named_open_on_edge(name, SMD_APPS_MODEM, _ch, priv,
				      notify);
}
EXPORT_SYMBOL(smd_open);

int smd_close(smd_channel_t *ch)
{
	unsigned long flags;

	SMD_INFO("smd_close(%p)\n", ch);

	if (ch == 0)
		return -1;

	spin_lock_irqsave(&smd_lock, flags);
	ch->notify = do_nothing_notify;
	list_del(&ch->ch_list);
	if (ch->n == SMD_LOOPBACK_CID) {
		ch->send->fDSR = 0;
		ch->send->fCTS = 0;
		ch->send->fCD = 0;
		ch->send->state = SMD_SS_CLOSED;
	} else
		ch_set_state(ch, SMD_SS_CLOSED);
	spin_unlock_irqrestore(&smd_lock, flags);

	mutex_lock(&smd_creation_mutex);
	list_add(&ch->ch_list, &smd_ch_closed_list);
	mutex_unlock(&smd_creation_mutex);

	return 0;
}
EXPORT_SYMBOL(smd_close);

int smd_read(smd_channel_t *ch, void *data, int len)
{
	return ch->read(ch, data, len);
}
EXPORT_SYMBOL(smd_read);

int smd_read_from_cb(smd_channel_t *ch, void *data, int len)
{
	return ch->read_from_cb(ch, data, len);
}
EXPORT_SYMBOL(smd_read_from_cb);

int smd_write(smd_channel_t *ch, const void *data, int len)
{
	return ch->write(ch, data, len);
}
EXPORT_SYMBOL(smd_write);

int smd_read_avail(smd_channel_t *ch)
{
	return ch->read_avail(ch);
}
EXPORT_SYMBOL(smd_read_avail);

int smd_write_avail(smd_channel_t *ch)
{
	return ch->write_avail(ch);
}
EXPORT_SYMBOL(smd_write_avail);

int smd_wait_until_readable(smd_channel_t *ch, int bytes)
{
	return -1;
}

int smd_wait_until_writable(smd_channel_t *ch, int bytes)
{
	return -1;
}

int smd_cur_packet_size(smd_channel_t *ch)
{
	return ch->current_packet;
}

int smd_tiocmget(smd_channel_t *ch)
{
	return  (ch->recv->fDSR ? TIOCM_DSR : 0) |
		(ch->recv->fCTS ? TIOCM_CTS : 0) |
		(ch->recv->fCD ? TIOCM_CD : 0) |
		(ch->recv->fRI ? TIOCM_RI : 0) |
		(ch->send->fCTS ? TIOCM_RTS : 0) |
		(ch->send->fDSR ? TIOCM_DTR : 0);
}

int smd_tiocmset(smd_channel_t *ch, unsigned int set, unsigned int clear)
{
	unsigned long flags;

	spin_lock_irqsave(&smd_lock, flags);
	if (set & TIOCM_DTR)
		ch->send->fDSR = 1;

	if (set & TIOCM_RTS)
		ch->send->fCTS = 1;

	if (clear & TIOCM_DTR)
		ch->send->fDSR = 0;

	if (clear & TIOCM_RTS)
		ch->send->fCTS = 0;

	ch->send->fSTATE = 1;
	barrier();
	ch->notify_other_cpu();
	spin_unlock_irqrestore(&smd_lock, flags);

	return 0;
}


/* -------------------------------------------------------------------------- */

void *smem_alloc(unsigned id, unsigned size)
{
	return smem_find(id, size);
}

void *smem_get_entry(unsigned id, unsigned *size)
{
	struct smem_shared *shared = (void *) MSM_SHARED_RAM_BASE;
	struct smem_heap_entry *toc = shared->heap_toc;

	if (id >= SMEM_NUM_ITEMS)
		return 0;

	if (toc[id].allocated) {
		*size = toc[id].size;
		return (void *) (MSM_SHARED_RAM_BASE + toc[id].offset);
	} else {
		*size = 0;
	}

	return 0;
}

void *smem_find(unsigned id, unsigned size_in)
{
	unsigned size;
	void *ptr;

	ptr = smem_get_entry(id, &size);
	if (!ptr)
		return 0;

	size_in = ALIGN(size_in, 8);
	if (size_in != size) {
		pr_err("smem_find(%d, %d): wrong size %d\n",
		       id, size_in, size);
		return 0;
	}

	return ptr;
}

static int smsm_init(void)
{
	struct smem_shared *shared = (void *) MSM_SHARED_RAM_BASE;
	int i;

	if (!smsm_info.state) {
		smsm_info.state = smem_alloc(ID_SHARED_STATE,
					     SMSM_NUM_ENTRIES *
					     sizeof(uint32_t));

		if (smsm_info.state) {
			writel(0, SMSM_STATE_ADDR(SMSM_APPS_STATE));
			if ((shared->version[VERSION_MODEM] >> 16) >= 0xB)
				writel(0, SMSM_STATE_ADDR(SMSM_APPS_DEM_I));
		}
	}

	if (!smsm_info.intr_mask) {
		smsm_info.intr_mask = smem_alloc(SMEM_SMSM_CPU_INTR_MASK,
						 SMSM_NUM_ENTRIES *
						 SMSM_NUM_HOSTS *
						 sizeof(uint32_t));

		if (smsm_info.intr_mask)
			for (i = 0; i < SMSM_NUM_ENTRIES; i++)
				writel(0xffffffff,
				       SMSM_INTR_MASK_ADDR(i, SMSM_APPS));
	}

	if (!smsm_info.intr_mux)
		smsm_info.intr_mux = smem_alloc(SMEM_SMD_SMSM_INTR_MUX,
						SMSM_NUM_INTR_MUX *
						sizeof(uint32_t));

	return 0;
}

void smsm_reset_modem(unsigned mode)
{
	if (mode == SMSM_SYSTEM_DOWNLOAD) {
		mode = SMSM_RESET | SMSM_SYSTEM_DOWNLOAD;
	} else if (mode == SMSM_MODEM_WAIT) {
		mode = SMSM_RESET | SMSM_MODEM_WAIT;
	} else { /* reset_mode is SMSM_RESET or default */
		mode = SMSM_RESET;
	}

	smsm_change_state(SMSM_APPS_STATE, mode, mode);
}
EXPORT_SYMBOL(smsm_reset_modem);

void smsm_reset_modem_cont(void)
{
	unsigned long flags;
	uint32_t state;

	if (!smsm_info.state)
		return;

	spin_lock_irqsave(&smem_lock, flags);
	state = readl(SMSM_STATE_ADDR(SMSM_APPS_STATE)) & ~SMSM_MODEM_WAIT;
	writel(state, SMSM_STATE_ADDR(SMSM_APPS_STATE));
	spin_unlock_irqrestore(&smem_lock, flags);
}
EXPORT_SYMBOL(smsm_reset_modem_cont);

static irqreturn_t smsm_irq_handler(int irq, void *data)
{
	unsigned long flags;
	static uint32_t prev_smem_q6_apps_smsm;
	uint32_t mux_val;

	smsm_init();
	if (irq == INT_ADSP_A11) {
		if (!smsm_info.intr_mux)
			return IRQ_HANDLED;
		mux_val = readl(SMSM_INTR_MUX_ADDR(SMEM_Q6_APPS_SMSM));
		if (mux_val == prev_smem_q6_apps_smsm)
			return IRQ_HANDLED;

		prev_smem_q6_apps_smsm = mux_val;
	}

	spin_lock_irqsave(&smem_lock, flags);
	if (!smsm_info.state) {
		SMSM_INFO("<SM NO STATE>\n");
	} else {
		unsigned old_apps, apps;
		unsigned modm = readl(SMSM_STATE_ADDR(SMSM_MODEM_STATE));

		old_apps = apps = readl(SMSM_STATE_ADDR(SMSM_APPS_STATE));

		SMSM_DBG("<SM %08x %08x>\n", apps, modm);
		if (apps & SMSM_RESET) {
			/* If we get an interrupt and the apps SMSM_RESET
			   bit is already set, the modem is acking the
			   app's reset ack. */
			apps &= ~SMSM_RESET;

			/* Issue a fake irq to handle any
			 * smd state changes during reset
			 */
			smd_fake_irq_handler(0);

			/* queue modem restart notify chain */
			modem_queue_start_reset_notify();

		} else if (modm & SMSM_RESET) {
			apps |= SMSM_RESET;
		} else {
			if (!(apps & SMSM_INIT)) {
				apps |= SMSM_INIT;
				modem_queue_smsm_init_notify();
			}

			if (modm & SMSM_SMDINIT)
				apps |= SMSM_SMDINIT;
			if ((apps & (SMSM_INIT | SMSM_SMDINIT | SMSM_RPCINIT)) ==
				(SMSM_INIT | SMSM_SMDINIT | SMSM_RPCINIT))
				apps |= SMSM_RUN;
		}

		if (old_apps != apps) {
			SMSM_DBG("<SM %08x NOTIFY>\n", apps);
			writel(apps, SMSM_STATE_ADDR(SMSM_APPS_STATE));
			do_smd_probe();
			notify_other_smsm(SMSM_APPS_STATE, (old_apps ^ apps));
		}
	}
	spin_unlock_irqrestore(&smem_lock, flags);
	return IRQ_HANDLED;
}

int smsm_change_intr_mask(uint32_t smsm_entry,
			  uint32_t clear_mask, uint32_t set_mask)
{
	uint32_t  old_mask, new_mask;
	unsigned long flags;

	if (smsm_entry >= SMSM_NUM_ENTRIES) {
		pr_err("smsm_change_state: Invalid entry %d\n",
		       smsm_entry);
		return -EINVAL;
	}

	if (!smsm_info.intr_mask) {
		pr_err("smsm_change_intr_mask <SM NO STATE>\n");
		return -EIO;
	}

	spin_lock_irqsave(&smem_lock, flags);

	old_mask = readl(SMSM_INTR_MASK_ADDR(smsm_entry, SMSM_APPS));
	new_mask = (old_mask & ~clear_mask) | set_mask;
	writel(new_mask, SMSM_INTR_MASK_ADDR(smsm_entry, SMSM_APPS));

	spin_unlock_irqrestore(&smem_lock, flags);

	return 0;
}

int smsm_get_intr_mask(uint32_t smsm_entry, uint32_t *intr_mask)
{
	if (smsm_entry >= SMSM_NUM_ENTRIES) {
		pr_err("smsm_change_state: Invalid entry %d\n",
		       smsm_entry);
		return -EINVAL;
	}

	if (!smsm_info.intr_mask) {
		pr_err("smsm_change_intr_mask <SM NO STATE>\n");
		return -EIO;
	}

	*intr_mask = readl(SMSM_INTR_MASK_ADDR(smsm_entry, SMSM_APPS));
	return 0;
}

int smsm_change_state(uint32_t smsm_entry,
		      uint32_t clear_mask, uint32_t set_mask)
{
	unsigned long flags;
	uint32_t  old_state, new_state;

	if (smsm_entry >= SMSM_NUM_ENTRIES) {
		pr_err("smsm_change_state: Invalid entry %d",
		       smsm_entry);
		return -EINVAL;
	}

	if (!smsm_info.state) {
		pr_err("smsm_change_state <SM NO STATE>\n");
		return -EIO;
	}
	spin_lock_irqsave(&smem_lock, flags);

	old_state = readl(SMSM_STATE_ADDR(smsm_entry));
	new_state = (old_state & ~clear_mask) | set_mask;
	writel(new_state, SMSM_STATE_ADDR(smsm_entry));
	SMSM_DBG("smsm_change_state %x\n", new_state);
	notify_other_smsm(SMSM_APPS_STATE, (old_state ^ new_state));

	spin_unlock_irqrestore(&smem_lock, flags);

	return 0;
}

uint32_t smsm_get_state(uint32_t smsm_entry)
{
	uint32_t rv = 0;

	/* needs interface change to return error code */
	if (smsm_entry >= SMSM_NUM_ENTRIES) {
		pr_err("smsm_change_state: Invalid entry %d",
		       smsm_entry);
		return 0;
	}

	if (!smsm_info.state)
		pr_err("smsm_get_state <SM NO STATE>\n");
	else
		rv = readl(SMSM_STATE_ADDR(smsm_entry));

	return rv;
}

int smd_core_init(void)
{
	int r;
	SMD_INFO("smd_core_init()\n");

	r = request_irq(INT_A9_M2A_0, smd_modem_irq_handler,
			IRQF_TRIGGER_RISING, "smd_dev", 0);
	if (r < 0)
		return r;
	r = enable_irq_wake(INT_A9_M2A_0);
	if (r < 0)
		pr_err("smd_core_init: "
		       "enable_irq_wake failed for INT_A9_M2A_0\n");

	r = request_irq(INT_A9_M2A_5, smsm_irq_handler,
			IRQF_TRIGGER_RISING, "smsm_dev", 0);
	if (r < 0) {
		free_irq(INT_A9_M2A_0, 0);
		return r;
	}
	r = enable_irq_wake(INT_A9_M2A_5);
	if (r < 0)
		pr_err("smd_core_init: "
		       "enable_irq_wake failed for INT_A9_M2A_5\n");

#if defined(CONFIG_QDSP6)
	r = request_irq(INT_ADSP_A11, smd_dsp_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_SHARED, "smd_dev",
			smd_dsp_irq_handler);
	if (r < 0) {
		free_irq(INT_A9_M2A_0, 0);
		free_irq(INT_A9_M2A_5, 0);
		return r;
	}

	r = request_irq(INT_ADSP_A11, smsm_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_SHARED, "smsm_dev",
			smsm_irq_handler);
	if (r < 0) {
		free_irq(INT_A9_M2A_0, 0);
		free_irq(INT_A9_M2A_5, 0);
		free_irq(INT_ADSP_A11, 0);
		return r;
	}

	r = enable_irq_wake(INT_ADSP_A11);
	if (r < 0)
		pr_err("smd_core_init: "
		       "enable_irq_wake failed for INT_ADSP_A11\n");
#endif

	/* we may have missed a signal while booting -- fake
	 * an interrupt to make sure we process any existing
	 * state
	 */
	smsm_irq_handler(0, 0);

	SMD_INFO("smd_core_init() done\n");

	return 0;
}

//SW2-5-1-BH-DbgCfgTool-00+[
static unsigned int msm_poweron_cause;
module_param_named(poweron_cause, msm_poweron_cause, int, S_IRUGO);
//SW2-5-1-BH-DbgCfgTool-00+]

#ifdef CONFIG_FIH_FXX

/* FIH, Debbie Sun, 2009/06/18 { */
/* get share memory command address dynamically */
unsigned int fih_hwid = 0x0;
/* modify new hardware id */
unsigned int fih_orig_hwid = 0x0;
unsigned int fih_mode = 0x0;
/* add for Read modem mode from smem */
unsigned int fih_network_mode = 0x0;

#ifdef POWER_ON_CAUSE_PROC_READ_ENTRY
unsigned int fih_power_on_cause = 0x0;
#endif

/* FIH, Debbie Sun, 2010/05/24 { */
/* get ram info and device name form share memory */
unsigned int fih_dram_magic_number = 0;
char fih_flash_name[32]  = {0};
/* FIH, Debbie Sun, 2010/05/24 } */

/* FIH, JiaHao, 2010/08/04{*/
/* jogball_exist = { (0)=not_exist, (1)=is_exist } */
int fih_jogball_exist = (0);
/* FIH, JiaHao, 2010/08/04 } */

/* switch UART for printk log */
unsigned int fih_host_log_from_uart = 0x0;
unsigned int fih_host_usb_id = 0x0;
unsigned int fih_host_enable_kpd = 0x0;

/* FIH, JiaHao, 2010/08/04 { */
/* init jogball gpio with GPIO_NO_PULL or GPIO_PULL_DOWN by orig_hwid */
void smem_init_jogball_gpio( unsigned int orig_hwid );
/* get orig_hwid from smem_hwid and gpio */
unsigned int smem_get_orig_hwid( unsigned int smem_hwid );
/* get hwid from orig_hwid */
unsigned int smem_get_hwid( unsigned int orig_hwid );
/* return jogball device is exist or not { (0)=not_exist, (1)=is_exist } */
int smem_get_jogball_exist(void);
/* FIH, JiaHao, 2010/08/04 } */

/* FIH, JiaHao, 2010/08/04 { */

/* init jogball gpio with GPIO_NO_PULL or GPIO_PULL_DOWN by orig_hwid */
void smem_init_jogball_gpio( unsigned int orig_hwid )
{
    int pull;

    /* get jogball gpio pin */
    unsigned gpio_neg_x  = (88);
    unsigned gpio_plus_y = (90);
    unsigned gpio_plus_x = (91);
    unsigned gpio_neg_y  = (93);

    /* get pull type : PR1, PR2 */
    switch ( orig_hwid )
    {
        case CMCS_HW_EVB1: // 0x0
        case CMCS_HW_EVB2:    
        case CMCS_HW_EVB3:
        case CMCS_HW_EVB4:
        case CMCS_HW_EVB5:
        case CMCS_ORIG_RTP_PR1:
        case CMCS_ORIG_CTP_PR1: // 0xd
        /* 850 family */
        case CMCS_850_RTP_PR2: // 0x10
        case CMCS_850_CTP_PR2: // 0x17
        /* 900 family */
        case CMCS_900_RTP_PR2: // 0x20
        case CMCS_900_CTP_PR2: // 0x27
            pull = GPIO_NO_PULL;
            break;
        default:
            pull = GPIO_PULL_DOWN;
            break;
    }

    /* get pull type : 7627 */
    if ( orig_hwid >= CMCS_7627_ORIG_EVB1 ) {
        pull = GPIO_NO_PULL;
    }

    /* set gpio pull */
    gpio_tlmm_config(GPIO_CFG(gpio_neg_x,
                              0, /* function */
                              GPIO_INPUT,
                              pull,
                              GPIO_2MA),
                     GPIO_ENABLE);
    gpio_tlmm_config(GPIO_CFG(gpio_plus_y,
                              0, /* function */
                              GPIO_INPUT,
                              pull,
                              GPIO_2MA),
                     GPIO_ENABLE);
    gpio_tlmm_config(GPIO_CFG(gpio_plus_x,
                              0, /* function */
                              GPIO_INPUT,
                              pull,
                              GPIO_2MA),
                     GPIO_ENABLE);
    gpio_tlmm_config(GPIO_CFG(gpio_neg_y,
                              0, /* function */
                              GPIO_INPUT,
                              pull,
                              GPIO_2MA),
                     GPIO_ENABLE);
}

/* return jogball device is exist or not { (0)=not_exist, (1)=is_exist } */
int smem_get_jogball_exist(void)
{
    int RetVal;

    unsigned int orig_hwid;
    int pr1, pr2, pr3;

    /* map to jogball gpio pin */
    unsigned gpio_neg_x  = (88);
    unsigned gpio_plus_y = (90);
    unsigned gpio_plus_x = (91);
    unsigned gpio_neg_y  = (93);

    /* get hardware id */
    orig_hwid = fih_read_orig_hwid_from_smem();

    /* avoid error, initial gpio of jogball first */
    smem_init_jogball_gpio( orig_hwid );

    /* get condition */
    pr1 = ( ( !gpio_get_value(gpio_neg_x)  )||
            ( !gpio_get_value(gpio_plus_x) )||
            ( !gpio_get_value(gpio_plus_y) )||
            ( !gpio_get_value(gpio_neg_y)  ) );
    pr2 = ( !gpio_get_value(gpio_neg_y) );
    pr3 = ( ( !gpio_get_value(gpio_plus_x) )&&
            ( !gpio_get_value(gpio_neg_y)  ) );

    #if (0)
    printk(KERN_INFO "smem_get_jogball_exist() : orig_hwid = 0x%08X\n", orig_hwid);
    printk(KERN_INFO "smem_get_jogball_exist() : gpio_get_value(gpio_neg_x)  = (%d)\n", gpio_get_value(gpio_neg_x) );
    printk(KERN_INFO "smem_get_jogball_exist() : gpio_get_value(gpio_plus_y) = (%d)\n", gpio_get_value(gpio_plus_y) );
    printk(KERN_INFO "smem_get_jogball_exist() : gpio_get_value(gpio_plus_x) = (%d)\n", gpio_get_value(gpio_plus_x) );
    printk(KERN_INFO "smem_get_jogball_exist() : gpio_get_value(gpio_neg_y)  = (%d)\n", gpio_get_value(gpio_neg_y) );
    printk(KERN_INFO "smem_get_jogball_exist() : pr1=(%d), pr2=(%d), pr3=(%d)\n", pr1, pr2, pr3);
    #endif

    switch ( orig_hwid )
    {
        case CMCS_ORIG_RTP_PR1:
        case CMCS_ORIG_CTP_PR1:
            RetVal = pr1;
            break;
        case CMCS_850_RTP_PR2: // F10
        case CMCS_850_RTP_PR3:
        case CMCS_850_RTP_PR4:
        case CMCS_850_RTP_PR5:
        case CMCS_850_RTP_MP1:
        case CMCS_850_RTP_MP2:
        case CMCS_850_RTP_MP3:
        case CMCS_900_RTP_PR2: // F10
        case CMCS_900_RTP_PR3:
        case CMCS_900_RTP_PR4:
        case CMCS_900_RTP_PR5:
        case CMCS_900_RTP_MP1:
        case CMCS_900_RTP_MP2:
        case CMCS_900_RTP_MP3:
        case CMCS_850_CTP_PR2: // F11
        case CMCS_900_CTP_PR2: // F11
        case CMCS_125_CTP_GRE_PR1: // GRE
        case CMCS_125_CTP_GRE_PR2:
        case CMCS_125_CTP_GRE_MP1:
        case CMCS_125_CTP_GRE_MP2:
            RetVal = pr2;
            break;
        case CMCS_850_CTP_PR3: // F11
        case CMCS_850_CTP_PR4:
        case CMCS_850_CTP_PR5:
        case CMCS_850_CTP_MP1:
        case CMCS_850_CTP_MP2:
        case CMCS_850_CTP_MP3:
        case CMCS_900_CTP_PR3: // F11
        case CMCS_900_CTP_PR4:
        case CMCS_900_CTP_PR5:
        case CMCS_900_CTP_MP1:
        case CMCS_900_CTP_MP2:
        case CMCS_900_CTP_MP3:
            RetVal = pr3;
            break;
        case CMCS_145_CTP_PR1: // AWS
            RetVal = (1);
            break;
        default:
            RetVal = (0);
            break;
    }

    #if (0)
    printk(KERN_INFO "smem_get_jogball_exist() : RetVal = (%d)\n", RetVal);
    #endif

    return RetVal;
}

/* correct orig_hwid for F917 */
/* get orig_hwid from smem_hwid and gpio */
unsigned int smem_get_orig_hwid( unsigned int smem_hwid )
{
    unsigned int RetVal = smem_hwid;
    int mask;

    /* map to jogball gpio pin */
    unsigned gpio_neg_x  = (88);
    unsigned gpio_plus_y = (90);
    unsigned gpio_plus_x = (91);
    unsigned gpio_neg_y  = (93);

    #if (0)
    printk(KERN_INFO "smem_get_orig_hwid() : smem_hwid = 0x%X\n", smem_hwid);
    #endif

    /* avoid error, initial gpio of jogball first */
    smem_init_jogball_gpio( smem_hwid );

    mask = 0;
    if ( gpio_get_value(gpio_neg_x)  ) mask |= 8;
    if ( gpio_get_value(gpio_plus_y) ) mask |= 4;
    if ( gpio_get_value(gpio_plus_x) ) mask |= 2;
    if ( gpio_get_value(gpio_neg_y)  ) mask |= 1;

    #if (0)
    printk(KERN_INFO "smem_get_orig_hwid() : mask = (0x%08X)\n", mask );
    #endif

    /*  _0000=0x00,  _0100=0x04,  _1000=0x08,  _1100=0x0C,
     *  _0001=0x01,  _0101=0x05,  _1001=0x09,  _1101=0x0D,
     *  _0010=0x02,  _0110=0x06,  _1010=0x0A,  _1110=0x0E,
     *  _0011=0x03,  _0111=0x07,  _1011=0x0B,  _1111=0x0F,
     */

    switch ( smem_hwid )
    {
        case CMCS_850_CTP_PR3:
            switch ( mask )
            {
                case 0x00000000: RetVal = CMCS_850_CTP_PR3;  break;
                case 0x00000001: RetVal = CMCS_850_CTP_PR3;  break;
                case 0x00000002: RetVal = CMCS_CTP_F917_PR1; break;
                default:         RetVal = CMCS_850_CTP_PR3;
                    printk(KERN_INFO "smem_get_orig_hwid() : No Match GPIO <WARNING>\n");
                    break;
            }
            break;
        case CMCS_900_CTP_PR3:
            switch ( mask )
            {
                case 0x00000002: RetVal = CMCS_CTP_F917_PR1; break;
                default:         RetVal = CMCS_900_CTP_PR3;
                    printk(KERN_INFO "smem_get_orig_hwid() : No Match GPIO <WARNING>\n");
                    break;
            }
            break;
        case CMCS_CTP_F917_PR2:
            switch ( mask )
            {
                case 0x00000002: RetVal = CMCS_CTP_F917_PR2;     break;
                case 0x00000004: RetVal = CMCS_128_4G4G_FAA_PR1; break;
                default:         RetVal = CMCS_CTP_F917_PR2;
                    printk(KERN_INFO "smem_get_orig_hwid() : No Match GPIO <WARNING>\n");
                    break;
            }
        default:
            RetVal = smem_hwid;
            printk(KERN_INFO "smem_get_orig_hwid() : No Mapping GPIO\n");
            break;
    }

    #if (0)
    printk(KERN_INFO "smem_get_orig_hwid() : RetVal = 0x%X\n", RetVal);
    #endif

    return RetVal;
}

/* map orig_hwid to hwid */
unsigned int smem_get_hwid( unsigned int orig_hwid )
{
    unsigned int RetVal = orig_hwid;

    switch ( orig_hwid )
    {
        case CMCS_850_RTP_PR2:
        case CMCS_900_RTP_PR2:
            RetVal = CMCS_RTP_PR2;
            break;
        case CMCS_850_RTP_PR3:
        case CMCS_900_RTP_PR3:
            RetVal = CMCS_RTP_PR3;
            break;
        case CMCS_850_RTP_PR4:
        case CMCS_900_RTP_PR4:
            RetVal = CMCS_RTP_PR4;
            break;
        case CMCS_850_RTP_PR5:
        case CMCS_900_RTP_PR5:
            RetVal = CMCS_RTP_PR5;
            break;
        case CMCS_850_RTP_MP1:
        case CMCS_900_RTP_MP1:
            RetVal = CMCS_RTP_MP1;
            break;
        case CMCS_850_RTP_MP2:
        case CMCS_900_RTP_MP2:
            RetVal = CMCS_RTP_MP2;
            break;
        case CMCS_850_RTP_MP3:
        case CMCS_900_RTP_MP3:
            RetVal = CMCS_RTP_MP3;
            break;
        case CMCS_850_CTP_PR2:
        case CMCS_900_CTP_PR2:
            RetVal = CMCS_CTP_PR2;
            break;
        case CMCS_850_CTP_PR3:
        case CMCS_900_CTP_PR3:
        /* FIH, Debbie Sun, 2009/12/02 { */ 
        /* add for FST */ 
        case CMCS_125_FST_PR1: //0x30
        case CMCS_125_FST_PR2:
        case CMCS_125_FST_MP1:
        case CMCS_128_FST_PR1: //0x40
        case CMCS_128_FST_PR2:
        case CMCS_128_FST_MP1:
        /* FIH, Debbie Sun, 2009/12/02 } */ 
	/* FIH, Debbie, 2010/05/04 { */
        case CMCS_CTP_F917_PR1: //0x50	
        case CMCS_CTP_F917_PR2:
        case CMCS_CTP_F917_PR3:
        case CMCS_CTP_F917_PR4:
        case CMCS_CTP_F917_MP1:
        case CMCS_CTP_F917_MP2:
        case CMCS_CTP_F917_MP3:
	/* FIH, Debbie, 2010/05/04 } */
        case CMCS_145_CTP_PR1:     //FIH, Debbie, 2009/12/16
	/* FIH, Paul Huang, 2010/03/03 { */
        case CMCS_125_CTP_GRE_PR1: //0x60
        case CMCS_125_CTP_GRE_PR2:
        case CMCS_125_CTP_GRE_MP1:
        case CMCS_125_CTP_GRE_MP2:
	/* FIH, Paul Huang, 2010/03/03 } */
	/* FIH, Debbie, 2010/05/04 { */
        case CMCS_125_FA9_PR1: //0x70
        case CMCS_125_FA9_PR2:
        case CMCS_125_FA9_PR3:
        case CMCS_125_FA9_MP1:
	/* FIH, Debbie, 2010/05/04 } */
        /* FIH, Debbie, 2010/05/24 { */
        case CMCS_125_4G4G_FAA_PR1: //0x80
        case CMCS_125_4G4G_FAA_PR2:
        case CMCS_125_4G4G_FAA_PR3:
        case CMCS_125_4G4G_FAA_MP1:
        /* FIH, JiaHao, 2010/07/30 { */
        case CMCS_15_4G4G_FAA_PR2: //0x84
        case CMCS_15_4G4G_FAA_PR3:
        case CMCS_15_4G4G_FAA_MP1:
        /* FIH, JiaHao, 2010/07/30 { */
        case CMCS_128_4G4G_FAA_PR1: //0x87
        case CMCS_128_4G4G_FAA_PR2:
        case CMCS_128_4G4G_FAA_PR3:
        case CMCS_128_4G4G_FAA_MP1:
        /* FIH, Debbie, 2010/05/24 } */
	/* FIH, JiaHao, 2010/07/30 { */
        case CMCS_MQ4_PR0: //0x90
        case CMCS_MQ4_18_PR1:
        case CMCS_MQ4_18_PR2:
        case CMCS_MQ4_18_PR3:
        case CMCS_MQ4_25_PR1:
        case CMCS_MQ4_25_PR2:
        case CMCS_MQ4_25_PR3:
	/* FIH, JiaHao, 2010/07/30 } */
            RetVal = CMCS_CTP_PR3;
            break;
        case CMCS_850_CTP_PR4:
        case CMCS_900_CTP_PR4:
            RetVal = CMCS_CTP_PR4;
            break;
        case CMCS_850_CTP_PR5:
        case CMCS_900_CTP_PR5:
            RetVal = CMCS_CTP_PR5;
            break;
        case CMCS_850_CTP_MP1:
        case CMCS_900_CTP_MP1:
            RetVal = CMCS_CTP_MP1;
            break;
        case CMCS_850_CTP_MP2:
        case CMCS_900_CTP_MP2:
            RetVal = CMCS_CTP_MP2;
            break;
        case CMCS_850_CTP_MP3:
        case CMCS_900_CTP_MP3:
            RetVal = CMCS_CTP_MP3;
            break;
	/* FIH, JiaHao, 2010/10/08 { */
	/* Domino Q */
	case CMCS_DOMINO_Q_W_PR1:
		fih_hwid = CMCS_DOMINO_Q;
		break;
	/* FIH, JiaHao, 2010/10/08 } */
	/* FIH, JiaHao, 2010/10/13 { */
	/* Domino Plus */
	case CMCS_DOMINO_PLUS_EG_PR1:
	case CMCS_DOMINO_PLUS_WG_PR1:
		fih_hwid = CMCS_DOMINO_PLUS;
		break;
	/* FIH, JiaHao, 2010/10/13 } */
	/* FIH, Debbie Sun, 2010/02/02 { */ 
        case CMCS_7627_F913_MP1_W:
        case CMCS_7627_F913_MP1_C_G:    //FIH, Debbie, 2009/12/07
        /* FIH, Debbie Sun, 2010/05/24 { */ 
        case CMCS_7627_F913_MP1_W_4G4G:
        case CMCS_7627_F913_MP1_C_G_4G4G:
        /* FIH, Debbie Sun, 2010/05/24 } */ 
	/* FIH, Henry Juang, 2010/08/26 Added this for FA3 8 layers PCB++{ */
	case CMCS_7627_F913_PCB_W:
	case CMCS_7627_F913_PCB_C_G:
	/* FIH, Henry Juang, 2010/08/26 } --*/
	/* FIH, JiaHao, 2010/10/01 { */
	case CMCS_7627_F913_HAIER_C:
	/* FIH, JiaHao, 2010/10/01 } */
        /* FIH, Debbie, 2010/05/04 { */
        case CMCS_7627_F20_PR1:
        case CMCS_7627_F20_PR2:
        case CMCS_7627_F20_PR3:
        case CMCS_7627_F20_MP1:
        /* FIH, Debbie, 2010/05/04 } */
            RetVal = CMCS_F913_MP1;
            break;
	/* FIH, Debbie Sun, 2010/02/02 } */ 
	/* FIH, JiaHao, 2010/08/23 { */ 
        //[FM6]Terry +++++
        case CMCS_128_FM6_PR1:
            RetVal = CMCS_FM6_PR1;
            break;
        case CMCS_128_FM6_PR2:
        case CMCS_125_FM6_PR2:
            RetVal = CMCS_FM6_PR2;
            break;
        case CMCS_128_FM6_PR3:
        case CMCS_125_FM6_PR3:
        case CMCS_145_FM6_PR3:
            RetVal = CMCS_FM6_PR3;
            break;
        case CMCS_128_FM6_8L:
        case CMCS_125_FM6_8L:
        case CMCS_145_FM6_8L:
            RetVal = CMCS_FM6_8L;
            break;
        case CMCS_128_FM6_MP:
        case CMCS_125_FM6_MP:
            RetVal = CMCS_FM6_MP;
            break;
        //[FM6]Terry -----
        // +++ For FN6, FIH, Paul Huang, 2010,06,18
        case CMCS_FN6_ORIG_PR1:
            RetVal = CMCS_FN6_PR1;
            break;
        case CMCS_FN6_ORIG_PR2:
            RetVal = CMCS_FN6_PR2;
            break;
        case CMCS_FN6_ORIG_PR3:
            RetVal = CMCS_FN6_PR3;
            break;
        case CMCS_FN6_ORIG_MP1:
            RetVal = CMCS_FN6_MP1;
            break;
        // --- For FN6, FIH, Paul Huang, 2010,06,18
	/* FIH, JiaHao, 2010/08/23 } */
        default:
            RetVal = orig_hwid;
            break;
    }

    return RetVal;
}
/* FIH, JiaHao, 2010/08/04 } */

void fih_smem_alloc(void)
{
    struct smem_oem_info *fih_smem_info;

    printk(KERN_INFO "fih_smem_alloc\n");
    fih_smem_info = smem_alloc(SMEM_ID_VENDOR0, sizeof(*fih_smem_info));

    if (!fih_smem_info)
    {
	printk(KERN_ERR "ERROR: unable to read shared memory SMEM_ID_VENDOR0\n");
        return;
    }

    

    fih_mode = (fih_smem_info->keypad_info);
	
	//SW252-rexer-SAX-00+[
	#ifdef CONFIG_FIH_PROJECT_SAX
	fih_orig_hwid = fih_smem_info->hw_id;
	#else
    /* FIH, JiaHao, 2010/08/04 { */
    /* get orig_hwid from smem_hwid and gpio */
    fih_orig_hwid = smem_get_orig_hwid( fih_smem_info->hw_id );
    /* map orig_hwid to hwid */
    fih_hwid = smem_get_hwid( fih_orig_hwid );
    /* FIH, JiaHao, 2010/08/04 } */
    fih_network_mode = (fih_smem_info->network_mode);
	fih_dram_magic_number = (fih_smem_info->dram_info);
    #endif /*CONFIG_FIH_PROJECT_SAX*/
	//SW252-rexer-SAX-00+]

    #ifdef POWER_ON_CAUSE_PROC_READ_ENTRY
    fih_power_on_cause = (fih_smem_info->power_on_cause);
	msm_poweron_cause = fih_power_on_cause; //SW2-5-1-BH-DbgCfgTool-00+[
    #endif 

    /* FIH, Debbie Sun, 2010/05/24 { */
    /* get ram info and device name form share memory */
    memcpy(fih_flash_name, fih_smem_info->flash_name, 32);
 
    /* FIH, Debbie Sun, 2010/05/24 } */	

    /* FIH, JiaHao, 2010/08/04 { */
    /* jogball_exist should be after orig_hwid and hwid set !! */
	//SW252-rexer-SAX-00+[
	#ifdef CONFIG_FIH_PROJECT_SAX
	#else
    fih_jogball_exist = smem_get_jogball_exist();
	#endif
	//SW252-rexer-SAX-00+]
    /* FIH, JiaHao, 2010/08/04 } */

    /* FIH, JiaHao, 2010/08/04 { */
    #if (1)
    printk(KERN_INFO "fih_smem_alloc() : fih_orig_hwid = 0x%X\n",  fih_orig_hwid);
    printk(KERN_INFO "fih_smem_alloc() : fih_hwid = 0x%X\n",  fih_hwid);
    printk(KERN_INFO "fih_smem_alloc() : fih_mode = %d\n",fih_mode);
    printk(KERN_INFO "fih_smem_alloc() : fih_network_mode = %d\n", fih_network_mode);
    printk(KERN_INFO "fih_smem_alloc() : fih_power_on_cause = 0x%X\n", fih_power_on_cause);
    printk(KERN_INFO "fih_smem_alloc() : fih_flash_name = %s\n", fih_flash_name);
    printk(KERN_INFO "fih_smem_alloc() : fih_dram_magic_number = 0x%X\n", fih_dram_magic_number);
    printk(KERN_INFO "fih_smem_alloc() : fih_jogball_exist = %d\n", fih_jogball_exist);
    #endif
    /* FIH, JiaHao, 2010/08/04 } */
}

/* FIH, Debbie, 2009/09/11 { */
/* switch UART for printk log */
void fih_smem_alloc_for_host_used(void)
{
    struct smem_host_oem_info* fih_smem_host_used;

    printk(KERN_INFO "fih_smem_alloc_for_host_used\n");
    fih_smem_host_used = smem_alloc(SMEM_ID_VENDOR2, sizeof(*fih_smem_host_used));

    if ( fih_smem_host_used )
    {
	 fih_host_log_from_uart = (fih_smem_host_used->host_log_from_uart);
	 fih_host_usb_id = (fih_smem_host_used->host_usb_id);
	 //SW252-rexer-SAX-00+[
	 #ifndef CONFIG_FIH_PROJECT_SAX
	 fih_host_enable_kpd = fih_smem_host_used->host_enable_kpd;
	 #endif
	 //SW252-rexer-SAX-00+]
    }

    #if (1)
    printk(KERN_INFO "smd.c, fih_smem_host_used address = 0x%X\n", (unsigned int)fih_smem_host_used);
    printk(KERN_INFO "smd.c, host_usb_id = 0x%X\n", fih_smem_host_used->host_usb_id);
    printk(KERN_INFO "smd.c, host_log_from_uart = 0x%X\n", fih_smem_host_used->host_log_from_uart);
    #endif
}
/* FIH, Debbie, 2009/09/11 } */

/* FIH, JiaHao, 2010/08/04 { */
unsigned int fih_read_orig_hwid_from_smem(void)
{
    return fih_orig_hwid;
}

unsigned int fih_read_hwid_from_smem(void)
{
    return fih_hwid;
}

int fih_read_jogball_exist_from_smem(void)
{
    return fih_jogball_exist;
}
/* FIH, JiaHao, 2010/08/04 } */

unsigned int fih_read_mode_from_smem(void)
{
    return fih_mode;
}

/* add for Read modem mode from smem */
unsigned int fih_read_network_mode_from_smem(void)
{
    return fih_network_mode;
}

#ifdef POWER_ON_CAUSE_PROC_READ_ENTRY
unsigned int fih_read_power_on_cuase_from_smem(void)
{
    return fih_power_on_cause;
}
#endif

/* switch UART for printk log */
unsigned int fih_read_uart_switch_from_smem(void)
{
    return fih_host_log_from_uart;
}

unsigned int fih_read_usb_id_from_smem(void)
{
    return fih_host_usb_id;
}

unsigned int fih_read_kpd_from_smem(void)
{
    return fih_host_enable_kpd;
}

/* FIH, Debbie Sun, 2010/05/24 { */
/* get ram info and device name form share memory */
unsigned int fih_read_dram_info_from_smem(void)
{
    return fih_dram_magic_number;
}

char* fih_read_flash_name_from_smem(void)
{
    return fih_flash_name ;
}
/* FIH, Debbie Sun, 2010/05/24 } */

#endif /* CONFIG_FIH_FXX */

static int __init msm_smd_probe(struct platform_device *pdev)
{
	/* enable smd and smsm info messages */
	msm_smd_debug_mask = 0xc;

	SMD_INFO("smd probe\n");

	INIT_WORK(&probe_work, smd_channel_probe_worker);

	if (smsm_init()) {
		pr_err("smsm_init() failed\n");
		return -1;
	}

	if (smd_core_init()) {
		pr_err("smd_core_init() failed\n");
		return -1;
	}

	smd_initialized = 1;

	smd_alloc_loopback_channel();

	return 0;
}

static struct platform_driver msm_smd_driver = {
	.probe = msm_smd_probe,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init msm_smd_init(void)
{
	return platform_driver_register(&msm_smd_driver);
}

module_init(msm_smd_init);

MODULE_DESCRIPTION("MSM Shared Memory Core");
MODULE_AUTHOR("Brian Swetland <swetland@google.com>");
MODULE_LICENSE("GPL");
