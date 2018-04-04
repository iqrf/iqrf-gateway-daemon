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
    iqrf.embed.ledr.Pulse_Response();
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
    iqrf.embed.ledg.Pulse_Response();
    var result =
    {
    };
    return result;
};
