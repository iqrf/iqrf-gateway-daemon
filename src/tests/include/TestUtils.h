#pragma once

#include "rapidjson/document.h"
#include "rapidjson/pointer.h"

inline rapidjson::Document parseJsonString(const std::string& text) {
  rapidjson::Document doc;
  doc.Parse(text.c_str());
  return doc;
}
