/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

#include <functional>
#include <map>
#include <string>

namespace iqrf {

	/// IQRF Network Enumeration interface
	class IIqrfNetworkEnum {
	public:

		/**
		 * Enumeration parameters
		 */
		struct EnumParams {
			bool reenumerate = false;
			bool standards = false;
		};

		/**
		 * Enumeration progress class
		 */
		class EnumerationProgress {
		public:
			/**
			 * Enumeration progress steps
			 */
			enum Steps {
				Start,
				NetworkDone,
				Devices,
				DevicesDone,
				Products,
				ProductsDone,
				Standards,
				StandardsDone,
				Finish
			};

			/**
			 * Base constructor
			 */
			EnumerationProgress() {}

			/**
			 * Full constructor
			 * @param step Enumeration step
			 */
			EnumerationProgress(Steps step) : step(step) {};

			/**
			 * Returns enumeration step
			 * @return Enumeration step
			 */
			Steps getStep() { return step; }

			/**
			 * Returns message corresponding to the step
			 * @param step Current step
			 * @return Step message
			 */
			std::string getStepMessage() { return stepMessages[step]; }
		private:
			/// Enumeration step
			Steps step = Steps::Start;
			/// Map of enumeration steps and messages
			std::map<Steps, std::string> stepMessages = {
				{Steps::Start, "Enumeration started, checking current state of the network."},
				{Steps::NetworkDone, "Finished checking current state of network."},
				{Steps::Devices, "Enumerating device information."},
				{Steps::DevicesDone, "Finished enumerating device information."},
				{Steps::Products, "Enumerating product information."},
				{Steps::ProductsDone, "Finished enumerating product information."},
				{Steps::Standards, "Enumerating device standards."},
				{Steps::StandardsDone, "Finished enumerating standards."},
				{Steps::Finish, "Enumeration finished."}
			};
		};

		/////

		/// Enumeration handler type
		typedef std::function<void(EnumerationProgress)> EnumerationHandler;

		/**
		 * Runs enumeration
		 * @param reenumerate Executes full enumeration regardless of the database contents
		 * @param standards Enumerates standards
		 */
		virtual void enumerate(EnumParams &parameters) = 0;

		/**
		 * Check if enumeration is in progress
		 * @return true if enumeration is in progress, false otherwise
		 */
		virtual bool isRunning() = 0;

		/**
		 * Register enumeration handler
		 * @param clientId Handler owner
		 * @param handler Handler function
		 */
		virtual void registerEnumerationHandler(const std::string &clientId, EnumerationHandler handler) = 0;

		/**
		 * Unregister enumeration handler
		 * @param clientId Handler owner
		 */
		virtual void unregisterEnumerationHandler(const std::string &clientId) = 0;

		/**
		 * Reloads all drivers
		 */
		virtual void reloadDrivers() = 0;

		/**
		 * Reloads coordinator drivers on demand
		 */
		virtual void reloadCoordinatorDrivers() = 0;
	};
}
