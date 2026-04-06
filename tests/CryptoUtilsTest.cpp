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

#include <gtest/gtest.h>
#include "CryptoUtils.h"

class CryptoUtilsTest : public ::testing::Test {
protected:
  std::vector<unsigned char> salt;
  std::string salt_base64;
  std::vector<unsigned char> key;
  std::string key_base64;
  std::string argon2id_key;
  std::vector<unsigned char> hash;
  std::string sha256_hash_base64;

  void SetUp() override {
    CryptoUtils::initSodium();

    salt = {0x1b,0x9d,0x6e,0x6d,0x73,0xbc,0x2b,0xe3,0x48,0x06,0x39,0xac,0xef,0x34,0x5a,0xaf};
    salt_base64 = "G51ubXO8K+NIBjms7zRarw==";
    key = {0x6d,0xf8,0xfb,0xe5,0xbe,0x63,0x4f,0x4f,0x61,0xec,0x3f,0xb5,0x53,0x8c,0xd0,0x1c,0x04,0x20,0x1c,0xbc,0x92,0x73,0xb5,0x91,0x15,0x67,0x53,0x5d,0xe7,0x54,0x84,0x0c};
    key_base64 = "bfj75b5jT09h7D+1U4zQHAQgHLySc7WRFWdTXedUhAw=";
    argon2id_key = "$argon2id$v=19$m=4096,t=2,p=1$wrLCicKewrHCqW0$eMIfvHjllucG3qIkrKUgQUfPtYezaJHp6Wjuy1SkUpU";
    hash = {0x27,0x30,0xde,0x0b,0x61,0xf5,0x0f,0xcf,0xab,0x1b,0x4f,0xd9,0xf5,0x00,0x01,0x5b,0x3d,0xc0,0x3d,0x9f,0xbe,0x95,0xf9,0x82,0xda,0xdc,0xf0,0x4c,0xe8,0x1a,0x20,0x2f};
    sha256_hash_base64 = "JzDeC2H1D8+rG0/Z9QABWz3APZ++lfmC2tzwTOgaIC8=";
  }
};

TEST_F(CryptoUtilsTest, sha256Hash_valid) {
  EXPECT_EQ(hash, CryptoUtils::sha256Hash(salt, key));
}

TEST_F(CryptoUtilsTest, base64Encode_data_valid) {
  EXPECT_EQ(salt_base64, CryptoUtils::base64Encode(salt));
  EXPECT_EQ(key_base64, CryptoUtils::base64Encode(key));
  EXPECT_EQ(sha256_hash_base64, CryptoUtils::base64Encode(hash));
}

TEST_F(CryptoUtilsTest, base64Decode_data_valid) {
  EXPECT_EQ(salt, CryptoUtils::base64Decode(salt_base64));
  EXPECT_EQ(key, CryptoUtils::base64Decode(key_base64));
  EXPECT_EQ(hash, CryptoUtils::base64Decode(sha256_hash_base64));
}

TEST_F(CryptoUtilsTest, argon2idHash) {
  auto hash = CryptoUtils::argon2idHash(key_base64);
  EXPECT_TRUE(CryptoUtils::argon2idVerify(hash, key_base64));
}

TEST_F(CryptoUtilsTest, argon2IdVerify) {
  EXPECT_TRUE(CryptoUtils::argon2idVerify(argon2id_key, key_base64));
  EXPECT_FALSE(CryptoUtils::argon2idVerify("nonsense", key_base64));
}
