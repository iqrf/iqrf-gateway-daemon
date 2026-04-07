/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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

#include "ApiTokenUtils.h"

#include <gtest/gtest.h>

namespace iqrf {

  TEST(ApiTokenUtils, isBase64Character_valid) {
    EXPECT_TRUE(ApiTokenUtils::isBase64Character('a'));
    EXPECT_TRUE(ApiTokenUtils::isBase64Character('A'));
    EXPECT_TRUE(ApiTokenUtils::isBase64Character('g'));
    EXPECT_TRUE(ApiTokenUtils::isBase64Character('g'));
    EXPECT_TRUE(ApiTokenUtils::isBase64Character('z'));
    EXPECT_TRUE(ApiTokenUtils::isBase64Character('Z'));
    EXPECT_TRUE(ApiTokenUtils::isBase64Character('0'));
    EXPECT_TRUE(ApiTokenUtils::isBase64Character('5'));
    EXPECT_TRUE(ApiTokenUtils::isBase64Character('9'));
    EXPECT_TRUE(ApiTokenUtils::isBase64Character('+'));
    EXPECT_TRUE(ApiTokenUtils::isBase64Character('/'));
    EXPECT_TRUE(ApiTokenUtils::isBase64Character('='));
  }

  TEST(ApiTokenUtils, isBase64Character_invalid) {
    EXPECT_FALSE(ApiTokenUtils::isBase64Character('-'));
    EXPECT_FALSE(ApiTokenUtils::isBase64Character('*'));
    EXPECT_FALSE(ApiTokenUtils::isBase64Character('~'));
    EXPECT_FALSE(ApiTokenUtils::isBase64Character('`'));
    EXPECT_FALSE(ApiTokenUtils::isBase64Character('!'));
    EXPECT_FALSE(ApiTokenUtils::isBase64Character('_'));
  }

  TEST(crypto_utils, parseToken_valid) {
  uint32_t id;
  std::string secret;

  ApiTokenUtils::parseToken("iqrfgd2;1;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", id, secret);
  EXPECT_EQ(1, id);
  EXPECT_EQ("updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", secret);
}

TEST(crypto_utils, parseToken_invalid_token_count) {
  uint32_t id;
  std::string secret;
  // no separators
  try {
    ApiTokenUtils::parseToken("invalid", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Invalid API token format.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
  // invalid separator
  try {
    ApiTokenUtils::parseToken("iqrfgd2:1:updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Invalid API token format.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
  // too many separators
  try {
    ApiTokenUtils::parseToken("iqrfgd2:1:updgKmdhceGzmI9i0M9UELE;xYZwSKrCBv1vusq+TfHk=", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Invalid API token format.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
}

TEST(crypto_utils, parseToken_invalid_prefix) {
  uint32_t id;
  std::string secret;
  // no separators
  try {
    ApiTokenUtils::parseToken("iqaros;1;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Unsupported API token type.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
}

TEST(crypto_utils, parseToken_id_invalid_characters) {
  uint32_t id;
  std::string secret;
  try {
    ApiTokenUtils::parseToken("iqrfgd2;1b;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("API token ID part contains non-numeric characters.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
}

TEST(crypto_utils, parseToken_secret_incorrect_len) {
  uint32_t id;
  std::string secret;
  // too short
  try {
    ApiTokenUtils::parseToken("iqrfgd2;1;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+T=", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Invalid API token secret length.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
  // too long
  try {
    ApiTokenUtils::parseToken("iqrfgd2;1;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk==", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Invalid API token secret length.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
}

TEST(crypto_utils, parseToken_secret_invalid_characters) {
  uint32_t id;
  std::string secret;
  try {
    ApiTokenUtils::parseToken("iqrfgd2;1;updgKmdhceGzmI9i0M9UELExYZwSK-CBv1vusq*TfHk=", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("API token secret contains invalid characters.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
}
}  // iqrf namespace
