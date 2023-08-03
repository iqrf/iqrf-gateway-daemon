#include <stdexcept>

class PeripheralException : public std::runtime_error {
public:
    /**
     * Char constructor
     * @param cause Exception cause
     */
    PeripheralException(const char *cause) : runtime_error(cause) {}

    /**
     * String constructor
     * @param cause Exception cause
     */
    PeripheralException(const std::string &cause) : runtime_error(cause) {}
};

class PeripheralCommandException : public std::runtime_error {
public:
    /**
     * Char constructor
     * @param cause Exception cause
     */
    PeripheralCommandException(const char *cause) : runtime_error(cause) {}

    /**
     * String constructor
     * @param cause Exception cause
     */
    PeripheralCommandException(const std::string &cause) : runtime_error(cause) {}
};
