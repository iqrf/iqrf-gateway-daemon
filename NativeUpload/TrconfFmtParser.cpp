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
    std::ifstream infile(file_name, std::ios::binary);
    data.resize(CFG_LEN);
    
    if (!infile.read (buffer, CFG_FILE_LEN)) {
        TR_THROW_FMT_EXCEPTION(file_name, 1, 0, "Can not load configuration data in TRCONF format!");
    }
    
    std::copy_n(buffer, CFG_LEN, data.begin());
    rfpgm = static_cast<unsigned char>(buffer[32]);
    parsed = true;
}

// This only checks global vaild channels. No per country restriction enforced.
// Caution: For channel selecting, users have to ensure observing local provisions and restrictions.
static bool checkChannel(unsigned char rfband, unsigned char channel) {
    switch (rfband & RFBAND_MASK) {
        case RF_433:
            if ((channel >= 0) && (channel <= 16))
                return true;
            else
                return false;
            break;
        case RF_868:
            if ((channel >= 0) && (channel <= 67))
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

std::basic_string<unsigned char> TrconfFmtParser::getData(void) {
    if (!parsed)
        parse();
    return data;
}
