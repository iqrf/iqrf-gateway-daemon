/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <sqlite_orm/sqlite_orm.h>

#include "Entities/BinaryOutput.h"
#include "Entities/Device.h"
#include "Entities/DeviceSensor.h"
#include "Entities/Driver.h"
#include "Entities/Migration.h"
#include "Entities/Light.h"
#include "Entities/Product.h"
#include "Entities/ProductDriver.h"
#include "Entities/Sensor.h"

static inline auto initializeDb(const std::string &fileName) {
	using namespace sqlite_orm;
	return make_storage(fileName,
		make_table("product",
			make_column("id", &Product::getId, &Product::setId, primary_key().autoincrement()),
			make_column("hwpid", &Product::getHwpid, &Product::setHwpid),
			make_column("hwpidVersion", &Product::getHwpidVersion, &Product::setHwpidVersion),
			make_column("osBuild", &Product::getOsBuild, &Product::setOsBuild),
			make_column("osVersion", &Product::getOsVersion, &Product::setOsVersion),
			make_column("dpaVersion", &Product::getDpaVersion, &Product::setDpaVersion),
			make_column("handlerUrl", &Product::getHandlerUrl, &Product::setHandlerUrl),
			make_column("handlerHash", &Product::getHandlerHash, &Product::setHandlerHash),
			make_column("customDriver", &Product::getCustomDriver, &Product::setCustomDriver),
			make_column("packageId", &Product::getPackageId, &Product::setPackageId),
			make_column("name", &Product::getName, &Product::setName),
			unique(&Product::getHwpid, &Product::getHwpidVersion, &Product::getOsBuild, &Product::getDpaVersion)
		),
		make_table("driver",
			make_column("id", &Driver::getId, &Driver::setId, primary_key().autoincrement()),
			make_column("name", &Driver::getName, &Driver::setName),
			make_column("peripheralNumber", &Driver::getPeripheralNumber, &Driver::setPeripheralNumber),
			make_column("version", &Driver::getVersion, &Driver::setVersion),
			make_column("versionFlags", &Driver::getVersionFlags, &Driver::setVersionFlags),
			make_column("driver", &Driver::getDriver, &Driver::setDriver),
			make_column("driverHash", &Driver::getDriverHash, &Driver::setDriverHash),
			unique(&Driver::getPeripheralNumber, &Driver::getVersion)
		),
		make_table("productDriver",
			make_column("productId", &ProductDriver::getProductId, &ProductDriver::setProductId),
			make_column("driverId", &ProductDriver::getDriverId, &ProductDriver::setDriverId),
			foreign_key(&ProductDriver::getDriverId).references(&Driver::getId),
			foreign_key(&ProductDriver::getProductId).references(&Product::getId),
			primary_key(&ProductDriver::getProductId, &ProductDriver::getDriverId)
		),
		make_table("device",
			make_column("id", &Device::getId, &Device::setId, primary_key().autoincrement()),
			make_column("address", &Device::getAddress, &Device::setAddress, unique()),
			make_column("discovered", &Device::isDiscovered, &Device::setDiscovered),
			make_column("mid", &Device::getMid, &Device::setMid, unique()),
			make_column("vrn", &Device::getVrn, &Device::setVrn),
			make_column("zone", &Device::getZone, &Device::setZone),
			make_column("parent", &Device::getParent, &Device::setParent),
			make_column("enumerated", &Device::isEnumerated, &Device::setEnumerated),
			make_column("productId", &Device::getProductId, &Device::setProductId),
			make_column("metadata", &Device::getMetadata, &Device::setMetadata),
			foreign_key(&Device::getProductId).references(&Product::getId)
		),
		make_table("bo",
			make_column("id", &BinaryOutput::getId, &BinaryOutput::setId, primary_key().autoincrement()),
			make_column("deviceId", &BinaryOutput::getDeviceId, &BinaryOutput::setDeviceId),
			make_column("count", &BinaryOutput::getCount, &BinaryOutput::setCount),
			foreign_key(&BinaryOutput::getDeviceId).references(&Device::getId).on_delete.cascade()
		),
		make_table("light",
			make_column("id", &Light::getId, &Light::setId, primary_key().autoincrement()),
			make_column("deviceId", &Light::getDeviceId, &Light::setDeviceId),
			foreign_key(&Light::getDeviceId).references(&Device::getId).on_delete.cascade()
		),
		make_table("sensor",
			make_column("id", &Sensor::getId, &Sensor::setId, primary_key().autoincrement()),
			make_column("type", &Sensor::getType, &Sensor::setType),
			make_column("name", &Sensor::getName, &Sensor::setName),
			make_column("shortname", &Sensor::getShortname, &Sensor::setShortname),
			make_column("unit", &Sensor::getUnit, &Sensor::setUnit),
			make_column("decimals", &Sensor::getDecimals, &Sensor::setDecimals),
			make_column("frc2Bit", &Sensor::hasFrc2Bit, &Sensor::setFrc2Bit),
			make_column("frc1Byte", &Sensor::hasFrc1Byte, &Sensor::setFrc1Byte),
			make_column("frc2Byte", &Sensor::hasFrc2Byte, &Sensor::setFrc2Byte),
			make_column("frc4Byte", &Sensor::hasFrc4Byte, &Sensor::setFrc4Byte),
			unique(&Sensor::getType, &Sensor::getName)
		),
		make_table("deviceSensor",
			make_column("address", &DeviceSensor::getAddress, &DeviceSensor::setAddress),
			make_column("type", &DeviceSensor::getType, &DeviceSensor::setType),
			make_column("globalIndex", &DeviceSensor::getGlobalIndex, &DeviceSensor::setGlobalIndex),
			make_column("typeIndex", &DeviceSensor::getTypeIndex, &DeviceSensor::setTypeIndex),
			make_column("sensorId", &DeviceSensor::getSensorId, &DeviceSensor::setSensorId),
			make_column("value", &DeviceSensor::getValue, &DeviceSensor::setValue),
			make_column("updated", &DeviceSensor::getUpdated, &DeviceSensor::setUpdated),
			make_column("metadata", &DeviceSensor::getMetadata, &DeviceSensor::setMetadata),
			foreign_key(&DeviceSensor::getAddress).references(&Device::getAddress).on_delete.cascade(),
			foreign_key(&DeviceSensor::getSensorId).references(&Sensor::getId),
			primary_key(&DeviceSensor::getAddress, &DeviceSensor::getType, &DeviceSensor::getGlobalIndex)
		),
		make_table("migrations",
			make_column("version", &Migration::getVersion, &Migration::setVersion, primary_key()),
			make_column("executedAt", &Migration::getExecutedAt, &Migration::setExecutedAt)
		)
	);
}

using Storage = decltype(initializeDb(""));
