#pragma once

#include <api_token.hpp>

#include <cstdint>
#include <iostream>
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>

/**
 * @file cli_utils.h
 * @brief Command-line interface helpers for printing usage, parsing options,
 *        and managing shared parameters across commands.
 */

/**
 * @brief Print general usage help message.
 */
void print_generic_help();

/**
 * @brief Print create command usage help message.
 */
void print_create_help();

/**
 * @brief Print list command usage help message.
 */
void print_list_help();

/**
 * @brief Prints get command usage help message.
 */
void print_get_help();

/**
 * @brief Print revoke command usage help message.
 */
void print_revoke_help();

/**
 * @struct SharedParams
 * @brief Holds shared command line options applicable to all commands
 */
struct SharedParams {
  std::string db_path;    ///< Path to SQLite database file
  bool json_output;       ///< Output as JSON

  /**
   * Constructor
   * @param path Path to SQLite database file
   * @param json Output as JSON
   */
  SharedParams(std::string path, bool json)
    : db_path(path), json_output(json) {}
};

/**
 * @brief Factory method to construct SharedParams from boost program options variables map
 * @param vm Variable map
 * @return SharedParams struct
 */
SharedParams make_shared_params(const boost::program_options::variables_map& vm);

/**
 * @brief Method to create base program options description defining shared options and switches
 * @return Base program options description
 */
boost::program_options::options_description make_base_options();

/**
 * @brief Retrieves token ID from boost program options variables map and checks for validity
 * @param vm Variables map
 * @return Token ID
 */
uint32_t get_token_id(boost::program_options::variables_map& vm);

/**
 * @brief Serializes API token to JSON document
 * @param token API token
 * @return JSON document representing token
 */
nlohmann::json token_to_json(const iqrf::db::models::ApiToken& token);

/**
 * @brief Serializes API token to JSON string
 * @param token API token
 * @return JSON-serialized token string
 */
std::string token_to_json_string(const iqrf::db::models::ApiToken& token);
