#pragma once

#include "ShapeDefines.h"
#include <string>
#include "BondResult.h"

#ifdef ILocalBondService_EXPORTS
#define ILocalBondService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define ILocalBondService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  /// \class ILocalBondService
  class ILocalBondService_DECLSPEC ILocalBondService
  {
  public:
    virtual ~ILocalBondService() {}

    /// \brief Bonds node at the coordinator side to specified address and
    /// returns result of the bonding.
    /// \param [in] nodeAddr     a requested address for the bonded node
    /// \return  result of bonding 
    /// \details
    /// A requested address must NOT be used (bonded yet). If this parameter
    /// equals to 0, then the 1st free address is assigned to the node.
    virtual BondResult bondNode(const uint16_t nodeAddr) = 0;
  };
}
