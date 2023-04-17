#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h";

char* ssid = "Fatec WiFi";
char* pwd = "";
String uid;
char* server = "http://172.16.7.84:5001/";
HTTPClient hget;
HTTPClient hpost;
int httpReturn;

char* ntpServer = "br.pool.ntp.org";
long gmtOffset = 0;
int daylight = 0;

time_t now;
struct tm timeinfo;

float t = 20.0;
float u = 50.0;

String data;

void connectWiFi(){
  Serial.print("Conectando");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConectado ao WiFi com o Ip: ");
  Serial.println(WiFi.localIP());
}


void buscarDados(){
    Serial.println("\n\n### GET ###");
    String tmp = String(server) + "buscar";
    hget.begin(tmp.c_str());
    httpReturn = hget.GET();
    String dados = hget.getString();
    if(httpReturn > 0){
      Serial.println(tmp);
      Serial.printf("Status: %d\n", httpReturn);
      Serial.printf("Response: %s\n", dados.c_str());
      //Serial.print("quantidade de dados: " + String(static_cast<int>(hget.getSize())));
    } else {
      Serial.println("Ocorreu um ERRO ao realizar um GET!");
    }
}


void cadastrarDados() {
  Serial.println("\n\n### POST ###");
  String url = String(server) + "cadastrar";
  WiFiClient wClient;

  hpost.begin(wClient, url);
  hpost.addHeader("Content-Type", "application/json");
  hpost.addHeader("x-app-key", "kkk");
  String jsonString = "{\"uid\":\"" + uid + "\",\"temp\":" + String(t,2) + ",\"umi\":" + String(u,2) + ",\"unx\":" + time(&now) + "}";

  Serial.print(jsonString);
//  DynamicJsonDocument jsonDoc(256);
//  jsonDoc["uid"] = uid;
//  jsonDoc["temp"] = t;
//  jsonDoc["umi"] = u;

//  String jsonString;
//  serializeJson(jsonDoc, jsonString);
  httpReturn = hpost.POST(jsonString);
  String dados = hpost.getString();
 
  if (httpReturn > 0) {
    Serial.println(url);
    Serial.printf("Status: %d\n", httpReturn);
    Serial.printf("Response: %s\n", dados.c_str());
    
  } else {
    Serial.println("Ocorreu um ERRO ao realizar um POST!");
  }

  hpost.end();
}


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pwd);
  uid = WiFi.macAddress();
  uid.replace(":", "");
  connectWiFi();
  configTime(gmtOffset, daylight, ntpServer);
  if(!getLocalTime(&timeinfo)){
     Serial.println("Erro ao acessar o NTP server");
  }else{
    Serial.print("A hora agora eh ");
    Serial.println(time(&now));
  }
}


void loop() {
  if((time(&now) % 10) == 0){
      Serial.println("\n\n##### TRANSMITINDO DADOS #####");
      Serial.println(time(&now));
  
    if(WiFi.status() == WL_CONNECTED){
      buscarDados();
      cadastrarDados();
    } else {
      Serial.println("Ocorreu algum erro ao se conectar a rede : (");
      connectWiFi();
    }
    t = t + 0.6;
    u = u + 0.8;
    delay(30000);
  }
}
