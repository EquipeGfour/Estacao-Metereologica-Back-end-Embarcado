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

HTTPClient hget;
HTTPClient hpost;
int httpReturn;

time_t now;
struct tm timeinfo;

float temp;
float umi;
float pluv;
float bat;

TaskHandle_t tTask1;
TaskHandle_t tTask2;
SemaphoreHandle_t mutex;

std::vector<String> medidas;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> generateRandomFloat(0.0f, 100.0f);

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
    temp += 0.9;
    umi += 0.11;
    pluv+= 0.3;
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

  temp = generateRandomFloat(gen);
  umi = generateRandomFloat(gen);
  pluv = generateRandomFloat(gen);
  bat = generateRandomFloat(gen);

  String medidaTemp = "{\"uid\":\"" + uid + "\",\"temp\":" + String(temp*100, 2) + ",\"unx\":" + String(time(&now)) + "}";
  String medidaUmi = "{\"uid\":\"" + uid + "\",\"umi\":" + String(umi*100, 2) + ",\"unx\":" + String(time(&now)) + "}";
  String medidaPluv = "{\"uid\":\"" + uid + "\",\"pluv\":" + String(pluv*100, 2) + ",\"unx\":" + String(time(&now)) + "}";
  String medidaBat = "{\"uid\":\"" + uid + "\",\"bat\":" + String(bat*100, 2) + ",\"unx\":" + String(time(&now)) + "}";
  
  medidas.push_back(medidaTemp + ",");
  medidas.push_back(medidaUmi + ",");
  medidas.push_back(medidaPluv + ",");
  medidas.push_back(medidaBat + ",");
  
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
    Serial.print("A hora agora eh ");
    Serial.println(time(&now));
  }
}


void loop() {
  if((time(&now) % 60) == 0){
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
