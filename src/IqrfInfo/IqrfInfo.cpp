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
        int osVer,
        int dpaVer
      )
        : m_nadr(nadr)
        , m_mid(mid)
        , m_discovered(discovered)
        , m_hwpid(hwpid)
        , m_hwpidVer(hwpidVer)
        , m_osBuild(osBuild)
        , m_osVer(osVer)
        , m_dpaVer(dpaVer)
      {}

      int m_nadr;
      unsigned m_mid;
      int m_discovered;
      int m_hwpid;
      int m_hwpidVer;
      int m_osBuild;
      int m_osVer;
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

  private:

    IJsRenderService* m_iJsRenderService = nullptr;
    IJsCacheService* m_iJsCacheService = nullptr;
    IEnumerateService* m_iEnumerateService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    shape::ILaunchService* m_iLaunchService = nullptr;

    std::unique_ptr<database> m_db;

    IEnumerateService::IFastEnumerationPtr m_fastEnum;

    // get m_bonded map according nadr from DB
    std::map<int, BondNodeDb> m_mapNadrBondNodeDb;
    // need full enum
    std::set<int> m_nadrFullEnum;

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

        if (!dbExists) {

          std::string sqlpath = dataDir;
          sqlpath += "/DB/";
          //create tables
          SqlFile::makeSqlFile(db, sqlpath + "init/IqrfInfo.db.sql");

          //insert data
          //SqlFile::makeSqlFile(db, sqlpath + "init/insert_Nodes.sql");
        }

        syncDatabaseWithCoordinator();
        fullEnum();
        loadDrivers();
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

    void syncDatabaseWithCoordinator()
    {
      TRC_FUNCTION_ENTER("");

      m_fastEnum = m_iEnumerateService->getFastEnumeration();

      database & db = *m_db;

      int m_nadr;
      unsigned m_mid;
      int discovery;
      int m_hwpid;
      int m_hwpidVer;
      int m_osBuild;
      int m_osVer;
      int m_dpaVer;

      db << "select "
        "b.Nadr "
        ", b.Mid "
        ", b.Dis "
        ", d.Hwpid "
        ", d.HwpidVer "
        ", d.OsBuild "
        ", d.OsVer "
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
          int osVer,
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
          osVer,
          dpaVer
        )));
      };

      auto const & enums = m_fastEnum->getEnumerated();

      // sync non discovered Nadrs
      for (auto nadr : m_fastEnum->getNonDiscovered()) {

        std::unique_ptr<int> nadrPtrRes, disPtrRes, midPtrRes;
        int dis = 0, mid = 0;

        db << "select Nadr, Dis, Mid from Bonded where nadr = ? ;" << nadr
          >> [&](std::unique_ptr<int> nadrPtr, std::unique_ptr<int> disPtr, std::unique_ptr<int> midPtr)
        {
          nadrPtrRes = std::move(nadrPtr);
          disPtrRes = std::move(disPtr);
          midPtrRes = std::move(midPtr);
        };

        if (nadrPtrRes) {
          if (*disPtrRes != 0 || !midPtrRes) {
            // Nadr exists in DB and is set as discovered or has mid => set to nondiscovered and null mid in Bonded
            TRC_INFORMATION(PAR(nadr) << " set to nondiscovered in bonded list");
            db << "update Bonded set Dis = ? , Mid = ?, Enm = ? where Nadr = ?;" << 0 << nullptr << 0 << nadr;
          }
        }
        else {
          // Nadr does not exist in DB => insert and set to nondiscovered and null mid in Bonded
          TRC_INFORMATION(PAR(nadr) << " insert and set to nondiscovered in bonded list");
          db << "insert into Bonded (Nadr, Dis, Enm)  values (?, ?, ?);" << nadr << 0 << 0;
        }
      }

      // delete Nadr from DB if it doesnt exist in Net
      for (const auto & bo : m_mapNadrBondNodeDb) {
        int nadr = bo.first;
        const auto & b = bo.second;
        auto found = enums.find(nadr);
        if (found == enums.end()) {
          // Nadr not found in Net => delete from Bonded
          TRC_INFORMATION(PAR(nadr) << " remove from bonded list")
            db << "delete from Bonded where Nadr = ?;" << nadr;
        }
      }

      // compare fast enum and DB
      for (const auto & en : enums) {
        const auto & e = *(en.second);
        int nadr = en.first;
        auto found = m_mapNadrBondNodeDb.find(nadr);
        if (found == m_mapNadrBondNodeDb.end()) {
          // Nadr from Net not found in DB => provide full enum
          m_nadrFullEnum.insert(nadr);
        }
        else {
          auto const & n = found->second;
          if (e.getMid() != n.m_mid || e.getHwpid() != n.m_hwpid || e.getHwpidVer() != n.m_hwpidVer ||
            e.getOsBuild() != n.m_osBuild || e.getOsVer() != n.m_osVer || e.getDpaVer() != n.m_dpaVer) {
            // Nadr from Net is already in DB, but fast enum comparison failed => provide full enum
            TRC_INFORMATION(PAR(nadr) << " fast enum does not fit => schedule full enum")
              m_nadrFullEnum.insert(nadr);
          }
        }
      }

      TRC_FUNCTION_LEAVE("");
    }

    void fullEnum()
    {
      TRC_FUNCTION_ENTER("");

      database & db = *m_db;

      for (auto nadr : m_nadrFullEnum) {

        try {

          IEnumerateService::INodeDataPtr nd = m_iEnumerateService->getNodeData(nadr);

          unsigned mid = nd->getEmbedOsRead()->getMid();
          int hwpid = nd->getHwpid();
          int hwpidVer = nd->getEmbedExploreEnumerate()->getHwpidVer();
          int osBuild = nd->getEmbedOsRead()->getOsBuild();
          int osVer = nd->getEmbedOsRead()->getOsVersion();
          int dpaVer = nd->getEmbedExploreEnumerate()->getDpaVer();

          //int deviceId = deviceInDb(hwpid, hwpidVer, osBuild, osVer, dpaVer);
          //nodeInDb(mid, deviceId, nd->getEmbedExploreEnumerate()->getModeStd(), nd->getEmbedExploreEnumerate()->getStdAndLpSupport());

          // get package from JsCache
          const iqrf::IJsCacheService::Package *pckg = m_iJsCacheService->getPackage((uint16_t)hwpid, (uint16_t)hwpidVer, (uint16_t)osBuild, (uint16_t)dpaVer);
          std::vector<const IJsCacheService::StdDriver *> drivers;

          if (!pckg) {
            // no package in IqrfRepo => get drivers by enumeration
            std::map<int, int> perVerMap;
            const std::set<int> & embedPer = nd->getEmbedExploreEnumerate()->getEmbedPer();
            const std::set<int> & userPer = nd->getEmbedExploreEnumerate()->getUserPer();

            // Get for hwpid 0 plain DPA plugin
            const iqrf::IJsCacheService::Package *pckg0 = m_iJsCacheService->getPackage((uint16_t)0, (uint16_t)0, (uint16_t)osBuild, (uint16_t)dpaVer);
            
            //TODO we need to load everything because of DeamonWrapper.js workaround now
            //for (auto drv : pckg0->m_stdDriverVect) {
            //  perVerMap.insert(std::make_pair(drv->getId(), drv->getVersion()));
            //}

            for (auto per : embedPer) {
              for (auto drv : pckg0->m_stdDriverVect) {
                if (drv->getId() == -1) {
                  perVerMap.insert(std::make_pair(-1, drv->getVersion())); // driver library
                }
                if (drv->getId() == per) {
                  perVerMap.insert(std::make_pair(per, drv->getVersion()));
                }
              }
            }
            for (auto per : userPer) {
              //Get peripheral information for sensor, binout and TODO other std if presented
              if (PERIF_STANDARD_BINOUT == per || PERIF_STANDARD_SENSOR == per) {
                //TODO temp workaround
                embed::explore::PeripheralInformationPtr peripheralInformationPtr = m_iEnumerateService->getPeripheralInformationData(nadr, per);
                int version = peripheralInformationPtr->getPar1();
                //TODO temp workaround
                if (PERIF_STANDARD_SENSOR == per) version = 15;
                perVerMap.insert(std::make_pair(per, version));
              }
              else {
                perVerMap.insert(std::make_pair(per, -1));
              }
            }

            for (auto pv : perVerMap) {
              const IJsCacheService::StdDriver *sd =  m_iJsCacheService->getDriver(pv.first, pv.second);
              if (sd) {
                drivers.push_back(sd);
              }
            }
          }
          else {
            drivers = pckg->m_stdDriverVect;
          }

          db << "begin transaction;";

          int deviceId = deviceInDb(hwpid, hwpidVer, osBuild, osVer, dpaVer);

          // store drivers in DB if doesn't exists already
          for (auto d : drivers) {
            int driverId = driverInDb(d);
            int count = 0;
            db << "select count(*) from DeviceDriver where DeviceId = ? and DriverId = ? ;" << deviceId << driverId >> count;
            if (count == 0) {
              db << "insert into DeviceDriver (DeviceId, DriverId) values (?, ?);" << deviceId << driverId;
            }
          }
          nodeInDb(mid, deviceId, nd->getEmbedExploreEnumerate()->getModeStd(), nd->getEmbedExploreEnumerate()->getStdAndLpSupport());

          bondedInDb(nadr, 1, mid, 1);

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
          db << "rollback;";
        }
      }

      TRC_FUNCTION_LEAVE("");
    }

    void loadDrivers()
    {
      TRC_FUNCTION_ENTER("");
      database & db = *m_db;

      std::map<int, std::vector<Driver>> mapDeviceDrivers;

      try {

        database & db = *m_db;

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
          mapDeviceDrivers[id].push_back(Driver(name, sid, ver, drv));
        };

        db << "select Id, CustomDriver from Device;" 
          >> [&](int id, std::string drv)
        {
          mapDeviceDrivers[id].push_back(Driver("custom", -100, 0, drv));
        };

        
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
            str2load += drv.m_drv;
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
            " WHERE Node.DeviceId = ?"
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
      }
      catch (sqlite_exception &e)
      {
        CATCH_EXC_TRC_WAR(sqlite_exception, e, "Unexpected error " << NAME_PAR(code, e.get_code()) << NAME_PAR(ecode, e.get_extended_code()) << NAME_PAR(SQL, e.get_sql()));
      }
      catch (std::exception &e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, "Cannot load drivers ");
      }

      TRC_FUNCTION_LEAVE("");
    }

    void bondedInDb(int nadr, int dis, unsigned mid, int enm)
    {
      database & db = *m_db;

      int count = 0;
      db << "select count(*) from Bonded where Nadr = ?" << nadr >> count;

      if (count == 0) {
        TRC_INFORMATION(PAR(nadr) << " insert and set to discovered in bonded list");
        db << "insert into Bonded (Nadr, Dis, Mid, Enm)  values (?, ?, ?, ?);" << nadr << dis << mid << enm;
      }
      else {
        db << "update Bonded  set Dis = ? , Mid = ?, Enm = ? where Nadr = ?; " << dis << mid << enm << nadr;
      }
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
      TRC_FUNCTION_ENTER("");

      std::string name = drv->getName();
      int standardId = drv->getId();
      int version = drv->getVersion();

      database & db = *m_db;

      std::unique_ptr<int> id = selectDriver(drv);

      if (!id) {
        TRC_INFORMATION(PAR(name) << PAR(standardId) << PAR(version) << " insert driver to DB");

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
        THROW_EXC_TRC_WAR(std::logic_error, PAR(name) << PAR(standardId) << PAR(version) << " shall be selected from DB")
      }

      TRC_FUNCTION_ENTER("");
      return *id;
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

    // check id device exist and if not insert and return id
    int deviceInDb(int hwpid, int hwpidVer, int osBuild, int osVer, int dpaVer)
    {
      TRC_FUNCTION_ENTER(PAR(hwpid) << PAR(hwpidVer) << PAR(osBuild) << PAR(osVer) << PAR(dpaVer));

      std::unique_ptr<int> id;
      database & db = *m_db;
      db << "select "
        "d.Id "
        "from "
        "Device as d "
        "where "
        "d.Hwpid = ? and "
        "d.HwpidVer = ? and "
        "d.OsBuild = ? and "
        "d.DpaVer = ? "
        ";"
        << hwpid
        << hwpidVer
        << osBuild
        << dpaVer
        >> [&](std::unique_ptr<int> d)
      {
        id = std::move(d);
      };

      if (!id) {
        // device doesn't exist in DB
        db << "insert into Device ("
          "Hwpid"
          ", HwpidVer"
          ", OsBuild"
          ", OsVer"
          ", DpaVer"
          ")  values ( "
          "?"
          ", ?"
          ", ?"
          ", ?"
          ", ?"
          ");"
          << hwpid
          << hwpidVer
          << osBuild
          << osVer
          << dpaVer
          ;
      }

      db << "select "
        "d.Id "
        "from "
        "Device as d "
        "where "
        "d.Hwpid = ? and "
        "d.HwpidVer = ? and "
        "d.OsBuild = ? and "
        "d.DpaVer = ? "
        ";"
        << hwpid
        << hwpidVer
        << osBuild
        << dpaVer
        >> [&](std::unique_ptr<int> d)
      {
        id = std::move(d);
      };

      if (!id) {
        THROW_EXC_TRC_WAR(std::logic_error, "insert failed: " << PAR(hwpid) << PAR(hwpidVer) << PAR(osBuild) << PAR(osVer) << PAR(dpaVer))
      }

      TRC_FUNCTION_LEAVE("");

      return *id;
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
