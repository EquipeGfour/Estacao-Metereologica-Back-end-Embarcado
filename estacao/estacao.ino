#include "DHT.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <vector>
#include <numeric>
#include "time.h";
#include <random>

#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

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

float temp;
float umi;
float pluv;
float bat;

TaskHandle_t tTask1;
SemaphoreHandle_t mutex;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> generateRandomFloat(0.0f, 100.0f);

void connectWiFi(){
  Serial.print("\nConectando");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConectado ao WiFi com o IP: ");
  Serial.println(WiFi.localIP());
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
   temp = dht.readTemperature();
   Serial.println(dht.readTemperature());
   if (isnan(temp)) {
    Serial.println("Erro ao coletar temperatura...");
    return;
   }
   String medida = "{\"uid\":\"" + uid + "\",\"temp\":" + String(temp*100, 2) + ",\"unx\":" + String(time(&datetime)) + "}";
   sendParameterFromEspToMongo(medida);
}


void sendHumidity(){
   umi = dht.readHumidity();
   if (isnan(temp)) {
    Serial.println("Erro ao coletar humidade...");
    return;
   }
   String medida = "{\"uid\":\"" + uid + "\",\"umi\":" + String(umi*100, 2) + ",\"unx\":" + String(time(&datetime)) + "}";
   sendParameterFromEspToMongo(medida);
}


void sendBatery(){
   bat = generateRandomFloat(gen);
   String medida = "{\"uid\":\"" + uid + "\",\"bat\":" + String(bat*100, 2) + ",\"unx\":" + String(time(&datetime)) + "}";
   sendParameterFromEspToMongo(medida);
}


void sendPluviometro(){
   pluv = generateRandomFloat(gen);
   String medida = "{\"uid\":\"" + uid + "\",\"pluv\":" + String(pluv*100, 2) + ",\"unx\":" + String(time(&datetime)) + "}";
   sendParameterFromEspToMongo(medida);
}



void coletarDados(void *pvParameters){
  Serial.println("Iniciando coleta de dados...");
  while(true){
    xSemaphoreTake(mutex, portMAX_DELAY);
    temp += 0.9;
    umi += 0.11;
    pluv+= 0.3;
    xSemaphoreGive(mutex);
    delay(10000);
  }
}

void setup() {
  
  Serial.begin(115200);
  dht.begin();
  mutex = xSemaphoreCreateMutex();
  if(mutex == NULL){
    Serial.println("deu ruim no mutex");
  }


/*
  xTaskCreatePinnedToCore(
    coletarDados, //function name
    "coletarDados", //task name
    10000, // task stack size
    NULL, // task parameters
    1, // task priority
    &tTask1, // task handle
    0); //task core - loop run on core 1
*/

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
  if((time(&datetime) % 10) == 0){
      Serial.println("\n\n##### TRANSMITINDO DADOS #####");
      Serial.print("Horario: ");
      adjustedTime = datetime - 3 * 3600;
      Serial.println(ctime(&adjustedTime));

    if(WiFi.status() == WL_CONNECTED){
      sendTemperature();
      sendHumidity();
      sendBatery();
      sendPluviometro();
    } else {
      Serial.println("Ocorreu algum erro ao se conectar a rede : (");
      connectWiFi();
    }
    delay(30000);
  }
}
