#pragma once
#include <cstdint>

namespace iqrf {

  /// \class BondResult
  /// \brief Result of bonding of a node.
  class BondResult {
  public:
    /// Type of result
    enum class Type {
      NoError,
      AlreadyUsed,
      NoFreeSpace,
      AboveAddressLimit,
      InternalCallFail,
      PingFailed
    };

    /// \brief Constructor
    /// \param [in] type              type of result
    /// \param [in] bondedNodeAddr    address of newly bonded node
    /// \param [in] bondedNodesNum    number of bonded nodes
    BondResult(Type type, uint16_t bondedNodeAddr, uint16_t bondedNodesNum);

    /// \brief Returns type of bonding result.
    /// \return Type of bonding result
    Type getType() const { return m_type; };

    /// \brief Returns address of the newly bonded node.
    /// \return address of the newly bonded node
    /// \details
    /// Returned value is valid only if bonding was successfully processed,
    /// i.e. the result type is NoError. Otherwise the returned value is 0.
    uint16_t getBondedAddr() const { return m_bondedAddr; };

    /// \brief Returns number of bonded network nodes.
    /// \return number of bonded network nodes
    /// \details
    /// Returned value is valid only if bonding was successfully processed,
    /// i.e. the result type is NoError. Otherwise the returned value is 0.
    uint16_t getBondedNodesNum() const { return m_bondedNodesNum; };


  private:
    Type m_type;
    uint16_t m_bondedAddr;
    uint16_t m_bondedNodesNum;
  };
}
