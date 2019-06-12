#include "IqrfInfo.h"

#include <sqlite_modern_cpp.h>

#include "Trace.h"
#include "rapidjson/pointer.h"
#include <fstream>
#include <set>

#include "iqrf__IqrfInfo.hxx"

TRC_INIT_MODULE(iqrf::IqrfInfo);

using namespace  sqlite;

namespace iqrf {

  const int PERIF_STANDARD_SENSOR = 94;
  const int PERIF_STANDARD_BINOUT = 75;

  namespace sensor {
    //FRC command to return 2 - bits sensor data of the supporting sensor types.
    const int STD_SENSOR_FRC_2BITS = 0x10;
    //FRC command to return 1-byte wide sensor data of the supporting sensor types.
    const int STD_SENSOR_FRC_1BYTE = 0x90;
    //FRC command to return 2-bytes wide sensor data of the supporting sensor types.
    const int STD_SENSOR_FRC_2BYTES = 0xE0;
    //FRC command to return 4-bytes wide sensor data of the supporting sensor types.
    const int STD_SENSOR_FRC_4BYTES = 0xF9;
  }

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
  private:

    IEnumerateService* m_iEnumerateService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    shape::ILaunchService* m_iLaunchService = nullptr;

    std::unique_ptr<database> m_db;


  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    void attachInterface(iqrf::IEnumerateService* iface)
    {
      TRC_FUNCTION_ENTER(PAR(iface));
      m_iEnumerateService = iface;
      TRC_FUNCTION_LEAVE("")
    }

    void detachInterface(iqrf::IEnumerateService* iface)
    {
      TRC_FUNCTION_ENTER(PAR(iface));
      if (m_iEnumerateService == iface) {
        m_iEnumerateService = nullptr;
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

      using namespace rapidjson;
      const Document& doc = props->getAsJson();

      //{
      //  const Value* val = rapidjson::Pointer("/gwIdentModeByte").Get(doc);
      //  if (val && val->IsInt()) {
      //    m_gwIdentModeByte = (uint8_t)val->GetInt();
      //  }
      //}

      initDb();

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

      TRC_FUNCTION_LEAVE("")
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

        if (!dbExists) {

          std::string sqlpath = dataDir;
          sqlpath += "/DB/";
          //create tables
          SqlFile::makeSqlFile(db, sqlpath + "init/IqrfInfo.db.sql");

          //insert data
          //SqlFile::makeSqlFile(db, sqlpath + "init/insert_Nodes.sql");
        }

        syncDatabaseWithCoordinator();
        //daemonEnumerate();
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

    class Bond
    {
    public:
      Bond(int nadr, unsigned mid, bool dis)
        : m_nadr(nadr)
        , m_mid(mid)
        , m_dis(dis)
        , m_hwpid(-1)
        , m_hwpidVer(-1)
        , m_fullEnum(true)
      {}

      int m_nadr;
      unsigned m_mid;
      bool m_dis;
      int m_hwpid;
      int m_hwpidVer;
      bool m_fullEnum;
    };

    // get key info without JS driver support to load proper coordinator driver
    //void initCoordinatorInfo()
    //{
    //  TRC_FUNCTION_ENTER("");
    //  IEnumerateService::CoordinatorData coordinatorData = m_iEnumerateService->getCoordinatorData();
    //  TRC_FUNCTION_LEAVE("");
    //}

    void syncDatabaseWithCoordinator()
    {
      TRC_FUNCTION_ENTER("");

      IEnumerateService::IFastEnumerationPtr fastEnum = m_iEnumerateService->getFastEnumeration();

      database & db = *m_db;

      // get bonded map according nadr from DB
      std::map<int, Bond> bonded;
      db << "select "
        "b.Nadr "
        ", b.Dis "
        ", b.Mid "
        "from "
        "Bonded as b"
        ";"
        >> [&](int nadr, int dis, unsigned mid)
      {
        bool bdisc = dis != 0;
        bonded.insert(std::make_pair(nadr, Bond(nadr, mid, bdisc)));
      };

      // check if coord bonded exist in db bonded
      for (auto nadr : fastEnum->getBonded()) {
        auto found = bonded.find(nadr);
        if (found == bonded.end())
        {
          // Nadr not found in db bonded => insert
          Bond b(nadr, 0, false);
          db << "insert into Bonded ("
            "Nadr"
            ", Dis"
            ", Enm"
            ")  values ( "
            "?"
            ", ?"
            ", ?"
            ");"
            << nadr
            << (b.m_dis ? 1 : 0)
            << 0
            ;
          bonded.insert(std::make_pair(nadr, b));
        }
      }

      // check if db bonded exist in coord bonded
      {
        std::map<int, Bond> ::iterator bondIt = bonded.begin();
        const std::set<int> & coordBonded = fastEnum->getBonded();

        while (bondIt != bonded.end()) {
          int nadr = bondIt->first;
          auto found = coordBonded.find(nadr);
          if (found == coordBonded.end())
          {
            // Nadr not found in coord bonded => delete from db bonded
            db << "delete from Bonded where nadr = ?;"
              << nadr
              ;
            bonded.erase(bondIt++);
          }
          else {
            ++bondIt;
          }
        }
      }

      // check if db bonded discovery fit with coord discovery
      {
        const std::set<int> & coordDisc = fastEnum->getDiscovered();

        for (auto & bondIt : bonded) {
          int nadr = bondIt.first;
          Bond & b = bondIt.second;
          auto found = coordDisc.find(nadr);
          bool disc = found != coordDisc.end();
          if (disc != b.m_dis) {
            // Discovery state differ => update in DB
            b.m_dis = disc;
            db << "update Bonded set "
              " Dis = ?"
              " where Nadr = ?;"
              << (b.m_dis ? 1 : 0)
              << nadr
              ;
          }
        }
      }

      // get bonded hwpid for assigned mid if exist
      {
        for (auto & bondIt : bonded) {
          int nadr = bondIt.first;
          Bond & b = bondIt.second;
          int hwpid = -1, hwpidVer = -1;
          db << "select "
            "n.Hwpid "
            ", n.HwpidVer "
            " from "
            " Node as n"
            ", Bonded as b"
            " where "
            " b.Nadr = ? and "
            " n.Mid = b.Mid"
            ";"
            << nadr
            >> [&](int lhwpid, int lhwpidVer)
          {
            b.m_hwpid = lhwpid;
            b.m_hwpidVer = lhwpidVer;
          };
        }
      }

      // check fast enumerate result
      std::map<unsigned, Bond*> bondedMid;
      {
        for (auto & bondIt : bonded) {
          int nadr = bondIt.first;
          Bond & b = bondIt.second;

          try {
            auto found = fastEnum->getEnumerated().find(nadr);
            if (found == fastEnum->getEnumerated().end()) {
              THROW_EXC_TRC_WAR(std::logic_error, "Cannot find bonded: " << PAR(nadr));
            }
            const IEnumerateService::IFastEnumeration::Enumerated & nd = *(found->second.get());
            b.m_fullEnum = false;
            if (nd.getMid() != b.m_mid || nd.getHwpid() != b.m_hwpid || nd.getHwpidVer() != b.m_hwpidVer) {
              b.m_fullEnum = true;
            }
            auto ins = bondedMid.insert(std::make_pair(nd.getMid(), &b));
            if (!ins.second) {
              // detected duplicit mid
              ins.first->second->m_fullEnum = true;
              b.m_fullEnum = true;
            }
          }
          catch (std::exception &e)
          {
            CATCH_EXC_TRC_WAR(std::exception, e, "Cannot fast enumerate " << PAR(nadr));
            b.m_fullEnum = true;
          }
        }
      }

      // full enumerate
      {
        for (auto & bondIt : bonded) {
          int nadr = bondIt.first;
          Bond & b = bondIt.second;
          if (!b.m_fullEnum)
            continue;

          try {
            IEnumerateService::INodeDataPtr nd = m_iEnumerateService->getNodeData(nadr); //TODO full

            db << "begin transaction;";
            if (!midExistsInDb(b.m_mid)) {
              // mid doesn't exist in DB
              db << "insert into Node ("
                "Mid"
                ", Hwpid"
                ", HwpidVer"
                ", OsBuild"
                ", OsVer"
                ", DpaVer"
                ", ModeStd"
                ", StdAndLpNet"
                ")  values ( "
                "?"
                ", ?"
                ", ?"
                ", ?"
                ", ?"
                ", ?"
                ", ?"
                ", ?"
                ");"
                << nd->getEmbedOsRead()->getMid()
                << nd->getHwpid()
                << nd->getEmbedExploreEnumerate()->getHwpidVer()
                << nd->getEmbedOsRead()->getOsBuild()
                << nd->getEmbedOsRead()->getOsVersion()
                << nd->getEmbedExploreEnumerate()->getDpaVer()
                << nd->getEmbedExploreEnumerate()->getModeStd()
                << nd->getEmbedExploreEnumerate()->getStdAndLpSupport()
                ;
            }
            else {
              db << "update Node set "
                "Hwpid = ?"
                ", HwpidVer = ?"
                ", OsBuild = ?"
                ", OsVer = ?"
                ", DpaVer = ?"
                ", ModeStd = ?"
                ", StdAndLpNet = ?"
                " where Mid = ?;"
                << nd->getHwpid()
                << nd->getEmbedExploreEnumerate()->getHwpidVer()
                << nd->getEmbedOsRead()->getOsBuild()
                << nd->getEmbedOsRead()->getOsVersion()
                << nd->getEmbedExploreEnumerate()->getDpaVer()
                << nd->getEmbedExploreEnumerate()->getModeStd()
                << nd->getEmbedExploreEnumerate()->getStdAndLpSupport()
                << nd->getEmbedOsRead()->getMid()
                ;
            }

            db << "update Bonded set "
              "Mid = ?"
              ", Enm = ?"
              " where "
              " Nadr = ?"
              ";"
              << nd->getEmbedOsRead()->getMid()
              << 1
              << nadr
              ;

            db << "delete from Perifery "
              " where "
              " Mid = ?"
              ";"
              << nd->getEmbedOsRead()->getMid()
              ;

            db << "delete from Sensor "
              " where "
              " Mid = ?"
              ";"
              << nd->getEmbedOsRead()->getMid()
              ;

            db << "delete from Binout "
              " where "
              " Mid = ?"
              ";"
              << nd->getEmbedOsRead()->getMid()
              ;

            for (auto per : nd->getEmbedExploreEnumerate()->getEmbedPer()) {
              insertPerifery(nd->getEmbedOsRead()->getMid(), per);
            }
            for (auto per : nd->getEmbedExploreEnumerate()->getUserPer()) {
              insertPerifery(nd->getEmbedOsRead()->getMid(), per);
              insertSensor(nd->getEmbedOsRead()->getMid(), nadr, per);
              insertBinout(nd->getEmbedOsRead()->getMid(), nadr, per);
            }

            db << "commit;";
          }
          catch (sqlite_exception &e)
          {
            CATCH_EXC_TRC_WAR(sqlite_exception, e, "Unexpected error " << NAME_PAR(code, e.get_code()) << NAME_PAR(ecode, e.get_extended_code()) << NAME_PAR(SQL, e.get_sql()));
            db << "rollback;";
          }
          catch (std::exception &e)
          {
            CATCH_EXC_TRC_WAR(std::exception, e, "Cannot full enumerate " << PAR(nadr));
          }
        }
      }

      TRC_FUNCTION_LEAVE("");
    }

    void insertPerifery(unsigned mid, int per)
    {
      database & db = *m_db;
      db << "insert into Perifery ("
        "Mid"
        ", Per"
        ")  values ( "
        "?"
        ", ?"
        ");"
        << mid
        << per
        ;
    }

    void insertSensor(unsigned mid, int nadr, int per)
    {
      if (PERIF_STANDARD_SENSOR != per)
        return;

      //TODO per1 holds Standard version
      //IEnumerateService::IPeripheralInformationDataPtr pi = m_iEnumerateService->getPeripheralInformationData(nadr, PERIF_STANDARD_SENSOR);

      IEnumerateService::IStandardSensorDataPtr sen = m_iEnumerateService->getStandardSensorData(nadr);
      auto const & sensors = sen->getSensors();

      database & db = *m_db;
      int idx = 0;

      for (auto const & sen : sensors) {
        const auto & f = sen->getFrcs();
        const auto & e = sen->getFrcs().end();

        int frc2bit = (int)(e != f.find(iqrf::sensor::STD_SENSOR_FRC_2BITS));
        int frc1byte = (int)(e != f.find(iqrf::sensor::STD_SENSOR_FRC_1BYTE));
        int frc2byte = (int)(e != f.find(iqrf::sensor::STD_SENSOR_FRC_2BYTES));
        int frc4byte = (int)(e != f.find(iqrf::sensor::STD_SENSOR_FRC_4BYTES));
        
        db << "insert into Sensor ("
          "Mid"
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
          << mid << idx++ << sen->getSid() << sen->getType() << sen->getName() << sen->getShortName() << sen->getUnit() << sen->getDecimalPlaces()
          << frc2bit << frc1byte << frc2byte << frc4byte;
          ;
      }
    }

    void insertBinout(unsigned mid, int nadr, int per)
    {
      if (PERIF_STANDARD_BINOUT != per)
        return;

      IEnumerateService::IStandardBinaryOutputDataPtr bout = m_iEnumerateService->getStandardBinaryOutputData(nadr);

      database & db = *m_db;

      db << "insert into Binout ("
        "Mid"
        ", Num"
        ")  values ( "
        "?, ?"
        ");"
        << mid << bout->getBinaryOutputsNum();
    }

    bool midExistsInDb(unsigned mid)
    {
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
      return count > 0;
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
    (void)props; //silence -Wunused-parameter
  }

  void IqrfInfo::attachInterface(iqrf::IEnumerateService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void IqrfInfo::detachInterface(iqrf::IEnumerateService* iface)
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
