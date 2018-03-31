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
// par req{}
// return rawHdpRequest without set nadr, hwpid
iqrf.embed.thermometer.Read_Request_req = function ( req )
{
  return iqrf.embed.thermometer.Read_Request();
};

////////////////////////
// par rawHdpResponse{}
// return rsp{}
iqrf.embed.thermometer.Read_Response_rsp = function ( rawHdp )
{
  var rsp = new iqrf.daemon.RspObject(rawHdp);
  try {
    rsp.temperature = iqrf.embed.thermometer.Read_Response( rawHdp );
  }
  catch(err) {
	rsp.temperature = -275.15
  }
  return rsp;
};

////////////////////////
