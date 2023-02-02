/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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
      std::vector<item::Light> m_lights;
      //response
      std::vector<int> m_prevVals;

      SetPower()
      {}

      SetPower(const std::vector<item::Light> & lights)
        :m_lights(lights)
      {}

    public:
      // get param data passes by ctor
      const std::vector<item::Light> & getLights() const { return m_lights; }

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

      IncrementPower(const std::vector<item::Light> & lights)
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

      DecrementPower(const std::vector<item::Light> & lights)
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
