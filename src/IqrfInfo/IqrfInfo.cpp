#include "IqrfInfo.h"

#include <sqlite_modern_cpp.h>

#include "RawDpaEmbedExplore.h"
#include "RawDpaEmbedCoordinator.h"
#include "RawDpaEmbedOS.h"
#include "RawDpaEmbedEEEPROM.h"
#include "RawDpaEmbedFRC.h"
#include "JsDriverBinaryOutput.h"
#include "JsDriverSensor.h"
#include "JsDriverLight.h"
#include "InfoSensor.h"
#include "InfoBinaryOutput.h"
#include "InfoDali.h"
#include "InfoLight.h"
#include "InfoNode.h"
#include "HexStringCoversion.h"

#include "Trace.h"
#include "rapidjson/pointer.h"
#include <fstream>
#include <set>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <random>
#include <cstddef>

#include "iqrf__IqrfInfo.hxx"

TRC_INIT_MODULE(iqrf::IqrfInfo);

using namespace  sqlite;

namespace iqrf {

  const int PERIF_STANDARD_SENSOR = 94;
  const int PERIF_STANDARD_BINOUT = 75;
  const int PERIF_STANDARD_DALI = 74;
  const int PERIF_STANDARD_LIGHT = 113;

  class SqlFile
  {
  public:
    static void makeSqlFile(sqlite::database &db, const std::string & fname)
    {
      std::vector<std::string> sqls;

      std::ifstream f(fname);
      if (f.is_open()) {
        std::ostringstream strStream;
        strStream << f.rdbuf();

        std::string token;
        std::istringstream tokenStream(strStream.str());
        while (std::getline(tokenStream, token, ';'))
        {
          sqls.push_back(token);
        }

        if (sqls.size() == 0) {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot get SQL command from: " << PAR(fname))
        }

        for (const auto & sql : sqls) {
          db << sql;
        }

      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot read: " << PAR(fname));
      }
    }
  };

  class IqrfInfo::Imp
  {
    // Driver DB structure
    class Driver
    {
    public:
      Driver() = delete;
      Driver(std::string name, int stdId, int ver, std::string drv)
        : m_name(name)
        , m_stdId(stdId)
        , m_ver(ver)
        , m_drv(drv)
      {}
      std::string m_name;
      int m_stdId;
      int m_ver;
      std::string m_drv;
    };

    // Device DB structure
    class Device
    {
    public:
      Device() = delete;
      Device(
        int hwpid,
        int hwpidVer,
        int osBuild,
        int dpaVer
      )
        : m_hwpid(hwpid)
        , m_hwpidVer(hwpidVer)
        , m_osBuild(osBuild)
        , m_dpaVer(dpaVer)
        , m_repoPackageId(0)
        , m_inRepo(false)
      {}

      int m_hwpid;
      int m_hwpidVer;
      int m_osBuild;
      int m_dpaVer;
      int m_repoPackageId;
      std::string m_notes;
      std::string m_handlerhash;
      std::string m_handlerUrl;
      std::string m_customDriver;
      bool m_inRepo;
      std::vector<const IJsCacheService::StdDriver *> m_drivers;
    };

    // aux class for enumeration
    class NodeData
    {
    private:
      embed::node::BriefInfo m_node;
      embed::explore::EnumeratePtr m_exploreEnumerate;
      embed::os::ReadPtr m_osRead;

    public:
      NodeData(const embed::node::BriefInfo & node)
        : m_node(node)
      {}

      embed::node::BriefInfo & getNode()
      {
        return m_node;
      }

      void setNode(const embed::node::BriefInfo & node)
      {
        m_node = node;
      }

      const embed::explore::EnumeratePtr & getEmbedExploreEnumerate() const
      {
        return m_exploreEnumerate;
      }

      void setEmbedExploreEnumerate(embed::explore::RawDpaEnumeratePtr & ee)
      {
        m_exploreEnumerate = std::move(ee);
        m_node.setDpaVer(m_exploreEnumerate->getDpaVer());
        m_node.setHwpid(m_exploreEnumerate->getHwpidEnm());
        m_node.setHwpidVer(m_exploreEnumerate->getHwpidVer());
      }

      const embed::os::ReadPtr & getEmbedOsRead() const
      {
        return m_osRead;
      }

      void setEmbedOsRead(embed::os::RawDpaReadPtr & osr)
      {
        m_osRead = std::move(osr);
        m_node.setMid(m_osRead->getMid());
        m_node.setOsBuild(m_osRead->getOsBuild());
        if (m_osRead->is410Compliant()) {
          m_node.setDpaVer(m_osRead->getDpaVer());
          m_node.setHwpid(m_osRead->getHwpidValFromOs());
          m_node.setHwpidVer(m_osRead->getHwpidVer());
        }
      }

    };
    typedef std::unique_ptr<NodeData> NodeDataPtr;

  private:

    IJsRenderService* m_iJsRenderService = nullptr;
    IJsCacheService* m_iJsCacheService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    shape::ILaunchService* m_iLaunchService = nullptr;

    std::unique_ptr<database> m_db;

    // nadrs of nodes data to be fully enumerated
    std::map<int, NodeDataPtr> m_nadrFullEnumNodeMap;

    // set by AutoNw
    std::map<int, embed::node::BriefInfo> m_nadrInsertNodeMap;

    std::string m_instanceName;

    bool m_enumAtStartUp = false;
    bool m_enumUniformDpaVer = false;

    bool m_enumPeriod = 0;
    std::thread m_enumThread;
    std::atomic_bool m_enumThreadRun;
    std::mutex m_enumMtx;
    std::condition_variable m_enumCv;

    // rerun enum
    std::atomic<bool> m_repeatEnum;

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    void initDb()
    {
      TRC_FUNCTION_ENTER("");
      try
      {
        std::string dataDir = m_iLaunchService->getDataDir();
        std::string fname = dataDir + "/DB/IqrfInfo.db";

        std::ifstream f(fname);
        bool dbExists = f.is_open();
        f.close();

        m_db.reset(shape_new database(fname));
        database &db = *m_db;
        db << "PRAGMA foreign_keys=ON";

        std::string sqlpath = dataDir;
        sqlpath += "/DB/";

        if (!dbExists) {
          //create tables
          SqlFile::makeSqlFile(db, sqlpath + "init/IqrfInfo.db.sql");
        }

        /*
        //update DB
        try {
          SqlFile::makeSqlFile(db, sqlpath + "init/IqrfInfo_update1.db.sql");
        }
        catch (sqlite_exception &e)
        {
          db << "rollback;";
          TRC_DEBUG("Update DB: " << NAME_PAR(code, e.get_code()) << NAME_PAR(ecode, e.get_extended_code()) << NAME_PAR(SQL, e.get_sql()));
        }

        //update DB
        int maxVal = -1;
        try {
          // test column existence
          int maxVal = -1;
          db << "select max(ModeStd) from node;"
            >> maxVal;
        }
        catch (sqlite_exception &e)
        {
          TRC_DEBUG("Node.ModeStd doesn't exists");
        }
        if (maxVal > -1) {
          // column exists => delete
          SqlFile::makeSqlFile(db, sqlpath + "init/IqrfInfo_update2.db.sql");
        }
        */

      }
      catch (sqlite_exception &e)
      {
        CATCH_EXC_TRC_WAR(sqlite_exception, e, "Unexpected error " << NAME_PAR(code, e.get_code()) << NAME_PAR(ecode, e.get_extended_code()) << NAME_PAR(SQL, e.get_sql()));
      }
      catch (std::logic_error &e)
      {
        CATCH_EXC_TRC_WAR(std::logic_error, e, "Unexpected error ");
      }

      TRC_FUNCTION_LEAVE("");
    }

    void runEnum()
    {
      TRC_FUNCTION_ENTER("");

      while (m_enumThreadRun) {

        try {
          if (!m_iIqrfDpaService->hasExclusiveAccess()) {
            checkEnum();

            if (!m_enumThreadRun) break;
            fullEnum();

            if (!m_enumThreadRun) break;
            loadDeviceDrivers();

            if (!m_enumThreadRun) break;
            stdEnum();
            m_repeatEnum = false;
          }
          else {
            TRC_DEBUG("DPA has exclusive access");
          }
        }
        catch (std::exception & e) {
          CATCH_EXC_TRC_WAR(std::exception, e, "Enumeration failure");
        }

        // wait for next iteration
        std::unique_lock<std::mutex> lck(m_enumMtx);
        if (!m_repeatEnum) {
          if (m_enumPeriod > 0) {
            TRC_DEBUG("Going to sleep up to next period: " << PAR(m_enumPeriod));
            m_enumCv.wait_for(lck, std::chrono::minutes(m_enumPeriod));
            TRC_DEBUG("Wake up from periodic sleeping => going to do periodic enumeration: " << PAR(m_enumPeriod))
          }
          else {
            TRC_DEBUG("Going to sleep up to next enumeration request or network change DPA command");
            m_enumCv.wait(lck);
            TRC_DEBUG("Wake up by enumeration request or network change DPA command => going to do enumeration: " << PAR(m_enumPeriod))
          }
        }
        else {
          TRC_DEBUG("Enumeration has to be repeated: " << PAR(m_repeatEnum) << " => wait for 3 sec to continue");
          m_enumCv.wait_for(lck, std::chrono::seconds(3));
          TRC_DEBUG("Weke up and repeat enumeration");
        }

      }

      TRC_FUNCTION_LEAVE("");
    }

    void checkEnum()
    {
      TRC_FUNCTION_ENTER("");

      // prepare nadr bond node map
      std::map<int, embed::node::BriefInfo> nadrBondNodeMap;

      // get bonded nadrs from C
      iqrf::embed::coordinator::RawDpaBondedDevices iqrfEmbedCoordinatorBondedDevices;
      {
        std::unique_ptr<IDpaTransactionResult2> transResult;
        auto trn = m_iIqrfDpaService->executeDpaTransaction(iqrfEmbedCoordinatorBondedDevices.getRequest());
        iqrfEmbedCoordinatorBondedDevices.processDpaTransactionResult(trn->get());
      }
      std::set<int> bonded = iqrfEmbedCoordinatorBondedDevices.getBondedDevices();

      // get discovered nadrs from C
      iqrf::embed::coordinator::RawDpaDiscoveredDevices dd;
      {
        auto trn = m_iIqrfDpaService->executeDpaTransaction(dd.getRequest());
        dd.processDpaTransactionResult(trn->get());
      }
      std::set<int> discovered = dd.getDiscoveredDevices();

      if (bonded.size() > 0) {
        // eeeprom read parms for bonded
        const uint8_t maxlen = 54;
        const uint16_t baddress = 0x4000;
        const uint8_t bnlen = 8;

        int maxnadr = *bonded.rbegin();
        int maxdata = (maxnadr + 1) * bnlen;
        int maxhit = maxdata / maxlen;
        int remain = maxdata % maxlen;
        std::vector<uint8_t> pdataVec;
        pdataVec.reserve(maxdata);

        // read C eeeprom up to nadr storage
        for (int i = 0; i < maxhit + 1; i++) {
          uint8_t len = i <= maxhit ? maxlen : remain;
          if (len > 0) {
            uint16_t adr = (uint16_t)(baddress + i * maxlen);
            iqrf::embed::eeeprom::rawdpa::Read eeepromRead(0, adr, len);
            auto trn = m_iIqrfDpaService->executeDpaTransaction(eeepromRead.getRequest());
            auto trnResult = trn->get();
            eeepromRead.processDpaTransactionResult(std::move(trnResult));
            pdataVec.insert(pdataVec.end(), eeepromRead.getPdata().begin(), eeepromRead.getPdata().end());
          }
        }

        // parse stored mids TODO vrn data?
        for (int nadr : bonded) {
          int m = nadr * 8;
          uint32_t mid = (unsigned)pdataVec[m] | ((unsigned)pdataVec[m + 1] << 8) | ((unsigned)pdataVec[m + 2] << 16) | ((unsigned)pdataVec[m + 3] << 24);

          auto found = discovered.find(nadr);
          bool dis = found != discovered.end();
          nadrBondNodeMap[nadr] = embed::node::BriefInfo(mid, dis, false);
        }
      }

      // get coordinator mid already taken by IqrfDpa
      nadrBondNodeMap[0] = embed::node::BriefInfo(m_iIqrfDpaService->getCoordinatorParameters().mid, false, false);

      database & db = *m_db;

      // to be filled from DB
      std::map<int, embed::node::BriefInfo> nadrBondNodeDbMap;

      db << "select "
        "b.Nadr "
        ", b.Dis "
        ", b.Mid "
        ", b.Enm "
        "from "
        "Bonded as b "
        ";"
        >> [&](
          int nadr,
          int dis,
          uint32_t mid,
          int enm
          )
      {
        nadrBondNodeDbMap.insert(std::make_pair(nadr, embed::node::BriefInfo(mid, dis != 0, enm != 0)));
      };

      // delete Nadr from DB if it doesn't exist in Net
      for (const auto & bo : nadrBondNodeDbMap) {
        int nadr = bo.first;
        auto found = nadrBondNodeMap.find(nadr);
        if (found == nadrBondNodeMap.end()) {
          // Nadr not found in Net => delete from Bonded
          TRC_INFORMATION(PAR(nadr) << " remove from bonded list")
            db << "delete from Bonded where Nadr = ?;" << nadr;
        }
      }

      // compare fast enum and DB
      for (const auto & p : nadrBondNodeMap) {
        int nadr = p.first;
        uint32_t mid = p.second.getMid();
        auto found = nadrBondNodeDbMap.find(nadr);

        if (found == nadrBondNodeDbMap.end() || mid != found->second.getMid() || !found->second.getEnm()) {
          // Nadr from Net not found in DB or comparison failed => provide full enum
          m_nadrFullEnumNodeMap.insert(std::make_pair(nadr, NodeDataPtr(shape_new NodeData(p.second))));
          TRC_INFORMATION(PAR(nadr) << "check enum does not fit => assigned to full enum")
        }
        else if (found != nadrBondNodeDbMap.end()) {
          // compare and update discovery status
          bool dis = p.second.getDisc();
          if (dis != found->second.getDisc()) {
            db << "update Bonded  set Dis = ? where Nadr = ?; " << (dis ? 1 : 0) << nadr;
          }
        }
      }

      TRC_FUNCTION_LEAVE("");
    }

    // return deviceId
    std::unique_ptr<int> enumerateDeviceInRepo(Device & d, const iqrf::IJsCacheService::Package & pckg)
    {
      TRC_FUNCTION_ENTER(PAR(d.m_hwpid) << PAR(d.m_hwpidVer) << PAR(d.m_osBuild) << PAR(d.m_dpaVer));

      d.m_repoPackageId = pckg.m_packageId;
      d.m_notes = pckg.m_notes;
      d.m_handlerhash = pckg.m_handlerHash;
      d.m_handlerUrl = pckg.m_handlerUrl;
      d.m_customDriver = pckg.m_driver;
      d.m_inRepo = true;
      d.m_drivers = pckg.m_stdDriverVect;

      // find if such a device already stored in DB
      std::unique_ptr<int> deviceIdPtr = selectDevice(d);

      TRC_FUNCTION_LEAVE(NAME_PAR(deviceId, (deviceIdPtr ? *deviceIdPtr : 0)));
      return deviceIdPtr;
    }

    std::unique_ptr<int> enumerateDeviceOutsideRepo(int nadr, const NodeDataPtr & nd, Device & d)
    {
      TRC_FUNCTION_ENTER(PAR(d.m_hwpid) << PAR(d.m_hwpidVer) << PAR(d.m_osBuild) << PAR(d.m_dpaVer));

      const auto & exEnum = nd->getEmbedExploreEnumerate();
      if (!exEnum) {
        // wasn't enumerated yet => done by FRC
        std::unique_ptr<embed::explore::RawDpaEnumerate> exploreEnumeratePtr(shape_new embed::explore::RawDpaEnumerate(nadr));
        {
          auto trn = m_iIqrfDpaService->executeDpaTransaction(exploreEnumeratePtr->getRequest());
          exploreEnumeratePtr->processDpaTransactionResult(trn->get());
          nd->setEmbedExploreEnumerate(exploreEnumeratePtr);
        }
      }

      d.m_repoPackageId = 0;
      d.m_inRepo = false;

      // find if such a device already stored in DB
      std::unique_ptr<int> deviceIdPtr = selectDevice(d);

      if (!deviceIdPtr) {
        // no device in DB and no package in IqrfRepo => get drivers by enumeration at first
        std::map<int, int> perVerMap;

        // Get for hwpid 0 plain DPA plugin
        const iqrf::IJsCacheService::Package *pckg0 = m_iJsCacheService->getPackage((uint16_t)0, (uint16_t)0, (uint16_t)d.m_osBuild, (uint16_t)d.m_dpaVer);
        if (nullptr == pckg0) {
          TRC_WARNING("Cannot find package for:" << NAME_PAR(hwpid, 0) << NAME_PAR(hwpidVer, 0) << NAME_PAR(osBuild, d.m_osBuild) << NAME_PAR(dpaVer, d.m_dpaVer)
            << std::endl << "trying to find the package for previous version of DPA");

          for (uint16_t dpa = (uint16_t)d.m_dpaVer - 1; dpa > 300; dpa--) {
            pckg0 = m_iJsCacheService->getPackage((uint16_t)0, (uint16_t)0, (uint16_t)d.m_osBuild, dpa);
            if (nullptr != pckg0) {
              TRC_WARNING("Found and loading package for:" << NAME_PAR(hwpid, 0) << NAME_PAR(hwpidVer, 0) << NAME_PAR(osBuild, d.m_osBuild) << NAME_PAR(dpaVer, dpa));
              break;
            }
          }
        }

        if (nullptr != pckg0) {
          for (auto per : nd->getEmbedExploreEnumerate()->getEmbedPer()) {
            for (auto drv : pckg0->m_stdDriverVect) {
              if (drv->getId() == -1) {
                perVerMap.insert(std::make_pair(-1, drv->getVersion())); // driver library
              }
              if (drv->getId() == 255) {
                perVerMap.insert(std::make_pair(255, drv->getVersion())); // embedExplore library
              }
              if (drv->getId() == per) {
                perVerMap.insert(std::make_pair(per, drv->getVersion()));
              }
            }
          }
        }
        else {
          TRC_WARNING("Cannot find package for:" << NAME_PAR(hwpid, 0) << NAME_PAR(hwpidVer, 0) << NAME_PAR(osBuild, d.m_osBuild) << " any DPA")
        }

        for (auto per : nd->getEmbedExploreEnumerate()->getUserPer()) {

          //Get peripheral information for sensor, binout, dali, light and TODO other std if presented
          if (PERIF_STANDARD_BINOUT == per || PERIF_STANDARD_SENSOR == per || PERIF_STANDARD_DALI == per || PERIF_STANDARD_LIGHT == per) {

            embed::explore::RawDpaPeripheralInformation perInfo(nadr, per);
            perInfo.processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(perInfo.getRequest())->get());

            int version = perInfo.getPar1();
            perVerMap.insert(std::make_pair(per, version));
          }
          else {
            perVerMap.insert(std::make_pair(per, -1));
          }
        }

        for (auto pv : perVerMap) {
          const IJsCacheService::StdDriver *sd = m_iJsCacheService->getDriver(pv.first, pv.second);
          if (sd) {
            d.m_drivers.push_back(sd);
          }
        }
      }

      TRC_FUNCTION_LEAVE(NAME_PAR(deviceId, (deviceIdPtr ? *deviceIdPtr : 0)));
      return deviceIdPtr;
    }

    void fullEnumByFrc()
    {
      TRC_FUNCTION_ENTER("");

      // frc results
      std::map<int, uint32_t> hwpidMap;
      std::map<int, uint32_t> osBuildMap;
      std::map<int, uint32_t> dpaVerMap;
      bool anyValid = false;

      const uint16_t CBUFFER = 0x04a0; // msg buffer in node

      //TODO for test
      //m_nadrFullEnumNodeMap.clear();
      //for (int i = 3; i < 192; i++) {
      //  m_nadrFullEnumNodeMap.insert(std::make_pair(i, NodeDataPtr(shape_new NodeData(0))));
      //}

      std::set<int> nadrEnumSet;
      for (auto & it : m_nadrFullEnumNodeMap) {
        nadrEnumSet.insert(it.first);
      }
      std::vector<std::set<int>> setVect = iqrf::embed::frc::Send::splitSelectedNode<uint32_t>(nadrEnumSet);
      TRC_DEBUG("FRC handling split to: " << PAR(setVect.size()));

      iqrf::embed::frc::rawdpa::ExtraResult extra;

      while (true) {
        { // read HWPID, HWPIDVer
          TRC_DEBUG("Getting Hwpid, HwpidVer")
            // reuse data from AutoNetwork if any
            std::set<int> leftHwpidEnum(nadrEnumSet);

          if (m_nadrInsertNodeMap.size() > 0) {
            TRC_DEBUG("Detected nadr map of nodes from AN: " << PAR(m_nadrInsertNodeMap.size()))

              for (auto & it : m_nadrFullEnumNodeMap) {
                int nadr = it.first;
                auto found = m_nadrInsertNodeMap.find(nadr);
                if (m_nadrInsertNodeMap.end() != found) {
                  int hwpid = found->second.getHwpid();
                  int hwpidVer = found->second.getHwpidVer();
                  if (hwpid >= 0 && hwpidVer >= 0) {
                    m_nadrFullEnumNodeMap[nadr]->getNode().setHwpid(hwpid);
                    m_nadrFullEnumNodeMap[nadr]->getNode().setHwpidVer(hwpidVer);
                    leftHwpidEnum.erase(nadr);
                  }
                }
              }

            TRC_DEBUG("Something left to be Hwpid enumerated after AN: " << PAR(leftHwpidEnum.size()))
          }

          if (leftHwpidEnum.size() > 0) { // nothing to reuse go for FRC

            TRC_DEBUG("Start Hwpid enumeration");

            uint16_t address = CBUFFER + (uint16_t)offsetof(TEnumPeripheralsAnswer, HWPID);
            iqrf::embed::frc::rawdpa::MemoryRead4B frc(address, PNUM_ENUMERATION, CMD_GET_PER_INFO, true); //value += 1

            for (const auto & s : setVect) {
              TRC_DEBUG("Preparing 4B FRC to get HWPID for selected nodes: " << NAME_PAR(first, *s.begin()) << NAME_PAR(last, *s.rbegin()));

              frc.setSelectedNodes(s);
              frc.processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(frc.getRequest())->get());
              // Check FRC status
              uint8_t status = frc.getStatus();
              if (status > 0xEF) {
                TRC_WARNING("FRC to get HWPID failed: " << PAR(status));
                break; //no sense to continue now
              }
              if (!m_enumThreadRun) break;

              //TODO check status
              // get extra result
              extra.processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(extra.getRequest())->get());

              frc.getFrcDataAs(hwpidMap, extra.getFrcData());

              for (const auto & it : hwpidMap) {
                int nadr = it.first;
                uint32_t hwpidw = it.second;
                if (0 != hwpidw) {
                  anyValid = true;
                  // correct value from FRC => store it
                  --hwpidw;
                  auto found = m_nadrFullEnumNodeMap.find(nadr);
                  if (found != m_nadrFullEnumNodeMap.end()) {
                    found->second->getNode().setHwpid(hwpidw & 0xffff);
                    found->second->getNode().setHwpidVer(hwpidw >> 16);
                  }
                  else {
                    THROW_EXC_TRC_WAR(std::logic_error, "Inconsistence in get HWPID FRC processing");
                  }
                }
              }
              if (!m_enumThreadRun) break;
            }
          }
          if (!anyValid) {
            TRC_WARNING("FRC doesn't return any valid HWPID => no sense to continue and nothing is enumerated");
            break; //no sense to continue now
          }
          if (!m_enumThreadRun) break;
        }

        if (m_enumUniformDpaVer) {
          TRC_WARNING("Detected enumUniformDpaVer cfg par => DpaVer and OsBuild fill according coordinator for all nodes");

          // dpaVer and osBuild set according C
          auto cp = m_iIqrfDpaService->getCoordinatorParameters();
          for (auto & it : m_nadrFullEnumNodeMap) {
            it.second->getNode().setOsBuild(cp.osBuildWord);
            it.second->getNode().setDpaVer(cp.dpaVerWord);
          }
          break; // not necessary to continue
        }

        anyValid = false;
        { // read DpaVersion + 2B

          TRC_DEBUG("Start DpaVer enumeration");

          uint16_t address = CBUFFER + (uint16_t)offsetof(TEnumPeripheralsAnswer, DpaVersion);
          iqrf::embed::frc::rawdpa::MemoryRead4B frc(address, PNUM_ENUMERATION, CMD_GET_PER_INFO, false);

          for (const auto & s : setVect) {
            TRC_DEBUG("Preparing 4B FRC to get DpaVer for selected nodes: " << NAME_PAR(first, *s.begin()) << NAME_PAR(last, *s.rbegin()));

            frc.setSelectedNodes(s);
            frc.processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(frc.getRequest())->get());
            // Check FRC status
            uint8_t status = frc.getStatus();
            if (status > 0xEF) {
              TRC_WARNING("FRC to get DpaVer failed: " << PAR(status));
              break; //no sense to continue now
            }
            if (!m_enumThreadRun) break;

            // get extra result
            extra.processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(extra.getRequest())->get());

            frc.getFrcDataAs(dpaVerMap, extra.getFrcData());

            for (const auto & it : dpaVerMap) {
              int nadr = it.first;
              uint32_t dpaVer = it.second;
              if (0 != dpaVer) {
                anyValid = true;
                // correct value from FRC => store it
                auto found = m_nadrFullEnumNodeMap.find(nadr);
                if (found != m_nadrFullEnumNodeMap.end()) {
                  found->second->getNode().setDpaVer(dpaVer & 0x3fff);
                }
                else {
                  THROW_EXC_TRC_WAR(std::logic_error, "Inconsistence in get DpaVer FRC processing");
                }
              }
            }
            if (!m_enumThreadRun) break;
          }
        }
        if (!anyValid) {
          TRC_WARNING("FRC doesn't return any valid DpaVer => no sense to continue and nothing is enumerated");
          break; //no sense to continue now
        }
        if (!m_enumThreadRun) break;

        anyValid = false;
        { // read OsBuild + 2B 

          TRC_DEBUG("Start OsBuild enumeration");

          uint16_t address = CBUFFER + (uint16_t)offsetof(TPerOSRead_Response, OsBuild);
          iqrf::embed::frc::rawdpa::MemoryRead4B frc(address, PNUM_OS, CMD_OS_READ, false);

          for (const auto & s : setVect) {
            TRC_DEBUG("Preparing 4B FRC to get OsBuild for selected nodes: " << NAME_PAR(first, *s.begin()) << NAME_PAR(last, *s.rbegin()));

            frc.setSelectedNodes(s);
            frc.processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(frc.getRequest())->get());
            uint8_t status = frc.getStatus();
            if (status > 0xEF) {
              TRC_WARNING("FRC to get OsBuild failed: " << PAR(status));
              break; //no sense to continue now
            }
            if (!m_enumThreadRun) break;

            // get extra result
            extra.processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(extra.getRequest())->get());

            frc.getFrcDataAs(osBuildMap, extra.getFrcData());

            for (const auto & it : osBuildMap) {
              int nadr = it.first;
              uint32_t osBuild = it.second;
              if (0 != osBuild) {
                anyValid = true;
                // correct value from FRC => store it
                auto found = m_nadrFullEnumNodeMap.find(nadr);
                if (found != m_nadrFullEnumNodeMap.end()) {
                  found->second->getNode().setOsBuild(osBuild & 0xffff);
                }
                else {
                  THROW_EXC_TRC_WAR(std::logic_error, "Inconsistence in get OsBuild FRC processing");
                }
              }
            }
            if (!m_enumThreadRun) break;
          }
        }
        if (!anyValid) {
          TRC_WARNING("FRC doesn't return any valid OsBuild => nothing is enumerated");
        }
        break;
      }

      TRC_FUNCTION_LEAVE("");
    }

    void fullEnumByPoll()
    {
      TRC_FUNCTION_ENTER("");

      if (m_enumUniformDpaVer) {
        // dpaVer and osBuild set according C
        auto cp = m_iIqrfDpaService->getCoordinatorParameters();
        for (auto & it : m_nadrFullEnumNodeMap) {

          if (!m_enumThreadRun) break;

          int nadr = it.first;
          NodeDataPtr & nodeData = it.second;

          std::unique_ptr<embed::explore::RawDpaEnumerate> exploreEnumeratePtr(shape_new embed::explore::RawDpaEnumerate(nadr));
          exploreEnumeratePtr->processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(exploreEnumeratePtr->getRequest())->get());
          nodeData->setEmbedExploreEnumerate(exploreEnumeratePtr);

          nodeData->getNode().setOsBuild(cp.osBuildWord);
          nodeData->getNode().setDpaVer(cp.dpaVerWord);
        }
      }
      else {
        for (auto & it : m_nadrFullEnumNodeMap) {

          if (!m_enumThreadRun) break;

          int nadr = it.first;
          NodeDataPtr & nodeData = it.second;

          try {
            std::unique_ptr <embed::os::RawDpaRead> osReadPtr(shape_new embed::os::RawDpaRead(nadr));
            osReadPtr->processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(osReadPtr->getRequest())->get());
            nodeData->setEmbedOsRead(osReadPtr);
          }
          catch (std::exception & e) {
            TRC_INFORMATION("No response => cannot evaluate: " << PAR(nadr));
            continue;
          }

          if (!nodeData->getEmbedOsRead()->is410Compliant()) {
            std::unique_ptr<embed::explore::RawDpaEnumerate> exploreEnumeratePtr(shape_new embed::explore::RawDpaEnumerate(nadr));
            exploreEnumeratePtr->processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(exploreEnumeratePtr->getRequest())->get());
            nodeData->setEmbedExploreEnumerate(exploreEnumeratePtr);
          }
        }
      }

      TRC_FUNCTION_LEAVE("");
    }

    void fullEnum()
    {
      TRC_FUNCTION_ENTER(PAR(m_nadrFullEnumNodeMap.size()));

      if (m_nadrFullEnumNodeMap.size() == 0) {
        TRC_FUNCTION_LEAVE("");
        return;
      }

      std::cout << "Drv Enumeration started at:  " << encodeTimestamp(std::chrono::system_clock::now()) << std::endl;

      auto cp = m_iIqrfDpaService->getCoordinatorParameters();

      bool enumCoord = (0 == m_nadrFullEnumNodeMap.begin()->first);
      int enumNodeCount = m_nadrFullEnumNodeMap.size();
      if (enumCoord) {
        --enumNodeCount; //C is enumed separately
        // coordinator enumeration
        NodeDataPtr & c = m_nadrFullEnumNodeMap.begin()->second;
        c->getNode().setHwpid(0);
        c->getNode().setHwpidVer(0);
        c->getNode().setOsBuild(cp.osBuildWord);
        c->getNode().setDpaVer(cp.dpaVerWord);
      }

      if (enumNodeCount > 1 && cp.dpaVerWord >= 0x402) {
        // enumerate by FRC
        fullEnumByFrc();
      }
      else {
        if (enumNodeCount > 0) {
          // only one or obsolete DPA acoording [C]
          fullEnumByPoll();
        }
      }

      database & db = *m_db;

      for (const auto & it : m_nadrFullEnumNodeMap) {

        if (!m_enumThreadRun) break;

        int nadr = it.first;
        auto & nd = it.second;

        //if (!nd->getNode().isValid()) {
        //  // don't save to DB if invalid
        //  continue;
        //}

        try {
          uint32_t mid = nd->getNode().getMid();
          bool dis = nd->getNode().getDisc();

          if (nd->getNode().isValid()) {
            // valid hwpid, hwpidVer, dpaVer, osBuild => load appropriate drivers and use or create device 
            int hwpid = nd->getNode().getHwpid();
            int hwpidVer = nd->getNode().getHwpidVer();
            int osBuild = nd->getNode().getOsBuild();
            int dpaVer = nd->getNode().getDpaVer();

            std::unique_ptr<int> deviceIdPtr;
            int deviceId = 0;
            Device device(hwpid, hwpidVer, osBuild, dpaVer);

            // get package from JsCache if exists
            const iqrf::IJsCacheService::Package *pckg = nullptr;
            if (hwpid != 0) { // no custom handler => use default pckg0 to resolve periferies
              pckg = m_iJsCacheService->getPackage((uint16_t)hwpid, (uint16_t)hwpidVer, (uint16_t)osBuild, (uint16_t)dpaVer);
            }

            if (pckg) {
              deviceIdPtr = enumerateDeviceInRepo(device, *pckg);
            }
            else {
              deviceIdPtr = enumerateDeviceOutsideRepo(nadr, nd, device);
            }

            db << "begin transaction;";

            if (!deviceIdPtr) {
              // no device in DB => insert
              deviceId = insertDeviceWithDrv(device);
            }
            else {
              // device already in DB => get deviceId
              deviceId = *deviceIdPtr;
            }

            // insert node if not exists
            nodeInDb(mid, deviceId);
            // insert bonded
            bondedInDb(nadr, dis ? 1 : 0, mid, 1);

            db << "commit;";
          }
          else {
            // insert node if not exists
            nodeInDb(mid, 0);
            // insert bonded
            bondedInDb(nadr, dis ? 1 : 0, mid, 0);
          }
        }
        catch (sqlite_exception &e)
        {
          CATCH_EXC_TRC_WAR(sqlite_exception, e, "Unexpected error to store enumeration" << PAR(nadr) << NAME_PAR(code, e.get_code()) << NAME_PAR(ecode, e.get_extended_code()) << NAME_PAR(SQL, e.get_sql()));
          db << "rollback;";
        }
        catch (std::exception &e)
        {
          CATCH_EXC_TRC_WAR(std::exception, e, "Cannot Drv enumerate " << PAR(nadr));
          db << "rollback;";
        }
      }

      m_nadrFullEnumNodeMap.clear();
      std::cout << "Drv Enumeration finished at:  " << encodeTimestamp(std::chrono::system_clock::now()) << std::endl;

      TRC_FUNCTION_LEAVE("");
    }

    void insertNodes(const std::map<int, embed::node::BriefInfo> & nodes)
    {
      TRC_FUNCTION_ENTER("");

      {

        std::unique_lock<std::mutex> lck(m_enumMtx);
        m_nadrInsertNodeMap = nodes;
        m_enumCv.notify_all();
      }

      TRC_FUNCTION_LEAVE("")
    }

    void loadProvisoryDrivers()
    {
      TRC_FUNCTION_ENTER("");

      // daemon wrapper workaround
      std::string wrapperStr;
      std::string fname = m_iLaunchService->getDataDir();
      fname += "/javaScript/DaemonWrapper.js";
      std::ifstream file(fname);
      if (!file.is_open()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot open: " << PAR(fname));
      }
      std::ostringstream strStream;
      strStream << file.rdbuf();
      wrapperStr = strStream.str();

      // get parameters of coordinator - used to select drivers for all other nodes
      int osBuild = 0;
      int dpaVer = 0;

      auto cpars = m_iIqrfDpaService->getCoordinatorParameters();
      osBuild = cpars.osBuildWord;
      dpaVer = cpars.dpaVerWord;

      std::string str2load;

      // get standard drivers refferenced by all hwpid, hwpidVer
      // DriverId, DriverVersion, hwpid, hwpidVer
      std::map<int, std::map<int, std::vector<std::pair<int, int>>>> drivers =
        m_iJsCacheService->getDrivers(embed::os::Read::getOsBuildAsString(osBuild), embed::explore::Enumerate::getDpaVerAsHexaString(dpaVer));

      if (drivers.size() == 0) {
        std::ostringstream os;
        os << std::endl << "Cannot load required package for: "
          << NAME_PAR(os, embed::os::Read::getOsBuildAsString(osBuild))
          << NAME_PAR(dpa, embed::explore::Enumerate::getDpaVerAsHexaString(dpaVer));

        std::cout << os.str() << std::endl;
        TRC_WARNING(os.str());

        for (int dpa = dpaVer - 1; dpa > 300; dpa--) {
          drivers = m_iJsCacheService->getDrivers(embed::os::Read::getOsBuildAsString(osBuild), embed::explore::Enumerate::getDpaVerAsHexaString(dpa));
          if (drivers.size() > 0) {
            std::ostringstream os1;
            os1 << std::endl << "Loaded package for: "
              << NAME_PAR(os, embed::os::Read::getOsBuildAsString(osBuild))
              << NAME_PAR(dpa, embed::explore::Enumerate::getDpaVerAsHexaString(dpa));

            std::cout << os1.str() << std::endl;
            TRC_WARNING(os1.str());

            break;
          }
        }
      }

      // drivers id to be loaded
      std::set<int> driversIdSet;

      for (auto & drv : drivers) {
        int driverId = drv.first;
        int driverVer = 0;

        driversIdSet.insert(driverId);

        if (drv.second.size() > 0) {
          driverVer = drv.second.rbegin()->first; // get the highest one from reverse end
        }
        else {
          TRC_WARNING("Inconsistency in driver versions: " << PAR(driverId) << " no version");
        }
        const IJsCacheService::StdDriver* driver = nullptr;
        driver = m_iJsCacheService->getDriver(driverId, driverVer);
        if (driver) {
          str2load += driver->getDriver();
        }
        else {
          TRC_WARNING("Inconsistency in driver versions: " << PAR(driverId) << PAR(driverVer) << " no driver found");
        }
      }

      str2load += wrapperStr;
      m_iJsRenderService->loadJsCodeFenced(IJsRenderService::HWPID_DEFAULT_MAPPING, str2load, driversIdSet); // provisional context for all with empty custom drivers

      // get all non empty custom drivers because of breakdown
      // hwpid, hwpidVer, driver
      std::map<int, std::map<int, std::string>> customDrivers =
        m_iJsCacheService->getCustomDrivers(embed::os::Read::getOsBuildAsString(osBuild), embed::explore::Enumerate::getDpaVerAsHexaString(dpaVer));

      for (auto d : customDrivers) {
        std::string js = str2load;
        std::string driver = d.second.rbegin()->second; // get the highest hwpidVer one from reverse end
        js += driver;
        m_iJsRenderService->loadJsCodeFenced(IJsRenderService::HWPID_MAPPING_SPACE - d.first, js, driversIdSet);
      }

      TRC_FUNCTION_LEAVE("");
    }

    void loadDeviceDrivers()
    {
      TRC_FUNCTION_ENTER("");

      ////////// daemon wrapper workaround
      static std::string wrapperStr;
      if (wrapperStr.size() == 0) {
        std::string fname = m_iLaunchService->getDataDir();
        fname += "/javaScript/DaemonWrapper.js";
        std::ifstream file(fname);
        if (!file.is_open()) {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot open: " << PAR(fname));
        }
        std::ostringstream strStream;
        strStream << file.rdbuf();
        wrapperStr = strStream.str();
      }
      ////////// end daemon wrapper workaround

      database & db = *m_db;

      try {
        // get [C] device by Nadr = 0
        int coordDeviceId = 0;

        db << "SELECT "
          "Device.Id "
          " FROM Bonded "
          " INNER JOIN Node "
          " ON Bonded.Mid = Node.Mid "
          " INNER JOIN Device "
          " ON Node.DeviceId = Device.Id "
          " WHERE Bonded.Nadr = 0"
          ";"
          >> coordDeviceId;

        // get DeviceId map of DriversId set
        std::map<int, std::set<int>> mapDeviceIdDriverIdSet;
        // devices to reload in JsRender
        std::set<int> reloadDeviceIdSet;

        db << "SELECT "
          "Device.Id "
          ", Driver.Id "
          " FROM Driver "
          " INNER JOIN DeviceDriver "
          " ON Driver.Id = DeviceDriver.DriverId "
          " INNER JOIN Device "
          " ON DeviceDriver.DeviceId = Device.Id "
          ";"
          >> [&](int deviceId, int driverId)
        {
          mapDeviceIdDriverIdSet[deviceId].insert(driverId);
        };

        // check devices if some is to be reloade
        for (auto it : mapDeviceIdDriverIdSet) {
          int deviceId = it.first;
          const std::set<int> & driversIdSet = it.second;
          if (deviceId == coordDeviceId) continue; // don't compare [C] will be reloaded anyway in case of reload any device

          // drivers reload only if set of drivers differs from previous one
          auto origDriversIdSet = m_iJsRenderService->getDriverIdSet(deviceId);
          if (origDriversIdSet.size() != driversIdSet.size()) {
            reloadDeviceIdSet.insert(deviceId);
          }
          else {
            auto nid = driversIdSet.begin();
            for (auto oid : origDriversIdSet) {
              if (*nid++ != oid) {
                reloadDeviceIdSet.insert(deviceId);
                break;
              }
            }
          }
        }

        // now reload devices if any
        if (reloadDeviceIdSet.size() > 0) {
          reloadDeviceIdSet.insert(coordDeviceId); //reload [C] in any case

          for (int deviceId : reloadDeviceIdSet) {

            std::string customDrv;
            std::map<int, Driver> driverIdDriverMap;
            // get drivers according DeviceId
            db << "SELECT "
              " Driver.Id "
              ", Driver.Name "
              ", Driver.StandardId "
              ", Driver.Version "
              ", Driver.Driver "
              " FROM Driver "
              " INNER JOIN DeviceDriver "
              " ON Driver.Id = DeviceDriver.DriverId "
              " INNER JOIN Device "
              " ON DeviceDriver.DeviceId = Device.Id "
              " WHERE DeviceId = ?"
              ";"
              << deviceId
              >> [&](int driverId, std::string name, int sid, int ver, std::string drv)
            {
              driverIdDriverMap.insert(std::make_pair(driverId, Driver(name, sid, ver, drv)));
            };

            // get custom driver
            db << "select CustomDriver from Device "
              " WHERE Id = ?"
              ";"
              << deviceId
              >> customDrv;

            /////////// special [C] handling
            if (deviceId == coordDeviceId) {
              // add the highest standard version as their FRC shall be backward compatible to handle their FRC by [C] context
              // if not compatible we cannot cope with FRC over different device versions of the same standard 
              db << "SELECT "
                " Id "
                ", Name "
                ", StandardId "
                ", Version "
                ", Driver "
                ", MAX(Version) as MaxVersion "
                "FROM Driver "
                "GROUP BY StandardId "
                ";"
                >> [&](int driverId, std::string name, int sid, int ver, std::string drv, int maxVer)
              {
                (void)maxVer; //silence -Wunused-parameter
                driverIdDriverMap.insert(std::make_pair(driverId, Driver(name, sid, ver, drv)));
              };
            }

            std::string str2load;
            std::set<int> driverIdSet;
            for (auto it : driverIdDriverMap) {
              driverIdSet.insert(it.first);
              str2load += it.second.m_drv;
            }
            str2load += customDrv;
            str2load += wrapperStr; // add wrapper
            m_iJsRenderService->loadJsCodeFenced(deviceId, str2load, driverIdSet);

            // map nadrs to device dedicated context
            std::vector<int> nadrs;
            db << "SELECT "
              " Bonded.Nadr "
              " FROM Bonded "
              " INNER JOIN Node "
              " ON Bonded.Mid = Node.Mid "
              " INNER JOIN Device "
              " ON Node.DeviceId = Device.Id "
              " WHERE Node.DeviceId = ? "
              ";"
              << deviceId
              >> [&](int nadr)
            {
              nadrs.push_back(nadr);
            };

            // map according nadr
            for (auto nadr : nadrs) {
              m_iJsRenderService->mapNadrToFenced(nadr, deviceId);
            }

          }

        }

      }
      catch (sqlite_exception &e)
      {
        CATCH_EXC_TRC_WAR(sqlite_exception, e, "Unexpected DB error load drivers failed " << NAME_PAR(code, e.get_code()) << NAME_PAR(ecode, e.get_extended_code()) << NAME_PAR(SQL, e.get_sql()));
      }
      catch (std::exception &e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, "Cannot load drivers ");
      }

      TRC_FUNCTION_LEAVE("");
    }

    void bondedInDb(int nadr, int dis, unsigned mid, int enm)
    {
      TRC_FUNCTION_ENTER(PAR(nadr) << PAR(dis) << PAR(enm));
      database & db = *m_db;

      int count = 0;
      db << "select count(*) from Bonded where Nadr = ?" << nadr >> count;

      if (count == 0) {
        TRC_INFORMATION(PAR(nadr) << " insert into Bonded: " << PAR(nadr) << PAR(dis) << PAR(enm));
        db << "insert into Bonded (Nadr, Dis, Mid, Enm)  values (?, ?, ?, ?);" << nadr << dis << mid << enm;
      }
      else {
        TRC_INFORMATION(PAR(nadr) << " update Bonded: " << PAR(nadr) << PAR(dis) << PAR(enm));
        db << "update Bonded  set Dis = ? , Mid = ?, Enm = ? where Nadr = ?; " << dis << mid << enm << nadr;
      }
      TRC_FUNCTION_LEAVE("");
    }

    std::unique_ptr<int> selectDriver(const IJsCacheService::StdDriver* drv)
    {
      std::unique_ptr<int> id;

      *m_db << "select "
        "d.Id "
        "from "
        "Driver as d "
        "where "
        "d.StandardId = ? and "
        "d.Version = ? "
        ";"
        << drv->getId()
        << drv->getVersion()
        >> [&](std::unique_ptr<int> d)
      {
        id = std::move(d);
      };

      return id;
    }

    // check id device exist and if not insert and return id
    int driverInDb(const IJsCacheService::StdDriver* drv)
    {
      TRC_FUNCTION_ENTER(NAME_PAR(standardId, drv->getId()) << NAME_PAR(version, drv->getVersion()) << NAME_PAR(name, drv->getName()));

      std::string name = drv->getName();
      int standardId = drv->getId();
      int version = drv->getVersion();

      database & db = *m_db;

      std::unique_ptr<int> id = selectDriver(drv);

      if (!id) {
        TRC_INFORMATION(" insert into Driver: " << PAR(standardId) << PAR(version) << PAR(name));

        db << "insert into Driver ("
          "Notes"
          ", Name"
          ", Version"
          ", StandardId"
          ", VersionFlag"
          ", Driver"
          ")  values ( "
          "?"
          ", ?"
          ", ?"
          ", ?"
          ", ?"
          ", ?"
          ");"
          << drv->getNotes()
          << name
          << version
          << standardId
          << drv->getVersionFlags()
          << drv->getDriver()
          ;
      }

      id = selectDriver(drv);
      if (!id) {
        THROW_EXC_TRC_WAR(std::logic_error, " insert into Driver failed: " << PAR(standardId) << PAR(version) << PAR(name))
      }

      TRC_FUNCTION_ENTER("");
      return *id;
    }

    std::unique_ptr<int> selectDevice(const Device& dev)
    {
      std::unique_ptr<int> id;

      *m_db << "select "
        "d.Id "
        "from "
        "Device as d "
        "where "
        "d.Hwpid = ? and "
        "d.HwpidVer = ? and "
        "d.OsBuild = ? and "
        "d.DpaVer = ? "
        ";"
        << dev.m_hwpid
        << dev.m_hwpidVer
        << dev.m_osBuild
        << dev.m_dpaVer
        >> [&](std::unique_ptr<int> d)
      {
        id = std::move(d);
      };

      return id;
    }

    int insertDevice(const Device& dev)
    {
      TRC_FUNCTION_ENTER(NAME_PAR(hwpid, dev.m_hwpid) << NAME_PAR(hwpidVer, dev.m_hwpidVer) << NAME_PAR(osBuild, dev.m_osBuild) << NAME_PAR(dpaVer, dev.m_dpaVer));

      *m_db << "insert into Device ("
        "Hwpid"
        ", HwpidVer"
        ", OsBuild"
        ", DpaVer"
        ", RepoPackageId"
        ", Notes"
        ", HandlerHash"
        ", HandlerUrl"
        ", CustomDriver"
        ", StdEnum"
        ")  values ( "
        "?"
        ", ?"
        ", ?"
        ", ?"
        ", ?"
        ", ?"
        ", ?"
        ", ?"
        ", ?"
        ", ?"
        ");"
        << dev.m_hwpid
        << dev.m_hwpidVer
        << dev.m_osBuild
        << dev.m_dpaVer
        << dev.m_repoPackageId
        << dev.m_notes
        << dev.m_handlerhash
        << dev.m_handlerUrl
        << dev.m_customDriver
        << 0
        ;

      std::unique_ptr<int> id = selectDevice(dev);
      if (!id) {
        THROW_EXC_TRC_WAR(std::logic_error, "insert into Device failed: " <<
          NAME_PAR(hwpid, dev.m_hwpid) << NAME_PAR(hwpidVer, dev.m_hwpidVer) << NAME_PAR(osBuild, dev.m_osBuild) << NAME_PAR(dpaVer, dev.m_dpaVer))
      }

      TRC_FUNCTION_LEAVE("");

      return *id;
    }

    // call in "begin transaction - commit/rollback"
    // return deviceId
    int insertDeviceWithDrv(const Device& dev)
    {
      TRC_FUNCTION_ENTER(NAME_PAR(hwpid, dev.m_hwpid) << NAME_PAR(hwpidVer, dev.m_hwpidVer) << NAME_PAR(osBuild, dev.m_osBuild) << NAME_PAR(dpaVer, dev.m_dpaVer));

      *m_db << "insert into Device ("
        "Hwpid"
        ", HwpidVer"
        ", OsBuild"
        ", DpaVer"
        ", RepoPackageId"
        ", Notes"
        ", HandlerHash"
        ", HandlerUrl"
        ", CustomDriver"
        ", StdEnum"
        ")  values ( "
        "?"
        ", ?"
        ", ?"
        ", ?"
        ", ?"
        ", ?"
        ", ?"
        ", ?"
        ", ?"
        ", ?"
        ");"
        << dev.m_hwpid
        << dev.m_hwpidVer
        << dev.m_osBuild
        << dev.m_dpaVer
        << dev.m_repoPackageId
        << dev.m_notes
        << dev.m_handlerhash
        << dev.m_handlerUrl
        << dev.m_customDriver
        << 0
        ;

      int deviceId = 0;
      *m_db << "select last_insert_rowid();" >> deviceId;

      // store drivers in DB if doesn't exists already
      for (auto d : dev.m_drivers) {
        int driverId = driverInDb(d);
        // store relation to junction table
        *m_db << "insert into DeviceDriver (DeviceId, DriverId) values (?, ?);" << deviceId << driverId;
      }

      TRC_FUNCTION_LEAVE(PAR(deviceId));
      return deviceId;
    }

    // check if node with mid exist and if not insert
    void nodeInDb(unsigned mid, int deviceId)
    {
      TRC_FUNCTION_ENTER(PAR(mid) << PAR(deviceId))

        int count = 0;
      database & db = *m_db;
      db << "select "
        "count(*) "
        "from "
        "Node as n "
        "where "
        "n.Mid = ?"
        ";"
        << mid
        >> count;

      if (0 == count) {
        // mid doesn't exist in DB
        std::unique_ptr<int> did = deviceId != 0 ? std::make_unique<int>(deviceId) : nullptr;
        db << "insert into Node ("
          "Mid"
          ", DeviceId "
          ")  values ( "
          "?"
          ", ?"
          ");"
          << mid
          << did
          ;
      }
      else {
        db << "update Node set "
          "DeviceId = ?"
          " where Mid = ?;"
          << deviceId
          << mid
          ;
      }

      TRC_FUNCTION_LEAVE("")
    }

    void stdEnum()
    {
      TRC_FUNCTION_ENTER("");

      database & db = *m_db;

      // map device nadrs for devices to std enum
      std::map<int, std::vector<int >> mapDeviceVectNadr;
      db << "SELECT"
        " Device.Id"
        ", Bonded.Nadr"
        " FROM Bonded"
        " INNER JOIN Node"
        " ON Bonded.Mid = Node.Mid"
        " INNER JOIN Device"
        " ON Node.DeviceId = Device.Id"
        " WHERE Device.StdEnum = 0 "
        " ORDER BY Bonded.Nadr "
        ";"
        >> [&](int dev, int nadr)
      {
        mapDeviceVectNadr[dev].push_back(nadr);
      };

      if (mapDeviceVectNadr.size() > 0) {
        std::cout << "Std Enumeration started at:  " << encodeTimestamp(std::chrono::system_clock::now()) << std::endl;

        // std enum according first bonded nadr of the device
        for (auto it : mapDeviceVectNadr) {

          if (!m_enumThreadRun) break;

          int deviceId = it.first;
          int nadr = -1;
          std::vector<int> & nadrVect = it.second;

          if (nadrVect.size() > 0) {
            // get random nadr
            static std::random_device rd;
            unsigned idx = rd() % nadrVect.size();
            nadr = nadrVect[idx];
          }
          else {
            TRC_WARNING("Cannot std eval: " << PAR(deviceId) << " as there is no bonded nadr");
            continue;
          }

          try {
            std::vector<int> vectDrivers;

            db << "begin transaction;";

            db << "SELECT "
              "Driver.StandardId"
              " FROM Driver"
              " INNER JOIN DeviceDriver"
              " ON Driver.Id = DeviceDriver.DriverId"
              " INNER JOIN Device"
              " ON DeviceDriver.DeviceId = Device.Id"
              " WHERE Device.Id = ?"
              " ;"
              << deviceId
              >> [&](int drv)
            {
              vectDrivers.push_back(drv);
            };

            for (auto d : vectDrivers) {
              try {
                switch (d) {
                case PERIF_STANDARD_BINOUT:
                  stdBinoutEnum(nadr, deviceId);
                  break;
                case PERIF_STANDARD_LIGHT:
                  stdLightEnum(nadr, deviceId);
                  break;
                case PERIF_STANDARD_SENSOR:
                  stdSensorEnum(nadr, deviceId);
                  break;
                case PERIF_STANDARD_DALI:
                  stdDaliEnum(nadr, deviceId);
                  break;
                default:;
                }
              }
              catch (std::exception &e) {
                CATCH_EXC_TRC_WAR(std::exception, e, "Cannot std enumerate " << PAR(nadr) << NAME_PAR(perif, d));
              }
            }

            db << "update Device set "
              "StdEnum = ?"
              " where Id = ?;"
              << 1
              << deviceId
              ;

            db << "commit;";
          }
          catch (sqlite_exception &e)
          {
            CATCH_EXC_TRC_WAR(sqlite_exception, e, "Unexpected error to store std enumeration" << PAR(nadr) << NAME_PAR(code, e.get_code()) << NAME_PAR(ecode, e.get_extended_code()) << NAME_PAR(SQL, e.get_sql()));
            db << "rollback;";
          }
          catch (std::exception &e)
          {
            CATCH_EXC_TRC_WAR(std::exception, e, "Cannot std enumerate " << PAR(nadr));
            db << "rollback;";
          }
        }

        std::cout << "Std Enumeration finished at:  " << encodeTimestamp(std::chrono::system_clock::now()) << std::endl;
      }

      TRC_FUNCTION_LEAVE("");
    }

    void stdDaliEnum(int nadr, int deviceId)
    {
      TRC_FUNCTION_ENTER(PAR(nadr) << PAR(deviceId));

      //no enum

      database & db = *m_db;

      db << "delete from Dali where DeviceId = ?;"
        << deviceId;

      db << "insert into Dali (DeviceId)  values (?);"
        << deviceId;

      TRC_FUNCTION_LEAVE("")
    }

    void stdLightEnum(int nadr, int deviceId)
    {
      TRC_FUNCTION_ENTER(PAR(nadr) << PAR(deviceId));

      light::jsdriver::Enumerate lightEnum(m_iJsRenderService, nadr);
      lightEnum.processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(lightEnum.getRequest())->get());

      database & db = *m_db;

      db << "delete from Light where DeviceId = ?;"
        << deviceId;

      db << "insert into Light ("
        "DeviceId"
        ", Num"
        ")  values ( "
        "?, ?"
        ");"
        << deviceId << lightEnum.getLightsNum();

      TRC_FUNCTION_LEAVE("")
    }

    void stdBinoutEnum(int nadr, int deviceId)
    {
      TRC_FUNCTION_ENTER(PAR(nadr) << PAR(deviceId));

      binaryoutput::jsdriver::Enumerate binoutEnum(m_iJsRenderService, nadr);
      binoutEnum.processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(binoutEnum.getRequest())->get());

      database & db = *m_db;

      db << "delete from Binout where DeviceId = ?;"
        << deviceId;

      db << "insert into Binout ("
        "DeviceId"
        ", Num"
        ")  values ( "
        "?, ?"
        ");"
        << deviceId << binoutEnum.getBinaryOutputsNum();

      TRC_FUNCTION_LEAVE("")
    }

    void stdSensorEnum(int nadr, int deviceId)
    {
      TRC_FUNCTION_ENTER(PAR(nadr) << PAR(deviceId));

      sensor::jsdriver::Enumerate sensorEnum(m_iJsRenderService, nadr);
      sensorEnum.processDpaTransactionResult(m_iIqrfDpaService->executeDpaTransaction(sensorEnum.getRequest())->get());

      auto const & sensors = sensorEnum.getSensors();
      int idx = 0;

      database & db = *m_db;

      db << "delete from Sensor where DeviceId = ?;"
        << deviceId;

      for (auto const & sen : sensors) {
        const auto & f = sen->getFrcs();
        const auto & e = sen->getFrcs().end();

        int frc2bit = (int)(e != f.find(iqrf::sensor::STD_SENSOR_FRC_2BITS));
        int frc1byte = (int)(e != f.find(iqrf::sensor::STD_SENSOR_FRC_1BYTE));
        int frc2byte = (int)(e != f.find(iqrf::sensor::STD_SENSOR_FRC_2BYTES));
        int frc4byte = (int)(e != f.find(iqrf::sensor::STD_SENSOR_FRC_4BYTES));

        db << "insert into Sensor ("
          "DeviceId"
          ", Idx"
          ", Sid"
          ", Stype"
          ", Name"
          ", SName"
          ", Unit"
          ", Dplac"
          ", Frc2bit"
          ", Frc1byte"
          ", Frc2byte"
          ", Frc4byte"
          ")  values ( "
          "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?"
          ");"
          << deviceId << idx++ << sen->getSid() << sen->getType() << sen->getName() << sen->getShortName() << sen->getUnit() << sen->getDecimalPlaces()
          << frc2bit << frc1byte << frc2byte << frc4byte;
        ;
      }

      TRC_FUNCTION_LEAVE("")
    }

    ////////////////////////////////////////
    // Interface Implementation
    ////////////////////////////////////////

    std::map<int, sensor::EnumeratePtr> getSensors() const
    {
      TRC_FUNCTION_ENTER("");

      std::map<int, sensor::EnumeratePtr> retval;
      database & db = *m_db;

      db <<
        "select "
        "b.Nadr "
        ", s.Idx, s.Sid, s.Stype, s.Name, s.Sname, s.Unit, s.Dplac, s.Frc2bit, s.Frc1byte, s.Frc2byte, s.Frc4byte "
        "from "
        "Bonded as b "
        ", Device as d "
        ", Sensor as s "
        "where "
        "d.Id = (select DeviceId from Node as n where n.Mid = b.Mid) and "
        "d.Id = s.DeviceId "
        "order by s.Idx "
        ";"
        >> [&](int nadr,
          int idx, std::string sid, int stype, std::string name, std::string sname, std::string unit, int dplac,
          int frc2bit, int frc1byte, int frc2byte, int frc4byte)
      {
        std::set<int> frcs;
        if (frc2bit == 1) frcs.insert(iqrf::sensor::STD_SENSOR_FRC_2BITS);
        if (frc1byte == 1) frcs.insert(iqrf::sensor::STD_SENSOR_FRC_1BYTE);
        if (frc2byte == 1) frcs.insert(iqrf::sensor::STD_SENSOR_FRC_2BYTES);
        if (frc4byte == 1) frcs.insert(iqrf::sensor::STD_SENSOR_FRC_4BYTES);

        sensor::InfoEnumerate::InfoSensorPtr infoSensorPtr(shape_new sensor::InfoEnumerate::InfoSensor(idx, sid, stype, name, sname, unit, dplac, frcs));
        sensor::EnumeratePtr & enumeratePtr = retval[nadr];
        if (!enumeratePtr) {
          enumeratePtr.reset(shape_new sensor::InfoEnumerate());
        }
        dynamic_cast<sensor::InfoEnumerate*>(enumeratePtr.get())->addInfoSensor(std::move(infoSensorPtr));
      };

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    std::map<int, binaryoutput::EnumeratePtr> getBinaryOutputs() const
    {
      TRC_FUNCTION_ENTER("");

      std::map<int, binaryoutput::EnumeratePtr> retval;
      database & db = *m_db;

      db <<
        "select "
        "b.Nadr "
        ", o.Num "
        "from "
        "Bonded as b "
        ", Device as d "
        ", Binout as o "
        "where "
        "d.Id = (select DeviceId from Node as n where n.Mid = b.Mid) and "
        "d.Id = o.DeviceId "
        ";"
        >> [&](int nadr, int num)
      {
        retval.insert(std::make_pair(nadr, binaryoutput::InfoEnumeratePtr(shape_new binaryoutput::InfoEnumerate(num))));
      };

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    std::map<int, dali::EnumeratePtr> getDalis() const
    {
      TRC_FUNCTION_ENTER("");

      std::map<int, dali::EnumeratePtr> retval;
      database & db = *m_db;

      db <<
        "select "
        "b.Nadr "
        "from "
        "Bonded as b "
        ", Device as d "
        ", Dali as o "
        "where "
        "d.Id = (select DeviceId from Node as n where n.Mid = b.Mid) and "
        "d.Id = o.DeviceId "
        ";"
        >> [&](int nadr)
      {
        retval.insert(std::make_pair(nadr, dali::InfoEnumeratePtr(shape_new dali::InfoEnumerate())));
      };

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    std::map<int, light::EnumeratePtr> getLights() const
    {
      TRC_FUNCTION_ENTER("");

      std::map<int, light::EnumeratePtr> retval;
      database & db = *m_db;

      db <<
        "select "
        "b.Nadr "
        ", o.Num "
        "from "
        "Bonded as b "
        ", Device as d "
        ", Light as o "
        "where "
        "d.Id = (select DeviceId from Node as n where n.Mid = b.Mid) and "
        "d.Id = o.DeviceId "
        ";"
        >> [&](int nadr, int num)
      {
        retval.insert(std::make_pair(nadr, light::InfoEnumeratePtr(shape_new light::InfoEnumerate(num))));
      };

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    std::map<int, embed::node::BriefInfoPtr> getNodes() const
    {
      TRC_FUNCTION_ENTER("");

      std::map<int, embed::node::BriefInfoPtr> retval;
      database & db = *m_db;

      db <<
        "select "
        "b.Nadr "
        ", b.Dis "
        ", b.Mid "
        ", b.Enm "
        ", d.Hwpid "
        ", d.HwpidVer "
        ", d.OsBuild "
        ", d.DpaVer "
        "from "
        "Bonded as b "
        ", Device as d "
        "where "
        "d.Id = (select DeviceId from Node as n where n.Mid = b.Mid) "
        ";"
        >> [&](int nadr, int dis, unsigned mid, int enm, int hwpid, int hwpidVer, int osBuild, int dpaVer)
      {
        retval.insert(std::make_pair(nadr, embed::node::BriefInfoPtr(
          shape_new embed::node::info::BriefInfo(mid, dis != 0, hwpid, hwpidVer, osBuild, dpaVer, enm != 0)
        )));
      };

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    //TODO API
    void startEnumeration()
    {
      TRC_FUNCTION_ENTER("");
      if (!m_enumThreadRun) {
        if (m_enumThread.joinable()) {
          m_enumThread.join();
        }
        m_enumThreadRun = true;
        m_enumThread = std::thread([&]() { runEnum(); });
      }
      TRC_FUNCTION_LEAVE("")
    }

    //TODO API
    void stopEnumeration()
    {
      TRC_FUNCTION_ENTER("");
      m_enumThreadRun = false;
      m_enumCv.notify_all();

      if (m_enumThread.joinable()) {
        m_enumThread.join();
      }
      TRC_FUNCTION_LEAVE("")
    }

    //TODO API
    void enumerate()
    {
      TRC_FUNCTION_ENTER("");
      startEnumeration();
      {
        std::unique_lock<std::mutex> lck(m_enumMtx);
        m_enumCv.notify_all();
      }
      TRC_FUNCTION_LEAVE("")
    }

    //TODO API
    void refreshEnumerate()
    {
      TRC_FUNCTION_ENTER("");
      stopEnumeration();
      //TODO implement, API
      //delete and recreate DB
      TRC_FUNCTION_LEAVE("")
    }

    //TODO API
    // get non enumerated nodes

    //TODO API
    // remove unbond nodes from DB - nodes are not by default deleted if unbonded

    // analyze incoming DPA responses. If there is a msg with influence to DB cahed data it starts enum
    // it is hooked to IqrfDpa via callback
    void analyzeAnyMessage(const DpaMessage & msg)
    {
      auto messageDirection = msg.MessageDirection();

      if (messageDirection != DpaMessage::MessageType::kResponse ||
        (msg.DpaPacket().DpaResponsePacket_t.ResponseCode & STATUS_ASYNC_RESPONSE)) {
        return;
      }

      if (msg.NodeAddress() != 0) {
        return; //just coord addr
      }

      if (msg.PeripheralType() != 0) {
        return; //just coord perif
      }

      int cmd = msg.PeripheralCommand() & ~0x80;

      if (
        cmd == CMD_COORDINATOR_CLEAR_ALL_BONDS
        || cmd == CMD_COORDINATOR_BOND_NODE
        || cmd == CMD_COORDINATOR_REMOVE_BOND
        || cmd == CMD_COORDINATOR_DISCOVERY
        || cmd == CMD_COORDINATOR_RESTORE
        || cmd == CMD_COORDINATOR_SMART_CONNECT
        || cmd == CMD_COORDINATOR_SET_MID
        ) {

        m_repeatEnum = true; //repeat if just running
        TRC_INFORMATION("detected: " << PAR(cmd));
        //if (!m_iIqrfDpaService->hasExclusiveAccess()) {
        m_enumCv.notify_all();
        //}
        //else {
        //  TRC_INFORMATION("exclusive access detected => enum waits ... ");
        //}
      }
    }

    rapidjson::Document getNodeMetaData(int nadr) const
    {
      TRC_FUNCTION_ENTER("");

      std::string metaData;
      database & db = *m_db;

      db <<
        "select "
        "n.metaData "
        "from "
        "Bonded as b "
        ", Node as n "
        "where "
        "n.mid = b.mid "
        "and b.nadr = ? "
        ";"
        << nadr
        >> tie(metaData)
        ;

      rapidjson::Document doc;
      doc.Parse(metaData);

      if (doc.HasParseError()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Json parse error: " << NAME_PAR(emsg, doc.GetParseError()) <<
          NAME_PAR(eoffset, doc.GetErrorOffset()));
      }

      TRC_FUNCTION_LEAVE("");
      return doc;
    }

    void setNodeMetaData(int nadr, const rapidjson::Value & metaData)
    {
      TRC_FUNCTION_ENTER("");

      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      metaData.Accept(writer);
      std::string md = buffer.GetString();

      *m_db << "update Node set metaData = ? where mid = (select mid from Bonded where nadr = ?);" << md << nadr;

      TRC_FUNCTION_LEAVE("");
    }

    void attachInterface(iqrf::IJsRenderService* iface)
    {
      TRC_FUNCTION_ENTER(PAR(iface));
      m_iJsRenderService = iface;
      TRC_FUNCTION_LEAVE("")
    }

    void detachInterface(iqrf::IJsRenderService* iface)
    {
      TRC_FUNCTION_ENTER(PAR(iface));
      if (m_iJsRenderService == iface) {
        m_iJsCacheService = nullptr;
      }
      TRC_FUNCTION_LEAVE("")
    }

    void attachInterface(iqrf::IJsCacheService* iface)
    {
      TRC_FUNCTION_ENTER(PAR(iface));
      m_iJsCacheService = iface;
      TRC_FUNCTION_LEAVE("")
    }

    void detachInterface(iqrf::IJsCacheService* iface)
    {
      TRC_FUNCTION_ENTER(PAR(iface));
      if (m_iJsCacheService == iface) {
        m_iJsCacheService = nullptr;
      }
      TRC_FUNCTION_LEAVE("")
    }

    void attachInterface(iqrf::IIqrfDpaService* iface)
    {
      TRC_FUNCTION_ENTER(PAR(iface));
      m_iIqrfDpaService = iface;
      TRC_FUNCTION_LEAVE("")
    }

    void detachInterface(iqrf::IIqrfDpaService* iface)
    {
      TRC_FUNCTION_ENTER(PAR(iface));
      if (m_iIqrfDpaService == iface) {
        m_iIqrfDpaService = nullptr;
      }
      TRC_FUNCTION_LEAVE("")
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

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfInfo instance activate" << std::endl <<
        "******************************"
      );

      modify(props);

      m_iIqrfDpaService->registerAnyMessageHandler(m_instanceName, [&](const DpaMessage & msg)
      {
        analyzeAnyMessage(msg);
      }
      );

      initDb();

      loadProvisoryDrivers();

      m_repeatEnum = false;
      m_enumThreadRun = false;
      if (m_enumAtStartUp) {
        startEnumeration();
      }

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;
      const Document& doc = props->getAsJson();

      m_instanceName = Pointer("/instance").Get(doc)->GetString();

      {
        const Value* val = Pointer("/enumAtStartUp").Get(doc);
        if (val && val->IsBool()) {
          m_enumAtStartUp = val->GetBool();
        }
      }

      {
        const Value* val = Pointer("/enumPeriod").Get(doc);
        if (val && val->IsInt()) {
          m_enumPeriod = val->GetInt();
        }
      }

      {
        const Value* val = Pointer("/enumUniformDpaVer").Get(doc);
        if (val && val->IsBool()) {
          m_enumUniformDpaVer = val->GetBool();
        }
      }

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfInfo instance deactivate" << std::endl <<
        "******************************"
      );

      m_enumThreadRun = false;
      m_enumCv.notify_all();
      if (m_enumThread.joinable()) {
        m_enumThread.join();
      }

      m_iIqrfDpaService->unregisterAnyMessageHandler(m_instanceName);

      TRC_FUNCTION_LEAVE("")
    }

  };

  /////////////////////////////
  IqrfInfo::IqrfInfo()
  {
    m_imp = shape_new Imp();
  }

  IqrfInfo::~IqrfInfo()
  {
    delete m_imp;
  }

  std::map<int, sensor::EnumeratePtr> IqrfInfo::getSensors() const
  {
    return m_imp->getSensors();
  }

  std::map<int, binaryoutput::EnumeratePtr> IqrfInfo::getBinaryOutputs() const
  {
    return m_imp->getBinaryOutputs();
  }

  std::map<int, dali::EnumeratePtr> IqrfInfo::getDalis() const
  {
    return m_imp->getDalis();
  }

  std::map<int, light::EnumeratePtr> IqrfInfo::getLights() const
  {
    return m_imp->getLights();
  }

  std::map<int, embed::node::BriefInfoPtr> IqrfInfo::getNodes() const
  {
    return m_imp->getNodes();
  }

  void IqrfInfo::insertNodes(const std::map<int, embed::node::BriefInfo> & nodes)
  {
    return m_imp->insertNodes(nodes);
  }

  void IqrfInfo::startEnumeration()
  {
    return m_imp->startEnumeration();
  }

  rapidjson::Document IqrfInfo::getNodeMetaData(int nadr) const
  {
    return m_imp->getNodeMetaData(nadr);
  }

  void IqrfInfo::setNodeMetaData(int nadr, const rapidjson::Value & metaData)
  {
    m_imp->setNodeMetaData(nadr, metaData);
  }

  void IqrfInfo::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void IqrfInfo::deactivate()
  {
    m_imp->deactivate();
  }

  void IqrfInfo::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void IqrfInfo::attachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void IqrfInfo::detachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void IqrfInfo::attachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void IqrfInfo::detachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void IqrfInfo::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void IqrfInfo::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void IqrfInfo::attachInterface(shape::ILaunchService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void IqrfInfo::detachInterface(shape::ILaunchService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void IqrfInfo::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void IqrfInfo::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
