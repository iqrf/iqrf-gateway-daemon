#include <gtest/gtest.h>
#include "CryptoUtils.h"

class CryptoUtilsTest : public ::testing::Test {
protected:
  std::vector<unsigned char> salt;
  std::string salt_base64;
  std::vector<unsigned char> key;
  std::string key_base64;
  std::vector<unsigned char> hash;
  std::string hash_base64;

  void SetUp() override {
    salt = {0x1b,0x9d,0x6e,0x6d,0x73,0xbc,0x2b,0xe3,0x48,0x06,0x39,0xac,0xef,0x34,0x5a,0xaf};
    salt_base64 = "G51ubXO8K+NIBjms7zRarw==";
    key = {0x6d,0xf8,0xfb,0xe5,0xbe,0x63,0x4f,0x4f,0x61,0xec,0x3f,0xb5,0x53,0x8c,0xd0,0x1c,0x04,0x20,0x1c,0xbc,0x92,0x73,0xb5,0x91,0x15,0x67,0x53,0x5d,0xe7,0x54,0x84,0x0c};
    key_base64 = "bfj75b5jT09h7D+1U4zQHAQgHLySc7WRFWdTXedUhAw=";
    hash = {0x27,0x30,0xde,0x0b,0x61,0xf5,0x0f,0xcf,0xab,0x1b,0x4f,0xd9,0xf5,0x00,0x01,0x5b,0x3d,0xc0,0x3d,0x9f,0xbe,0x95,0xf9,0x82,0xda,0xdc,0xf0,0x4c,0xe8,0x1a,0x20,0x2f};
    hash_base64 = "JzDeC2H1D8+rG0/Z9QABWz3APZ++lfmC2tzwTOgaIC8=";
  }
};

TEST_F(CryptoUtilsTest, sha256_hash_data_valid) {
  EXPECT_EQ(hash, CryptoUtils::sha256_hash_data(salt, key));
}

TEST_F(CryptoUtilsTest, base64_encode_data_valid) {
  EXPECT_EQ(salt_base64, CryptoUtils::base64_encode_data(salt));
  EXPECT_EQ(key_base64, CryptoUtils::base64_encode_data(key));
  EXPECT_EQ(hash_base64, CryptoUtils::base64_encode_data(hash));
}

TEST_F(CryptoUtilsTest, base64_decode_data_valid) {
  EXPECT_EQ(salt, CryptoUtils::base64_decode_data(salt_base64));
  EXPECT_EQ(key, CryptoUtils::base64_decode_data(key_base64));
  EXPECT_EQ(hash, CryptoUtils::base64_decode_data(hash_base64));
}

TEST(crypto_utils, is_base64_character_valid) {
  EXPECT_TRUE(CryptoUtils::is_base64_character('a'));
  EXPECT_TRUE(CryptoUtils::is_base64_character('A'));
  EXPECT_TRUE(CryptoUtils::is_base64_character('g'));
  EXPECT_TRUE(CryptoUtils::is_base64_character('g'));
  EXPECT_TRUE(CryptoUtils::is_base64_character('z'));
  EXPECT_TRUE(CryptoUtils::is_base64_character('Z'));
  EXPECT_TRUE(CryptoUtils::is_base64_character('0'));
  EXPECT_TRUE(CryptoUtils::is_base64_character('5'));
  EXPECT_TRUE(CryptoUtils::is_base64_character('9'));
  EXPECT_TRUE(CryptoUtils::is_base64_character('+'));
  EXPECT_TRUE(CryptoUtils::is_base64_character('/'));
  EXPECT_TRUE(CryptoUtils::is_base64_character('='));
}

TEST(crypto_utils, is_base64_character_invalid) {
  EXPECT_FALSE(CryptoUtils::is_base64_character('-'));
  EXPECT_FALSE(CryptoUtils::is_base64_character('*'));
  EXPECT_FALSE(CryptoUtils::is_base64_character('~'));
  EXPECT_FALSE(CryptoUtils::is_base64_character('`'));
  EXPECT_FALSE(CryptoUtils::is_base64_character('!'));
  EXPECT_FALSE(CryptoUtils::is_base64_character('_'));
}

TEST(crypto_utils, parse_token_valid) {
  uint32_t id;
  std::string secret;

  CryptoUtils::parse_token("iqrfgd2;1;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", id, secret);
  EXPECT_EQ(1, id);
  EXPECT_EQ("updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", secret);
}

TEST(crypto_utils, parse_token_invalid_token_count) {
  uint32_t id;
  std::string secret;
  // no separators
  try {
    CryptoUtils::parse_token("invalid", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Invalid API token format.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
  // invalid separator
  try {
    CryptoUtils::parse_token("iqrfgd2:1:updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Invalid API token format.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
  // too many separators
  try {
    CryptoUtils::parse_token("iqrfgd2:1:updgKmdhceGzmI9i0M9UELE;xYZwSKrCBv1vusq+TfHk=", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Invalid API token format.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
}

TEST(crypto_utils, parse_token_invalid_prefix) {
  uint32_t id;
  std::string secret;
  // no separators
  try {
    CryptoUtils::parse_token("iqaros;1;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Unsupported API token type.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
}

TEST(crypto_utils, parse_token_id_invalid_characters) {
  uint32_t id;
  std::string secret;
  try {
    CryptoUtils::parse_token("iqrfgd2;1b;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("API token ID part contains non-numeric characters.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
}

TEST(crypto_utils, parse_token_secret_incorrect_len) {
  uint32_t id;
  std::string secret;
  // too short
  try {
    CryptoUtils::parse_token("iqrfgd2;1;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+T=", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Invalid API token secret length.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
  // too long
  try {
    CryptoUtils::parse_token("iqrfgd2;1;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk==", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("Invalid API token secret length.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
}

TEST(crypto_utils, parse_token_secret_invalid_characters) {
  uint32_t id;
  std::string secret;
  try {
    CryptoUtils::parse_token("iqrfgd2;1;updgKmdhceGzmI9i0M9UELExYZwSK-CBv1vusq*TfHk=", id, secret);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument &e) {
    EXPECT_STREQ("API token secret contains invalid characters.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown";
  }
}

TEST(crypto_utils, construct_shareable_token_plain) {
  EXPECT_EQ(
    "iqrfgd2;1;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=",
    CryptoUtils::construct_shareable_token(1, "updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", false)
  );
}

TEST(crypto_utils, construct_shareable_token_json) {
  EXPECT_EQ(
    "{\"token\":\"iqrfgd2;1;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=\"}",
    CryptoUtils::construct_shareable_token(1, "updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", true)
  );
}
