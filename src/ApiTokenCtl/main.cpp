#include "cli_utils.h"
#include "commands.h"

namespace bpo = boost::program_options;

int main(int argc, char** argv) {
  if (argc < 2) {
    print_generic_help();
    return EXIT_FAILURE;
  }

  std::string command = argv[1];
  std::vector<std::string> args(argv + 2, argv + argc);
  if (command == "help") {
    print_generic_help();
    EXIT_SUCCESS;
  } else if (command == "create") {
    auto opts = make_base_options();
    opts.add_options()
      (
        "owner,o",
        bpo::value<std::string>()->required(),
        "token owner"
      )
      (
        "expiration,e",
        bpo::value<std::string>()->required(),
        "expiration date in relative time or unix timestamp"
      )
      (
        "service,s",
        bpo::bool_switch()->default_value(false),
        "token can use service mode"
      );

    bpo::variables_map vm;
    try {
      bpo::store(bpo::command_line_parser(args).options(opts).run(), vm);
      if (vm.count("help")) {
        print_create_help();
        EXIT_SUCCESS;
      }
      bpo::notify(vm);

      create_token(
        vm["owner"].as<std::string>(),
        vm["expiration"].as<std::string>(),
        vm.count("service"),
        make_shared_params(vm)
      );
    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
      return EXIT_FAILURE;
    }
  } else if (command == "list") {
    auto opts = make_base_options();
    bpo::variables_map vm;
    try {
      bpo::store(bpo::command_line_parser(args).options(opts).run(), vm);
      if (vm.count("help")) {
        print_list_help();
        return EXIT_SUCCESS;
      }
      bpo::notify(vm);

      list_tokens(make_shared_params(vm));
    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
      return EXIT_FAILURE;
    }
  } else if (command == "get") {
    auto opts = make_base_options();
    opts.add_options()
      (
        "id,i",
        bpo::value<int64_t>()->required(),
        "API token ID"
      );
    bpo::variables_map vm;
    try {
      bpo::store(bpo::command_line_parser(args).options(opts).run(), vm);
      if (vm.count("help")) {
        print_get_help();
        return EXIT_SUCCESS;
      }
      bpo::notify(vm);

      auto id = get_token_id(vm);
      get_token(id, make_shared_params(vm));
    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
      return EXIT_FAILURE;
    }
  } else if (command == "revoke") {
    auto opts = make_base_options();
    opts.add_options()
      (
        "id,i",
        bpo::value<int64_t>()->required(),
        "API token ID"
      );
    bpo::variables_map vm;
    try {
      bpo::store(bpo::command_line_parser(args).options(opts).run(), vm);
      if (vm.count("help")) {
        print_revoke_help();
        return EXIT_SUCCESS;
      }
      bpo::notify(vm);

      auto id = get_token_id(vm);
      revoke_token(id, make_shared_params(vm));
    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
      return EXIT_FAILURE;
    }
  } else if (command == "verify") {
    auto opts = make_base_options();
    opts.add_options()
      (
        "token,t",
        bpo::value<std::string>()->required(),
        "token to verify"
      );

    bpo::variables_map vm;
    try {
      bpo::store(bpo::command_line_parser(args).options(opts).run(), vm);
      bpo::notify(vm);

      verify_token(
        vm["token"].as<std::string>(),
        make_shared_params(vm)
      );
    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
      return EXIT_FAILURE;
    }
  } else {
    std::cerr << "Unknown or unsupported commnad.\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
