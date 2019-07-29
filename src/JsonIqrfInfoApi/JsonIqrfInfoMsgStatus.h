#pragma once

#include "EnumStringConvertor.h"

namespace iqrf {
  enum class MsgStatus {
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
