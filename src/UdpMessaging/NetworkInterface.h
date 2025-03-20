/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

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
