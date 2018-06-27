#define IJsCacheService_EXPORTS
#define ISchedulerService_EXPORTS

#include "JsonApiMessageNames.h"
#include "JsCache.h"
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
#define TRC_CHANNEL 0

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
    shape::ILaunchService* m_iLaunchService = nullptr;
    iqrf::ISchedulerService* m_iSchedulerService = nullptr;
    shape::IRestApiService* m_iRestApiService = nullptr;

    mutable std::recursive_mutex m_updateMtx;
    std::string m_cacheDir = "";
    std::string m_urlRepo = "https://repository.iqrfalliance.org/api";
    int m_checkPeriodSec = 0;

    const int m_checkPeriodSecMin = 60;
    std::string m_name = "JsCache";

    std::map<int, Company> m_companyMap;
    std::map<int, Manufacturer> m_manufacturerMap;
    std::map<int, Product> m_productMap;
    std::map<int, OsDpa> m_osDpaMap;
    ServerState m_serverState;
    std::map<int, Package> m_packageMap;
    std::map<int, StdItem> m_standardMap;

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    const std::string& getDriver(int id, int ver) const
    {
      static std::string ni = "not implemented yet";
      return ni;
    }

    const std::map<int, const StdDriver*> getAllLatestDrivers() const
    {
      TRC_FUNCTION_ENTER("");

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      std::map<int, const StdDriver*> retval;
      for (const auto & stdItemPair : m_standardMap) {
        const StdItem& stdItem = stdItemPair.second;
        if (stdItem.m_valid && !stdItem.m_drivers.empty()) {
          const StdDriver& stdDriver = stdItem.m_drivers.crbegin()->second;
          retval.insert(std::make_pair(stdItemPair.first, &(stdDriver)));
        }
      }
      TRC_FUNCTION_LEAVE("");
      return retval;
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

    const Package* getPackage(uint16_t hwpid, const std::string& os, const std::string& dpa) const
    {
      TRC_FUNCTION_ENTER(PAR(hwpid));

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      const Package* retval = nullptr;
      for (const auto & pck : m_packageMap) {
        if (pck.second.m_os == os && pck.second.m_dpa == dpa) {
          retval = &(pck.second);
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
      std::cout << "Loading: " << path << std::endl;

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
      boost::filesystem::path dir(path);
      boost::filesystem::path parent(dir.parent_path());

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

    void encodeToFile(const std::string& path, rapidjson::Document& doc)
    {
      using namespace rapidjson;

      boost::filesystem::path dir(path);
      boost::filesystem::path parent(dir.parent_path());

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

      std::ofstream ofs(path);
      if (ofs.is_open()) {
        OStreamWrapper osw(ofs);
        PrettyWriter<OStreamWrapper> writer(osw);
        doc.Accept(writer);
        ofs.close();
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
        updateCacheServer();
        updateCacheCompany();
        updateCacheManufacturer();
        updateCacheProduct();
        updateCacheOsdpa();
        updateCacheStandard();
        updateCachePackage();
        TRC_INFORMATION("Iqrf Repo load success: ");
      }
      catch (std::logic_error &e) {
        CATCH_EXC_TRC_WAR(std::logic_error, e, "Cannot load Iqrf Repo");
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
          int companyId = -1;
          std::string name;
          std::string homePage;
          if (doc.IsArray()) {
            for (auto itr = doc.Begin(); itr != doc.End(); ++itr) {
              POINTER_GET_INT(COMPANIES_URL, itr, "/companyID", companyId, fname);
              POINTER_GET_STRING(COMPANIES_URL, itr, "/name", name, fname);
              POINTER_GET_STRING(COMPANIES_URL, itr, "/homePage", homePage, fname);
              m_companyMap.insert(std::make_pair(companyId, Company(companyId, name, homePage)));
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error not array: " << PAR(COMPANIES_URL) << PAR(fname));
          }
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
          int manufacturerId = -1;
          int companyId = -1;
          std::string name;
          if (doc.IsArray()) {
            for (auto itr = doc.Begin(); itr != doc.End(); ++itr) {
              POINTER_GET_INT(MANUFACTURERS_URL, itr, "/manufacturerID", manufacturerId, fname);
              POINTER_GET_INT(MANUFACTURERS_URL, itr, "/companyID", companyId, fname);
              POINTER_GET_STRING(MANUFACTURERS_URL, itr, "/name", name, fname);
              m_manufacturerMap.insert(std::make_pair(manufacturerId, Manufacturer(manufacturerId, companyId, name)));
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error not array: " << PAR(MANUFACTURERS_URL) << PAR(fname));
          }
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
              m_productMap.insert(std::make_pair(hwpid, Product(hwpid, manufacturerId, name, homePage, picture)));
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error not array: " << PAR(PRODUCTS_URL) << PAR(fname));
          }
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
          int osdpaId = -1;
          std::string os;
          std::string dpa;
          std::string notes;
          if (doc.IsArray()) {
            int idx = 0;
            for (auto itr = doc.Begin(); itr != doc.End(); ++itr) {
              POINTER_GET_STRING(OSDPA_URL, itr, "/os", os, fname);
              POINTER_GET_STRING(OSDPA_URL, itr, "/dpa", dpa, fname);
              POINTER_GET_STRING(OSDPA_URL, itr, "/notes", notes, fname);
              m_osDpaMap.insert(std::make_pair(idx, OsDpa(idx, os, dpa, notes)));
              ++idx;
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error not array: " << PAR(OSDPA_URL) << PAR(fname));
          }
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

    ServerState getCacheServer()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;
      using namespace boost;

      ServerState retval;
      
      std::string fname = getDataLocalFileName(SERVER_URL);

      if (filesystem::exists(fname)) {
        Document doc;
        if (parseFromFile(fname, doc)) {
          int apiVersion = -1;
          std::string hostname;
          std::string user;
          std::string buildDateTime;
          std::string startDateTime;
          std::string dateTime;
          int64_t databaseChecksum;
          std::string databaseChangeDateTime;
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
          THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
        }
      }

      TRC_FUNCTION_LEAVE("")
      return retval;
    }

    bool checkCache()
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;
      using namespace boost;
      bool result = false;

      std::lock_guard<std::recursive_mutex> lck(m_updateMtx);

      ServerState serverStateOld = getCacheServer();

      std::string fname = getDataLocalFileName(SERVER_URL);
      downloadData(SERVER_URL);

      if (!filesystem::exists(fname)) {
        TRC_WARNING("file not exist " << PAR(fname));
      }
      else {

        m_serverState = getCacheServer();

        result = m_serverState.m_databaseChecksum == serverStateOld.m_databaseChecksum;
        if (!result) {
          TRC_INFORMATION("Iqrf Repo has been changed => reload");
          filesystem::remove_all(m_cacheDir);
        }
      }

      TRC_FUNCTION_LEAVE(PAR(result));
      return result;
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
        Document doc;
        if (parseFromFile(fname, doc)) {
          int standardId = 0;
          std::string name;
          if (doc.IsArray()) {
            for (auto itr = doc.Begin(); itr != doc.End(); ++itr) {
              POINTER_GET_INT(STANDARDS_URL, itr, "/standardID", standardId, fname);
              POINTER_GET_STRING(STANDARDS_URL, itr, "/name", name, fname);
              m_standardMap.insert(std::make_pair(standardId, StdItem(name)));
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error not array: " << PAR(STANDARDS_URL) << PAR(fname));
          }
        }
        else {
          THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
        }
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
              std::string driver, notes;
              POINTER_GET_INT("", &doc, "/versionFlags", versionFlag, fname);
              POINTER_GET_STRING("", &doc, "/driver", driver, fname);
              POINTER_GET_STRING("", &doc, "/notes", notes, fname);
              stdDriver.second = StdDriver(stdItem.second.m_name, driver, notes, versionFlag);
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
      
      // daemon wrapper workaround
      {
        std::string fname = m_iLaunchService->getDataDir();
        fname += "/JavaScript/DaemonWrapper.js";
        std::ifstream file(fname);
        if (file.is_open()) {
          std::ostringstream strStream;
          strStream << file.rdbuf();
          std::string dwString = strStream.str();

          StdDriver dwStdDriver("DaemonWrapper", dwString, "", 0);

          StdItem dwStdItem("DaemonWrapper");
          dwStdItem.m_valid = true;
          dwStdItem.m_drivers.insert(std::make_pair(0, dwStdDriver));

          m_standardMap.insert(std::make_pair(1000, dwStdItem));
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot open: " << PAR(fname));
        }
      }

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
              m_packageMap.insert(std::make_pair(pck.m_packageId, pck));
            }
          }
          else {
            THROW_EXC(std::logic_error, "parse error not array: " << PAR(PACKAGES_URL) << PAR(fname));
          }
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
                int version = -1;
                for (auto itr = standardArray->Begin(); itr != standardArray->End(); ++itr) {
                  POINTER_GET_INT("", itr, "/standardID", standardId, fname);
                  POINTER_GET_DOUBLE("", itr, "/version", version, fname);
                  const StdDriver* stdDrv = getStandard(standardId, version);
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

      std::string urlLoading = getDataAbsoluteUrl(relativeUrl);

      TRC_DEBUG("Getting: " << PAR(urlLoading));
      std::cout << "Getting: " << urlLoading << std::endl;
      m_iRestApiService->getFile(urlLoading, pathLoading);
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
      std::cout << "Getting: " << urlLoading << std::endl;
      m_iRestApiService->getFile(urlLoading, urlFname);
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
      m_cacheDir = m_iLaunchService->getCacheDir() + "/jscache";
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
      v = Pointer("/urlRepo").Get(doc);
      if (v && v->IsString()) {
        m_urlRepo = v->GetString();
      }
      v = Pointer("/checkPeriodSec").Get(doc);
      if (v && v->IsInt()) {
        m_checkPeriodSec = v->GetInt();
        if (m_checkPeriodSec > 0 && m_checkPeriodSec < m_checkPeriodSecMin) {
          TRC_WARNING(PAR(m_checkPeriodSec) << " from configuration forced to: " << PAR(m_checkPeriodSecMin));
          m_checkPeriodSec = m_checkPeriodSecMin;
        }
      }
        
      m_iSchedulerService->registerTaskHandler(m_name, [=](const rapidjson::Value & task)
      {
        if (task.IsString() && std::string(task.GetString()) == CHECK_CACHE) {
          if (!checkCache()) {
            loadCache();
          }
        }
      });

      if (m_checkPeriodSec > 0) {
        Document task;
        task.SetString(CHECK_CACHE.c_str(), task.GetAllocator());
        m_iSchedulerService->scheduleTaskPeriodic(m_name, task, std::chrono::seconds(m_checkPeriodSec));
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

  const std::string& JsCache::getDriver(int id, int ver) const
  {
    return m_imp->getDriver(id, ver);
  }

  const std::map<int, const IJsCacheService::StdDriver*>  JsCache::getAllLatestDrivers() const
  {
    return m_imp->getAllLatestDrivers();
  }

  const IJsCacheService::Manufacturer* JsCache::getManufacturer(uint16_t hwpid) const
  {
    return m_imp->getManufacturer(hwpid);
  }

  const IJsCacheService::Product* JsCache::getProduct(uint16_t hwpid) const
  {
    return m_imp->getProduct(hwpid);
  }

  const IJsCacheService::Package* JsCache::getPackage(uint16_t hwpid, const std::string& os, const std::string& dpa) const
  {
    return m_imp->getPackage(hwpid, os, dpa);
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

  void JsCache::attachInterface(shape::ILaunchService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsCache::detachInterface(shape::ILaunchService* iface)
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

  void JsCache::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsCache::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }
}
