/*
 Advanced Chat Server

 A more advanced server that distributes any incoming messages
 to all connected clients but the client the message comes from.
 To use telnet to  your device's IP address and type.
 You can see the client's input in the serial monitor as well.
 Using an Arduino Wiznet Ethernet shield.

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)

 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe
 redesigned to make use of operator== 25 Nov 2013
 by Norbert Truchsess

 */

#include <SPI.h>
#include <Ethernet.h>
#include <WebSocketProtocol.h>
#include "utility/w5100.h"

#include "Eth_Boost.h"
#include "RingBuff.h"
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(10, 0, 0, 52);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);


// telnet defaults to port 23
EthernetServer server(5213);

//EthernetClient clients[4];
WebSocketProtocol WSP[4];

RingBuff RB;
char buff[600];
char *buffiter;
void setup() {
  // initialize the ethernet device
  Ethernet.begin(mac, ip, gateway, subnet);
  // start listening for clients
  server.begin();
  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  cb_init(&RB, buff, sizeof(buff));

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
    if (client.available() > 0) {
      RB.head=RB.tail=RB.buffer;
      KL = RECVData(&RB, client._sock);
    }
   // WSP[0].setClientOBJ((void*)&client);
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
    //Serial.print(recvData + 2);
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
  
  Serial.println("NO exist, find available");
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
