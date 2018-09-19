#pragma once

#include "ShapeDefines.h"
#include <map>
#include <string>
#include <functional>
#include <vector>

#ifdef IJsCacheService_EXPORTS
#define IJsCacheService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IJsCacheService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  class IJsCacheService_DECLSPEC IJsCacheService
  {
  public:
    class Company
    {
    public:
      Company() = delete;
      Company(int companyId, const std::string& name, const std::string& homePage)
        :m_companyId(companyId), m_name(name), m_homePage(homePage)
      {}
      int m_companyId;
      std::string m_name;
      std::string m_homePage;
    };

    class Manufacturer
    {
    public:
      Manufacturer() = delete;
      Manufacturer(int manufacturerId, int companyId, const std::string& name)
        :m_manufacturerId(manufacturerId), m_companyId(companyId), m_name(name)
      {}
      int m_manufacturerId;
      int m_companyId;
      std::string m_name;
    };

    class Product
    {
    public:
      Product() = delete;
      Product(int hwpid, int manufacturerId, const std::string& name, const std::string& homePage, const std::string& picture)
        :m_hwpid(hwpid), m_manufacturerId(manufacturerId), m_name(name), m_homePage(homePage), m_picture(picture)
      {}
      int m_hwpid;
      int m_manufacturerId;
      std::string m_name;
      std::string m_homePage;
      std::string m_picture;
    };

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

    class Package
    {
    public:
      Package() {}
      bool m_valid = false;
      int m_packageId = -1;
      int m_hwpid = -1;
      int m_hwpidVer = -1;
      std::string m_handlerUrl;
      std::string m_handlerHex;
      bool m_handlerValid = false;
      std::string m_handlerHash;
      std::string m_os;
      std::string m_dpa;
      std::string m_notes;
      std::string m_driver;
      std::vector<const StdDriver*> m_stdDriverVect;
    };

    class OsDpa
    {
    public:
      OsDpa() = delete;
      OsDpa(int osdpaId, const std::string& os, const std::string& dpa, const std::string& notes)
        :m_osdpaId(osdpaId), m_os(os), m_dpa(dpa), m_notes(notes)
      {}
      int m_osdpaId;
      std::string m_os;
      std::string m_dpa;
      std::string m_notes;
    };

    class ServerState
    {
    public:
      ServerState() {}
      int m_apiVersion = -1;
      std::string m_hostname;
      std::string m_user;
      std::string m_buildDateTime;
      std::string m_startDateTime;
      std::string m_dateTime;
      int64_t m_databaseChecksum = -1;
      std::string m_databaseChangeDateTime;
    };

    typedef std::function<void(int statusCode, const std::string & data)> DataHandlerFunc;

    //TODO change to return by value as poineters are dangerous in case of cache update
    virtual const std::string& getDriver(int id, int ver) const = 0;
    virtual const Manufacturer* getManufacturer(uint16_t hwpid) const = 0;
    virtual const Product* getProduct(uint16_t hwpid) const = 0;
    virtual const Package* getPackage(uint16_t hwpid, const std::string& os, const std::string& dpa) const = 0;
    virtual const OsDpa* getOsDpa(int id) const = 0;
    virtual ServerState getServerState() const = 0;

    virtual ~IJsCacheService() {};
  };


}
