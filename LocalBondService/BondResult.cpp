#include "BondResult.h"

namespace iqrf {

  BondResult::BondResult(Type type, uint16_t bondedNodeAddr, uint16_t bondedNodesNum) {
    this->m_type = type;
    this->m_bondedAddr = bondedNodeAddr;
    this->m_bondedNodesNum = bondedNodesNum;

    if (type != Type::NoError) {
      this->m_bondedAddr = 0;
      this->m_bondedNodesNum = 0;
    }
  }
}
