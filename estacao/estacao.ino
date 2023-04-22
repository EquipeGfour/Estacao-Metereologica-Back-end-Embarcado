#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h";
#include <vector>
#include <numeric>


// WiFi config
const char* ssid = "Fatec";
const char* pwd = "";

// Server config
const char* server = "http://192.168.15.37:5001/";
const char* app_key = "estacao";

// NTP config
const char* ntpServer = "br.pool.ntp.org";
const long gmtOffset = 0;
const int daylight = 0;

String uid;

HTTPClient hget;
HTTPClient hpost;
int httpReturn;

time_t now;
struct tm timeinfo;

float t = 20.0;
float u = 50.0;
float p = 10.0;

TaskHandle_t tTask1;
TaskHandle_t tTask2;
SemaphoreHandle_t mutex;

std::vector<String> medidas;

String data;

void connectWiFi(){
  Serial.print("\nConectando");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConectado ao WiFi com o IP: ");
  Serial.println(WiFi.localIP());
}

void coletarDados(void *pvParameters){
  Serial.println("Iniciando coleta de dados...");
  while(true){
    xSemaphoreTake(mutex, portMAX_DELAY);
    t += 0.24;
    u += 0.51;
    xSemaphoreGive(mutex);
    delay(10000);
  }
}

void cadastrarDados() {
  Serial.println("\n\n### POST ###");
  String url = String(server) + "cadastrar-multiplos";
  WiFiClient wClient;

  hpost.begin(wClient, url);
  hpost.addHeader("Content-Type", "application/json");
  hpost.addHeader("x-app-key", "kkk");
  
  String medidaTemp = "{\"uid\":\"" + uid + "\",\"temp\":" + String(t, 2) + ",\"unx\":" + String(time(&now)) + "}";
  String medidaUmi = "{\"uid\":\"" + uid + "\",\"umi\":" + String(u, 2) + ",\"unx\":" + String(time(&now)) + "}";
  String medidaPluv = "{\"uid\":\"" + uid + "\",\"pluv\":" + String(p, 2) + ",\"unx\":" + String(time(&now)) + "}";
  
  
  medidas.push_back(medidaTemp + ",");
  medidas.push_back(medidaUmi + ",");
  medidas.push_back(medidaPluv + ",");
  
  String str_medidas = std::accumulate(medidas.begin(), medidas.end(), String("["));
  str_medidas.remove(str_medidas.length() - 1, 1);
  str_medidas += "]";
  
  httpReturn = hpost.POST(str_medidas.c_str());
  medidas.clear();
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
  mutex = xSemaphoreCreateMutex();
  if(mutex == NULL){
    Serial.println("deu ruim no mutex");
  }


  xTaskCreatePinnedToCore(
    coletarDados, //function name
    "coletarDados", //task name
    10000, // task stack size
    NULL, // task parameters
    1, // task priority
    &tTask1, // task handle
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
    Serial.print("A hora agora eh ");
    Serial.println(time(&now));
  }
}


void loop() {
  if((time(&now) % 300) == 0){
      Serial.println("\n\n##### TRANSMITINDO DADOS #####");
      Serial.println(time(&now));
  
    if(WiFi.status() == WL_CONNECTED){
      cadastrarDados();
      
    } else {
      Serial.println("Ocorreu algum erro ao se conectar a rede : (");
      connectWiFi();
    }

    delay(30000);
  }
}
