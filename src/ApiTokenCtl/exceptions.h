#pragma once

#include <stdexcept>

class token_not_found : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class token_expired : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class token_revoked : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};
