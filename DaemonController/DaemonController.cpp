#define IDaemonControllerService_EXPORTS

#include "DaemonController.h"
#include "Trace.h"

#include "iqrfgw__DaemonController.hxx"

TRC_INIT_MODULE(iqrfgw::DaemonController);

namespace iqrfgw {
  DaemonController::DaemonController()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  DaemonController::~DaemonController()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  std::string DaemonController::doService(const std::string & str) const
  {
    TRC_FUNCTION_ENTER(PAR(str));
    std::string revStr(str);
    std::reverse(revStr.begin(), revStr.end());
    TRC_FUNCTION_LEAVE(PAR(revStr));
    return revStr;
  }

  void DaemonController::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "DaemonController instance activate" << std::endl <<
      "******************************"
    );
    TRC_FUNCTION_LEAVE("")
  }

  void DaemonController::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "DaemonController instance deactivate" << std::endl <<
      "******************************"
    );
    TRC_FUNCTION_LEAVE("")
  }

  void DaemonController::modify(const shape::Properties *props)
  {
  }

  void DaemonController::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void DaemonController::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
