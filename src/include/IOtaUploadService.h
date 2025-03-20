/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
