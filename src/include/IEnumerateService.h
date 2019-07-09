#pragma once

#include "EmbedExplore.h"
#include "EmbedOS.h"
#include "ShapeDefines.h"
#include "stdint.h"
#include <string>
#include <set>
#include <vector>
#include <map>

namespace iqrf {
  /// \class IEnumerateService
  class IEnumerateService
  {
  public:
    class INodeData
    {
    public:
      virtual int getNadr() const = 0;
      virtual int getHwpid() const = 0;
      virtual const embed::explore::EnumeratePtr & getEmbedExploreEnumerate() const = 0;
      virtual const embed::os::ReadPtr & getEmbedOsRead() const = 0;
      virtual ~INodeData() {}
    };
    typedef std::unique_ptr<INodeData> INodeDataPtr;

    class IFastEnumeration
    {
    public:
      class Enumerated
      {
      public:
        virtual unsigned getMid() const = 0;
        virtual int getNadr() const = 0;
        virtual int getHwpid() const = 0;
        virtual int getHwpidVer() const = 0;
        virtual int getOsBuild() const = 0;
        virtual int getOsVer() const = 0;
        virtual int getDpaVer() const = 0;
        virtual INodeDataPtr getNodeData() = 0;
        virtual ~Enumerated() {}
      };
      typedef std::unique_ptr<Enumerated> EnumeratedPtr;
      virtual const std::map<int, EnumeratedPtr> & getEnumerated() const = 0;
      virtual const std::set<int> & getBonded() const = 0;
      virtual const std::set<int> & getDiscovered() const = 0;
      virtual const std::set<int> & getNonDiscovered() const = 0;
      virtual ~IFastEnumeration() {}
    };
    typedef std::unique_ptr<IFastEnumeration> IFastEnumerationPtr;

    class IStandardSensorData
    {
    public:
      class ISensor
      {
      public:
        virtual const std::string & getSid() const = 0;
        virtual int getType() const = 0;
        virtual const std::string & getName() const = 0;
        virtual const std::string & getShortName() const = 0;
        virtual const std::string & getUnit() const = 0;
        virtual int getDecimalPlaces() const = 0;
        virtual const std::set<int> & getFrcs() const = 0;
        //TODO breakdown - array : [optional] see <iqrf.sensor.ReadSensorsWithTypes_Response> for more information.
      
        virtual ~ISensor() {}
      };

      typedef std::unique_ptr< ISensor> ISensorPtr;

      // get data as returned from driver
      virtual const std::vector<ISensorPtr> & getSensors() const = 0;
      virtual ~IStandardSensorData() {}
    };
    typedef std::unique_ptr<IStandardSensorData> IStandardSensorDataPtr;

    class IStandardBinaryOutputData
    {
    public:
      virtual int getBinaryOutputsNum() const = 0;
      virtual ~IStandardBinaryOutputData() {}
    };
    typedef std::unique_ptr<IStandardBinaryOutputData> IStandardBinaryOutputDataPtr;

    virtual IFastEnumerationPtr getFastEnumeration() const = 0;
    virtual INodeDataPtr getNodeData(uint16_t nadr) const = 0;
    virtual IStandardSensorDataPtr getStandardSensorData(uint16_t nadr) const = 0;
    virtual IStandardBinaryOutputDataPtr getStandardBinaryOutputData(uint16_t nadr) const = 0;
    virtual embed::explore::PeripheralInformationPtr getPeripheralInformationData(uint16_t nadr, int per) const = 0;
    virtual embed::explore::MorePeripheralInformationPtr getMorePeripheralInformationData(uint16_t nadr, int per) const = 0;

    virtual ~IEnumerateService() {}
  };

}
