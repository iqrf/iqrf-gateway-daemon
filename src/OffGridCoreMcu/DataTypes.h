#pragma once

#include <string>
#include <vector>
#include <chrono>

#include "Trace.h"

namespace iqrf {
  namespace offgrid {

    class Time
    {
    private:
      uint8_t m_hour;
      uint8_t m_min;
      uint8_t m_sec;

    public:
      Time()
        :m_hour(0)
        , m_min(0)
        , m_sec(0)
      {}

      std::string getTime() const
      {
        // "time": "hh:mm:ss"
        std::ostringstream os;
        os << std::setfill('0') << std::setw(2) << (int)m_hour << ':'
          << std::setfill('0') << std::setw(2) << (int)m_min << ':'
          << std::setfill('0') << std::setw(2) << (int)m_sec;
        return os.str();
      }

      void setTime(const std::string & timeStr)
      {
        std::string tstr(timeStr);

        std::replace(tstr.begin(), tstr.end(), ':', ' ');
        std::istringstream is(tstr);
        int hh, mm, ss;
        try {
          is >> hh >> mm >> ss;
        }
        catch (std::exception &e) {
          THROW_EXC_TRC_WAR(std::logic_error, "Bad format: " << PAR(timeStr));
        }

        //check values
        if (hh > 23 || hh < 0 || mm > 59 || mm < 0 || ss > 59 || ss < 0) {
          THROW_EXC_TRC_WAR(std::logic_error, "Bad values: " << PAR(timeStr));
        }

        m_hour = (uint8_t)hh;
        m_min = (uint8_t)mm;
        m_sec = (uint8_t)ss;
      }

      void serialize(std::vector<uint8_t> & vect) const {
        vect.push_back(m_hour);
        vect.push_back(m_min);
        vect.push_back(m_sec);
      }

      void deserialize(std::vector<uint8_t>::iterator & pos, const std::vector<uint8_t>::iterator end) {
        if (end - pos < 3) {
          THROW_EXC_TRC_WAR(std::out_of_range, "provided buffer is too short");
        }
        m_hour = *pos++;
        m_min = *pos++;
        m_sec = *pos++;
      }
    };

    class Date
    {
    public:
      uint8_t m_year;
      uint8_t m_month;
      uint8_t m_mday;
      uint8_t m_wday;

    public:
      Date()
        :m_year(0)
        , m_month(0)
        , m_mday(0)
        , m_wday(0)
      {}

      std::string getDate() const
      {
        // "date": "YYYY:MM:DD" => we don't care weak day
        std::ostringstream os;
        int YYYY = 2000 + (int)m_year; // good luck guys from next millenium
        os << YYYY << '-'
          << std::setfill('0') << std::setw(2) << (int)m_month << '-'
          << std::setfill('0') << std::setw(2) << (int)m_mday;
        return os.str();
      }

      //set date from timepoint date part
      void setDate(const std::string & dateStr)
      {
        std::string ds(dateStr);
        ds += "T01:00:00";
        std::chrono::system_clock::time_point tp = parseTimestamp(ds);
        auto time = std::chrono::system_clock::to_time_t(tp);
        auto tm = *std::localtime(&time);
        m_year = (uint8_t)((tm.tm_year + 1900) % 1000);
        m_month = (uint8_t)tm.tm_mon + 1;
        m_mday = (uint8_t)tm.tm_mday;
        m_wday = tm.tm_wday != 0 ? (uint8_t)tm.tm_wday : 7;
      }

      void serialize(std::vector<uint8_t> & vect) const {
        vect.push_back(m_mday);
        vect.push_back(m_month);
        vect.push_back(m_year);
        vect.push_back(m_wday);
      }
      
      virtual void deserialize(std::vector<uint8_t>::iterator & pos, const std::vector<uint8_t>::iterator end) {
        if (end - pos < 3) {
          THROW_EXC_TRC_WAR(std::out_of_range, "provided buffer is too short");
        }
        m_mday = *pos++;
        m_month = *pos++;
        m_year = *pos++;
        //m_wday = *pos++;
      }
    };

    class DateWithWeek : public Date
    {
    public:
      void deserialize(std::vector<uint8_t>::iterator & pos, const std::vector<uint8_t>::iterator end) override {
        Date::deserialize(pos, end);
        if (end - pos < 1) {
          THROW_EXC_TRC_WAR(std::out_of_range, "provided buffer is too short");
        }
        m_wday = *pos++;
      }
    };

    ////////////////
    class OffGridCmd
    {
    protected:
      OffGridCmd() = delete;

      OffGridCmd(uint8_t pnum, uint8_t pcmd)
        :m_pnum(pnum)
        , m_pcmd(pcmd)
        , m_rcode(0)
      {}
    public:
      const std::vector<uint8_t> & encodeRequest() {
        m_req.clear();
        m_req.push_back(m_pnum);
        m_req.push_back(m_pcmd);
        m_req.push_back(0); //reserve for len
        //call overrided to encode data of derived classes
        encode(m_req);
        m_req[2] = (uint8_t)(m_req.size() + 1); //len is plus 1 because of CRC computed by lower layer
        return m_req;
      }
      const void parseResponse(const std::vector<uint8_t> & res) {
        m_res = res;
        std::vector<uint8_t>::iterator pos = m_res.begin();
        const std::vector<uint8_t>::iterator end = m_res.end();

        //check min len
        if (res.size() < 3) {
          THROW_EXC_TRC_WAR(std::out_of_range, "Provided buffer is too short");
        }

        //check PNUM
        uint8_t pnum = *pos++;
        if ((0x7F & pnum) != m_pnum) { //get rid of MSB
          THROW_EXC_TRC_WAR(std::logic_error, "Responded by unexpected " << std::hex << NAME_PAR(pnum, (int)pnum));
        }

        //check PCMD
        uint8_t pcmd = *pos++;
        if (pcmd != m_pcmd) {
          THROW_EXC_TRC_WAR(std::logic_error, "Responded by unexpected " << std::hex << NAME_PAR(pcmd, (int)pcmd));
        }

        //check len
        uint8_t len = *pos++;
        if (len != res.size() + 1) { //len is plus 1 because of CRC computed by lower layer
          THROW_EXC_TRC_WAR(std::logic_error, "Responded by unexpected" << PAR(len) << PAR(res.size()));
        }

        //check retval
        uint8_t retval = *pos++;
        if (retval != 0) { //retval == 0 if all OK
          THROW_EXC_TRC_WAR(std::logic_error, "Responded return error" << NAME_PAR(retval, (int)retval));
        }

        //call overrided to parse data of derived classes
        parse(pos, end);
      }

    protected:
      virtual void encode(std::vector<uint8_t> & vect) const {
        //nothing to do here by default
      }
      virtual void parse(std::vector<uint8_t>::iterator & pos, const std::vector<uint8_t>::iterator end) {
        //nothing to do here by default
      }
      uint8_t m_pnum;
      uint8_t m_pcmd;
      uint8_t m_rcode;
      std::vector<uint8_t> m_req;
      std::vector<uint8_t> m_res;
    };

    ///////////////
    // Common base for all set time msgs 
    class SetTimeCmd : public OffGridCmd
    {
    public:
      SetTimeCmd(uint8_t pnum, uint8_t pcmd)
        :OffGridCmd(pnum, pcmd)
      {}

      void setTime(const std::string & timeStr) {
        m_time.setTime(timeStr);
      }

      std::string getTime() {
        return m_time.getTime();
      }

    protected:
      virtual void encode(std::vector<uint8_t> & vect) const {
        m_time.serialize(vect);
      }

    private:
      Time m_time;
    };

    ///////////////
    // Common base for all get time msgs 
    class GetTimeCmd : public OffGridCmd
    {
    public:
      GetTimeCmd(uint8_t pnum, uint8_t pcmd)
        :OffGridCmd(pnum, pcmd)
      {}

      std::string getTime() const {
        return m_time.getTime();
      }

    protected:
      virtual void parse(std::vector<uint8_t>::iterator & pos, const std::vector<uint8_t>::iterator end)
      {
        m_time.deserialize(pos, end);
      };

    private:
      Time m_time;
    };

    ///////////////
    // Common base for all get time msgs 
    class GetDateTimeCmd : public OffGridCmd
    {
    public:
      GetDateTimeCmd(uint8_t pnum, uint8_t pcmd)
        :OffGridCmd(pnum, pcmd)
      {}

      std::string getTime() const {
        return m_time.getTime();
      }

      std::string getDate() const {
        return m_date.getDate();
      }

    protected:
      virtual void parse(std::vector<uint8_t>::iterator & pos, const std::vector<uint8_t>::iterator end)
      {
        m_date.deserialize(pos, end);
        m_time.deserialize(pos, end);
      };

    private:
      Time m_time;
      Date m_date;
    };

    ///////////////
    class SetPwrOffTimeCmd : public SetTimeCmd
    {
    public:
      SetPwrOffTimeCmd()
        :SetTimeCmd(1, 1)
      {}
    };

    ///////////////
    class SetWakeUpTimeCmd : public SetTimeCmd
    {
    public:
      SetWakeUpTimeCmd()
        :SetTimeCmd(1, 2)
      {}
    };

    ///////////////
    class GetPwrOffTimeCmd : public GetDateTimeCmd
    {
    public:
      GetPwrOffTimeCmd()
        :GetDateTimeCmd(1, 3)
      {}
    };

    ///////////////
    class GetWakeUpTimeCmd : public GetDateTimeCmd
    {
    public:
      GetWakeUpTimeCmd()
        :GetDateTimeCmd(1, 4)
      {}
    };

    ///////////////
    class SetRTCTimeCmd : public SetTimeCmd
    {
    public:
      SetRTCTimeCmd()
        :SetTimeCmd(2, 1)
      {}
    };

    ///////////////
    class SetRTCDateCmd : public OffGridCmd
    {
    public:
      SetRTCDateCmd()
        :OffGridCmd(2, 2)
      {}

      void setDate(const std::string & dateStr) {
        return m_date.setDate(dateStr);
      }

    protected:
      virtual void encode(std::vector<uint8_t> & vect) const {
        m_date.serialize(vect);
      }

    private:
      Date m_date;
    };

    ///////////////
    class GetRTCTimeCmd : public GetTimeCmd
    {
    public:
      GetRTCTimeCmd()
        :GetTimeCmd(2, 3)
      {}
    };

    ///////////////
    class GetRTCDateCmd : public OffGridCmd
    {
    public:
      GetRTCDateCmd()
        :OffGridCmd(2, 4)
      {}

      std::string getDate() {
        return m_date.getDate();
      }

    protected:
      virtual void parse(std::vector<uint8_t>::iterator & pos, const std::vector<uint8_t>::iterator end)
      {
        m_date.deserialize(pos, end);
      };

    private:
      DateWithWeek m_date;
    };

    ///////////////
    // Common base to get 2B number
    class GetNumCmd : public OffGridCmd
    {
    public:
      GetNumCmd(uint8_t pnum, uint8_t pcmd)
        :OffGridCmd(pnum, pcmd)
        , m_number(0)
        , m_hbyte(0)
        , m_lbyte(0)
      {}

    protected:
      virtual void parse(std::vector<uint8_t>::iterator & pos, const std::vector<uint8_t>::iterator end)
      {
        if (end - pos < 2) {
          THROW_EXC_TRC_WAR(std::out_of_range, "provided buffer is too short");
        }
        m_lbyte = *pos++;
        m_hbyte = *pos++;
        m_number = m_lbyte;
        m_number += m_hbyte << 8;
      };

      int16_t m_number;
      uint8_t m_hbyte;
      uint8_t m_lbyte;
    };

    class GetVoltageCmd : public GetNumCmd
    {
    public:
      GetVoltageCmd()
        :GetNumCmd(3, 1)
      {}

      float getVoltage() { return (float)m_number / 1000.0f; }
    };

    class GetCurrentCmd : public GetNumCmd
    {
    public:
      GetCurrentCmd()
        :GetNumCmd(3, 2)
      {}

      float getCurrent() { return (float)m_number / 1000.0f; }
    };

    class GetPowerCmd : public GetNumCmd
    {
    public:
      GetPowerCmd()
        :GetNumCmd(3, 3)
      {}

      float getPower() { return (float)m_number / 4.0f; }
    };

    class GetTemperatureCmd : public GetNumCmd
    {
    public:
      GetTemperatureCmd()
        :GetNumCmd(3, 4)
      {}

      float getTemperature() { return (float)m_number / 16.0f; }
    };

    class GetRepCapCmd : public GetNumCmd
    {
    public:
      GetRepCapCmd()
        :GetNumCmd(3, 5)
      {}

      int getRepCap() { return m_number; }
    };

    class GetRepSocCmd : public GetNumCmd
    {
    public:
      GetRepSocCmd()
        :GetNumCmd(3, 6)
      {}

      int getRepSoc() { return m_number; }
    };

    class GetTteCmd : public GetNumCmd
    {
    public:
      GetTteCmd()
        :GetNumCmd(3, 7)
      {}

      int getSeconds() { return m_number; }
    };

    class GetTtfCmd : public GetNumCmd
    {
    public:
      GetTtfCmd()
        :GetNumCmd(3, 8)
      {}

      int getSeconds() { return m_number; }
    };

    /////////////////////
    // common base for device power get state
    class GetDeviceStateCmd : public OffGridCmd
    {
    public:
      GetDeviceStateCmd(uint8_t pnum, uint8_t pcmd)
        :OffGridCmd(pnum, pcmd)
        , m_state(false)
      {}

      bool getState() const { return m_state; }

    protected:
      virtual void parse(std::vector<uint8_t>::iterator & pos, const std::vector<uint8_t>::iterator end)
      {
        if (end - pos < 1) {
          THROW_EXC_TRC_WAR(std::out_of_range, "provided buffer is too short");
        }
        m_state = (bool)(*pos++ ? true : false);
      };

      bool m_state;
    };

    class SetLteOnCmd : public OffGridCmd
    {
    public:
      SetLteOnCmd()
        :OffGridCmd(4, 1)
      {}
    };

    class SetLteOffCmd : public OffGridCmd
    {
    public:
      SetLteOffCmd()
        :OffGridCmd(4, 2)
      {}
    };

    class GetLteStateCmd : public GetDeviceStateCmd
    {
    public:
      GetLteStateCmd()
        :GetDeviceStateCmd(4, 3)
      {}
    };

    class SetLoraOnCmd : public OffGridCmd
    {
    public:
      SetLoraOnCmd()
        :OffGridCmd(5, 1)
      {}
    };

    class SetLoraOffCmd : public OffGridCmd
    {
    public:
      SetLoraOffCmd()
        :OffGridCmd(5, 2)
      {}
    };

    class GetLoraStateCmd : public GetDeviceStateCmd
    {
    public:
      GetLoraStateCmd()
        :GetDeviceStateCmd(5, 3)
      {}
    };

    class SendLoraAtCmd : public OffGridCmd
    {
    public:
      SendLoraAtCmd()
        :OffGridCmd(5, 4)
      {}

      const std::string& getAt() const {
        return m_at;
      }

      void setAt(const std::string& at) {
        m_at = at;
      }

    protected:
      virtual void parse(std::vector<uint8_t>::iterator& pos, const std::vector<uint8_t>::iterator end)
      {
        m_at = std::string(pos, end);
      };

      virtual void encode(std::vector<uint8_t>& vect) const {
        vect.insert(std::end(vect), std::begin(m_at), std::end(m_at));
      }

    private:
      std::string m_at;
    };

    class ReceiveLoraAtCmd : public OffGridCmd
    {
    public:
      ReceiveLoraAtCmd()
        :OffGridCmd(5, 5)
      {}

      const std::string& getAt() const {
        return m_at;
      }

    protected:
      virtual void parse(std::vector<uint8_t>::iterator& pos, const std::vector<uint8_t>::iterator end)
      {
        m_at.insert(std::end(m_at), pos, end);
      };

    private:
      std::string m_at;
    };

    class GetVerCmd : public GetNumCmd
    {
    public:
      GetVerCmd()
        :GetNumCmd(0x20, 1)
      {}

      std::string getVersion() { 
        std::ostringstream os;
        os << (int)m_hbyte << '.' << std::setfill('0') << std::setw(2) << (int)m_lbyte;
        return os.str();
      }
    };
  }
}
