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

#include "EnumStringConvertor.h"

namespace iqrf {
  enum class mngMetaDataMsgStatus {
    st_ok,
    st_badParams,
    st_duplicitParams,
    st_metaIdUnknown,
    st_metaIdAssigned,
    st_midAssigned,
    st_midUnknown,
    st_nadrUnknown,
    st_metaIdInconsistent,
    st_midInconsistent,
  };

  class ModeConvertTable
  {
  public:
    static const std::vector<std::pair<mngMetaDataMsgStatus, std::string>>& table()
    {
      static std::vector <std::pair<mngMetaDataMsgStatus, std::string>> table = {
        { mngMetaDataMsgStatus::st_ok, "ok" },
        { mngMetaDataMsgStatus::st_badParams, "bad parameters" },
        { mngMetaDataMsgStatus::st_duplicitParams, "duplicit parameters" },
        { mngMetaDataMsgStatus::st_metaIdUnknown, "metaId unknown" },
        { mngMetaDataMsgStatus::st_metaIdAssigned, "metaId assigned" },
        { mngMetaDataMsgStatus::st_midAssigned, "mid assigned" },
        { mngMetaDataMsgStatus::st_midUnknown, "mid unknown" },
        { mngMetaDataMsgStatus::st_nadrUnknown, "nadr unknown" },
        { mngMetaDataMsgStatus::st_metaIdInconsistent, "metaId inconsistent" },
        { mngMetaDataMsgStatus::st_midInconsistent, "mid inconsistent" },
      };
      return table;
    }
    static mngMetaDataMsgStatus defaultEnum()
    {
      return mngMetaDataMsgStatus::st_badParams;
    }
    static const std::string& defaultStr()
    {
      static std::string u("unknown");
      return u;
    }
  };

  typedef shape::EnumStringConvertor<mngMetaDataMsgStatus, ModeConvertTable> mngMetaDataMsgStatusConvertor;
}
