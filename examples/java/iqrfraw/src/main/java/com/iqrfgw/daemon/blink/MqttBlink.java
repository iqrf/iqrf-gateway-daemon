package com.iqrfgw.daemon.blink;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.SerializationFeature;
import com.iqrfgw.daemon.api.iqrfRaw.request.Data;
import com.iqrfgw.daemon.api.iqrfRaw.request.IqrfRawRequest100;
import com.iqrfgw.daemon.api.iqrfRaw.request.Req;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;

/**
 * Sending request to pulse red led on device.
 * 
 * @author Michal Konopa
 */
public class MqttBlink {
    
    public static void main(String[] args) throws InterruptedException {
        // set request
        Req req = new Req();
        req.setrData("00.00.06.03.ff.ff");
       
        // set data
        Data data = new Data();
        data.setMsgId("test0");
        data.setTimeout(10000);
        data.setReq(req);
        data.setReturnVerbose(Boolean.TRUE);
        
        // set raw request
        IqrfRawRequest100 rawRequest = new IqrfRawRequest100();
        rawRequest.setmType(IqrfRawRequest100.MType.IQRF_RAW);
        rawRequest.setData(data);
        
        ObjectMapper mapper = new ObjectMapper();
        mapper.configure(SerializationFeature.INDENT_OUTPUT, true);

        // serialize request into JSON document
        // request in the JSON form
        String jsonRequest;
        try {
            // write resulting JSON document into the string
            jsonRequest = mapper.writeValueAsString(rawRequest);
            //System.out.println("JSON request: " + jsonRequest);
            //System.exit(0);
        } catch (JsonProcessingException ex) {
            System.out.println("Error during request serialization: " + ex);
            return; 
        }
        
        // mqtt connection
        String topicReq     = "Iqrf/DpaRequest";
        String topicResp    = "Iqrf/DpaResponse";
        int qos             = 1;
        String broker       = "tcp://mqtt.iqrfsdk.org:1883";
        String clientId     = "MqttBlinkTest";
        
        MemoryPersistence persistence = new MemoryPersistence();
        
        // some parts based on the example from: https://www.eclipse.org/paho/clients/java/
        try {
            MqttClient mqttClient = new MqttClient(broker, clientId, persistence);
            
            MqttConnectOptions connOpts = new MqttConnectOptions();
            connOpts.setCleanSession(true);
            
            System.out.println("Connecting to broker: " + broker);
            mqttClient.connect(connOpts);
            System.out.println("Connected");
            
            // subscribe to receive responses from the server
            mqttClient.subscribe(topicResp, qos, (topic, msg) -> {
                   System.out.println("Response: " + msg); 
                }
            );
            
            MqttMessage message = new MqttMessage(jsonRequest.getBytes());
            message.setQos(qos);
            
            System.out.println("Publishing message: " + message.toString());
            mqttClient.publish(topicReq, message);
            System.out.println("Message published");
            
            // wait for a while for the response
            Thread.sleep(5000);
            
            mqttClient.disconnect();
            System.out.println("Disconnected");
        } catch(MqttException e) {
            System.err.println("Error during message publish: " + e);
        }
    }
}
