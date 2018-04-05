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
        devNr: iqrf.embed.coordinator.Discovery_Response(rawHdp)
    };
    return result;
};

//TODO more detailed param in iqrfEmbedCoordinatorSetDpaParams-request-1-0-0.json
// then expected by this function?
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
    return iqrf.embed.coordinator.DiscoveryData_Request()
};

iqrf.embed.coordinator.DiscoveryData_Response_rsp = function (rawHdp) {
    var result =
    {
        discData: iqrf.embed.coordinator.DiscoveryData_Response(rawHdp)
    };
    return result;
};

iqrf.embed.coordinator.Backup_Request_req = function (param) {
    return iqrf.embed.coordinator.Backup_Request(param.index)
};

iqrf.embed.coordinator.Backup_Response_rsp = function (rawHdp) {
    var result =
    {
        netData: iqrf.embed.coordinator.Backup_Response(rawHdp)
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
    return iqrf.embed.coordinator.EnableRemoteBonding_Request()
};

iqrf.embed.coordinator.EnableRemoteBonding_Response_rsp = function (rawHdp) {
    iqrf.embed.coordinator.EnableRemoteBonding_Response(rawHdp);
    var result =
    {
    };
    return result;
};

iqrf.embed.coordinator.SmartConnect_Request_req = function (p) {
    return iqrf.embed.coordinator.SmartConnect_Request(p.reqAddr, p.bondingTestRetries, p.ibk, p.mid, p.bondingChannel, p.virtualDeviceAddress, p.userData)
};

iqrf.embed.coordinator.SmartConnect_Response_rsp = function (rawHdp) {
    iqrf.embed.coordinator.SmartConnect_Response(rawHdp);
    var result = iqrf.embed.coordinator.SmartConnect_Response(rawHdp);
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
        onOff: iqrf.embed.ledr.Set_Response(rawHdp)
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
        onOff: iqrf.embed.ledg.Set_Response(rawHdp)
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
        //TODO "result" shall be renamed in iqrfSensor_Enumerate-response-1-0-0.json
        // else we have perplexing result:{result:[]}
        result: iqrf.sensor.Enumerate_Response(rawHdp)
    };
    return result;
};

iqrf.sensor.ReadSensorsWithTypes_Request_req = function (param) {
    //TODO "params" shall be renamed in iqrfSensor_ReadSensorsWithTypes-request-1-0-0.json
    // else we have perplexing param:{params:[]}
    return iqrf.sensor.ReadSensorsWithTypes_Request(param.params);
};

iqrf.sensor.ReadSensorsWithTypes_Response_rsp = function (rawHdp) {
    var result =
    {
        //TODO "result" shall be renamed in iqrfSensor_ReadSensorsWithTypes-response-1-0-0.json
        // else we have perplexing result:{result:[]}
        result: iqrf.sensor.ReadSensorsWithTypes_Response(rawHdp)
    };
    return result;
};

// TODO needs special handling with state info
//iqrf.sensor.Frc_Request_req = function (p) {
//    return iqrf.sensor.Frc_Request(p.sensorType, p.sensorIndex, p.frcCmd, p.selectedNodes, p.sleepAfterFrc);
//};

//iqrf.sensor.Frc_Response_rsp = function (rawHdp) {
//    var result =
//    {
//        result: iqrf.sensor.Frc_Response(rawHdp)
//    };
//    return result;
//};

