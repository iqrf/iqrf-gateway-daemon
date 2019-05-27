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

    void syncDatabaseWithCoordinator()
    {
      TRC_FUNCTION_ENTER("");

      IEnumerateService::CoordinatorData coordinatorData = m_iEnumerateService->getCoordinatorData();

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
      for (auto nadr : coordinatorData.m_bonded) {
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
        std::set<int> & coordBonded = coordinatorData.m_bonded;

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
        std::set<int> & coordDisc = coordinatorData.m_discovered;

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

      // fast enumerate
      std::map<unsigned, Bond*> bondedMid;
      {
        for (auto & bondIt : bonded) {
          int nadr = bondIt.first;
          Bond & b = bondIt.second;

          try {
            IEnumerateService::NodeData nd = m_iEnumerateService->getNodeData(nadr); //TODO fast
            b.m_fullEnum = false;
            if (!nd.isValid() || nd.getMid() != b.m_mid || nd.getHwpid() != b.m_hwpid || nd.getHwpidVer() != b.m_hwpidVer) {
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

          if (b.m_fullEnum) {
            try {
              IEnumerateService::NodeData nd = m_iEnumerateService->getNodeData(nadr); //TODO full

              try {
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
                    << nd.getMid()
                    << nd.getHwpid()
                    << nd.getHwpidVer()
                    << nd.getOsBuild()
                    << nd.getOsVer()
                    << nd.getDpaVer()
                    << nd.getModeStd()
                    << nd.getStdAndLpNet()
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
                    << nd.getHwpid()
                    << nd.getHwpidVer()
                    << nd.getOsBuild()
                    << nd.getOsVer()
                    << nd.getDpaVer()
                    << nd.getModeStd()
                    << nd.getStdAndLpNet()
                    << nd.getMid()
                    ;
                }

                db << "update Bonded set "
                  "Mid = ?"
                  ", Enm = ?"
                  " where "
                  " Nadr = ?"
                  ";"
                  << nd.getMid()
                  << 1
                  << nadr
                  ;

                db << "commit;";
              }
              catch (sqlite_exception &e)
              {
                CATCH_EXC_TRC_WAR(sqlite_exception, e, "Unexpected error " << NAME_PAR(code, e.get_code()) << NAME_PAR(ecode, e.get_extended_code()) << NAME_PAR(SQL, e.get_sql()));
                db << "rollback;";
              }
            }
            catch (std::exception &e)
            {
              CATCH_EXC_TRC_WAR(std::exception, e, "Cannot full enumerate " << PAR(nadr));
            }
          }
        }
      }

      TRC_FUNCTION_LEAVE("");
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
