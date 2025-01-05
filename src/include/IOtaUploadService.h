/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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
#pragma once

#include "ShapeDefines.h"
#include <stdint.h>

#ifdef IOtaUploadService_EXPORTS
#define IOtaUploadService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IOtaUploadService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

	/// \class IOtaUploadService
	/// \brief IOtaUploadService interface
	class IOtaUploadService_DECLSPEC IOtaUploadService
	{
	public:

    /// \brief Type of loading content
    enum class LoadingContentType {
      Hex,
      Iqrf_plugin
    };

    // Node authorization error definition
    enum MemoryType
    {
      flash,
      eeprom,
      eeeprom
    };

    enum MemoryRanges {
      SERIAL_EEPROM_BOTTOM = 0x0200,
      SERIAL_EEPROM_TOP = 0x09FF,
      IQRF_LICENCED_MEM_MIN_ADR = 0x2C00,
      IQRF_LICENCED_MEM_MAX_ADR = 0x37BF,
      IQRF_CONFIG_MEM_L_ADR = 0x37C0,
      IQRF_CONFIG_MEM_H_ADR = 0x37DF,
      IQRF_MAIN_MEM_MIN_ADR = 0x3A00,
      IQRF_MAIN_MEM_MAX_ADR_D = 0x3FFF,
      IQRF_MAIN_MEM_MAX_ADR_G = 0x4FFF,
      INTERNAL_EEPROM_BOTTOM = 0xF000,
      INTERNAL_EEPROM_TOP = 0xF0BF,
    };

    /// \brief Module info struct
    typedef struct ModuleInfo {
      /// MCU type
      uint8_t mcuType;
      /// TR series
      uint8_t trSeries;
      /// OS major version
      uint8_t osMajor;
      /// OS minor version
      uint8_t osMinor;
      /// OS build number
      uint16_t osBuild;
    } ModuleInfo;

	/// \brief Destructor
    virtual ~IOtaUploadService() {};
	};
}
