/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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

#include <chrono>
#include <optional>

#include "repositories/api_token_repo.hpp"

namespace iqrf {

  /**
   * Authentication service
   */
  class IAuthService {
  public:

    /**
     * Return API token entity by ID
     * @param id API token ID
     * @return API token entity
     */
    virtual std::unique_ptr<ApiToken> getApiToken(const uint32_t id) = 0;

    /**
     * @brief Attempt to authenticate with token
     *
     * The method returns status of token as an optional value, authentication is successful if a token not expired or revoked,
     * if the optional carries no value, the token is invalid or the record does not exist in database.
     *
     * If a token is not marked as expired in database, and would be, the table record is updated.
     *
     * @param id Token ID
     * @param secret Secret
     * @param expiration Token expiration
     * @param service Service mode permissions
     * @return Token status if token exists, nullopt if token does not exist, or if the token is invalid
     */
    virtual std::optional<ApiToken::Status> authenticate(
      const uint32_t id,
      const std::string& secret,
      std::chrono::system_clock::time_point& expiration,
      bool& service
    ) = 0;

    /**
     * @brief Check if token is revoked
     *
     * @param id Token ID
     * @return Token revoked status, nullopt if token does not exist
     */
    virtual std::optional<bool> isRevoked(const uint32_t id) = 0;
  };

}
