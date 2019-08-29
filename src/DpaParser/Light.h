#pragma once

#include <string>
#include <set>
#include <vector>
#include <memory>

namespace iqrf
{
  namespace light
  {
    namespace item
    {
      class Light
      {
      public:
        Light()
        {}

        int getIndex() const { return m_index; }
        int getPower() const { return m_power; }
        int getTime() const { return m_time; }

        Light(int index, int power, int time)
          :m_index(index)
          , m_power(power)
          , m_time(time)
        {}

      protected:
        int m_index = 0;
        int m_power = 0;
        int m_time = 0;
      };
      typedef std::unique_ptr<Light> LightPtr;
    }

    ////////////////
    class Enumerate
    {
    protected:
      int m_lightsNum;

      Enumerate()
      {}

    public:
      virtual ~Enumerate() {}

      int getLightsNum() const { return m_lightsNum; }
    };
    typedef std::unique_ptr<Enumerate> EnumeratePtr;

    ////////////////
    class SetPower
    {
    protected:
      //param
      std::vector<item::LightPtr> m_lights;
      //response
      std::vector<int> m_prevVals;

      SetPower()
      {}

      SetPower(const std::vector<item::LightPtr> & lights)
        :m_lights(lights)
      {}

    public:
      // get param data passes by ctor
      const std::vector<item::LightPtr> & getLights() const { return m_lights; }

      // get data as returned from driver
      const std::vector<int> & getPrevVals() const { return m_prevVals; }

      virtual ~SetPower() {}
    };
    typedef std::unique_ptr<SetPower> SetPowerPtr;

    ////////////////
    class IncrementPower : public SetPower
    {
    protected:
      IncrementPower()
        :SetPower()
      {}

      IncrementPower(const std::vector<item::LightPtr> & lights)
        :SetPower(lights)
      {}

    public:
      virtual ~IncrementPower() {}
    };
    typedef std::unique_ptr<IncrementPower> IncrementPowerPtr;

    ////////////////
    class DecrementPower : public SetPower
    {
    protected:
      DecrementPower()
        :SetPower()
      {}

      DecrementPower(const std::vector<item::LightPtr> & lights)
        :SetPower(lights)
      {}

    public:
      virtual ~DecrementPower() {}
    };
    typedef std::unique_ptr<DecrementPower> DecrementPowerPtr;

    ////////////////
    // not implemented just kept here for future use and to be consistent with other standards
    class Frc
    {
    protected:
      Frc()
      {}

    public:
      virtual ~Frc() {}
    };
    typedef std::unique_ptr<Frc> FrcPtr;

  } //namespace light
} //namespace iqrf
