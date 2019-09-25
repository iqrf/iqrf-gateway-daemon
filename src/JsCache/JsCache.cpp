#define IJsCacheService_EXPORTS
#define ISchedulerService_EXPORTS

#include "JsCache.h"
#include "EmbedExplore.h"
#include "EmbedOS.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/reader.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include "Trace.h"
#include <map>
#include <fstream>
#include <exception>
#include <iostream>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 33

#include "iqrf__JsCache.hxx"

TRC_INIT_MODULE(iqrf::JsCache);

namespace iqrf {

  static const char* COMPANIES_URL = "companies";
  static const char* MANUFACTURERS_URL = "manufacturers";
  static const char* PRODUCTS_URL = "products";
  static const char* OSDPA_URL = "osdpa";
  static const char* SERVER_URL = "server";
  static const char* STANDARDS_URL = "standards";
  static const char* PACKAGES_URL = "packages";

  class StdItem
  {
  public:
    StdItem() = delete;
    StdItem(const std::string& name)
      :m_name(name)
    {}
    StdItem(const std::string& name, const std::map<int, IJsCacheService::StdDriver>& drvs)
      :m_name(name), m_drivers(drvs)
    {}
    bool m_valid = false;
    std::string m_name;
    std::map<int, IJsCacheService::StdDriver> m_drivers;
  };

  class JsCache::Imp
  {
  private:
    iqrf::IIqrfDpaService* m_iIqrfDpaService = nullptr;
    iqrf::IJsRenderService* m_iJsRenderService = nullptr;
    iqrf::ISchedulerService* m_iSchedulerService = nullptr;
    shape::ILaunchService* m_iLaunchService = nullptr;
    shape::IRestApiService* m_iRestApiService = nullptr;

    mutable std::recursive_mutex m_updateMtx;
    std::string m_cacheDir = "";
    int m_cacheDirSet = 0;
    std::string m_urlRepo = "https://repository.iqrfalliance.org/api";
    std::string m_iqrfRepoCache = "iqrfRepoCache";
    double m_checkPeriodInMinutes = 0;

#ifdef _DEBUG
    const double m_checkPeriodInMinutesMin = 0.01;
#else
    const double m_checkPeriodInMinutesMin = 1;
#endif
    std::string m_name = "JsCache";

    std::map<int, Company> m_companyMap;
    std::map<int, Manufacturer> m_manufacturerMap;
    std::map<int, Product> m_productMap;
    std::map<int, OsDpa> m_osDpaMap;
    ServerState m_serverState;
    std::map<int, Package> m_packageMap;
    std::map<int, StdItem> m_standardMap;

    //std::map<int, std::string>  m_customDrivers;

    bool m_upToDate = false;

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    const StdDriver* getDriver(int id, int ver) const
    {
      TRC_FUNCTION_ENTER(PAR(id) << PAR(ver));
      const StdDriver* drv = nullptr;
      auto foundDrv = m_standardMap.find(id);
      if (foundDrv != m_standardMap.end()) {
        const StdItem& stdItem = foundDrv->second;
        auto foundVer = stdItem.m_drivers.find(ver);
        if (foundVer != stdItem.m_drivers.end()) {
          drv = &foundVer->second;
        }
      }
      TRC_FUNCTION_LEAVE(PAR(drv));
      return drv;
    }

    const Manufacturer* getManufacturer(uint16_t hwpid) const
    {
      TRC_FUNCTION_ENTER(PAR(hwpid));

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      const Manufacturer* retval = nullptr;
      auto found = m_productMap.find(hwpid);
      if (found != m_productMap.end()) {
        int manufacturerId = found->second.m_manufacturerId;
        auto foundManuf = m_manufacturerMap.find(manufacturerId);
        if (foundManuf != m_manufacturerMap.end()) {
          retval = &(foundManuf->second);
        }
      }

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    const Product* getProduct(uint16_t hwpid) const
    {
      TRC_FUNCTION_ENTER(PAR(hwpid));

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      const Product* retval = nullptr;
      auto found = m_productMap.find(hwpid);
      if (found != m_productMap.end()) {
        retval = &(found->second);
      }

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    const Package* getPackage(uint16_t hwpid, uint16_t hwpidVer, const std::string& os, const std::string& dpa) const
    {
      TRC_FUNCTION_ENTER(PAR(hwpid) << PAR(hwpidVer) << PAR(os) << PAR(dpa));

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      const Package* retval = nullptr;
      for (const auto & pck : m_packageMap) {
        const Package& pckp = pck.second;
        if (pckp.m_hwpid == hwpid && pckp.m_hwpidVer == hwpidVer && pckp.m_os == os && pckp.m_dpa == dpa) {
          retval = &(pck.second);
          break;
        }
      }

      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    const Package* getPackage(uint16_t hwpid, uint16_t hwpidVer, uint16_t os, uint16_t dpa) const
    {
      TRC_FUNCTION_ENTER(PAR(hwpid) << PAR(hwpidVer) << PAR(os) << PAR(dpa));

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      const Package* retval = nullptr;
      for (const auto & pck : m_packageMap) {
        const Package& pckp = pck.second;
        if (pckp.m_hwpid == hwpid && pckp.m_hwpidVer == hwpidVer &&
          pckp.m_os == embed::os::Read::getOsBuildAsString(os) && pckp.m_dpa == embed::explore::Enumerate::getDpaVerAsHexaString(dpa)) {
          retval = &(pck.second);
          break;
        }
      }

      TRC_FUNCTION_LEAVE(PAR(retval));
      return retval;
    }

    std::map<int, std::map<int, std::vector<std::pair<int, int>>>> getDrivers(const std::string& os, const std::string& dpa)
    {
      TRC_FUNCTION_ENTER(PAR(os) << PAR(dpa));

      //DriverId, DriverVersion, hwpid, hwpidVer
      std::map<int, std::map<int, std::vector<std::pair<int, int>>>> map2;

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      std::ostringstream ostr;
      for (const auto & pck : m_packageMap) {
        const Package& p = pck.second;
        if (p.m_os == os && p.m_dpa == dpa) {
          for (const auto & drv : p.m_stdDriverVect) {
            map2[drv->getId()][drv->getVersion()].push_back(std::make_pair(p.m_hwpid, p.m_hwpidVer));
            ostr << '[' << drv->getId() << ',' << drv->getVersion() << "] ";
          }
        }
      }

      TRC_INFORMATION("Loading drivers: " << ostr.str());
      TRC_FUNCTION_LEAVE("");
      return map2;
    }

    // get non empty custom drivers
    std::map<int, std::map<int,std::string>> getCustomDrivers(const std::string& os, const std::string& dpa)
    {
      TRC_FUNCTION_ENTER(PAR(os) << PAR(dpa));

      //hwpid, hwpidVer, driver
      std::map<int, std::map<int, std::string>> map2;

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      for (const auto & pck : m_packageMap) {
        const Package& p = pck.second;
        if (p.m_os == os && p.m_dpa == dpa) {
          if (!p.m_driver.empty() && p.m_driver.size() > 20) {
            map2[p.m_hwpid].insert(std::make_pair(p.m_hwpidVer, p.m_driver));
          }
        }
      }

      TRC_FUNCTION_LEAVE("");
      return map2;
    }

    const IJsCacheService::OsDpa* getOsDpa(int id) const
    {
      TRC_FUNCTION_ENTER(PAR(id));

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      const OsDpa* retval = nullptr;
      auto found = m_osDpaMap.find(id);
      if (found != m_osDpaMap.end()) {
        retval = &(found->second);
      }

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    const IJsCacheService::OsDpa* getOsDpa(const std::string& os, const std::string& dpa) const
    {
      TRC_FUNCTION_ENTER(PAR(os) << PAR(dpa));

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      const OsDpa* retval = nullptr;
      for (auto & a : m_osDpaMap) {
        if (os == a.second.m_os && dpa == a.second.m_dpa) {
          retval = &a.second;
          break;
        }
      }

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    ServerState getServerState() const
    {
      TRC_FUNCTION_ENTER("");

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      TRC_FUNCTION_LEAVE("");
      return m_serverState;
    }

    const StdDriver* getStandard(int standardId, int version)
    {
      TRC_FUNCTION_ENTER(PAR(standardId) << PAR(version));

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      const StdDriver *retval = nullptr;
      auto found = m_standardMap.find(standardId);
      if (found != m_standardMap.end()) {
        auto foundVer = found->second.m_drivers.find(version);
        if (foundVer != found->second.m_drivers.end()) {
          retval = &(foundVer->second);
        }
      }

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    bool parseFromFile(const std::string& path, rapidjson::Document& doc)
    {
      using namespace rapidjson;
      TRC_FUNCTION_ENTER("Loading: " << path);
      //std::cout << "Loading: " << path << std::endl;

      std::ifstream ifs(path);
      IStreamWrapper isw(ifs);
      doc.ParseStream(isw);
      if (doc.HasParseError()) {
        TRC_WARNING("Json parse error: " << NAME_PAR(emsg, doc.GetParseError()) <<
          NAME_PAR(eoffset, doc.GetErrorOffset()));
        return false;
      }
      else {
        return true;
      }
      TRC_FUNCTION_LEAVE("")
    }

    void createPathFile(const std::string& path)
    {
      boost::filesystem::path createdFile(path);
      boost::filesystem::path parent(createdFile.parent_path());

      try {
        if (!(boost::filesystem::exists(parent))) {
          if (boost::filesystem::create_directories(parent)) {
            TRC_DEBUG("Created: " << PAR(parent));
          }
          else {
            TRC_DEBUG("Cannot create: " << PAR(parent));
          }
        }
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "cannot create: " << PAR(parent));
      }
    }

    bool parseStr(const std::string& str, rapidjson::Document& doc)
    {
      using namespace rapidjson;
      StringStream sstr(str.data());
      doc.ParseStream(sstr);
      if (doc.HasParseError()) {
        TRC_WARNING("Json parse error: " << NAME_PAR(emsg, doc.GetParseError()) <<
          NAME_PAR(eoffset, doc.GetErrorOffset()));
        return false;
      }
      else {
        return true;
      }
    }

    void loadCache()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;
      using namespace boost;

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      try {
        TRC_INFORMATION("Loading cache ... ");
        std::cout << "Loading cache ... " << std::endl;

        updateCacheServer();
        updateCacheCompany();
        updateCacheManufacturer();
        updateCacheProduct();
        updateCacheOsdpa();
        updateCacheStandard();
        updateCachePackage();

        m_upToDate = true;
        TRC_INFORMATION("Loading cache success");
        std::cout << "Loading cache success" << std::endl;
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::logic_error, e, "Loading cache failed");
        std::cerr << "Loading cache failed: " << e.what() << std::endl;
      }

      TRC_FUNCTION_LEAVE("")
    }

#define POINTER_GET_STRING(iterKey, iter, key, val, fileName) \
    { rapidjson::Value* jsVal = nullptr; \
    if ((jsVal = rapidjson::Pointer(key).Get(*iter)) && jsVal->IsString()) { \
        val = jsVal->GetString(); \
    } \
    else { \
          THROW_EXC(std::logic_error, "parse error " << iterKey << '/' << key << PAR(fileName)); \
    } }

#define POINTER_GET_INT(iterKey, iter, key, val, fileName) \
    { rapidjson::Value* jsVal = nullptr; \
    if ((jsVal = rapidjson::Pointer(key).Get(*iter)) && jsVal->IsNumber()) { \
        val = jsVal->GetInt(); \
    } \
    else { \
          THROW_EXC(std::logic_error, "parse error " << iterKey << '/' << key << PAR(fileName)); \
    } }

#define POINTER_GET_DOUBLE(iterKey, iter, key, val, fileName) \
    { rapidjson::Value* jsVal = nullptr; \
    if ((jsVal = rapidjson::Pointer(key).Get(*iter)) && jsVal->IsDouble()) { \
        val = jsVal->GetDouble(); \
    } \
    else { \
          THROW_EXC(std::logic_error, "parse error " << iterKey << '/' << key << PAR(fileName)); \
    } }

#define POINTER_GET_INT64(iterKey, iter, key, val, fileName) \
    { rapidjson::Value* jsVal = nullptr; \
    if ((jsVal = rapidjson::Pointer(key).Get(*iter)) && jsVal->IsInt64()) { \
        val = jsVal->GetInt64(); \
    } \
    else { \
          THROW_EXC(std::logic_error, "parse error " << iterKey << '/' << key << PAR(fileName)); \
    } }

    void updateCacheCompany()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;
      using namespace boost;

      std::string fname = getDataLocalFileName(COMPANIES_URL);

      if (!filesystem::exists(fname)) {
        downloadData(COMPANIES_URL);
      }

      if (filesystem::exists(fname)) {
        Document doc;
        if (parseFromFile(fname, doc)) {
          std::map<int, Company> companyMap;
          int companyId = -1;
          std::string name;
          std::string homePage;
          if (doc.IsArray()) {
            for (auto itr = doc.Begin(); itr != doc.End(); ++itr) {
              POINTER_GET_INT(COMPANIES_URL, itr, "/companyID", companyId, fname);
              POINTER_GET_STRING(COMPANIES_URL, itr, "/name", name, fname);
              POINTER_GET_STRING(COMPANIES_URL, itr, "/homePage", homePage, fname);
              companyMap.insert(std::make_pair(companyId, Company(companyId, name, homePage)));
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error not array: " << PAR(COMPANIES_URL) << PAR(fname));
          }
          m_companyMap = companyMap;
        }
        else {
          THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
        }
      }
      else {
        THROW_EXC(std::logic_error, "file not exist " << PAR(fname));
      }

      TRC_FUNCTION_LEAVE("")
    }

    void updateCacheManufacturer()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;
      using namespace boost;

      std::string fname = getDataLocalFileName(MANUFACTURERS_URL);

      if (!filesystem::exists(fname)) {
        downloadData(MANUFACTURERS_URL);
      }

      if (filesystem::exists(fname)) {
        Document doc;
        if (parseFromFile(fname, doc)) {
          std::map<int, Manufacturer> manufacturerMap;
          int manufacturerId = -1;
          int companyId = -1;
          std::string name;
          if (doc.IsArray()) {
            for (auto itr = doc.Begin(); itr != doc.End(); ++itr) {
              POINTER_GET_INT(MANUFACTURERS_URL, itr, "/manufacturerID", manufacturerId, fname);
              POINTER_GET_INT(MANUFACTURERS_URL, itr, "/companyID", companyId, fname);
              POINTER_GET_STRING(MANUFACTURERS_URL, itr, "/name", name, fname);
              manufacturerMap.insert(std::make_pair(manufacturerId, Manufacturer(manufacturerId, companyId, name)));
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error not array: " << PAR(MANUFACTURERS_URL) << PAR(fname));
          }
          m_manufacturerMap = manufacturerMap;
        }
        else {
          THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
        }
      }
      else {
        THROW_EXC(std::logic_error, "file not exist " << PAR(fname));
      }

      TRC_FUNCTION_LEAVE("")
    }

    void updateCacheProduct()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;
      using namespace boost;

      std::string fname = getDataLocalFileName(PRODUCTS_URL);

      if (!filesystem::exists(fname)) {
        downloadData(PRODUCTS_URL);
      }

      if (filesystem::exists(fname)) {
        Document doc;
        if (parseFromFile(fname, doc)) {
          std::map<int, Product> productMap;
          int hwpid = -1;
          int manufacturerId = -1;
          std::string name;
          std::string homePage;
          std::string picture;
          if (doc.IsArray()) {
            for (auto itr = doc.Begin(); itr != doc.End(); ++itr) {
              POINTER_GET_INT(PRODUCTS_URL, itr, "/hwpid", hwpid, fname);
              POINTER_GET_INT(PRODUCTS_URL, itr, "/manufacturerID", manufacturerId, fname);
              POINTER_GET_STRING(PRODUCTS_URL, itr, "/name", name, fname);
              POINTER_GET_STRING(PRODUCTS_URL, itr, "/homePage", homePage, fname);
              POINTER_GET_STRING(PRODUCTS_URL, itr, "/picture", picture, fname);
              productMap.insert(std::make_pair(hwpid, Product(hwpid, manufacturerId, name, homePage, picture)));
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error not array: " << PAR(PRODUCTS_URL) << PAR(fname));
          }
          m_productMap = productMap;
        }
        else {
          THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
        }
      }
      else {
        THROW_EXC(std::logic_error, "file not exist " << PAR(fname));
      }

      TRC_FUNCTION_LEAVE("")
    }

    void updateCacheOsdpa()
    {
      TRC_FUNCTION_LEAVE("");

      using namespace rapidjson;
      using namespace boost;

      std::string fname = getDataLocalFileName(OSDPA_URL);

      if (!filesystem::exists(fname)) {
        downloadData(OSDPA_URL);
      }

      if (filesystem::exists(fname)) {
        Document doc;
        if (parseFromFile(fname, doc)) {
          std::string os;
          std::string dpa;
          std::string notes;
          std::map<int, OsDpa> osDpaMap;
          if (doc.IsArray()) {
            int idx = 0;
            for (auto itr = doc.Begin(); itr != doc.End(); ++itr) {
              POINTER_GET_STRING(OSDPA_URL, itr, "/os", os, fname);
              POINTER_GET_STRING(OSDPA_URL, itr, "/dpa", dpa, fname);
              POINTER_GET_STRING(OSDPA_URL, itr, "/notes", notes, fname);
              osDpaMap.insert(std::make_pair(idx, OsDpa(idx, os, dpa, notes)));
              ++idx;
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error not array: " << PAR(OSDPA_URL) << PAR(fname));
          }
          m_osDpaMap = osDpaMap;
        }
        else {
          THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
        }
      }
      else {
        THROW_EXC(std::logic_error, "file not exist " << PAR(fname));
      }

      TRC_FUNCTION_LEAVE("")
    }

    ServerState getCacheServer(const std::string & dataFile = "data.json")
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;
      using namespace boost;

      ServerState retval;

      std::string fname = getDataLocalFileName(SERVER_URL, dataFile);

      if (filesystem::exists(fname)) {
        Document doc;
        if (parseFromFile(fname, doc)) {
          POINTER_GET_INT(SERVER_URL, &doc, "/apiVersion", retval.m_apiVersion, fname);
          POINTER_GET_STRING(SERVER_URL, &doc, "/hostname", retval.m_hostname, fname);
          POINTER_GET_STRING(SERVER_URL, &doc, "/user", retval.m_user, fname);
          POINTER_GET_STRING(SERVER_URL, &doc, "/buildDateTime", retval.m_buildDateTime, fname);
          POINTER_GET_STRING(SERVER_URL, &doc, "/startDateTime", retval.m_startDateTime, fname);
          POINTER_GET_STRING(SERVER_URL, &doc, "/dateTime", retval.m_dateTime, fname);
          POINTER_GET_INT64(SERVER_URL, &doc, "/databaseChecksum", retval.m_databaseChecksum, fname);
          POINTER_GET_STRING(SERVER_URL, &doc, "/databaseChangeDateTime", retval.m_databaseChangeDateTime, fname);
        }
        else {
          THROW_EXC(std::logic_error, "parse error file: " << PAR(fname));
        }
      }
      else {
        THROW_EXC(std::logic_error, "cannot find file: " << PAR(fname));
      }

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    void checkCache()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("=============================================================" << std::endl <<
        "Checking Iqrf Repo for updates");
      //std::cout << "Checking Iqrf Repo for updates" << std::endl;

      using namespace boost;

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      ServerState serverStateOld = getCacheServer();

      downloadData(SERVER_URL, "dataCheck.json");

      m_serverState = getCacheServer("dataCheck.json");
      m_upToDate = m_serverState.m_databaseChecksum == serverStateOld.m_databaseChecksum;

      if (!m_upToDate) {
        TRC_INFORMATION("Iqrf Repo has been changed => reload");
        //std::cout << "Iqrf Repo has been changed => reload" << std::endl;
        filesystem::remove_all(m_cacheDir);
      }
      else {
        TRC_INFORMATION("Iqrf Repo is up to date");
        //std::cout << "Iqrf Repo is up to date" << std::endl;
      }

      TRC_FUNCTION_LEAVE(PAR(m_upToDate));
    }

    void updateCacheServer()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;
      using namespace boost;

      std::string fname = getDataLocalFileName(SERVER_URL);

      if (!filesystem::exists(fname)) {
        downloadData(SERVER_URL);
      }

      m_serverState = getCacheServer();

      TRC_FUNCTION_LEAVE("");
    }

    void updateCacheStandard()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;
      using namespace boost;

      std::string fname = getDataLocalFileName(STANDARDS_URL);

      if (!filesystem::exists(fname)) {
        downloadData("standards");
      }

      if (filesystem::exists(fname)) {
        std::map<int, StdItem> standardMap;
        Document doc;
        if (parseFromFile(fname, doc)) {
          int standardId = 0;
          std::string name;
          if (doc.IsArray()) {
            for (auto itr = doc.Begin(); itr != doc.End(); ++itr) {
              POINTER_GET_INT(STANDARDS_URL, itr, "/standardID", standardId, fname);
              POINTER_GET_STRING(STANDARDS_URL, itr, "/name", name, fname);
              standardMap.insert(std::make_pair(standardId, StdItem(name)));
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error not array: " << PAR(STANDARDS_URL) << PAR(fname));
          }
        }
        else {
          THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
        }
        m_standardMap = standardMap;
      }
      else {
        THROW_EXC(std::logic_error, "file not exist " << PAR(fname));
      }

      for (auto & stdItem : m_standardMap) { // 1

        std::ostringstream os;
        os << STANDARDS_URL << '/' << stdItem.first;
        std::string url = os.str();
        fname = getDataLocalFileName(url);

        if (!filesystem::exists(fname)) {
          downloadData(url);
        }

        if (filesystem::exists(fname)) {
          Document doc;
          if (parseFromFile(fname, doc)) {
            if (Value* versionArray = Pointer("/versions").Get(doc)) {
              if (versionArray->IsArray()) {
                for (auto itr = versionArray->Begin(); itr != versionArray->End(); ++itr) {
                  double version;
                  if (itr->IsDouble()) {
                    version = itr->GetDouble();
                  }
                  else {
                    THROW_EXC(std::logic_error, "parse error /versions[] item not double " << PAR(fname));
                  }
                  stdItem.second.m_drivers.insert(std::make_pair((int)version, StdDriver()));
                }
                stdItem.second.m_valid = true;
              }
              else {
                THROW_EXC(std::logic_error, "parse error /versions[] not array " << PAR(fname));
              }
            }
            else {
              THROW_EXC(std::logic_error, "parse error /versions[] not exist " << PAR(fname));
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
          }
        }
        else {
          THROW_EXC(std::logic_error, "file not exist " << PAR(fname));
        }
      } // for 1

      for (auto & stdItem : m_standardMap) { // 2
        for (auto & stdDriver : stdItem.second.m_drivers) { // 3

          std::ostringstream os;
          os << STANDARDS_URL << '/' << stdItem.first << '/' << stdDriver.first;
          std::string url = os.str();
          fname = getDataLocalFileName(url);

          if (!filesystem::exists(fname)) {
            downloadData(url);
          }

          if (filesystem::exists(fname)) {
            Document doc;
            if (parseFromFile(fname, doc)) {
              int versionFlag;
              double version;
              std::string driver, notes;
              POINTER_GET_DOUBLE("", &doc, "/version", version, fname);
              POINTER_GET_INT("", &doc, "/versionFlags", versionFlag, fname);
              POINTER_GET_STRING("", &doc, "/driver", driver, fname);
              POINTER_GET_STRING("", &doc, "/notes", notes, fname);
              stdDriver.second = StdDriver(stdItem.first, stdItem.second.m_name, version, driver, notes, versionFlag);
            }
            else {
              THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
            }
          }
          else {
            THROW_EXC(std::logic_error, "file not exist " << PAR(fname));
          }
        } // for 3
      } // for 2

      TRC_FUNCTION_LEAVE("")
    }

    void updateCachePackage()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;
      using namespace boost;

      std::string fname = getDataLocalFileName(PACKAGES_URL);

      if (!filesystem::exists(fname)) {
        downloadData(PACKAGES_URL);
      }

      if (filesystem::exists(fname)) {
        Document doc;
        if (parseFromFile(fname, doc)) {
          std::map<int, Package> packageMap;
          if (doc.IsArray()) {
            for (auto itr = doc.Begin(); itr != doc.End(); ++itr) {
              Package pck;
              POINTER_GET_INT(PACKAGES_URL, itr, "/packageID", pck.m_packageId, fname);
              POINTER_GET_INT(PACKAGES_URL, itr, "/hwpid", pck.m_hwpid, fname);
              POINTER_GET_INT(PACKAGES_URL, itr, "/hwpidVer", pck.m_hwpidVer, fname);
              POINTER_GET_STRING(PACKAGES_URL, itr, "/handlerUrl", pck.m_handlerUrl, fname);
              POINTER_GET_STRING(PACKAGES_URL, itr, "/handlerHash", pck.m_handlerHash, fname);
              POINTER_GET_STRING(PACKAGES_URL, itr, "/os", pck.m_os, fname);
              POINTER_GET_STRING(PACKAGES_URL, itr, "/dpa", pck.m_dpa, fname);
              packageMap.insert(std::make_pair(pck.m_packageId, pck));
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error not array: " << PAR(PACKAGES_URL) << PAR(fname));
          }
          m_packageMap = packageMap;
        }
        else {
          THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
        }
      }
      else {
        THROW_EXC(std::logic_error, "file not exist " << PAR(fname));
      }

      for (auto & pck : m_packageMap) { // 1

        std::ostringstream os;
        os << PACKAGES_URL << '/' << pck.first;
        std::string url = os.str();
        fname = getDataLocalFileName(url);

        if (!filesystem::exists(fname)) {
          downloadData(url);
        }

        if (filesystem::exists(fname)) {
          Document doc;
          if (parseFromFile(fname, doc)) {
            POINTER_GET_STRING("", &doc, "/driver", pck.second.m_driver, fname);
            POINTER_GET_STRING("", &doc, "/notes", pck.second.m_notes, fname);
            if (Value* standardArray = Pointer("/standards").Get(doc)) {
              if (standardArray->IsArray()) {
                int standardId = -10;
                double version = -1;
                for (auto itr = standardArray->Begin(); itr != standardArray->End(); ++itr) {
                  POINTER_GET_INT("", itr, "/standardID", standardId, fname);
                  POINTER_GET_DOUBLE("", itr, "/version", version, fname);
                  const StdDriver* stdDrv = getStandard(standardId, static_cast<int>(version));
                  if (stdDrv) {
                    pck.second.m_stdDriverVect.push_back(stdDrv);
                  }
                }
                pck.second.m_valid = true;
              }
              else {
                THROW_EXC(std::logic_error, "parse error not array: " << PAR(PACKAGES_URL) << "/standards" << PAR(fname));
              }
            }
            else {
              THROW_EXC(std::logic_error, "parse error /standards not exist " << PAR(fname));
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
          }
        }
        else {
          THROW_EXC(std::logic_error, "file not exist " << PAR(fname));
        }
      } // for 1

      for (auto & pck : m_packageMap) { // 2

        std::ostringstream os;
        os << PACKAGES_URL << '/' << pck.first;
        std::string url = os.str();
        std::string fname = getDataLocalFileName(url, "handler.hex");

        if (std::string::npos != pck.second.m_handlerUrl.find("https://")) {
          if (!filesystem::exists(fname)) {
            downloadFile(pck.second.m_handlerUrl, fname);
          }

          if (filesystem::exists(fname)) {
            std::ifstream file(fname);
            if (file.is_open()) {
              std::ostringstream strStream;
              strStream << file.rdbuf();
              pck.second.m_handlerHex = strStream.str();
            }
            else {
              THROW_EXC_TRC_WAR(std::logic_error, "Cannot open: " << PAR(fname));
            }
          }
          else {
            THROW_EXC(std::logic_error, "file not exist " << PAR(fname));
          }
        }

      } // for 2

      TRC_FUNCTION_LEAVE("")
    }

    std::string getDataLocalFileName(const std::string& relativeUrl, const std::string& fname = "data.json")
    {
      std::ostringstream os;
      os << m_cacheDir << '/' << relativeUrl << '/' << fname;
      return os.str();
    }

    std::string getDataAbsoluteUrl(const std::string& relativeUrl)
    {
      std::ostringstream os;
      os << m_urlRepo << '/' << relativeUrl;
      return os.str();
    }

    // loads data from REST API with url relative to root Repo URL
    // and stores to local repo cache to relative path
    void downloadData(const std::string& relativeUrl, const std::string& fname = "data.json")
    {
      TRC_FUNCTION_ENTER(PAR(relativeUrl) << PAR(fname));
      std::string pathLoading = getDataLocalFileName(relativeUrl, fname);
      createPathFile(pathLoading);

      std::string urlLoading;

      //using repo undocumented feature for test
      //if (relativeUrl == "server") {
      //  urlLoading = getDataAbsoluteUrl("debug/httpstatus/503");
      //}
      //else {
      //  urlLoading = getDataAbsoluteUrl(relativeUrl);
      //}
      urlLoading = getDataAbsoluteUrl(relativeUrl);

      TRC_DEBUG("Getting: " << PAR(urlLoading));
      //std::cout << "Getting: " << urlLoading << std::endl;

      try {
        boost::filesystem::path getFile(pathLoading);
        boost::filesystem::path downloadFile(pathLoading);
        downloadFile += ".download";
        boost::filesystem::remove(downloadFile);

        m_iRestApiService->getFile(urlLoading, downloadFile.string());

        boost::filesystem::copy_file(downloadFile, getFile, boost::filesystem::copy_option::overwrite_if_exists);
      }
      catch (boost::filesystem::filesystem_error& e) {
        CATCH_EXC_TRC_WAR(boost::filesystem::filesystem_error, e, "cannot get " << PAR(fname));
        throw e;
      }

      TRC_FUNCTION_LEAVE("")
    }


    // loads file from exact URL
    // and stores to local repo cache to relative path
    void downloadFile(const std::string& fileUrl, const std::string& urlFname)
    {
      TRC_FUNCTION_ENTER(PAR(fileUrl) << PAR(urlFname));
      createPathFile(urlFname);

      std::string urlLoading = fileUrl;

      TRC_DEBUG("Getting: " << PAR(urlLoading));
      //std::cout << "Getting: " << urlLoading << std::endl;

      try {
        boost::filesystem::path getFile(urlFname);
        boost::filesystem::path downloadFile(urlFname);
        downloadFile += ".download";
        boost::filesystem::remove(downloadFile);

        m_iRestApiService->getFile(urlLoading, downloadFile.string());

        boost::filesystem::copy_file(downloadFile, getFile, boost::filesystem::copy_option::overwrite_if_exists);
      }
      catch (boost::filesystem::filesystem_error& e) {
        CATCH_EXC_TRC_WAR(boost::filesystem::filesystem_error, e, "cannot get " << PAR(urlFname));
        throw e;
      }

      TRC_FUNCTION_LEAVE("")
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsCache instance activate" << std::endl <<
        "******************************"
      );

      modify(props);

      loadCache();

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");

      m_iSchedulerService->removeAllMyTasks(m_name);
      m_iSchedulerService->unregisterTaskHandler(m_name);

      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsCache instance deactivate" << std::endl <<
        "******************************"
      );

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      using namespace rapidjson;
      const std::string CHECK_CACHE("checkCache");
      const Document& doc = props->getAsJson();

      m_iSchedulerService->removeAllMyTasks(m_name);
      m_iSchedulerService->unregisterTaskHandler(m_name);

      const Value* v = Pointer("/instance").Get(doc);
      if (v && v->IsString()) {
        m_name = v->GetString();
      }
      v = Pointer("/iqrfRepoCache").Get(doc);
      if (v && v->IsString()) {
        m_iqrfRepoCache = v->GetString();
      }
      v = Pointer("/urlRepo").Get(doc);
      if (v && v->IsString()) {
        m_urlRepo = v->GetString();
      }
      v = Pointer("/checkPeriodInMinutes").Get(doc);
      if (v && v->IsNumber()) {
        m_checkPeriodInMinutes = v->GetDouble();
        if (m_checkPeriodInMinutes > 0 && m_checkPeriodInMinutes < m_checkPeriodInMinutesMin) {
          TRC_WARNING(PAR(m_checkPeriodInMinutes) << " from configuration forced to: " << PAR(m_checkPeriodInMinutesMin));
          m_checkPeriodInMinutes = m_checkPeriodInMinutesMin;
        }
      }

      m_cacheDir = m_iLaunchService->getCacheDir() + "/" + m_iqrfRepoCache;
      TRC_DEBUG("Using cache directory: " << PAR(m_cacheDir))

        m_iSchedulerService->registerTaskHandler(m_name, [=](const rapidjson::Value & task)
      {
        if (task.IsString() && std::string(task.GetString()) == CHECK_CACHE) {
          try {
            checkCache();
            if (!m_upToDate) {
              loadCache();
            }
          }
          catch (std::exception& e) {
            CATCH_EXC_TRC_WAR(std::logic_error, e, std::endl << "Iqrf Repo download failure ... next attempt in " << m_checkPeriodInMinutes << " minutes");
            std::cerr << e.what() << std::endl << "Iqrf Repo download failure ... next attempt in " << m_checkPeriodInMinutes << " minutes" << std::endl;
          }
        }
      });

      if (m_checkPeriodInMinutes > 0) {
        int checkPeriodInSeconds = static_cast<int>(m_checkPeriodInMinutes * 60);
        Document task;
        task.SetString(CHECK_CACHE.c_str(), task.GetAllocator());
        auto tp = std::chrono::system_clock::now();
        tp += std::chrono::seconds(checkPeriodInSeconds);
        //delay 1.st period
        m_iSchedulerService->scheduleTaskPeriodic(m_name, task, std::chrono::seconds(checkPeriodInSeconds), tp);
        TRC_INFORMATION("Cache update scheduled: " << PAR(m_checkPeriodInMinutes));
      }
      else {
        TRC_INFORMATION("Cache update is not scheduled: " << PAR(m_checkPeriodInMinutes));
      }

    }

    void attachInterface(iqrf::IIqrfDpaService* iface)
    {
      m_iIqrfDpaService = iface;
    }

    void detachInterface(iqrf::IIqrfDpaService* iface)
    {
      if (m_iIqrfDpaService == iface) {
        m_iIqrfDpaService = nullptr;
      }
    }

    void attachInterface(iqrf::IJsRenderService* iface)
    {
      m_iJsRenderService = iface;
    }

    void detachInterface(iqrf::IJsRenderService* iface)
    {
      if (m_iJsRenderService == iface) {
        m_iJsRenderService = nullptr;
      }
    }

    void attachInterface(iqrf::ISchedulerService* iface)
    {
      m_iSchedulerService = iface;
    }

    void detachInterface(iqrf::ISchedulerService* iface)
    {
      if (m_iSchedulerService == iface) {
        m_iSchedulerService = nullptr;
      }
    }

    void attachInterface(shape::ILaunchService* iface)
    {
      m_iLaunchService = iface;
    }

    void detachInterface(shape::ILaunchService* iface)
    {
      if (m_iLaunchService == iface) {
        m_iLaunchService = nullptr;
      }
    }

    void attachInterface(shape::IRestApiService* iface)
    {
      m_iRestApiService = iface;
    }

    void detachInterface(shape::IRestApiService* iface)
    {
      if (m_iRestApiService == iface) {
        m_iRestApiService = nullptr;
      }
    }

  };

  //////////////////////////////////////////////////
  JsCache::JsCache()
  {
    m_imp = shape_new Imp();
  }

  JsCache::~JsCache()
  {
    delete m_imp;
  }

  const IJsCacheService::StdDriver* JsCache::getDriver(int id, int ver) const
  {
    return m_imp->getDriver(id, ver);
  }

  const IJsCacheService::Manufacturer* JsCache::getManufacturer(uint16_t hwpid) const
  {
    return m_imp->getManufacturer(hwpid);
  }

  const IJsCacheService::Product* JsCache::getProduct(uint16_t hwpid) const
  {
    return m_imp->getProduct(hwpid);
  }

  const IJsCacheService::Package* JsCache::getPackage(uint16_t hwpid, uint16_t hwpidVer, const std::string& os, const std::string& dpa) const
  {
    return m_imp->getPackage(hwpid, hwpidVer, os, dpa);
  }

  const IJsCacheService::Package* JsCache::getPackage(uint16_t hwpid, uint16_t hwpidVer, uint16_t os, uint16_t dpa) const
  {
    return m_imp->getPackage(hwpid, hwpidVer, os, dpa);
  }

  std::map<int, std::map<int, std::vector<std::pair<int, int>>>> JsCache::getDrivers(const std::string& os, const std::string& dpa) const
  {
    return m_imp->getDrivers(os, dpa);
  }

  std::map<int, std::map<int, std::string>> JsCache::getCustomDrivers(const std::string& os, const std::string& dpa) const
  {
    return m_imp->getCustomDrivers(os, dpa);
  }

  const IJsCacheService::OsDpa* JsCache::getOsDpa(int id) const
  {
    return m_imp->getOsDpa(id);
  }

  const IJsCacheService::OsDpa* JsCache::getOsDpa(const std::string& os, const std::string& dpa) const
  {
    return m_imp->getOsDpa(os, dpa);
  }

  IJsCacheService::ServerState JsCache::getServerState() const
  {
    return m_imp->getServerState();
  }

  void JsCache::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsCache::deactivate()
  {
    m_imp->deactivate();
  }

  void JsCache::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsCache::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsCache::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsCache::attachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsCache::detachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsCache::attachInterface(iqrf::ISchedulerService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsCache::detachInterface(iqrf::ISchedulerService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsCache::attachInterface(shape::IRestApiService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsCache::detachInterface(shape::IRestApiService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsCache::attachInterface(shape::ILaunchService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsCache::detachInterface(shape::ILaunchService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsCache::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsCache::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }
}
