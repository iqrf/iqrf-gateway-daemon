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
