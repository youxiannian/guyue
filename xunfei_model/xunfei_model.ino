#include "My_WebSocketClientAsync.h"
#include "xunfei_sound.h"
#include <time.h>
#include <libb64/cencode.h>
#include <CustomJWT.h>
#include <BearSSLHelpers.h>
#include <string.h>
#include <U8g2lib.h>
#include <ScrollTextDisplay.h>
#include <base64.h> 
#include "flash_store.h"

#define SERIAL_RX_BUFFER_SIZE 39*2
#define SAMPLE_RATE 16000
#define FRAME_SIZE 256
#define BUFFER 2048
#define JITTER_STEP_MS 20
#define RESAMPLE_RATE 8000
#define BUFFEFR 200
#define MAX_FLASH_LOC 4040
#define MAX_PAGE 0x105
#define INIT_PAGE 0xff

void Flash_Init(uint16_t Addr_Sec){
  spi_flash_erase_sector (Addr_Sec);
}
int Flash_Write(uint16_t Addr_Sec,uint16_t Addr_Loc,uint8_t *byte,uint16_t length){
  if(Addr_Loc>4096)
    return -1;
  spi_flash_write(Addr_Sec*4096+Addr_Loc,(uint32_t *)byte,length);
  return 1;
}
int Flash_Read(uint16_t Addr_Sec,uint16_t Addr_Loc,uint8_t *byte,uint16_t length){
  if(Addr_Loc>4096)
    return -1;
  spi_flash_read(Addr_Sec*4096+Addr_Loc,(uint32_t *)byte,length);
  return 1;
}



BearSSL::WiFiClientSecure client;
String startRequest;
My_WebSocketClientAsync wsClient(true);  // true for secure connection
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
ScrollTextDisplay scrollDisplay(u8g2);
int Page_Num = INIT_PAGE;
int Page_Read_Num = INIT_PAGE;
const char *ssid = "guyue";
const char *password = "guyue123."; 
int rtr,wtr;
static int num_sound=0;
String tim_url = "api.uuni.cn";
String Chinese;

//char speex_word[BUFFEFR][39] PROGMEM= {};
void String_to_Uint8(String Str,uint8_t * Cha){
      for(int i=0;i<38;i++){
        String byte_str = Str.substring(i*2,i*2+2);
        Cha[i] = (uint8_t)strtol(byte_str.c_str(),NULL,16);
      }
}


uint8_t flash_read_data[40]={0};
uint8_t uint8_data[40]={0};

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);  // 输出底层WiFi和SSL日志
  Serial.println("AEC initialized");
  
  //timer1.attach(1,timer1_rec);
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  u8g2.setCursor(0, 15);
  u8g2.print("hello world");
  u8g2.sendBuffer();
  system_soft_wdt_feed();

  Flash_Init(Page_Num);
  while(1){
  if(Serial.available()){
    String data= Serial.readStringUntil('\n');
    if (data!= "" &&data.length() > 0) {
      String_to_Uint8(data,uint8_data);      
      if(((uint8_data[0]&0xf0)!=0x20)){
        Serial.print("end speex input");
        Serial.printf("\n page_read=%02X page_num=%02X rtr=%d wtr=%d \n",Page_Read_Num,Page_Num,rtr,wtr);
        break;
      }
      Flash_Write(Page_Num,wtr,uint8_data,40);
      delay(2);
      system_soft_wdt_feed();
      if(wtr==MAX_FLASH_LOC)
      {
        Serial.printf("page_num = 0x%02x\n ", Page_Num);
        if(Page_Num==MAX_PAGE) 
          break;
        Page_Num+=1;
        Flash_Init(Page_Num);
        wtr=0;
      }
      else wtr+=40;
    }
  }
}
  connectWiFi(ssid, password);
  client.setInsecure();
  String sound_date = wsClient.get_time(tim_url);
  String wsUrl = get_xurl(sound_date);
  wsClient.begin("iat-api.xfyun.cn",443,wsUrl.c_str());
  wsClient.loop();

  while(Page_Read_Num!=MAX_PAGE)
    {
      if((Page_Read_Num==Page_Num)&&(rtr==wtr)) break;
      Flash_Read(Page_Read_Num,rtr,flash_read_data,40);
      String encodedStr = base64::encode(flash_read_data,38, false);
      if((flash_read_data[0]&0xff)==0xff){
        break;
      }
      encodedStr.replace("\n", "");  // 移除可能的换行符
      delay(1);
      for(int i=0;i<38;i++){
        Serial.printf("%02X", flash_read_data[i]);
      }
      Serial.printf("\n");
      //uint8 speex[79];
      // for(int i=0;i<38;i++){
      // }
      if(num_sound==0){
        Serial.println("bengin send speex");
        startRequest = soundreq(encodedStr, num_sound);
        wsClient.send(startRequest);
        num_sound=1;
      }
      else if(num_sound==1) 
      {
          startRequest = soundreq(encodedStr, num_sound);
           wsClient.send(startRequest);   
      }
      if(rtr==MAX_FLASH_LOC)
      {
        Page_Read_Num+=1;
        Flash_Init(Page_Num);
        rtr=0;
      }
      else  rtr+=40;
    }
    String encodedStr_stop;
    startRequest = soundreq(encodedStr_stop,2);
    wsClient.send(startRequest);
    wsClient.read();
    Chinese = Sound_Chinese();
    Serial.println(Chinese);
    wsClient.stop();
    // connectWiFi(ssid, password);
    // client.setInsecure();
    httpReq();    
}

void loop() {
   {
    
    // if(rtr!=wtr)
    // {
    //    for(int i=0;i<38;i++){
    //     send_data[i] = (uint8_t)strtol(speex_word[wtr],NULL,16);
    //   }
    //   String encodedStr = base64::encode(send_data,38, false);
    //   encodedStr.replace("\n", "");  // 移除可能的换行符
    //   if(num_sound==0){
    //     Serial.println("bengin send speex");
    //     startRequest = soundreq(encodedStr, num_sound);
    //     wsClient.send(startRequest);
    //     num_sound=1;
    //   }
    //   else if(num_sound==1) 
    //   {
    //       startRequest = soundreq(encodedStr, num_sound);
    //        wsClient.send(startRequest);   
    //   }
    //   wtr++;
    // }
    // else if(num_sound==1&&rtr==wtr){
    //   String encodedStr_stop;
    //   startRequest = soundreq(encodedStr_stop,2);
    //   wsClient.send(startRequest);
    //   wsClient.read();
    //   num_sound=3;
    // }
    
    // if(rtr!=wtr)
    // {
    //   Serial.println(data[wtr]); // 可选：确认接收到的数据
    //   uint8_t binary_data[38];
    //   for(int i=0;i<38;i++){
    //     String byte_str = data[wtr].substring(i*2,i*2+2);
    //     binary_data[i] = (uint8_t)strtol(byte_str.c_str(),NULL,16);
    //   }
    //   String encodedStr = base64::encode(binary_data,38, false);
    //   encodedStr.replace("\n", "");  // 移除可能的换行符
    //   if(num_sound==0){
    //     Serial.println("bengin send speex");
    //     startRequest = soundreq(encodedStr, num_sound);
    //     wsClient.send(startRequest);
    //     num_sound=1;
    //   }
    //   else if(num_sound==1)
    //   {
    //       startRequest = soundreq(encodedStr, num_sound);
    //        wsClient.send(startRequest);
    //   }
    //   wtr++;
    //   delay(30);
    // }
    // else if(rtr==wtr&&num_sound==1){
    //     String encodedStr_stop;
    //     startRequest = soundreq(encodedStr_stop,2);
    //     wsClient.send(startRequest);
    //     wsClient.read();
    //     num_sound=3;
    // }

    
  // u8g2.setCursor(0, 30);
  // u8g2.print(wtr);
  // u8g2.sendBuffer();
    }
}
const char *host = "spark-api-open.xf-yun.com";
const int httpsPort = 443;
const char *apiPassword = "WtvTnWVoQALqASdIRkxU:ffdAmQSlsoTkRPzAdQfl"; 

void httpReq() {
  Serial.println("\nConnecting to server...");
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }

  // 构建JSON请求体
  DynamicJsonDocument requestDoc(1024);
  requestDoc["model"] = "x1";
  requestDoc["user"] = "youxiannian";
  
  JsonArray messages = requestDoc.createNestedArray("messages");
  JsonObject msg = messages.createNestedObject();
  msg["role"] = "user";
  msg["content"] =  Chinese.c_str();

  JsonArray tools = requestDoc.createNestedArray("tools");
  JsonObject searchTool = tools.createNestedObject();
  searchTool["type"] = "web_search";
  searchTool["web_search"]["enable"] = true;
  searchTool["web_search"]["search_mode"] = "normal";

  String requestBody;
  serializeJson(requestDoc, requestBody);

  client.print(String("POST /v2/chat/completions HTTP/1.1\r\n") +
               "Host: " + host + "\r\n" +
               "Authorization: Bearer " + apiPassword + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length: " + requestBody.length() + "\r\n\r\n" +
               requestBody);

  Serial.println("Request sent");
  Serial.println("Waiting for response...");

  // 处理响应
  unsigned long timeout = millis();
  while (1) {
    String line = client.readStringUntil('\n');
    if (line.startsWith("{")) {
      Serial.println("\nResponse:");
      Serial.println(line);
      
      DynamicJsonDocument responseDoc(2048);
      DeserializationError error = deserializeJson(responseDoc, line);
      
      if (!error) {
        const char* content = responseDoc["choices"][0]["message"]["content"];
        const char* ai_responsse =responseDoc["choices"][0]["message"]["reasoning_content"];
        Serial.println("\nAI回复:");
        Serial.println(content);
        scrollDisplay.setText(content);      
      } else {
        Serial.print("JSON解析错误: ");
        Serial.println(error.c_str());
      }
      break;
    }
    delay(10);
  }
  client.stop();
  Serial.println("Connection closed");
}