#include <exception>
#include <stdexcept>

/// UDP channel exception class
class UdpChannelException: public std::logic_error {
public:
	/**
	 * C char constructor
	 * @param cause Exception cause
	 */
	UdpChannelException(const char* cause): logic_error(cause) {}

	/**
	 * C++ string constructor
	 * @param cause Exception cause
	 */
	UdpChannelException(const std::string& cause): logic_error(cause) {}
};
