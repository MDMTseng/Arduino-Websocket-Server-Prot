/*
 Advanced Chat Server Protocol

 This example demostrate a simple echo server.
 It demostrate how the library <WebSocketProtocol.h> works 
 and how to handle the state changes.
 
 dependent library:WIZNET <Ethernet.h>
 


 created 11 June 2014
 by MDM Tseng
 */

#include <SPI.h>
#include <Ethernet.h>
#include <WebSocketProtocol.h>
#include "utility/w5100.h"
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(10, 0, 0, 52);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
EthernetServer server(5213);

WebSocketProtocol WSP[4];

char buff[600];
char *buffiter;
void setup() {
  // initialize the ethernet device
  Ethernet.begin(mac, ip, gateway, subnet);
  // start listening for clients
  server.begin();
  // Open serial communications and wait for port to open:
  Serial.begin(57600);


  Serial.print("Chat server address:");
  Serial.println(Ethernet.localIP());
}
void loop() {
  // wait for a new client:
  EthernetClient client = server.available();

  if (client) {

  clearUnavalibleClient();
    buffiter = buff;
    byte KL = 0;
    buffiter=buff;
    KL=client.available();
    for(;KL;KL--) {
      *(buffiter++)=client.read();
    }
    WebSocketProtocol* WSPptr  =findFromProt(client);
      Serial.println(WSPptr-WSP);
    if(WSPptr==null)
    {
      client.stop();
      return;
    }
    client=WSPptr->getClientOBJ();
    
    char *recvData = WSPptr->processRecvPkg(buff, KL);
    //WSP.printState();
    //Serial.println();
    if (WSPptr->getState() == WS_HANDSHAKE)
    {
      client.print(buff);
      return;
    }
    if (WSPptr->getRecvOPState() == WSOP_CLOSE)
    {
      client.stop();
      WSPptr->rmClientOBJ();
      return;
    }
    if(WSPptr->getRecvOPState() == WSOP_UNKNOWN)
    {
      client.print(WSPptr->codeSendPkg_endConnection(buff));
      
      client.stop();
      WSPptr->rmClientOBJ();
      return;
    }
    
    //WSP.printRecvOPState();
    //Serial.println();
    recvData -= 2;
    recvData[0] = 0x81;
    client.print(recvData); return;
  }
}
void clearUnavalibleClient()
{
  for(byte i=0;i<4;i++)
  {
    EthernetClient Rc=WSP[i].getClientOBJ();
    if(Rc&&Rc.status()==SnSR::CLOSED)
    {
      Serial.print("clear unused sock::");
      Serial.println(Rc._sock);
      Rc.stop();
      WSP[i].rmClientOBJ();
    }
  }
}


WebSocketProtocol* findFromProt(EthernetClient client)
{
  
  for(byte i=0;i<4;i++)
  {
    EthernetClient Rc=WSP[i].getClientOBJ();
    if(Rc==client)
    {
      
      Serial.print(i);Serial.print("::");Serial.print(Rc);Serial.print("==");Serial.println(client);
      return WSP+i;
    }
  }
  
  Serial.println("NO exist client, find available");
  for(byte i=0;i<4;i++)
  {
    if(!WSP[i].getClientOBJ())
    {
      
      Serial.print(i);
      Serial.print("  ::::  ");
      Serial.println(WSP[i].getClientOBJ());
      WSP[i].setClientOBJ(client);
      return WSP+i;
    } 
  }
  return null;
}
