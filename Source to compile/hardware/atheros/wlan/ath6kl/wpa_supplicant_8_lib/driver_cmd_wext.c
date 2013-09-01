/*
 * Driver interaction with extended Linux Wireless Extensions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 */

#include "includes.h"
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <net/if.h>

#include "linux/wireless.h"
#include "common.h"
#include "driver.h"
#include "eloop.h"
#include "priv_netlink.h"
#include "driver_wext.h"
#include "ieee802_11_defs.h"
#include "wpa_common.h"
#include "wpa_ctrl.h"
#include "wpa_supplicant_i.h"
#include "config.h"
#include "linux_ioctl.h"
#include "scan.h"

#include "driver_cmd_wext.h"
#ifdef ANDROID
#include "android_drv.h"
#endif /* ANDROID */

#define RSSI_CMD			"RSSI"
#define LINKSPEED_CMD			"LINKSPEED"

/**
 * wpa_driver_wext_set_scan_timeout - Set scan timeout to report scan completion
 * @priv:  Pointer to private wext data from wpa_driver_wext_init()
 *
 * This function can be used to set registered timeout when starting a scan to
 * generate a scan completed event if the driver does not report this.
 */
static void wpa_driver_wext_set_scan_timeout(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	int timeout = 10; /* In case scan A and B bands it can be long */

	/* Not all drivers generate "scan completed" wireless event, so try to
	 * read results after a timeout. */
	if (drv->scan_complete_events) {
	/*
	 * The driver seems to deliver SIOCGIWSCAN events to notify
	 * when scan is complete, so use longer timeout to avoid race
	 * conditions with scanning and following association request.
	 */
		timeout = 30;
	}
	wpa_printf(MSG_DEBUG, "Scan requested - scan timeout %d seconds",
		   timeout);
	eloop_cancel_timeout(wpa_driver_wext_scan_timeout, drv, drv->ctx);
	eloop_register_timeout(timeout, 0, wpa_driver_wext_scan_timeout, drv,
			       drv->ctx);
}

/**
 * wpa_driver_wext_combo_scan - Request the driver to initiate combo scan
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @params: Scan parameters
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_combo_scan(void *priv, struct wpa_driver_scan_params *params)
{
	char buf[WEXT_CSCAN_BUF_LEN];
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret, bp;
	unsigned i;
        const u8 *ssid=(params->ssids[0]).ssid;
        size_t ssid_len=(params->ssids[0]).ssid_len;

	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(drv->ctx);

	if (!drv->driver_is_started) {
		wpa_printf(MSG_DEBUG, "%s: Driver stopped", __func__);
		return 0;
	}

	wpa_printf(MSG_DEBUG, "%s: Start", __func__);
	struct iw_scan_req req;
	int scan_probe_flag = 0;

	if (ssid_len > IW_ESSID_MAX_SIZE) {
		wpa_printf(MSG_DEBUG, "%s: too long SSID (%lu)",
			   __FUNCTION__, (unsigned long) ssid_len);
		return -1;
	}

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
#ifdef ANDROID
	if (wpa_s->prev_scan_ssid != WILDCARD_SSID_SCAN) {
		scan_probe_flag = wpa_s->prev_scan_ssid->scan_ssid;
	}
	wpa_printf(MSG_DEBUG, "%s: specific scan = %d", __func__,
		(scan_probe_flag && (ssid && ssid_len)) ? 1 : 0);
	if (scan_probe_flag && (ssid && ssid_len)) {
#else
	if (ssid && ssid_len) {
#endif
		os_memset(&req, 0, sizeof(req));
		req.essid_len = ssid_len;
		req.bssid.sa_family = ARPHRD_ETHER;
		os_memset(req.bssid.sa_data, 0xff, ETH_ALEN);
		os_memcpy(req.essid, ssid, ssid_len);
		iwr.u.data.pointer = (caddr_t) &req;
		iwr.u.data.length = sizeof(req);
		iwr.u.data.flags = IW_SCAN_THIS_ESSID;
	}

        ret = ioctl(drv->ioctl_sock, SIOCSIWSCAN, &iwr);
	if (ret < 0) {
		wpa_printf(MSG_ERROR, "ioctl[SIOCSIWSCAN] ret = ",ret);
		ret = -1;
	}

	return ret;
}


int wpa_driver_wext_driver_cmd( void *priv, char *cmd, char *buf, size_t buf_len )
{
    struct wpa_driver_wext_data *drv = priv;
    struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(drv->ctx);
    struct iwreq iwr;
    int ret = 0, flags;

    wpa_printf(MSG_DEBUG, "%s %s len = %d", __func__, cmd, buf_len);

    if (!drv->driver_is_started && (os_strcasecmp(cmd, "START") != 0)) {
        wpa_printf(MSG_ERROR,"WEXT: Driver not initialized yet");
        return -1;
    }
    if (os_strcasecmp(cmd, "start") == 0) {
        wpa_printf(MSG_DEBUG,"Start command");
        return -1;
    }
    if (os_strcasecmp(cmd, "macaddr") == 0) {
        struct ifreq ifr;
        os_memset(&ifr, 0, sizeof(ifr));
        os_strncpy(ifr.ifr_name, drv->ifname, IFNAMSIZ);
        if (ioctl(drv->ioctl_sock, SIOCGIFHWADDR, &ifr) < 0) {
            perror("ioctl[SIOCGIFHWADDR]");
            ret = -1;
        } else {
            u8 *macaddr = (u8 *) ifr.ifr_hwaddr.sa_data;
            ret = snprintf(buf, buf_len, "Macaddr = " MACSTR "\n",
                    MAC2STR(macaddr));
        }
        return ret;
    }
    else if (os_strcasecmp(cmd, "scan-passive") == 0) {
        wpa_printf(MSG_DEBUG,"Scan Passive command");
        return 0;
    }
    else if (os_strcasecmp(cmd, "SETBAND") == 0) {
        wpa_printf(MSG_DEBUG,"Setband command");
        ret=0;
        return 0;
    }
    else if (os_strcasecmp(cmd, "scan-active") == 0) {
        wpa_printf(MSG_DEBUG,"Scan Active command");
        ret=0;
        return 0;
    }
    else if (os_strcasecmp(cmd, "linkspeed") == 0) {
        struct iwreq wrq;
        unsigned int linkspeed;
        os_strncpy(wrq.ifr_name, drv->ifname, IFNAMSIZ);
        if (ioctl(drv->ioctl_sock, SIOCGIWRATE, &wrq) < 0) {
            perror("ioctl[SIOCGIWRATE]");
            ret = -1;
        } else {
            linkspeed = wrq.u.bitrate.value / 1000000;
            ret = snprintf(buf, buf_len, "LinkSpeed %d\n", linkspeed);
            wpa_printf(MSG_DEBUG, "[REPLY]: %s", buf);
        }
        return 0;
    }
    else if (os_strncasecmp(cmd, "scan-channels", 13) == 0) {
    }
    else if (os_strncasecmp(cmd, "rssi", 4) == 0) {
        /* Matches both rssi and rssi-approx */
        struct iwreq wrq;
        struct iw_statistics stats;
        signed int rssi;

        wrq.u.data.pointer = (caddr_t) &stats;
        wrq.u.data.length = sizeof(stats);
        wrq.u.data.flags = 1; /* Clear updated flag */
        strncpy(wrq.ifr_name, drv->ifname, IFNAMSIZ);


        if (ioctl(drv->ioctl_sock, SIOCGIWSTATS, &wrq) < 0) {
            perror("ioctl[SIOCGIWSTATS]");
            ret = -1;
        } else {
            if (stats.qual.updated & IW_QUAL_DBM) {
                /* Values in dBm, stored in u8 with range 63 : -192 */
                rssi = ( stats.qual.level > 63 ) ?
                    stats.qual.level - 0x100 :
                    stats.qual.level;
            } else {
                rssi = stats.qual.level;
            }
            if (wpa_s->conf->ssid->ssid_len != 0 && wpa_s->conf->ssid->ssid_len < buf_len) {
                os_memcpy((void *) buf, (void *) (wpa_s->conf->ssid->ssid),
                        wpa_s->conf->ssid->ssid_len );
                ret = wpa_s->conf->ssid->ssid_len;
                ret += snprintf(&buf[ret], buf_len-ret,
                        " rssi %d\n", rssi);
                wpa_printf(MSG_DEBUG, "[REPLY]: %s", buf);
                if (ret < (int)buf_len) {
                    return ret;
                }
            } else {
                ret = -1;
            }

        }

    }
    else if (os_strncasecmp(cmd, "powermode", 9) == 0) {
    }
    else if (os_strncasecmp(cmd, "getpower", 8) == 0) {
    }
    else if (os_strncasecmp(cmd, "get-rts-threshold", 17) == 0) {
        struct iwreq wrq;
        unsigned int rtsThreshold;

        strncpy(wrq.ifr_name, drv->ifname, IFNAMSIZ);

        if (ioctl(drv->ioctl_sock, SIOCGIWRTS, &wrq) < 0) {
            perror("ioctl[SIOCGIWRTS]");
            ret = -1;
        } else {
            rtsThreshold = wrq.u.rts.value;
            wpa_printf(MSG_DEBUG,"Get RTS Threshold command = %d",
                    rtsThreshold);
            ret = snprintf(buf, buf_len, "rts-threshold = %u\n",
                    rtsThreshold);
            if (ret < (int)buf_len) {
                return ret;
            }
        }
    }
    else if (os_strncasecmp(cmd, "set-rts-threshold", 17) == 0) {
        struct iwreq wrq;
        unsigned int rtsThreshold;
        char *cp = cmd + 17;
        char *endp;

        strncpy(wrq.ifr_name, drv->ifname, IFNAMSIZ);

        if (*cp != '\0') {
            rtsThreshold = (unsigned int)strtol(cp, &endp, 0);
            if (endp != cp) {
                wrq.u.rts.value = rtsThreshold;
                wrq.u.rts.fixed = 1;
                wrq.u.rts.disabled = 0;

                if (ioctl(drv->ioctl_sock, SIOCSIWRTS, &wrq) < 0) {
                    perror("ioctl[SIOCGIWRTS]");
                    ret = -1;
                } else {
                    rtsThreshold = wrq.u.rts.value;
                    wpa_printf(MSG_DEBUG,"Set RTS Threshold command = %d", rtsThreshold);
                    ret = 0;
                }
            }
        }
    }
    else if (os_strcasecmp(cmd, "btcoexscan-start") == 0) {
    }
    else if (os_strcasecmp(cmd, "btcoexscan-stop") == 0) {
    }
    else if (os_strcasecmp(cmd, "rxfilter-start") == 0) {
        wpa_printf(MSG_DEBUG,"Rx Data Filter Start command");
    }
    else if (os_strcasecmp(cmd, "rxfilter-stop") == 0) {
        wpa_printf(MSG_DEBUG,"Rx Data Filter Stop command");
    }
    else if (os_strcasecmp(cmd, "rxfilter-statistics") == 0) {
    }
    else if (os_strncasecmp(cmd, "rxfilter-add", 12) == 0 ) {
    }
    else if (os_strncasecmp(cmd, "rxfilter-remove",15) == 0) {
    }
    else if (os_strcasecmp(cmd, "snr") == 0) {
        struct iwreq wrq;
        struct iw_statistics stats;
        int snr, rssi, noise;

        wrq.u.data.pointer = (caddr_t) &stats;
        wrq.u.data.length = sizeof(stats);
        wrq.u.data.flags = 1; /* Clear updated flag */
        strncpy(wrq.ifr_name, drv->ifname, IFNAMSIZ);

        if (ioctl(drv->ioctl_sock, SIOCGIWSTATS, &wrq) < 0) {
            perror("ioctl[SIOCGIWSTATS]");
            ret = -1;
        } else {
            if (stats.qual.updated & IW_QUAL_DBM) {
                /* Values in dBm, stored in u8 with range 63 : -192 */
                rssi = ( stats.qual.level > 63 ) ?
                    stats.qual.level - 0x100 :
                    stats.qual.level;
                noise = ( stats.qual.noise > 63 ) ?
                    stats.qual.noise - 0x100 :
                    stats.qual.noise;
            } else {
                rssi = stats.qual.level;
                noise = stats.qual.noise;
            }

            snr = rssi - noise;

            ret = snprintf(buf, buf_len, "snr = %u\n", (unsigned int)snr);
            if (ret < (int)buf_len) {
                return ret;
            }
        }
    }
    else if (os_strncasecmp(cmd, "btcoexmode", 10) == 0) {
    }
    else if( os_strcasecmp(cmd, "btcoexstat") == 0 ) {
    }
    else {
        wpa_printf(MSG_DEBUG,"Unsupported command");

    }
    return ret;
}

int wpa_driver_signal_poll(void *priv, struct wpa_signal_info *si)
{
	char buf[MAX_DRV_CMD_SIZE];
	struct wpa_driver_wext_data *drv = priv;
	char *prssi;
	int res;

	os_memset(si, 0, sizeof(*si));
	res = wpa_driver_wext_driver_cmd(priv, RSSI_CMD, buf, sizeof(buf));
	/* Answer: SSID rssi -Val */
	if (res < 0)
		return res;
	prssi = strcasestr(buf, RSSI_CMD);
	if (!prssi)
		return -1;
	si->current_signal = atoi(prssi + strlen(RSSI_CMD) + 1);

	res = wpa_driver_wext_driver_cmd(priv, LINKSPEED_CMD, buf, sizeof(buf));
	/* Answer: LinkSpeed Val */
	if (res < 0)
		return res;
	si->current_txrate = atoi(buf + strlen(LINKSPEED_CMD) + 1) * 1000;

	return 0;
}
