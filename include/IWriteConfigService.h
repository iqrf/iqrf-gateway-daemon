/*
* Copyright 2018 MICRORISC s.r.o.
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
#include "WriteResult.h"
#include <string>
#include <list>

#ifdef IWriteConfigService_EXPORTS
#define IWriteConfigService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IWriteConfigService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  /// \class IWriteConfigService
	/// \brief IWriteConfigService interface
	class IWriteConfigService_DECLSPEC IWriteConfigService
	{
	public:

    /// \brief Writes specified configuration bytes and returns result.
    /// \param [in]	configBytes			  configuration bytes to write
    /// \param [in] targetNodes				nodes, to write configuration into
    /// \return												result of write
    virtual WriteResult writeConfigBytes(
      const std::vector<HWP_ConfigByte>& configBytes,
      const std::list<uint16_t>& targetNodes
    ) = 0;

		/// \brief Destructor
    virtual ~IWriteConfigService() {};
	};
}

