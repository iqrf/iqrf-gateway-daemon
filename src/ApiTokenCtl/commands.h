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
