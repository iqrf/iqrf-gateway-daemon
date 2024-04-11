/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "ShapeDefines.h"
#include <map>
#include <set>
#include <string>
#include <functional>
#include <vector>
#include <memory>

#ifdef IJsCacheService_EXPORTS
#define IJsCacheService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IJsCacheService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  class IJsCacheService_DECLSPEC IJsCacheService {
  public:
    ///// Company /////

    class Company {
    public:
      Company(
        unsigned int companyId,
        const std::string &name,
        const std::string &homePage
      ):
        m_companyId(companyId),
        m_name(name),
        m_homePage(homePage)
      {}

      unsigned int m_companyId;
      std::string m_name;
      std::string m_homePage;
    };

    ///// Manufacturer /////

    class Manufacturer {
    public:
      Manufacturer(
        unsigned int manufacturerId,
        unsigned int companyId,
        const std::string &name
      ):
        m_manufacturerId(manufacturerId),
        m_companyId(companyId),
        m_name(name)
      {}

      unsigned int m_manufacturerId;
      unsigned int m_companyId;
      std::string m_name;
    };

    ///// Product metadata /////

    class MetadataHwpidProfile {
    public:
      MetadataHwpidProfile(
        uint8_t hwpidVerMin = 0,
        int8_t hwpidVerMax = -1,
        bool routing = false,
        bool beaming = false,
        bool repeater = false,
        bool frcAggregation = false,
        bool iqarosCompatible = false,
        const std::vector<uint8_t> sensors = {},
        uint8_t binouts = 0
      ):
        m_hwpidVerMin(hwpidVerMin),
        m_hwpidVerMax(hwpidVerMax),
        m_routing(routing),
        m_beaming(beaming),
        m_repeater(repeater),
        m_frcAggregation(frcAggregation),
        m_iqarosCompatible(iqarosCompatible),
        m_sensors(sensors),
        m_binouts(binouts)
      {}

      uint8_t m_hwpidVerMin;
      int8_t m_hwpidVerMax;
      bool m_routing;
      bool m_beaming;
      bool m_repeater;
      bool m_frcAggregation;
      bool m_iqarosCompatible;
      std::vector<uint8_t> m_sensors;
      uint8_t m_binouts;
    };

    class Metadata {
    public:
      Metadata(
        uint8_t version = 0,
        std::vector<MetadataHwpidProfile> profiles = {}
      ):
        m_version(version),
        m_profiles(profiles)
      {}

      std::shared_ptr<MetadataHwpidProfile> getProfileByHwpidVersion(uint8_t hwpidVersion) {
        for (auto &profile : m_profiles) {
          if (hwpidVersion >= profile.m_hwpidVerMin && (profile.m_hwpidVerMax == -1 || profile.m_hwpidVerMax >= hwpidVersion)) {
            return std::make_shared<MetadataHwpidProfile>(profile);
          }
        }
        return nullptr;
      }

      uint8_t m_version;
      std::vector<MetadataHwpidProfile> m_profiles;
    };

    ///// Product /////

    class Product {
    public:
      Product(
        uint16_t hwpid,
        unsigned int manufacturerId,
        const std::string &name,
        const std::string &homePage,
        const std::string &picture,
        std::vector<Metadata> metadata
      ):
        m_hwpid(hwpid),
        m_manufacturerId(manufacturerId),
        m_name(name),
        m_homePage(homePage),
        m_picture(picture),
        m_metadata(metadata)
      {}

      uint16_t m_hwpid;
      unsigned int m_manufacturerId;
      std::string m_name;
      std::string m_homePage;
      std::string m_picture;
      std::vector<Metadata> m_metadata;
    };

    ///// Driver /////

    class StdDriver {
    public:
      StdDriver() {}

      StdDriver(
        int id,
        const std::string &name,
        double version,
        const std::shared_ptr<std::string> &driver,
        const std::shared_ptr<std::string> &notes,
        int verFlags
      ):
        m_id(id),
        m_version(version),
        m_versionFlags(verFlags),
        m_name(name),
        m_driver(driver),
        m_notes(notes)
      {}

      const std::string & getName() const { return m_name; }
      const std::shared_ptr<std::string> & getDriver() const { return m_driver; }
      const std::shared_ptr<std::string> & getNotes() const { return m_notes; }
      int getVersionFlags() const { return m_versionFlags; }
      double getVersion() const { return m_version; }
      int getId() const { return m_id; }
    private:
      int m_id = 0;
      double m_version = 0;
      int m_versionFlags = 0;
      std::string m_name;
      std::shared_ptr<std::string> m_driver;
      std::shared_ptr<std::string> m_notes;
    };

    ///// Standard /////

    class StdItem {
    public:
      StdItem() = delete;
      StdItem(const std::string &name) : m_name(name) {}
      StdItem(const std::string &name, const std::map<double, StdDriver> &drvs) : m_name(name), m_drivers(drvs) {}

      bool m_valid = false;
      std::string m_name;
      std::map<double, StdDriver> m_drivers;
    };

    ///// Package /////

    class Package {
    public:
      Package(
        unsigned int packageId,
        uint16_t hwpid,
        uint16_t hwpidVer,
        const std::string &handlerUrl,
        const std::string &handlerHash,
        const std::string &os,
        const std::string &dpa,
        const std::string &notes,
        const std::string &driver,
        const std::vector<StdDriver> &driverVect
      ):
        m_packageId(packageId),
        m_hwpid(hwpid),
        m_hwpidVer(hwpidVer),
        m_handlerUrl(handlerUrl),
        m_handlerHash(handlerHash),
        m_os(os),
        m_dpa(dpa),
        m_notes(notes),
        m_driver(driver),
        m_stdDriverVect(driverVect)
      {}

      unsigned int m_packageId;
      uint16_t m_hwpid;
      uint16_t m_hwpidVer;
      std::string m_handlerUrl;
      std::string m_handlerHash;
      std::string m_os;
      std::string m_dpa;
      std::string m_notes;
      std::string m_driver;
      std::vector<StdDriver> m_stdDriverVect;
    };

    ///// OsDpa /////

    class OsDpa {
    public:
      OsDpa(
        unsigned int osdpaId,
        const std::string &os,
        const std::string &dpa,
        const std::string &notes
      ):
        m_osdpaId(osdpaId),
        m_os(os),
        m_dpa(dpa),
        m_notes(notes)
      {}

      unsigned int m_osdpaId;
      std::string m_os;
      std::string m_dpa;
      std::string m_notes;
    };

    // get os map of dpa list as integers
    typedef std::map<int, std::set<int>> MapOsListDpa;

    ///// Quantity /////

    class Quantity {
    public:
      Quantity(
        const uint8_t &type,
        const std::string &id,
        const std::string &name,
        const std::string &shortName,
        const std::string &unit,
        const uint8_t &precision,
        const std::vector<uint8_t> &frcs,
        const uint8_t &width,
        const std::string &driverKey
      ):
        m_type(type),
        m_id(id),
        m_name(name),
        m_shortName(shortName),
        m_unit(unit),
        m_precision(precision),
        m_frcs(frcs),
        m_width(width),
        m_driverKey(driverKey)
      {}

      uint8_t m_type;
      std::string m_id;
      std::string m_name;
      std::string m_shortName;
      std::string m_unit;
      uint8_t m_precision;
      std::vector<uint8_t> m_frcs;
      uint8_t m_width;
      std::string m_driverKey;
    };

    ///// Server state /////

    class ServerState {
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

    /**
     * JS cache status enum
     */
    enum CacheStatus
    {
      PENDING,
      UP_TO_DATE,
      UPDATE_NEEDED,
      UPDATED,
      UPDATE_FAILED,
    };

    virtual std::shared_ptr<StdDriver> getDriver(int id, double ver) const = 0;
    virtual std::shared_ptr<Manufacturer> getManufacturer(uint16_t hwpid) const = 0;
    virtual std::shared_ptr<Product> getProduct(uint16_t hwpid) const = 0;
    virtual std::shared_ptr<Package> getPackage(uint16_t hwpid, uint16_t hwpidVer, const std::string& os, const std::string& dpa) const = 0;
    virtual std::shared_ptr<Package> getPackage(uint16_t hwpid, uint16_t hwpidVer, uint16_t os, uint16_t dpa) const = 0;
    virtual std::map<int, std::map<double, std::vector<std::pair<int,int>>>> getDrivers(const std::string& os, const std::string& dpa) const = 0;
    virtual std::map<int, std::map<int, std::string>> getCustomDrivers(const std::string& os, const std::string& dpa) const = 0;
    virtual MapOsListDpa getOsDpa() const = 0;
    virtual std::shared_ptr<OsDpa> getOsDpa(int id) const = 0;
    virtual std::shared_ptr<OsDpa> getOsDpa(const std::string& os, const std::string& dpa) const = 0;
    virtual std::shared_ptr<Quantity> getQuantity(const uint8_t &type) const = 0;
    virtual ServerState getServerState() const = 0;
    virtual std::tuple<CacheStatus, std::string> invokeWorker() = 0;

    typedef std::function<void()> CacheReloadedFunc;
    virtual void registerCacheReloadedHandler(const std::string & clientId, CacheReloadedFunc hndl) = 0;
    virtual void unregisterCacheReloadedHandler(const std::string & clientId) = 0;

    virtual ~IJsCacheService() {};
  };
}
