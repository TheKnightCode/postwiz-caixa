#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

//Terminais
const int pinReset = 34; //Botao reset
const int pinSensor = 35; //Sensor IR
const int pinBuzz = 18; //Buzzer
const int pinRS = 13, pinEN = 27, pinD4 = 26, pinD5 = 25, pinD6 = 33, pinD7 = 32; //Visor

//Tipo detectado
const int MAIL = 0; //Carta
const int SPAM = 1; //Spam

//Configuracoes. NAO ESQUECA DE CONFIGURAR
const char* ssid = "NOME DA REDE WIFI"; //Nome da rede Wi-Fi
const char* password = "SENHA DA REDE WIFI"; //Senha da rede Wi-Fi

//Aumente em caso de falso-positivo, reduza em caso de falso-negativo
const int thresholdMail = 1000; //Limiar de detecao da carta. Se o sensor der um valor menor que isso eh detectado como carta.
const int thresholdSpam = 3000; //Limiar de detecao do spam. Se o sensor der um valor menor que isso eh detectado como spam.

//Contagem
int countMail = 0; //Cartas
int countSpam = 0; //Spam

//Dominio do servidor
String serverName = "http://daily-singular-goblin.ngrok-free.app";

//Tempo desde o ultimo upload
unsigned long lastTime = 0;
//Intervalo entre uploadds
unsigned long timerDelay = 5000;
bool retry = false; //Caso uma comunicacao com o servidor tenha sido mal-sucedida, esta variavel vai para true, e o ESP-32 faz tentativas de comunicacao em intervalos regulares

//Envia dados para o servidor
String upload(int mail, int spam) { //Argumentos: numero de cartas, numero de spam
  if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      String serverPath = serverName + "/box/update"; //URL do servidor

      http.begin(serverPath.c_str());

      //Corpo da requisicao (json com os dados)
      String body = "{\"mail\": " + String(mail) + ", \"spam\": " + String(spam) + "}";
      
      //Envia requisicao POST
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(body);

      //Resposta da requisicao
      String payload;
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        payload = http.getString();
        Serial.println(payload);
      }
      else {
        payload = "";
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
      return payload;
    }
    else {
      Serial.println("WiFi Disconnected");
      return "";
    }
}


//Detecta cartas
int detection() {
  unsigned int sampleSum = 0;
  unsigned int sampleCount = 1; //A contagem comeca em 1 para evitar erro de divisao por 0.

  delay(100);
  //Enquanto a carta esta sendo colocada, o ESP-32 pega varias amostras do sinal e faz a media.
  while (analogRead(pinSensor) < thresholdSpam) {
      sampleSum += analogRead(pinSensor); //Soma das amostras
      sampleCount++; //Contagem de amostras
      delay(1);
  }
  
  int signalAverage = sampleSum/sampleCount; //Sinal medio
  delay(500);

  if (signalAverage < thresholdMail) { //Carta detectada
    Serial.print("MAIL - Signal: ");
    Serial.println(signalAverage);
    tone(pinBuzz, 1000, 50); //Toca o buzzer
    return MAIL;
  } else { //Spam detectado
    Serial.print("SPAM - Signal: ");
    Serial.println(signalAverage);
    return SPAM;
  }
}

void setup() {
  //Inicializa monitor serial
  Serial.begin(115200); 

  //Configura terminais
  pinMode(pinReset, INPUT);
  pinMode(pinSensor, INPUT);

  //Conecta Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Conectando");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("CONECTADO - Endereco IP: ");
  Serial.println(WiFi.localIP());

}

void loop() {
  int sensorSignal = analogRead(pinSensor);
  //Serial.println(sensorSignal);
  if (sensorSignal < thresholdSpam) {
   detection();
  }
}
