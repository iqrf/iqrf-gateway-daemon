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
    //aux class initiated from DB to compare with fast enum result
    class BondNodeDb
    {
    public:
      BondNodeDb() = delete;
      BondNodeDb(
        int nadr,
        unsigned mid,
        int discovered,
        int hwpid,
        int hwpidVer,
        int osBuild,
        int dpaVer
      )
        : m_nadr(nadr)
        , m_mid(mid)
        , m_discovered(discovered)
        , m_hwpid(hwpid)
        , m_hwpidVer(hwpidVer)
        , m_osBuild(osBuild)
        , m_dpaVer(dpaVer)
      {}

      int m_nadr;
      unsigned m_mid;
      int m_discovered;
      int m_hwpid;
      int m_hwpidVer;
      int m_osBuild;
      int m_dpaVer;
    };

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

    // TODO siplify
    class NodeData
    {
    public:
      NodeData() = delete;

      NodeData(int nadr, int hwpid, embed::explore::RawDpaEnumeratePtr & e, embed::os::RawDpaReadPtr & r)
        :m_nadr(nadr)
        , m_hwpid(hwpid)
        , m_exploreEnumerate(std::move(e))
        , m_osRead(std::move(r))
      {}

      //const embed::explore::EnumeratePtr & getEmbedExploreEnumerate() const
      //{
      //  return m_exploreEnumerate;
      //}

      //const embed::os::ReadPtr & getEmbedOsRead() const
      //{
      //  return m_osRead;
      //}

      //unsigned mid = nd->getEmbedOsRead()->getMid();
      //bool dis = (m_fastEnum->getDiscovered().find(nadr) != m_fastEnum->getDiscovered().end());
      //int hwpid = nd->getHwpid();
      //int hwpidVer = nd->getEmbedExploreEnumerate()->getHwpidVer();
      //int osBuild = nd->getEmbedOsRead()->getOsBuild();
      //int dpaVer = nd->getEmbedExploreEnumerate()->getDpaVer();

      unsigned getMid() const
      {
        return m_osRead->getMid();
      }
      
      int getHwpidVer() const
      {
        return m_exploreEnumerate->getHwpidVer();
      }
      
      int getOsBuild() const
      {
        return m_osRead->getOsBuild();
      }
      
      int getDpaVer() const
      {
        return m_exploreEnumerate->getDpaVer();
      }

      int getNadr() const
      {
        return m_nadr;
      }

      int getHwpid() const
      {
        return m_hwpid;
      }

      const std::set<int> getEmbedPer() const
      {
        return m_exploreEnumerate->getEmbedPer();
      }

      const std::set<int> & getUserPer() const
      {
        return m_exploreEnumerate->getUserPer();
      }

      int getModeStd() const
      {
        return m_exploreEnumerate->getModeStd();
      }

      int getStdAndLpSupport() const
      {
        return m_exploreEnumerate->getStdAndLpSupport();
      }

    private:
      int m_nadr;
      int m_hwpid;
      embed::explore::EnumeratePtr m_exploreEnumerate;
      embed::os::ReadPtr m_osRead;
    };
    typedef std::unique_ptr<NodeData> NodeDataPtr;

    class FastEnumeration
    {
    public:
      class Enumerated : public embed::node::info::BriefInfo
      {
      public:
        Enumerated() = delete;
        Enumerated(unsigned mid, bool disc, int hwpid, int hwpidVer, int osBuild, int dpaVer, NodeDataPtr nodeDataPtr)
          :embed::node::info::BriefInfo(mid, disc, hwpid, hwpidVer, osBuild, dpaVer)
          , m_nodeDataPtr(std::move(nodeDataPtr))
        {}
        NodeDataPtr getNodeData() { return std::move(m_nodeDataPtr); }
        virtual ~Enumerated() {}

      private:
        NodeDataPtr m_nodeDataPtr;
      };
      typedef std::unique_ptr<Enumerated> EnumeratedPtr;

      const std::map<int, EnumeratedPtr> & getEnumerated() const { return m_enumeratedMap; }
      const std::set<int> & getBonded() const { return m_bonded; }
      const std::set<int> & getDiscovered() const { return m_discovered; }
      const std::set<int> & getNonDiscovered() const { return m_nonDiscovered; }
      const std::map<int, uint32_t>  & getBondedMidMap() { return m_bondedMidMap; }

      void setBondedDiscovered(const std::set<int> &bonded, const std::set<int> &discovered)
      {
        m_bonded = bonded;
        m_discovered = discovered;
        for (auto i : m_bonded) {
          if (m_discovered.find(i) == m_discovered.end()) {
            m_nonDiscovered.insert(i);
          }
        }
      }

      void setBondedMidMap(const std::map<int, uint32_t>  & bondedMidMap)
      {
        m_bondedMidMap = bondedMidMap;
      }

      void addItem(int nadr, unsigned mid, bool disc, int hwpid, int hwpidVer, int osBuild, int dpaVer, NodeDataPtr nodeDataPtr)
      {
        m_enumeratedMap.insert(std::make_pair(nadr, EnumeratedPtr(shape_new Enumerated(mid, disc, hwpid, hwpidVer, osBuild, dpaVer, std::move(nodeDataPtr)))));
      }
      virtual ~FastEnumeration() {}
    private:
      std::map<int, EnumeratedPtr> m_enumeratedMap;
      std::set<int> m_bonded;
      std::set<int> m_discovered;
      std::set<int> m_nonDiscovered;
      std::map<int, uint32_t> m_bondedMidMap;
    };
    typedef std::unique_ptr<FastEnumeration> FastEnumerationPtr;

  private:

    IJsRenderService* m_iJsRenderService = nullptr;
    IJsCacheService* m_iJsCacheService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    shape::ILaunchService* m_iLaunchService = nullptr;

    std::unique_ptr<database> m_db;

    // get m_bonded map according nadr from DB
    std::map<int, BondNodeDb> m_mapNadrBondNodeDb;
    // need full enum
    std::set<int> m_nadrFullEnum;
    bool m_enumAtStartUp = false;
    std::thread m_enumThread;
    std::atomic_bool m_enumThreadRun;
    std::mutex m_enumMtx;

    FastEnumerationPtr m_fastEnum;

  public:
    Imp()
    {
      m_enumThreadRun = false;
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

        //update DB
        try {
          SqlFile::makeSqlFile(db, sqlpath + "init/IqrfInfo_update1.db.sql");
        }
        catch (sqlite_exception &e)
        {
          db << "rollback;";
          CATCH_EXC_TRC_WAR(sqlite_exception, e, "Update DB: " << NAME_PAR(code, e.get_code()) << NAME_PAR(ecode, e.get_extended_code()) << NAME_PAR(SQL, e.get_sql()));
        }

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

      std::lock_guard<std::mutex> lck(m_enumMtx);

      try {
        std::cout << std::endl << "Fast Enumeration started at: " << encodeTimestamp(std::chrono::system_clock::now());

        auto exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
        if (!exclusiveAccess) {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot get exclusive access to IqrfDpa");
        }

        fastEnum(exclusiveAccess);
        std::cout << std::endl << "Full Enumeration started at: " << encodeTimestamp(std::chrono::system_clock::now());
        fullEnum(exclusiveAccess);
        loadDrivers();
        std::cout << std::endl << "Std Enumeration started at:  " << encodeTimestamp(std::chrono::system_clock::now());
        stdEnum(exclusiveAccess);
        std::cout << std::endl << "Enumeration finished at:     " << encodeTimestamp(std::chrono::system_clock::now()) << std::endl;

        exclusiveAccess.reset();
      }
      catch (std::exception & e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Enumeration failure");
        std::cout << std::endl << "Enumeration failure at:      " << encodeTimestamp(std::chrono::system_clock::now()) << std::endl <<
          e.what() << std::endl;
      }

      m_fastEnum.release();
      m_enumThreadRun = false;

      TRC_FUNCTION_LEAVE("");
    }

    // read bonded mid for coordinator eeeprom
    std::map<int, uint32_t> getBondedMids(IIqrfDpaService::ExclusiveAccessPtr & exclusiveAccess, const std::set<int> & bonded) const
    {
      TRC_FUNCTION_ENTER("");

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

      // retval
      std::map<int, uint32_t> bondedMidMap;

      // read C eeeprom up to nadr storage
      for (int i = 0; i < maxhit + 1; i++) {
        uint8_t len = i <= maxhit ? maxlen : remain;
        if (len > 0) {
          uint16_t adr = (uint16_t)(0x4000 + i * maxlen);
          iqrf::embed::eeeprom::rawdpa::Read eeepromRead(0, adr, len);
          std::unique_ptr<IDpaTransactionResult2> trnResult;
          exclusiveAccess->executeDpaTransactionRepeat(eeepromRead.getRequest(), trnResult, 3);
          eeepromRead.processDpaTransactionResult(std::move(trnResult));
          pdataVec.insert(pdataVec.end(), eeepromRead.getPdata().begin(), eeepromRead.getPdata().end());
        }
      }

      // parse stored mids TODO vrn data?
      for (int nadr : bonded) {
        int m = nadr * 8;
        uint32_t mid = (unsigned)pdataVec[m] | ((unsigned)pdataVec[m+1] << 8) | ((unsigned)pdataVec[m+2] << 16) | ((unsigned)pdataVec[m+3] << 24);
        bondedMidMap[nadr] = mid;
      }

      auto mids1 = getBondedDpaVer(exclusiveAccess, bonded);
      
      TRC_FUNCTION_LEAVE("");
      return bondedMidMap;
    }

    // read bonded mid for coordinator eeeprom
    std::map<int, uint32_t> getBondedDpaVer(IIqrfDpaService::ExclusiveAccessPtr & exclusiveAccess, const std::set<int> & bonded) const
    {
      TRC_FUNCTION_ENTER("");

      uint16_t address = 0x04a0; //mid hardcoded ???
      //TPerOSRead_Response osRead;
      //void * relative = &(osRead.OsBuild) - &osRead;

      //PNUM_ENUMERATION, CMD_GET_PER_INFO

      //uint16_t address = 0x4A4; //os
      uint8_t pnum = PNUM_OS;
      uint8_t pcmd = CMD_OS_READ;
      
      std::set<int> bondedTest;
      for (int i = 0; i < 64; i++) {
        bondedTest.insert(i);
      }
      bondedTest.insert(bonded.begin(), bonded.end());

      iqrf::embed::frc::rawdpa::MemoryRead4B_SendSelective frc(address, pnum, pcmd);
      std::vector<std::set<int>> setVect = frc.splitSelectedNode(bondedTest);
      std::map<int, uint32_t> mids;

      for (const auto & s : setVect) {
        frc.setSelectedNodes(s);
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(frc.getRequest(), transResult, 3);
        frc.processDpaTransactionResult(std::move(transResult));
        frc.getFrcDataAs(mids);
      }

      TRC_FUNCTION_LEAVE("");
      return mids;
    }

    //////////////////////////////////////////////////////
    // Sets selected nodes to specified PData of FRC command
    void setFRCSelectedNodes(uns8* pData, const std::vector<uint8_t>& selectedNodes)
    {
      // Initialize to zero values
      memset(pData, 0, 30 * sizeof(uns8));
      for (uint8_t i : selectedNodes)
      {
        uns8 byteIndex = i / 8;
        uns8 bitIndex = i % 8;
        pData[byteIndex] |= (0x01 << bitIndex);
      }
    }

    // FRC_PrebondedMemoryReadPlus1 (used to read MIDs and HWPID)
    std::basic_string<uint8_t> FrcPrebondedMemoryRead(
      IIqrfDpaService::ExclusiveAccessPtr & exclusiveAccess
      , const std::vector<uint8_t>& prebondedNodes
      , const uint8_t nodeSeed
      , const uint8_t offset
      , const uint16_t address
      , const uint8_t PNUM
      , const uint8_t PCMD
    )
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage prebondedMemoryRequest;
        DpaMessage::DpaPacket_t prebondedMemoryPacket;
        prebondedMemoryPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        prebondedMemoryPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
        prebondedMemoryPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
        prebondedMemoryPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        // FRC Command
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_PrebondedMemoryReadPlus1;
        // Selected nodes - prebonded alive nodes
        setFRCSelectedNodes(prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes, prebondedNodes);
        // Node seed, offset
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00] = nodeSeed;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x01] = offset;
        // OS Read command
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x02] = address & 0xff;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x03] = address >> 0x08;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x04] = PNUM;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x05] = PCMD;
        prebondedMemoryPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x06] = 0x00;
        prebondedMemoryRequest.DataToBuffer(prebondedMemoryPacket.Buffer, sizeof(TDpaIFaceHeader) + 38);
        // Execute the DPA request
        exclusiveAccess->executeDpaTransactionRepeat(prebondedMemoryRequest, transResult, 3);
        TRC_DEBUG("Result from FRC Prebonded Memory Read transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("FRC FRC Prebonded Memory Read successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, prebondedMemoryRequest.PeripheralType())
          << NAME_PAR(Node address, prebondedMemoryRequest.NodeAddress())
          << NAME_PAR(Command, (int)prebondedMemoryRequest.PeripheralCommand())
        );
        // Data from FRC
        std::basic_string<uint8_t> prebondedMemoryData;
        // Check status
        uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
        if (status < 0xFD)
        {
          TRC_INFORMATION("FRC Prebonded Memory Read status ok." << NAME_PAR_HEX("Status", status));
          //prebondedMemoryData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData + sizeof(TMID), 51);
          prebondedMemoryData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData + sizeof(uint32_t), 51);
          TRC_DEBUG("Size of FRC data: " << PAR(prebondedMemoryData.size()));
        }
        else
        {
          TRC_WARNING("FRC Prebonded Memory Read NOT ok." << NAME_PAR_HEX("Status", (int)status));
          //AutonetworkError error(AutonetworkError::Type::PrebondedMemoryRead, "Bad FRC status.");
          //autonetworkResult.setError(error);
          THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)status));
        }
        // Add FRC result
        //autonetworkResult.addTransactionResult(transResult);

        // Read FRC extra result (if needed)
        if (prebondedNodes.size() > 12)
        {
          // Read FRC extra results
          DpaMessage extraResultRequest;
          DpaMessage::DpaPacket_t extraResultPacket;
          extraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
          extraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
          extraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
          extraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          extraResultRequest.DataToBuffer(extraResultPacket.Buffer, sizeof(TDpaIFaceHeader));
          // Execute the DPA request
          exclusiveAccess->executeDpaTransactionRepeat(extraResultRequest, transResult, 3);
          TRC_DEBUG("Result from FRC CMD_FRC_EXTRARESULT transaction as string:" << PAR(transResult->getErrorString()));
          dpaResponse = transResult->getResponse();
          TRC_INFORMATION("FRC CMD_FRC_EXTRARESULT successful!");
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(Peripheral type, extraResultRequest.PeripheralType())
            << NAME_PAR(Node address, extraResultRequest.NodeAddress())
            << NAME_PAR(Command, (int)extraResultRequest.PeripheralCommand())
          );
          // Append FRC data
          prebondedMemoryData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData, 9);
          // Add FRC extra result
          //autonetworkResult.addTransactionResult(transResult);
        }
        TRC_FUNCTION_LEAVE("");
        return prebondedMemoryData;
      }
      catch (std::exception& e)
      {
        //AutonetworkError error(AutonetworkError::Type::PrebondedMemoryRead, e.what());
        //autonetworkResult.setError(error);
        //autonetworkResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }
    //////////////////////////////////////////////////////


    // performs fast enumeration
    // it gets nadr, mid, os, hwpid, hwpidVer => if differ from DB states add to candidates for full enum
    FastEnumerationPtr getFastEnumeration(IIqrfDpaService::ExclusiveAccessPtr & exclusiveAccess) const
    {
      TRC_FUNCTION_ENTER("");

      std::unique_ptr<FastEnumeration> retval(shape_new FastEnumeration);

      iqrf::embed::coordinator::RawDpaBondedDevices iqrfEmbedCoordinatorBondedDevices;
      iqrf::embed::coordinator::RawDpaDiscoveredDevices iqrfEmbedCoordinatorDiscoveredDevices;

      {
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(iqrfEmbedCoordinatorBondedDevices.getRequest(), transResult, 3);
        iqrfEmbedCoordinatorBondedDevices.processDpaTransactionResult(std::move(transResult));
      }

      {
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(iqrfEmbedCoordinatorDiscoveredDevices.getRequest(), transResult, 3);
        iqrfEmbedCoordinatorDiscoveredDevices.processDpaTransactionResult(std::move(transResult));
      }

      retval->setBondedMidMap(getBondedMids(exclusiveAccess, iqrfEmbedCoordinatorBondedDevices.getBondedDevices()));

      retval->setBondedDiscovered(iqrfEmbedCoordinatorBondedDevices.getBondedDevices(), iqrfEmbedCoordinatorDiscoveredDevices.getDiscoveredDevices());
      std::set<int> evaluated = retval->getBonded();
      std::set<int> discovered = retval->getDiscovered();
      evaluated.insert(0); //eval coordinator

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    NodeDataPtr getNodeDataPriv(uint16_t nadr, std::unique_ptr<iqrf::IIqrfDpaService::ExclusiveAccess> & exclusiveAccess) const
    {
      TRC_FUNCTION_ENTER(nadr);

      NodeDataPtr nodeData;

      std::unique_ptr<embed::explore::RawDpaEnumerate> exploreEnumeratePtr(shape_new embed::explore::RawDpaEnumerate(nadr));
      std::unique_ptr <embed::os::RawDpaRead> osReadPtr(shape_new embed::os::RawDpaRead(nadr));

      {
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(osReadPtr->getRequest(), transResult, 3);
        osReadPtr->processDpaTransactionResult(std::move(transResult));
      }

      {
        std::unique_ptr<IDpaTransactionResult2> transResult;
        exclusiveAccess->executeDpaTransactionRepeat(exploreEnumeratePtr->getRequest(), transResult, 3);
        exploreEnumeratePtr->processDpaTransactionResult(std::move(transResult));
      }

      nodeData.reset(shape_new NodeData(nadr, osReadPtr->getHwpid(), exploreEnumeratePtr, osReadPtr));

      TRC_FUNCTION_LEAVE("");
      return nodeData;
    }

    void fastEnum(IIqrfDpaService::ExclusiveAccessPtr & exclusiveAccess)
    {
      TRC_FUNCTION_ENTER("");

      m_fastEnum = getFastEnumeration(exclusiveAccess);

      database & db = *m_db;

      db << "select "
        "b.Nadr "
        ", b.Mid "
        ", b.Dis "
        ", d.Hwpid "
        ", d.HwpidVer "
        ", d.OsBuild "
        ", d.DpaVer "
        "from "
        "Bonded as b "
        ", Device as d "
        " where "
        " d.Id = (select DeviceId from Node as n where n.Mid = b.Mid) "
        ";"
        >> [&](
          int nadr,
          unsigned mid,
          int discovery,
          int hwpid,
          int hwpidVer,
          int osBuild,
          int dpaVer
          )
      {
        m_mapNadrBondNodeDb.insert(std::make_pair(nadr, BondNodeDb(
          nadr,
          mid,
          discovery,
          hwpid,
          hwpidVer,
          osBuild,
          dpaVer
        )));
      };

      auto const & bondedMidMap = m_fastEnum->getBondedMidMap();

      // delete Nadr from DB if it doesn't exist in Net
      for (const auto & bo : m_mapNadrBondNodeDb) {
        int nadr = bo.first;
        const auto & b = bo.second;
        auto found = bondedMidMap.find(nadr);
        if (found == bondedMidMap.end()) {
          // Nadr not found in Net => delete from Bonded
          TRC_INFORMATION(PAR(nadr) << " remove from bonded list")
            db << "delete from Bonded where Nadr = ?;" << nadr;
        }
      }

      // compare fast enum and DB
      for (const auto & p : bondedMidMap) {
        //const auto & e = *(en.second);
        int nadr = p.first;
        uint32_t mid = p.second;
        auto found = m_mapNadrBondNodeDb.find(nadr);
        if (found == m_mapNadrBondNodeDb.end()) {
          // Nadr from Net not found in DB => provide full enum
          m_nadrFullEnum.insert(nadr);
        }
        else {
          auto const & n = found->second;
          if (mid != n.m_mid) {
            // Nadr from Net is already in DB, but fast enum comparison failed => provide full enum
            TRC_INFORMATION(PAR(nadr) << " fast enum does not fit => schedule full enum")
              m_nadrFullEnum.insert(nadr);
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

    // return deviceId
    std::unique_ptr<int> enumerateDeviceOutsideRepo(int nadr, Device & d, const std::set<int> & embedPer, const std::set<int> & userPer
      , IIqrfDpaService::ExclusiveAccessPtr & exclusiveAccess)
    {
      TRC_FUNCTION_ENTER(PAR(d.m_hwpid) << PAR(d.m_hwpidVer) << PAR(d.m_osBuild) << PAR(d.m_dpaVer));

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
          for (auto per : embedPer) {
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

        for (auto per : userPer) {

          //Get peripheral information for sensor, binout, dali, light and TODO other std if presented
          if (PERIF_STANDARD_BINOUT == per || PERIF_STANDARD_SENSOR == per || PERIF_STANDARD_DALI == per || PERIF_STANDARD_LIGHT == per) {

            embed::explore::RawDpaPeripheralInformation perInfo(nadr, per);
            perInfo.processDpaTransactionResult(exclusiveAccess->executeDpaTransaction(perInfo.getRequest())->get());

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

    void fullEnum(IIqrfDpaService::ExclusiveAccessPtr & exclusiveAccess)
    {
      TRC_FUNCTION_ENTER("");

      database & db = *m_db;

      for (auto nadr : m_nadrFullEnum) {

        try {

          // enum thread stopped
          if (!m_enumThreadRun) break;

          NodeDataPtr nd;

          // try to get node data from fast enum
          //auto found = m_fastEnum->getEnumerated().find(nadr);
          //if (found != m_fastEnum->getEnumerated().end()) {
          //  nd = found->second->getNodeData();
          //}
          //else {
          //  // node not found - fast enum was done by other means (FRC) => we need to get data explicitely
          //  nd = getNodeDataPriv(nadr, exclusiveAccess);
          //}

          // enumerate node - fast enum was done by other means (FRC) => we need to get data explicitely


          nd = getNodeDataPriv(nadr, exclusiveAccess);

          unsigned mid = nd->getMid();
          bool dis = (m_fastEnum->getDiscovered().find(nadr) != m_fastEnum->getDiscovered().end());
          int hwpid = nd->getHwpid();
          int hwpidVer = nd->getHwpidVer();
          int osBuild = nd->getOsBuild();
          int dpaVer = nd->getDpaVer();

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
            deviceIdPtr = enumerateDeviceOutsideRepo(nadr, device, nd->getEmbedPer(), nd->getUserPer()
              , exclusiveAccess);
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
          nodeInDb(mid, deviceId, nd->getModeStd(), nd->getStdAndLpSupport());
          // insert bonded
          bondedInDb(nadr, dis ? 1 : 0, mid, 1);

          db << "commit;";
        }
        catch (sqlite_exception &e)
        {
          CATCH_EXC_TRC_WAR(sqlite_exception, e, "Unexpected error to store enumeration" << PAR(nadr) << NAME_PAR(code, e.get_code()) << NAME_PAR(ecode, e.get_extended_code()) << NAME_PAR(SQL, e.get_sql()));
          db << "rollback;";
        }
        catch (std::exception &e)
        {
          CATCH_EXC_TRC_WAR(std::exception, e, "Cannot full enumerate " << PAR(nadr));
          db << "rollback;";
        }
      }

      TRC_FUNCTION_LEAVE("");
    }

    void insertNodes(const std::map<int, embed::node::BriefInfoPtr> & nodes)
    {
      TRC_FUNCTION_ENTER("");

      if (nodes.size() > 0) {
        std::cout << std::endl << "AutoNw Enumeration started at: " << encodeTimestamp(std::chrono::system_clock::now());
        //fullEnum();

        TRC_INFORMATION("Wait for enumeration ...")
          std::lock_guard<std::mutex> lck(m_enumMtx);
        TRC_INFORMATION("Wait for enumeration finished");

        database & db = *m_db;

        for (const auto & it : nodes) {

          const embed::node::BriefInfoPtr & ndPtr = it.second;
          int nadr = it.first;

          try {

            std::unique_ptr<int> deviceIdPtr;
            int deviceId = 0;
            Device device(ndPtr->getHwpid(), ndPtr->getHwpidVer(), ndPtr->getOsBuild(), ndPtr->getDpaVer());

            // get package from JsCache if exists
            const iqrf::IJsCacheService::Package *pckg = nullptr;
            if (device.m_hwpid != 0) { // no custom handler => use default pckg0 to resolve periferies
              pckg = m_iJsCacheService->getPackage((uint16_t)device.m_hwpid, (uint16_t)device.m_hwpidVer, (uint16_t)device.m_osBuild, (uint16_t)device.m_dpaVer);
            }

            // TODO only for enumerateDeviceOutsideRepo if we know here nd->getEmbedExploreEnumerate()->getModeStd(), nd->getEmbedExploreEnumerate()->getStdAndLpSupport()
            //std::unique_ptr<embed::explore::RawDpaEnumerate> exploreEnumeratePtr(shape_new embed::explore::RawDpaEnumerate(nadr));
            //std::unique_ptr<IDpaTransactionResult2> transResult;
            //exclusiveAccess->executeDpaTransactionRepeat(exploreEnumeratePtr->getRequest(), transResult, 3);
            //exploreEnumeratePtr->processDpaTransactionResult(std::move(transResult));

            //bool modeStd = exploreEnumeratePtr->getModeStd();
            //bool stdAndLpSupport = exploreEnumeratePtr->getStdAndLpSupport();

            // TODO do we need them in DB at all?
            bool modeStd = true;
            bool stdAndLpSupport = false;

            if (pckg) {
              deviceIdPtr = enumerateDeviceInRepo(device, *pckg);
            }
            else {
              //std::unique_ptr<embed::explore::RawDpaEnumerate> exploreEnumeratePtr(shape_new embed::explore::RawDpaEnumerate(nadr));
              //std::unique_ptr<IDpaTransactionResult2> transResult;
              //exclusiveAccess->executeDpaTransactionRepeat(exploreEnumeratePtr->getRequest(), transResult, 3);
              //exploreEnumeratePtr->processDpaTransactionResult(std::move(transResult));
              //deviceIdPtr = enumerateDeviceOutsideRepo(nadr, device, exploreEnumeratePtr->getEmbedPer(), exploreEnumeratePtr->getUserPer(), exclusiveAccess);
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
            nodeInDb(ndPtr->getMid(), deviceId, modeStd, stdAndLpSupport);
            // insert bonded
            bondedInDb(nadr, ndPtr->getDisc() ? 1 : 0, ndPtr->getMid(), 1);

            db << "commit;";
          }
          catch (sqlite_exception &e)
          {
            CATCH_EXC_TRC_WAR(sqlite_exception, e, "Unexpected error to store enumeration" << PAR(nadr) << NAME_PAR(code, e.get_code()) << NAME_PAR(ecode, e.get_extended_code()) << NAME_PAR(SQL, e.get_sql()));
            db << "rollback;";
          }
          catch (std::exception &e)
          {
            CATCH_EXC_TRC_WAR(std::exception, e, "Cannot full enumerate " << PAR(nadr));
            db << "rollback;";
          }
        }

        loadDrivers();
        std::cout << std::endl << "AutoNw Std Enumeration started at:  " << encodeTimestamp(std::chrono::system_clock::now());
        //stdEnum(exclusiveAccess);
        std::cout << std::endl << "AutoNw Enumeration finished at:     " << encodeTimestamp(std::chrono::system_clock::now()) << std::endl;
      }
      else {
        std::cout << std::endl << "AutoNw no nodes to be added at:  " << encodeTimestamp(std::chrono::system_clock::now());
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
      int hwpid = 0;
      int hwpidVar = 0;
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
          << NAME_PAR(dpa,embed::explore::Enumerate::getDpaVerAsHexaString(dpaVer));
        
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

      for (auto & drv : drivers) {
        int driverId = drv.first;
        int driverVer = 0;
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
      m_iJsRenderService->loadJsCodeFenced(IJsRenderService::HWPID_DEFAULT_MAPPING, str2load); // provisional context for all with empty custom drivers

      // get all non empty custom drivers because of breakdown
      // hwpid, hwpidVer, driver
      std::map<int, std::map<int, std::string>> customDrivers =
        m_iJsCacheService->getCustomDrivers(embed::os::Read::getOsBuildAsString(osBuild), embed::explore::Enumerate::getDpaVerAsHexaString(dpaVer));

      for (auto d : customDrivers) {
        std::string js = str2load;
        std::string driver = d.second.rbegin()->second; // get the highest hwpidVer one from reverse end
        js += driver;
        m_iJsRenderService->loadJsCodeFenced(IJsRenderService::HWPID_MAPPING_SPACE - d.first, js);
      }

      TRC_FUNCTION_LEAVE("");
    }


    void loadDrivers()
    {
      TRC_FUNCTION_ENTER("");

      try {

        database & db = *m_db;
        std::map<int, std::map<int, Driver>> mapDeviceDrivers;

        // get drivers according DeviceId
        db << "SELECT "
          "Device.Id "
          ", Driver.Name "
          ", Driver.StandardId "
          ", Driver.Version "
          ", Driver.Driver "
          " FROM Driver "
          " INNER JOIN DeviceDriver "
          " ON Driver.Id = DeviceDriver.DriverId "
          " INNER JOIN Device "
          " ON DeviceDriver.DeviceId = Device.Id "
          ";"
          >> [&](int id, std::string name, int sid, int ver, std::string drv)
        {
          mapDeviceDrivers[id].insert(std::make_pair(sid, Driver(name, sid, ver, drv)));
        };

        db << "select Id, CustomDriver from Device;"
          >> [&](int id, std::string drv)
        {
          int sid = -100;
          mapDeviceDrivers[id].insert(std::make_pair(sid, Driver("custom", -100, 0, drv)));
        };

        /////////// special coord handling - gets all highest ver drivers not assigned in previous steps
        // get [C] device by Nadr = 0 to add later some std FRC drivers to coordinator
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

        std::map<int, Driver> & coordDriversMap = mapDeviceDrivers[coordDeviceId];
        db << "SELECT "
          "Name "
          ", StandardId "
          ", Version "
          ", Driver "
          ", MAX(Version) as MaxVersion "
          "FROM Driver "
          "GROUP BY StandardId "
          ";"
          >> [&](std::string name, int sid, int ver, std::string drv, int maxVer)
        {
          // it doesn't insert if sid already there => inserted just not presented
          coordDriversMap.insert(std::make_pair(sid, Driver(name, sid, ver, drv)));
        };
        ////////// end of special coord handling

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

        // load drivers to device dedicated context
        for (auto devIt : mapDeviceDrivers) {
          int deviceId = devIt.first;
          std::string str2load;
          auto const & drvs = devIt.second;
          for (auto drv : drvs) {
            str2load += drv.second.m_drv;
          }
          str2load += wrapperStr;
          m_iJsRenderService->loadJsCodeFenced(deviceId, str2load);

          // map nadrs to device dedicated context
          std::vector<int> nadrs;
          db << "SELECT "
            "Device.Id "
            ", Bonded.Nadr "
            " FROM Bonded "
            " INNER JOIN Node "
            " ON Bonded.Mid = Node.Mid "
            " INNER JOIN Device "
            " ON Node.DeviceId = Device.Id "
            " WHERE Node.DeviceId = ? "
            ";"
            << deviceId
            >> [&](int id, int nadr)
          {
            nadrs.push_back(nadr);
          };

          for (auto nadr : nadrs) {
            m_iJsRenderService->mapNadrToFenced(nadr, deviceId);
          }

        }

        //now we have all mappings => get rid of provisional contexts
        m_iJsRenderService->unloadProvisionalContexts();

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
    void nodeInDb(unsigned mid, int deviceId, int modeStd, int stdAndLpSupport)
    {
      TRC_FUNCTION_ENTER(PAR(mid) << PAR(deviceId) << PAR(modeStd) PAR(stdAndLpSupport))

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
        db << "insert into Node ("
          "Mid"
          ", DeviceId "
          ", ModeStd "
          ", StdAndLpSupport "
          ")  values ( "
          "?"
          ", ?"
          ", ?"
          ", ?"
          ");"
          << mid
          << deviceId
          << modeStd
          << stdAndLpSupport
          ;
      }
      else {
        db << "update Node set "
          "ModeStd = ?"
          ", StdAndLpSupport = ?"
          " where Mid = ?;"
          << modeStd
          << stdAndLpSupport
          << mid
          ;
      }

      TRC_FUNCTION_LEAVE("")
    }

    void stdEnum(IIqrfDpaService::ExclusiveAccessPtr & exclusiveAccess)
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
        " WHERE Device.StdEnum = 0"
        ";"
        >> [&](int dev, int nadr)
      {
        mapDeviceVectNadr[dev].push_back(nadr);
      };

      // std enum according first bonded nadr of the device
      for (auto it : mapDeviceVectNadr) {

        int deviceId = it.first;
        int nadr = -1;
        if (it.second.size() > 0) {
          // get first
          nadr = it.second[0];
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
                stdBinoutEnum(nadr, deviceId, exclusiveAccess);
                break;
              case PERIF_STANDARD_LIGHT:
                stdLightEnum(nadr, deviceId, exclusiveAccess);
                break;
              case PERIF_STANDARD_SENSOR:
                stdSensorEnum(nadr, deviceId, exclusiveAccess);
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

    void stdLightEnum(int nadr, int deviceId, IIqrfDpaService::ExclusiveAccessPtr & exclusiveAccess)
    {
      TRC_FUNCTION_ENTER(PAR(nadr) << PAR(deviceId));

      light::jsdriver::Enumerate lightEnum(m_iJsRenderService, nadr);
      lightEnum.processDpaTransactionResult(exclusiveAccess->executeDpaTransaction(lightEnum.getRequest())->get());

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

    void stdBinoutEnum(int nadr, int deviceId, IIqrfDpaService::ExclusiveAccessPtr & exclusiveAccess)
    {
      TRC_FUNCTION_ENTER(PAR(nadr) << PAR(deviceId))

        binaryoutput::jsdriver::Enumerate binoutEnum(m_iJsRenderService, nadr);
      binoutEnum.processDpaTransactionResult(exclusiveAccess->executeDpaTransaction(binoutEnum.getRequest())->get());

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

    void stdSensorEnum(int nadr, int deviceId, IIqrfDpaService::ExclusiveAccessPtr & exclusiveAccess)
    {
      TRC_FUNCTION_ENTER(PAR(nadr) << PAR(deviceId))

        sensor::jsdriver::Enumerate sensorEnum(m_iJsRenderService, nadr);
      sensorEnum.processDpaTransactionResult(exclusiveAccess->executeDpaTransaction(sensorEnum.getRequest())->get());

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
        >> [&](int nadr, int dis, unsigned mid, int hwpid, int hwpidVer, int osBuild, int dpaVer)
      {
        retval.insert(std::make_pair(nadr, embed::node::BriefInfoPtr(
          shape_new embed::node::info::BriefInfo(mid, (dis == 0 ? false : true), hwpid, hwpidVer, osBuild, dpaVer)
        )));
      };

      TRC_FUNCTION_LEAVE("");
      return retval;
    }

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
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Enumeration is already running");
      }
      TRC_FUNCTION_LEAVE("")
    }

    rapidjson::Document getNodeMetaData(int nadr) const
    {
      TRC_FUNCTION_ENTER("");

      std::string metaData;
      database & db = *m_db;

      //TODO
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

      initDb();

      loadProvisoryDrivers();

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

      {
        const Value* val = Pointer("/enumAtStartUp").Get(doc);
        if (val && val->IsBool()) {
          m_enumAtStartUp = (uint8_t)val->GetBool();
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
      if (m_enumThread.joinable()) {
        m_enumThread.join();
      }

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

  void IqrfInfo::insertNodes(const std::map<int, embed::node::BriefInfoPtr> & nodes)
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
