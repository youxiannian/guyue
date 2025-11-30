#ifndef XUNFEI_SOUND_H
#define XUNFEI_SOUND_H
#include "My_WebSocketClientAsync.h"
#include <base64.h>
#include <ArduinoJson.h>

String soundreq(
  const String &audioBase64,
  int status);
String urlEncode(const String &str);
char hexDigit(uint8_t d);
String get_xurl(String sound_date);
String soudreq_stop();
void sound_json(String jsonString);
#endif