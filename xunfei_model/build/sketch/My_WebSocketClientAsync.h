#line 1 "D:\\01-同步文件夹\\Arduino\\mini_test\\My_WebSocketClientAsync.h"
#ifndef WEBSOCKET_CLIENT_ASYNC_H
#define WEBSOCKET_CLIENT_ASYNC_H

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <functional>
#include <ESP8266HTTPClient.h>

void connectWiFi(const char *ssid,const char *password);
String extractDateHeader(const String& httpResponse);
enum WebSocketClientState {
  WS_DISCONNECTED,
  WS_CONNECTING,
  WS_CONNECTED,
  WS_DISCONNECTING
};

enum WebSocketOpcode {
  WS_OP_CONT = 0x0,
  WS_OP_TEXT = 0x1,
  WS_OP_BINARY = 0x2,
  WS_OP_CLOSE = 0x8,
  WS_OP_PING = 0x9,
  WS_OP_PONG = 0xA
};

 typedef std::function<void()> WSConnectedHandler;
typedef std::function<void()> WSDisconnectedHandler;
typedef std::function<void(WebSocketOpcode opcode, uint8_t* payload, size_t length)> WSDataHandler;
 typedef std::function<void(const char* error)> WSErrorHandler;



class My_WebSocketClientAsync {
public:
  My_WebSocketClientAsync(bool secure = false);
  ~My_WebSocketClientAsync();

  void begin(const char* host, uint16_t port, const char* path = "/", const char* protocol = "arduino");
  void begin(String host, uint16_t port, String path = "/", String protocol = "arduino");
  void end();
  String get_time(String tim_url);
  void loop();
  
  
  bool read();
  bool send(WebSocketOpcode opcode, uint8_t* payload, size_t length, bool mask = true);
  bool send(String str);
  bool send(char* str);
  bool send(uint8_t* payload, size_t length);
  bool ping(uint8_t* payload = nullptr, size_t length = 0);
  
  void onConnect(WSConnectedHandler handler);
  void onDisconnect(WSDisconnectedHandler handler);
  void onData(WSDataHandler handler);
  void onError(WSErrorHandler handler);

  WebSocketClientState getState();
  
private:
  WiFiClient* _client;
  BearSSL::WiFiClientSecure My_client;

  bool _secure;
  WebSocketClientState _state;
  
  String _host;
  uint16_t _port;
  String _path;
  String _protocol;
  String _key;
  
  WSConnectedHandler _connectHandler;
  WSDisconnectedHandler _disconnectHandler;
  WSDataHandler _dataHandler;
  WSErrorHandler _errorHandler;
  
  uint8_t* _rxBuffer;
  size_t _rxBufferSize;
  
  void _handleError(const char* error);
  bool _sendFrame(WebSocketOpcode opcode, uint8_t* payload, size_t length, bool mask);
  void _handleFrame();
  bool _readBytes(uint8_t* buffer, size_t length, unsigned long timeout = 10000);
   void _generateKey();
   String _base64Encode(uint8_t* data, size_t length);
   void _handshake();
};

#endif