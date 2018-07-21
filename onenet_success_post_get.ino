/*
HTTP协议实现post和get发往onenet平台。
post为温湿度上传。
get为开关消息获取并执行。
阿正修改，资源源于网络。
个人博客：http://wenzheng.club/
*/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#define LED 14
#define DEBUG 0
#define ledPin 14                          // 定义ledPin连接到D5/GPIO14
#define DHTPIN 12                         // what digital pin we're connected to; D6 GPIO12
#define DHTTYPE DHT11
int ds;
const char* ssid     = "maker_space";         // XXXXXX -- 使用时请修改为当前你的 wifi ssid
const char* password = "chuangke666";     // XXXXXX -- 使用时请修改为当前你的 wifi 密码
const char* host = "api.heclouds.com";
const char* APIKEY = "dDu2j8l23dNMJSWv5XVqfc6hXoM=";    // API KEY
int32_t deviceId = 28315875;                             // Device ID
const char* DataStreams = "led";                // 数据流
const char* DS_Temp = "wendu";                        // 数据流 - Temp
const char* DS_Baojing = "w_baojing";
const char* DS_Hum = "shidu";
const size_t MAX_CONTENT_SIZE = 1024;
const unsigned long HTTP_TIMEOUT = 2100;               // max respone time from server
WiFiClient client;
const int tcpPort = 80;
DHT dht(DHTPIN, DHTTYPE);
struct UserData {
  int errno_val;                // 错误返回值
  char error[32];               // 错误返回信息
  int test_led_Val;             // TEST LED 状态值
  char udate_at[32];            // 最后更新时间及日期
};

// Skip HTTP headers so that we are at the beginning of the response's body
//  -- 跳过 HTTP 头，使我们在响应正文的开头
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
    Serial.println("No response or invalid response!");
  }
  return ok;
}
// Read the body of the response from the HTTP server -- 从HTTP服务器响应中读取正文
void readReponseContent(char* content, size_t maxSize) {
  //  size_t length = client.peekBytes(content, maxSize);
  size_t length = client.readBytes(content, maxSize);
  delay(20);
  Serial.println(length);
  Serial.println("Get the data from Internet!");
  content[length] = 0;
  Serial.println(content);
  Serial.println("Read Over!");
}
bool parseUserData_test(char* content, struct UserData* userData) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  //  -- 根据我们需要解析的数据来计算JSON缓冲区最佳大小
  // This is only required if you use StaticJsonBuffer. -- 如果你使用StaticJsonBuffer时才需要
  const size_t BUFFER_SIZE = 1024;
  // Allocate a temporary memory pool on the stack -- 在堆栈上分配一个临时内存池
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  //  -- 如果堆栈的内存池太大，使用 DynamicJsonBuffer jsonBuffer 代替
  // If the memory pool is too big for the stack, use this instead:
  //  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(content);
  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }
  // Here were copy the strings we're interested in
  userData->errno_val = root["errno"];
  strcpy(userData->error, root["error"]);
  if ( userData->errno_val == 0 ) {
    userData->test_led_Val = root["data"]["current_value"];
    strcpy(userData->udate_at, root["data"]["update_at"]);
    Serial.print("YF-Test_LED Value : ");
    Serial.print(userData->test_led_Val);
    Serial.print("\t The last update time : ");
    Serial.println(userData->udate_at);
  }
  Serial.print("errno : ");
  Serial.print(userData->errno_val);
  Serial.print("\t error : ");
  Serial.println(userData->error);
  return true;
}
void colLED(int sta) {
  digitalWrite(ledPin, sta);
}
void smartConfig()
{
  WiFi.mode(WIFI_STA);
  Serial.println("\r\nWait for Smartconfig");
  WiFi.beginSmartConfig();
  while (1)
  {
    Serial.print(".");
    digitalWrite(LED, 0);
    delay(500);
    digitalWrite(LED, 1);
    delay(500);
    if (WiFi.smartConfigDone())
    {
      Serial.println("SmartConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      break;
    }
  }
}
void setup() {
  WiFi.mode(WIFI_AP_STA);                 //set work mode:  WIFI_AP /WIFI_STA /WIFI_AP_STA
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  dht.begin();
  smartConfig();
}
void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  if (!client.connect(host, tcpPort)) {//链接tcp服务器测试
    Serial.println("connection failed");
    return;
  }
  getdata(); 
   if (!client.connect(host, tcpPort)) {//链接tcp服务器测试
    Serial.println("connection failed");
    return;
  }
  postData(deviceId, t, h);    
}
void postData(int dId, float val_t, float val_h) {
  String url = "/devices/";
  url += String(dId);
  url += "/datapoints?type=3";           //http://open.iot.10086.cn/doc/art190.html#43
  String data = "{\"" + String(DS_Temp) + "\":" + String(val_t) + ",\"" + String(DS_Hum) + "\":" + String(val_h) + ",\"" +  String(DS_Baojing) + "\":" + String(val_t) + "}";
  Serial.println(data);
  Serial.print("data length:");
  Serial.println(String(data.length()));
  String post_data = "POST " + url + " HTTP/1.1\r\n" +
                     "api-key:" + APIKEY + "\r\n" +
                     "Host:" + host + "\r\n" +
                     "Content-Length: " + String(data.length()) + "\r\n" +                     //发送数据长度
                     "Connection: close\r\n\r\n" +
                     data;
  client.print(post_data);
}
void getdata() {
  // We now create a URI for the request
  String url2 = "/devices/";
  url2 += String(deviceId);
  url2 += "/datastreams/";
  url2 += DataStreams;
  String send_data = String("GET ") + url2 + " HTTP/1.1\r\n" +
                     "api-key:" + APIKEY + "\r\n" +
                     "Host:" + host + "\r\n" +
                     "Connection: close\r\n\r\n";
  client.print(send_data);
  if (DEBUG) {
    Serial.println(send_data);
  }
  if (skipResponseHeaders())  { //  发送请求json解析
    char response[MAX_CONTENT_SIZE];
    readReponseContent(response, sizeof(response));
    UserData userData_testLED;
    if (parseUserData_test(response, &userData_testLED)) {
      Serial.println("daily data parse OK!");
      colLED(userData_testLED.test_led_Val);
    }
  }
}




