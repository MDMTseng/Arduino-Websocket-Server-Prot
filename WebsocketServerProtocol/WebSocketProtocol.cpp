#include <WebSocketProtocol.h>
#include "sha1.h"
#include "Base64.h"
#include <Ethernet.h>
//#include "Eth_Boost.h"
//#include "RingBuff.h"


#define DEBUG_
#ifdef DEBUG_
#define DEBUG_print(A, ...) Serial.print(A,##__VA_ARGS__)
#define DEBUG_println(A, ...) Serial.println(A,##__VA_ARGS__)
#else
#define DEBUG_print(A, ...)
#define DEBUG_println(A, ...)
#endif

/*
example PKG:
GET / HTTP/1.1
Upgrade: websocket
Connection: Upgrade
Host: 10.0.0.52:5213
Origin: http://mdm.noip.me
Pragma: no-cache
Cache-Control: no-cache
Sec-WebSocket-Key: XY1RK1rsvSdk4Q4xggisMg==
Sec-WebSocket-Version: 13
Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits, x-webkit-deflate-frame
User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gec
*/
#define HANDSHAKE_GETHTTP "GET / HTTP/1.1"
#define HANDSHAKE_UPGRAGE "Upgrade:"//
#define HANDSHAKE_CONNECTION "Connection:"//
#define HANDSHAKE_Host "Host:"//
#define HANDSHAKE_ORIGIN "Origin:"//
#define HANDSHAKE_PRAGMA "Pragma:"//ignore
#define HANDSHAKE_CACHECON "Cache-Control:"
#define HANDSHAKE_SECWSKEY "Sec-WebSocket-Key:"//
#define HANDSHAKE_SECWSVER "Sec-WebSocket-Version:"//
#define HANDSHAKE_EXTEN "Sec-WebSocket-Extensions:"//ignore

#define HANDSHAKE_HEADER "HTTP/1.1 101 Switching Protocols\r\n\
Upgrade: websocket\r\n\
Connection: Upgrade\r\n\
Sec-WebSocket-Accept: "


bool CheckHead(char* str,char* pattern)
{
	for(;*pattern;str++,pattern++)
		if(*str!=*pattern)return false;
	return true;
}
void WebSocketProtocol::printState()
{
	switch(state)
	{
		case DISCONNECTED: Serial.print("DISCONNECTED");break;
		case WS_HANDSHAKE: Serial.print("WS_HANDSHAKE");break;
		case WS_CONNECTED: Serial.print("WS_CONNECTED");break;
		case UNKNOWN_CONNECTED: Serial.print("UNKNOWN_CONNECTED");break;
	}
}

void WebSocketProtocol::printRecvOPState()
{
	switch(recvOPState)
	{
		case WSOP_CLOSE: Serial.print("WSOP_CLOSE");break;
		case WSOP_OK: Serial.print("WSOP_OK");break;
		case WSOP_UNKNOWN: Serial.print("WSOP_UNKNOWN");break;
	}
}
WebSocketProtocol::WebSocketProtocol(const char *urlPrefix ):
    socket_urlPrefix(urlPrefix)
{
	
    rmClientOBJ();
}

//if you see decodeRecvPkg return null call it
WSState WebSocketProtocol::getState()
{
	return state;
}
RecvOP WebSocketProtocol::getRecvOPState()
{
	return recvOPState;
}

char * WebSocketProtocol::processRecvPkg(char *str, unsigned int length)
{
	if (state == DISCONNECTED||
	(*str=='H'&&CheckHead(str,HANDSHAKE_GETHTTP)) ) {
		if (doHandshake(str,length)) {
		
			state = WS_HANDSHAKE;
			return null;
		}
		state = UNKNOWN_CONNECTED;
		return str;
	}
	if (state == UNKNOWN_CONNECTED)return str;
	//else if(state == WS_CONNECTED)
	state = WS_CONNECTED;
	return decodeRecvPkg(str, length);
	
	
	
}


char * WebSocketProtocol::decodeRecvPkg(char *str, unsigned int length)
{
	char* striter=str;
	if(length<6)
	{
		recvOPState=WSOP_UNKNOWN;
		return null;
	}
	
    byte bite=*(striter++);
	
    frame.opcode = bite & 0xf; // Opcode
    frame.isFinal = bite & 0x80; // Final frame?
    // Determine length (only accept <= 64 for now)
    bite =*(striter++);
    frame.length = bite & 0x7f; // Length of payload
	/*if (frame.length > 64) {
        #ifdef DEBUG
            Serial.print("Too big frame to handle. Length: ");
            Serial.println(frame.length);
        #endif
        client.write((uint8_t) 0x08);
        client.write((uint8_t) 0x02);
        client.write((uint8_t) 0x03);
        client.write((uint8_t) 0xf1);
        return false;
    }*/
	frame.mask[0] = *(striter++);
    frame.mask[1] = *(striter++);
    frame.mask[2] = *(striter++);
    frame.mask[3] = *(striter++);
	unsigned int i;
	for ( i = 0; i < frame.length; i++)
		striter[i] = striter[i]^ frame.mask[i &0x3];//%4
	striter[i]='\0';
	striter[-1]=i;//trick to store string size 0~255;
    if (!frame.isFinal) {
        // We don't handle fragments! Close and disconnect.
       /* #ifdef DEBUG
            Serial.println("Non-final frame, doesn't handle that.");
        #endif
        client.print((uint8_t) 0x08);
        client.write((uint8_t) 0x02);
        client.write((uint8_t) 0x03);
        client.write((uint8_t) 0xf1);
        return false;*/
    }
	
	switch (frame.opcode) {
        case 0x01: // Txt frame
            // Call the user provided function
            /*if (onData)
                onData(*this, frame.data, frame.length);*/
			recvOPState=WSOP_OK;
			return striter;
            
        case 0x08:
            // Close frame. Answer with close and terminate tcp connection
            // TODO: Receive all bytes the client might send before closing? No?
            recvOPState=WSOP_CLOSE;
            return null;
            
        default:
            // Unexpected. Ignore. Probably should blow up entire universe here, but who cares.
    		
			recvOPState=WSOP_UNKNOWN;
			return null;
    }
	recvOPState=WSOP_UNKNOWN;
	return null;
	
	
	
}

char * WebSocketProtocol::doHandshake(char *str, unsigned int length){

	if(!CheckHead(str,HANDSHAKE_GETHTTP))
	{
		state=UNKNOWN_CONNECTED;
		return null;	
	}
	
    char *temp=str;
	//str[length]='\0';
	str+=sizeof(HANDSHAKE_GETHTTP);
    char *bite_ptr=str;
	
	
    char key[80];
    char bite;
    
    bool hasUpgrade = false;
    bool hasConnection = false;
    bool isSupportedVersion = false;
    bool hasHost = false;
    bool hasOrigin = false;
    bool hasKey = false;
    byte counter = 0;
	
	byte L=length;
	

    for (;*bite_ptr;) {
	    if(*bite_ptr != '\n'){
		
			bite_ptr++;
			continue;
		}
		
		
        
		*(bite_ptr-1)=0;
		bite_ptr++;if(*bite_ptr==null)break;
		
	
	
	
	   // temp[counter - 2] = 0; // Terminate string before CRLF \r\n
		
		// Ignore case when comparing and allow 0-n whitespace after ':'. See the spec:
		// http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html
		if (!hasUpgrade && CheckHead(bite_ptr,HANDSHAKE_UPGRAGE)) {
			// OK, it's a websockets handshake for sure
			bite_ptr+=sizeof(HANDSHAKE_UPGRAGE);
			hasUpgrade = true;	
		} else if (!hasConnection && CheckHead(bite_ptr, HANDSHAKE_CONNECTION)) {
			bite_ptr+=sizeof(HANDSHAKE_CONNECTION);
			hasConnection = true;
		} else if (!hasOrigin && CheckHead(bite_ptr, HANDSHAKE_ORIGIN)) {
			bite_ptr+=sizeof(HANDSHAKE_ORIGIN);
			hasOrigin = true;
		} else if (!hasHost && CheckHead(bite_ptr, HANDSHAKE_Host)) {
			bite_ptr+=sizeof(HANDSHAKE_Host);
			hasHost = true;
		} else if (!hasKey && CheckHead(bite_ptr, HANDSHAKE_SECWSKEY)) {
			bite_ptr+=sizeof(HANDSHAKE_SECWSKEY)-1;
			for(;*bite_ptr==' ';bite_ptr++)//skip space
				if(!*bite_ptr){state=UNKNOWN_CONNECTED;return null;}//sudden termination error
			byte tmpc=0;
			for(;*bite_ptr!='\r';tmpc++,bite_ptr++)//Copy key and prevent sudden termination error
				if(!*bite_ptr){state=UNKNOWN_CONNECTED;return null;}
				else key[tmpc]=*bite_ptr;
			key[tmpc]='\0';
			
			hasKey=true;
		} else if (!isSupportedVersion && CheckHead(bite_ptr,HANDSHAKE_SECWSVER)
		&& strstr(bite_ptr+sizeof(HANDSHAKE_SECWSVER), "13")) {
			bite_ptr+=sizeof(HANDSHAKE_SECWSVER)+2;
			isSupportedVersion = true;
			if (hasUpgrade && hasConnection && isSupportedVersion 
			&& hasHost && hasOrigin && hasKey)
				break;//usually the last one 
		}
           
        
    }
	
	bite_ptr=temp;
    // Assert that we have all headers that are needed. If so, go ahead and
    // send response headers.
    if (hasUpgrade && hasConnection && isSupportedVersion && hasHost && hasOrigin && hasKey) {
        strcat(key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"); // Add the omni-valid GUID
		
		
        Sha1.init();
        Sha1.print(key);
        uint8_t *hash = Sha1.result();
		strcpy(bite_ptr,HANDSHAKE_HEADER);
		bite_ptr+=sizeof(HANDSHAKE_HEADER)-1;
	
        base64_encode(bite_ptr, (char*)hash, 20);
		for(;*bite_ptr;bite_ptr++);
		strcpy(bite_ptr,CRLF);
		bite_ptr+=sizeof(CRLF)-1;
		strcpy(bite_ptr,CRLF);
		bite_ptr+=sizeof(CRLF)-1;
		
		
    } else {
        // Nope, failed handshake. Disconnect
		state=UNKNOWN_CONNECTED;
        return null;
    }
    
	state=WS_HANDSHAKE;/**/
    return temp;
}


char * WebSocketProtocol::codeSendPkg_setPkgL(char *Pkg, unsigned int length)
{
	Pkg[0]=(uint8_t) 0x81;
	Pkg[1]=(uint8_t) length;
}
char * WebSocketProtocol::codeSendPkg_getPkgContentSec(char *Pkg)
{
	return Pkg+2;
}
char * WebSocketProtocol::codeSendPkg_endConnection(char *Pkg)
{
    Pkg[0]= 0x08;
    Pkg[1]= 0x02;
    Pkg[2]= 0x03;
    Pkg[3]= 0xf1;
    Pkg[4]= 0;
	return Pkg;
}

void WebSocketProtocol::setClientOBJ(EthernetClient client)
{
		clientOBJ=client;
}

EthernetClient WebSocketProtocol::getClientOBJ()
{
    return clientOBJ;
}

void WebSocketProtocol::rmClientOBJ()
{
	clientOBJ._sock = MAX_SOCK_NUM;
	//clientOBJ=null;
    state = DISCONNECTED;
	recvOPState=WSOP_CLOSE;
}