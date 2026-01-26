#pragma once

#include "rapidjson/document.h"
#include "rapidjson/pointer.h"

namespace test_utils::json {

  inline rapidjson::Document parseJsonString(const std::string& text) {
    rapidjson::Document doc;
    doc.Parse(text.c_str());
    return doc;
  }

} // namespace json_test_utils
