#pragma once

#include "IMessagingSplitterService.h"
#include "IIqrfChannelService.h"
#include "IJsRenderService.h"
#include "ShapeProperties.h"
#include "ILaunchService.h"
#include "ITraceService.h"

namespace iqrf {

  class IIqrfDpaService;

  class TestJsonDpaApiIqrfStandard : public iqrf::IMessagingSplitterService, public iqrf::IIqrfChannelService
  {
  public:
    TestJsonDpaApiIqrfStandard();
    virtual ~TestJsonDpaApiIqrfStandard();

    //iqrf::IMessagingSplitterService
    void sendMessage(const std::string& messagingId, rapidjson::Document doc) const override;
    void registerFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters, FilteredMessageHandlerFunc handlerFunc) override;
    void unregisterFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters) override;

    //iqrf::IIqrfChannelService
    State getState() const override;
    std::unique_ptr<Accessor> getAccess(ReceiveFromFunc receiveFromFunc, AccesType access) override;
    bool hasExclusiveAccess() const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);
  };

}
