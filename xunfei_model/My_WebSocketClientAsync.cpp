#include "My_WebSocketClientAsync.h"
#include <base64.h>
#include <BearSSLHelpers.h>  
#include <ArduinoJson.h>
#include "xunfei_sound.h"
#include "U8g2lib.h"
#include <ScrollTextDisplay.h>

String chineseResult = "";

void connectWiFi(const char *ssid,const char *password) {
  WiFi.begin(ssid, password);
  Serial.print("Conect to");
  Serial.print(ssid);
  Serial.print("...");
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(i++);
    Serial.print(' ');
  }
  Serial.println("");
  Serial.print("conect success");
  Serial.print("ip address    ");
  Serial.println(WiFi.localIP());
}
String Sound_Chinese(){
  return chineseResult;
}
My_WebSocketClientAsync::My_WebSocketClientAsync(bool secure) : 
   _secure(secure),
  _state(WS_DISCONNECTED),
  _port(0),
  _rxBuffer(nullptr),
  _rxBufferSize(0) {
  
  if (_secure) {
   
  } else {
    _client = new WiFiClient();
  }
}

My_WebSocketClientAsync::~My_WebSocketClientAsync() {
  //end();
  if (!_secure) {
    delete _client;
  }
}

void My_WebSocketClientAsync::begin(const char* host, uint16_t port, const char* path, const char* protocol) {
  begin(String(host), port, String(path), String(protocol));
}
void My_WebSocketClientAsync::stop() {
  My_client.stop();
}
void My_WebSocketClientAsync::begin(String host, uint16_t port, String path, String protocol) {
  _host = host;
  _port = port;
  _path = path;
  _protocol = protocol;
  
  if (_state == WS_DISCONNECTED) {
    _state = WS_CONNECTING;
    _generateKey();
  }
}

void My_WebSocketClientAsync::end() {
  if (_state == WS_CONNECTED) {
    _state = WS_DISCONNECTING;
    send(WS_OP_CLOSE, nullptr, 0);
  } 
  
  if (_rxBuffer) {
    free(_rxBuffer);
    _rxBuffer = nullptr;
    _rxBufferSize = 0;
  }
  
  // if (My_client->connected()) {
  //   My_client->stop();
  // }
  
  _state = WS_DISCONNECTED;
}

void My_WebSocketClientAsync::loop() {
  My_client.setInsecure();
  if (_state == WS_CONNECTING) 
  {
    Serial.println("wifi_conncted");
    if (My_client.connect("iat-api.xfyun.cn", 443)) {
      delay(10);
       Serial.println("HTTP connect");
      _handshake();
    } else {
      Serial.println("HTTP erro");
      _handleError("Connection failed");
      _state = WS_DISCONNECTED;
    }
    return;
  }
  
  if (_state == WS_CONNECTED) {
    if (!My_client.connected()) {
      _state = WS_DISCONNECTED;
      if (_disconnectHandler) {
        Serial.println("\ninto _disconnectHandler");
        _disconnectHandler();
      }
      return;
    }  
    if (My_client.available()) {
      //_handleFrame();
    }
  }
  else{
    Serial.println("connected out");
  }
}
bool My_WebSocketClientAsync::read(){
  _handleFrame();
  return 1;
}
bool My_WebSocketClientAsync::send(WebSocketOpcode opcode, uint8_t* payload, size_t length, bool mask) {
  if (_state != WS_CONNECTED) {
    return false;
  }
  
  return _sendFrame(opcode, payload, length, mask);
}

bool My_WebSocketClientAsync::send(String str) {
  return send(WS_OP_TEXT, (uint8_t*)str.c_str(), str.length());
}

bool My_WebSocketClientAsync::send(char* str) {
  return send(WS_OP_TEXT, (uint8_t*)str, strlen(str));
}

bool My_WebSocketClientAsync::send(uint8_t* payload, size_t length) {
  return send(WS_OP_BINARY, payload, length);
}

bool My_WebSocketClientAsync::ping(uint8_t* payload, size_t length) {
  return send(WS_OP_PING, payload, length);
}

void My_WebSocketClientAsync::onConnect(WSConnectedHandler handler) {
  _connectHandler = handler;
}

void My_WebSocketClientAsync::onDisconnect(WSDisconnectedHandler handler) {
  _disconnectHandler = handler;
}

void My_WebSocketClientAsync::onData(WSDataHandler handler) {
  _dataHandler = handler;
}

void My_WebSocketClientAsync::onError(WSErrorHandler handler) {
  _errorHandler = handler;
}

WebSocketClientState My_WebSocketClientAsync::getState() {
  return _state;
}

void My_WebSocketClientAsync::_handleError(const char* error) {
  if (_errorHandler) {
    _errorHandler(error);
    Serial.println((String)"\nerro"+error);
  }
}

bool My_WebSocketClientAsync::_sendFrame(WebSocketOpcode opcode, uint8_t* payload, size_t length, bool mask) {
  if (!My_client.connected()) {
    return false;
  }
  
  // 发送帧头
  uint8_t header[10];
  size_t headerSize = 2;
  
  header[0] = 0x80 | (opcode & 0x0F); // FIN + opcode
  
  if (length < 126) {
    header[1] = (mask ? 0x80 : 0x00) | (length & 0x7F);
  } else if (length < 65536) {
    header[1] = (mask ? 0x80 : 0x00) | 126;
    header[2] = (length >> 8) & 0xFF;
    header[3] = length & 0xFF;
    headerSize += 2;
  } else {
    header[1] = (mask ? 0x80 : 0x00) | 127;
    for (int i = 0; i < 8; i++) {
      header[2 + i] = (length >> (8 * (7 - i))) & 0xFF;
    }
    headerSize += 8;
  }
  
  My_client.write(header, headerSize);

  uint8_t maskKey[4];
  if (mask) {
    maskKey[0] = random(0, 256);
    maskKey[1] = random(0, 256);
    maskKey[2] = random(0, 256);
    maskKey[3] = random(0, 256);
    My_client.write(maskKey, 4);
    
    // 应用掩码到有效载荷
    for (size_t i = 0; i < length; i++) {
      payload[i] ^= maskKey[i % 4];
    }
  }
   //发送有效载荷
  My_client.write(payload, length);
 
  return true;
}
#include <ArduinoJson.h>
#include <String.h>


void My_WebSocketClientAsync::_handleFrame() {
    uint8_t header[10];
    bool fin, masked;
    WebSocketOpcode opcode;
    uint64_t payloadLength = 0;
    uint8_t maskKey[4];
    

    // 读取基础帧头
    if (!_readBytes(header, 2, 1000)) {
        Serial.println("帧头读取失败");
        return;
    }
    fin = (header[0] & 0x80) != 0;
    opcode = static_cast<WebSocketOpcode>(header[0] & 0x0F);
    masked = (header[1] & 0x80) != 0;
    payloadLength = header[1] & 0x7F;

    // 处理扩展长度
    if (payloadLength == 126) {
        if (!_readBytes(header + 2, 2, 1000)) {
            Serial.println("长度126读取失败");
            return;
        }
        payloadLength = (static_cast<uint16_t>(header[2]) << 8) | header[3];
    } else if (payloadLength == 127) {
        if (!_readBytes(header + 2, 8, 1000)) {
            Serial.println("长度127读取失败");
            return;
        }
        payloadLength = 0;
        for (int i = 0; i < 8; i++)
            payloadLength |= static_cast<uint64_t>(header[2 + i]) << (8 * (7 - i));
    }

    // 读取掩码键
    if (masked && !_readBytes(maskKey, 4, 1000)) {
        Serial.println("掩码键读取失败");
        return;
    }

    // 仅处理文本帧
    if (opcode != WS_OP_TEXT) {
        Serial.println("非文本帧，跳过");
        return;
    }

    // 读取并处理Payload
    if (payloadLength > 0) {
        // 分配缓冲区
        if (_rxBufferSize < payloadLength) {
            if (_rxBuffer) free(_rxBuffer);
            _rxBuffer = (uint8_t*)malloc(payloadLength);
            if (!_rxBuffer) {
                Serial.println("缓冲区分配失败");
                return;
            }
            _rxBufferSize = payloadLength;
        }

        // 读取Payload
        if (!_readBytes(_rxBuffer, payloadLength, 1000)) {
            Serial.println("Payload读取失败");
            return;
        }

        // 解密掩码
        if (masked) {
            for (size_t i = 0; i < payloadLength; i++) {
                _rxBuffer[i] ^= maskKey[i % 4];
            }
        }
       
       Serial.printf("code=%s", _rxBuffer);  // 使用格式化字符串，%s 表示字符串
       delay(10);
        // 核心逻辑：通过2字节和3字节UTF-8编码范围提取汉字
        for (size_t i = 0; i < payloadLength; ) {
            uint8_t b1 = _rxBuffer[i];

            // 1. 提取2字节UTF-8汉字（范围：0xC0~0xDF 且 0x80~0xBF）
            if (b1 >= 0xC0 && b1 <= 0xDF) {
                if (i + 1 < payloadLength) {  // 确保有第二个字节
                    uint8_t b2 = _rxBuffer[i + 1];
                    if (b2 >= 0x80 && b2 <= 0xBF) {  // 第二个字节符合范围
                        chineseResult += (char)b1;
                        chineseResult += (char)b2;
                        i += 2;  // 跳过这两个字节
                        continue;
                    }
                }
            }

            // 2. 提取3字节UTF-8汉字（范围：0xE0~0xEF 且后续两字节0x80~0xBF）
            else if (b1 >= 0xE0 && b1 <= 0xEF) {
                if (i + 2 < payloadLength) {  // 确保有第三个字节
                    uint8_t b2 = _rxBuffer[i + 1];
                    uint8_t b3 = _rxBuffer[i + 2];
                    if (b2 >= 0x80 && b2 <= 0xBF && b3 >= 0x80 && b3 <= 0xBF) {
                        chineseResult += (char)b1;
                        chineseResult += (char)b2;
                        chineseResult += (char)b3;
                        i += 3;  // 跳过这三个字节
                        continue;
                    }
                }
            }

            // 非汉字字节，跳过
            i++;
        }

        // 输出提取到的汉字
        if (chineseResult.length() > 0) {
            Serial.println("\n===== 识别结果 =====");
            Serial.println(chineseResult);  // 直接输出所有提取的汉字
            
            Serial.println("====================\n");
        } else {
            Serial.println("未提取到任何汉字");
        }
    }

    // 处理控制帧
    if (opcode == WS_OP_PING) send(WS_OP_PONG, _rxBuffer, payloadLength);
    else if (opcode == WS_OP_CLOSE && _state == WS_CONNECTED) {
        _state = WS_DISCONNECTING;
        send(WS_OP_CLOSE, nullptr, 0);
    }
}

// 参数：
//   buffer: 接收数据的缓冲区
//   length: 需要读取的字节数
//   timeout: 超时时间（单位：毫秒，默认1000ms）
// 返回值：true=读取成功，false=超时或读取失败
bool My_WebSocketClientAsync::_readBytes(uint8_t* buffer, size_t length, unsigned long timeout) {
  if (!My_client.connected()) {
        Serial.println("My_client is NOT connected (read failed)");  // 新增日志
        _handleError("_readBytes: client not connected");
        return false;
    }
    if (buffer == nullptr || length == 0) {
        _handleError("_readBytes: buffer is null or length is 0");
        return false;
    }
    unsigned long startTime = millis();  // 记录开始时间（用于超时判断）
    size_t bytesRead = 0;                // 已读取的字节数

    // 循环读取，直到读满指定长度或超时
    while (bytesRead < length) {
        // 检查是否有数据可读取
        if (My_client.available() > 0) {
            // buffer[bytesRead++] = My_client.read();  // 读取1字节到缓冲区
            My_client.read(buffer,1);
            buffer++;
            bytesRead++;
        }

        //检查是否超时
        if (millis() - startTime > timeout) {
            _handleError("_readBytes: timeout");
            Serial.printf("_readBytes timeout: expected %d bytes, read %d bytes\n", length, bytesRead);
            return false;
        }
      
        delay(1);  // 轻微延时，避免占用过多CPU
    }
    Serial.printf("read end");
    // 读取到指定长度，返回成功
    return true;
}

void My_WebSocketClientAsync::_generateKey() {
  uint8_t key[16];
  for (int i = 0; i < 16; i++) {
    key[i] = random(0, 256);
  }
  _key = _base64Encode(key, 16);
}

String My_WebSocketClientAsync::_base64Encode(uint8_t* data, size_t length) {
  String result = base64::encode(data, length);
  result.replace("\n", ""); // 移除换行符
  return result;
}

void My_WebSocketClientAsync::_handshake() {
  String request = "GET " + _path + " HTTP/1.1\r\n";
  request += "Host: " + _host + ":" + _port + "\r\n";
  request += "Upgrade: websocket\r\n";
  request += "Connection: Upgrade\r\n";
  request += "Sec-WebSocket-Key: " + _key + "\r\n";
  request += "Sec-WebSocket-Version: 13\r\n";
  if (_protocol.length() > 0) {
    request += "Sec-WebSocket-Protocol: " + _protocol + "\r\n";
  }
  request += "\r\n";
  
  My_client.print(request);
//  Serial.print(request);
  // 读取响应
  // String response=My_client.readString();
  // Serial.print(response);
  // unsigned long start = millis();
  // while (millis() - start < 5000) {
  //   if (My_client.available()) {
  //     char c = My_client.readString();
  //     response += c;
  //     Serial.print(response);
  //     if (response.endsWith("\r\n\r\n")) {
  //       break;
  //     }
  //   }
  // }
    uint8_t buffer[1024]; // 定义一个足够大的缓冲区
    size_t read = My_client.readBytes(buffer, sizeof(buffer)); // 读取可用数据
     String receivedStr;
    for (size_t i = 0; i < read; i++) {
      receivedStr += (char)buffer[i];
    }
    Serial.println("Received: " + receivedStr);
//   // 检查响应
  // if (response.indexOf("HTTP/1.1 101") == -1) {
  //   _handleError("Invalid handshake response");
  //   My_client.stop();
  //   _state = WS_DISCONNECTED;
  //   return;
  // }
  

  String expectedAccept = _key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  
  br_sha1_context sha1Context;
  uint8_t sha1Hash[20];
  
  br_sha1_init(&sha1Context);
  br_sha1_update(&sha1Context, expectedAccept.c_str(), expectedAccept.length());
  br_sha1_out(&sha1Context, sha1Hash);
  
  String expectedKey = _base64Encode(sha1Hash, 20);
  
  String actualKey;
  int keyIndex = receivedStr.indexOf("Sec-WebSocket-Accept: ");
  if (keyIndex != -1) {
    keyIndex += 22;
    int endIndex = receivedStr.indexOf("\r\n", keyIndex);
    actualKey = receivedStr.substring(keyIndex, endIndex);
  }
  
  if (actualKey != expectedKey) {
    _handleError("Invalid Sec-WebSocket-Accept key");
    _client->stop();
    _state = WS_DISCONNECTED;
    return;
  }
  else{
    Serial.println("handshake success");
  }
  _state = WS_CONNECTED;
  if (_connectHandler) {
    _connectHandler();
  }
}


String My_WebSocketClientAsync::get_time(String tim_url) {
  My_client.setInsecure();
  if(My_client.connect(tim_url.c_str(), 443))
  {
    Serial.println("conect with day http success");
    My_client.print((String) "POST " + "/api/time" + " HTTP/1.1\r\n");  //地址处理
    My_client.print((String) "Host: "+tim_url+"\r\n\r\n");
    if (My_client.connected()) {
        Serial.println("connected");
        //String send_back = My_client.readStringUntil('}')+"}";
         String send_back = My_client.readString();
         Serial.println(send_back);
      String dateHeader = extractDateHeader(send_back);
      if(dateHeader.length() > 0) {
        return dateHeader;
      } else {
        Serial.println("Date header not found in response");
      }
    } else {
      Serial.print("conected erero\n");
    }
  }
  else{
    Serial.println("conect with day http error");
  }
  My_client.stop();
}



String extractDateHeader(const String& httpResponse) {
    int dateStart = httpResponse.indexOf("Date: ");
    if (dateStart == -1) return "";
    
    dateStart += 6; // 跳过"Date: "前缀
    int dateEnd = httpResponse.indexOf("\r\n", dateStart);
    if (dateEnd == -1) dateEnd = httpResponse.length();
    
    // 修正：先保存substring结果，再trim
    String dateStr = httpResponse.substring(dateStart, dateEnd);
    dateStr.trim();
    return dateStr;
}