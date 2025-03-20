/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>

namespace iqrf::message_types {

  /// @brief Embedded peripherals and standards
  enum class Peripherals : uint8_t {
    Coordinator,
    Node,
    Os,
    Eeprom,
    Eeeprom,
    Ram,
    Ledr,
    Ledg,
    Spi,
    Io,
    Thermometer,
    Uart = 0x0C,
    Frc,
    Light = 0x4A,
    BinaryOutput,
    Sensor = 0x5E,
    Exploration = 0xFF,
  };

  /// @brief Peripheral commands
  enum class Commands : uint8_t {
    Coordinator_AddrInfo = 0x00,
    Coordinator_DiscoveredDevices,
    Coordinator_BondedDevices,
    Coordinator_ClearAllBonds,
    Coordinator_BondNode,
    Coordinator_RemoveNode,
    Coordinator_Discovery = 0x07,
    Coordinator_SetDpaParams,
    Coordinator_SetHops,
    Coordinator_Backup = 0x0B,
    Coordinator_Restore,
    Coordinator_AuthorizeBond,
    Coordinator_SmartConnect = 0x12,
    Coordinator_SetMid,
    Node_Read = 0x00,
    Node_RemoveBond,
    Node_Backup = 0x06,
    Node_Restore,
    Node_ValidateBonds,
    Os_Read = 0x00,
    Os_Reset,
    Os_ReadCfg,
    Os_Rfpgm,
    Os_Sleep,
    Os_Batch,
    Os_SetSecurity,
    Os_Indicate,
    Os_Restart,
    Os_WriteCfgByte,
    Os_LoadCode,
    Os_SelectiveBatch,
    Os_TestRfSignal,
    Os_FactorySettings,
    Os_WriteCfg = 0x0F,
    Eeprom_Read = 0x00,
    Eeprom_Write,
    Eeeprom_Read = 0x02,
    Eeeprom_Write,
    Ram_Read = 0x00,
    Ram_Write,
    Ram_ReadAny = 0x05,
    Led_SetOff = 0x00,
    Led_SetOn,
    Led_Pulse = 0x03,
    Led_Flashing,
    Spi_WriteRead = 0x00,
    Io_Direction = 0x00,
    Io_Set,
    Io_Get,
    Thermometer_Read = 0x00,
    Uart_Open = 0x00,
    Uart_Close,
    Uart_WriteRead,
    Uart_ClearWriteRead,
    Frc_Send = 0x00,
    Frc_ExtraResult,
    Frc_SendSelective,
    Frc_SetParams,
    Light_SendLdiCommands = 0x00,
    Light_SendLdiCommandsAsync,
    Light_SetLai,
    BinaryOutput_SetOutput = 0x00,
    Sensor_ReadSensors = 0x00,
    Sensor_ReadSensorsWithTypes,
    Exploration_PerEnum = 0x3F,
    Exploration_PerInfo = 0x3F,
    Standard_Enumerate = 0x3E,
  };

  typedef std::tuple<Peripherals, Commands> PerCommandTuple;

  /// @brief Message type converter utility class
  class MessageTypeConverter {
  public:

    /**
     * Convert message type to periperal and command tuple
     * @param mType Message type
     * @return `PerCommandTuple` Peripheral and command tuple
     */
    static PerCommandTuple messageTypeToPerCmd(const std::string &mType) {
      if (mType == "iqrfEmbedExplore_Enumerate") { // Exploration
        return {Peripherals::Exploration, Commands::Exploration_PerEnum};
      } else if (mType == "iqrfEmbedCoordinator_AddrInfo") { // Coordinator
        return {Peripherals::Coordinator, Commands::Coordinator_AddrInfo};
      } else if (mType == "iqrfEmbedCoordinator_DiscoveredDevices") {
        return {Peripherals::Coordinator, Commands::Coordinator_DiscoveredDevices};
      } else if (mType == "iqrfEmbedCoordinator_BondedDevices") {
        return {Peripherals::Coordinator, Commands::Coordinator_BondedDevices};
      } else if (mType == "iqrfEmbedCoordinator_ClearAllBonds") {
        return {Peripherals::Coordinator, Commands::Coordinator_ClearAllBonds};
      } else if (mType == "iqrfEmbedCoordinator_BondNode") {
        return {Peripherals::Coordinator, Commands::Coordinator_BondNode};
      } else if (mType == "iqrfEmbedCoordinator_RemoveBond") {
        return {Peripherals::Coordinator, Commands::Coordinator_RemoveNode};
      } else if (mType == "iqrfEmbedCoordinator_Discovery") {
        return {Peripherals::Coordinator, Commands::Coordinator_Discovery};
      } else if (mType == "iqrfEmbedCoordinator_SetDpaParams") {
        return {Peripherals::Coordinator, Commands::Coordinator_SetDpaParams};
      } else if (mType == "iqrfEmbedCoordinator_SetHops") {
        return {Peripherals::Coordinator, Commands::Coordinator_SetHops};
      } else if (mType == "iqrfEmbedCoordinator_Backup") {
        return {Peripherals::Coordinator, Commands::Coordinator_Backup};
      } else if (mType == "iqrfEmbedCoordinator_Restore") {
        return {Peripherals::Coordinator, Commands::Coordinator_Restore};
      } else if (mType == "iqrfEmbedCoordinator_AuthorizeBond") {
        return {Peripherals::Coordinator, Commands::Coordinator_AuthorizeBond};
      } else if (mType == "iqrfEmbedCoordinator_SmartConnect") {
        return {Peripherals::Coordinator, Commands::Coordinator_SmartConnect};
      } else if (mType == "iqrfEmbedCoordinator_SetMID") {
        return {Peripherals::Coordinator, Commands::Coordinator_SetMid};
      } else if (mType == "iqrfEmbedNode_Read") { // Node
        return {Peripherals::Node, Commands::Node_Read};
      } else if (mType == "iqrfEmbedNode_RemoveBond") {
        return {Peripherals::Node, Commands::Node_RemoveBond};
      } else if (mType == "iqrfEmbedNode_Backup") {
        return {Peripherals::Node, Commands::Node_Backup};
      } else if (mType == "iqrfEmbedNode_Restore") {
        return {Peripherals::Node, Commands::Node_Restore};
      } else if (mType == "iqrfEmbedNode_ValidateBonds") {
        return {Peripherals::Node, Commands::Node_ValidateBonds};
      } else if (mType == "iqrfEmbedOs_Read") { // Os
        return {Peripherals::Os, Commands::Os_Read};
      } else if (mType == "iqrfEmbedOs_Reset") {
        return {Peripherals::Os, Commands::Os_Reset};
      } else if (mType == "iqrfEmbedOs_ReadCfg") {
        return {Peripherals::Os, Commands::Os_ReadCfg};
      } else if (mType == "iqrfEmbedOs_Rfpgm") {
        return {Peripherals::Os, Commands::Os_Rfpgm};
      } else if (mType == "iqrfEmbedOs_Sleep") {
        return {Peripherals::Os, Commands::Os_Sleep};
      } else if (mType == "iqrfEmbedOs_Batch") {
        return {Peripherals::Os, Commands::Os_Batch};
      } else if (mType == "iqrfEmbedOs_SetSecurity") {
        return {Peripherals::Os, Commands::Os_SetSecurity};
      } else if (mType == "iqrfEmbedOs_Indicate") {
        return {Peripherals::Os, Commands::Os_Indicate};
      } else if (mType == "iqrfEmbedOs_Restart") {
        return {Peripherals::Os, Commands::Os_Restart};
      } else if (mType == "iqrfEmbedOs_WriteCfgByte") {
        return {Peripherals::Os, Commands::Os_WriteCfgByte};
      } else if (mType == "iqrfEmbedOs_LoadCode") {
        return {Peripherals::Os, Commands::Os_LoadCode};
      } else if (mType == "iqrfEmbedOs_SelectiveBatch") {
        return {Peripherals::Os, Commands::Os_SelectiveBatch};
      } else if (mType == "iqrfEmbedOs_TestRfSignal") {
        return {Peripherals::Os, Commands::Os_TestRfSignal};
      } else if (mType == "iqrfEmbedOs_FactorySettings") {
        return {Peripherals::Os, Commands::Os_FactorySettings};
      } else if (mType == "iqrfEmbedOs_WriteCfg") {
        return {Peripherals::Os, Commands::Os_WriteCfg};
      } else if (mType == "iqrfEmbedEeprom_Read") { // Eeprom
        return {Peripherals::Eeprom, Commands::Eeprom_Read};
      } else if (mType == "iqrfEmbedEeprom_Write") {
        return {Peripherals::Eeprom, Commands::Eeprom_Write};
      } else if (mType == "iqrfEmbedEeeprom_Read") { // Eeeprom
        return {Peripherals::Eeeprom, Commands::Eeeprom_Read};
      } else if (mType == "iqrfEmbedEeeprom_Write") {
        return {Peripherals::Eeeprom, Commands::Eeeprom_Write};
      } else if (mType == "iqrfEmbedRam_Read") { // Ram
        return {Peripherals::Ram, Commands::Ram_Read};
      } else if (mType == "iqrfEmbedRam_Write") {
        return {Peripherals::Ram, Commands::Ram_Write};
      } else if (mType == "iqrfEmbedLedr_SetOff") { // Ledr
        return {Peripherals::Ledr, Commands::Led_SetOff};
      } else if (mType == "iqrfEmbedLedr_SetOn") {
        return {Peripherals::Ledr, Commands::Led_SetOn};
      } else if (mType == "iqrfEmbedLedr_Pulse") {
        return {Peripherals::Ledr, Commands::Led_Pulse};
      } else if (mType == "iqrfEmbedLedr_Flashing") {
        return {Peripherals::Ledr, Commands::Led_Flashing};
      } else if (mType == "iqrfEmbedLedg_SetOff") { // Ledg
        return {Peripherals::Ledg, Commands::Led_SetOff};
      } else if (mType == "iqrfEmbedLedg_SetOn") {
        return {Peripherals::Ledg, Commands::Led_SetOn};
      } else if (mType == "iqrfEmbedLedg_Pulse") {
        return {Peripherals::Ledg, Commands::Led_Pulse};
      } else if (mType == "iqrfEmbedLedg_Flashing") {
        return {Peripherals::Ledg, Commands::Led_Flashing};
      } else if (mType == "iqrfEmbedSpi_WriteRead") { // Spi
        return {Peripherals::Spi, Commands::Spi_WriteRead};
      } else if (mType == "iqrfEmbedIo_Direction") { // Io
        return {Peripherals::Io, Commands::Io_Direction};
      } else if (mType == "iqrfEmbedIo_Set") {
        return {Peripherals::Io, Commands::Io_Set};
      } else if (mType == "iqrfEmbedIo_Get") {
        return {Peripherals::Io, Commands::Io_Get};
      } else if (mType == "iqrfEmbedThermometer_Read") { // Thermometer
        return {Peripherals::Thermometer, Commands::Thermometer_Read};
      } else if (mType == "iqrfEmbedUart_Open") { // Uart
        return {Peripherals::Uart, Commands::Uart_Open};
      } else if (mType == "iqrfEmbedUart_Close") {
        return {Peripherals::Uart, Commands::Uart_Close};
      } else if (mType == "iqrfEmbedUart_WriteRead") {
        return {Peripherals::Uart, Commands::Uart_WriteRead};
      } else if (mType == "iqrfEmbedUart_ClearWriteRead") {
        return {Peripherals::Uart, Commands::Uart_ClearWriteRead};
      } else if (mType == "iqrfEmbedFrc_Send") { // Frc
        return {Peripherals::Frc, Commands::Frc_Send};
      } else if (mType == "iqrfEmbedFrc_ExtraResult") {
        return {Peripherals::Frc, Commands::Frc_ExtraResult};
      } else if (mType == "iqrfEmbedFrc_SendSelective") {
        return {Peripherals::Frc, Commands::Frc_SendSelective};
      } else if (mType == "iqrfEmbedFrc_SetParams") {
        return {Peripherals::Frc, Commands::Frc_SetParams};
      } else if (mType == "iqrfDali_SendCommands") { // old Dali standard, backwards compatibility
        return {Peripherals::Light, Commands::Light_SendLdiCommands};
      } else if (mType == "iqrfDali_SendCommandsAsync") {
        return {Peripherals::Light, Commands::Light_SendLdiCommandsAsync};
      } else if (mType == "iqrfLight_SendLdiCommands") { // Light standard
        return {Peripherals::Light, Commands::Light_SendLdiCommands};
      } else if (mType == "iqrfLight_SendLdiCommandsAsync") {
        return {Peripherals::Light, Commands::Light_SendLdiCommandsAsync};
      } else if (mType == "iqrfLight_SetLai") {
        return {Peripherals::Light, Commands::Light_SetLai};
      } else if (mType == "iqrfBinaryoutput_SetOutput") { // BinaryOutput standard
        return {Peripherals::BinaryOutput, Commands::BinaryOutput_SetOutput};
      } else if (mType == "iqrfBinaryoutput_Enumerate") {
        return {Peripherals::BinaryOutput, Commands::Standard_Enumerate};
      } else if (mType == "iqrfSensor_ReadSensorsWithTypes") { // Sensor standard
        return {Peripherals::Sensor, Commands::Sensor_ReadSensorsWithTypes};
      } else if (mType == "iqrfSensor_Enumerate") {
        return {Peripherals::Sensor, Commands::Standard_Enumerate};
      } else {
        throw std::domain_error("Unknown or unsupported message type.");
      }
    }

    /**
     * Check if message type is a Standard FRC message
     * @param mType Message type
     * @return true Message type is a Standard FRC message, false otherwise
     */
    static bool isFrcStandardMessageType(const std::string &mType) {
      return (mType == "iqrfDali_Frc" || mType == "iqrfLight_FrcLaiRead" || mType == "iqrfLight_FrcLdiSend" || mType == "iqrfSensor_Frc");
    }

    /**
     * Get FRC peripheral and command tuple depending on selectedNodes being used
     * @param selective Selected nodes used
     * @return PerCommandTuple Peripheral and command tuple
     */
    static PerCommandTuple frcMessageTypeToPerCmd(bool selective) {
      if (selective) {
        return {Peripherals::Frc, Commands::Frc_SendSelective};
      }
      return {Peripherals::Frc, Commands::Frc_Send};
    }
  };
}
