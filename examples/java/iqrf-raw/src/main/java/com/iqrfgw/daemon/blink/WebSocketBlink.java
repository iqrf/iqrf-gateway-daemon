package com.iqrfgw.daemon.blink;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.SerializationFeature;
import com.iqrfgw.daemon.api.iqrfRaw.request.Data;
import com.iqrfgw.daemon.api.iqrfRaw.request.IqrfRawRequest100;
import com.iqrfgw.daemon.api.iqrfRaw.request.Req;
import java.io.IOException;
import java.net.URI;
import javax.websocket.ClientEndpoint;
import javax.websocket.CloseReason;
import javax.websocket.ContainerProvider;
import javax.websocket.OnClose;
import javax.websocket.OnError;
import javax.websocket.OnMessage;
import javax.websocket.OnOpen;
import javax.websocket.Session;
import javax.websocket.WebSocketContainer;

/**
 * Sending request to pulse red led on device.
 * 
 * @author Michal Konopa
 */
public class WebSocketBlink {
    
    // web socket client
    @ClientEndpoint
    public static class WebsocketClient {
        private Session userSession = null;
        private MessageHandler messageHandler;

        public WebsocketClient(URI endpointURI) {
            try {
                WebSocketContainer container = ContainerProvider.getWebSocketContainer();
                container.connectToServer(this, endpointURI);
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }

         // Callback hook for Connection open events
        @OnOpen
        public void onOpen(Session userSession) {
            System.out.println("opening websocket");
            this.userSession = userSession;
        }

        // Callback hook for Connection close events.
        @OnClose
        public void onClose(Session userSession, CloseReason reason) {
            System.out.println("closing websocket");
            this.userSession = null;
        }

        // Callback hook for Message Events.
        @OnMessage
        public void onMessage(String message) {
            if (this.messageHandler != null) {
                this.messageHandler.handleMessage(message);
            }
        }
        
        // Callback hook for errors
        @OnError
        public void onError(Session session, Throwable throwable) {
            System.out.println("Session error.");
        }
        
        // registers message handler
        public void addMessageHandler(MessageHandler msgHandler) {
            this.messageHandler = msgHandler;
        }

        // sends message
        public void sendMessage(String message) {
            try {
                this.userSession.getBasicRemote().sendText(message);
            } catch (IOException ex) {
                System.out.println("Sending message failed: " + ex);
            }
        }

        // message handler
        public static interface MessageHandler {
            public void handleMessage(String message);
        }
    }   
    
    public static void main(String[] args) {
        // set request
        Req req = new Req();
        req.setrData("00.00.06.03.ff.ff");
       
        // set data
        Data data = new Data();
        data.setMsgId("test0");
        //data.setTimeout(10000);
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
        } catch (JsonProcessingException ex) {
            System.out.println("Error during request serialization: " + ex);
            return; 
        }
        
        // sending JSON request to the server
        try {
            // open websocket
            final WebsocketClient client 
                    = new WebsocketClient(new URI("ws://localhost:1338"));

            // add listener
            client.addMessageHandler((String message) -> {
                System.out.println("Response:" + message);
            });

            // sends request to websocket
            client.sendMessage(jsonRequest);

            // wait 5 seconds for messages from websocket
            Thread.sleep(5000);
        } catch (Exception ex) {
            System.err.println("Error during socket communication: " + ex);
        }
    }
}
