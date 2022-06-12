/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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
#include "NetworkInfoResult.h"
#include <set>

#ifdef IIqrfNetworkInfo_EXPORTS
#define IIqrfNetworkInfo_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IIqrfNetworkInfo_DECLSPEC SHAPE_ABI_IMPORT
#endif

/// iqrf namespace
namespace iqrf {

	/// IqrfNetworkInfo interface
	class IIqrfNetworkInfo_DECLSPEC IIqrfNetworkInfo {
	public:
		/**
		 * Destructor
		 */
		virtual ~IIqrfNetworkInfo() {};

		virtual void getNetworkInfo(NetworkInfoResult &result, const uint8_t &repeat) = 0;

		virtual std::list<std::unique_ptr<IDpaTransactionResult2>> getTransactionResults() = 0;

		virtual int getErrorCode() = 0;
	};
}
