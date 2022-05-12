#include <linux/hid.h>
#include <linux/hidraw.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/power_supply.h>
#include <linux/errno.h>
#include <linux/delay.h>
//#include <linux/vermagic.h>
#include <generated/utsrelease.h>

#include "hid-ids.h"


#define FEATURE_BUFFER_SIZE 	20
#define REPORT_BUFFER_SIZE		18
#define MAX_BATTERIES_NUM		2

#define REPORT_ID_BASE			1


struct  battery_data {
	u8						id;
	//ushort					BatteryMode;
	//ushort					Temperature;
	ushort					Voltage;
	short 					Current;
    short					AverageCurrent;
    ushort 					Remainpercent;
   	ushort 					RemainingCapacity;
    ushort					FullChargeCapacity;
    //ushort					AverageTimeToEmpty;
    //ushort					AverageTimeToFull;
    //short 					ChargingCurrent;
    //ushort 					ChargingVoltage;
    //ushort 					GaugeStatus;
    ushort 					Cyclecount;
    ushort 					DesignCapacity;
    ushort 					DesignVoltage;
    //ushort 					ManufactureDate;
    //char 					Serialnumber[22];
    //u8 						Status;
    u8						AC_IN;
    u8						FullyCharged;
};

struct  battery_data *presnet_battery;

struct drive_data {
	struct hid_device       *hdev;
	u8						*report_buf;
	u8                      *feature_buf;
	struct mutex			lock;
	int 					batteries_count;
	struct battery_data		batteries[MAX_BATTERIES_NUM];
};


enum power_id {
	AC,
	BATTERY,
	POWER_NUM,
};

static int onyx_power_get_ac_property(struct power_supply *psy,
				      enum power_supply_property psp,
				      union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = presnet_battery->AC_IN;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}


static int onyx_power_get_battery_property(struct power_supply *psy,
					   enum power_supply_property psp,
					   union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = "ONYX P09C";
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = "iHELPER";
		break;

	case POWER_SUPPLY_PROP_SERIAL_NUMBER:
		val->strval = UTS_RELEASE;
		break;

	case POWER_SUPPLY_PROP_STATUS:
		val->intval =  (presnet_battery->AC_IN  == 0  ? POWER_SUPPLY_STATUS_DISCHARGING :  
			(presnet_battery->FullyCharged == 1 ?  POWER_SUPPLY_STATUS_FULL : POWER_SUPPLY_STATUS_CHARGING));
		break;
/*
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		break;
*/
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval =  presnet_battery->id > 0 ? 1 : 0;
		break;

	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;


	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = presnet_battery->DesignVoltage * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = presnet_battery->Voltage * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_POWER_NOW:
		val->intval = presnet_battery->Current * 1000;
		break;		


	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = presnet_battery->DesignCapacity * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
	case POWER_SUPPLY_PROP_ENERGY_FULL:
		val->intval = presnet_battery->FullChargeCapacity * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		val->intval = presnet_battery->RemainingCapacity * 1000;
		break;

	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = presnet_battery->Remainpercent;
		break;

	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		if (presnet_battery->Remainpercent <= 5)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
		else if(presnet_battery->Remainpercent <= 15)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_LOW;
		else if(presnet_battery->Remainpercent >= 95)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_FULL;
		else
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
		break;

/*
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		val->intval = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = presnet_battery->RemainingCapacity;
		break;

	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = presnet_battery->Current;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = presnet_battery->Remainpercent;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG:
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = 26;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = presnet_battery->Voltage;
		break;
*/
	default:
		pr_info("%s: some properties deliberately report errors.\n",
			__func__);
		return -EINVAL;
	}
	return 0;
}

static enum power_supply_property onyx_power_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property onyx_power_battery_props[] = {
	/*
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_SERIAL_NUMBER,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	*/
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_SERIAL_NUMBER,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_POWER_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_ENERGY_FULL,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_ENERGY_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL
};

static char *onyx_power_ac_supplied_to[] = {
	"onyx_battery",
};

static struct power_supply *onyx_power_supplies[POWER_NUM];

static const struct power_supply_desc onyx_power_desc[] = {
	[AC] = {
		.name = "onyx_ac",
		.type = POWER_SUPPLY_TYPE_MAINS,
		.properties = onyx_power_ac_props,
		.num_properties = ARRAY_SIZE(onyx_power_ac_props),
		.get_property = onyx_power_get_ac_property,
	},
	[BATTERY] = {
		.name = "onyx_battery",
		.type = POWER_SUPPLY_TYPE_BATTERY,
		.properties = onyx_power_battery_props,
		.num_properties = ARRAY_SIZE(onyx_power_battery_props),
		.get_property = onyx_power_get_battery_property,
	},
};

static const struct power_supply_config onyx_power_configs[] = {
	{
		/* test_ac */
		.supplied_to = onyx_power_ac_supplied_to,
		.num_supplicants = ARRAY_SIZE(onyx_power_ac_supplied_to),
	}, {
		/* test_battery */
	}, {
		/* test_usb */
		.supplied_to = onyx_power_ac_supplied_to,
		.num_supplicants = ARRAY_SIZE(onyx_power_ac_supplied_to),
	},
};


static int register_power_supply(struct hid_device *hdev)
{
	int i;
	int ret = 0;
	
	for (i = 0; i < ARRAY_SIZE(onyx_power_supplies); i++) {
		onyx_power_supplies[i] = power_supply_register(NULL,
						&onyx_power_desc[i],
						&onyx_power_configs[i]);
		if (IS_ERR(onyx_power_supplies[i])) {
			pr_err("%s: failed to register %s\n", __func__,
				onyx_power_desc[i].name);
			ret = PTR_ERR(onyx_power_supplies[i]);
		}
	}

	return ret;
}


static void update_power_supply(struct hid_device *hdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(onyx_power_supplies); i++)
		power_supply_changed(onyx_power_supplies[i]);
}

static int unregister_power_supply(struct hid_device *hdev)
{
	int i;

	/* Let's see how we handle changes... */

	for (i = 0; i < ARRAY_SIZE(onyx_power_supplies); i++)
		power_supply_changed(onyx_power_supplies[i]);

	pr_info("%s: 'changed' event sent, sleeping for 10 seconds...\n",
		__func__);
	ssleep(10);

	for (i = 0; i < ARRAY_SIZE(onyx_power_supplies); i++)
		power_supply_unregister(onyx_power_supplies[i]);

	return 0;

}


static void hid_onyx_power_find_out_reports(struct hid_device *hdev)
{
	struct hid_report_enum *report_enum = &hdev->report_enum[HID_FEATURE_REPORT];
	struct list_head *list = &report_enum->report_list;
	struct drive_data *drv_data = hid_get_drvdata(hdev);

	int count = 0;
	while (list == &report_enum->report_list) {
			count++;
			list = list->next;
	}

	drv_data->batteries_count = count;

	hid_info(hdev,"list of reports = %d \n", drv_data->batteries_count);
}

static int hid_onyx_power_get_feature(struct drive_data *drv_data)
{
	int ret = -1, i;


	//hid_info(drv_data->hdev,"HID device report number = % \n", i, drv_data->hdev->report_enum[HID_REPORT_TYPES].numbered );


	mutex_lock(&drv_data->lock);


	for(i =0 ; i < drv_data->batteries_count; i++)
	{
		memset(drv_data->feature_buf, 0, FEATURE_BUFFER_SIZE);
		ret = hid_hw_raw_request(drv_data->hdev, REPORT_ID_BASE + i, drv_data->feature_buf , FEATURE_BUFFER_SIZE, 
					   HID_FEATURE_REPORT,
					   HID_REQ_GET_REPORT);
		if(ret >= 0)
		{
			drv_data->batteries[i].id = i + 1;
			drv_data->batteries[i].DesignCapacity = drv_data->feature_buf[3] + drv_data->feature_buf[4] * 0x100 + drv_data->feature_buf[5] * 0x10000;
			drv_data->batteries[i].DesignVoltage = drv_data->feature_buf[15] + drv_data->feature_buf[16] * 0x100;
			drv_data->batteries[i].FullChargeCapacity = drv_data->feature_buf[17] + drv_data->feature_buf[18] * 0x100 + drv_data->feature_buf[19] * 0x10000;
/*
			hid_info(drv_data->hdev,"%x DesignCapacity = %d \n", i, drv_data->batteries[i].DesignCapacity);
			hid_info(drv_data->hdev,"%x DesignVoltage = %d \n", i, drv_data->batteries[i].DesignVoltage);
			hid_info(drv_data->hdev,"%x FullChargeCapacity = %d \n", i, drv_data->batteries[i].FullChargeCapacity);
*/			
		}
		else
			memset(&drv_data->batteries[i], 0, sizeof(drv_data->batteries[i]));
	}

	mutex_unlock(&drv_data->lock);


	presnet_battery = &drv_data->batteries[0];


	return ret;
}


static int hid_onyx_power_raw_event(struct hid_device *hdev,
		struct hid_report *report, u8 *raw_data, int size)
{

	struct drive_data *drv_data = hid_get_drvdata(hdev);
	int index = 0;

	mutex_lock(&drv_data->lock);

	if(report->id >= REPORT_ID_BASE && report->id < REPORT_ID_BASE + MAX_BATTERIES_NUM &&
		report->type == HID_INPUT_REPORT)
	{
		index = report->id - REPORT_ID_BASE;
		drv_data->batteries[index].Voltage = raw_data[1] + raw_data[2] * 0x100;
		drv_data->batteries[index].Current = raw_data[3] + raw_data[4] * 0x100;
		drv_data->batteries[index].AverageCurrent = raw_data[5] + raw_data[6] * 0x100;
		drv_data->batteries[index].RemainingCapacity = raw_data[7] + raw_data[8] * 0x100 + raw_data[9] * 0x10000;
		drv_data->batteries[index].Remainpercent = raw_data[14];
		drv_data->batteries[index].Cyclecount = raw_data[15] + raw_data[16] * 0x100;
		drv_data->batteries[index].AC_IN = ( (raw_data[17] & 0x01) > 0 ? 0x01 : 0x00);
		drv_data->batteries[index].FullyCharged = ((raw_data[17] & 0x10) > 0 ? 0x01 : 0x00);
/*		
		hid_info(drv_data->hdev,"%x Voltage = %d \n", index, drv_data->batteries[index].Voltage);
		hid_info(drv_data->hdev,"%x Current = %d \n", index, drv_data->batteries[index].Current);
		hid_info(drv_data->hdev,"%x AverageCurrent = %d \n", index, drv_data->batteries[index].AverageCurrent);
		hid_info(drv_data->hdev,"%x RemainingCapacity = %d \n", index, drv_data->batteries[index].RemainingCapacity);
		hid_info(drv_data->hdev,"%x Remainpercent = %d \n", index, drv_data->batteries[index].Remainpercent);
		hid_info(drv_data->hdev,"%x Cyclecount = %d \n", index, drv_data->batteries[index].Cyclecount);
*/		
	}

	mutex_unlock(&drv_data->lock);

	update_power_supply(hdev);


	return 1;
}



static int hid_onyx_power_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	struct drive_data *drv_data;

	int ret, i;

	// create a speace for struct drive_data
	drv_data = devm_kzalloc(&hdev->dev, sizeof(*drv_data), GFP_KERNEL);
	if (!drv_data)
	{
		return -ENOMEM;
	}

	hid_set_drvdata(hdev, drv_data);

	drv_data->hdev = hdev;
	mutex_init(&drv_data->lock);


	//hid_info(hdev, "struct drive_data size = %lu \n", sizeof(*drv_data));
	//hid_info(hdev, "struct battery_data size = %lu \n", sizeof(drv_data->batteries[i]));

	for(i = 0; i < MAX_BATTERIES_NUM; i++)
	{
		memset(&drv_data->batteries[i], 0, sizeof(drv_data->batteries[i]));
	}


	drv_data->report_buf = devm_kmalloc(&hdev->dev, REPORT_BUFFER_SIZE, GFP_KERNEL);
	if (!drv_data->report_buf)
	{
		return -ENOMEM;
	}

	drv_data->feature_buf = devm_kmalloc(&hdev->dev, FEATURE_BUFFER_SIZE, GFP_KERNEL);
	if (!drv_data->feature_buf)
	{
		return -ENOMEM;
	}	

	ret = hid_parse(hdev);
	if (ret)
	{
		return ret;
	}



	ret = hid_hw_start(hdev, HID_CONNECT_HIDRAW);
	if (ret)
	{
		return ret;
	}


	hid_onyx_power_find_out_reports(hdev);


	ret = hid_onyx_power_get_feature(drv_data);
	if(ret < 0)
	{
		hid_err(hdev, "hid_onyx_power_get_feature failed\n");
		return ret;
	}


	/* Open the device to receive reports with battery info */
	ret = hid_hw_open(hdev);
	if (ret < 0) {
		hid_err(hdev, "hw open failed\n");
		return ret;
	}


	ret = register_power_supply(hdev);
	if (ret < 0) {
		hid_err(hdev, "register_power_supply failed\n");
		return ret;
	}



	hid_info(hdev, "%s initialized\n", "usb hid ups");

	return 0;


}

static void hid_onyx_power_remove(struct hid_device *hdev)
{
	//struct drive_data *drv_data = hid_get_drvdata(hdev);
	//int i;

	hid_hw_stop(hdev);

	/*
	for(i =0 ; i < drv_data->batteries_count; i++)
	{
		memset(&drv_data->batteries[i], 0, sizeof(drv_data->batteries[i]));
	}*/

	unregister_power_supply(hdev);

	hid_info(hdev, "removed\n");

}



static const struct hid_device_id hid_onyx_power_table[] = {
	//{ HID_DEVICE(HID_BUS_ANY, HID_GROUP_GENERIC, HID_ANY_ID, HID_ANY_ID) },
	//	USB_VENDOR_ID_RASPBERRYPI, USB_DEVICE_ID_RPI_PICO) },	
	//{ HID_USB_DEVICE(USB_VENDOR_ID_ONYX_ATMEL, USB_DEVICE_ID_UPOWER22)},
	{ HID_USB_DEVICE(USB_VENDOR_ID_RASPBERRYPI, USB_DEVICE_ID_RPI_PICO)},	
	{ }
};
MODULE_DEVICE_TABLE(hid, hid_onyx_power_table);

static struct hid_driver hid_onyx_power_driver = {
	.name = "hid-onyx-power",
	.probe = hid_onyx_power_probe,
	.id_table = hid_onyx_power_table,
	.remove = hid_onyx_power_remove,
	.raw_event = hid_onyx_power_raw_event,
};

module_hid_driver(hid_onyx_power_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Terry Lee <kidieslee@gmail.com>");
MODULE_DESCRIPTION("USB HID UPS driver");
