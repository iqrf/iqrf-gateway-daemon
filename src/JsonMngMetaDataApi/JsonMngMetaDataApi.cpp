#include "VersionInfo.h"
#include "ApiMsg.h"
#include "mngMetaDataMsgStatus.h"
#include "JsonMngMetaDataApi.h"
#include "ObjectFactory.h"
#include "HexStringCoversion.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/schema.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <random>
#include <climits>

#include "iqrf__JsonMngMetaDataApi.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonMngMetaDataApi);

using namespace rapidjson;

namespace iqrf {
  //////////////////////////////////////////////
  class JsonMngMetaDataApi::Imp
  {
  public:

    int getRandom() {
      static std::random_device rd;
      static std::mt19937 gen{ rd() };
      static std::uniform_int_distribution<> dis{ 0, INT_MAX };
      return dis(gen);
    }

    ///////////////////
    //Data structures:
    class MetaData
    {
    public:
      MetaData()
      {
        m_doc.SetObject();
      }

      MetaData(const MetaData& o)
      {
        m_doc.CopyFrom(o.m_doc, m_doc.GetAllocator());
      }

      MetaData& operator = (const MetaData& o)
      {
        m_doc.CopyFrom(o.m_doc, m_doc.GetAllocator());
        return *this;
      }

      const rapidjson::Value& getValue() const
      {
        return m_doc;
      }

      void setValue(const rapidjson::Value& val)
      {
        m_doc.CopyFrom(val, m_doc.GetAllocator());
      }

      bool empty() const
      {
        return m_doc.MemberCount() == 0;
      }

    private:
      rapidjson::Document m_doc;
    };

    typedef std::string metaId;
    typedef std::string mid;
    typedef uint16_t nadr;
    typedef std::map<metaId, std::shared_ptr<MetaData>> metaIdMetaDataMap;

    template<typename K, typename V>
    class UniquePairMap : private std::map<K, V>
    {
    public:
      typedef typename std::map<K, V>::iterator iterator;
      
      iterator myBegin()
      {
        return this->begin();
      }

      iterator myEnd()
      {
        return this->end();
      }

      static constexpr int success = 0;
      static constexpr int duplicit_key = -1;
      static constexpr int duplicit_val = -2;
      static constexpr int failure = -3;

      int myInsert(const K& key, const V& val) {
        if (keyset.find(key) != keyset.end()) return duplicit_key;
        if (valset.find(val) != valset.end()) return duplicit_val;
        keyset.insert(key);
        valset.insert(val);
        auto res = this->insert(std::make_pair(key, val));
        return res.second ? success : failure;
      }

      void myErase(const K& key) {
        auto found = this->find(key);
        if (found != this->end()) {
          Imp::mid val = found->second;
          keyset.erase(key);
          valset.erase(val);
          this->erase(key);
        }
      }

      void myClear() {
        this->clear();
      }

      V myFind(const K& key, const V& defaultVal) {
        auto found = this->find(key);
        if (found != this->end()) {
          return found->second;
        }
        return defaultVal;
      }

      K myFindKey(const V& val, const K& defaultKey) {
        for (auto & p : *this) {
          if (p.second == val) {
            return p.first;
          }
        }
        return defaultKey;
      }

      bool hasValue(const V& val) {
        return (valset.find(val) != valset.end());
      }

    private:
      std::set<K> keyset;
      std::set<V> valset;
    };

    typedef UniquePairMap<Imp::nadr, Imp::mid> uniqueNadrMidMap;
    typedef UniquePairMap<Imp::mid, Imp::metaId> uniqueMidMetaIdMap;

    metaIdMetaDataMap m_metaIdMetaDataMap;
    uniqueMidMetaIdMap m_midMetaIdMap;
    uniqueNadrMidMap m_nadrMidMap;

    std::recursive_mutex m_mux;

    static const nadr nadrEmpty = 0xFFFF;

    const std::string mType_ExportMetaDataAll = "mngMetaData_ExportMetaDataAll";
    const std::string mType_ExportNadrMidMap = "mngMetaData_ExportNadrMidMap";
    const std::string mType_GetNadrMetaData = "mngMetaData_GetNadrMetaData";
    const std::string mType_ImportMetaDataAll = "mngMetaData_ImportMetaDataAll";
    const std::string mType_ImportNadrMidMap = "mngMetaData_ImportNadrMidMap";
    const std::string mType_VerifyMetaDataAll = "mngMetaData_VerifyMetaDataAll";
    const std::string mType_GetMetaData = "mngMetaData_GetMetaData";
    const std::string mType_GetMidMetaId = "mngMetaData_GetMidMetaId";
    const std::string mType_SetMetaData = "mngMetaData_SetMetaData";
    const std::string mType_SetMidMetaId = "mngMetaData_SetMidMetaId";

    class MetaDataMsg : public ApiMsg
    {
    public:
      MetaDataMsg() = delete;
      MetaDataMsg(const rapidjson::Document& doc)
        :ApiMsg(doc)
      {
      }

      virtual ~MetaDataMsg()
      {
      }

      mngMetaDataMsgStatus getErr()
      {
        return m_st;
      }

      void setErr(mngMetaDataMsgStatus st)
      {
        m_st = st;
        m_success = false;
      }

      bool isSuccess()
      {
        return m_success;
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        if (m_success) {
          setStatus("ok", 0);
        }
        else {
          if (getVerbose()) {
            Pointer("/data/errorStr").Set(doc, mngMetaDataMsgStatusConvertor::enum2str(m_st));
          }
          setStatus("err", -1);
        }
      }

      virtual void handleMsg(JsonMngMetaDataApi::Imp* imp) = 0;

    private:
      mngMetaDataMsgStatus m_st = mngMetaDataMsgStatus::st_ok;
      bool m_success = true;
    };

    //////////////////////////////////////////////
    class ImportNadrMidMap : public MetaDataMsg
    {
    public:
      ImportNadrMidMap() = delete;
      ImportNadrMidMap(const rapidjson::Document& doc)
        :MetaDataMsg(doc)
      {
        const Value *val = Pointer("/data/req/nadrMidMap").Get(doc);
        for (auto it = val->Begin(); it != val->End(); it++) {
          int nadr = Pointer("/nAdr").Get(*it)->GetInt();
          std::string mid = Pointer("/mid").Get(*it)->GetString();
          if (uniqueNadrMidMap::success != m_myMap.myInsert(nadr, mid)) {
            m_nonUniquePairsMap.insert(std::make_pair(nadr, mid));
          }
        }
      }

      virtual ~ImportNadrMidMap()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Value dup;
        dup.SetArray();
        for (auto it : m_nonUniquePairsMap) {
          Value item;
          item.SetObject();
          Pointer("/nAdr").Set(item, it.first, doc.GetAllocator());
          Pointer("/mid").Set(item, it.second, doc.GetAllocator());
          dup.PushBack(item, doc.GetAllocator());
        }
        Pointer("/data/rsp/duplicityNadrMid").Set(doc, dup);
        MetaDataMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonMngMetaDataApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        std::lock_guard<std::recursive_mutex> lock(imp->getMux());

        if (m_nonUniquePairsMap.size() == 0) {
          imp->getNadrMidMap() = m_myMap;
          //update file
          imp->updateMetaData();
        }
        else {
          setErr(mngMetaDataMsgStatus::st_duplicitParams);
        }

        TRC_FUNCTION_LEAVE("");
      }

    private:
      uniqueNadrMidMap m_myMap;
      std::multimap<Imp::nadr, Imp::mid>  m_nonUniquePairsMap;
    };

    //////////////////////////////////////////////
    class ExportNadrMidMap : public MetaDataMsg
    {
    public:
      ExportNadrMidMap() = delete;
      ExportNadrMidMap(const rapidjson::Document& doc)
        :MetaDataMsg(doc)
      {
      }

      virtual ~ExportNadrMidMap()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        std::lock_guard<std::recursive_mutex> lock(m_imp->getMux());

        if (m_imp) {
          Value arr;
          arr.SetArray();
          auto& mmap = m_imp->getNadrMidMap();
          for (auto it = mmap.myBegin(); it != mmap.myEnd(); it++) {
            Value item;
            Pointer("/nAdr").Set(item, it->first, doc.GetAllocator());
            Pointer("/mid").Set(item, it->second, doc.GetAllocator());
            arr.PushBack(item, doc.GetAllocator());
          }
          Pointer("/data/rsp/nadrMidMap").Set(doc, arr);
        }
        MetaDataMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonMngMetaDataApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        m_imp = imp;
        TRC_FUNCTION_LEAVE("");
      }

    private:
      JsonMngMetaDataApi::Imp* m_imp = nullptr;
    };

    //////////////////////////////////////////////
    class ImportMetaDataAll : public MetaDataMsg
    {
    public:
      ImportMetaDataAll() = delete;
      ImportMetaDataAll(const rapidjson::Document& doc)
        :MetaDataMsg(doc)
      {
        //process metaIdMetaDataMap
        const Value *val1 = Pointer("/data/req/metaIdMetaDataMap").Get(doc);

        for (auto it = val1->Begin(); it != val1->End(); it++) {

          std::string metaId = Pointer("/metaId").Get(*it)->GetString();

          std::shared_ptr<Imp::MetaData> metaData(shape_new Imp::MetaData());
          metaData->setValue(*(Pointer("/metaData").Get(*it)));

          auto res = m_myMetaIdMetaDataMap.insert(std::make_pair(metaId, metaData));
          if (!res.second) {
            m_duplicitMetaId.push_back(metaId);
          }
        }

        //process midMetaIdMap
        const Value *val2 = Pointer("/data/req/midMetaIdMap").Get(doc);

        for (auto it = val2->Begin(); it != val2->End(); it++) {

          std::string mid = Pointer("/mid").Get(*it)->GetString();
          std::string metaId = Pointer("/metaId").Get(*it)->GetString();

          if (uniqueNadrMidMap::success != m_myMidMetaIdMap.myInsert(mid, metaId)) {
            m_nonUniquePairsMap.insert(std::make_pair(mid, metaId));
          }
        }
      }

      virtual ~ImportMetaDataAll()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        {
          Value arr;
          arr.SetArray();
          for (auto it : m_duplicitMetaId) {
            Value item;
            item.SetString(it, doc.GetAllocator());
            //Pointer()
            arr.PushBack(item, doc.GetAllocator());
          }
          Pointer("/data/rsp/duplicitMetaId").Set(doc, arr);
        }
        {
          Value arr;
          arr.SetArray();
          for (auto it : m_nonUniquePairsMap) {
            Value item;
            Pointer("/mid").Set(item, it.first, doc.GetAllocator());
            Pointer("/metaId").Set(item, it.second, doc.GetAllocator());
            arr.PushBack(item, doc.GetAllocator());
          }
          Pointer("/data/rsp/duplicitMidMetaIdPair").Set(doc, arr);
        }

        MetaDataMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonMngMetaDataApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        std::lock_guard<std::recursive_mutex> lock(imp->getMux());

        if (m_nonUniquePairsMap.size() != 0 || m_duplicitMetaId.size() != 0) {
          setErr(mngMetaDataMsgStatus::st_duplicitParams);
        }
        else {
          imp->getMidMetaIdMap() = m_myMidMetaIdMap;
          imp->getMetaIdMetaDataMap() = m_myMetaIdMetaDataMap;
          //update file
          imp->updateMetaData();
        }

        TRC_FUNCTION_LEAVE("");
      }

    private:
      metaIdMetaDataMap m_myMetaIdMetaDataMap;
      std::vector<std::string> m_duplicitMetaId;

      uniqueMidMetaIdMap m_myMidMetaIdMap;
      std::multimap<Imp::mid, Imp::metaId>  m_nonUniquePairsMap;
    };

    //////////////////////////////////////////////
    class ExportMetaDataAll : public MetaDataMsg
    {
    public:
      ExportMetaDataAll() = delete;
      ExportMetaDataAll(const rapidjson::Document& doc)
        :MetaDataMsg(doc)
      {
      }

      virtual ~ExportMetaDataAll()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        if (m_imp) {
          std::lock_guard<std::recursive_mutex> lock(m_imp->getMux());
          {
            Value arr;
            arr.SetArray();
            auto& mmap = m_imp->getMidMetaIdMap();
            for (auto it = mmap.myBegin(); it != mmap.myEnd(); it++) {
              Value item;
              Pointer("/mid").Set(item, it->first, doc.GetAllocator());
              Pointer("/metaId").Set(item, it->second, doc.GetAllocator());
              arr.PushBack(item, doc.GetAllocator());
            }
            Pointer("/data/rsp/midMetaIdMap").Set(doc, arr);
          }
          {
            Value arr;
            arr.SetArray();
            auto& mmap = m_imp->getMetaIdMetaDataMap();
            for (auto it = mmap.begin(); it != mmap.end(); it++) {
              Value item;
              Pointer("/metaId").Set(item, it->first, doc.GetAllocator());
              Pointer("/metaData").Set(item, it->second->getValue(), doc.GetAllocator());
              arr.PushBack(item, doc.GetAllocator());
            }
            Pointer("/data/rsp/metaIdMetaDataMap").Set(doc, arr);
          }
        }

        MetaDataMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonMngMetaDataApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        m_imp = imp;
        TRC_FUNCTION_LEAVE("");
      }

    private:
      JsonMngMetaDataApi::Imp* m_imp = nullptr;
    };

    //////////////////////////////////////////////
    class VerifyMetaDataAll : public MetaDataMsg
    {
    public:
      VerifyMetaDataAll() = delete;
      VerifyMetaDataAll(const rapidjson::Document& doc)
        :MetaDataMsg(doc)
      {
      }

      virtual ~VerifyMetaDataAll()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        {
          Value arr;
          arr.SetArray();
          for (auto it : m_inconsistentMid) {
            Value item;
            item.SetString(it, doc.GetAllocator());
            arr.PushBack(item, doc.GetAllocator());
          }
          Pointer("/data/rsp/inconsistentMid").Set(doc, arr);
        }

        {
          Value arr;
          arr.SetArray();
          for (auto it : m_orphanedMid) {
            Value item;
            item.SetString(it, doc.GetAllocator());
            arr.PushBack(item, doc.GetAllocator());
          }
          Pointer("/data/rsp/orphanedMid").Set(doc, arr);
        }

        {
          Value arr;
          arr.SetArray();
          for (auto it : m_inconsistentMetaId) {
            Value item;
            item.SetString(it, doc.GetAllocator());
            arr.PushBack(item, doc.GetAllocator());
          }
          Pointer("/data/rsp/inconsistentMetaId").Set(doc, arr);
        }

        {
          Value arr;
          arr.SetArray();
          for (auto it : m_orphanedMetaId) {
            Value item;
            item.SetString(it, doc.GetAllocator());
            arr.PushBack(item, doc.GetAllocator());
          }
          Pointer("/data/rsp/orphanedMetaId").Set(doc, arr);
        }

        MetaDataMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonMngMetaDataApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        std::lock_guard<std::recursive_mutex> lock(imp->getMux());

        auto & nadrMidMap = imp->getNadrMidMap();
        auto & midMetaIdMap = imp->getMidMetaIdMap();
        auto & metaIdMetaDataMap = imp->getMetaIdMetaDataMap();

        //get inconsistentMid
        for (auto it = nadrMidMap.myBegin(); it != nadrMidMap.myEnd(); it++) {
          Imp::metaId metaId;
          metaId = midMetaIdMap.myFind(it->second, metaId);
          if (metaId.empty()) {
            m_inconsistentMid.push_back(it->second);
          }
        }

        //get orphanedMid
        for (auto it = midMetaIdMap.myBegin(); it != midMetaIdMap.myEnd(); it++) {
          if (!nadrMidMap.hasValue(it->first)) {
            m_orphanedMid.push_back(it->first);
          }
        }

        //get inconsistentMetaId
        for (auto it = midMetaIdMap.myBegin(); it != midMetaIdMap.myEnd(); it++) {
          auto found = metaIdMetaDataMap.find(it->second);
          if (found == metaIdMetaDataMap.end()) {
            m_inconsistentMetaId.push_back(it->second);
          }
        }

        //get orphanedMetaId
        for (auto it : metaIdMetaDataMap) {
          if (!midMetaIdMap.hasValue(it.first)) {
            m_orphanedMetaId.push_back(it.first);
          }
        }
        TRC_FUNCTION_LEAVE("");
      }

    private:
      std::vector<Imp::mid> m_inconsistentMid;
      std::vector<Imp::mid> m_orphanedMid;
      std::vector<Imp::mid> m_inconsistentMetaId;
      std::vector<Imp::mid> m_orphanedMetaId;
    };

    //////////////////////////////////////////////
    class GetMetaData : public MetaDataMsg
    {
    public:
      GetMetaData() = delete;
      GetMetaData(const rapidjson::Document& doc)
        :MetaDataMsg(doc)
      {
        m_metaId = Pointer("/data/req/metaId").Get(doc)->GetString();
        m_metaData.reset(shape_new Imp::MetaData());
      }

      virtual ~GetMetaData()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Pointer("/data/rsp/metaId").Set(doc, m_metaId);
        Pointer("/data/rsp/mid").Set(doc, m_mid);
        Pointer("/data/rsp/metaData").Set(doc, m_metaData->getValue());
        MetaDataMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonMngMetaDataApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        if (m_metaId.empty()) { // bad param
          setErr(mngMetaDataMsgStatus::st_badParams);
        }
        else { //find MetaData
          auto md = imp->getMetaData(m_metaId);
          if (md) { //exists
            *m_metaData = *md;
          }
          else {
            setErr(mngMetaDataMsgStatus::st_metaIdUnknown);
          }
        }

        TRC_FUNCTION_LEAVE("");
      }

    private:
      Imp::mid m_mid;
      Imp::metaId m_metaId;
      std::shared_ptr<Imp::MetaData> m_metaData;
    };

    //////////////////////////////////////////////
    class SetMetaData : public MetaDataMsg
    {
    public:
      SetMetaData() = delete;
      SetMetaData(const rapidjson::Document& doc)
        :MetaDataMsg(doc)
      {
        m_metaId = Pointer("/data/req/metaId").Get(doc)->GetString();
        m_metaData.reset(shape_new Imp::MetaData());
        m_metaData->setValue(*(Pointer("/data/req/metaData").Get(doc)));
      }

      virtual ~SetMetaData()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Pointer("/data/rsp/metaId").Set(doc, m_metaId);
        Pointer("/data/rsp/mid").Set(doc, m_mid);
        Pointer("/data/rsp/metaData").Set(doc, m_metaData->getValue());
        MetaDataMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonMngMetaDataApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        std::lock_guard<std::recursive_mutex> lock(imp->getMux());

        if (m_metaId.empty()) { // add MetaData
          if (!m_metaData->empty()) {
            while (true) {
              // get random  ID
              int val = imp->getRandom();
              std::ostringstream os;
              os << val;
              m_metaId = os.str();
              auto found = imp->getMetaIdMetaDataMap().find(m_metaId);
              if (imp->getMetaIdMetaDataMap().end() == found)
                break; //unique for sure
            }
            imp->getMetaIdMetaDataMap()[m_metaId] = m_metaData;
            //update file
            imp->updateMetaData();
          }
          else {
            setErr(mngMetaDataMsgStatus::st_badParams);
          }
        }
        else { //update MetaData
          auto md = imp->getMetaData(m_metaId);
          if (md) { //exists - update
            if (!m_metaData->empty()) { //not empty - update
              *md = *m_metaData;
              //update file
              imp->updateMetaData();
            }
            else { //emty - erase
              if (imp->getMidMetaIdMap().hasValue(m_metaId)) {
                //cannot erase as it is assigned to mid 
                setErr(mngMetaDataMsgStatus::st_metaIdAssigned);
              }
              else {
                imp->getMetaIdMetaDataMap().erase(m_metaId);
                //update file
                imp->updateMetaData();
              }
            }
          }
          else {
            setErr(mngMetaDataMsgStatus::st_metaIdUnknown);
          }
        }

        TRC_FUNCTION_LEAVE("");
      }

    private:
      Imp::mid m_mid;
      Imp::metaId m_metaId;
      std::shared_ptr<Imp::MetaData> m_metaData;
    };

    //////////////////////////////////////////////
    class GetMidMetaId : public MetaDataMsg
    {
    public:
      GetMidMetaId() = delete;
      GetMidMetaId(const rapidjson::Document& doc)
        :MetaDataMsg(doc)
      {
        m_mid = Pointer("/data/req/mid").Get(doc)->GetString();
        m_metaData.reset(shape_new Imp::MetaData());
      }

      virtual ~GetMidMetaId()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Pointer("/data/rsp/mid").Set(doc, m_mid);
        Pointer("/data/rsp/metaId").Set(doc, m_metaId);
        Pointer("/data/rsp/metaData").Set(doc, m_metaData->getValue());
        MetaDataMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonMngMetaDataApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        std::lock_guard<std::recursive_mutex> lock(imp->getMux());

        if (!m_mid.empty()) {
          m_metaId = imp->getMidMetaIdMap().myFind(m_mid, m_metaId);
          if (!m_metaId.empty()) {
            auto md = imp->getMetaData(m_metaId);
            if (md) { //exists
              *m_metaData = *md;
            }
            else {
              setErr(mngMetaDataMsgStatus::st_metaIdInconsistent);
            }
          }
          else {
            //mid not found
            setErr(mngMetaDataMsgStatus::st_midUnknown);
          }
        }
        else {
          setErr(mngMetaDataMsgStatus::st_badParams);
        }

        TRC_FUNCTION_LEAVE("");
      }

    private:
      Imp::mid m_mid;
      Imp::metaId m_metaId;
      std::shared_ptr<Imp::MetaData> m_metaData;
    };

    //////////////////////////////////////////////
    class SetMidMetaId : public MetaDataMsg
    {
    public:
      SetMidMetaId() = delete;
      SetMidMetaId(const rapidjson::Document& doc)
        :MetaDataMsg(doc)
      {
        m_mid = Pointer("/data/req/mid").Get(doc)->GetString();
        m_metaId = Pointer("/data/req/metaId").Get(doc)->GetString();
      }

      virtual ~SetMidMetaId()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Pointer("/data/rsp/mid").Set(doc, m_mid);
        Pointer("/data/rsp/metaId").Set(doc, m_metaId);
        Pointer("/data/rsp/metaIdMid").Set(doc, m_metaIdMid);
        MetaDataMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonMngMetaDataApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");

        std::lock_guard<std::recursive_mutex> lock(imp->getMux());

        if (!m_mid.empty()) {
          if (!m_metaId.empty()) {
            auto md = imp->getMetaData(m_metaId);
            if (md) {
              //insert
              auto res = imp->getMidMetaIdMap().myInsert(m_mid, m_metaId);
              switch (res) {
              case uniqueMidMetaIdMap::success:
                //update file
                imp->updateMetaData();
                m_metaIdMid = m_mid;
                break;
              case uniqueMidMetaIdMap::duplicit_key:
                setErr(mngMetaDataMsgStatus::st_midAssigned);
                m_metaIdMid = m_mid;
                break;
              case uniqueMidMetaIdMap::duplicit_val:
                setErr(mngMetaDataMsgStatus::st_metaIdAssigned);
                m_metaIdMid = imp->getMidMetaIdMap().myFindKey(m_metaId, m_metaIdMid);
                break;
              case uniqueMidMetaIdMap::failure:
              default: //ok
                setErr(mngMetaDataMsgStatus::st_badParams);
              }
            }
            else {
              setErr(mngMetaDataMsgStatus::st_metaIdUnknown);
            }
          }
          else {
            //erase
            imp->getMidMetaIdMap().myErase(m_mid);
            //update file
            imp->updateMetaData();
          }
        }
        else {
          setErr(mngMetaDataMsgStatus::st_badParams);
        }

        TRC_FUNCTION_LEAVE("");
      }

    private:
      Imp::mid m_mid;
      Imp::metaId m_metaId;
      Imp::mid m_metaIdMid;
    };

    //////////////////////////////////////////////
    class GetNadrMetaData : public MetaDataMsg
    {
    public:
      GetNadrMetaData() = delete;
      GetNadrMetaData(const rapidjson::Document& doc)
        :MetaDataMsg(doc)
      {
        m_nadr = Pointer("/data/req/nAdr").Get(doc)->GetInt();
        m_metaData.reset(shape_new Imp::MetaData());
      }

      virtual ~GetNadrMetaData()
      {
      }

      void createResponsePayload(rapidjson::Document& doc) override
      {
        Pointer("/data/rsp/nAdr").Set(doc, m_nadr);
        Pointer("/data/rsp/mid").Set(doc, m_mid);
        Pointer("/data/rsp/metaId").Set(doc, m_metaId);
        Pointer("/data/rsp/metaData").Set(doc, m_metaData->getValue());

        MetaDataMsg::createResponsePayload(doc);
      }

      void handleMsg(JsonMngMetaDataApi::Imp* imp) override
      {
        TRC_FUNCTION_ENTER("");
        
        std::lock_guard<std::recursive_mutex> lock(imp->getMux());

        m_mid = imp->getNadrMidMap().myFind(m_nadr, m_mid);
        if (!m_mid.empty()) {
          m_metaId = imp->getMidMetaIdMap().myFind(m_mid, m_metaId);
          if (!m_metaId.empty()) {
            auto md = imp->getMetaData(m_metaId);
            if (md) {
              m_metaData = md;
            }
            else {
              setErr(mngMetaDataMsgStatus::st_metaIdInconsistent);
            }
          }
          else {
            setErr(mngMetaDataMsgStatus::st_midInconsistent);
          }
        }
        else {
          setErr(mngMetaDataMsgStatus::st_nadrUnknown);
        }

        TRC_FUNCTION_LEAVE("");
      }

    private:
      Imp::nadr m_nadr = nadrEmpty;
      Imp::mid m_mid;
      Imp::metaId m_metaId;
      std::shared_ptr<Imp::MetaData> m_metaData;
    };

    ///////////////////////////
    std::vector<std::string> m_filters =
    {
      "mngMetaData"
    };

    shape::ILaunchService* m_iLaunchService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    
    bool m_metaDataToMessages = false;
    std::string m_cacheDir;
    std::string m_metaDataFile;
    std::string m_schemaMetaDataFile;
    std::unique_ptr<SchemaDocument> m_schemaPtr;

    ObjectFactory<MetaDataMsg, rapidjson::Document&> m_objectFactory;

  public:
    Imp()
    {
      m_objectFactory.registerClass <ExportMetaDataAll>(mType_ExportMetaDataAll);
      m_objectFactory.registerClass <ExportNadrMidMap>(mType_ExportNadrMidMap);
      m_objectFactory.registerClass <GetNadrMetaData>(mType_GetNadrMetaData);
      m_objectFactory.registerClass <ImportMetaDataAll>(mType_ImportMetaDataAll);
      m_objectFactory.registerClass <ImportNadrMidMap>(mType_ImportNadrMidMap);
      m_objectFactory.registerClass <VerifyMetaDataAll>(mType_VerifyMetaDataAll);
      m_objectFactory.registerClass <GetMetaData>(mType_GetMetaData);
      m_objectFactory.registerClass <GetMidMetaId>(mType_GetMidMetaId);
      m_objectFactory.registerClass <SetMetaData>(mType_SetMetaData);
      m_objectFactory.registerClass <SetMidMetaId>(mType_SetMidMetaId);
    }

    ~Imp()
    {
    }

    void handleMsg(const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document reqDoc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      Document respDoc;
      std::unique_ptr<MetaDataMsg> msg = m_objectFactory.createObject(msgType.m_type, reqDoc);

      msg->handleMsg(this);
      msg->createResponse(respDoc);
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(respDoc));

      TRC_FUNCTION_LEAVE("");
    }

    std::shared_ptr<MetaData> getMetaData(metaId k)
    {
      std::shared_ptr<MetaData> retval;
      auto found = m_metaIdMetaDataMap.find(k);
      if (found != m_metaIdMetaDataMap.end()) {
        retval = found->second;
      }
      return retval;
    }

    void loadMetaData()
    {
      TRC_FUNCTION_ENTER("");

      std::lock_guard<std::recursive_mutex> lock(getMux());

      std::ifstream ifs(m_metaDataFile);
      if (!ifs.is_open()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot open: " << PAR(m_metaDataFile));
      }

      Document doc;
      rapidjson::IStreamWrapper isw(ifs);
      doc.ParseStream(isw);
      if (doc.HasParseError()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Json parse error: " << NAME_PAR(emsg, doc.GetParseError()) <<
          NAME_PAR(eoffset, doc.GetErrorOffset()));
      }

      SchemaValidator validator(*m_schemaPtr);

      if (!doc.Accept(validator)) {
        // Input JSON is invalid according to the schema
        StringBuffer sb;
        std::string schema, keyword, document;
        validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
        schema = sb.GetString();
        keyword = validator.GetInvalidSchemaKeyword();
        sb.Clear();
        validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
        document = sb.GetString();
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid: " << PAR(schema) << PAR(keyword));
      }
      TRC_INFORMATION("MetaDataFile successfully validated.")

      const Value *val;
      
      //load madrMidMap
      val = Pointer("/nadrMidMap").Get(doc);
      for (auto it = val->Begin(); it != val->End(); it++) {
        int nadr = Pointer("/nAdr").Get(*it)->GetInt();
        std::string mid = Pointer("/mid").Get(*it)->GetString();
        int res = m_nadrMidMap.myInsert(nadr, mid);
        switch (res) {
        case uniqueMidMetaIdMap::success:
          break; //ok
        case uniqueMidMetaIdMap::duplicit_key:
          TRC_WARNING("Duplicit nadr:" << PAR(nadr) << PAR(mid));
          break;
        case uniqueMidMetaIdMap::duplicit_val:
          TRC_WARNING("Duplicit mid:" << PAR(nadr) << PAR(mid));
          break;
        case uniqueMidMetaIdMap::failure:
        default:
          TRC_WARNING("Other insert error:" << PAR(nadr) << PAR(mid));
        }
      }

      //process midMetaIdMap
      val = Pointer("/midMetaIdMap").Get(doc);
      for (auto it = val->Begin(); it != val->End(); it++) {
        std::string mid = Pointer("/mid").Get(*it)->GetString();
        std::string metaId = Pointer("/metaId").Get(*it)->GetString();
        int res = m_midMetaIdMap.myInsert(mid, metaId);
        switch (res) {
        case uniqueMidMetaIdMap::success:
          break; //ok
        case uniqueMidMetaIdMap::duplicit_key:
          TRC_WARNING("Duplicit nadr:" << PAR(mid) << PAR(metaId));
          break;
        case uniqueMidMetaIdMap::duplicit_val:
          TRC_WARNING("Duplicit mid:" << PAR(mid) << PAR(metaId));
          break;
        case uniqueMidMetaIdMap::failure:
        default:
          TRC_WARNING("Other insert error:" << PAR(mid) << PAR(metaId));
        }
      }

      //load metaIdMetaDataMap
      val = Pointer("/metaIdMetaDataMap").Get(doc);
      for (auto it = val->Begin(); it != val->End(); it++) {
        std::string metaId = Pointer("/metaId").Get(*it)->GetString();
        std::shared_ptr<Imp::MetaData> metaData(shape_new Imp::MetaData());
        metaData->setValue(*(Pointer("/metaData").Get(*it)));
        auto res = m_metaIdMetaDataMap.insert(std::make_pair(metaId, metaData));
        if (!res.second) {
          TRC_WARNING("Duplicit nadr:" << PAR(metaId));
        }
      }

      TRC_FUNCTION_LEAVE("")
    }

    void updateMetaData()
    {
      TRC_FUNCTION_ENTER("");

      Document doc;
      
      std::lock_guard<std::recursive_mutex> lock(getMux());

      { // update nadrMidMap
        Value arr;
        arr.SetArray();
        for (auto it = m_nadrMidMap.myBegin(); it != m_nadrMidMap.myEnd(); it++) {
          Value item;
          Pointer("/nAdr").Set(item, it->first, doc.GetAllocator());
          Pointer("/mid").Set(item, it->second, doc.GetAllocator());
          arr.PushBack(item, doc.GetAllocator());
        }
        Pointer("/nadrMidMap").Set(doc, arr);
      }
      
      { // update midMetaIdMap
        Value arr;
        arr.SetArray();
        for (auto it = m_midMetaIdMap.myBegin(); it != m_midMetaIdMap.myEnd(); it++) {
          Value item;
          Pointer("/mid").Set(item, it->first, doc.GetAllocator());
          Pointer("/metaId").Set(item, it->second, doc.GetAllocator());
          arr.PushBack(item, doc.GetAllocator());
        }
        Pointer("/midMetaIdMap").Set(doc, arr);
      }

      { // update metaIdMetaDataMap
        Value arr;
        arr.SetArray();
        for (auto it = m_metaIdMetaDataMap.begin(); it != m_metaIdMetaDataMap.end(); it++) {
          Value item;
          Pointer("/metaId").Set(item, it->first, doc.GetAllocator());
          Pointer("/metaData").Set(item, it->second->getValue(), doc.GetAllocator());
          arr.PushBack(item, doc.GetAllocator());
        }
        Pointer("/metaIdMetaDataMap").Set(doc, arr);
      }

      std::ofstream ofs(m_metaDataFile);
      OStreamWrapper osw(ofs);
      PrettyWriter<OStreamWrapper> writer(osw);
      doc.Accept(writer);

      TRC_FUNCTION_LEAVE("")
    }

    std::recursive_mutex& getMux()
    {
      return m_mux;
    }

    uniqueMidMetaIdMap& getMidMetaIdMap()
    {
      return m_midMetaIdMap;
    }

    uniqueNadrMidMap& getNadrMidMap()
    {
      return m_nadrMidMap;
    }

    metaIdMetaDataMap& getMetaIdMetaDataMap()
    {
      return m_metaIdMetaDataMap;
    }

    bool iSmetaDataToMessages() const
    {
      return m_metaDataToMessages;
    }

    rapidjson::Document getMetaDataImpl(uint16_t nAdr)
    {
      TRC_FUNCTION_ENTER("");

      std::lock_guard<std::recursive_mutex> lock(m_mux);

      rapidjson::Document retval;
      retval.SetObject();
      std::string lmid = getNadrMidMap().myFind(nAdr, lmid);
      if (!lmid.empty()) {
        std::string lmetaId = getMidMetaIdMap().myFind(lmid, lmetaId);
        if (!lmetaId.empty()) {
          auto md = getMetaData(lmetaId);
          if (md) {
            retval.CopyFrom(md->getValue(), retval.GetAllocator());
          }
        }
      }
      
      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    ///////////////////

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonMngMetaDataApi instance activate" << std::endl <<
        "******************************"
      );

      props->getMemberAsBool("metaDataToMessages", m_metaDataToMessages);

      m_cacheDir = m_iLaunchService->getCacheDir();
      m_cacheDir += "/metaData";
      m_metaDataFile = m_cacheDir;
      m_metaDataFile += "/meta_data.json";
      m_schemaMetaDataFile = m_cacheDir;
      m_schemaMetaDataFile += "/schema_meta_data.json";
      TRC_INFORMATION("Using: " << PAR(m_cacheDir) << PAR(m_metaDataFile) << PAR(m_schemaMetaDataFile));

      std::ifstream ifs(m_schemaMetaDataFile);
      if (!ifs.is_open()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot open: " << PAR(m_schemaMetaDataFile));
      }

      Document sd;
      rapidjson::IStreamWrapper isw(ifs);
      sd.ParseStream(isw);
      if (sd.HasParseError()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Json parse error: " << NAME_PAR(emsg, sd.GetParseError()) <<
          NAME_PAR(eoffset, sd.GetErrorOffset()));
      }

      m_schemaPtr.reset(shape_new SchemaDocument(sd));

      loadMetaData();

      m_iMessagingSplitterService->registerFilteredMsgHandler(m_filters,
        [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
      {
        handleMsg(messagingId, msgType, std::move(doc));
      });

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonMngMetaDataApi instance deactivate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      props->getMemberAsBool("metaDataToMessages", m_metaDataToMessages);
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

    void attachInterface(IMessagingSplitterService* iface)
    {
      m_iMessagingSplitterService = iface;
    }

    void detachInterface(IMessagingSplitterService* iface)
    {
      if (m_iMessagingSplitterService == iface) {
        m_iMessagingSplitterService = nullptr;
      }
    }

  };

  /////////////////////////
  JsonMngMetaDataApi::JsonMngMetaDataApi()
  {
    m_imp = shape_new Imp();
  }

  JsonMngMetaDataApi::~JsonMngMetaDataApi()
  {
    delete m_imp;
  }

  void JsonMngMetaDataApi::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonMngMetaDataApi::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonMngMetaDataApi::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  bool JsonMngMetaDataApi::iSmetaDataToMessages() const
  {
    return m_imp->iSmetaDataToMessages();
  }

  rapidjson::Document JsonMngMetaDataApi::getMetaData(uint16_t nAdr) const
  {
    return m_imp->getMetaDataImpl(nAdr);
  }

  void JsonMngMetaDataApi::attachInterface(shape::ILaunchService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonMngMetaDataApi::detachInterface(shape::ILaunchService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonMngMetaDataApi::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonMngMetaDataApi::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonMngMetaDataApi::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonMngMetaDataApi::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
