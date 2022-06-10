#pragma once

#include "DataTypes.h"
#include "HexStringCoversion.h"
#include "rapidjson/pointer.h"

#include <algorithm>

#include "Trace.h"

namespace iqrf {
  namespace offgrid {
    namespace api {

      class JsonTime : public Time
      {
      public:
        const rapidjson::Value * parse(const rapidjson::Value &v)
        {
          // "time": "hh:mm:ss"
          const rapidjson::Value *val = rapidjson::Pointer("/time").Get(v);
          if (!val || !val->IsString()) {
            THROW_EXC_TRC_WAR(std::logic_error, "Expected time string");
          }
          
          setTime(val->GetString());
        }

        rapidjson::Value encode(rapidjson::Document::AllocatorType & a) const
        {
          using namespace rapidjson;
          Value val;
          Pointer("/time").Set(val, getTime(), a);
          return val;
        }
      };

      class JsonDateTime
      {
      public:
        const rapidjson::Value * parse(const rapidjson::Value &v)
        {
          // "dateTime": "YYYY-MM-DDThh:mm:ss"
          const rapidjson::Value *val = rapidjson::Pointer("/dateTime").Get(v);
          if (!val || !val->IsString()) {
            THROW_EXC_TRC_WAR(std::logic_error, "Expected dateTime string");
          }

          std::chrono::time_point<std::chrono::system_clock> tp = parseTimestamp(val->GetString());
          m_date.setDate(tp);
          m_time.setTime(tp);
        }

        rapidjson::Value encode(rapidjson::Document::AllocatorType & a) const
        {
          using namespace rapidjson;
          std::ostringstream os;
          os << m_date.getDate() << 'T' << m_time.getTime();
          Value val;
          Pointer("/dateTime").Set(val, os.str(), a);
          return val;
        }
      private:
        Date m_date;
        Time m_time;
      };
    }
  }
}
