//"use strict";
namespace( 'iqrf.daemon' );

iqrf.daemon.RspObject = function ( rawHdp )
{
  this.nAdr = '0000';
  this.hwpId = '0000';
  this.rCode = rawHdp.rcode;
  this.dpaVal = rawHdp.dpaval;
};

////////////////////////
// Thermometer
////////////////////////
iqrf.embed.thermometer.Read_Request_req = function (req)
{
  return iqrf.embed.thermometer.Read_Request();
};

iqrf.embed.thermometer.Read_Response_rsp = function ( rawHdp )
{
  var rsp = new iqrf.daemon.RspObject(rawHdp);
  rsp.temperature = iqrf.embed.thermometer.Read_Response( rawHdp );
  return rsp;
};

////////////////////////
// LEDR
////////////////////////
iqrf.embed.ledr.Set_Request_req = function (req) {
    return iqrf.embed.ledr.Set_Request(req.onOff);
};

iqrf.embed.ledr.Set_Response_rsp = function (rawHdp) {
    var rsp = new iqrf.daemon.RspObject(rawHdp);
    rsp.onOff = iqrf.embed.ledr.Set_Response(rawHdp);
    return rsp;
};

iqrf.embed.ledr.Get_Request_req = function (req) {
    return iqrf.embed.ledr.Get_Request();
};

iqrf.embed.ledr.Get_Response_rsp = function (rawHdp) {
    var rsp = new iqrf.daemon.RspObject(rawHdp);
    rsp.onOff = iqrf.embed.ledr.Get_Response(rawHdp);
    return rsp;
};

iqrf.embed.ledr.Pulse_Request_req = function (req) {
    return iqrf.embed.ledr.Pulse_Request();
};

iqrf.embed.ledr.Pulse_Response_rsp = function (rawHdp) {
    var rsp = new iqrf.daemon.RspObject(rawHdp);
    return rsp;
};

////////////////////////
// LEDG
////////////////////////
iqrf.embed.ledg.Set_Request_req = function (req) {
    return iqrf.embed.ledg.Set_Request(req.onOff);
};

iqrf.embed.ledg.Set_Response_rsp = function (rawHdp) {
    var rsp = new iqrf.daemon.RspObject(rawHdp);
    rsp.onOff = iqrf.embed.ledg.Set_Response(rawHdp);
    return rsp;
};

iqrf.embed.ledg.Get_Request_req = function (req) {
    return iqrf.embed.ledg.Get_Request();
};

iqrf.embed.ledg.Get_Response_rsp = function (rawHdp) {
    var rsp = new iqrf.daemon.RspObject(rawHdp);
    rsp.onOff = iqrf.embed.ledg.Get_Response(rawHdp);
    return rsp;
};

iqrf.embed.ledg.Pulse_Request_req = function (req) {
    return iqrf.embed.ledg.Pulse_Request();
};

iqrf.embed.ledg.Pulse_Response_rsp = function (rawHdp) {
    var rsp = new iqrf.daemon.RspObject(rawHdp);
    return rsp;
};
