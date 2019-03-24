/*
 * Parse file in trconf format into internal representation.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 */

#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>

#include "TrconfFmtParser.h"
#include "TrFmtException.h"

const size_t CFG_FILE_LEN = 33;
const size_t CFG_LEN = 32;

const size_t CFG_SUBNET_CHANNEL_A = 0x06;
const size_t CFG_SUBNET_CHANNEL_B = 0x07;
const size_t CFG_MAINNET_CHANNEL_A = 0x11;
const size_t CFG_MAINNET_CHANNEL_B = 0x12;

const unsigned char RFBAND_MASK = 0x03;
const unsigned char RF_868 = 0x00;
const unsigned char RF_916 = 0x01;
const unsigned char RF_433 = 0x10; // TODO: Verify

void TrconfFmtParser::parse() {
    char buffer[CFG_FILE_LEN];
    char work_buffer[CFG_LEN];
    uint8_t cnt;

    std::ifstream infile(file_name, std::ios::binary);
    data.resize(CFG_LEN);
    cfg_data_1of2.resize(CFG_LEN);
    cfg_data_2of2.resize(CFG_LEN);

    if (!infile.read (buffer, CFG_FILE_LEN)) {
        TR_THROW_FMT_EXCEPTION(file_name, 1, 0, "Can not load configuration data in TRCONF format!");
    }

    // prepare first half of configuration data
    for (cnt=0; cnt<16; cnt++) {
        work_buffer[2*cnt] = buffer[cnt];
        work_buffer[2*cnt + 1] = 0x34;
    }
    std::copy_n(work_buffer, CFG_LEN, cfg_data_1of2.begin());

    // prepare second half of configuration data
    for (cnt=0; cnt<16; cnt++) {
        work_buffer[2*cnt] = buffer[cnt+16];
        work_buffer[2*cnt + 1] = 0x34;
    }
    std::copy_n(work_buffer, CFG_LEN, cfg_data_2of2.begin());

    // copy raw configuration data
    std::copy_n(buffer, CFG_LEN, data.begin());
    // prepare rfpgm parameter
    rfpgm = static_cast<unsigned char>(buffer[32]);
    parsed = true;
}

// This only checks global vaild channels. No per country restriction enforced.
// Caution: For channel selecting, users have to ensure observing local provisions and restrictions.
static bool checkChannel(unsigned char rfband, unsigned char channel) {
    switch (rfband & RFBAND_MASK) {
        case RF_433:
            if (channel <= 16)
                return true;
            else
                return false;
            break;
        case RF_868:
            if (channel <= 67)
                return true;
            else
                return false;
            break;
        case RF_916:
            // All 256 channels are valid
            // TODO: Add checks for chips distributed in Izrael which are restricted to channels 98 - 102
            return true;
            break;
        default:
            TR_THROW_EXCEPTION(TrException, "Invalid RF band received from TR!");
            break;
    }
}

void TrconfFmtParser::checkChannels(unsigned char rfband) {
    if (!parsed)
        parse();

    if (!checkChannel(rfband, data[CFG_SUBNET_CHANNEL_A])) {
        TR_THROW_FMT_EXCEPTION(file_name, 1, 0, "Invalid main RF channel A of the optional subordinate network for configured RFBAND!");
    }

    if (!checkChannel(rfband, data[CFG_SUBNET_CHANNEL_B])) {
        TR_THROW_FMT_EXCEPTION(file_name, 1, 0, "Invalid main RF channel B of the optional subordinate network for configured RFBAND!");
    }

    if (!checkChannel(rfband, data[CFG_MAINNET_CHANNEL_A])) {
        TR_THROW_FMT_EXCEPTION(file_name, 1, 0, "Invalid main RF channel A of the main network for configured RFBAND!");
    }

    if (!checkChannel(rfband, data[CFG_MAINNET_CHANNEL_B])) {
        TR_THROW_FMT_EXCEPTION(file_name, 1, 0, "Invalid main RF channel B of the main network for configured RFBAND!");
    }
}

unsigned char TrconfFmtParser::getRFPMG(void) {
    if (!parsed)
        parse();
    return rfpgm;
}

std::basic_string<unsigned char> TrconfFmtParser::getRawData(void) {
    if (!parsed)
        parse();
    return data;
}

std::basic_string<unsigned char> TrconfFmtParser::getCfgData1of2(void) {
    if (!parsed)
        parse();
    return cfg_data_1of2;
}

std::basic_string<unsigned char> TrconfFmtParser::getCfgData2of2(void) {
    if (!parsed)
        parse();
    return cfg_data_2of2;
}
