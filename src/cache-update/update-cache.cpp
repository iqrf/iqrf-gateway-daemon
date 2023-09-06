#include <boost/program_options.hpp>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/utils/nlohmann_json_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>
#include <zip.h>
#include <iostream>
#include <fstream>
#include <string>

using json = nlohmann::json;
namespace bpo = boost::program_options;

const std::string API_URL = "https://repository.iqrfalliance.org/api";
const std::string DEV_API_URL = "https://devrepo.iqrfalliance.org/api";

static const json* fetch_document(const std::string &uri) {
  json *doc = new json();
  auto path = (uri == "definitions.json") ? "schemas/metadataVersions/" + uri : "schemas/" + uri;
  if (!valijson::utils::loadDocument(path, *doc)) {
    return nullptr;
  }
  return doc;
}

static void free_document(const json *adapter) {
  delete adapter;
}

void get_server_check(const std::string &url) {
  cpr::Response rsp = cpr::Get(
    cpr::Url{url + "/server"},
    cpr::Header{{"accept", "application/json"}}
  );
  if (rsp.status_code != 200) {
    throw std::logic_error("Cannot get server state file, status code: " + std::to_string(rsp.status_code));
  }
  json doc = json::parse(rsp.text);
  std::ofstream file("serverCheck.json");
  file << doc;
}

void download_cache(const std::string &url) {
  cpr::Response rsp = cpr::Get(
    cpr::Url{url + "/zip"}
  );
  if (rsp.status_code != 200) {
    throw std::logic_error("Cannot download zip file, status code: " + std::to_string(rsp.status_code));
  }
  std::ofstream file("cache.zip", std::ios::binary);
  file << rsp.text;
  // extract
  const zip_uint64_t BUF_SIZE = 8196;
  char buf[BUF_SIZE];
  int err = 0;
  zip_t *archive = zip_open("cache.zip", 0, &err);
  if (archive == nullptr) {
    throw std::logic_error("Cannot open zip archive, error: " + std::to_string(err));
  }
  if (std::filesystem::exists("cache")) {
    std::filesystem::remove_all("cache");
  }
  if (!std::filesystem::create_directory("cache")) {
    throw std::logic_error("Cannot create cache directory.");
  }
  zip_int64_t num_entries = zip_get_num_entries(archive, 0);
  auto current_path = std::filesystem::current_path();
  for (zip_uint64_t i = 0; i < (zip_uint64_t)num_entries; ++i) {
    std::string name = zip_get_name(archive, i, 0);

    zip_stat_t zip_stat;
    err = zip_stat_index(archive, i, 0, &zip_stat);
    if (err != 0) {
      throw std::logic_error("Cannot obtain information about file in zip: " + name);
    }
    zip_file_t *zip_file = zip_fopen_index(archive, i, 0);
    if (!zip_file) {
      throw std::logic_error("Cannot open file in zip: " + name);
    }
    std::string extracted_file_name = current_path.string() + "/cache/" + name;
    auto extracted_file_path = std::filesystem::path(extracted_file_name);
    if (!std::filesystem::exists(extracted_file_path.parent_path())) {
      if (!std::filesystem::create_directories(extracted_file_path.parent_path())) {
        throw std::logic_error("Cannot create cache directory tree structure: " + extracted_file_path.parent_path().string());
      }
    }
    std::ofstream extracted(extracted_file_name, std::ofstream::binary | std::ios::trunc);
    zip_uint64_t sum = 0;
    zip_int64_t len = 0;
    while (sum != zip_stat.size) {
      len = zip_fread(zip_file, buf, BUF_SIZE);
      if (len < 0) {
        throw std::logic_error("cannot read file " + name);
      }
      extracted.write(buf, len);
      sum += len;
    }

    extracted.close();
    zip_fclose(zip_file);
  }
}

void populate_schema(valijson::Schema &schema, const std::string &schema_path) {
  std::ifstream schemaFile(schema_path);

  json schemaDoc = json::parse(schemaFile);
  valijson::SchemaParser parser;
  valijson::adapters::NlohmannJsonAdapter schemaAdapter(schemaDoc);

  try {
    parser.populateSchema(schemaAdapter, schema, fetch_document, free_document);
  } catch (const std::exception &e) {
    std::cerr << "Failed to parse companies jsonschema";
    throw e;
  }
}

void validate_companies_file() {
  valijson::Schema schema;
  populate_schema(schema, "schemas/companies.json");

  std::ifstream file("cache/companies/data.json");
  json doc = json::parse(file);
  valijson::Validator validator(valijson::Validator::kStrongTypes);
  valijson::ValidationResults errors;
  valijson::adapters::NlohmannJsonAdapter adapter(doc);

  if (!validator.validate(schema, adapter, &errors)) {
    valijson::ValidationResults::Error error;
    std::cerr << "Errors found when validating companies data file file." << std::endl;
    while (errors.popError(error)) {
      std::string context;
      std::vector<std::string>::iterator itr = error.context.begin();
      for (; itr != error.context.end(); ++itr) {
        context += *itr;
      }
      std::cerr << "Violating member: " << context << ". Violation: " << error.description << std::endl;
    }
    throw std::logic_error("Companies data file contains invalid data.");
  }
}

void validate_manufacturers_file() {
  valijson::Schema schema;
  populate_schema(schema, "schemas/manufacturers.json");

  std::ifstream file("cache/manufacturers/data.json");
  json doc = json::parse(file);
  valijson::Validator validator(valijson::Validator::kStrongTypes);
  valijson::ValidationResults errors;
  valijson::adapters::NlohmannJsonAdapter adapter(doc);

  if (!validator.validate(schema, adapter, &errors)) {
    std::cerr << "Errors found when validating manufacturers data file file." << std::endl;
    valijson::ValidationResults::Error error;
    while (errors.popError(error)) {
      std::string context;
      std::vector<std::string>::iterator itr = error.context.begin();
      for (; itr != error.context.end(); ++itr) {
        context += *itr;
      }
      std::cerr << "Violating member: " << context << ". Violation: " << error.description << std::endl;
    }
    throw std::logic_error("Manufacturers data file contains invalid data.");
  }
}

void validate_products_file() {
  valijson::Schema schema;
  populate_schema(schema, "schemas/products.json");

  std::ifstream file("cache/products/data.json");
  json doc = json::parse(file);
  valijson::Validator validator(valijson::Validator::kStrongTypes);
  valijson::ValidationResults errors;
  valijson::adapters::NlohmannJsonAdapter adapter(doc);

  if (!validator.validate(schema, adapter, &errors)) {
    std::cerr << "Errors found when validating products data file file." << std::endl;
    valijson::ValidationResults::Error error;
    while (errors.popError(error)) {
      std::string context;
      std::vector<std::string>::iterator itr = error.context.begin();
      for (; itr != error.context.end(); ++itr) {
        context += *itr;
      }
      std::cerr << "Violating member: " << context << ". Violation: " << error.description << std::endl;
    }
    throw std::logic_error("Products data file contains invalid data.");
  }
}

void validate_osdpa_file() {
  valijson::Schema schema;
  populate_schema(schema, "schemas/osdpa.json");

  std::ifstream file("cache/osdpa/data.json");
  json doc = json::parse(file);
  valijson::Validator validator(valijson::Validator::kStrongTypes);
  valijson::ValidationResults errors;
  valijson::adapters::NlohmannJsonAdapter adapter(doc);

  if (!validator.validate(schema, adapter, &errors)) {
    std::cerr << "Errors found when validating OSDPA data file." << std::endl;
    valijson::ValidationResults::Error error;
    while (errors.popError(error)) {
      std::string context;
      std::vector<std::string>::iterator itr = error.context.begin();
      for (; itr != error.context.end(); ++itr) {
        context += *itr;
      }
      std::cerr << "Violating member: " << context << ". Violation: " << error.description << std::endl;
    }
    throw std::logic_error("OSDPA data file contains invalid data.");
  }
}

void validate_standards_list_file() {
  valijson::Schema schema;
  populate_schema(schema, "schemas/standardList.json");

  std::ifstream file("cache/standards/data.json");
  json doc = json::parse(file);
  valijson::Validator validator(valijson::Validator::kStrongTypes);
  valijson::ValidationResults errors;
  valijson::adapters::NlohmannJsonAdapter adapter(doc);

  if (!validator.validate(schema, adapter, &errors)) {
    std::cerr << "Errors found when validating Standards list data file." << std::endl;
    valijson::ValidationResults::Error error;
    while (errors.popError(error)) {
      std::string context;
      std::vector<std::string>::iterator itr = error.context.begin();
      for (; itr != error.context.end(); ++itr) {
        context += *itr;
      }
      std::cerr << "Violating member: " << context << ". Violation: " << error.description << std::endl;
    }
    throw std::logic_error("Standards list data file contains invalid data.");
  }
}

void validate_standards_versions() {
  valijson::Schema schema;
  populate_schema(schema, "schemas/standardVersions.json");

  std::vector<std::filesystem::path> dirs;
  for (const auto &item : std::filesystem::directory_iterator("cache/standards")) {
    if (item.is_directory()) {
      dirs.emplace_back(item.path());
    }
  }

  for (const auto &dir : dirs) {
    std::string standard_file_name = dir.string() + "/data.json";
    std::ifstream standard_file(standard_file_name);
    json standard_doc = json::parse(standard_file);

    valijson::Validator validator(valijson::Validator::kStrongTypes);
    valijson::ValidationResults errors;
    valijson::adapters::NlohmannJsonAdapter adapter(standard_doc);

    if (!validator.validate(schema, adapter, &errors)) {
      std::cerr << "Errors found when validating Standard versions data file: " << standard_file_name << std::endl;
      valijson::ValidationResults::Error error;
      while (errors.popError(error)) {
        std::string context;
        std::vector<std::string>::iterator itr = error.context.begin();
        for (; itr != error.context.end(); ++itr) {
          context += *itr;
        }
        std::cerr << "Violating member: " << context << ". Violation: " << error.description << std::endl;
      }
      throw std::logic_error("Standard versions data file contains invalid data.");
    }

    std::vector<double> versions = standard_doc["versions"];

    valijson::Schema data_schema;
    populate_schema(data_schema, "schemas/standardData.json");

    for (const auto &version : versions) {
      std::ostringstream oss;
      oss << dir.string() << "/" << std::setprecision(2) << std::fixed << version << "/data.json";
      std::string data_file_name = oss.str();
      std::ifstream data_file(data_file_name);

      json data_doc = json::parse(data_file);

      valijson::adapters::NlohmannJsonAdapter data_adapter(data_doc);
      if (!validator.validate(data_schema, data_adapter, &errors)) {
        std::cerr << "Errors found when validating Standard data file: " << data_file_name << std::endl;
        valijson::ValidationResults::Error error;
        while (errors.popError(error)) {
          std::string context;
          std::vector<std::string>::iterator itr = error.context.begin();
          for (; itr != error.context.end(); ++itr) {
            context += *itr;
          }
          std::cerr << "Violating member: " << context << ". Violation: " << error.description << std::endl;
        }
        throw std::logic_error("Standard data file contains invalid data.");
      }
    }
  }
}

void validate_package_files() {
  valijson::Schema schema;
  populate_schema(schema, "schemas/package.json");

  std::vector<std::filesystem::path> dirs;
  for (const auto &item : std::filesystem::directory_iterator("cache/packages/id")) {
    if (item.is_directory()) {
      dirs.emplace_back(item.path());
    }
  }

  for (const auto &dir : dirs) {
    std::string file_name = dir.string() + "/data.json";
    std::ifstream file(file_name);
    json doc = json::parse(file);

    valijson::Validator validator(valijson::Validator::kStrongTypes);
    valijson::ValidationResults errors;
    valijson::adapters::NlohmannJsonAdapter adapter(doc);

    if (!validator.validate(schema, adapter, &errors)) {
      std::cerr << "Errors found when validating package data file: " << file_name << std::endl;
      valijson::ValidationResults::Error error;
      while (errors.popError(error)) {
        std::string context;
        std::vector<std::string>::iterator itr = error.context.begin();
        for (; itr != error.context.end(); ++itr) {
          context += *itr;
        }
        std::cerr << "Violating member: " << context << ". Violation: " << error.description << std::endl;
      }
      throw std::logic_error("Package data file contains invalid data.");
    }
  }
}

void validate_quantities_file() {
  valijson::Schema schema;
  populate_schema(schema, "schemas/quantities.json");

  std::ifstream file("cache/quantities/data.json");
  json doc = json::parse(file);
  valijson::Validator validator(valijson::Validator::kStrongTypes);
  valijson::ValidationResults errors;
  valijson::adapters::NlohmannJsonAdapter adapter(doc);

  if (!validator.validate(schema, adapter, &errors)) {
    std::cerr << "Errors found when validating quantities data file." << std::endl;
    valijson::ValidationResults::Error error;
    while (errors.popError(error)) {
      std::string context;
      std::vector<std::string>::iterator itr = error.context.begin();
      for (; itr != error.context.end(); ++itr) {
        context += *itr;
      }
      std::cerr << "Violating member: " << context << ". Violation: " << error.description << std::endl;
    }
    throw std::logic_error("Quantities data file contains invalid data.");
  }
}

int main(int argc, char** argv) {
  bool dev;
  bpo::options_description opts("General options");
  opts.add_options()
    ("help,h", "display help message")
    (
      "dev,d",
      bpo::bool_switch(&dev)->default_value(false),
      "update cache from dev repository"
    )
    (
      "path,p",
      bpo::value<std::string>()->default_value("/var/cache/iqrf-gateway-daemon/iqrfRepoCache"),
      "working directory path"
    );
  bpo::variables_map vm;
  try {
    bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
    bpo::notify(vm);
    if (vm.count("help")) {
      std::cout << "Usage: iqrfgd2-cache-updater [options]" << std::endl;
      std::cout << opts << std::endl;
      return EXIT_SUCCESS;
    }
    std::string path = vm["path"].as<std::string>();
    if (!std::filesystem::exists(path)) {
      std::filesystem::create_directories(path);
    }
    std::filesystem::current_path(std::filesystem::path(path));
    std::string url = dev ? DEV_API_URL : API_URL;
    get_server_check(url);
    download_cache(url);
    validate_companies_file();
    validate_manufacturers_file();
    validate_products_file();
    validate_osdpa_file();
    validate_standards_list_file();
    validate_standards_versions();
    validate_package_files();
    if (std::filesystem::exists("cache/quantities")) {
      validate_quantities_file();
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
