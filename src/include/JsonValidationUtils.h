/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/utils/nlohmann_json_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

using json = nlohmann::json;

namespace iqrf {

  class JsonValidationUtils {
  private:
    static const json *fetchDocument(const std::string &uri) {
      json *doc = new json();
      if (!valijson::utils::loadDocument(uri, *doc)) {
        return nullptr;
      }
      return doc;
    }

    /**
     * Free document for remote schema resolution
     * @param adapter Remote schema document
     */
    static void freeDocument(const json *document) {
      delete document;
    }

  public:
    /**
     * Populate validation schema from jsonschema file
     * @param schema Schema to populate
     * @param path Path to schema file
     */
    static void populateSchema(valijson::Schema &schema, const std::string &path) {
      std::ifstream schemaFile(path);

      json schemaDoc = json::parse(schemaFile);
      valijson::SchemaParser parser;
      valijson::adapters::NlohmannJsonAdapter schemaAdapter(schemaDoc);
      parser.populateSchema(schemaAdapter, schema, fetchDocument, freeDocument);
    }

    /**
     * Populate validation schema from jsonschema file, and attach custom remote reference resolver
     * @param schema Schema to populate
     * @param path Path to schema file
     * @param fetchDocCallback Remote reference resolver
     */
    static void populateSchema(
        valijson::Schema &schema,
        const std::string &path,
        std::function<const json *(const std::string &)> fetchDocCallback)
    {
      std::ifstream schemaFile(path);

      json schemaDoc = json::parse(schemaFile);
      valijson::SchemaParser parser;
      valijson::adapters::NlohmannJsonAdapter schemaAdapter(schemaDoc);
      parser.populateSchema(schemaAdapter, schema, fetchDocCallback, freeDocument);
    }

    static json jsonFromFile(const std::string& path) {
      std::ifstream file;
			try {
				file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
				file.open(path);
			} catch (const std::exception &e) {
				throw std::runtime_error("Failed to open file " + path + ": " + std::strerror(errno));
			}
      json doc = json::parse(file);
      return doc;
    }

    static valijson::Schema schemaFromFile(const std::string& path) {
      json schemaDoc = jsonFromFile(path);
      valijson::Schema schema;
      valijson::SchemaParser parser;
      valijson::adapters::NlohmannJsonAdapter schemaAdapter(schemaDoc);
      parser.populateSchema(schemaAdapter, schema, fetchDocument, freeDocument);
      return schema;
    }

    /**
     * Validate file against schema
     * @param schema Validation schema
     * @param filePath Path to file
     * @param errors Validation errors
     */
    static bool validate(const valijson::Schema &schema, const std::string &filePath, valijson::ValidationResults &errors) {
      auto doc = jsonFromFile(filePath);
      return validateJson(schema, doc, errors);
    }

    static bool validate(const std::string &schemaPath, const std::string &filePath, valijson::ValidationResults &errors) {
      auto doc = jsonFromFile(filePath);
      auto schema = schemaFromFile(schemaPath);
      return validateJson(schema, doc, errors);
    }

    static bool validateJson(const valijson::Schema& schema, const json& doc, valijson::ValidationResults &errors) {
      valijson::Validator validator(valijson::Validator::kStrongTypes);
      valijson::adapters::NlohmannJsonAdapter adapter(doc);
      return validator.validate(schema, adapter, &errors);
    }

    /**
     * Process validation errors and return vector of schema violations
     * @param errors Validation errors
     * @return std::vector<std::string> Vector of schema violations
     */
    static std::vector<std::string> getSchemaViolations(valijson::ValidationResults &errors) {
      std::vector<std::string> errorMessages;

      valijson::ValidationResults::Error error;
      while (errors.popError(error)) {
        std::string context;
        std::vector<std::string>::iterator itr = error.context.begin();
        for (; itr != error.context.end(); ++itr) {
          context += *itr;
        }
        errorMessages.push_back("Violating member: " + context + ". Violation: " + error.description);
      }
      return errorMessages;
    }
  };
}
