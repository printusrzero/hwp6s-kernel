#include <linux/module.h>
#include <linux/init.h>
#include <wlioctl.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <hw_wifi.h>
#include <wl_cfg80211.h>

/* Customized Locale table : OPTIONAL feature */
const struct cntry_locales_custom hw_translate_custom_table[] = {
/* Table should be filled out based on custom platform regulatory requirement */
	/* Table should be filled out based
	on custom platform regulatory requirement */
	{"",   "XZ", 11},  /* Universal if Country code is unknown or empty */
	{"IR", "CN", 0},  // add by huawei
	{"CU", "CN", 0},  // add by huawei
	{"KP", "CN", 0},   // add by huawei
	{"PK", "CN", 0},  // add by huawei
	{"SD", "CN", 0},  //add by huawei
	{"SY", "CN", 0},    //add by huawei
	{"AE", "AE", 1},
	{"AR", "AR", 1},
	{"AT", "AT", 1},
	{"AU", "AU", 2},
	{"BE", "BE", 1},
	{"BG", "BG", 1},
	{"BN", "BN", 1},
	{"CA", "CA", 2},
	{"CH", "CH", 1},
	{"CY", "CY", 1},
	{"CZ", "CZ", 1},
	{"DE", "DE", 3},
	{"DK", "DK", 1},
	{"EE", "EE", 1},
	{"ES", "ES", 1},
	{"FI", "FI", 1},
	{"FR", "FR", 1},
	{"GB", "GB", 1},
	{"GR", "GR", 1},
	{"HR", "HR", 1},
	{"HU", "HU", 1},
	{"IE", "IE", 1},
	{"IS", "IS", 1},
	{"IT", "IT", 1},
	{"JP", "JP", 5},
	{"KR", "KR", 24},
	{"KW", "KW", 1},
	{"LI", "LI", 1},
	{"LT", "LT", 1},
	{"LU", "LU", 1},
	{"LV", "LV", 1},
	{"MA", "MA", 1},
	{"MT", "MT", 1},
	{"MX", "MX", 1},
	{"NL", "NL", 1},
	{"NO", "NO", 1},
	{"PL", "PL", 1},
	{"PT", "PT", 1},
	{"PY", "PY", 1},
	{"RO", "RO", 1},
	{"RU", "RU", 5},
	{"SE", "SE", 1},
	{"SG", "SG", 4},
	{"SI", "SI", 1},
	{"SK", "SK", 1},
	{"TR", "TR", 7},
	{"TW", "TW", 2},
	{"US", "US", 46},
};


/* Customized Locale convertor
*  input : ISO 3166-1 country abbreviation
*  output: customized cspec
*/
void get_customized_country_code_for_hw(char *country_iso_code, wl_country_t *cspec)
{
	int size, i;

	printk(KERN_ERR "enter : %s.\n", __FUNCTION__);
	size = ARRAYSIZE(hw_translate_custom_table);

	if (cspec == 0)
		 return;

	if (size == 0)
		 return;

	printk(KERN_ERR "input country code: %s.\n", country_iso_code);
	for (i = 0; i < size; i++) {
		if (strcmp(country_iso_code, hw_translate_custom_table[i].iso_abbrev) == 0) {
			memcpy(cspec->ccode, hw_translate_custom_table[i].custom_locale, WLC_CNTRY_BUF_SZ);
			memcpy(cspec->country_abbrev, hw_translate_custom_table[i].custom_locale, WLC_CNTRY_BUF_SZ);
			cspec->rev = hw_translate_custom_table[i].custom_locale_rev;
			printk(KERN_ERR "output country code: %s, ver: %d.\n", cspec->ccode, cspec->rev);
			return;
		}
	}
	
	return;
}

#ifdef HW_PATCH_DISABLE_TCP_TIMESTAMPS
/*  sysctl_tcp_timestamps is defined in net/ipv4/tcp_input.c
 *  here to check wlan0 network interface and
 *  when wlan0 is connected, try to disable tcp_timestamps by set it to 0.
 *  and when wlan0 is disconnected, restore to enable tcp_timestamps.
 */
static int hw_dhd_wan0_connected = 0; /* 1: wlan0 connected*/
extern int sysctl_tcp_timestamps;
void hw_set_connect_status(struct net_device *ndev, int status)
{
    struct wireless_dev *wdev = NULL;

    if (NULL == ndev) {
        printk("interface is null, skip set status.\n");
        return;
    }

    wdev = ndev_to_wdev(ndev);
    if (NL80211_IFTYPE_STATION != wdev->iftype) {
        printk("interface type is %d, skip set sta status.\n", wdev->iftype);
        return;
    }

    hw_dhd_wan0_connected = status;
    /* if wlan0 disconnect, and tcp_ts is 0, restore it to 1 */
    if (0 == hw_dhd_wan0_connected && 0 == sysctl_tcp_timestamps) {
        sysctl_tcp_timestamps = 1;
        printk("wlan0 disconnected, restore tcp_timestamps.\n");
    }
}
#endif

/* this function should only be called when tcp_timestamps err in ipv4/tcp_input.c */
void hw_dhd_check_and_disable_timestamps(void)
{
#ifdef HW_PATCH_DISABLE_TCP_TIMESTAMPS
    /* disable tcp_timestamps to 0 only when wlan0 connected.*/
    printk("check wlan0 connect status = %d.\n", hw_dhd_wan0_connected);
    if (hw_dhd_wan0_connected) {
        sysctl_tcp_timestamps = 0;
        printk("wlan0 connected, disable tcp_timestamps.\n");
    }
#endif
}

