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

#ifdef INativeUploadService_EXPORTS
#define INativeUploadService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define INativeUploadService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

	/// \class INativeUploadService
	/// \brief INativeUploadService interface
	class INativeUploadService_DECLSPEC INativeUploadService
	{
	public:

	/// \brief Destructor
    virtual ~INativeUploadService() {};
	};
}