/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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

#include "ComBase.h"
#include "JsonUtils.h"
#include <list>
#include <map>

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	/// Sensor data input parameters struct
	typedef struct {
		std::map<uint8_t, std::list<uint8_t>> sensors;
		std::set<uint8_t> devices;
		int repeat = 1;
	} TSensorDataInputParams;

	/// Sensor data parameters class
	class SensorDataParams : public ComBase {
	public:
		/**
		 * Delete base constructor
		 */
		SensorDataParams() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		explicit SensorDataParams(Document &doc) : ComBase(doc) {
			parse(doc);
		}
	
		/**
		 * Destructor
		 */
		virtual ~SensorDataParams() {};

		/**
		 * Returns sensor data request parameters
		 * @return Sensor data request parameters
		 */
		const TSensorDataInputParams getInputParams() const {
			return m_params;
		}
	protected:
		/**
		 * Populates response document
		 * @param doc Response document
		 * @param res Transaction result
		 */
		void createResponsePayload(Document &doc, const IDpaTransactionResult2 &res) override {
			Pointer("/data/rsp/response").Set(doc, HexStringConversion::encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
		}
	private:
		/**
		 * Parses request document stores sensor data parameters
		 * @param doc Request document
		 */
		void parse(Document &doc) {
			const Value *val;
			// Sensors
			/*val = Pointer("/data/req/sensors").Get(doc);
			if (val && val->IsArray()) {
				for (auto itr = val->Begin(); itr != val->End(); ++itr) {
					const auto sensorObj = itr->GetObject();
					const uint8_t type = static_cast<uint8_t>(sensorObj["type"].GetUint());
					const auto arr = sensorObj["devices"].GetArray();
					std::list<uint8_t> devices = std::list<uint8_t>();
					for (auto it = arr.Begin(); it != arr.End(); ++it) {
						devices.push_back(static_cast<uint8_t>(it->GetUint()));
					}
					m_params.sensors.insert(std::make_pair(type, devices));
				}
			}*/
			// Devices
			val = Pointer("/data/req/devices").Get(doc);
			if (val && val->IsArray()) {
				for (auto it = val->Begin(); it != val->End(); ++it) {
					m_params.devices.insert(static_cast<uint8_t>(it->GetUint()));
				}
			}
			// Repeat
			val = Pointer("/data/repeat").Get(doc);
			if (val && val->IsInt()) {
				m_params.repeat = val->GetInt();
			}
		}
	
		/// Sensor data parameters
		TSensorDataInputParams m_params;
	};
}
