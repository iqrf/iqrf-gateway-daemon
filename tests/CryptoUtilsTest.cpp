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

namespace iqrf {

  class CryptoUtilsTest : public ::testing::Test {
  protected:
    std::vector<unsigned char> key;
    std::string key_base64;
    std::vector<unsigned char> hash;
    std::string hash_base64;

    void SetUp() override {
      key = {0x6d,0xf8,0xfb,0xe5,0xbe,0x63,0x4f,0x4f,0x61,0xec,0x3f,0xb5,0x53,0x8c,0xd0,0x1c,0x04,0x20,0x1c,0xbc,0x92,0x73,0xb5,0x91,0x15,0x67,0x53,0x5d,0xe7,0x54,0x84,0x0c};
      key_base64 = "bfj75b5jT09h7D+1U4zQHAQgHLySc7WRFWdTXedUhAw=";
      hash = {0x00,0xd2,0xdd,0xdf,0xaf,0xaa,0x0e,0xf4,0xe5,0x5c,0x95,0x16,0xd2,0x53,0xea,0x70,0x35,0x23,0xe3,0x1b,0x86,0x95,0x19,0x01,0x60,0x29,0x45,0x4c,0xf5,0xd2,0xba,0xe5};
      hash_base64 = "ANLd36+qDvTlXJUW0lPqcDUj4xuGlRkBYClFTPXSuuU=";
    }
  };

  TEST_F(CryptoUtilsTest, sha256_hash_data_valid) {
    EXPECT_EQ(hash, CryptoUtils::sha256Hash(key));
  }

  TEST_F(CryptoUtilsTest, base64_encode_data_valid) {
    EXPECT_EQ(key_base64, CryptoUtils::base64Encode(key));
    EXPECT_EQ(hash_base64, CryptoUtils::base64Encode(hash));
  }

  TEST_F(CryptoUtilsTest, base64_decode_data_valid) {
    EXPECT_EQ(key, CryptoUtils::base64Decode(key_base64));
    EXPECT_EQ(hash, CryptoUtils::base64Decode(hash_base64));
  }

}
