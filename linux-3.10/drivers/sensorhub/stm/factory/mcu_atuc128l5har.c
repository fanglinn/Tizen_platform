/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include "../ssp.h"

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define MODEL_NAME			"STM32F401CCY6B"

ssize_t mcu_revision_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "ST01%u,ST01%u\n", data->uCurFirmRev,
		get_module_rev(data));
}

ssize_t mcu_model_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", MODEL_NAME);
}

ssize_t mcu_update_kernel_bin_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bSuccess = false;
	int iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("MCU binany update!\n");

	iRet = forced_to_download_binary(data, UMS_BINARY);
	if (iRet == SUCCESS) {
		bSuccess = true;
		goto out;
	}

	iRet = forced_to_download_binary(data, KERNEL_BINARY);
	if (iRet == SUCCESS)
		bSuccess = true;
	else
		bSuccess = false;
out:
	return sprintf(buf, "%s\n", (bSuccess ? "OK" : "NG"));
}

ssize_t mcu_update_kernel_crashed_bin_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bSuccess = false;
	int iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("MCU binany update!\n");

	iRet = forced_to_download_binary(data, UMS_BINARY);
	if (iRet == SUCCESS) {
		bSuccess = true;
		goto out;
	}

	iRet = forced_to_download_binary(data, KERNEL_CRASHED_BINARY);
	if (iRet == SUCCESS)
		bSuccess = true;
	else
		bSuccess = false;
out:
	return sprintf(buf, "%s\n", (bSuccess ? "OK" : "NG"));
}

ssize_t mcu_update_ums_bin_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bSuccess = false;
	int iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("MCU binany update!\n");

	iRet = forced_to_download_binary(data, UMS_BINARY);
	if (iRet == SUCCESS)
		bSuccess = true;
	else
		bSuccess = false;

	return sprintf(buf, "%s\n", (bSuccess ? "OK" : "NG"));
}

ssize_t mcu_reset_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	reset_mcu(data);

	return sprintf(buf, "OK\n");
}

ssize_t mcu_dump_show(struct device *dev, struct device_attribute *attr,
		char *buf) {
	struct ssp_data *data = dev_get_drvdata(dev);
	int status = 1, iDelaycnt = 0;

	data->bDumping = true;
	set_big_data_start(data, BIG_TYPE_DUMP, 0);
	msleep(300);
	while (data->bDumping) {
		mdelay(10);
		if (iDelaycnt++ > 1000) {
			status = 0;
			break;
		}
	}
	return sprintf(buf, "%s\n", status ? "OK" : "NG");
}

static char buffer[FACTORY_DATA_MAX];

ssize_t mcu_factorytest_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet = 0;
	struct ssp_msg *msg;

	if (sysfs_streq(buf, "1")) {
		msg = kzalloc(sizeof(*msg), GFP_KERNEL);
		msg->cmd = MCU_FACTORY;
		msg->length = 5;
		msg->options = AP2HUB_READ;
		msg->buffer = buffer;
		msg->free_buffer = 0;

		memset(msg->buffer, 0, 5);

		iRet = ssp_spi_async(data, msg);

	} else {
		ssp_err("invalid value %d\n", *buf);
		return -EINVAL;
	}

	ssp_dbg("MCU Factory Test Start! - %d\n", iRet);

	return size;
}

ssize_t mcu_factorytest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bMcuTestSuccessed = false;
	struct ssp_data *data = dev_get_drvdata(dev);

	if (data->bSspShutdown == true) {
		ssp_dbg(" MCU Bin is crashed\n");
		return sprintf(buf, "NG,NG,NG\n");
	}

	ssp_dbg("MCU Factory Test Data : %u, %u, %u, %u, %u\n", buffer[0],
			buffer[1], buffer[2], buffer[3], buffer[4]);

		/* system clock, RTC, I2C Master, I2C Slave, externel pin */
	if ((buffer[0] == SUCCESS)
			&& (buffer[1] == SUCCESS)
			&& (buffer[2] == SUCCESS)
			&& (buffer[3] == SUCCESS)
			&& (buffer[4] == SUCCESS))
		bMcuTestSuccessed = true;

	ssp_dbg("MCU Factory Test Result - %s, %s, %s\n", MODEL_NAME,
		(bMcuTestSuccessed ? "OK" : "NG"), "OK");

	return sprintf(buf, "%s,%s,%s\n", MODEL_NAME,
		(bMcuTestSuccessed ? "OK" : "NG"), "OK");
}

ssize_t mcu_sleep_factorytest_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet = 0;
	struct ssp_msg *msg;

	if (sysfs_streq(buf, "1")) {
		msg = kzalloc(sizeof(*msg), GFP_KERNEL);
		msg->cmd = MCU_SLEEP_FACTORY;
		msg->length = FACTORY_DATA_MAX;
		msg->options = AP2HUB_READ;
		msg->buffer = buffer;
		msg->free_buffer = 0;

		iRet = ssp_spi_async(data, msg);

	} else {
		ssp_err("invalid value %d\n", *buf);
		return -EINVAL;
	}

	ssp_dbg("MCU Sleep Factory Test Start! - %d\n", iRet);

	return size;
}

ssize_t mcu_sleep_factorytest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int iDataIdx, iSensorData = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	struct sensor_value fsb[SENSOR_MAX];
	u16 chLength = 0;

	memcpy(&chLength, buffer, 2);
	memset(fsb, 0, sizeof(struct sensor_value) * SENSOR_MAX);

	for (iDataIdx = 2; iDataIdx < chLength + 2;) {
		iSensorData = (int)buffer[iDataIdx++];

		if ((iSensorData < 0) ||
			(iSensorData >= (SENSOR_MAX - 1))) {
			ssp_err("Mcu data frame error %d\n",
				iSensorData);
			goto exit;
		}

		data->get_sensor_data[iSensorData]((char *)buffer,
			&iDataIdx, &(fsb[iSensorData]));
	}

	convert_acc_data(&fsb[ACCELEROMETER_SENSOR].x);
	convert_acc_data(&fsb[ACCELEROMETER_SENSOR].y);
	convert_acc_data(&fsb[ACCELEROMETER_SENSOR].z);

	fsb[ACCELEROMETER_SENSOR].x -= data->accelcal.x;
	fsb[ACCELEROMETER_SENSOR].y -= data->accelcal.y;
	fsb[ACCELEROMETER_SENSOR].z -= data->accelcal.z;

	fsb[GYROSCOPE_SENSOR].x -= data->gyrocal.x;
	fsb[GYROSCOPE_SENSOR].y -= data->gyrocal.y;
	fsb[GYROSCOPE_SENSOR].z -= data->gyrocal.z;

	fsb[PRESSURE_SENSOR].pressure[0] -= data->iPressureCal;

exit:
	ssp_dbg("Result\n"
		"accel %d,%d,%d\n"
		"gyro %d,%d,%d\n"
		"mag %d,%d,%d\n"
		"baro %d,%d\n"
		"ges %d,%d,%d,%d\n"
		"prox %u,%u\n"
		"temp %d,%d,%d\n"
#ifdef CONFIG_SENSORS_SSP_MAX88921
		"light %u,%u,%u,%u,%u,%u\n",
#else
		"light %u,%u,%u,%u\n",
#endif
		fsb[ACCELEROMETER_SENSOR].x, fsb[ACCELEROMETER_SENSOR].y,
		fsb[ACCELEROMETER_SENSOR].z, fsb[GYROSCOPE_SENSOR].x,
		fsb[GYROSCOPE_SENSOR].y, fsb[GYROSCOPE_SENSOR].z,
		fsb[GEOMAGNETIC_SENSOR].x, fsb[GEOMAGNETIC_SENSOR].y,
		fsb[GEOMAGNETIC_SENSOR].z, fsb[PRESSURE_SENSOR].pressure[0],
		fsb[PRESSURE_SENSOR].pressure[1],
		fsb[GESTURE_SENSOR].data[0], fsb[GESTURE_SENSOR].data[1],
		fsb[GESTURE_SENSOR].data[2], fsb[GESTURE_SENSOR].data[3],
		fsb[PROXIMITY_SENSOR].prox[0], fsb[PROXIMITY_SENSOR].prox[1],
		fsb[TEMPERATURE_HUMIDITY_SENSOR].data[0],
		fsb[TEMPERATURE_HUMIDITY_SENSOR].data[1],
		fsb[TEMPERATURE_HUMIDITY_SENSOR].data[2],
		fsb[LIGHT_SENSOR].r, fsb[LIGHT_SENSOR].g, fsb[LIGHT_SENSOR].b,
		fsb[LIGHT_SENSOR].w
#ifdef CONFIG_SENSORS_SSP_MAX88921
		, fsb[LIGHT_SENSOR].ir_cmp, fsb[LIGHT_SENSOR].amb_pga
#endif
		);

	return sprintf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,"
//#ifdef CONFIG_SENSORS_SSP_MAX88921
//		"%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d\n",
//#else
		"%u,%u,%u,%u,%d,%d,%d,%d,%d,%d\n",
//#endif
		fsb[ACCELEROMETER_SENSOR].x, fsb[ACCELEROMETER_SENSOR].y,
		fsb[ACCELEROMETER_SENSOR].z, fsb[GYROSCOPE_SENSOR].x,
		fsb[GYROSCOPE_SENSOR].y, fsb[GYROSCOPE_SENSOR].z,
		fsb[GEOMAGNETIC_SENSOR].x, fsb[GEOMAGNETIC_SENSOR].y,
		fsb[GEOMAGNETIC_SENSOR].z, fsb[PRESSURE_SENSOR].pressure[0],
		fsb[PRESSURE_SENSOR].pressure[1], fsb[PROXIMITY_SENSOR].prox[1],
		fsb[LIGHT_SENSOR].r, fsb[LIGHT_SENSOR].g, fsb[LIGHT_SENSOR].b,
		fsb[LIGHT_SENSOR].w,
//#ifdef CONFIG_SENSORS_SSP_MAX88921
//		fsb[LIGHT_SENSOR].ir_cmp, fsb[LIGHT_SENSOR].amb_pga,
//#endif
		fsb[GESTURE_SENSOR].data[0], fsb[GESTURE_SENSOR].data[1],
		fsb[GESTURE_SENSOR].data[2], fsb[GESTURE_SENSOR].data[3],
		fsb[TEMPERATURE_HUMIDITY_SENSOR].data[0],
		fsb[TEMPERATURE_HUMIDITY_SENSOR].data[1]);
}
