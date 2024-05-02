#include <cstdint>

namespace iqrf {

  class EnumDevice {
  public:
    bool discovered = false;
    uint32_t mid = 0;
    uint16_t hwpid = 0;
    uint16_t hwpidVer = 0;
  };
}
