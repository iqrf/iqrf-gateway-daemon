"use strict";
namespace('iqrf.daemon');

////////////////////////
// Coordinator
////////////////////////
iqrf.embed.coordinator.AddrInfo_Request_req = function (param) {
    return iqrf.embed.coordinator.AddrInfo_Request()
};

iqrf.embed.coordinator.AddrInfo_Response_rsp = function (rawHdp) {
    var result = iqrf.embed.coordinator.AddrInfo_Response(rawHdp)
    return result
};

iqrf.embed.coordinator.DiscoveredDevices_Request_req = function (param) {
    return iqrf.embed.coordinator.DiscoveredDevices_Request()
};

iqrf.embed.coordinator.DiscoveredDevices_Response_rsp = function (rawHdp) {
    var result =
    {
        discoveredDevices: iqrf.embed.coordinator.DiscoveredDevices_Response(rawHdp)
    };
    return result;
};

iqrf.embed.coordinator.BondedDevices_Request_req = function (param) {
    return iqrf.embed.coordinator.BondedDevices_Request()
};

iqrf.embed.coordinator.BondedDevices_Response_rsp = function (rawHdp) {
    var result =
    {
        bondedDevices: iqrf.embed.coordinator.BondedDevices_Response(rawHdp)
    };
    return result;
};

iqrf.embed.coordinator.ClearAllBonds_Request_req = function (param) {
    return iqrf.embed.coordinator.ClearAllBonds_Request()
};

iqrf.embed.coordinator.ClearAllBonds_Response_rsp = function (rawHdp) {
    iqrf.embed.coordinator.ClearAllBonds_Response(rawHdp)
    var result =
    {
    };
    return result;
};

iqrf.embed.coordinator.BondNode_Request_req = function (param) {
    return iqrf.embed.coordinator.BondNode_Request(param.reqAddr, param.bondingMask)
};

iqrf.embed.coordinator.BondNode_Response_rsp = function (rawHdp) {
    var result = iqrf.embed.coordinator.BondNode_Response(rawHdp)
    return result;
};

iqrf.embed.coordinator.RemoveBond_Request_req = function (param) {
    return iqrf.embed.coordinator.RemoveBond_Request(param.bondAddr)
};

iqrf.embed.coordinator.RemoveBond_Response_rsp = function (rawHdp) {
    var result =
    {
        devNr: iqrf.embed.coordinator.RemoveBond_Response(rawHdp)
    };
    return result;
};

iqrf.embed.coordinator.RebondNode_Request_req = function (param) {
    return iqrf.embed.coordinator.RebondNode_Request(param.bondAddr)
};

iqrf.embed.coordinator.RebondNode_Response_rsp = function (rawHdp) {
    var result =
    {
        devNr: iqrf.embed.coordinator.RebondNode_Response(rawHdp)
    };
    return result;
};

iqrf.embed.coordinator.Discovery_Request_req = function (param) {
    return iqrf.embed.coordinator.Discovery_Request(param.txPower, param.maxAddr)
};

iqrf.embed.coordinator.Discovery_Response_rsp = function (rawHdp) {
    var result =
    {
        discNr: iqrf.embed.coordinator.Discovery_Response(rawHdp)
    };
    return result;
};

iqrf.embed.coordinator.SetDpaParams_Request_req = function (param) {
    return iqrf.embed.coordinator.SetDpaParams_Request(param.dpaParam)
};

iqrf.embed.coordinator.SetDpaParams_Response_rsp = function (rawHdp) {
    var result =
    {
        prevDpaParam: iqrf.embed.coordinator.SetDpaParams_Response(rawHdp)
    };
    return result;
};

iqrf.embed.coordinator.SetHops_Request_req = function (param) {
    return iqrf.embed.coordinator.SetHops_Request(param.requestHops, param.responseHops)
};

iqrf.embed.coordinator.SetHops_Response_rsp = function (rawHdp) {
    var result = iqrf.embed.coordinator.SetHops_Response(rawHdp)
    return result;
};

iqrf.embed.coordinator.DiscoveryData_Request_req = function (param) {
    return iqrf.embed.coordinator.DiscoveryData_Request(param.address)
};

iqrf.embed.coordinator.DiscoveryData_Response_rsp = function (rawHdp) {
    var result =
    {
        discoveryData: iqrf.embed.coordinator.DiscoveryData_Response(rawHdp)
    };
    return result;
};

iqrf.embed.coordinator.Backup_Request_req = function (param) {
    return iqrf.embed.coordinator.Backup_Request(param.index)
};

iqrf.embed.coordinator.Backup_Response_rsp = function (rawHdp) {
    var result =
    {
        networkData: iqrf.embed.coordinator.Backup_Response(rawHdp)
    };
    return result;
};

iqrf.embed.coordinator.Restore_Request_req = function (param) {
    return iqrf.embed.coordinator.Restore_Request(param.netData)
};

iqrf.embed.coordinator.Restore_Response_rsp = function (rawHdp) {
    iqrf.embed.coordinator.Restore_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.coordinator.AuthorizeBond_Request_req = function (param) {
    return iqrf.embed.coordinator.AuthorizeBond_Request(param.reqAddr, param.mid)
};

iqrf.embed.coordinator.AuthorizeBond_Response_rsp = function (rawHdp) {
    iqrf.embed.coordinator.AuthorizeBond_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.coordinator.ReadRemotelyBondedMid_Request_req = function (param) {
    return iqrf.embed.coordinator.ReadRemotelyBondedMid_Request()
};

iqrf.embed.coordinator.ReadRemotelyBondedMid_Response_rsp = function (rawHdp) {
    var result =
    {
        prebondedNodes: iqrf.embed.coordinator.ReadRemotelyBondedMid_Response(rawHdp)
    };
    return result;
};

iqrf.embed.coordinator.ClearRemotelyBondedMid_Request_req = function (param) {
    return iqrf.embed.coordinator.ClearRemotelyBondedMid_Request()
};

iqrf.embed.coordinator.ClearRemotelyBondedMid_Response_rsp = function (rawHdp) {
    iqrf.embed.coordinator.ClearRemotelyBondedMid_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.coordinator.EnableRemoteBonding_Request_req = function (param) {
    return iqrf.embed.coordinator.EnableRemoteBonding_Request(param.bondingMask, param.control, param.userData)
};

iqrf.embed.coordinator.EnableRemoteBonding_Response_rsp = function (rawHdp) {
    iqrf.embed.coordinator.EnableRemoteBonding_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.coordinator.SmartConnect_Request_req = function (param) {
    return iqrf.embed.coordinator.SmartConnect_Request(param.reqAddr, param.bondingTestRetries, param.ibk, param.mid, param.bondingChannel, param.virtualDeviceAddress, param.userData)
};

iqrf.embed.coordinator.SmartConnect_Response_rsp = function (rawHdp) {
    var result = iqrf.embed.coordinator.SmartConnect_Response(rawHdp);
    return result;
};

////////////////////////
// IO
////////////////////////
iqrf.embed.io.Direction_Request_req = function (param) {
    return iqrf.embed.io.Direction_Request(param.ports);
};

iqrf.embed.io.Direction_Response_rsp = function (rawHdp) {
    iqrf.embed.io.Direction_Response(rawHdp);
    var result =
    {
    };
    return result;
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
    return iqrf.embed.io.Set_Request(param.ports);
};

iqrf.embed.io.Set_Response_rsp = function (rawHdp) {
    iqrf.embed.io.Set_Response(rawHdp);
    var result =
    {
    };
    return result;
};

////////////////////////
// OS
////////////////////////
iqrf.embed.os.Read_Request_req = function (param) {
    return iqrf.embed.os.Read_Request();
};

iqrf.embed.os.Read_Response_rsp = function (rawHdp) {
    var result = iqrf.embed.os.Read_Response(rawHdp);
    return result;
};

iqrf.embed.os.Reset_Request_req = function (param) {
    return iqrf.embed.os.Reset_Request();
};

iqrf.embed.os.Reset_Response_rsp = function (rawHdp) {
    iqrf.embed.os.Reset_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.os.ReadCfg_Request_req = function (param) {
    return iqrf.embed.os.Read_Request();
};

iqrf.embed.os.ReadCfg_Response_rsp = function (rawHdp) {
    var result = iqrf.embed.os.Read_Response(rawHdp);
    return result;
};

iqrf.embed.os.Rfpgm_Request_req = function (param) {
    return iqrf.embed.os.Rfpgm_Request();
};

iqrf.embed.os.Rfpgm_Response_rsp = function (rawHdp) {
    iqrf.embed.os.Rfpgm_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.os.Sleep_Request_req = function (param) {
    return iqrf.embed.os.Sleep_Request(param.time, param.control);
};

iqrf.embed.os.Sleep_Response_rsp = function (rawHdp) {
    iqrf.embed.os.Sleep_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.os.Batch_Request_req = function (param) {
    return iqrf.embed.os.Batch_Request(param.requests);
};

iqrf.embed.os.Batch_Response_rsp = function (rawHdp) {
    iqrf.embed.os.Batch_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.os.SetSecurity_Request_req = function (param) {
    return iqrf.embed.os.SetSecurity_Request(param.type, param.data);
};

iqrf.embed.os.SetSecurity_Response_rsp = function (rawHdp) {
    iqrf.embed.os.SetSecurity_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.os.Restart_Request_req = function (param) {
    return iqrf.embed.os.Restart_Request();
};

iqrf.embed.os.Restart_Response_rsp = function (rawHdp) {
    iqrf.embed.os.Restart_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.os.WriteCfgByte_Request_req = function (param) {
    return iqrf.embed.os.WriteCfgByte_Request(param.cfgBytes);
};

iqrf.embed.os.WriteCfgByte_Response_rsp = function (rawHdp) {
    iqrf.embed.os.WriteCfgByte_Response(rawHdp);
    var result =
    {
    };
    return result;
};


iqrf.embed.os.LoadCode_Request_req = function (param) {
    return iqrf.embed.os.LoadCodeCfgByte_Request(param.flags, param.address, param.length, param.checkSum);
};

iqrf.embed.os.LoadCode_Response_rsp = function (rawHdp) {
    iqrf.embed.os.LoadCode_Response(rawHdp);
    var result =
    {
        lresult: iqrf.embed.os.LoadCode_Response(rawHdp)
    };
    return result;
};

iqrf.embed.os.SelectiveBatch_Request_req = function (param) {
    return iqrf.embed.os.SelectiveBatch_Request(param.selectedNodes, param.requests);
};

iqrf.embed.os.SelectiveBatch_Response_rsp = function (rawHdp) {
    iqrf.embed.os.SelectiveBatch_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.os.WriteCfg_Request_req = function (param) {
    return iqrf.embed.os.WriteCfg_Request(param.checkSum, param.cfgBytes, param.rfPgm);
};

iqrf.embed.os.WriteCfg_Response_rsp = function (rawHdp) {
    iqrf.embed.os.WriteCfg_Response(rawHdp);
    var result =
    {
    };
    return result;
};

////////////////////////
// EEPROM
////////////////////////
iqrf.embed.eeprom.Read_Request_req = function (param) {	
    return iqrf.embed.eeprom.Read_Request(param.address, param.len)
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
    var result = 
    {
    };
    return result;
};

////////////////////////
// EEEPROM
////////////////////////
iqrf.embed.eeeprom.Read_Request_req = function (param) {	
    return iqrf.embed.eeeprom.Read_Request(param.address, param.len)
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
    var result = 
    {
    };
    return result;
};

////////////////////////
// RAM
////////////////////////
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
    var result = 
    {
    };
    return result;
};

////////////////////////
// Thermometer
////////////////////////
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

////////////////////////
// SPI
////////////////////////
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

////////////////////////
// UART
////////////////////////
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
    var result =
    {
    };
    return result;
};

iqrf.embed.uart.Close_Request_req = function (param) {
    return iqrf.embed.uart.Close_Request();
};

iqrf.embed.uart.Close_Response_rsp = function (rawHdp) {
    iqrf.embed.uart.Close_Response(rawHdp);
    var result =
    {
    };
    return result;
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

////////////////////////
// LEDR
////////////////////////
iqrf.embed.ledr.Set_Request_req = function (param) {
    return iqrf.embed.ledr.Set_Request(param.onOff);
};

iqrf.embed.ledr.Set_Response_rsp = function (rawHdp) {
    iqrf.embed.ledr.Set_Response(rawHdp);
    var result =
    {
    };
    return result;
};

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
    var result =
    {
    };
    return result;
};

////////////////////////
// LEDG
////////////////////////
iqrf.embed.ledg.Set_Request_req = function (param) {
    return iqrf.embed.ledg.Set_Request(param.onOff);
};

iqrf.embed.ledg.Set_Response_rsp = function (rawHdp) {
    iqrf.embed.ledg.Set_Response(rawHdp);
    var result =
    {
    };
    return result;
};

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
    var result =
    {
    };
    return result;
};

////////////////////////
// Node
////////////////////////
iqrf.embed.node.Backup_Request_req = function (param) {
    return iqrf.embed.node.Backup_Request(param.index);
};

iqrf.embed.node.Backup_Response_rsp = function (rawHdp) {
    var result =
    {
        backupData: iqrf.embed.node.Backup_Response(rawHdp)
    };
    return result;
};

iqrf.embed.node.ClearRemotelyBondedMid_Request_req = function (param) {
    return iqrf.embed.node.ClearRemotelyBondedMid_Request();
};

iqrf.embed.node.ClearRemotelyBondedMid_Response_rsp = function (rawHdp) {
    iqrf.embed.node.ClearRemotelyBondedMid_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.node.EnableRemoteBonding_Request_req = function (param) {
    return iqrf.embed.node.EnableRemoteBonding_Request(param.bondingMask, param.control, param.userData);
};

iqrf.embed.node.EnableRemoteBonding_Response_rsp = function (rawHdp) {
    iqrf.embed.node.EnableRemoteBonding_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.node.Read_Request_req = function (param) {
    return iqrf.embed.node.Read_Request();
};

iqrf.embed.node.Read_Response_rsp = function (rawHdp) {
    var result = iqrf.embed.node.Read_Response(rawHdp)
    return result;
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

iqrf.embed.node.RemoveBondAddress_Request_req = function (param) {
    return iqrf.embed.node.RemoveBondAddress_Request();
};

iqrf.embed.node.RemoveBondAddress_Response_rsp = function (rawHdp) {
    iqrf.embed.node.RemoveBondAddress_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.node.Restore_Request_req = function (param) {
    return iqrf.embed.node.Restore_Request(param.networkData);
};

iqrf.embed.node.Restore_Response_rsp = function (rawHdp) {
    iqrf.embed.node.Restore_Response(rawHdp);
    var result =
    {
    };
    return result;
};

////////////////////////
// FRC
////////////////////////
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

////////////////////////
// IqrfStandardBinaryOutput
////////////////////////
iqrf.binaryoutput.Enumerate_Request_req = function (param) {
    return iqrf.binaryoutput.Enumerate_Request();
};

iqrf.binaryoutput.Enumerate_Response_rsp = function (rawHdp) {
    var result =
    {
        binOuts: iqrf.binaryoutput.Enumerate_Response(rawHdp)
    };
    return result;
};

iqrf.binaryoutput.SetOutput_Request_req = function (param) {
    return iqrf.binaryoutput.SetOutput_Request(param.binOuts);
};

iqrf.binaryoutput.SetOutput_Response_rsp = function (rawHdp) {
    var result =
    {
        prevVals: iqrf.binaryoutput.SetOutput_Response(rawHdp)
    };
    return result;
};

////////////////////////
// IqrfStandardLight
////////////////////////
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

////////////////////////
// IqrfStandardSensor
////////////////////////
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
    return iqrf.sensor.ReadSensorsWithTypes_Request(param.sensorIndexes);
};

iqrf.sensor.ReadSensorsWithTypes_Response_rsp = function (rawHdp) {
    var result =
    {
        sensors: iqrf.sensor.ReadSensorsWithTypes_Response(rawHdp)
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
       sensors: iqrf.sensor.Frc_Response(param.sensorType, param.frcCommand, param.responseFrcSend, param.responseFrcExtraResult)
   };
   return result;
};

////////////////////////
// EXPLORE
////////////////////////
iqrf.embed.explore.Enumerate_Request_req = function (param) {
    return iqrf.embed.explore.Enumerate_Request();
};

iqrf.embed.explore.Enumerate_Response_rsp = function (rawHdp) {
    var result = iqrf.embed.explore.Enumerate_Response(rawHdp);
    return result;
};

iqrf.embed.explore.PeripheralInformation_Request_req = function (param) {
    return iqrf.embed.explore.PeripheralInformation_Request(param.per);
};

iqrf.embed.explore.PeripheralInformation_Response_rsp = function (rawHdp) {
   var result = iqrf.embed.explore.PeripheralInformation_Response(parseInt( rawHdp.pnum, 16 ), rawHdp);
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
