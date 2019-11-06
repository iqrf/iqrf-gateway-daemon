/*
 * TR interface types.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 */

#ifndef __TRTYPES_H__
#define __TRTYPES_H__

enum class TrMemory {
    ERROR,
    FLASH,
    INTERNAL_EEPROM,
    EXTERNAL_EEPROM
};

enum class TrMcu {
    NONE,
    PIC16F1938
};

enum class TrSeries {
    NONE,
    DCTR_5xD,
    DCTR_7xD
};

typedef unsigned char TrOsVersion;
typedef unsigned int  TrOsBuild;

struct TrModuleInfo {
    TrMcu       mcu;
    TrSeries     series;
    TrOsVersion osVersion;
    TrOsBuild   osBuild;
};

#endif //__TRTYPES_H__
