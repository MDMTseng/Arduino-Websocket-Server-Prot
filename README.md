## Arduino-Websocket-Server-Protocol


To rewrite the websocket lib for Arduino I extract the protocol part from whole websocket transceiver module. It's kind of diffcault to use for now

The next step would be make another lib to manage transceiver and protocol part and make it easier to use, also in raw speed!!!!


2014/06/12- make it robust using addional function to control timeout, retry number, and auto detect if client reachable.
4 sockets(clients) works great.
