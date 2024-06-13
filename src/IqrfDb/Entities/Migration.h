#pragma once

#include <string>

class Migration {
public:
  Migration() = default;

  Migration(const std::string &version) : version(version) {}

  const std::string& getVersion() {
    return version;
  }

  void setVersion(const std::string &version) {
    this->version = version;
  }

  const std::string& getExecutedAt() {
    return executedAt;
  }

  void setExecutedAt(const std::string &executedAt) {
    this->executedAt = executedAt;
  }
private:
  std::string version;
  std::string executedAt;
};
