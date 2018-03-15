#pragma once

#include "ComBase.h"

namespace iqrf {
  //const std::string mType_comSdevBinaryOutputEnum = "comSdevBinaryOutputEnum";
  //const std::string mType_comSdevBinaryOutputSetOutput = "comSdevBinaryOutputSetOutput";
  //const std::string mType_comSdevLightDecrementPower = "comSdevLightDecrementPower";
  //const std::string mType_comSdevLightEnum = "comSdevLightEnum";
  //const std::string mType_comSdevLightIncrementPower = "comSdevLightIncrementPower";
  //const std::string mType_comSdevLightSetPower = "comSdevLightSetPower";
  //const std::string mType_comSdevSensorEnum = "comSdevSensorEnum";
  //const std::string mType_comSdevSensorFrc = "comSdevSensorFrc";
  //const std::string mType_comSdevSensorReadwt = "comSdevSensorReadwt";

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
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
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
    }

    virtual ~ComSdevLightSetPower()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
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
    }

    virtual ~ComSdevSensorEnum()
    {
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
