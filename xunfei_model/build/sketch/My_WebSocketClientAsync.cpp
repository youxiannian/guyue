#line 1 "D:\\01-同步文件夹\\Arduino\\mini_test\\My_WebSocketClientAsync.cpp"
#include "My_WebSocketClientAsync.h"
#include <base64.h>
#include <BearSSLHelpers.h>  
#include <ArduinoJson.h>
#include "xunfei_sound.h"
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
 
   
  //  uint8_t buffer[1024]; // 定义一个足够大的缓冲区
  //   size_t read = My_client.readBytes(buffer, sizeof(buffer)); // 读取可用数据
  //    String receivedStr;
  //   for (size_t i = 0; i < read; i++) {
  //     receivedStr += (char)buffer[i];
  //   }
  //   Serial.println("send_back\n: " + receivedStr);
  //   sound_json(receivedStr);
  //_handleFrame();
  return true;
}

// 你的类中可能需要的成员变量（如果没有请添加到.h文件）
// uint8_t* _rxBuffer = nullptr;
// size_t _rxBufferSize = 0;
// 假设已有_dataHandler回调函数指针（用于处理解析后的有效数据）

void My_WebSocketClientAsync::_handleFrame() {
    uint8_t header[10];
    // 1. 打印：开始解析帧头
    Serial.println("\n===== 开始解析WebSocket帧 =====");
    
    // 读取帧头前2字节（基础帧头）
    if (!_readBytes(header, 2, 1000)) {  
        _handleError("Failed to read frame header");
        Serial.print("not send back");
        Serial.println("===== 帧解析失败（帧头读取超时） =====");
        return;
    }

    // 2. 打印：帧头前2字节（16进制）
    Serial.print("1. 帧头前2字节（16进制）：");
    for (int i = 0; i < 2; i++) {
        if (header[i] < 0x10) Serial.print("0"); // 补前导0
        Serial.print(header[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    // 解析帧头核心信息
    bool fin = (header[0] & 0x80) != 0;  
    WebSocketOpcode opcode = static_cast<WebSocketOpcode>(header[0] & 0x0F);  
    bool masked = (header[1] & 0x80) != 0;  
    uint64_t payloadLength = header[1] & 0x7F;  

    // 3. 打印：帧头解析结果
    Serial.println("2. 帧头解析结果：");
    Serial.print("   - 是否完整帧（FIN）：");
    Serial.println(fin ? "是" : "否");
    
    Serial.print("   - 帧类型（opcode）：");
    switch(opcode) {
        case WS_OP_TEXT: Serial.println("文本帧"); break;
        case WS_OP_BINARY: Serial.println("二进制帧"); break;
        case WS_OP_PING: Serial.println("PING帧"); break;
        case WS_OP_PONG: Serial.println("PONG帧"); break;
        case WS_OP_CLOSE: Serial.println("关闭帧"); break;
        default: 
            Serial.print("未知（0x");
            if (opcode < 0x10) Serial.print("0");
            Serial.print(opcode, HEX);
            Serial.println("）");
            break;
    }
    
    Serial.print("   - 是否带掩码（MASK）：");
    Serial.println(masked ? "是" : "否");
    
    Serial.print("   - 初始Payload长度：");
    Serial.println(payloadLength);


    // 4. 处理扩展长度（126/127）
    if (payloadLength == 126) {
        Serial.println("3. 检测到长度标记126，读取2字节扩展长度...");
        if (!_readBytes(header + 2, 2, 1000)) {  
            _handleError("Failed to read extended length (126)");
            Serial.println("===== 帧解析失败（扩展长度超时） =====");
            return;
        }
        // 打印扩展长度字节
        Serial.print("   - 扩展长度字节（16进制）：");
        for (int i = 2; i < 4; i++) {
            if (header[i] < 0x10) Serial.print("0");
            Serial.print(header[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        // 计算实际长度
        payloadLength = (static_cast<uint16_t>(header[2]) << 8) | header[3];
        Serial.print("   - 实际Payload长度：");
        Serial.println(payloadLength);

    } else if (payloadLength == 127) {
        Serial.println("3. 检测到长度标记127，读取8字节扩展长度...");
        if (!_readBytes(header + 2, 20, 1000)) {
            _handleError("Failed to read extended length (127)");
            Serial.println("===== 帧解析失败（扩展长度超时） =====");
            return;
        }
        // 打印扩展长度字节
        Serial.print("   - 扩展长度字节（16进制）：");
        for (int i = 2; i < 10; i++) {
            if (header[i] < 0x10) Serial.print("0");
            Serial.print(header[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        // 计算实际长度
        payloadLength = 0;
        for (int i = 0; i < 8; i++) {
            payloadLength |= static_cast<uint64_t>(header[2 + i]) << (8 * (7 - i));
        }
        Serial.print("   - 实际Payload长度：");
        Serial.println(payloadLength);
    }


    // 5. 读取掩码键
    uint8_t maskKey[4];
    if (masked) {
        Serial.println("4. 读取4字节掩码键...");
        if (!_readBytes(maskKey, 4, 1000)) {
            _handleError("Failed to read mask key");
            Serial.println("===== 帧解析失败（掩码键超时） =====");
            return;
        }
        // 打印掩码键
        Serial.print("   - 掩码键（16进制）：");
        for (int i = 0; i < 4; i++) {
            if (maskKey[i] < 0x10) Serial.print("0");
            Serial.print(maskKey[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    } else {
        Serial.println("4. 无掩码，跳过掩码键读取");
    }


    // 6. 读取并处理Payload
    if (payloadLength > 0) {
        Serial.print("5. 读取Payload（长度：");
        Serial.print(payloadLength);
        Serial.println(" 字节）...");
        
        // 确保缓冲区足够
        if (_rxBufferSize < payloadLength) {
            if (_rxBuffer) free(_rxBuffer);  
            _rxBuffer = (uint8_t*)malloc(payloadLength);  
            if (!_rxBuffer) {
                _handleError("Failed to allocate RX buffer");
                Serial.println("===== 帧解析失败（缓冲区分配失败） =====");
                return;
            }
            _rxBufferSize = payloadLength;
            Serial.print("   - 分配缓冲区成功（大小：");
            Serial.print(payloadLength);
            Serial.println(" 字节）");
        }

        // 读取Payload
        if (!_readBytes(_rxBuffer, payloadLength, 1000)) {
            _handleError("Failed to read payload");
            Serial.println("===== 帧解析失败（Payload超时） =====");
            return;
        }
        Serial.println("   - Payload读取成功");

        // 解密掩码
        if (masked) {
            Serial.println("   - 开始Payload掩码解密...");
            for (size_t i = 0; i < payloadLength; i++) {
                _rxBuffer[i] ^= maskKey[i % 4];  
            }
            Serial.println("   - 解密完成");
        }

        // 打印Payload（关键修正：直接逐字节打印文本，不构造String）
        Serial.println("6. Payload内容：");
        // 文本格式（直接打印char*缓冲区）
      Serial.print("   - 文本格式：");
      Serial.print((const char*)_rxBuffer);
        // for (size_t i = 0; i < payloadLength; i++) {
        //     // 只打印可显示字符（避免乱码字符刷屏）
        //     if (_rxBuffer[i] >= 0x20 && _rxBuffer[i] <= 0x7E) {
        //         Serial.write(_rxBuffer[i]);
        //     } else {
        //         Serial.print("."); // 不可显示字符用"."代替
        //     }
        // }
        Serial.println();

        // 16进制格式（前20字节）
        Serial.print("   - 16进制格式（前20字节）：");
        size_t printLen = (payloadLength > 20) ? 20 : payloadLength;
        for (size_t i = 0; i < printLen; i++) {
            if (_rxBuffer[i] < 0x10) Serial.print("0");
            Serial.print(_rxBuffer[i], HEX);
            Serial.print(" ");
        }
        if (payloadLength > 20) Serial.print("...");
        Serial.println();

    } else {
        Serial.println("5. Payload长度为0，跳过读取");
    }

    delay(2);
    delay(2);

    // 7. 处理帧类型
    Serial.println("7. 处理帧数据...");
    switch (opcode) {
        case WS_OP_TEXT:
            Serial.println("   - 处理文本帧：调用_dataHandler");
            if (_dataHandler && fin) {  
                _dataHandler(opcode, _rxBuffer, payloadLength);
            }
            break;

        case WS_OP_BINARY:
            Serial.println("   - 处理二进制帧：暂不处理");
            break;

        case WS_OP_PING:
            Serial.println("   - 处理PING帧：回复PONG");
            send(WS_OP_PONG, _rxBuffer, payloadLength);
            break;

        case WS_OP_PONG:
            Serial.println("   - 处理PONG帧：无需操作");
            break;

        case WS_OP_CLOSE:
            Serial.println("   - 处理关闭帧：切换断开状态");
            if (_state == WS_CONNECTED) {
                _state = WS_DISCONNECTING;
                send(WS_OP_CLOSE, nullptr, 0);  
            }
            break;

        default:
            Serial.println("   - 处理未知帧：忽略");
            break;
    }

    // 8. 解析结束
    Serial.println("===== WebSocket帧解析完成 =====\n");
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