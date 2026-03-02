#pragma once

enum class StatusCodes: int {
  UNKNOWN_COMMAND = 2,
  TOKEN_NOT_FOUND = 3,
  TOKEN_EXPIRED = 4,
  TOKEN_REVOKED = 5
};
