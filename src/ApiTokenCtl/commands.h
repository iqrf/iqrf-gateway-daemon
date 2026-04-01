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

#include "cli_utils.h"

/**
 * @file "commands.h"
 * @brief Command-line interface command handlers.
 */

/**
 * @brief Creates a new API token from supplied data
 *
 * Command creates a new API token, attempts to persist it to database,
 * and on success, outputs the token to standard output.
 *
 * @param owner API token owner
 * @param expiration Expiration string
 * @param service Token can use service mode
 * @param params Shared CLI parameters
 */
void create_token(const std::string& owner, const std::string& expiration, bool service, const SharedParams& params);

/**
 * @brief Retrieves an existing API token and outputs information to standard output
 * @param id Token ID
 * @param params Shared CLI parameters
 */
void get_token(uint32_t id, const SharedParams& params);

/**
 * @brief Outputs a list of API tokens to standard output
 * @param params Shared CLI parameters
 */
void list_tokens(const SharedParams& params);

/**
 * @brief Revokes an existing API token
 * @param id Token ID
 * @param params Shared CLI parameters
 */
void revoke_token(uint32_t id, const SharedParams& params);

/**
 * @brief Rotates an existing API token
 * @param id Token ID
 * @param params Shared CLI parameters
 */
void rotate_token(const uint32_t id, const SharedParams& params);
