#define IIqrfDpa_EXPORTS

#include "IqrfDpa.h"

#ifdef TRC_CHANNEL
#undefine TRC_CHANNEL
#endif
#define TRC_CHANNEL 1

#include "IIqrfDpa.h"
#include "Trace.h"

#include "iqrfgw__IqrfDpa.hxx"

TRC_INIT_MODULE(iqrfgw::IqrfDpa);

namespace iqrfgw {
  IqrfDpa::IqrfDpa()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  IqrfDpa::~IqrfDpa()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  std::string IqrfDpa::doService(const std::string & str) const
  {
    TRC_FUNCTION_ENTER(PAR(str));
    std::string revStr(str);
    std::reverse(revStr.begin(), revStr.end());
    TRC_FUNCTION_LEAVE(PAR(revStr));
    return revStr;
  }

  void IqrfDpa::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "IqrfDpa instance activate" << std::endl <<
      "******************************"
    );
    TRC_FUNCTION_LEAVE("")
  }

  void IqrfDpa::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "IqrfDpa instance deactivate" << std::endl <<
      "******************************"
    );
    TRC_FUNCTION_LEAVE("")
  }

  void IqrfDpa::modify(const shape::Properties *props)
  {
  }

  void IqrfDpa::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void IqrfDpa::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
