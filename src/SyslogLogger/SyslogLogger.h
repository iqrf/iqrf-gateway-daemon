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

#include "ITraceService.h"
#include "ShapeProperties.h"
#include <string>

using namespace shape;

namespace iqrf {

	class SyslogLogger : public ITraceService {
	public:
		SyslogLogger();
		virtual ~SyslogLogger();

		bool isValid(int level, int chan) const override;
		void writeMsg(int level, int channel, const char* moduleName, const char* sourceFile, int sourceLine, const char* funcName, const std::string & msg) override;

		void activate(const Properties *props = 0);
		void deactivate();
		void modify(const Properties *props);
	private:
		class Imp;
		Imp * imp;
	};
}
