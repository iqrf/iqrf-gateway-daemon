#include "IqrfInfo.h"

#include <sqlite_modern_cpp.h>

#include "Trace.h"
#include "rapidjson/pointer.h"
#include <fstream>

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

    void daemonEnumerate()
    {
      TRC_FUNCTION_ENTER("");
      
      IEnumerateService::CoordinatorData coordinatorData = m_iEnumerateService->getCoordinatorData();

      //m_db 

      for (auto nadr : coordinatorData.m_discovered) {
        IEnumerateService::NodeData nodeData = m_iEnumerateService->getNodeData(nadr);
      }

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
