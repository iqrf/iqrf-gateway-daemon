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

#include "IRestartService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "IMessagingSplitterService.h"
#include "ITraceService.h"
#include <string>

/// iqrf namespace
namespace iqrf {
	/// \class RestartService
	/// \brief Implementation of IRestartService
	class RestartService : public IRestartService {
	public:
		/**
		 * Constructor
		 */
		RestartService();

		/**
		 * Destructor
		 */
		virtual ~RestartService();

		void activate(const shape::Properties *props = 0);
		void modify(const shape::Properties *props);
		void deactivate();

		void attachInterface(iqrf::IIqrfDpaService *iface);
		void detachInterface(iqrf::IIqrfDpaService *iface);

		void attachInterface(IMessagingSplitterService *iface);
		void detachInterface(IMessagingSplitterService *iface);

		void attachInterface(shape::ITraceService *iface);
		void detachInterface(shape::ITraceService *iface);
	private:
		class Imp;
		Imp *m_imp;
	};
}
