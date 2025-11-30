#line 1 "D:\\01-同步文件夹\\Arduino\\mini_test\\xunfei_sound.cpp"
#include "xunfei_sound.h"
const char *sound_host = "iat-api.xfyun.cn";
const char *sound_url = "wss://iat-api.xfyun.cn/v2/iat";
const char *sound_secret = "OGUxZDY1MzZjMTg4NDg1ZGVmNTlhMGY3";

String sound_authorization;
const char *_apiKey = "9e19e0d4291c76376aa9b3201a81aee0";
const char *_apiid = "95182ee1";

String urlEncode(const String &str) {
  String encoded = "";
  char c;
  for (size_t i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      // 未保留字符直接保留
      encoded += c;
    } else {
      // 其他字符（包括空格）转为 %XX
      encoded += '%';
      encoded += hexDigit(c >> 4);    // 高4位转16进制
      encoded += hexDigit(c & 0x0F);  // 低4位转16进制
    }
  }
  return encoded;
}

char hexDigit(uint8_t d) {
  return d < 10 ? '0' + d : 'A' + d - 10;
}
String generateSignature(const String &key, const String &message) {
  br_hmac_key_context kc;
  br_hmac_context ctx;
  uint8_t hmacResult[32];

  // 计算HMAC-SHA256
  br_hmac_key_init(&kc, &br_sha256_vtable, key.c_str(), key.length());
  br_hmac_init(&ctx, &kc, 0);
  br_hmac_update(&ctx, message.c_str(), message.length());
  br_hmac_out(&ctx, hmacResult);

  // Base64编码并手动添加填充
  String base64Str = base64::encode(hmacResult, 32);

  // 确保长度是4的倍数（32字节->44字符）
  while (base64Str.length() % 4 != 0) {
    base64Str += '=';
  }

  return base64Str;
}
String get_xurl(String sound_date){

  String requestLine = "GET /v2/iat HTTP/1.1";  // 注意包含 "HTTP/1.1"
    String tmp = (String) "host: " + sound_host + "\n" + "date: " + sound_date + "\n" + requestLine;
    Serial.println(tmp);

    String signature = generateSignature(sound_secret, tmp);
    Serial.println(signature);
    String auth = (String) "api_key=\"" + _apiKey + "\", "
                  + "algorithm=\"hmac-sha256\","
                  + "headers=\"host date request-line\", "
                  + " signature=\""
                  + signature + "\"";
    Serial.println("\n" + auth);
    String authorization = base64::encode((uint8_t *)auth.c_str(), auth.length());
    authorization.replace("\n", "");  // 移除可能的换行符
    Serial.println("Final Authorization: " + authorization);
 
    String params = "authorization=" + urlEncode(authorization) + "&date=" + urlEncode(sound_date) + "&host=" + urlEncode("iat-api.xfyun.cn");
    // String params = "host="+ urlEncode("iat-api.xfyun.cn")+"&date=" + urlEncode(sound_date)+"&authorization="+ urlEncode(authorization);
    // 拼接完整URL
    String wsUrl = "wss://iat-api.xfyun.cn/v2/iat?" + params;
    String wsUrl_copy = "/v2/iat?" + params;
    Serial.println("Final WebSocket URL:");
    Serial.println(wsUrl);
    // /*client sent-------------------------------------*/
    Serial.println("go http client\n");


    /*test sha256 base64加密-------------------------*/
    String temp = (String) "host: " + "iat-api.xfyun.cn" + "\n" + "date: " + "Wed, 10 Jul 2019 07:35:43 GMT" + "\n" + "GET /v2/iat HTTP/1.1";
    String temp_ws = generateSignature("secretxxxxxxxx2df7900c09xxxxxxxx", temp);
    Serial.println((String) "temp_WS = " + temp_ws);
    return wsUrl;
}

String soundreq(
  const String &audioBase64,
  int status) {
  const String &appId = _apiid;
  const String &language = "zh_cn";
  const String &domain = "iat";
  const String &accent = "mandarin";
  const String &dwa = "wpgs";
  const String &format = "audio/L16;rate=8000";
  const String &encoding = "speex";
  const int speex_size= 38*2;
  DynamicJsonDocument doc(1024);  // 根据实际数据大小调整
 
  if(status==0){
    // 公共参数
    JsonObject common = doc.createNestedObject("common");
    common["app_id"] = appId;

    // 业务参数
    JsonObject business = doc.createNestedObject("business");
    business["language"] = language;
    business["domain"] = domain;
    business["accent"] = accent;
    business["dwa"] = dwa;
    // business["speex_size"] = audioBase64.length();
    business["speex_size"] = speex_size;
    // 音频数据
    JsonObject data = doc.createNestedObject("data");
    data["status"] = status;
    data["format"] = format;
    data["encoding"] = encoding;
    data["audio"] = audioBase64;
  }
  else if(status==1){
    // 音频数据
    JsonObject data = doc.createNestedObject("data");
    data["status"] = status;
    data["format"] = format;
    data["encoding"] = encoding;
    data["audio"] = audioBase64;
  }
  else if(status==2){
    JsonObject data = doc.createNestedObject("data");
    data["status"] = status;
  }
  String output;
  serializeJson(doc, output);
  return output;
}
String soudreq_stop() {
  DynamicJsonDocument doc(64);
  JsonObject data = doc.createNestedObject("data");
  data["status"] = 2;  // 结束标识
  String output;
  serializeJson(doc, output);
  return output;
}
void sound_json(String jsonString) {
   int keyIndex = jsonString.indexOf("{");
  if (keyIndex != -1) {
    keyIndex += 22;
    int endIndex = jsonString.indexOf("}}}", keyIndex);
    jsonString = "{"+jsonString.substring(keyIndex, endIndex)+"}}}";
  }

  DynamicJsonDocument doc(1024);  // 根据JSON大小调整缓冲区
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
    Serial.print("JSON解析失败: ");
    Serial.println(error.c_str());
    return;
  }

  // 检查基础字段
  if (doc["code"] != 0) {
    Serial.println("接口返回错误");
    return;
  }
  // 提取data部分
  JsonObject data = doc["data"];
  int status = data["status"];  // 1=识别中, 2=识别结束

  // 提取result部分
  JsonObject result = data["result"];
  int sn = result["sn"];            // 句子序号
  bool ls = result["ls"];           // 是否最后一句
  const char *pgs = result["pgs"];  // 段落状态

  // 拼接识别文本
  String recognizedText = "";
  JsonArray ws = result["ws"];
  for (JsonObject wordObj : ws) {
    JsonArray cw = wordObj["cw"];
    const char *word = cw[0]["w"];  // 取第一个候选词
    recognizedText += word;
  }

  // 打印结果f
  Serial.println("======== 识别结果 ========");
  Serial.printf("状态: %s\n", status == 1 ? "识别中" : "识别结束");
  Serial.printf("句子序号: %d\n", sn);
  Serial.printf("是否最后一句: %s\n", ls ? "是" : "否");
  Serial.printf("段落状态: %s\n", pgs);
  Serial.printf("识别内容: %s\n", recognizedText.c_str());
  Serial.println("========================");
}