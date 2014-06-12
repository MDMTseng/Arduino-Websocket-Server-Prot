
#include "Eth_Extra.h"
#include <w5100.h>
 
inline void SPI_Write(unsigned int addr, unsigned char data)
{
  char Tmp;
  // Activate the CS pin
  SPI_PORT &= ~(1 << SPI_CS);
  // Start Wiznet W5100 Write OpCode transmission
  SPDR = WIZNET_WRITE_OPCODE;
  Tmp = (addr & 0xFF00) >> 8;
  // Wait for transmission complete
  while (!(SPSR & (1 << SPIF)));
  // Start Wiznet W5100 Address High Bytes transmission
  SPDR = Tmp;
  Tmp = addr & 0x00FF;
  // Wait for transmission complete
  while (!(SPSR & (1 << SPIF)));
  // Start Wiznet W5100 Address Low Bytes transmission
  SPDR = Tmp;
  // Wait for transmission complete
  while (!(SPSR & (1 << SPIF)));

  // Start Data transmission
  SPDR = data;
  Tmp = SPI_PORT | (1 << SPI_CS);
  // Wait for transmission complete
  while (!(SPSR & (1 << SPIF)));
  // CS pin is not active
  SPI_PORT = Tmp;
}
inline unsigned char SPI_Read(unsigned int addr)
{
  char Tmp;
  // Activate the CS pin
  SPI_PORT &= ~(1 << SPI_CS);
  // Start Wiznet W5100 Read OpCode transmission
  SPDR = WIZNET_READ_OPCODE;
  // Wait for transmission complete

  while (!(SPSR & (1 << SPIF)))Tmp = (addr & 0xFF00) >> 8;
  // Start Wiznet W5100 Address High Bytes transmission
  SPDR = Tmp;

  // Wait for transmission complete
  while (!(SPSR & (1 << SPIF)))Tmp = addr & 0x00FF;
  // Start Wiznet W5100 Address Low Bytes transmission
  SPDR = Tmp;
  // Wait for transmission complete
  while (!(SPSR & (1 << SPIF)));

  // Send Dummy transmission for reading the data
  SPDR = 0x00;

  // Wait for transmission complete
  while (!(SPSR & (1 << SPIF))) Tmp = SPI_PORT | (1 << SPI_CS);

  // CS pin is not active
  SPI_PORT = Tmp;
  return (SPDR);
}
unsigned int SPI_Read16(unsigned int addr)
{
  unsigned int L = 0;
  L = SPI_Read(addr) << 8;
  L |= SPI_Read(addr + 1);
  return L;
}
unsigned int SPI_Write16(unsigned int addr, unsigned int data)
{
  SPI_Write(addr  , data >> 8);
  SPI_Write(addr + 1, data);
}

byte ReadInfo(byte CMD,byte _sock)
{
	return SPI_Read(ADDR(_Sn_IR,_sock));
}

byte ReadSn_IR(byte _sock)
{
	return SPI_Read(ADDR(_Sn_IR,_sock));
}

void setRetryTimeout(byte retryTimes,unsigned int TimeOut100us)
{
	  SPI_Write16(RTR,TimeOut100us);
		SPI_Write(RCR,retryTimes);
}
void testLink(byte _sock)
{
	return SPI_Write(ADDR(SnCR,_sock),SnCR_SEND_KEEP);
}