/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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

#include <map>

#ifdef IDpaValueService_EXPORTS
#define IDpaValueService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IDpaValueService_DECLSPEC SHAPE_ABI_IMPORT
#endif

#ifdef IDpaHopsService_EXPORTS
#define IDpaHopsService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IDpaHopsService_DECLSPEC SHAPE_ABI_IMPORT
#endif

#ifdef IFrcParamsService_EXPORTS
#define IFrcParamsService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IFrcParamsService_DECLSPEC SHAPE_ABI_IMPORT
#endif

/// iqrf namespace
namespace iqrf {
	/// DPA value actions
	typedef enum {
		GET = 0,
		SET = 1
	} TDpaParamAction;

	/// DPA param action enum to string map
	static std::map<TDpaParamAction, std::string> dpaParamActionEnumStringMap = {
		std::make_pair(TDpaParamAction::GET, "get"),
		std::make_pair(TDpaParamAction::SET, "set"),
	};

	/// DPA param action string to enum map
	static std::map<std::string, TDpaParamAction> dpaParamActionStringEnumMap = {
		std::make_pair("get", TDpaParamAction::GET),
		std::make_pair("set", TDpaParamAction::SET),
	};

	/// DPA Value service interface
	class IDpaValueService_DECLSPEC IDpaValueService {
	public:
		/**
		 * Destructor
		 */
		virtual ~IDpaValueService() {};
	};

	/// DPA Hops service interface
	class IDpaHopsService_DECLSPEC IDpaHopsService {
	public:
		/**
		 * Destructor
		 */
		virtual ~IDpaHopsService() {};
	};

	/// DPA FRC params service interface
	class IFrcParamsService_DECLSPEC IFrcParamsService {
	public:
		/**
		 * Destructor
		 */
		virtual ~IFrcParamsService() {};
	};
}
