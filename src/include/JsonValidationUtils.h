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
    static const json* fetchDocument(const std::string &uri) {
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

    class InvalidJsonException : public std::invalid_argument {
    public:
      /**
       * C-style character string constructor
       * @param cause Exception cause
       */
      InvalidJsonException(const char *cause) : invalid_argument(cause) {};

      /**
       * C++ string constructor
       * @param cause Exception cause
       */
      InvalidJsonException(const std::string &cause) : invalid_argument(cause) {};
    };

    class InvalidJsonSchemaException : public std::invalid_argument {
    public:
      /**
       * C-style character string constructor
       * @param cause Exception cause
       */
      InvalidJsonSchemaException(const char *cause) : invalid_argument(cause) {};

      /**
       * C++ string constructor
       * @param cause Exception cause
       */
      InvalidJsonSchemaException(const std::string &cause) : invalid_argument(cause) {};
    };

    class SchemaViolationException : public std::invalid_argument {
    public:
      /**
       * C-style character string constructor
       * @param cause Exception cause
       */
      SchemaViolationException(const char *cause) : invalid_argument(cause) {};

      /**
       * C++ string constructor
       * @param cause Exception cause
       */
      SchemaViolationException(const std::string &cause) : invalid_argument(cause) {};
    };

    /**
     * Populate validation schema from jsonschema file
     * @param schema Schema to populate
     * @param path Path to schema file
     */
    static void populateSchema(valijson::Schema &schema, const std::string &path) {
      std::ifstream schemaFile(path);

      try {
        json schemaDoc = json::parse(schemaFile);
        valijson::SchemaParser parser;
        valijson::adapters::NlohmannJsonAdapter schemaAdapter(schemaDoc);
        parser.populateSchema(schemaAdapter, schema, fetchDocument, freeDocument);
      } catch (const json::exception &e) {
        throw InvalidJsonException(e.what());
      } catch (const std::exception &e) {
        throw InvalidJsonSchemaException(e.what());
      }

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
      std::function<const json* (const std::string&)> fetchDocCallback
    ) {
      std::ifstream schemaFile(path);

      try {
        json schemaDoc = json::parse(schemaFile);
        valijson::SchemaParser parser;
        valijson::adapters::NlohmannJsonAdapter schemaAdapter(schemaDoc);
        parser.populateSchema(schemaAdapter, schema, fetchDocCallback, freeDocument);
      } catch (const json::exception &e) {
        throw InvalidJsonException(e.what());
      } catch (const std::exception &e) {
        throw InvalidJsonSchemaException(e.what());
      }
    }

    /**
     * Validate file against schema
     * @param schema Validation schema
     * @param path Path to file
     * @param errors Validation errors
     */
    static bool validateFile(const valijson::Schema &schema, const std::string &path, valijson::ValidationResults &errors) {
      std::ifstream file(path);

      try {
        json doc = json::parse(file);
        valijson::Validator validator(valijson::Validator::kStrongTypes);
        valijson::adapters::NlohmannJsonAdapter adapter(doc);
        return validator.validate(schema, adapter, &errors);
      } catch (const json::exception &e) {
        throw InvalidJsonException("Failed to parse file " + path + " for validation: " + e.what());
      }
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