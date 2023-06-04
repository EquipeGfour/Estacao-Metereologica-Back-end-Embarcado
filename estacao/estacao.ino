#include <WiFi.h>
#include <HTTPClient.h>
#include <vector>
#include <numeric>
#include "time.h";
#include <random>

// WiFi config
const char* ssid = "Fatec WiFi"; //ADICIONE NO LUGAR DE "Fatec WiFi" A SUA REDE
const char* pwd = ""; // ADICIONE SUA SENHA

// Server config
const char* server = ""; //ADICIONE AQUI A URL DO BACK-END
const char* app_key = "estacao";

// NTP config
const char* ntpServer = "br.pool.ntp.org";
const long gmtOffset = 0;
const int daylight = 0;

String uid;

HTTPClient hpost;

time_t datetime;
time_t adjustedTime;
struct tm timeinfo;

float temp = 0.0;
float umi = 0.0;
float wspeed = 0.0;
float wdirection = 0.0;
uint32_t pluv = 0;
float bat = 0.0;

TaskHandle_t tSimulacao;
SemaphoreHandle_t mutex;

void connectWiFi(){
  Serial.print("\nConectando");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConectado ao WiFi com o IP: ");
  Serial.println(WiFi.localIP());
}


void simulacao(void *pvParameters){
  Serial.println("Iniciando a Taks1");
  while (true){
    xSemaphoreTake(mutex, portMAX_DELAY);
    temp = random(22, 27) + (random(100) / 100.0);
    umi = random(65, 98) + (random(100) / 100.0);
    wspeed = random(4, 15) + (random(100) / 100.0);
    wdirection = random(1, 360) + (random(100) / 100.0);
    bat = analogRead(34) * 3.3 / 4095;
    pluv = pluv + random(12);
    xSemaphoreGive(mutex);
    delay(60000);
  }
}


void sendParameterFromEspToMongo(String parameter){
   String url = String(server) + "cadastrar";
   WiFiClient wClient;
   hpost.begin(wClient, url);
   hpost.addHeader("Content-Type", "application/json");
   hpost.addHeader("x-app-key", "kkk");
   int httpReturn = hpost.POST(parameter);
   String dados = hpost.getString();
   Serial.println(url);
   //Serial.print(parameter);
   PrintResponseContent(dados, httpReturn);
   hpost.end();
}

void PrintResponseContent(String response,int httpStatus ){
  if (httpStatus > 0) {
    Serial.printf("Status: %d\n", httpStatus);
    Serial.printf("Response: %s\n", response.c_str());
  } else {
    Serial.println("Ocorreu um ERRO ao realizar um POST!");
  }
}


void sendTemperature(){
   String medida = "{\"uid\":\"" + uid + "\",\"temp\":" + String(temp) + ",\"unx\":" + String(time(&datetime)) + "}";
   sendParameterFromEspToMongo(medida);
}


void sendHumidity(){
   String medida = "{\"uid\":\"" + uid + "\",\"umi\":" + String(umi) + ",\"unx\":" + String(time(&datetime)) + "}";
   sendParameterFromEspToMongo(medida);
}


void sendBatery(){
   String medida = "{\"uid\":\"" + uid + "\",\"bat\":" + String(bat) + ",\"unx\":" + String(time(&datetime)) + "}";
   sendParameterFromEspToMongo(medida);
}


void sendPluviometro(){
   String medida = "{\"uid\":\"" + uid + "\",\"pluv\":" + String(pluv) + ",\"unx\":" + String(time(&datetime)) + "}";
   sendParameterFromEspToMongo(medida);
}

void sendWspeed(){
   String medida = "{\"uid\":\"" + uid + "\",\"wspeed\":" + String(wspeed) + ",\"unx\":" + String(time(&datetime)) + "}";
   sendParameterFromEspToMongo(medida);
}

void sendWdirection(){
   String medida = "{\"uid\":\"" + uid + "\",\"wdirection\":" + String(wdirection) + ",\"unx\":" + String(time(&datetime)) + "}";
   sendParameterFromEspToMongo(medida);
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));
  pinMode(34, INPUT);
  mutex = xSemaphoreCreateMutex();
  if(mutex == NULL){
    Serial.println("deu ruim no mutex");
  }

  xTaskCreatePinnedToCore(
    simulacao, //function name
    "Simulacao", // task name
    10000, //task stack size
    NULL, //task parameters
    1, //task priority
    &tSimulacao, //task handle
    0); //task core - loop run on core 1

  WiFi.begin(ssid, pwd);
  uid = WiFi.macAddress();
  uid.replace(":", "");
  Serial.print("Uid: ");
  Serial.print(uid);
  connectWiFi();
  
  configTime(gmtOffset, daylight, ntpServer);
  if(!getLocalTime(&timeinfo)){
     Serial.println("Erro ao acessar o NTP server");
  }else{
    Serial.print("Horario: ");
    datetime = time(&datetime);
    adjustedTime = datetime - 3 * 3600;
    Serial.println(ctime(&adjustedTime));
  }
}


void loop() {
  if((time(&datetime) % 600) == 0){
      Serial.println("\n\n##### TRANSMITINDO DADOS #####");
      Serial.print("Horario: ");
      adjustedTime = datetime - 3 * 3600;
      Serial.println(ctime(&adjustedTime));

    if(WiFi.status() == WL_CONNECTED){
      sendTemperature();
      sendHumidity();
      sendBatery();
      sendPluviometro();
      sendWspeed();
      sendWdirection();
    } else {
      Serial.println("Ocorreu algum erro ao se conectar a rede : (");
      connectWiFi();
    }
    delay(30000);
  }
}
