/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
"use strict";
namespace('iqrf.daemon');

////////////////////////
// Coordinator
////////////////////////
if (iqrf.embed.coordinator !== undefined) {

  iqrf.embed.coordinator.AddrInfo_Request_req = function (param) {
    return iqrf.embed.coordinator.AddrInfo_Request();
  };

  iqrf.embed.coordinator.AddrInfo_Response_rsp = function (rawHdp) {
    return iqrf.embed.coordinator.AddrInfo_Response(rawHdp);
  };

  iqrf.embed.coordinator.DiscoveredDevices_Request_req = function (param) {
    return iqrf.embed.coordinator.DiscoveredDevices_Request();
  };

  iqrf.embed.coordinator.DiscoveredDevices_Response_rsp = function (rawHdp) {
    var result =
    {
      discoveredDevices: iqrf.embed.coordinator.DiscoveredDevices_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.coordinator.BondedDevices_Request_req = function (param) {
    return iqrf.embed.coordinator.BondedDevices_Request();
  };

  iqrf.embed.coordinator.BondedDevices_Response_rsp = function (rawHdp) {
    var result =
    {
      bondedDevices: iqrf.embed.coordinator.BondedDevices_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.coordinator.ClearAllBonds_Request_req = function (param) {
    return iqrf.embed.coordinator.ClearAllBonds_Request();
  };

  iqrf.embed.coordinator.ClearAllBonds_Response_rsp = function (rawHdp) {
    iqrf.embed.coordinator.ClearAllBonds_Response(rawHdp);
    return {};
  };

  iqrf.embed.coordinator.BondNode_Request_req = function (param) {
    return iqrf.embed.coordinator.BondNode_Request(param.reqAddr, param.bondingTestRetries);
  };

  iqrf.embed.coordinator.BondNode_Response_rsp = function (rawHdp) {
    return iqrf.embed.coordinator.BondNode_Response(rawHdp);
  };

  iqrf.embed.coordinator.RemoveBond_Request_req = function (param) {
    return iqrf.embed.coordinator.RemoveBond_Request(param.bondAddr);
  };

  iqrf.embed.coordinator.RemoveBond_Response_rsp = function (rawHdp) {
    var result =
    {
      devNr: iqrf.embed.coordinator.RemoveBond_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.coordinator.RebondNode_Request_req = function (param) {
    return iqrf.embed.coordinator.RebondNode_Request(param.bondAddr);
  };

  iqrf.embed.coordinator.RebondNode_Response_rsp = function (rawHdp) {
    var result =
    {
      devNr: iqrf.embed.coordinator.RebondNode_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.coordinator.Discovery_Request_req = function (param) {
    return iqrf.embed.coordinator.Discovery_Request(param.txPower, param.maxAddr);
  };

  iqrf.embed.coordinator.Discovery_Response_rsp = function (rawHdp) {
    var result =
    {
      discNr: iqrf.embed.coordinator.Discovery_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.coordinator.SetDpaParams_Request_req = function (param) {
    return iqrf.embed.coordinator.SetDpaParams_Request(param.dpaParam);
  };

  iqrf.embed.coordinator.SetDpaParams_Response_rsp = function (rawHdp){
    return {
        dpaParam: iqrf.embed.coordinator.SetDpaParams_Response(rawHdp)
    };
  };

  iqrf.embed.coordinator.SetHops_Request_req = function (param) {
    return iqrf.embed.coordinator.SetHops_Request(param.requestHops, param.responseHops);
  };

  iqrf.embed.coordinator.SetHops_Response_rsp = function (rawHdp) {
    return iqrf.embed.coordinator.SetHops_Response(rawHdp);
  };

  iqrf.embed.coordinator.DiscoveryData_Request_req = function (param) {
    return iqrf.embed.coordinator.DiscoveryData_Request(param.address);
  };

  iqrf.embed.coordinator.DiscoveryData_Response_rsp = function (rawHdp) {
    var result =
    {
      discoveryData: iqrf.embed.coordinator.DiscoveryData_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.coordinator.Backup_Request_req = function (param) {
    return iqrf.embed.coordinator.Backup_Request(param.index);
  };

  iqrf.embed.coordinator.Backup_Response_rsp = function (rawHdp) {
    var result =
    {
      networkData: iqrf.embed.coordinator.Backup_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.coordinator.Restore_Request_req = function (param) {
    return iqrf.embed.coordinator.Restore_Request(param.netData);
  };

  iqrf.embed.coordinator.Restore_Response_rsp = function (rawHdp) {
    iqrf.embed.coordinator.Restore_Response(rawHdp);
    return {};
  };

  iqrf.embed.coordinator.AuthorizeBond_Request_req = function (param) {
    if (param.nodes !== undefined) {
      return iqrf.embed.coordinator.AuthorizeBond_Request(param.nodes);
    } else {
      return iqrf.embed.coordinator.AuthorizeBond_Request([{
        reqAddr: param.reqAddr,
        mid: param.mid
      }]);
    }
  };

  iqrf.embed.coordinator.AuthorizeBond_Response_rsp = function (rawHdp) {
    return iqrf.embed.coordinator.AuthorizeBond_Response(rawHdp);
  };

  iqrf.embed.coordinator.ReadRemotelyBondedMid_Request_req = function (param) {
    return iqrf.embed.coordinator.ReadRemotelyBondedMid_Request();
  };

  iqrf.embed.coordinator.ReadRemotelyBondedMid_Response_rsp = function (rawHdp) {
    var result =
    {
      prebondedNodes: iqrf.embed.coordinator.ReadRemotelyBondedMid_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.coordinator.ClearRemotelyBondedMid_Request_req = function (param) {
    return iqrf.embed.coordinator.ClearRemotelyBondedMid_Request();
  };

  iqrf.embed.coordinator.ClearRemotelyBondedMid_Response_rsp = function (rawHdp) {
    iqrf.embed.coordinator.ClearRemotelyBondedMid_Response(rawHdp);
    return {};
  };

  iqrf.embed.coordinator.EnableRemoteBonding_Request_req = function (param) {
    return iqrf.embed.coordinator.EnableRemoteBonding_Request(param.bondingMask, param.control, param.userData);
  };

  iqrf.embed.coordinator.EnableRemoteBonding_Response_rsp = function (rawHdp) {
    iqrf.embed.coordinator.EnableRemoteBonding_Response(rawHdp);
    return {};
  };

  iqrf.embed.coordinator.SmartConnect_Request_req = function (param) {
    return iqrf.embed.coordinator.SmartConnect_Request(param.reqAddr, param.bondingTestRetries, param.ibk, param.mid, param.virtualDeviceAddress, param.userData);
  };

  iqrf.embed.coordinator.SmartConnect_Response_rsp = function (rawHdp) {
    return iqrf.embed.coordinator.SmartConnect_Response(rawHdp);
  };

  iqrf.embed.coordinator.SetMID_Request_req = function (param) {
    return iqrf.embed.coordinator.SetMID_Request(param.bondAddr, param.mid)
  };

  iqrf.embed.coordinator.SetMID_Response_rsp = function (rawHdp) {
    iqrf.embed.coordinator.SetMID_Response(rawHdp);
    return {};
  };

}

////////////////////////
// IO
////////////////////////
if (iqrf.embed.io !== undefined) {
  iqrf.embed.io.Direction_Request_req = function (param) {
    return iqrf.embed.io.Direction_Request(param.subcommands);
  };

  iqrf.embed.io.Direction_Response_rsp = function (rawHdp) {
    iqrf.embed.io.Direction_Response(rawHdp);
    return {};
  };

  iqrf.embed.io.Get_Request_req = function (param) {
    return iqrf.embed.io.Get_Request(param.ports);
  };

  iqrf.embed.io.Get_Response_rsp = function (rawHdp) {
    var result =
    {
      ports: iqrf.embed.io.Get_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.io.Set_Request_req = function (param) {
    return iqrf.embed.io.Set_Request(param.subcommands);
  };

  iqrf.embed.io.Set_Response_rsp = function (rawHdp) {
    iqrf.embed.io.Set_Response(rawHdp);
    return {};
  };
}
////////////////////////
// OS
////////////////////////
if (iqrf.embed.os !== undefined) {
  iqrf.embed.os.Read_Request_req = function (param) {
    return iqrf.embed.os.Read_Request();
  };

  iqrf.embed.os.Read_Response_rsp = function (rawHdp) {
    return iqrf.embed.os.Read_Response(rawHdp);
  };

  iqrf.embed.os.Reset_Request_req = function (param) {
    return iqrf.embed.os.Reset_Request();
  };

  iqrf.embed.os.Reset_Response_rsp = function (rawHdp) {
    iqrf.embed.os.Reset_Response(rawHdp);
    return {};
  };

  iqrf.embed.os.ReadCfg_Request_req = function (param) {
    return iqrf.embed.os.ReadCfg_Request();
  };

  iqrf.embed.os.ReadCfg_Response_rsp = function (rawHdp) {
    return iqrf.embed.os.ReadCfg_Response(rawHdp);
  };

  iqrf.embed.os.Rfpgm_Request_req = function (param) {
    return iqrf.embed.os.Rfpgm_Request();
  };

  iqrf.embed.os.Rfpgm_Response_rsp = function (rawHdp) {
    iqrf.embed.os.Rfpgm_Response(rawHdp);
    return {};
  };

  iqrf.embed.os.Sleep_Request_req = function (param) {
    return iqrf.embed.os.Sleep_Request(param.time, param.control);
  };

  iqrf.embed.os.Sleep_Response_rsp = function (rawHdp) {
    iqrf.embed.os.Sleep_Response(rawHdp);
    return {};
  };

  iqrf.embed.os.Batch_Request_req = function (param) {
    return iqrf.embed.os.Batch_Request(param.requests);
  };

  iqrf.embed.os.Batch_Response_rsp = function (rawHdp) {
    iqrf.embed.os.Batch_Response(rawHdp);
    return {};
  };

  iqrf.embed.os.SetSecurity_Request_req = function (param) {
    return iqrf.embed.os.SetSecurity_Request(param.type, param.data);
  };

  iqrf.embed.os.SetSecurity_Response_rsp = function (rawHdp) {
    iqrf.embed.os.SetSecurity_Response(rawHdp);
    return {};
  };

  iqrf.embed.os.Restart_Request_req = function (param) {
    return iqrf.embed.os.Restart_Request();
  };

  iqrf.embed.os.Restart_Response_rsp = function (rawHdp) {
    iqrf.embed.os.Restart_Response(rawHdp);
    return {};
  };

  iqrf.embed.os.WriteCfgByte_Request_req = function (param) {
    return iqrf.embed.os.WriteCfgByte_Request(param.bytes);
  };

  iqrf.embed.os.WriteCfgByte_Response_rsp = function (rawHdp) {
    iqrf.embed.os.WriteCfgByte_Response(rawHdp);
    return {};
  };

  iqrf.embed.os.LoadCode_Request_req = function (param) {
    return iqrf.embed.os.LoadCode_Request(param.flags, param.address, param.length, param.checkSum);
  };

  iqrf.embed.os.LoadCode_Response_rsp = function (rawHdp) {
    iqrf.embed.os.LoadCode_Response(rawHdp);
    var result =
    {
      loadingCode: iqrf.embed.os.LoadCode_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.os.SelectiveBatch_Request_req = function (param) {
    return iqrf.embed.os.SelectiveBatch_Request(param.selectedNodes, param.requests);
  };

  iqrf.embed.os.SelectiveBatch_Response_rsp = function (rawHdp) {
    iqrf.embed.os.SelectiveBatch_Response(rawHdp);
    return {};
  };

  iqrf.embed.os.WriteCfg_Request_req = function (param) {
    return iqrf.embed.os.WriteCfg_Request(param.configuration, param.rfpgm);
  };

  iqrf.embed.os.WriteCfg_Response_rsp = function (rawHdp) {
    iqrf.embed.os.WriteCfg_Response(rawHdp);
    return {};
  };

  iqrf.embed.os.TestRfSignal_Request_req = function (param) {
    return iqrf.embed.os.TestRfSignal_Request(param.channel, param.rxFilter, param.time);
  };

  iqrf.embed.os.TestRfSignal_Response_rsp = function (rawHdp) {
    iqrf.embed.os.TestRfSignal_Response(rawHdp);
    var result =
    {
      counter: iqrf.embed.os.TestRfSignal_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.os.FactorySettings_Request_req = function (param) {
    return iqrf.embed.os.FactorySettings_Request();
  };

  iqrf.embed.os.FactorySettings_Response_rsp = function (rawHdp) {
    iqrf.embed.os.FactorySettings_Response(rawHdp);
    return {};
  };

  iqrf.embed.os.Indicate_Request_req = function (param) {
    return iqrf.embed.os.Indicate_Request(param.control);
  };

  iqrf.embed.os.Indicate_Response_rsp = function (rawHdp) {
    iqrf.embed.os.Indicate_Response(rawHdp);
    return {};
  };
}

////////////////////////
// EEPROM
////////////////////////
if (iqrf.embed.eeprom !== undefined) {
  iqrf.embed.eeprom.Read_Request_req = function (param) {
    return iqrf.embed.eeprom.Read_Request(param.address, param.length)
  };

  iqrf.embed.eeprom.Read_Response_rsp = function (rawHdp) {
    var result =
    {
      pData: iqrf.embed.eeprom.Read_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.eeprom.Write_Request_req = function (param) {
    return iqrf.embed.eeprom.Write_Request(param.address, param.pData)
  };

  iqrf.embed.eeprom.Write_Response_rsp = function (rawHdp) {
    iqrf.embed.eeprom.Write_Response(rawHdp)
    return {};
  };
}
////////////////////////
// EEEPROM
////////////////////////
if (iqrf.embed.eeeprom !== undefined) {
  iqrf.embed.eeeprom.Read_Request_req = function (param) {
    return iqrf.embed.eeeprom.Read_Request(param.address, param.length)
  };

  iqrf.embed.eeeprom.Read_Response_rsp = function (rawHdp) {
    var result =
    {
      pData: iqrf.embed.eeeprom.Read_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.eeeprom.Write_Request_req = function (param) {
    return iqrf.embed.eeeprom.Write_Request(param.address, param.pData)
  };

  iqrf.embed.eeeprom.Write_Response_rsp = function (rawHdp) {
    iqrf.embed.eeeprom.Write_Response(rawHdp)
    return {};
  };
}
////////////////////////
// RAM
////////////////////////
if (iqrf.embed.ram !== undefined) {
  iqrf.embed.ram.Read_Request_req = function (param) {
    return iqrf.embed.ram.Read_Request(param.address, param.len)
  };

  iqrf.embed.ram.Read_Response_rsp = function (rawHdp) {
    var result =
    {
      pData: iqrf.embed.ram.Read_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.ram.Write_Request_req = function (param) {
    return iqrf.embed.ram.Write_Request(param.address, param.pData)
  };

  iqrf.embed.ram.Write_Response_rsp = function (rawHdp) {
    iqrf.embed.ram.Write_Response(rawHdp)
    return {};
  };
}
////////////////////////
// Thermometer
////////////////////////
if (iqrf.embed.thermometer !== undefined) {
  iqrf.embed.thermometer.Read_Request_req = function (param) {
    return iqrf.embed.thermometer.Read_Request();
  };

  iqrf.embed.thermometer.Read_Response_rsp = function (rawHdp) {
    var result =
    {
      temperature: iqrf.embed.thermometer.Read_Response(rawHdp)
    };
    return result;
  };
}
////////////////////////
// SPI
////////////////////////
if (iqrf.embed.spi !== undefined) {
  iqrf.embed.spi.WriteRead_Request_req = function (param) {
    return iqrf.embed.spi.WriteRead_Request(param.readTimeout, param.writtenData);
  };

  iqrf.embed.spi.WriteRead_Response_rsp = function (rawHdp) {
    var result =
    {
      readData: iqrf.embed.spi.WriteRead_Response(rawHdp)
    };
    return result;
  };
}
////////////////////////
// UART
////////////////////////
if (iqrf.embed.uart !== undefined) {
  iqrf.embed.uart.WriteRead_Request_req = function (param) {
    return iqrf.embed.uart.WriteRead_Request(param.readTimeout, param.writtenData);
  };

  iqrf.embed.uart.WriteRead_Response_rsp = function (rawHdp) {
    var result =
    {
      readData: iqrf.embed.uart.WriteRead_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.uart.Open_Request_req = function (param) {
    return iqrf.embed.uart.Open_Request(param.baudRate);
  };

  iqrf.embed.uart.Open_Response_rsp = function (rawHdp) {
    iqrf.embed.uart.Open_Response(rawHdp);
    return {};
  };

  iqrf.embed.uart.Close_Request_req = function (param) {
    return iqrf.embed.uart.Close_Request();
  };

  iqrf.embed.uart.Close_Response_rsp = function (rawHdp) {
    iqrf.embed.uart.Close_Response(rawHdp);
    return {};
  };

  iqrf.embed.uart.ClearWriteRead_Request_req = function (param) {
    return iqrf.embed.uart.ClearWriteRead_Request(param.readTimeout, param.writtenData);
  };

  iqrf.embed.uart.ClearWriteRead_Response_rsp = function (rawHdp) {
    var result =
    {
      readData: iqrf.embed.uart.ClearWriteRead_Response(rawHdp)
    };
    return result;
  };
}
////////////////////////
// LEDR
////////////////////////
if (iqrf.embed.ledr !== undefined) {
  iqrf.embed.ledr.Set_Request_req = function (param) {
    return iqrf.embed.ledr.Set_Request(param.onOff);
  };

  iqrf.embed.ledr.Set_Response_rsp = function (rawHdp) {
    iqrf.embed.ledr.Set_Response(rawHdp);
    return {};
  };

  iqrf.embed.ledr.SetOn_Request_req = function () {
    return iqrf.embed.ledr.Set_Request(true);
  }

  iqrf.embed.ledr.SetOn_Response_rsp = function(rawHdp) {
    iqrf.embed.ledr.Set_Response(rawHdp);
    return {};
  }

  iqrf.embed.ledr.SetOff_Request_req = function () {
    return iqrf.embed.ledr.Set_Request(false);
  }

  iqrf.embed.ledr.SetOff_Response_rsp = function(rawHdp) {
    iqrf.embed.ledr.Set_Response(rawHdp);
    return {};
  }

  iqrf.embed.ledr.Get_Request_req = function (param) {
    return iqrf.embed.ledr.Get_Request();
  };

  iqrf.embed.ledr.Get_Response_rsp = function (rawHdp) {
    var result =
    {
      onOff: iqrf.embed.ledr.Get_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.ledr.Pulse_Request_req = function (param) {
    return iqrf.embed.ledr.Pulse_Request();
  };

  iqrf.embed.ledr.Pulse_Response_rsp = function (rawHdp) {
    iqrf.embed.ledr.Pulse_Response(rawHdp);
    return {};
  };

  iqrf.embed.ledr.Flashing_Request_req = function (param) {
    return iqrf.embed.ledr.Flashing_Request();
  };

  iqrf.embed.ledr.Flashing_Response_rsp = function (rawHdp) {
    iqrf.embed.ledr.Flashing_Response(rawHdp);
    return {};
  };
}
////////////////////////
// LEDG
////////////////////////
if (iqrf.embed.ledg !== undefined) {
  iqrf.embed.ledg.Set_Request_req = function (param) {
    return iqrf.embed.ledg.Set_Request(param.onOff);
  };

  iqrf.embed.ledg.Set_Response_rsp = function (rawHdp) {
    iqrf.embed.ledg.Set_Response(rawHdp);
    return {};
  };

  iqrf.embed.ledg.SetOn_Request_req = function () {
    return iqrf.embed.ledg.SetOn_Request(true);
  }

  iqrf.embed.ledg.SetOn_Response_rsp = function(rawHdp) {
    iqrf.embed.ledg.SetOn_Response(rawHdp);
    return {};
  }

  iqrf.embed.ledg.SetOff_Request_req = function () {
    return iqrf.embed.ledg.SetOff_Request(false);
  }

  iqrf.embed.ledg.SetOff_Response_rsp = function(rawHdp) {
    iqrf.embed.ledg.SetOff_Response(rawHdp);
    return {};
  }

  iqrf.embed.ledg.Get_Request_req = function (param) {
    return iqrf.embed.ledg.Get_Request();
  };

  iqrf.embed.ledg.Get_Response_rsp = function (rawHdp) {
    var result =
    {
      onOff: iqrf.embed.ledg.Get_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.ledg.Pulse_Request_req = function (param) {
    return iqrf.embed.ledg.Pulse_Request();
  };

  iqrf.embed.ledg.Pulse_Response_rsp = function (rawHdp) {
    iqrf.embed.ledg.Pulse_Response(rawHdp);
    return {};
  };

  iqrf.embed.ledg.Flashing_Request_req = function (param) {
    return iqrf.embed.ledg.Flashing_Request();
  };

  iqrf.embed.ledg.Flashing_Response_rsp = function (rawHdp) {
    iqrf.embed.ledg.Flashing_Response(rawHdp);
    return {};
  };
}
////////////////////////
// Node
////////////////////////
if (iqrf.embed.node !== undefined) {
  iqrf.embed.node.Backup_Request_req = function (param) {
    return iqrf.embed.node.Backup_Request(param.index);
  };

  iqrf.embed.node.Backup_Response_rsp = function (rawHdp) {
    var result =
    {
      networkData: iqrf.embed.node.Backup_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.node.ClearRemotelyBondedMid_Request_req = function (param) {
    return iqrf.embed.node.ClearRemotelyBondedMid_Request();
  };

  iqrf.embed.node.ClearRemotelyBondedMid_Response_rsp = function (rawHdp) {
    iqrf.embed.node.ClearRemotelyBondedMid_Response(rawHdp);
    return {};
  };

  iqrf.embed.node.EnableRemoteBonding_Request_req = function (param) {
    return iqrf.embed.node.EnableRemoteBonding_Request(param.bondingMask, param.control, param.userData);
  };

  iqrf.embed.node.EnableRemoteBonding_Response_rsp = function (rawHdp) {
    iqrf.embed.node.EnableRemoteBonding_Response(rawHdp);
    return {};
  };

  iqrf.embed.node.Read_Request_req = function (param) {
    return iqrf.embed.node.Read_Request();
  };

  iqrf.embed.node.Read_Response_rsp = function (rawHdp) {
    return iqrf.embed.node.Read_Response(rawHdp)
  };

  iqrf.embed.node.ReadRemotelyBondedMid_Request_req = function (param) {
    return iqrf.embed.node.ReadRemotelyBondedMid_Request();
  };

  iqrf.embed.node.ReadRemotelyBondedMid_Response_rsp = function (rawHdp) {
    var result =
    {
      prebondedNodes: iqrf.embed.node.ReadRemotelyBondedMid_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.node.RemoveBond_Request_req = function (param) {
    return iqrf.embed.node.RemoveBond_Request();
  };

  iqrf.embed.node.RemoveBond_Response_rsp = function (rawHdp) {
    iqrf.embed.node.RemoveBond_Response(rawHdp);
    return {};
  };

  iqrf.embed.node.Restore_Request_req = function (param) {
    return iqrf.embed.node.Restore_Request(param.backupData);
  };

  iqrf.embed.node.Restore_Response_rsp = function (rawHdp) {
    iqrf.embed.node.Restore_Response(rawHdp);
    return {};
  };

  iqrf.embed.node.ValidateBonds_Request_req = function (param) {
    return iqrf.embed.node.ValidateBonds_Request(param.nodes);
  };

  iqrf.embed.node.ValidateBonds_Response_rsp = function (rawHdp) {
    iqrf.embed.node.ValidateBonds_Response(rawHdp);
    return {};
  };
}
////////////////////////
// FRC
////////////////////////
if (iqrf.embed.frc !== undefined) {
  iqrf.embed.frc.Send_Request_req = function (param) {
    return iqrf.embed.frc.Send_Request(param.frcCommand, param.userData);
  };

  iqrf.embed.frc.Send_Response_rsp = function (rawHdp) {
    return iqrf.embed.frc.Send_Response(rawHdp);
  };

  iqrf.embed.frc.ExtraResult_Request_req = function (param) {
    return iqrf.embed.frc.ExtraResult_Request();
  };

  iqrf.embed.frc.ExtraResult_Response_rsp = function (rawHdp) {
    var result =
    {
      frcData: iqrf.embed.frc.ExtraResult_Response(rawHdp)
    };
    return result;
  };

  iqrf.embed.frc.SendSelective_Request_req = function (param) {
    return iqrf.embed.frc.SendSelective_Request(param.frcCommand, param.selectedNodes, param.userData);
  };

  iqrf.embed.frc.SendSelective_Response_rsp = function (rawHdp) {
    return iqrf.embed.frc.SendSelective_Response(rawHdp);
  };

  iqrf.embed.frc.SetParams_Request_req = function (param) {
    return iqrf.embed.frc.SetParams_Request(param.frcResponseTime);
  };

  iqrf.embed.frc.SetParams_Response_rsp = function (rawHdp) {
    var result =
    {
      frcResponseTime: iqrf.embed.frc.SetParams_Response(rawHdp)
    };
    return result;
  };
}
////////////////////////
// IqrfStandardBinaryOutput
////////////////////////
if (iqrf.binaryoutput !== undefined) {

  iqrf.binaryoutput.Enumerate_Request_req = function (param) {
    return iqrf.binaryoutput.Enumerate_Request();
  };

  iqrf.binaryoutput.Enumerate_Response_rsp = function (rawhdp) {
    var result =
    {
      binOuts: iqrf.binaryoutput.Enumerate_Response(rawhdp)
    };
    return result;
  };

  iqrf.binaryoutput.SetOutput_Request_req = function (param) {
    return iqrf.binaryoutput.SetOutput_Request(param.binOuts);
  };

  iqrf.binaryoutput.SetOutput_Response_rsp = function (rawhdp) {
    var result =
    {
      prevVals: iqrf.binaryoutput.SetOutput_Response(rawhdp)
    };
    return result;
  };
}

////////////////////////
// IqrfStandardLight
////////////////////////
if (iqrf.light !== undefined) {

  // NEW API

  iqrf.light.SendLdiCommands_Request_req = function (params) {
    return iqrf.light.SendLdiCommands_Request(params.commands);
  };

  iqrf.light.SendLdiCommands_Response_rsp = function (rawHdp) {
    return {
      answers: iqrf.light.SendLdiCommands_Response(rawHdp)
    };
  };

  iqrf.light.SendLdiCommandsAsync_Request_req = function (params) {
    return iqrf.light.SendLdiCommandsAsync_Request(params.commands);
  };

  iqrf.light.SendLdiCommandsAsync_Response_rsp = function (rawHdp) {
    iqrf.light.SendLdiCommandsAsync_Response(rawHdp);
    return {};
  };

  iqrf.light.SetLai_Request_req = function (params) {
    return iqrf.light.SetLai_Request(params.voltage * 1000);
  }

  iqrf.light.SetLai_Response_rsp = function (rawHdp) {
    return {
      prevVoltage: iqrf.light.SetLai_Response(rawHdp) / 1000,
    };
  }

  iqrf.light.FrcLdiSend_Request_req = function (params) {
    return {
      retpars: iqrf.light.FrcLdiSend_Request(params.command, params.selectedNodes)
    };
  };

  iqrf.light.FrcLdiSend_Response_rsp = function (params) {
    return {
      answers: iqrf.light.FrcLdiSend_Response(params.responseFrcSend, params.responseFrcExtraResult)
    };
  };

  iqrf.light.FrcLaiRead_Request_req = function (params) {
    return {
      retpars: iqrf.light.FrcLaiRead_Request(params.selectedNodes)
    };
  }

  iqrf.light.FrcLaiRead_Response_rsp = function (params) {
    return {
      voltages: iqrf.light.FrcLaiRead_Response(params.responseFrcSend, params.responseFrcExtraResult)
    }
  }

  // DEPRECATED API

  iqrf.light.Enumerate_Request_req = function (param) {
    return iqrf.light.Enumerate_Request();
  };

  iqrf.light.Enumerate_Response_rsp = function (rawHdp) {
    var result =
    {
      lights: iqrf.light.Enumerate_Response(rawHdp)
    };
    return result;
  };

  iqrf.light.SetPower_Request_req = function (param) {
    return iqrf.light.SetPower_Request(param.lights);
  };

  iqrf.light.SetPower_Response_rsp = function (rawHdp) {
    var result =
    {
      prevVals: iqrf.light.SetPower_Response(rawHdp)
    };
    return result;
  };

  iqrf.light.IncrementPower_Request_req = function (param) {
    return iqrf.light.IncrementPower_Request(param.lights);
  };

  iqrf.light.IncrementPower_Response_rsp = function (rawHdp) {
    var result =
    {
      prevVals: iqrf.light.IncrementPower_Response(rawHdp)
    };
    return result;
  };

  iqrf.light.DecrementPower_Request_req = function (param) {
    return iqrf.light.DecrementPower_Request(param.lights);
  };

  iqrf.light.DecrementPower_Response_rsp = function (rawHdp) {
    var result =
    {
      prevVals: iqrf.light.DecrementPower_Response(rawHdp)
    };
    return result;
  };
}

////////////////////////
// IqrfStandardSensor
////////////////////////
if (iqrf.sensor !== undefined) {

  iqrf.sensor.Enumerate_Request_req = function (param) {
    return iqrf.sensor.Enumerate_Request();
  };

  iqrf.sensor.Enumerate_Response_rsp = function (rawHdp) {
    var result =
    {
      sensors: iqrf.sensor.Enumerate_Response(rawHdp)
    };
    return result;
  };

  iqrf.sensor.ReadSensorsWithTypes_Request_req = function (param) {
    return iqrf.sensor.ReadSensorsWithTypes_Request(param.sensorIndexes, param.writtenData);
  };

  iqrf.sensor.ReadSensorsWithTypes_Response_rsp = function (rawHdp) {
    var result =
    {
      sensors: iqrf.sensor.ReadSensorsWithTypes_Response(rawHdp, rawHdp.originalRequest)
    };
    return result;
  };

  iqrf.sensor.Frc_Request_req = function (param) {
    var result =
    {
      retpars: iqrf.sensor.Frc_Request(param.sensorType, param.sensorIndex, param.frcCommand, param.selectedNodes, param.sleepAfterFrc)
    };
    return result;
  };

  //special handling with state info because of parameter per to be passed
  iqrf.sensor.Frc_Response_rsp = function (param) {
    var result =
    {
      sensors: iqrf.sensor.Frc_Response(param.sensorType, param.frcCommand, param.responseFrcSend, param.responseFrcExtraResult, param.frcSendRequest)
    };
    return result;
  };
}

///////////////////
// IqrfStandardDALI // DEPRECATED
///////////////////
if (iqrf.dali !== undefined) {

  iqrf.dali.SendCommands_Request_req = function (param) {
    return iqrf.dali.SendCommands_Request(param.commands);
  };

  iqrf.dali.SendCommands_Response_rsp = function (rawHdp) {
    var result =
    {
      answers: iqrf.dali.SendCommands_Response(rawHdp)
    };
    return result;
  };

  iqrf.dali.SendCommandsAsync_Request_req = function (param) {
    return iqrf.dali.SendCommandsAsync_Request(param.commands);
  };

  iqrf.dali.SendCommandsAsync_Response_rsp = function (rawHdp) {
    iqrf.dali.SendCommandsAsync_Response(rawHdp)
    return {};
  };

  iqrf.dali.Frc_Request_req = function (param) {
    var result =
    {
      retpars: iqrf.dali.Frc_Request(param.command, param.selectedNodes)
    };
    return result;
  };

  iqrf.dali.Frc_Response_rsp = function (param) {
    var result =
    {
      answers: iqrf.dali.Frc_Response(param.responseFrcSend, param.responseFrcExtraResult)
    };
    return result;
  };
}

////////////////////////
// EXPLORE
////////////////////////
if (iqrf.embed.explore !== undefined) {
  iqrf.embed.explore.Enumerate_Request_req = function (param) {
    return iqrf.embed.explore.Enumerate_Request();
  };

  iqrf.embed.explore.Enumerate_Response_rsp = function (rawHdp) {
    var result = iqrf.embed.explore.Enumerate_Response(rawHdp);

    // Rename perNr to userPerNr for compliance with DPA
    if (result.hasOwnProperty('perNr')) {
        Object.defineProperty(result, 'userPerNr', Object.getOwnPropertyDescriptor(result, 'perNr'));
        delete result['perNr'];
    }

    return result;
  };

  iqrf.embed.explore.PeripheralInformation_Request_req = function (param) {
    return iqrf.embed.explore.PeripheralInformation_Request(param.per);
  };

  iqrf.embed.explore.PeripheralInformation_Response_rsp = function (rawHdp) {
    var result = iqrf.embed.explore.PeripheralInformation_Response(parseInt(rawHdp.pnum, 16), rawHdp);
    return result;
  };

  iqrf.embed.explore.MorePeripheralsInformation_Request_req = function (param) {
    return iqrf.embed.explore.MorePeripheralsInformation_Request(param.per);
  };

  iqrf.embed.explore.MorePeripheralsInformation_Response_rsp = function (rawHdp) {
    var result =
    {
      peripherals: iqrf.embed.explore.MorePeripheralsInformation_Response(rawHdp.pcmd, rawHdp)
    };
    return result;
  };
}
