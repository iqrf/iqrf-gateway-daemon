#include "WriteError.h"

iqrf::WriteError& iqrf::WriteError::operator=(const iqrf::WriteError& error) {
  if (this == &error) {
    return *this;
  }

  this->m_type = error.m_type;
  this->m_message = error.m_message;

  return *this;
}