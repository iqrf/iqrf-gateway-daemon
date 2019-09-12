/***
 Author: 2017                                                              
         Rostislav Spinar <rostislav.spinar@iqrf.com>                      
         IQRF Tech s.r.o.                                                  
***/

/***
 npm install mqtt --save                                                   
***/                                                                           

var mqtt = require('mqtt')
var client = mqtt.connect('mqtt://10.1.30.13:1883')

/*** client on connect ***/
client.on('connect', function () {
  console.log("Client is connected.");
})

/*** client on reconnect ***/
client.on("reconnect", function() {
  console.log("Client is reconnected.");
})

/*** client on error ***/
client.on("error", function(err) {
  console.log("Error from client --> ", err);
})

/*** client on close ***/
client.on("close", function() {
  console.log("Client is closed.");
})

/*** client on offline ***/
client.on("offline", function(err) {
  console.log("Client is offline.");
})

/** subscribe to a topic */
// subscribe method takes two arguments {topic,options}.
client.subscribe('Iqrf/DpaResponse', { qos: 1 }, function(err, granted) {
  if (err)
    console.log(err);
  else
    console.log("Client subscribed : ", granted);
});

/*** json dpa raw message - pulse ledr node1 ***/
var dpa_message =
{
    ctype: "dpa",
    type: "raw",
    msgid: "1",
    request: "01.00.06.03.ff.ff",
    request_ts: "",
    confirmation: "",
    confirmation_ts: "",
    response: "",
    response_ts: ""
}

/*** publish a message ***/
// set retain true to deliver a message (like welcome messages) to the newly subscribed client.
// set qos = 1 to guarantee delivery service implement.
// broker will always store the last retain message per topic if retain is true for messages.
client.publish('Iqrf/DpaRequest', JSON.stringify(dpa_message), { retain: true, qos: 1 });

/*** listen for a message and exit ***/
client.on('message', function (topic, message) {
  // message is Buffer
  console.log(message.toString())
  client.end()
})
