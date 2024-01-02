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

#include "EmbedNode.h"

namespace iqrf
{
  namespace embed
  {
    namespace node
    {
      namespace info
      {
        ////////////////
        class BriefInfo : public node::BriefInfo
        {
        public:
          BriefInfo(unsigned mid, bool disc, int hwpid, int hwpidVer, int osBuild, int dpaVer, bool enm)
            :node::BriefInfo(mid, disc, hwpid, hwpidVer, osBuild, dpaVer, enm)
          {}

          virtual ~BriefInfo() {}
        };
        typedef std::unique_ptr<BriefInfo> BriefInfoPtr;

      } //namespace info
    } //namespace node
  } //namespace embed
} //namespace iqrf
