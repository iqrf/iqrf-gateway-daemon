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

  enum class DowloadState {
    ERROR,
    INIT,
    STANDARD,
    STANDARD_IDS,
    DRIVERS,
    READY,
  };

  //class StdDriver
  //{
  //public:
  //  StdDriver()
  //    :m_valid(false)
  //  {}
  //  StdDriver(const std::string& name, const std::string& driver, const std::string& notes, int verFlags)
  //    :m_name(name), m_driver(driver), m_notes(notes), m_versionFlags(verFlags), m_valid(true)
  //  {}
  //  bool isValid() const { return m_valid; }
  //  const std::string& getName() const { return m_name; }
  //  const std::string& getDriver() const { return m_driver; }
  //  const std::string& getNotes() const { return m_notes; }
  //  int getVersionFlags() { return m_versionFlags; }
  //private:
  //  bool m_valid = false;
  //  int m_versionFlags = 0;
  //  std::string m_name;
  //  std::string m_driver;
  //  std::string m_notes;
  //};

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
    iqrf::ISchedulerService* m_iSchedulerService = nullptr;
    shape::IRestApiService* m_iRestApiService = nullptr;
    
    std::mutex m_updateMtx;
    DowloadState m_downloadState = DowloadState::INIT;
    std::string m_pathLoading;
    std::string m_urlLoading;
    std::string cacheDir = "./configuration/jscache";
    std::string m_urlRepo = "https://repository.iqrfalliance.org/api";
    int m_stdItemLoading = -1;

    std::multimap<int, StdItem> m_standardMap;

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

    const std::map<int, const IJsCacheService::StdDriver*> getAllLatestDrivers() const
    {
      TRC_FUNCTION_ENTER("");
      std::map<int, const IJsCacheService::StdDriver*> retval;
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

    const std::string& getManufacturer(uint16_t hwpid) const
    {
      static std::string ni = "not implemented yet";
      return ni;
    }

    const std::string& getProduct(uint16_t hwpid) const
    {
      static std::string ni = "not implemented yet";
      return ni;
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
        //Writer<OStreamWrapper> writer(osw);
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

    std::string getDataFileName(const std::string& url)
    {
      std::ostringstream os;
      os << cacheDir << '/' << url << "/data.json";
      return os.str();
    }

    std::string getDataUrl(const std::string& url)
    {
      std::ostringstream os;
      os << m_urlRepo << '/' << url;
      return os.str();
    }

    void downloadData(const std::string& url)
    {
      TRC_FUNCTION_ENTER(PAR(url));
      m_pathLoading = getDataFileName(url);
      m_urlLoading = getDataUrl(url);
      TRC_DEBUG("Getting: " << m_urlLoading);
      std::cout << "Getting: " << m_urlLoading << std::endl;
      m_iRestApiService->getData(m_urlLoading);
      TRC_FUNCTION_LEAVE("")
    }

    void loadCache()
    {
      TRC_FUNCTION_ENTER("");

      std::lock_guard<std::mutex> lck(m_updateMtx);
      m_downloadState = DowloadState::INIT;
      updateCache();

      TRC_FUNCTION_LEAVE("")
    }

    void updateCache()
    {
      using namespace rapidjson;
      using namespace boost;

      TRC_FUNCTION_ENTER("");

      // daemon wrapper workaround
      {
        std::ifstream file("./configuration/JavaScript/DaemonWrapper.js");
        if (file.is_open()) {
          std::ostringstream strStream;
          strStream << file.rdbuf();
          std::string dwString = strStream.str();

          IJsCacheService::StdDriver dwStdDriver("DaemonWrapper", dwString, "", 0);

          StdItem dwStdItem("DaemonWrapper");
          dwStdItem.m_valid = true;
          dwStdItem.m_drivers.insert(std::make_pair(0, dwStdDriver));

          m_standardMap.insert(std::make_pair(1000, dwStdItem));
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot open: " << "./configuration/JavaScript/DaemonWrapper.js");
        }
      }

      try {
        switch (m_downloadState) {
        case DowloadState::INIT:
        {
          if (!filesystem::exists(getDataFileName("standards"))) {
            // wait for rest
            m_downloadState = DowloadState::STANDARD;
            downloadData("standards");
            break;
          }
        }
        case DowloadState::STANDARD:
        {
          std::string fname = getDataFileName("standards");
          if (filesystem::exists(fname)) {
            Document doc;
            if (parseFromFile(fname, doc)) {
              int standardId = 0;
              std::string name;
              if (doc.IsArray()) {
                for (auto itr = doc.Begin(); itr != doc.End(); ++itr) {
                  if (Value* standardIdVal = Pointer("/standardID").Get(*itr)) {
                    standardId = standardIdVal->GetInt();
                  }
                  else {
                    THROW_EXC(std::logic_error, "parse error /standards[]/standardID " << PAR(fname));
                  }
                  if (Value* nameVal = Pointer("/name").Get(*itr)) {
                    name = nameVal->GetString();
                  }
                  else {
                    THROW_EXC(std::logic_error, "parse error /standards[]/name " << PAR(fname));
                  }
                  m_standardMap.insert(std::make_pair(standardId, StdItem(name)));
                }
              }
              else {
                THROW_EXC(std::logic_error, "parse error /standards[] not array " << PAR(fname));
              }
            }
            else {
              THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
            }
          }
          else {
            THROW_EXC(std::logic_error, "file not exist " << PAR(fname));
          }
          m_downloadState = DowloadState::STANDARD_IDS; // don't break next => state
        }
        case DowloadState::STANDARD_IDS:
        {
          const std::string url = "standards/";

          bool switchBreak = false;
          for (auto & stdItem : m_standardMap) { // 1

            if (stdItem.second.m_valid) continue; //already updated

            std::ostringstream os;
            os << "standards/" << stdItem.first;
            std::string url = os.str();
            std::string fname = getDataFileName(url);

            if (!filesystem::exists(fname)) {
              // wait for rest
              downloadData(url);
              switchBreak = true;
              break; // 1
            }
            else {
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
          } // for 1
          if (switchBreak) {
            // not finished yet 
            break; // switch 
          }
          else {
            //all standard/id loaded
            m_downloadState = DowloadState::DRIVERS;
          }
        }
        case DowloadState::DRIVERS:
        {
          bool switchBreak = false;
          for (auto & stdItem : m_standardMap) { // 1
            for (auto & stdDriver : stdItem.second.m_drivers) { // 2

              if (stdDriver.second.isValid()) continue; //already updated

              std::ostringstream os;
              os << "standards/" << stdItem.first << '/' << stdDriver.first;
              std::string url = os.str();
              std::string fname = getDataFileName(url);

              if (!filesystem::exists(fname)) {
                // wait for rest
                downloadData(url);
                switchBreak = true;
                break; // for 2
              }
              else {
                Document doc;
                if (parseFromFile(fname, doc)) {
                  int versionFlag;
                  std::string driver, notes;
                  if (Value* versionFlagsVal = Pointer("/versionFlags").Get(doc)) {
                    versionFlag = versionFlagsVal->GetInt();
                  }
                  else {
                    THROW_EXC(std::logic_error, "parse error /versionFlags not exist " << PAR(fname));
                  }
                  if (Value* driverVal = Pointer("/driver").Get(doc)) {
                    driver = driverVal->GetString();
                  }
                  else {
                    THROW_EXC(std::logic_error, "parse error /driver not exist " << PAR(fname));
                  }
                  if (Value* notesVal = Pointer("/notes").Get(doc)) {
                    notes = notesVal->GetString();
                  }
                  else {
                    THROW_EXC(std::logic_error, "parse error /notes not exist " << PAR(fname));
                  }
                  stdDriver.second = StdDriver(stdItem.second.m_name, driver, notes, versionFlag);
                }
                else {
                  THROW_EXC(std::logic_error, "parse error file " << PAR(fname));
                }
              }
            } // for 2
            if (switchBreak) {
              // stop iterate wait for rest
              break; // for 1
            }
          } // for 1
          if (switchBreak) {
            // not finished yet 
            break; // switch
          }
          else {
            //all standard/id/ver loaded
            m_downloadState = DowloadState::READY;
          }
        }
        case DowloadState::READY:
          //finit
        case DowloadState::ERROR:
          break;
        default:
          ;
        };
      }
      catch (std::logic_error &e) {
        CATCH_EXC_TRC_WAR(std::logic_error, e, "Cannot load IQRF Repo");
        m_downloadState = DowloadState::ERROR;
      }

      TRC_FUNCTION_LEAVE("")
    }

    void dataHandler(int statusCode, const std::string& data)
    {
      TRC_FUNCTION_ENTER("");

      std::lock_guard<std::mutex> lck(m_updateMtx);

      if (statusCode == 200) {
        rapidjson::Document doc;
        if (parseStr(data, doc)) {
          encodeToFile(m_pathLoading, doc);
          updateCache();
        }
      }
      else {
        TRC_WARNING("Cannot get data: " << PAR(statusCode) << NAME_PAR(url, m_urlLoading));
        m_downloadState = DowloadState::ERROR;
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


      //TODO name from cfg
      //m_iSchedulerService->registerMessageHandler("JsCache", [=](const std::string& task)
      //{
      //  if (task == "downloadRepo") {
      //    downLoadRepo();
      //  }
      //});

      //m_iSchedulerService->scheduleTaskPeriodic("JsCache", "downloadRepo", std::chrono::seconds(10));

      m_iRestApiService->registerDataHandler([=](int statusCode, const std::string& data)
      {
        dataHandler(statusCode, data);
      });

      loadCache();

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");

      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsCache instance deactivate" << std::endl <<
        "******************************"
      );

      m_iRestApiService->unregisterDataHandler();

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
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

  const std::string& JsCache::getManufacturer(uint16_t hwpid) const
  {
    return m_imp->getManufacturer(hwpid);
  }

  const std::string& JsCache::getProduct(uint16_t hwpid) const
  {
    return m_imp->getProduct(hwpid);
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
