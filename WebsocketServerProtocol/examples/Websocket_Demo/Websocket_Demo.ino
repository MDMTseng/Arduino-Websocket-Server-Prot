/*
 Websocket Server Protocol

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
#include "utility/socket.h"
#include <ETH_Extra.h>
#ifdef DEBUG_
#define DEBUG_print(A, ...) Serial.print(A,##__VA_ARGS__)
#define DEBUG_println(A, ...) Serial.println(A,##__VA_ARGS__)
#else
#define DEBUG_print(A, ...)
#define DEBUG_println(A, ...)
#endif

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

char retPackage[20];
void setup() {
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  Serial.begin(57600);


  DEBUG_print("Chat server address:");
  DEBUG_println(Ethernet.localIP());
  setRetryTimeout(4,1000);
}

unsigned int counter2Pin=0;

byte LiveClient=0;
void loop() {
  // wait for a new client:
  if(counter2Pin++>1000)
  {
    counter2Pin=0;
    PingAllClient();
    clearUnreachableClient();
    
    LiveClient=countConnected();
    retPackage[0]=0x81;
    sprintf((retPackage+2),"Your Socket: @  Total live: %d",LiveClient);
    retPackage[1]=strlen(retPackage+2);
  }
  EthernetClient client = server.available();

  if (client) {

    buffiter = buff;
    unsigned int  KL = 0;

    unsigned int PkgL =  client.available();
    KL = PkgL;
    recv(client._sock, (uint8_t*)buffiter, PkgL);
    WebSocketProtocol* WSPptr  = findFromProt(client);
    if (WSPptr == null)
    {
      client.stop();
      return;
    }
    client = WSPptr->getClientOBJ();
    char *recvData = WSPptr->processRecvPkg(buff, KL);
    if (WSPptr->getState() == WS_HANDSHAKE)
    {
      DEBUG_print("WS_HANDSHAKE::");
      DEBUG_println(client._sock);
      client.print(buff);
      return;
    }
    if (WSPptr->getRecvOPState() == WSOP_CLOSE)
    {
      
      DEBUG_print("Normal close::");
      DEBUG_println(client._sock);
      client.stop();
      WSPptr->rmClientOBJ();
      return;
    }
    if (WSPptr->getRecvOPState() == WSOP_UNKNOWN)
    {
      DEBUG_print("unusual close::");
      DEBUG_println(client._sock);
      client.print(WSPptr->codeSendPkg_endConnection(buff));

      client.stop();
      WSPptr->rmClientOBJ();
      return;
    }
   // *(recvData-2)= 0x81;
   // WSPptr->getClientOBJ().print(recvData-2);
    retPackage[15]=client._sock+'0';
    WSPptr->getClientOBJ().print(retPackage);
  }
}
void clearUnreachableClient()
{
  for (byte i = 0; i < 4; i++)
  {
    EthernetClient Rc = WSP[i].getClientOBJ();
    if (Rc && Rc.status() == SnSR::CLOSED)
    {
      DEBUG_print("clear timeout sock::");
      DEBUG_println(Rc._sock);
      Rc.stop();
      WSP[i].rmClientOBJ();
    }
  }
}
void PingAllClient()
{
  for (byte i = 0; i < 4; i++)
    if (WSP[i].getClientOBJ())
    {
      //byte SnIR = ReadSn_IR(WSP[i].getClientOBJ()._sock);


      /*buff[0] = 0x81;
      buff[1] = 1;
      buff[2] = 1;
      buff[3] = 0;
      WSP[i].getClientOBJ().print(buff);*/
      TestAlive(WSP[i].getClientOBJ()._sock);
    }
}
byte countConnected()
{
  byte C = 0;
  for (byte i = 0; i < 4; i++)
    if (WSP[i].getClientOBJ())
      C++;
  return C;
}

WebSocketProtocol* findFromProt(EthernetClient client)
{

  for (byte i = 0; i < 4; i++)
  {
    EthernetClient Rc = WSP[i].getClientOBJ();
    if (Rc == client)
      return WSP + i;
  }

  DEBUG_print("NO exist sock, find available:::");
  LiveClient=countConnected();
  DEBUG_println(LiveClient);
  for (byte i = 0; i < 4; i++)
  {
    if (!WSP[i].getClientOBJ())
    {

      DEBUG_print(i);
      DEBUG_print("  ::::  ");
      DEBUG_println(WSP[i].getClientOBJ());
      WSP[i].setClientOBJ(client);
      return WSP + i;
    }
  }
  return null;
}
