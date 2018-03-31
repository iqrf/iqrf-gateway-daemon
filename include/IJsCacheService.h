#pragma once

#include "ShapeDefines.h"
#include <map>
#include <string>
#include <functional>

#ifdef IJsCacheService_EXPORTS
#define IJsCacheService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IJsCacheService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  class IJsCacheService_DECLSPEC IJsCacheService
  {
  public:
    class StdDriver
    {
    public:
      StdDriver()
        :m_valid(false)
      {}
      StdDriver(const std::string& name, const std::string& driver, const std::string& notes, int verFlags)
        :m_name(name), m_driver(driver), m_notes(notes), m_versionFlags(verFlags), m_valid(true)
      {}
      bool isValid() const { return m_valid; }
      const std::string& getName() const { return m_name; }
      const std::string& getDriver() const { return m_driver; }
      const std::string& getNotes() const { return m_notes; }
      int getVersionFlags() { return m_versionFlags; }
    private:
      bool m_valid = false;
      int m_versionFlags = 0;
      std::string m_name;
      std::string m_driver;
      std::string m_notes;
    };

    typedef std::function<void(int statusCode, const std::string & data)> DataHandlerFunc;

    virtual const std::string& getDriver(int id, int ver) const = 0;
    virtual const std::map<int, const StdDriver*> getAllLatestDrivers() const = 0;
    virtual const std::string& getManufacturer(uint16_t hwpid) const = 0;
    virtual const std::string& getProduct(uint16_t hwpid) const = 0;

    virtual ~IJsCacheService() {};
  };
}
