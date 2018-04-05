#pragma once

#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "IMessagingService.h"
#include "ITraceService.h"
#include <map>

namespace iqrf {
  class JsonSplitter : public IMessagingSplitterService
  {
  public:
    JsonSplitter();
    virtual ~JsonSplitter();
    void sendMessage(const std::string& messagingId, rapidjson::Document doc) const override;
    void registerFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters, FilteredMessageHandlerFunc handlerFunc) override;
    void unregisterFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters) override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IMessagingService* iface);
    void detachInterface(iqrf::IMessagingService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp = nullptr;
  };
}
