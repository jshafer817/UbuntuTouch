/*
  USB Driver for GSM modems

  Copyright (C) 2005  Matthias Urlichs <smurf@smurf.noris.de>

  This driver is free software; you can redistribute it and/or modify
  it under the terms of Version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  Portions copied from the Keyspan driver by Hugh Blemings <hugh@blemings.org>

  History: see the git log.

  Work sponsored by: Sigos GmbH, Germany <info@sigos.de>

  This driver exists because the "normal" serial driver doesn't work too well
  with GSM modems. Issues:
  - data loss -- one single Receive URB is not nearly enough
  - nonstandard flow (Option devices) control
  - controlling the baud rate doesn't make sense

  This driver is named "option" because the most common device it's
  used for is a PC-Card (with an internal OHCI-USB interface, behind
  which the GSM interface sits), made by Option Inc.

  Some of the "one port" devices actually exhibit multiple USB instances
  on the USB bus. This is not a bug, these ports are used for different
  device features.
*/

#define DRIVER_VERSION "v0.7.4"
#define DRIVER_AUTHOR "Matthias Urlichs <smurf@smurf.noris.de>"
#define DRIVER_DESC "USB Driver for GSM modems with Selective Suspend"

#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/usb.h>
#include <linux/usb/serial.h>
#include <linux/slab.h>


#ifdef GM_DEBUG
#define gmdbg(msg...) printk(msg)
#else
#define gmdbg(msg...) do { } while(0)
#endif

/* Function prototypes */
static int  option_probe(struct usb_serial *serial,
			const struct usb_device_id *id);
static int  option_open(struct tty_struct *tty, struct usb_serial_port *port);
static void option_close(struct usb_serial_port *port);
static void option_dtr_rts(struct usb_serial_port *port, int on);

static int  option_startup(struct usb_serial *serial);
static void option_disconnect(struct usb_serial *serial);
static void option_release(struct usb_serial *serial);
static int  option_write_room(struct tty_struct *tty);

static void option_instat_callback(struct urb *urb);

static int option_write(struct tty_struct *tty, struct usb_serial_port *port,
			const unsigned char *buf, int count);
static int  option_chars_in_buffer(struct tty_struct *tty);
static void option_set_termios(struct tty_struct *tty,
			struct usb_serial_port *port, struct ktermios *old);
static int  option_tiocmget(struct tty_struct *tty, struct file *file);
static int  option_tiocmset(struct tty_struct *tty, struct file *file,
				unsigned int set, unsigned int clear);
static int  option_send_setup(struct usb_serial_port *port);
#ifdef CONFIG_PM
static int  option_suspend(struct usb_serial *serial, pm_message_t message);
static int  option_resume(struct usb_serial *serial);
#endif

/* Vendor and product IDs */
#define OPTION_VENDOR_ID			0x0AF0
#define OPTION_PRODUCT_COLT			0x5000
#define OPTION_PRODUCT_RICOLA			0x6000
#define OPTION_PRODUCT_RICOLA_LIGHT		0x6100
#define OPTION_PRODUCT_RICOLA_QUAD		0x6200
#define OPTION_PRODUCT_RICOLA_QUAD_LIGHT	0x6300
#define OPTION_PRODUCT_RICOLA_NDIS		0x6050
#define OPTION_PRODUCT_RICOLA_NDIS_LIGHT	0x6150
#define OPTION_PRODUCT_RICOLA_NDIS_QUAD		0x6250
#define OPTION_PRODUCT_RICOLA_NDIS_QUAD_LIGHT	0x6350
#define OPTION_PRODUCT_COBRA			0x6500
#define OPTION_PRODUCT_COBRA_BUS		0x6501
#define OPTION_PRODUCT_VIPER			0x6600
#define OPTION_PRODUCT_VIPER_BUS		0x6601
#define OPTION_PRODUCT_GT_MAX_READY		0x6701
#define OPTION_PRODUCT_FUJI_MODEM_LIGHT		0x6721
#define OPTION_PRODUCT_FUJI_MODEM_GT		0x6741
#define OPTION_PRODUCT_FUJI_MODEM_EX		0x6761
#define OPTION_PRODUCT_KOI_MODEM		0x6800
#define OPTION_PRODUCT_SCORPION_MODEM		0x6901
#define OPTION_PRODUCT_ETNA_MODEM		0x7001
#define OPTION_PRODUCT_ETNA_MODEM_LITE		0x7021
#define OPTION_PRODUCT_ETNA_MODEM_GT		0x7041
#define OPTION_PRODUCT_ETNA_MODEM_EX		0x7061
#define OPTION_PRODUCT_ETNA_KOI_MODEM		0x7100
#define OPTION_PRODUCT_GTM380_MODEM		0x7201

#define HUAWEI_VENDOR_ID			0x12D1
#define HUAWEI_PRODUCT_E600			0x1001
#define HUAWEI_PRODUCT_E220			0x1003
#define HUAWEI_PRODUCT_E220BIS			0x1004
#define HUAWEI_PRODUCT_E1401			0x1401
#define HUAWEI_PRODUCT_E1402			0x1402
#define HUAWEI_PRODUCT_E1403			0x1403
#define HUAWEI_PRODUCT_E1404			0x1404
#define HUAWEI_PRODUCT_E1405			0x1405
#define HUAWEI_PRODUCT_E1406			0x1406
#define HUAWEI_PRODUCT_E1407			0x1407
#define HUAWEI_PRODUCT_E1408			0x1408
#define HUAWEI_PRODUCT_E1409			0x1409
#define HUAWEI_PRODUCT_E140A			0x140A
#define HUAWEI_PRODUCT_E140B			0x140B
#define HUAWEI_PRODUCT_E140C			0x140C
#define HUAWEI_PRODUCT_E140D			0x140D
#define HUAWEI_PRODUCT_E140E			0x140E
#define HUAWEI_PRODUCT_E140F			0x140F
#define HUAWEI_PRODUCT_E1410			0x1410
#define HUAWEI_PRODUCT_E1411			0x1411
#define HUAWEI_PRODUCT_E1412			0x1412
#define HUAWEI_PRODUCT_E1413			0x1413
#define HUAWEI_PRODUCT_E1414			0x1414
#define HUAWEI_PRODUCT_E1415			0x1415
#define HUAWEI_PRODUCT_E1416			0x1416
#define HUAWEI_PRODUCT_E1417			0x1417
#define HUAWEI_PRODUCT_E1418			0x1418
#define HUAWEI_PRODUCT_E1419			0x1419
#define HUAWEI_PRODUCT_E141A			0x141A
#define HUAWEI_PRODUCT_E141B			0x141B
#define HUAWEI_PRODUCT_E141C			0x141C
#define HUAWEI_PRODUCT_E141D			0x141D
#define HUAWEI_PRODUCT_E141E			0x141E
#define HUAWEI_PRODUCT_E141F			0x141F
#define HUAWEI_PRODUCT_E1420			0x1420
#define HUAWEI_PRODUCT_E1421			0x1421
#define HUAWEI_PRODUCT_E1422			0x1422
#define HUAWEI_PRODUCT_E1423			0x1423
#define HUAWEI_PRODUCT_E1424			0x1424
#define HUAWEI_PRODUCT_E1425			0x1425
#define HUAWEI_PRODUCT_E1426			0x1426
#define HUAWEI_PRODUCT_E1427			0x1427
#define HUAWEI_PRODUCT_E1428			0x1428
#define HUAWEI_PRODUCT_E1429			0x1429
#define HUAWEI_PRODUCT_E142A			0x142A
#define HUAWEI_PRODUCT_E142B			0x142B
#define HUAWEI_PRODUCT_E142C			0x142C
#define HUAWEI_PRODUCT_E142D			0x142D
#define HUAWEI_PRODUCT_E142E			0x142E
#define HUAWEI_PRODUCT_E142F			0x142F
#define HUAWEI_PRODUCT_E1430			0x1430
#define HUAWEI_PRODUCT_E1431			0x1431
#define HUAWEI_PRODUCT_E1432			0x1432
#define HUAWEI_PRODUCT_E1433			0x1433
#define HUAWEI_PRODUCT_E1434			0x1434
#define HUAWEI_PRODUCT_E1435			0x1435
#define HUAWEI_PRODUCT_E1436			0x1436
#define HUAWEI_PRODUCT_E1437			0x1437
#define HUAWEI_PRODUCT_E1438			0x1438
#define HUAWEI_PRODUCT_E1439			0x1439
#define HUAWEI_PRODUCT_E143A			0x143A
#define HUAWEI_PRODUCT_E143B			0x143B
#define HUAWEI_PRODUCT_E143C			0x143C
#define HUAWEI_PRODUCT_E143D			0x143D
#define HUAWEI_PRODUCT_E143E			0x143E
#define HUAWEI_PRODUCT_E143F			0x143F
#define HUAWEI_PRODUCT_E14AC			0x14AC

#define QUANTA_VENDOR_ID			0x0408
#define QUANTA_PRODUCT_Q101			0xEA02
#define QUANTA_PRODUCT_Q111			0xEA03
#define QUANTA_PRODUCT_GLX			0xEA04
#define QUANTA_PRODUCT_GKE			0xEA05
#define QUANTA_PRODUCT_GLE			0xEA06

#define NOVATELWIRELESS_VENDOR_ID		0x1410

/* YISO PRODUCTS */

#define YISO_VENDOR_ID				0x0EAB
#define YISO_PRODUCT_U893			0xC893

/* MERLIN EVDO PRODUCTS */
#define NOVATELWIRELESS_PRODUCT_V640		0x1100
#define NOVATELWIRELESS_PRODUCT_V620		0x1110
#define NOVATELWIRELESS_PRODUCT_V740		0x1120
#define NOVATELWIRELESS_PRODUCT_V720		0x1130

/* MERLIN HSDPA/HSPA PRODUCTS */
#define NOVATELWIRELESS_PRODUCT_U730		0x1400
#define NOVATELWIRELESS_PRODUCT_U740		0x1410
#define NOVATELWIRELESS_PRODUCT_U870		0x1420
#define NOVATELWIRELESS_PRODUCT_XU870		0x1430
#define NOVATELWIRELESS_PRODUCT_X950D		0x1450

/* EXPEDITE PRODUCTS */
#define NOVATELWIRELESS_PRODUCT_EV620		0x2100
#define NOVATELWIRELESS_PRODUCT_ES720		0x2110
#define NOVATELWIRELESS_PRODUCT_E725		0x2120
#define NOVATELWIRELESS_PRODUCT_ES620		0x2130
#define NOVATELWIRELESS_PRODUCT_EU730		0x2400
#define NOVATELWIRELESS_PRODUCT_EU740		0x2410
#define NOVATELWIRELESS_PRODUCT_EU870D		0x2420

/* OVATION PRODUCTS */
#define NOVATELWIRELESS_PRODUCT_MC727		0x4100
#define NOVATELWIRELESS_PRODUCT_MC950D		0x4400
#define NOVATELWIRELESS_PRODUCT_U727		0x5010
#define NOVATELWIRELESS_PRODUCT_MC727_NEW	0x5100
#define NOVATELWIRELESS_PRODUCT_MC760		0x6000
#define NOVATELWIRELESS_PRODUCT_OVMC760		0x6002

/* FUTURE NOVATEL PRODUCTS */
#define NOVATELWIRELESS_PRODUCT_EVDO_HIGHSPEED	0X6001
#define NOVATELWIRELESS_PRODUCT_HSPA_FULLSPEED	0X7000
#define NOVATELWIRELESS_PRODUCT_HSPA_HIGHSPEED	0X7001
#define NOVATELWIRELESS_PRODUCT_EVDO_EMBEDDED_FULLSPEED	0X8000
#define NOVATELWIRELESS_PRODUCT_EVDO_EMBEDDED_HIGHSPEED	0X8001
#define NOVATELWIRELESS_PRODUCT_HSPA_EMBEDDED_FULLSPEED	0X9000
#define NOVATELWIRELESS_PRODUCT_HSPA_EMBEDDED_HIGHSPEED	0X9001
#define NOVATELWIRELESS_PRODUCT_GLOBAL		0XA001

/* AMOI PRODUCTS */
#define AMOI_VENDOR_ID				0x1614
#define AMOI_PRODUCT_H01			0x0800
#define AMOI_PRODUCT_H01A			0x7002
#define AMOI_PRODUCT_H02			0x0802

#define DELL_VENDOR_ID				0x413C

/* Dell modems */
#define DELL_PRODUCT_5700_MINICARD		0x8114
#define DELL_PRODUCT_5500_MINICARD		0x8115
#define DELL_PRODUCT_5505_MINICARD		0x8116
#define DELL_PRODUCT_5700_EXPRESSCARD		0x8117
#define DELL_PRODUCT_5510_EXPRESSCARD		0x8118

#define DELL_PRODUCT_5700_MINICARD_SPRINT	0x8128
#define DELL_PRODUCT_5700_MINICARD_TELUS	0x8129

#define DELL_PRODUCT_5720_MINICARD_VZW		0x8133
#define DELL_PRODUCT_5720_MINICARD_SPRINT	0x8134
#define DELL_PRODUCT_5720_MINICARD_TELUS	0x8135
#define DELL_PRODUCT_5520_MINICARD_CINGULAR	0x8136
#define DELL_PRODUCT_5520_MINICARD_GENERIC_L	0x8137
#define DELL_PRODUCT_5520_MINICARD_GENERIC_I	0x8138

#define DELL_PRODUCT_5730_MINICARD_SPRINT	0x8180
#define DELL_PRODUCT_5730_MINICARD_TELUS	0x8181
#define DELL_PRODUCT_5730_MINICARD_VZW		0x8182

#define KYOCERA_VENDOR_ID			0x0c88
#define KYOCERA_PRODUCT_KPC650			0x17da
#define KYOCERA_PRODUCT_KPC680			0x180a

#define ANYDATA_VENDOR_ID			0x16d5
#define ANYDATA_PRODUCT_ADU_620UW		0x6202
#define ANYDATA_PRODUCT_ADU_E100A		0x6501
#define ANYDATA_PRODUCT_ADU_500A		0x6502

#define AXESSTEL_VENDOR_ID			0x1726
#define AXESSTEL_PRODUCT_MV110H			0x1000

#define BANDRICH_VENDOR_ID			0x1A8D
#define BANDRICH_PRODUCT_C100_1			0x1002
#define BANDRICH_PRODUCT_C100_2			0x1003
#define BANDRICH_PRODUCT_1004			0x1004
#define BANDRICH_PRODUCT_1005			0x1005
#define BANDRICH_PRODUCT_1006			0x1006
#define BANDRICH_PRODUCT_1007			0x1007
#define BANDRICH_PRODUCT_1008			0x1008
#define BANDRICH_PRODUCT_1009			0x1009
#define BANDRICH_PRODUCT_100A			0x100a

#define BANDRICH_PRODUCT_100B			0x100b
#define BANDRICH_PRODUCT_100C			0x100c
#define BANDRICH_PRODUCT_100D			0x100d
#define BANDRICH_PRODUCT_100E			0x100e

#define BANDRICH_PRODUCT_100F			0x100f
#define BANDRICH_PRODUCT_1010			0x1010
#define BANDRICH_PRODUCT_1011			0x1011
#define BANDRICH_PRODUCT_1012			0x1012

#define AMOI_VENDOR_ID			0x1614
#define AMOI_PRODUCT_9508			0x0800

#define QUALCOMM_VENDOR_ID			0x05C6

#define MAXON_VENDOR_ID				0x16d8

#define TELIT_VENDOR_ID				0x1bc7
#define TELIT_PRODUCT_UC864E			0x1003
#define TELIT_PRODUCT_UC864G			0x1004

/* ZTE PRODUCTS */
#define ZTE_VENDOR_ID				0x19d2
#define ZTE_PRODUCT_MF622			0x0001
#define ZTE_PRODUCT_MF628			0x0015
#define ZTE_PRODUCT_MF626			0x0031
#define ZTE_PRODUCT_CDMA_TECH			0xfffe
#define ZTE_PRODUCT_AC8710			0xfff1
#define ZTE_PRODUCT_AC2726			0xfff5

#define BENQ_VENDOR_ID				0x04a5
#define BENQ_PRODUCT_H10			0x4068

#define DLINK_VENDOR_ID				0x1186
#define DLINK_PRODUCT_DWM_652			0x3e04
#define DLINK_PRODUCT_DWM_652_U5		0xce16

#define QISDA_VENDOR_ID				0x1da5
#define QISDA_PRODUCT_H21_4512			0x4512
#define QISDA_PRODUCT_H21_4523			0x4523
#define QISDA_PRODUCT_H20_4515			0x4515
#define QISDA_PRODUCT_H20_4519			0x4519

/* TLAYTECH PRODUCTS */
#define TLAYTECH_VENDOR_ID			0x20B9
#define TLAYTECH_PRODUCT_TEU800			0x1682

/* TOSHIBA PRODUCTS */
#define TOSHIBA_VENDOR_ID			0x0930
#define TOSHIBA_PRODUCT_HSDPA_MINICARD		0x1302
#define TOSHIBA_PRODUCT_G450			0x0d45

#define ALINK_VENDOR_ID				0x1e0e
#define ALINK_PRODUCT_3GU			0x9200

/* ALCATEL PRODUCTS */
#define ALCATEL_VENDOR_ID			0x1bbb
#define ALCATEL_PRODUCT_X060S			0x0000

/* Airplus products */
#define AIRPLUS_VENDOR_ID			0x1011
#define AIRPLUS_PRODUCT_MCD650			0x3198

/* 4G Systems products */
#define FOUR_G_SYSTEMS_VENDOR_ID		0x1c9e
#define FOUR_G_SYSTEMS_PRODUCT_W14		0x9603

/* Haier products */
#define HAIER_VENDOR_ID				0x201e
#define HAIER_PRODUCT_CE100			0x2009

static struct usb_device_id option_ids[] = {
	{ USB_DEVICE_AND_INTERFACE_INFO(HUAWEI_VENDOR_ID, HUAWEI_PRODUCT_E1404, 0xff, 0xff, 0xff) },
	{ } /* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, option_ids);

static struct usb_driver option_driver = {
	.name       = "option_huawei",
	.probe      = usb_serial_probe,
	.disconnect = usb_serial_disconnect,
#ifdef CONFIG_PM
	.suspend    = usb_serial_suspend,
	.resume     = usb_serial_resume,
	.supports_autosuspend =	1,
#endif
	.id_table   = option_ids,
	.no_dynamic_id = 	1,
};

/* The card has three separate interfaces, which the serial driver
 * recognizes separately, thus num_port=1.
 */

static struct usb_serial_driver option_1port_device = {
	.driver = {
		.owner =	THIS_MODULE,
		.name =		"option1_huawei",
	},
	.description       = "Huawei GSM modem (1-port)",
	.usb_driver        = &option_driver,
	.id_table          = option_ids,
	.num_ports         = 1,
	.probe             = option_probe,
	.open              = option_open,
	.close             = option_close,
	.dtr_rts	   = option_dtr_rts,
	.write             = option_write,
	.write_room        = option_write_room,
	.chars_in_buffer   = option_chars_in_buffer,
	.set_termios       = option_set_termios,
	.tiocmget          = option_tiocmget,
	.tiocmset          = option_tiocmset,
	.attach            = option_startup,
	.disconnect        = option_disconnect,
	.release           = option_release,
	.read_int_callback = option_instat_callback,
#ifdef CONFIG_PM
	.suspend           = option_suspend,
	.resume            = option_resume,
#endif
};

static int debug;

/* per port private data */

#define N_IN_URB 4
#define N_OUT_URB 4
#define IN_BUFLEN 4096
#define OUT_BUFLEN 4096

struct option_intf_private {
	spinlock_t susp_lock;
	unsigned int suspended:1;
	int in_flight;
};

struct option_port_private {
	/* Input endpoints and buffer for this port */
	struct urb *in_urbs[N_IN_URB];
	u8 *in_buffer[N_IN_URB];
	/* Output endpoints and buffer for this port */
	struct urb *out_urbs[N_OUT_URB];
	u8 *out_buffer[N_OUT_URB];
	unsigned long out_busy;		/* Bit vector of URBs in use */
	int opened;
	struct usb_anchor delayed;

	/* Settings for the port */
	int rts_state;	/* Handshaking pins (outputs) */
	int dtr_state;
	int cts_state;	/* Handshaking pins (inputs) */
	int dsr_state;
	int dcd_state;
	int ri_state;

	unsigned long tx_start_time[N_OUT_URB];
};
/*Begin: Declared the global variables for selective suspend feature by fangxiaozhi*/
static struct delayed_work *hw_suspend_wq = NULL;
static struct usb_device   *hw_udev = NULL;

static void option_suspend_check_work(struct work_struct *work);
/*End: Declared the global variables for selective suspend feature by fangxiaozhi*/

/* Functions used by new usb-serial code. */
static int __init option_init(void)
{
	int retval;
	retval = usb_serial_register(&option_1port_device);
	if (retval)
		goto failed_1port_device_register;
	retval = usb_register(&option_driver);
	if (retval)
		goto failed_driver_register;

	gmdbg(KERN_INFO KBUILD_MODNAME ": " DRIVER_VERSION ":"
	       DRIVER_DESC "\n");

	return 0;

failed_driver_register:
	usb_serial_deregister(&option_1port_device);
failed_1port_device_register:
	return retval;
}

static void __exit option_exit(void)
{
	usb_deregister(&option_driver);
	usb_serial_deregister(&option_1port_device);
}

module_init(option_init);
module_exit(option_exit);

static int option_probe(struct usb_serial *serial,
			const struct usb_device_id *id)
{
	struct option_intf_private *data;
	/* D-Link DWM 652 still exposes CD-Rom emulation interface in modem mode */
	if (serial->dev->descriptor.idVendor == DLINK_VENDOR_ID &&
		serial->dev->descriptor.idProduct == DLINK_PRODUCT_DWM_652 &&
		serial->interface->cur_altsetting->desc.bInterfaceClass == 0x8)
		return -ENODEV;

	/*Begin: Added by fangxiaozhi to activate the CDC interface 20101220*/
	if (serial->dev->descriptor.idVendor == HUAWEI_VENDOR_ID
 		&& (serial->dev->descriptor.idProduct == HUAWEI_PRODUCT_E1404
		  || serial->dev->descriptor.idProduct == HUAWEI_PRODUCT_E140C)
		&& serial->interface->cur_altsetting->desc.bInterfaceNumber==5) {

		char szdata[2] = {0xFE, 0x01};
		printk("option_probe InterfaceNumber 5 enter\n");
		usb_control_msg(serial->dev, usb_sndctrlpipe(serial->dev, 0), 0xFE,
			0x40, 0x00, serial->interface->cur_altsetting->desc.bInterfaceNumber,
			szdata, 0x02, 1000);
		return -ENODEV;
	}

	/*End: Added by fangxiaozhi to activate the CDC interface 20101220*/
	data = serial->private = kzalloc(sizeof(struct option_intf_private), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	spin_lock_init(&data->susp_lock);


	/*Begin: Added for selective suspend by fangxiaozhi*/
	if (serial->dev->descriptor.idVendor == HUAWEI_VENDOR_ID) {
		if (!hw_udev) {
			hw_udev = interface_to_usbdev(serial->interface);
			device_init_wakeup(&hw_udev->dev, 1);
			usb_enable_autosuspend(hw_udev);
		}
		if (hw_udev == serial->dev &&
			!hw_suspend_wq) {
			hw_suspend_wq = kmalloc(sizeof(struct delayed_work), GFP_KERNEL);
			if (!hw_suspend_wq) {
				 gmdbg(KERN_ERR "fxz-%s:Alloc memroy failed for hw_suspend_wq!\n", __func__);
				 return -ENOMEM;
			}
			INIT_DELAYED_WORK(hw_suspend_wq, option_suspend_check_work);
		} else {
			if (hw_udev == serial->dev) {
				schedule_delayed_work(hw_suspend_wq, 10 * HZ);
			}
		}
	}

	/*End: Added for selective suspend by fangxiaozhi*/
	return 0;
}

static void option_set_termios(struct tty_struct *tty,
		struct usb_serial_port *port, struct ktermios *old_termios)
{
	dbg("%s", __func__);
	/* Doesn't support option setting */
	tty_termios_copy_hw(tty->termios, old_termios);
	option_send_setup(port);
}

static int option_tiocmget(struct tty_struct *tty, struct file *file)
{
	struct usb_serial_port *port = tty->driver_data;
	unsigned int value;
	struct option_port_private *portdata;

	portdata = usb_get_serial_port_data(port);

	value = ((portdata->rts_state) ? TIOCM_RTS : 0) |
		((portdata->dtr_state) ? TIOCM_DTR : 0) |
		((portdata->cts_state) ? TIOCM_CTS : 0) |
		((portdata->dsr_state) ? TIOCM_DSR : 0) |
		((portdata->dcd_state) ? TIOCM_CAR : 0) |
		((portdata->ri_state) ? TIOCM_RNG : 0);

	return value;
}

static int option_tiocmset(struct tty_struct *tty, struct file *file,
			unsigned int set, unsigned int clear)
{
	struct usb_serial_port *port = tty->driver_data;
	struct option_port_private *portdata;

	portdata = usb_get_serial_port_data(port);

	/* FIXME: what locks portdata fields ? */
	if (set & TIOCM_RTS)
		portdata->rts_state = 1;
	if (set & TIOCM_DTR)
		portdata->dtr_state = 1;

	if (clear & TIOCM_RTS)
		portdata->rts_state = 0;
	if (clear & TIOCM_DTR)
		portdata->dtr_state = 0;
	return option_send_setup(port);
}

/* Write */
static int option_write(struct tty_struct *tty, struct usb_serial_port *port,
			const unsigned char *buf, int count)
{
	struct option_port_private *portdata;
	struct option_intf_private *intfdata;
	int i;
	int left, todo;
	struct urb *this_urb = NULL; /* spurious */
	int err;
	unsigned long flags;

	portdata = usb_get_serial_port_data(port);
	intfdata = port->serial->private;

	dbg("%s: write (%d chars)", __func__, count);
	gmdbg(KERN_ERR"fxz-%s: write (%d chars)\n", __func__, count);

	i = 0;
	left = count;
	for (i = 0; left > 0 && i < N_OUT_URB; i++) {
		todo = left;
		if (todo > OUT_BUFLEN)
			todo = OUT_BUFLEN;

		this_urb = portdata->out_urbs[i];
		if (test_and_set_bit(i, &portdata->out_busy)) {
			if (time_before(jiffies,
					portdata->tx_start_time[i] + 10 * HZ))
				continue;
			usb_unlink_urb(this_urb);
			continue;
		}
		dbg("%s: endpoint %d buf %d", __func__,
			usb_pipeendpoint(this_urb->pipe), i);

		err = usb_autopm_get_interface_async(port->serial->interface);
		if (err < 0)
			break;

		/* send the data */
		memcpy(this_urb->transfer_buffer, buf, todo);
		this_urb->transfer_buffer_length = todo;

		spin_lock_irqsave(&intfdata->susp_lock, flags);
		if (intfdata->suspended) {
			usb_anchor_urb(this_urb, &portdata->delayed);
			spin_unlock_irqrestore(&intfdata->susp_lock, flags);
		} else {
			intfdata->in_flight++;
			spin_unlock_irqrestore(&intfdata->susp_lock, flags);
			err = usb_submit_urb(this_urb, GFP_ATOMIC);
			if (err) {
				dbg("usb_submit_urb %p (write bulk) failed "
					"(%d)", this_urb, err);
				clear_bit(i, &portdata->out_busy);
				spin_lock_irqsave(&intfdata->susp_lock, flags);
				intfdata->in_flight--;
				spin_unlock_irqrestore(&intfdata->susp_lock, flags);
				continue;
			}
		}

		portdata->tx_start_time[i] = jiffies;
		buf += todo;
		left -= todo;
	}
	/*Begin: Added for selective suspend by fangxiaozhi*/
	usb_autopm_put_interface_async(port->serial->interface);
	/*End: Added for selective suspend by fangxiaozhi*/

	count -= left;
	dbg("%s: wrote (did %d)", __func__, count);
	gmdbg(KERN_ERR"fxz-%s: wrote (did %d)\n", __func__, count);
	return count;
}

static void option_indat_callback(struct urb *urb)
{
	int err;
	int endpoint;
	struct usb_serial_port *port;
	struct tty_struct *tty;
	unsigned char *data = urb->transfer_buffer;
	int status = urb->status;

	dbg("%s: %p", __func__, urb);

	endpoint = usb_pipeendpoint(urb->pipe);
	port =  urb->context;

	if (status) {
		pr_err("%s: nonzero status: %d on endpoint %02x.",
		    __func__, status, endpoint);
	} else {
		tty = tty_port_tty_get(&port->port);
		if (urb->actual_length) {
			tty_buffer_request_room(tty, urb->actual_length);
			tty_insert_flip_string(tty, data, urb->actual_length);
			tty_flip_buffer_push(tty);
		} else
			dbg("%s: empty read urb received", __func__);
		tty_kref_put(tty);

		/* Resubmit urb so we continue receiving */
		if (port->port.count && status != -ESHUTDOWN) {
			err = usb_submit_urb(urb, GFP_ATOMIC);
			if (err)
				gmdbg(KERN_ERR "%s: resubmit read urb failed. "
					"(%d)", __func__, err);
			else {

				/*Begin: Added for selective suspend by fangxiaozhi*/
				{
					struct usb_device *udev = interface_to_usbdev(port->serial->interface);
					//gmdbg(KERN_ERR"fxz-%s: get last busy udev=[%p], equal[%d]\n", __func__, hw_udev, (int)(hw_udev==port->serial->dev));
					usb_mark_last_busy(udev);
				}
				/*End: Added for selective suspend by fangxiaozhi*/
			}
		}

	}
	return;
}

static void option_outdat_callback(struct urb *urb)
{
	struct usb_serial_port *port;
	struct option_port_private *portdata;
	struct option_intf_private *intfdata;
	int i;

	dbg("%s", __func__);

	port =  urb->context;
	intfdata = port->serial->private;

	usb_serial_port_softint(port);
	//usb_autopm_put_interface_async(port->serial->interface); /*deleted by fangxiaozhi*/
	portdata = usb_get_serial_port_data(port);
	spin_lock(&intfdata->susp_lock);
	intfdata->in_flight--;
	spin_unlock(&intfdata->susp_lock);

	for (i = 0; i < N_OUT_URB; ++i) {
		if (portdata->out_urbs[i] == urb) {
			smp_mb__before_clear_bit();
			clear_bit(i, &portdata->out_busy);
			break;
		}
	}

	/*Begin: Added for selective suspend by fangxiaozhi*/
	{
		struct usb_device *udev = interface_to_usbdev(port->serial->interface);
		usb_mark_last_busy(udev);
	}
	/*End: Added for selective suspend by fangxiaozhi*/
}

static void option_instat_callback(struct urb *urb)
{
	int err;
	int status = urb->status;
	struct usb_serial_port *port =  urb->context;
	struct option_port_private *portdata = usb_get_serial_port_data(port);

	dbg("%s", __func__);
	dbg("%s: urb %p port %p has data %p", __func__, urb, port, portdata);

	if (status == 0) {
		struct usb_ctrlrequest *req_pkt =
				(struct usb_ctrlrequest *)urb->transfer_buffer;

		if (!req_pkt) {
			dbg("%s: NULL req_pkt\n", __func__);
			return;
		}
		if ((req_pkt->bRequestType == 0xA1) &&
				(req_pkt->bRequest == 0x20)) {
			int old_dcd_state;
			unsigned char signals = *((unsigned char *)
					urb->transfer_buffer +
					sizeof(struct usb_ctrlrequest));

			dbg("%s: signal x%x", __func__, signals);

			old_dcd_state = portdata->dcd_state;
			portdata->cts_state = 1;
			portdata->dcd_state = ((signals & 0x01) ? 1 : 0);
			portdata->dsr_state = ((signals & 0x02) ? 1 : 0);
			portdata->ri_state = ((signals & 0x08) ? 1 : 0);

			if (old_dcd_state && !portdata->dcd_state) {
				struct tty_struct *tty =
						tty_port_tty_get(&port->port);
				if (tty && !C_CLOCAL(tty))
					tty_hangup(tty);
				tty_kref_put(tty);
			}
		} else {
			dbg("%s: type %x req %x", __func__,
				req_pkt->bRequestType, req_pkt->bRequest);
		}
	} else
		err("%s: error %d", __func__, status);

	/* Resubmit urb so we continue receiving IRQ data */
	if (status != -ESHUTDOWN && status != -ENOENT) {
		err = usb_submit_urb(urb, GFP_ATOMIC);
		if (err)
			dbg("%s: resubmit intr urb failed. (%d)",
				__func__, err);
	}

	/*Begin: Added for selective suspend by fangxiaozhi*/
	{
		struct usb_device *udev = interface_to_usbdev(port->serial->interface);
		usb_mark_last_busy(udev);
	}
	/*End: Added for selective suspend by fangxiaozhi*/
}

static int option_write_room(struct tty_struct *tty)
{
	struct usb_serial_port *port = tty->driver_data;
	struct option_port_private *portdata;
	int i;
	int data_len = 0;
	struct urb *this_urb;

	portdata = usb_get_serial_port_data(port);

	for (i = 0; i < N_OUT_URB; i++) {
		this_urb = portdata->out_urbs[i];
		if (this_urb && !test_bit(i, &portdata->out_busy))
			data_len += OUT_BUFLEN;
	}

	dbg("%s: %d", __func__, data_len);
	return data_len;
}

static int option_chars_in_buffer(struct tty_struct *tty)
{
	struct usb_serial_port *port = tty->driver_data;
	struct option_port_private *portdata;
	int i;
	int data_len = 0;
	struct urb *this_urb;

	portdata = usb_get_serial_port_data(port);

	for (i = 0; i < N_OUT_URB; i++) {
		this_urb = portdata->out_urbs[i];
		/* FIXME: This locking is insufficient as this_urb may
		   go unused during the test */
		if (this_urb && test_bit(i, &portdata->out_busy))
			data_len += this_urb->transfer_buffer_length;
	}
	dbg("%s: %d", __func__, data_len);
	return data_len;
}

static int option_open(struct tty_struct *tty, struct usb_serial_port *port)
{
	struct option_port_private *portdata;
	struct option_intf_private *intfdata;
	struct usb_serial *serial = port->serial;
	int i, err;
	struct urb *urb;

	portdata = usb_get_serial_port_data(port);
	intfdata = serial->private;

	dbg("%s", __func__);
	gmdbg(KERN_ERR"fxz-%s: called\n", __func__);
	/* Start reading from the IN endpoint */
	for (i = 0; i < N_IN_URB; i++) {
		urb = portdata->in_urbs[i];
		if (!urb)
			continue;
		err = usb_submit_urb(urb, GFP_KERNEL);
		if (err) {
			dbg("%s: submit urb %d failed (%d) %d",
				__func__, i, err,
				urb->transfer_buffer_length);
		}
	}

	option_send_setup(port);

	serial->interface->needs_remote_wakeup = 1;
	spin_lock_irq(&intfdata->susp_lock);
	portdata->opened = 1;
	spin_unlock_irq(&intfdata->susp_lock);
	//usb_autopm_get_interface(serial->interface);

	return 0;
}

static void option_dtr_rts(struct usb_serial_port *port, int on)
{
	struct usb_serial *serial = port->serial;
	struct option_port_private *portdata;

	dbg("%s", __func__);
	portdata = usb_get_serial_port_data(port);
	mutex_lock(&serial->disc_mutex);
	portdata->rts_state = on;
	portdata->dtr_state = on;
	if (serial->dev)
		option_send_setup(port);
	mutex_unlock(&serial->disc_mutex);
}


static void option_close(struct usb_serial_port *port)
{
	int i;
	struct usb_serial *serial = port->serial;
	struct option_port_private *portdata;
	struct option_intf_private *intfdata = port->serial->private;

	dbg("%s", __func__);
	gmdbg(KERN_ERR"fxz-%s: called\n", __func__);
	portdata = usb_get_serial_port_data(port);

	if (serial->dev) {
		/* Stop reading/writing urbs */
		spin_lock_irq(&intfdata->susp_lock);
		portdata->opened = 0;
		spin_unlock_irq(&intfdata->susp_lock);

		for (i = 0; i < N_IN_URB; i++)
			usb_kill_urb(portdata->in_urbs[i]);
		for (i = 0; i < N_OUT_URB; i++)
			usb_kill_urb(portdata->out_urbs[i]);
		//usb_autopm_put_interface(serial->interface);
		serial->interface->needs_remote_wakeup = 0;
	}
}

/* Helper functions used by option_setup_urbs */
static struct urb *option_setup_urb(struct usb_serial *serial, int endpoint,
		int dir, void *ctx, char *buf, int len,
		void (*callback)(struct urb *))
{
	struct urb *urb;

	if (endpoint == -1)
		return NULL;		/* endpoint not needed */

	urb = usb_alloc_urb(0, GFP_KERNEL);		/* No ISO */
	if (urb == NULL) {
		dbg("%s: alloc for endpoint %d failed.", __func__, endpoint);
		return NULL;
	}

		/* Fill URB using supplied data. */
	usb_fill_bulk_urb(urb, serial->dev,
		      usb_sndbulkpipe(serial->dev, endpoint) | dir,
		      buf, len, callback, ctx);

	return urb;
}

/* Setup urbs */
static void option_setup_urbs(struct usb_serial *serial)
{
	int i, j;
	struct usb_serial_port *port;
	struct option_port_private *portdata;

	dbg("%s", __func__);

	for (i = 0; i < serial->num_ports; i++) {
		port = serial->port[i];
		portdata = usb_get_serial_port_data(port);

		/* Do indat endpoints first */
		for (j = 0; j < N_IN_URB; ++j) {
			portdata->in_urbs[j] = option_setup_urb(serial,
					port->bulk_in_endpointAddress,
					USB_DIR_IN, port,
					portdata->in_buffer[j],
					IN_BUFLEN, option_indat_callback);
		}

		/* outdat endpoints */
		for (j = 0; j < N_OUT_URB; ++j) {
			portdata->out_urbs[j] = option_setup_urb(serial,
					port->bulk_out_endpointAddress,
					USB_DIR_OUT, port,
					portdata->out_buffer[j],
					OUT_BUFLEN, option_outdat_callback);
		}
	}
}


/** send RTS/DTR state to the port.
 *
 * This is exactly the same as SET_CONTROL_LINE_STATE from the PSTN
 * CDC.
*/
static int option_send_setup(struct usb_serial_port *port)
{
	struct usb_serial *serial = port->serial;
	struct option_port_private *portdata;
	int ifNum = serial->interface->cur_altsetting->desc.bInterfaceNumber;
	int val = 0;
	dbg("%s", __func__);

	portdata = usb_get_serial_port_data(port);

	if (portdata->dtr_state)
		val |= 0x01;
	if (portdata->rts_state)
		val |= 0x02;

	return usb_control_msg(serial->dev,
		usb_rcvctrlpipe(serial->dev, 0),
		0x22, 0x21, val, ifNum, NULL, 0, USB_CTRL_SET_TIMEOUT);
}

static int option_startup(struct usb_serial *serial)
{
	int i, j, err;
	struct usb_serial_port *port;
	struct option_port_private *portdata;
	u8 *buffer;

	dbg("%s", __func__);

	/* Now setup per port private data */
	for (i = 0; i < serial->num_ports; i++) {
		port = serial->port[i];
		portdata = kzalloc(sizeof(*portdata), GFP_KERNEL);
		if (!portdata) {
			dbg("%s: kmalloc for option_port_private (%d) failed!.",
					__func__, i);
			return 1;
		}
		init_usb_anchor(&portdata->delayed);

		for (j = 0; j < N_IN_URB; j++) {
			buffer = (u8 *)__get_free_page(GFP_KERNEL);
			if (!buffer)
				goto bail_out_error;
			portdata->in_buffer[j] = buffer;
		}

		for (j = 0; j < N_OUT_URB; j++) {
			buffer = kmalloc(OUT_BUFLEN, GFP_KERNEL);
			if (!buffer)
				goto bail_out_error2;
			portdata->out_buffer[j] = buffer;
		}

		usb_set_serial_port_data(port, portdata);

		if (!port->interrupt_in_urb)
			continue;
		err = usb_submit_urb(port->interrupt_in_urb, GFP_KERNEL);
		if (err)
			dbg("%s: submit irq_in urb failed %d",
				__func__, err);
	}
	option_setup_urbs(serial);
	return 0;

bail_out_error2:
	for (j = 0; j < N_OUT_URB; j++)
		kfree(portdata->out_buffer[j]);
bail_out_error:
	for (j = 0; j < N_IN_URB; j++)
		if (portdata->in_buffer[j])
			free_page((unsigned long)portdata->in_buffer[j]);
	kfree(portdata);
	return 1;
}

static void stop_read_write_urbs(struct usb_serial *serial)
{
	int i, j;
	struct usb_serial_port *port;
	struct option_port_private *portdata;

	/* Stop reading/writing urbs */
	for (i = 0; i < serial->num_ports; ++i) {
		port = serial->port[i];
		portdata = usb_get_serial_port_data(port);
		for (j = 0; j < N_IN_URB; j++)
			usb_kill_urb(portdata->in_urbs[j]);
		for (j = 0; j < N_OUT_URB; j++)
			usb_kill_urb(portdata->out_urbs[j]);
	}
}

static void option_disconnect(struct usb_serial *serial)
{
	dbg("%s", __func__);

	stop_read_write_urbs(serial);

	/*Begin: Added for selective suspend by fangxiaozhi*/
	if(hw_udev && hw_udev == serial->dev) {
		if (hw_suspend_wq) {
			cancel_delayed_work_sync(hw_suspend_wq);
		}
		hw_udev = NULL;
    }

	/*End: Added for selective suspend by fangxiaozhi*/
}

static void option_release(struct usb_serial *serial)
{
	int i, j;
	struct usb_serial_port *port;
	struct option_port_private *portdata;

	dbg("%s", __func__);

	/* Now free them */
	for (i = 0; i < serial->num_ports; ++i) {
		port = serial->port[i];
		portdata = usb_get_serial_port_data(port);

		for (j = 0; j < N_IN_URB; j++) {
			if (portdata->in_urbs[j]) {
				usb_free_urb(portdata->in_urbs[j]);
				free_page((unsigned long)
					portdata->in_buffer[j]);
				portdata->in_urbs[j] = NULL;
			}
		}
		for (j = 0; j < N_OUT_URB; j++) {
			if (portdata->out_urbs[j]) {
				usb_free_urb(portdata->out_urbs[j]);
				kfree(portdata->out_buffer[j]);
				portdata->out_urbs[j] = NULL;
			}
		}
	}

	/* Now free per port private data */
	for (i = 0; i < serial->num_ports; i++) {
		port = serial->port[i];
		kfree(usb_get_serial_port_data(port));
	}

	/*Begin: Added for selective suspend by fangxiaozhi*/
	if (hw_suspend_wq) {
		cancel_delayed_work_sync(hw_suspend_wq);
		kfree(hw_suspend_wq);
		hw_suspend_wq = NULL;
	}
	/*End: Added for selective suspend by fangxiaozhi*/
}

#ifdef CONFIG_PM
static int option_suspend(struct usb_serial *serial, pm_message_t message)
{
	struct option_intf_private *intfdata = serial->private;

	gmdbg(KERN_ERR"fxz-%s entered\n", __func__);


	spin_lock_irq(&intfdata->susp_lock);
	if (message.event & PM_EVENT_AUTO) {
		if (intfdata->in_flight) {
			spin_unlock_irq(&intfdata->susp_lock);
			return -EBUSY;
		}
	}

	intfdata->suspended = 1;
	spin_unlock_irq(&intfdata->susp_lock);
	stop_read_write_urbs(serial);

	/*Begin: Added for selective suspend by fangxiaozhi*/
	if (hw_udev && hw_udev == serial->dev && hw_suspend_wq) {
		gmdbg(KERN_ERR"fxz-%s: cancel suspend work\n", __func__);
		cancel_delayed_work(hw_suspend_wq);
	}
	/*End: Added for selective suspend by fangxiaozhi*/
	return 0;
}

static void play_delayed(struct usb_serial_port *port)
{
	struct option_intf_private *data;
	struct option_port_private *portdata;
	struct urb *urb;
	int err;

	portdata = usb_get_serial_port_data(port);
	data = port->serial->private;
	while ((urb = usb_get_from_anchor(&portdata->delayed))) {
		err = usb_submit_urb(urb, GFP_ATOMIC);
		if (!err)
			data->in_flight++;
	}
}

static int option_resume(struct usb_serial *serial)
{
	int i, j;
	struct usb_serial_port *port;
	struct option_intf_private *intfdata = serial->private;
	struct option_port_private *portdata;
	struct urb *urb;
	int err = 0;

	gmdbg(KERN_ERR"fxz-%s entered\n", __func__);
	/* get the interrupt URBs resubmitted unconditionally */
	for (i = 0; i < serial->num_ports; i++) {
		port = serial->port[i];
		if (!port->interrupt_in_urb) {
			dbg("%s: No interrupt URB for port %d\n", __func__, i);
			continue;
		}
		err = usb_submit_urb(port->interrupt_in_urb, GFP_NOIO);
		dbg("Submitted interrupt URB for port %d (result %d)", i, err);
		if (err < 0) {
			err("%s: Error %d for interrupt URB of port%d",
				 __func__, err, i);
			goto err_out;
		}
	}

	for (i = 0; i < serial->num_ports; i++) {
		/* walk all ports */
		port = serial->port[i];
		portdata = usb_get_serial_port_data(port);

		/* skip closed ports */
		spin_lock_irq(&intfdata->susp_lock);
		if (!portdata->opened) {
			spin_unlock_irq(&intfdata->susp_lock);
			continue;
		}

		for (j = 0; j < N_IN_URB; j++) {
			urb = portdata->in_urbs[j];
			err = usb_submit_urb(urb, GFP_ATOMIC);
			if (err < 0) {
				err("%s: Error %d for bulk URB %d",
					 __func__, err, i);
				spin_unlock_irq(&intfdata->susp_lock);
				goto err_out;
			}
		}
		play_delayed(port);
		spin_unlock_irq(&intfdata->susp_lock);
	}
	spin_lock_irq(&intfdata->susp_lock);
	intfdata->suspended = 0;
	spin_unlock_irq(&intfdata->susp_lock);

	/*Begin: Added for selective suspend by fangxiaozhi*/
	if (hw_udev && hw_udev == serial->dev && hw_suspend_wq) {
		gmdbg(KERN_ERR"fxz-%s: call suspend work\n", __func__);
		schedule_delayed_work(hw_suspend_wq, 1 * HZ);
	}
	/*End: Added for selective suspend by fangxiaozhi*/
err_out:
	return err;
}
#endif

/*********************************************************************************/
/* Interface for selective suspend feature by fangxiaozhi*/
static void option_suspend_check_work(struct work_struct *work)
{
    struct usb_interface    *intf = NULL;
    int status;
    int i = 0;
	unsigned long current_time;
	status = 0;
	/*If not Huawei device, do nothing, return*/
	if (NULL == hw_udev) {
		return;
	}

	current_time = jiffies;
	//gmdbg(KERN_ERR"fxz-%s: cur_time[%ld], last_busy[%ld], delay[%ld]\n", __func__, current_time, hw_udev->last_busy, hw_udev->autosuspend_delay);
	/*If there is no transfer within <autosuspend_delay / HZ> seconds, then ask for suspend*/
	if (current_time > hw_udev->last_busy + hw_udev->autosuspend_delay) {
        if (hw_udev->actconfig) {
            for (; i < hw_udev->actconfig->desc.bNumInterfaces; i++) {
                intf = hw_udev->actconfig->interface[i];

                usb_autopm_put_interface(intf);
            }
        }
        gmdbg(KERN_ERR"fxz-%s:option_udev->last_busy = %ld, current_time=%ld, status[%d]\n", __func__,hw_udev->last_busy, current_time, status);

        schedule_delayed_work(hw_suspend_wq, 2 * HZ);
    } else {
        schedule_delayed_work(hw_suspend_wq, 1 * HZ);
    }
}
/***************************************************************************************/

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

module_param(debug, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Debug messages");
