#pragma once

#include <api_token.hpp>

#include <cstdint>
#include <iostream>
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>

#define OUTPUT_ID_LEN 5
#define MAX_OWNER_LEN 64
#define OUTPUT_DT_LEN 10
#define OUTPUT_REVOKED_LEN 7
#define OUTPUT_SERVICE_LEN 7

/**
 * @file cli_utils.h
 * @brief Command-line interface helpers for printing usage, parsing options,
 *        and managing shared parameters across commands.
 */

/**
 * @brief Prints general usage help message.
 */
void print_generic_help();

/**
 * @brief Prints create command usage help message.
 */
void print_create_help();

/**
 * @brief Prints list command usage help message.
 */
void print_list_help();

/**
 * @brief Prints get command usage help message.
 */
void print_get_help();

/**
 * @brief Prints revoke command usage help message.
 */
void print_revoke_help();

/**
 * @brief Prints table horizontal line
 */
void print_table_horizontal_line();

/**
 * @brief Prints table header
 */
void print_list_header();

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

/**
 * @brief Pad string end with specified character up to maximum width
 *
 * If the input text is longer than maximum width, it will be truncated.
 *
 * @param text Text to pad
 * @param max_width Maximum output width
 * @param pad_character Character to pad string with
 *
 * @return `std::string` Padded string
 */
std::string pad_end(const std::string& text, std::size_t max_width, char pad_character = ' ');
