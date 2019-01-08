#include "mngMetaDataMsgStatus.h"
#include "TestJsonMngMetaDataApi.h"
#include "Trace.h"
#include "GTestStaticRunner.h"
#include "HexStringCoversion.h"

#include "gtest/gtest.h"

#include "rapidjson/pointer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include <fstream>
#include <cstdlib>
#include <ctime>

#include "iqrf__TestJsonMngMetaDataApi.hxx"

TRC_INIT_MNAME(iqrf::TestJsonMngMetaDataApi)

using namespace rapidjson;

namespace iqrf {

  class Imp {
  private:
    Imp()
    {
    }

  public:
    shape::ILaunchService* m_iLaunchService = nullptr;
    iqrf::ITestSimulationMessaging* m_iTestSimulationMessaging = nullptr;

    shape::GTestStaticRunner m_gtest;

    static Imp& get()
    {
      static Imp imp;
      return imp;
    }

    ~Imp()
    {
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "TestJsonMngMetaDataApi instance activate" << std::endl <<
        "******************************"
      );

      m_gtest.runAllTests(m_iLaunchService);

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "TestJsonMngMetaDataApi instance deactivate" << std::endl <<
        "******************************"
      );
      TRC_FUNCTION_LEAVE("")
    }

    void attachInterface(iqrf::ITestSimulationMessaging* iface)
    {
      m_iTestSimulationMessaging = iface;
    }

    void detachInterface(iqrf::ITestSimulationMessaging* iface)
    {
      if (m_iTestSimulationMessaging == iface) {
        m_iTestSimulationMessaging = nullptr;
      }
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

  };

  ////////////////////////////////////
  TestJsonMngMetaDataApi::TestJsonMngMetaDataApi()
  {
  }

  TestJsonMngMetaDataApi::~TestJsonMngMetaDataApi()
  {
  }

  void TestJsonMngMetaDataApi::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestJsonMngMetaDataApi::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestJsonMngMetaDataApi::modify(const shape::Properties *props)
  {
  }

  void TestJsonMngMetaDataApi::attachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonMngMetaDataApi::detachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonMngMetaDataApi::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonMngMetaDataApi::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonMngMetaDataApi::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestJsonMngMetaDataApi::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  ////////////////////////////////////////////////////////
  class TestMetaData : public ::testing::Test
  {
  protected:

    void SetUp(void) override
    {
      ASSERT_NE(nullptr, &Imp::get().m_iLaunchService);
    };

    void TearDown(void) override
    {
    };

    static std::string jsonToStr(const rapidjson::Value& val)
    {
      rapidjson::Document doc;
      doc.CopyFrom(val, doc.GetAllocator());
      rapidjson::StringBuffer buffer;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      return buffer.GetString();
    }

    static std::string loadJsonMsg(const std::string& fname)
    {
      std::ifstream jsFile(fname);
      if (jsFile.is_open()) {
        std::ostringstream strStream;
        strStream << jsFile.rdbuf();
        std::string msg = strStream.str();
        return msg;
      }
    }

    static std::string getOutAndParse(rapidjson::Document& jmoDoc)
    {
      std::string jmo = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(2000);
      TRC_DEBUG(jmo);
      jmoDoc.Parse(jmo);
      return jmo;
    }

    template <typename T>
    static void getVal(const char* name, rapidjson::Value* v, T &val )
    {
      if (v) {
        Value* valPtr = Pointer(name).Get(*v);
        if (valPtr && valPtr->Is<T>()) {
          val = valPtr->Get<T>();
        }
      }
    }

    static std::string m_metaIdOver;
    static int m_snOver;

  };

  std::string TestMetaData::m_metaIdOver;
  int TestMetaData::m_snOver;
  const char* UNEXPECTED = "unexpected";

  ///////////////////////////////////////////////
  ////////////////// Tests //////////////////////
  ///////////////////////////////////////////////

  const unsigned MILLIS_WAIT = 1000;

  TEST_F(TestMetaData, ExportNadrMidMap_validDefault)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ExportNadrMidMap_valid.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);

    int cnt = -1;
    int nAdr = -1;
    std::string mid;
    auto pVal = Pointer("/data/rsp/nadrMidMap").Get(jmoDoc);
    if (pVal && pVal->IsArray()) {
      cnt = pVal->Size();
      Value* it = pVal->Begin();
      getVal("/nAdr", it, nAdr);
      getVal("/mid", it, mid);
    }

    EXPECT_EQ(cnt, 4);
    EXPECT_EQ(nAdr, 0);
    EXPECT_EQ(mid, "000");
  }

  TEST_F(TestMetaData, ExportMetaDataAll_validDefault)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ExportMetaDataAll_valid.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);

    int cnt = -1;
    std::string mid;
    std::string metaId;
    auto pVal = Pointer("/data/rsp/midMetaIdMap").Get(jmoDoc);
    if (pVal && pVal->IsArray()) {
      cnt = pVal->Size();
      Value* it = pVal->Begin();
      getVal("/mid", it, mid);
      getVal("/metaId", it, metaId);
    }

    EXPECT_EQ(cnt, 4);
    EXPECT_EQ(mid, "000");
    EXPECT_EQ(metaId, "0");
  }

  TEST_F(TestMetaData, ImportNadrMidMap_valid)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ImportNadrMidMap_valid.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);
  }

  TEST_F(TestMetaData, ImportNadrMidMap_duplicity1)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ImportNadrMidMap_duplicity1.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_duplicitParams), estr);

    int nAdr = -1;
    std::string mid;
    auto dupVal = Pointer("/data/rsp/duplicityNadrMid").Get(jmoDoc);
    if (dupVal && dupVal->IsArray()) {
      Value* it = dupVal->Begin();
      getVal("/nAdr", it, nAdr);
      getVal("/mid", it, mid);
    }

    EXPECT_EQ(nAdr, 0);
    EXPECT_EQ(mid, "111");
  }

  TEST_F(TestMetaData, ImportNadrMidMap_duplicity2)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ImportNadrMidMap_duplicity2.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_duplicitParams), estr);

    int nAdr = -1;
    std::string mid;
    auto dupVal = Pointer("/data/rsp/duplicityNadrMid").Get(jmoDoc);
    if (dupVal && dupVal->IsArray()) {
      Value* it = dupVal->Begin();
      getVal("/nAdr", it, nAdr);
      getVal("/mid", it, mid);
    }

    EXPECT_EQ(nAdr, 1);
    EXPECT_EQ(mid, "000");
  }

  TEST_F(TestMetaData, ExportNadrMidMap_valid)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ExportNadrMidMap_valid.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);

    int cnt = -1;
    int nAdr = -1;
    std::string mid;
    auto pVal = Pointer("/data/rsp/nadrMidMap").Get(jmoDoc);
    if (pVal && pVal->IsArray()) {
      cnt = pVal->Size();
      Value* it = pVal->Begin();
      getVal("/nAdr", it, nAdr);
      getVal("/mid", it, mid);
    }

    EXPECT_EQ(cnt, 4);
    EXPECT_EQ(nAdr, 0);
    EXPECT_EQ(mid, "000");
  }

  TEST_F(TestMetaData, ImportMetaDataAll_valid)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ImportMetaDataAll_valid.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);
  }

  TEST_F(TestMetaData, ImportMetaDataAll_duplicitMetaId)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ImportMetaDataAll_duplicitMetaId.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_duplicitParams), estr);

    std::string metaId;
    auto dupVal = Pointer("/data/rsp/duplicitMetaId/0").Get(jmoDoc);
    if (dupVal && dupVal->IsString()) {
      metaId = dupVal->GetString();
    }

    EXPECT_EQ("4", metaId);

  }

  TEST_F(TestMetaData, ImportMetaDataAll_duplicitMidMetaIdPair1)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ImportMetaDataAll_duplicitMidMetaIdPair1.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_duplicitParams), estr);

    std::string mid;
    std::string metaId;
    auto dupVal = Pointer("/data/rsp/duplicitMidMetaIdPair").Get(jmoDoc);
    if (dupVal && dupVal->IsArray()) {
      Value* it = dupVal->Begin();
      getVal("/mid", it, mid);
      getVal("/metaId", it, metaId);
    }

    EXPECT_EQ("444", mid);
    EXPECT_EQ("5", metaId);
  }

  TEST_F(TestMetaData, ImportMetaDataAll_duplicitMidMetaIdPair2)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ImportMetaDataAll_duplicitMidMetaIdPair2.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_duplicitParams), estr);

    std::string mid;
    std::string metaId;
    auto dupVal = Pointer("/data/rsp/duplicitMidMetaIdPair").Get(jmoDoc);
    if (dupVal && dupVal->IsArray()) {
      Value* it = dupVal->Begin();
      getVal("/mid", it, mid);
      getVal("/metaId", it, metaId);
    }

    EXPECT_EQ("555", mid);
    EXPECT_EQ("4", metaId);
  }

  TEST_F(TestMetaData, ExportMetaDataAll_valid)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ExportMetaDataAll_valid.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);

    int cnt = -1;
    std::string mid;
    std::string metaId;
    auto pVal = Pointer("/data/rsp/midMetaIdMap").Get(jmoDoc);
    if (pVal && pVal->IsArray()) {
      cnt = pVal->Size();
      Value* it = pVal->Begin();
      getVal("/mid", it, mid);
      getVal("/metaId", it, metaId);
    }

    EXPECT_EQ(cnt, 5);
    EXPECT_EQ(mid, "111");
    EXPECT_EQ(metaId, "1");
  }

  TEST_F(TestMetaData, VerifyMetaDataAll)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/VerifyMetaDataAll.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);

    int cnt = -1;
    std::string mid0, mid1;
    std::string metaId;

    {
      auto pVal = Pointer("/data/rsp/inconsistentMid").Get(jmoDoc);
      if (pVal && pVal->IsArray()) {
        cnt = pVal->Size();
        getVal("/data/rsp/inconsistentMid/0", &jmoDoc, mid0);
      }

    EXPECT_EQ(cnt, 1);
    EXPECT_EQ(mid0, "000");
    }

    {
      auto pVal = Pointer("/data/rsp/orphanedMid").Get(jmoDoc);
      if (pVal && pVal->IsArray()) {
        cnt = pVal->Size();
        getVal("/data/rsp/orphanedMid/0", &jmoDoc, mid0);
        getVal("/data/rsp/orphanedMid/1", &jmoDoc, mid1);
      }

      EXPECT_EQ(cnt, 2);
      EXPECT_EQ(mid0, "444");
      EXPECT_EQ(mid1, "555");
    }

    {
      auto pVal = Pointer("/data/rsp/inconsistentMetaId").Get(jmoDoc);
      if (pVal && pVal->IsArray()) {
        cnt = pVal->Size();
        getVal("/data/rsp/inconsistentMetaId/0", &jmoDoc, metaId);
      }

      EXPECT_EQ(cnt, 1);
      EXPECT_EQ(metaId, "6");
    }

    {
      auto pVal = Pointer("/data/rsp/orphanedMetaId").Get(jmoDoc);
      if (pVal && pVal->IsArray()) {
        cnt = pVal->Size();
        getVal("/data/rsp/orphanedMetaId/0", &jmoDoc, metaId);
      }

      EXPECT_EQ(cnt, 1);
      EXPECT_EQ(metaId, "5");
    }
  }

  TEST_F(TestMetaData, SetMetaData_unknownMetaId)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/SetMetaData_unknownMetaId.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_metaIdUnknown), estr);
  }

  TEST_F(TestMetaData, GetMetaData_unknownMetaId)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/GetMetaData_unknownMetaId.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_metaIdUnknown), estr);
  }

  TEST_F(TestMetaData, GetMetaData_emptyMetaId)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/GetMetaData_emptyMetaId.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_badParams), estr);
  }

  TEST_F(TestMetaData, SetMetaData_addMetaData)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/SetMetaData_addMetaData.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    //test
    std::string metaId = UNEXPECTED;
    getVal("/data/rsp/metaId", &jmoDoc, metaId);
    int sn = -1;
    getVal("/data/rsp/metaData/sn", &jmoDoc, sn);

    // save for next testcase
    TestMetaData::m_metaIdOver = metaId;
    TestMetaData::m_snOver = sn;

    EXPECT_NE("", metaId);
  }

  TEST_F(TestMetaData, GetMetaData_validMetaId)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/GetMetaData_emptyMetaId.json");
    
    //set valid metaData
    Document jmi;
    jmi.Parse(msg);
    //set metaId from previous addMetaData testcase
    Pointer("/data/req/metaId").Set(jmi, m_metaIdOver);
    msg = jsonToStr(jmi);
   
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    //test
    int sn = -1;
    getVal("/data/rsp/metaData/sn", &jmoDoc, sn);

    EXPECT_EQ(m_snOver, sn);
  }

  TEST_F(TestMetaData, SetMetaData_updateMetaData)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/SetMetaData_addMetaData.json");
    
    //set valid metaData
    Document jmi;
    jmi.Parse(msg);
    //set metaId from previous addMetaData testcase
    Pointer("/data/req/metaId").Set(jmi, m_metaIdOver);
    m_snOver = 9876; //updated value
    Pointer("/data/req/metaData/sn").Set(jmi, m_snOver);
    msg = jsonToStr(jmi);

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    //test
    int sn = -1;
    getVal("/data/rsp/metaData/sn", &jmoDoc, sn);

    EXPECT_EQ(m_snOver, sn);
  }

  TEST_F(TestMetaData, GetMetaData_updatedMetaData)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/GetMetaData_emptyMetaId.json");

    //set valid metaData
    Document jmi;
    jmi.Parse(msg);
    //set metaId from previous addMetaData testcase
    Pointer("/data/req/metaId").Set(jmi, m_metaIdOver);
    msg = jsonToStr(jmi);

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    //test
    int sn = -1;
    getVal("/data/rsp/metaData/sn", &jmoDoc, sn);

    EXPECT_EQ(m_snOver, sn);
  }

  TEST_F(TestMetaData, SetMetaData_eraseMetaData)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/SetMetaData_addMetaData.json");

    //empty metaData
    Document jmi;
    jmi.Parse(msg);
    //empty metaData
    Pointer("/data/req/metaData").Set(jmi, Value().SetObject());
    //set metaId from previous addMetaData testcase
    Pointer("/data/req/metaId").Set(jmi, m_metaIdOver);
    msg = jsonToStr(jmi);

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    //test
    std::string metaId = UNEXPECTED;
    getVal("/data/rsp/metaId", &jmoDoc, metaId);

    TestMetaData::m_metaIdOver = metaId;

    EXPECT_NE("", metaId);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);
  }

  TEST_F(TestMetaData, GetMetaData_erasedMetaData)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/GetMetaData_emptyMetaId.json");

    //set valid metaData
    Document jmi;
    jmi.Parse(msg);
    //set metaId from previous addMetaData testcase
    Pointer("/data/req/metaId").Set(jmi, m_metaIdOver);
    msg = jsonToStr(jmi);

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_metaIdUnknown), estr);
  }

  ///////////////////////
  TEST_F(TestMetaData, SetMetaData_addMetaData2)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/SetMetaData_addMetaData.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    //test
    std::string metaId = UNEXPECTED;
    getVal("/data/rsp/metaId", &jmoDoc, metaId);

    int sn = -1;
    getVal("/data/rsp/metaData/sn", &jmoDoc, sn);

    // save for next testcase
    TestMetaData::m_metaIdOver = metaId;
    TestMetaData::m_snOver = sn;

    EXPECT_NE("", metaId);
  }

  TEST_F(TestMetaData, SetMidMetaId_emptyMid)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/SetMidMetaId_emptyMid.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_badParams), estr);
  }

  TEST_F(TestMetaData, SetMidMetaId_insertMidMetaIdUnknown)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/SetMidMetaId_insertMidMetaIdUnknown.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_metaIdUnknown), estr);
  }

  TEST_F(TestMetaData, SetMidMetaId_insertMidMetaIdValid)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/SetMidMetaId_insertMidMetaIdUnknown.json");
    
    //set valid metaData
    Document jmi;
    jmi.Parse(msg);
    //set metaId from previous addMetaData2 testcase
    Pointer("/data/req/metaId").Set(jmi, m_metaIdOver);
    msg = jsonToStr(jmi);

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);
  }

  TEST_F(TestMetaData, SetMidMetaId_insertMidMetaIdAssignedMid)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/SetMidMetaId_insertMidMetaIdUnknown.json");

    //set valid metaData
    Document jmi;
    jmi.Parse(msg);
    //set metaId from previous addMetaData2 testcase
    Pointer("/data/req/metaId").Set(jmi, m_metaIdOver);
    msg = jsonToStr(jmi);

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_midAssigned), estr);
  }

  TEST_F(TestMetaData, SetMidMetaId_insertMidMetaIdAssignedMetaId)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/SetMidMetaId_insertMidMetaIdAssignedMetaId.json");

    //set valid metaData
    Document jmi;
    jmi.Parse(msg);
    //set metaId from previous addMetaData2 testcase
    Pointer("/data/req/metaId").Set(jmi, m_metaIdOver);
    msg = jsonToStr(jmi);

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_metaIdAssigned), estr);
  }

  TEST_F(TestMetaData, GetMidMetaId_MidMetaIdValid)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/GetMidMetaId_midMetaIdValid.json");

    //set valid metaData
    Document jmi;

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);

    int sn = -1;
    getVal("/data/rsp/metaData/sn", &jmoDoc, sn);

    EXPECT_EQ(m_snOver, sn);
  }

  TEST_F(TestMetaData, GetMidMetaId_unknownMid)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/GetMidMetaId_unknownMid.json");

    //set valid metaData
    Document jmi;

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_midUnknown), estr);
  }

  TEST_F(TestMetaData, GetMidMetaId_emptyMid)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/GetMidMetaId_emptyMid.json");

    //set valid metaData
    Document jmi;

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_badParams), estr);
  }

  TEST_F(TestMetaData, GetNadrMetaData_empty)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/GetNadrMetaData_empty.json");

    //set valid metaData
    Document jmi;

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    std::string estr = UNEXPECTED;
    getVal("/data/errorStr", &jmoDoc, estr);

    EXPECT_EQ(mngMetaDataMsgStatusConvertor::enum2str(mngMetaDataMsgStatus::st_midInconsistent), estr);
  }

  TEST_F(TestMetaData, GetNadrMetaData_valid)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/GetNadrMetaData_valid.json");

    //set valid metaData
    Document jmi;

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);
  }

  TEST_F(TestMetaData, ImportNadrMidMap_validDefault)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ImportNadrMidMap_validDefault.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);
  }

  TEST_F(TestMetaData, ImportMetaDataAll_validDefault)
  {
    using namespace rapidjson;

    std::string msg = loadJsonMsg("./configuration/json/ImportMetaDataAll_validDefault.json");
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(msg);

    Document jmoDoc;
    std::string jmo = getOutAndParse(jmoDoc);

    int status = -1;
    getVal("/data/status", &jmoDoc, status);

    EXPECT_EQ(0, status);
  }

}
