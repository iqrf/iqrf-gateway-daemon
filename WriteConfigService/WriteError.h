#pragma once
#include <string>

namespace iqrf {

  /// \class WriteError
  /// \brief Holds information about errors, which encounter during configuration write
  class WriteError {
  public:

    /// Type of error
    enum class Type {
      NoError,
      NodeNotBonded,
      Write
    };

    /// \brief Constructs new object of error with type of NoError and empty error string.
    WriteError() : m_type(Type::NoError), m_message("") {};

    /// \brief Constructs new object of error.
    /// \param [in] errorType	type of error
    /// \details
    /// Error message will be set to empty string. 
    WriteError(Type errorType) : m_type(errorType), m_message("") {};

    /// \brief Constructs new object of error.
    /// \param [in] errorType		type of error
    /// \param [in] message			error message with more detailed information
    WriteError(Type errorType, const std::string& message) : m_type(errorType), m_message(message) {};

    /// \brief Returns type of error
    /// \return type of error
    Type getType() const { return m_type; };

    /// \brief Returns error message
    /// \return error message
    std::string getMessage() const { return m_message; };

    WriteError& operator=(const WriteError& error);


  private:
    Type m_type;
    std::string m_message;
  };

}