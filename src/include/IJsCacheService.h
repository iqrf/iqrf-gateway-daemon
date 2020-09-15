#pragma once

#include "ShapeDefines.h"
#include <map>
#include <set>
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
      Company()
        :m_companyId(-1)
      {}

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
      Manufacturer()
        :m_manufacturerId(-1), m_companyId(-1)
      {}

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
      Product()
        :m_hwpid(-1), m_manufacturerId(-1)
      {}

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
      StdDriver(int id, const std::string& name, double version, const std::string& driver, const std::string& notes, int verFlags)
        : m_valid(true)
        ,m_id(id)
        ,m_version(version)
        ,m_versionFlags(verFlags)
        ,m_name(name)
        ,m_driver(driver)
        ,m_notes(notes)
      {}
      bool isValid() const { return m_valid; }
      const std::string& getName() const { return m_name; }
      const std::string& getDriver() const { return m_driver; }
      const std::string& getNotes() const { return m_notes; }
      int getVersionFlags() const { return m_versionFlags; }
      double getVersion() const { return m_version; }
      int getId() const { return m_id; }
    private:
      bool m_valid = false;
      int m_id = 0;
      double m_version = 0;
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
      std::vector<StdDriver> m_stdDriverVect;
    };

    class OsDpa
    {
    public:
      OsDpa()
        :m_osdpaId(0)
      {}

      OsDpa(int osdpaId, const std::string& os, const std::string& dpa, const std::string& notes)
        :m_osdpaId(osdpaId), m_os(os), m_dpa(dpa), m_notes(notes)
      {}
      int m_osdpaId;
      std::string m_os;
      std::string m_dpa;
      std::string m_notes;
    };

    // get os map of dpa list as integers
    typedef std::map<int, std::set<int>> MapOsListDpa;

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

    //TODO change to return by value as poineters are dangerous in case of cache update
    virtual StdDriver getDriver(int id, double ver) const = 0;
    virtual Manufacturer getManufacturer(uint16_t hwpid) const = 0;
    virtual Product getProduct(uint16_t hwpid) const = 0;
    virtual Package getPackage(uint16_t hwpid, uint16_t hwpidVer, const std::string& os, const std::string& dpa) const = 0;
    virtual Package getPackage(uint16_t hwpid, uint16_t hwpidVer, uint16_t os, uint16_t dpa) const = 0;
    virtual std::map<int, std::map<double, std::vector<std::pair<int,int>>>> getDrivers(const std::string& os, const std::string& dpa) const = 0;
    virtual std::map<int, std::map<int, std::string>> getCustomDrivers(const std::string& os, const std::string& dpa) const = 0;
    virtual MapOsListDpa getOsDpa() const = 0;
    virtual OsDpa getOsDpa(int id) const = 0;
    virtual OsDpa getOsDpa(const std::string& os, const std::string& dpa) const = 0;
    virtual ServerState getServerState() const = 0;

    typedef std::function<void()> CacheReloadedFunc;
    virtual void registerCacheReloadedHandler(const std::string & clientId, CacheReloadedFunc hndl) = 0;
    virtual void unregisterCacheReloadedHandler(const std::string & clientId) = 0;

    virtual ~IJsCacheService() {};
  };


}
