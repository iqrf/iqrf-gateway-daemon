#include "cli_utils.h"

namespace bpo = boost::program_options;
using json = nlohmann::json;

void print_generic_help() {
  std::cout << "Usage: iqrfgd2-tokenctl <command> [options]\n"
    << "Commands:\n"
    << "  create    Create a new API token\n"
    << "  revoke    Revoke an API token\n"
    << "  help      Show help\n\n"
    << "Generic options:\n"
    << "  -h, --help: Show help\n"
    << "  -p, --path: Path to SQLite database file\n"
    << "  -j, --json: JSON output format"
    << "Use 'iqrfgd2-tokenctl <command> --help' to command options.\n";
}

void print_create_help() {
  std::cout << "Usage: iqrfgd2-tokenctl create [-h] -o <owner> -e <expiration> -s\n"
    << "Command options:\n"
    << "  -h, --help: Show command help\n"
    << "  -o, --owner: API token owner\n"
    << "  -e, --expiration: relative expiration date (e.g. 30d) or unix timestamp (e.g. 1764603510)\n"
    << "  -s, --service: API token can use service \n";
}

void print_list_help() {
  std::cout << "Usage: iqrfgd2-tokenctl list [-h]\n"
    << "Command options:\n"
    << "  -h, --help: Show command help\n";
}

void print_get_help() {
  std::cout << "Usage: iqrfgd2-tokenctl get [-h] -i <token_id>\n"
    << "Command options:\n"
    << "  -h, --help: Show command help\n"
    << "  -i, --id: API token ID\n";
}

void print_revoke_help() {
  std::cout << "Usage: iqrfgd2-tokenctl revoke [-h] -i <token_id>\n"
    << "Command options:\n"
    << "  -h, --help: Show command help\n"
    << "  -i, --id: API token ID\n";
}

SharedParams make_shared_params(const bpo::variables_map& vm) {
  return SharedParams(
    vm["path"].as<std::string>(),
    vm["json"].as<bool>()
  );
}

bpo::options_description make_base_options() {
  bpo::options_description base_opts("Base options");
  base_opts.add_options()
    (
      "help,h",
      "Show help"
    )
    (
      "json,j",
      bpo::bool_switch()->default_value(false),
      "json output"
    )
    (
      "path,p",
      bpo::value<std::string>()->required()->default_value("/usr/share/iqrf-gateway-daemon/DB/IqrfDb.db"),
      "path to sqlite database file"
    );
  return base_opts;
}

uint32_t get_token_id(bpo::variables_map& vm) {
  auto id = vm["id"].as<int64_t>();
  if (id <= 0 || id > std::numeric_limits<uint32_t>::max()) {
    throw std::invalid_argument("Token ID should be between 1 and 4294967295.");
  }
  return static_cast<uint32_t>(id);
}

json token_to_json(const iqrf::db::models::ApiToken& token) {
  return json({
      {"id", token.getId()},
      {"owner", token.getOwner()},
      {"created_at", token.getCreatedAt()},
      {"expires_at", token.getExpiresAt()},
      {"revoked", token.isRevoked()},
      {"service", token.canUseServiceMode()}
  });
}

std::string token_to_json_string(const iqrf::db::models::ApiToken& token) {
  return json({
      {"id", token.getId()},
      {"owner", token.getOwner()},
      {"created_at", token.getCreatedAt()},
      {"expires_at", token.getExpiresAt()},
      {"revoked", token.isRevoked()},
      {"service", token.canUseServiceMode()}
  }).dump();
}
