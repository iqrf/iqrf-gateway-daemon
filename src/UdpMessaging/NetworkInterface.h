#include <ctime>
#include <string>

/// Network interface class
class NetworkInterface {
	public:
		/**
		 * Constructor
		 * @param ip Interface IP address
		 * @param mac Interface MAC address
		 * @param expiration Expiration timestamp
		 */
		NetworkInterface(const std::string &ip, const std::string &mac, const std::time_t &expiration);

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
		 * Checks if network interface information is valid by expiration
		 * @return true if information validity expired, false otherwise
		 */
		bool isExpired() const;

		/**
		 * Sets new expiration timestamp
		 * @param expiration Expiration timestamp
		 */
		void setExpiration(const std::time_t expiration);

	private:
		/// IP address
		std::string ip;
		/// MAC address
		std::string mac;
		/// Expiration timestamp
		std::time_t expiration;
};
