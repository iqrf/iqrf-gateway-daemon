#pragma once

#include "ComBase.h"

namespace iqrf {
  //-------------------------------------------------------
  class ComSdevBinaryOutputEnum : public ComBase
  {
  public:
    ComSdevBinaryOutputEnum() = delete;
    ComSdevBinaryOutputEnum(rapidjson::Document& doc)
      :ComBase(doc)
    {
    }

    virtual ~ComSdevBinaryOutputEnum()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
    }

  private:
  };

  //-------------------------------------------------------
  class ComSdevBinaryOutputSetOutput : public ComBase
  {
  public:
    ComSdevBinaryOutputSetOutput() = delete;
    ComSdevBinaryOutputSetOutput(rapidjson::Document& doc)
      :ComBase(doc)
    {
    }

    virtual ~ComSdevBinaryOutputSetOutput()
    {
      /*"rsp": {
        "nAdr": "EEEE",
          "hwpId" : "DD",
          "rCode" : "aaaaa",
          "dpaVal" : "c",
          "prevVals" : [
            false,
              true
          ]
      },*/
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      /*
      "req": {
      "nAdr": "EEEE",
      "hwpId": "DD",
      "binOuts": [
      {
      "index": 3.140000,
      "state": true,
      "time": 3.140000
      },
      {
      "index": 3.140000,
      "state": true,
      "time": 3.140000
      },
      {
      "index": 3.140000,
      "state": true,
      "time": 3.140000
      }
      ]
      },*/
    }

  private:
  };
  
  //-------------------------------------------------------
  class ComSdevLightDecrementPower : public ComBase
  {
  public:
    ComSdevLightDecrementPower() = delete;
    ComSdevLightDecrementPower(rapidjson::Document& doc)
      :ComBase(doc)
    {
    }

    virtual ~ComSdevLightDecrementPower()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
    }

  private:
  };

  //-------------------------------------------------------
  class ComSdevLightEnum : public ComBase
  {
  public:
    ComSdevLightEnum() = delete;
    ComSdevLightEnum(rapidjson::Document& doc)
      :ComBase(doc)
    {
    }

    virtual ~ComSdevLightEnum()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
    }

  private:
  };

  //-------------------------------------------------------
  class ComSdevLightIncrementPower : public ComBase
  {
  public:
    ComSdevLightIncrementPower() = delete;
    ComSdevLightIncrementPower(rapidjson::Document& doc)
      :ComBase(doc)
    {
    }

    virtual ~ComSdevLightIncrementPower()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
    }

  private:
  };

  //-------------------------------------------------------
  class ComSdevLightSetPower : public ComBase
  {
  public:
    ComSdevLightSetPower() = delete;
    ComSdevLightSetPower(rapidjson::Document& doc)
      :ComBase(doc)
    {
      /*"req": {
        "nAdr": "EEEE",
          "hwpId" : "DD",
          "lights" : [
        {
          "index": 3.140000,
            "power" : 3.140000,
            "time" : 3.140000
        },
        {
          "index": 3.140000,
          "power" : 3.140000,
          "time" : 3.140000
        },
        {
          "index": 3.140000,
          "power" : 3.140000,
          "time" : 3.140000
        }
          ]
      },*/

    }

    virtual ~ComSdevLightSetPower()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      /*
      "rsp": {
        "nAdr": "EEEE",
          "hwpId" : "DD",
          "rCode" : "aaaaa",
          "dpaVal" : "c",
          "prevVals" : [
            3.140000,
              3.140000
          ]
      },*/
    }

  private:
  };

  //-------------------------------------------------------
  class ComSdevSensorEnum : public ComBase
  {
  public:
    ComSdevSensorEnum() = delete;
    ComSdevSensorEnum(rapidjson::Document& doc)
      :ComBase(doc)
    {
      /*"req": {
        "nAdr": "EEEE",
          "hwpId" : "DD"
      },*/
    }

    virtual ~ComSdevSensorEnum()
    {
      /*
      "rsp": {
        "nAdr": "EEEE",
          "hwpId" : "DD",
          "rCode" : "aaaaa",
          "dpaVal" : "c",
          "result" : [
        {
          "type": 3.140000,
            "name" : "CCCCCCC",
            "shortName" : "BBBB",
            "unit" : "DD",
            "frcs" : [
              1
            ]
        },
        {
          "type": 3.140000,
          "name" : "bbbbbbbb",
          "shortName" : "cccccc",
          "unit" : "DDDDDDDDD",
          "frcs" : [
            1,
              1,
              1
          ]
        }
          ]
      },*/
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
    }

  private:
  };

  //-------------------------------------------------------
  class ComSdevSensorFrc : public ComBase
  {
  public:
    ComSdevSensorFrc() = delete;
    ComSdevSensorFrc(rapidjson::Document& doc)
      :ComBase(doc)
    {
    }

    virtual ~ComSdevSensorFrc()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
    }

  private:
  };

  //-------------------------------------------------------
  class ComSdevSensorReadwt : public ComBase
  {
  public:
    ComSdevSensorReadwt() = delete;
    ComSdevSensorReadwt(rapidjson::Document& doc)
      :ComBase(doc)
    {
    }

    virtual ~ComSdevSensorReadwt()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
    }

  private:
  };
}
