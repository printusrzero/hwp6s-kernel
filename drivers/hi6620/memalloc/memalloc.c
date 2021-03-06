/*
 * Allocate memory blocks
 *
 * Copyright (C) 2012 Google Finland Oy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
--------------------------------------------------------------------------------
--
--  Abstract : Allocate memory blocks
--
------------------------------------------------------------------------------*/

#include <linux/kernel.h>
#include <linux/module.h>
/* needed for __init,__exit directives */
#include <linux/init.h>
/* needed for remap_page_range */
#include <linux/mm.h>
/* obviously, for kmalloc */
#include <linux/slab.h>
/* for struct file_operations, register_chrdev() */
#include <linux/fs.h>
/* standard error codes */
#include <linux/errno.h>
/* this header files wraps some common module-space operations ...
   here we use mem_map_reserve() macro */

#include <linux/platform_device.h>
#include <linux/kdev_t.h>

/* for semaphore */
#include <linux/semaphore.h>
#include <linux/dma-contiguous.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/list.h>
/* for current pid */
#include <linux/sched.h>
#include <linux/types.h>

/* Our header */
#include "memalloc.h"
#include "hisi_memheap.h"
#include <linux/android_pmem.h>
#include <linux/hisi_ion.h>

#include "mach/platform.h"
#include "mach/hardware.h"
#include "mach/early-debug.h"
#include "mach/hisi_mem.h"

/* add for video mntn, modifier: w00228831, begin */
#include "video_mntn_kernel.h"
/* add for video mntn, modifier: w00228831, end */

#include <hsad/config_mgr.h>

static int memalloc_major;	/* dynamic */
static ulong g_baseaddr;
static int open_count = 0;

static int mem_maxsize = 0; /* max cma mem */
static int mem_cursize = 0; /* currnt cma mem */

static struct semaphore mem_sem = __SEMAPHORE_INITIALIZER(mem_sem, 1);

#if 0
/* Added MMAP method */
static int memalloc_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long start = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	int retval = 0;

	PDEBUG(KERN_ERR "%s(%d):start:0x%x; size:%d\n",
	       __FUNCTION__, __LINE__, start, size);

	/* make buffers write-thru cacheable */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (remap_pfn_range(vma, start, vma->vm_pgoff, size, vma->vm_page_prot)) {
		retval = -ENOBUFS;
	}

	PDEBUG(KERN_ERR "%s(%d):QUIT:%d\n", __FUNCTION__, __LINE__, retval);
	return retval;
}
#endif

static long memalloc_ioctl(struct file *filp,
			   unsigned int cmd, unsigned long arg)
{
	int err = 0;
    unsigned int cfg_size;

	/*printk("ioctl cmd 0x%08x\n", cmd); */

	if (filp == NULL || arg == 0) {
		return -EFAULT;
	}

	if (!get_cma_type()) {
		if (true != get_hw_config_int("memory_config/codec", &cfg_size, NULL))
    	{
    	    printk(KERN_ERR "get_hw_config_int failed\n");
    	    return -EFAULT;
    	}
	}
	else {
	    cfg_size = CODEC_FIX_SIZE/1024;
	}

	if (0 == cfg_size) {
	    printk(KERN_ERR "codec memory size is 0.\n");
	    return -EFAULT;
	}


	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != MEMALLOC_IOC_MAGIC) {
		printk("memalloc_ioctl enter 1\n");
		return -ENOTTY;
	}
	if (_IOC_NR(cmd) > MEMALLOC_IOC_MAXNR) {
		printk("memalloc_ioctl enter 2\n");
		return -ENOTTY;
	}

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	if (err) {
		printk("memalloc_ioctl enter 3\n");
		return -EFAULT;
	}
	switch (cmd) {
	case MEMALLOC_IOCHARDRESET:{
			printk("%s(%d):HARDRESET\n", __FUNCTION__, __LINE__);

			destroy_list();
			initial_list(PAGE_ALIGN(cfg_size * SZ_1K), g_baseaddr);	/*init head */

			break;
		}
	case MEMALLOC_IOCXGETBUFFER:{
			int result;
			MemallocParams memparams;
			ulong ba;

			/*printk("GETBUFFER\n"); */
			/*spin_lock(&mem_lock); */
			down(&mem_sem);

			if (__copy_from_user
			    (&memparams, (const void *)arg,
			     sizeof(memparams))) {
				up(&mem_sem);
				printk(KERN_ERR "%s(%d):ioctl error\n",
				       __FUNCTION__, __LINE__);
				return -EFAULT;
			}

            /*add by daipeihua*/
            mntn_print_log(EN_ID_MNTN_BLVIDEO_DRV_MEMALLOC, EN_VIDEO_LOG_LEVLE_DEBUG, "memalloc: GETBUFFER:%u",memparams.size);

			if (get_cma_type()) {
				struct page *cma_pages;

                if ((mem_cursize + memparams.size) > mem_maxsize) {
                    up(&mem_sem);
               	    printk(KERN_ERR "---out-of-memory\n");
                    return -EFAULT;
            	}
                cma_pages = dma_alloc_from_contiguous(&codec_cma_dev, (PAGE_ALIGN(memparams.size)>>PAGE_SHIFT), 0);
                if(cma_pages== NULL)
                {
                     printk(KERN_ERR "[wuhuihui] memalloc Fail to allocate buffer\n");
                     ba = 0;
                } else {
                     ba = page_to_phys(cma_pages);
			         mod_cma_state(PAGE_ALIGN(memparams.size), NR_USED_CODEC_CMA);
                }
			} else {
				ba = memory_alloc(memparams.size);
			}
			printk(KERN_INFO "memalloc: GETBUFFER: phys = 0x%lx, size = %u\n", ba, memparams.size);
			if (!ba) {
				/*spin_unlock(&mem_lock); */
				/*up(&mem_sem);*/
				memparams.busAddress = 0;

				printk(KERN_ERR "out-of-memory\n");
				result = -ENOMEM;
			} else {	/*normal return */
				memparams.busAddress = ba;

                if (get_cma_type()) {
				    mem_cursize = mem_cursize + memparams.size;
				}
				result = 0;
			}

			if (__copy_to_user
			    ((void *)arg, &memparams, sizeof(memparams))) {
				up(&mem_sem);
				printk(KERN_ERR "%s(%d):ioctl error\n",
				       __FUNCTION__, __LINE__);
				return -EFAULT;
			}

			up(&mem_sem);

			return result;
		}

	case MEMALLOC_IOCSFREEBUFFER:{
			/*printk("%s(%d):FREEBUFFER\n",__FUNCTION__,__LINE__); */
			down(&mem_sem);

			if (get_cma_type()) {
				MemallocParams memparams;
				if (__copy_from_user
                           (&memparams, (const void *)arg,
							sizeof(memparams))) {
					up(&mem_sem);
					printk(KERN_ERR "%s(%d):ioctl error\n",
					__FUNCTION__, __LINE__);
					return -EFAULT;
				}

				dma_release_from_contiguous(&codec_cma_dev, phys_to_page(memparams.busAddress), (PAGE_ALIGN(memparams.size)>>PAGE_SHIFT));
			    mod_cma_state(-PAGE_ALIGN(memparams.size), NR_USED_CODEC_CMA);
                printk(KERN_INFO "memalloc: FREEBUFFER: phys = 0x%x, size = %u\n", memparams.busAddress, memparams.size);
                               mem_cursize = mem_cursize - memparams.size;
			} else {
				unsigned long busaddr;

				__get_user(busaddr, (unsigned long *)arg);
				memory_free(busaddr);
			}

			up(&mem_sem);
			return 0;
		}

    case MEMALLOC_IOCXGETSTATICBUFFER:{
			int result;
			MemallocParams memparams;
			ulong ba;

			down(&mem_sem);

			if (__copy_from_user
			    (&memparams, (const void *)arg,
			     sizeof(memparams))) {
				up(&mem_sem);
				printk(KERN_ERR "%s(%d):ioctl error\n",
				       __FUNCTION__, __LINE__);
				return -EFAULT;
			}

            /*add by daipeihua*/
            mntn_print_log(EN_ID_MNTN_BLVIDEO_DRV_MEMALLOC, EN_VIDEO_LOG_LEVLE_DEBUG, "memalloc: GETCMABUFFER:%u",memparams.size);

			ba = memory_alloc(memparams.size);
			printk(KERN_INFO "memalloc: GETCMABUFFER: phys = 0x%lx, size = %u\n", ba, memparams.size);
			if (!ba) {
				memparams.busAddress = 0;
				printk(KERN_ERR "out-of-memory\n");
				result = -ENOMEM;
			} else {	/*normal return */
				memparams.busAddress = ba;
				result = 0;
			}

			if (__copy_to_user
			    ((void *)arg, &memparams, sizeof(memparams))) {
				up(&mem_sem);
				printk(KERN_ERR "%s(%d):ioctl error\n",
				       __FUNCTION__, __LINE__);
				return -EFAULT;
			}

			up(&mem_sem);

			return result;
		}

    case MEMALLOC_IOCXFREESTATICBUFFER:{
			unsigned long busaddr;

			printk(KERN_INFO "memalloc: FREECMABUFFER\n");
			down(&mem_sem);

			__get_user(busaddr, (unsigned long *)arg);
			memory_free(busaddr);

			up(&mem_sem);
			return 0;
		}

	default:
		return -EFAULT;
	}
	return 0;
}

static int memalloc_open(struct inode *inode, struct file *filp)
{
	open_count++;
	PDEBUG("dev opened\n");
	return 0;
}

static int memalloc_release(struct inode *inode, struct file *filp)
{
	if (--open_count <= 0) {
		free_allocated_memory();
		open_count = 0;
	}
	PDEBUG("dev closed\n");
	return 0;
}

/* VFS methods */
static struct file_operations memalloc_fops = {
//mmap:
//	memalloc_mmap,
open:
	memalloc_open,
release:
	memalloc_release,
unlocked_ioctl:
	memalloc_ioctl,
};

static struct device_attribute memalloc_attrs[] = {
	__ATTR(name, S_IRUGO, NULL, NULL),
	__ATTR(index, S_IRUGO, NULL, NULL),
	__ATTR_NULL
};

static int memalloc_suspend(struct device *dev, pm_message_t state)
{
	printk("memalloc_suspend \n");
	return 0;
}

static int memalloc_resume(struct device *dev)
{
	printk("memalloc_resume \n");
	return 0;
}

static struct class memalloc_class = {
	.name = "memalloc",
	.dev_attrs = memalloc_attrs,
	.suspend = memalloc_suspend,
	.resume = memalloc_resume,
};

struct device g_dev;
int __init memalloc_init(void)
{
	int result;

    unsigned int cfg_size;
	unsigned int cma_size;

	PDEBUG("module init\n");

	if (!get_cma_type()) {
		if (true != get_hw_config_int("memory_config/codec", &cfg_size, NULL))
    	{
    	    printk(KERN_ERR "get_hw_config_int failed\n");
    	    return -EFAULT;
    	}
	}
	else {

		if (true != get_hw_config_int("memory_config/codec", &cma_size, NULL))
    	{
    	    printk(KERN_ERR "get_hw_config_int failed\n");
    	    return -EFAULT;
    	}
		cma_size -= (CODEC_DEC_SIZE/1024);
		mem_maxsize = PAGE_ALIGN(cma_size * SZ_1K);
		cfg_size = (CODEC_FIX_SIZE/1024);
	}

	if (0 == cfg_size) {
	    printk(KERN_ERR "codec memory size is 0.\n");
	    return -EFAULT;
	}

	result = register_chrdev(memalloc_major, "memalloc", &memalloc_fops);
	if (result < 0) {
		PDEBUG("memalloc: unable to get major %d\n", memalloc_major);
		goto err;
	} else if (result != 0) {	/* this is for dynamic major */
		memalloc_major = result;
	}

	memset(&g_dev, 0, sizeof(g_dev));

	class_register(&memalloc_class);

	g_dev.class = &memalloc_class;
	dev_set_name(&g_dev, "%s", "memalloc");
	g_dev.devt = MKDEV(memalloc_major, 0);
	result = device_register(&g_dev);

	printk("memalloc_major is %d\n", memalloc_major);

	g_baseaddr = hisi_reserved_codec_phymem;

	/*add by daipeihua*/
	printk(KERN_INFO "memalloc: hisi_reserved_codec_phymem = 0x%lx codec cfg_size = %u",
	       hisi_reserved_codec_phymem, PAGE_ALIGN(cfg_size * SZ_1K));

	if (INIT_FAILED == initial_list(PAGE_ALIGN(cfg_size * SZ_1K), g_baseaddr)) {
		printk("memalloc: initial memory list failed\n");
		goto err;
	}

	return 0;

err:
	PDEBUG("memalloc: module not inserted\n");
	unregister_chrdev(memalloc_major, "memalloc");
	return result;
}

void __exit memalloc_cleanup(void)
{

	PDEBUG("clenup called\n");

	unregister_chrdev(memalloc_major, "memalloc");
	destroy_list();

	PDEBUG("memalloc: module removed\n");

	return;
}

module_init(memalloc_init);
module_exit(memalloc_cleanup);

/* module description */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("hisilicon");
MODULE_DESCRIPTION("VDEC memory allocation");
