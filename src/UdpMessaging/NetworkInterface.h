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
#include <ctime>
#include <string>

/// Network interface class
class NetworkInterface {
public:
	/**
	 * Constructor
	 * @param ip Interface IP address
	 * @param mac Interface MAC address
	 * @param metric Interface metric
	 * @param expiration Expiration timestamp
	 */
	NetworkInterface(const std::string &ip, const std::string &mac, const int32_t &metric, const std::time_t &expiration);

	/**
	 * Destructor
	 */
	~NetworkInterface() {};

	/**
	 * Returns interface IP
	 * @return Interface IP
	 */
	const std::string getIp() const;

	/**
	 * Sets new interface IP
	 * @param ip Interface IP
	 */
	void setIp(const std::string &ip);

	/**
	 * Returns interface MAC
	 * @return Interface MAC
	 */
	const std::string getMac() const;

	/**
	 * Sets new interface MAC
	 * @param mac Interface MAC
	 */
	void setMac(const std::string &mac);

	/**
	 * Returns interface metric
	 * @return Interface metric
	 */
	const int32_t& getMetric() const;

	/**
	 * Checks if interface has lower metric than metric in argument
	 * @param metric Metric to check against
	 * @return Interface metric
	 */
	bool hasLowerMetric(const int32_t &metric) const;

	/**
	 * Sets new interface metric
	 * @param metric Interface metric
	 */
	void setMetric(const int32_t &metric);

	/**
	 * Returns expiration timestamp of network interface record
	 * @return Interface record expiration
	 */
	const std::time_t& getExpiration() const;

	/**
	 * Checks if network interface information is valid by expiration
	 * @return true if information validity expired, false otherwise
	 */
	bool isExpired() const;

	/**
	 * Sets new expiration timestamp
	 * @param expiration Expiration timestamp
	 */
	void setExpiration(const std::time_t &expiration);

private:
	/// IP address
	std::string ip;
	/// MAC address
	std::string mac;
	/// Metric
	int32_t metric;
	/// Expiration timestamp
	std::time_t expiration;
};
