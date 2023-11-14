#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <SSD1306.h>
#include <DataEncDec.h>
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Pin definitions 
#define SCK 5   // GPIO5  SCK
#define MISO 19 // GPIO19 MISO
#define MOSI 27 // GPIO27 MOSI
#define SS 18   // GPIO18 CS
#define RST 14  // GPIO14 RESET
#define DI00 26 // GPIO26 IRQ(Interrupt Request)
 
#define BAND 915E6 //Frequência do radio - exemplo : 433E6, 868E6, 915E6

//Wifi credentials
// const char* ssid = "IC-ALUNOS";
// const char* password = "icomputacaoufal";
// const char* ssid = "Igor_1";
// const char* password = "nlw3904nly9968";
// const char* ssid = "CLARO_2G913C4E";
// const char* password = "12913C4E";
const char* ssid = "MiniusinaUFALiot";
const char* password = "ufal25973662";

//Solaire access
HTTPClient http;

// const String url = "http://34.151.229.173:8000/";
// const String username = "softex";
// const String sl_password = "softexfotovoltaic123";

const String url = "http://192.168.1.100:8000/";
const String username = "admin";
const String sl_password = "admin";

String token = "";

// Configuration for NTP
const char* ntp_primary = "a.st1.ntp.br";
const char* ntp_secondary = "b.st1.ntp.br";
 
//Objects declaration
SSD1306 display(0x3c, 4, 15);
DataEncDec decoder(0);
hw_timer_t *timer = NULL;

//Variable declaration
float settings[6] = {2, 200, 2, 40, 50, 3600};
int settingsStation = 0;
int settingsDatalogger = 0;
long lastStationData = 0;
long lastDLData = 0;
int stationDataRedy = 0;
int dlDataReady = 0;
int ackSentStation = 0;
int ackSentDL = 0;

//Data to send
DateTime stationTime = time(NULL)-10800;
DateTime dlTime = time(NULL)-10800;

float ambTemperature = 0;
int humidity = 0;
float irradiance = 0;
float windSpeed = 0;
int windDirection = 0;
float accumulatedRain = 0;
float pvTemperature = 0;
float openCircuitVoltage = 0;
float shortCircuitCurrent = 0;

float powerAvg = 0;
float voltageS1 = 0;
float currentS1 = 0;
float voltageS2 = 0;
float currentS2 = 0;

void setupLoRa(){ 
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI00);
  digitalWrite(RST, LOW);
  digitalWrite(RST, HIGH);
 
  if (!LoRa.begin(BAND)){
    display.clear();
    display.drawString(0, 0, "Erro ao inicializar o LoRa!");
    display.display();
    Serial.println("Erro ao inicializar o LoRa!");
    delay(3000);
    esp_restart();
  }
 
  LoRa.enableCrc();
  // LoRa.receive();

  //US902_928 { 903900000, 125, 7, 10, 923300000, 500, 7, 12}
  //AU925_928 { 916800000, 125, 7, 10, 916800000, 125, 7, 12}

  LoRa.setSignalBandwidth(125E3); //7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, 500E3
  LoRa.setSpreadingFactor(7); //range: 6 to 12
  LoRa.setTxPower(10); //range: 2 to 20
  LoRa.setSyncWord(0x12); // default: 0x12
}

void printEncryptionType(int thisType) {
  // read the encryption type and print out the name:
  switch (thisType) {
    case 5:
      Serial.println("WEP");
      break;
    case 2:
      Serial.println("WPA");
      break;
    case 4:
      Serial.println("WPA2");
      break;
    case 7:
      Serial.println("None");
      break;
    case 8:
      Serial.println("Auto");
      break;
    default:
      Serial.println("Not supported");
      break;
  }
}

int listNetworks() {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid < 0) {
    Serial.println("Couldn't get a wifi connection");
    return 0;
  }

  // print the list of networks seen:
  Serial.print("number of available networks: ");
  Serial.println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    Serial.print("\tEncryption: ");
    printEncryptionType(WiFi.encryptionType(thisNet));
  }
  return 1;
}

void setupWifi(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  listNetworks();
  WiFi.begin(ssid, password);
  Serial.println("Connecting Wi-Fi");
  int count = 0;
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    count++;
    if(count > 120){
      Serial.print("\nUnable to connect WiFi. Erro code: ");
      Serial.println(WiFi.status());
      Serial.println("Reseting ESP\n");
      esp_restart();
    }
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void getToken(){
  http.begin((url + "api/accounts/token/").c_str());
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"username\": \"" + username + "\", \"password\": \"" + sl_password + "\"}";
  int httpResponseCode = http.sendRequest("GET", (uint8_t *) payload.c_str(), payload.length());
  if (httpResponseCode==200) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    token = payload.substring(10, 50);
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    Serial.println(http.getString());
    delay(2000);
    esp_restart();
  }
}

//Watchdog reset
void IRAM_ATTR resetModule(){
  Serial.println("Watchdog Reboot!\n\n");
  esp_restart();
}

void setup(){
  Serial.begin(115200);
  delay(200);

  //Config watchdog 2 min and 30 sec
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &resetModule, true);
  timerAlarmWrite(timer, 150000000, true);
  timerAlarmEnable(timer);

  pinMode(25, OUTPUT); //LED pin configuration
  pinMode(16, OUTPUT); //OLED RST pin

  digitalWrite(16, LOW); //OLED Reset
  delay(50);
  digitalWrite(16, HIGH);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  display.clear();
  display.drawString(0, 0, "Initializing gateway");
  display.display();
  Serial.println("\n");
  Serial.println("Initializing gateway");

  //Chama a configuração inicial do LoRa
  setupLoRa();
  display.clear();
  display.drawString(0, 0, "LoRa ready");
  display.display();
  Serial.println("LoRa ready");

  setupWifi();
  delay(1000);
  display.clear();
  display.drawString(0, 0, "WiFi ready");
  display.display();
  Serial.println("WiFi read");

  Serial.println("Synchronizing time");
  configTime(0, 0, ntp_primary, ntp_secondary);
  while (time(nullptr) < 1510644967) {
    delay(1000);
  }
  Serial.println("Time synced");

  // getToken();
  // display.clear();
  // display.drawString(0, 0, "CloudIoT initialized");
  // display.display();
  // Serial.println("CloudIoT initialized");

  delay(1000);

  display.clear();
  display.drawString(0, 0, "Gateway waiting...");
  display.display();
  Serial.println("Gateway waiting...");
}

void sendACK(uint8_t to){
  int size = 5;
  if((to == STATION && settingsStation) || (to == DATALOGGER && settingsDatalogger)){
    size = 14;
  }
  DataEncDec encoder(size);
  encoder.reset();

  if((to == STATION && settingsStation) || (to == DATALOGGER && settingsDatalogger)){
    encoder.addHeader(GATEWAY, to, 1, 1);
  }
  else{
    encoder.addHeader(GATEWAY, to, 1, 0);
  }

  configTime(0, 0, ntp_primary, ntp_secondary);
  while (time(nullptr) < 1510644967) {
    delay(10);
  }
  Serial.println("Time synced");

  DateTime now = time(NULL)-10800;
  encoder.addDate(now.unixtime());

  if(to == STATION && settingsStation){
    encoder.addVoltage(settings[0]);
    encoder.addVoltage(settings[1]);
    encoder.addVoltage(0);
    encoder.addPower(0);
    settingsStation = 0;
  }
  if(to == DATALOGGER && settingsDatalogger){
    encoder.addVoltage(settings[2]);
    encoder.addVoltage(settings[3]);
    encoder.addVoltage(settings[4]);
    encoder.addPower(settings[5]);
    settingsDatalogger = 0;
  }

  char* buffer = encoder.getBuffer();
  LoRa.beginPacket();
  for(int i = 0; i < size; i++){
    LoRa.print(buffer[i]);
  }
  LoRa.endPacket();

  Serial.println("ACk sent");
}

void readStationData(char* received){
  DateTime now = decoder.getDate(received[1], received[2], received[3], received[4]);
  float temp = decoder.getTemp(received[5], received[6]);
  int humi = decoder.getHumi(received[7]);
  float irrad = decoder.getIrrad(received[8], received[9]);
  float wSpeed = decoder.getWindSpeed(received[10]);
  int wDirection = decoder.getWindDirection(received[11]);
  float rain = decoder.getRain(received[12]);
  float pvtemp = decoder.getTemp(received[13], received[14]);
  float voltage = decoder.getVoltage(received[15], received[16]);
  float current = decoder.getCurrent(received[17]);

  Serial.println(String(now.year())+"-"+String(now.month())+"-"+String(now.day())+"T"+String(now.hour())+":"+String(now.minute())+":"+String(now.second())+".000000-03:00\"");
  Serial.println(temp);
  Serial.println(humi);
  Serial.println(irrad);
  Serial.println(wSpeed);
  Serial.println(wDirection);
  Serial.println(rain);
  Serial.println(pvtemp);
  Serial.println(voltage);
  Serial.println(current);

  if(lastStationData != now.unixtime()) {
    ambTemperature = temp;
    humidity = humi;
    irradiance = irrad;
    windSpeed = wSpeed;
    windDirection = wDirection;
    accumulatedRain = rain;
    pvTemperature = pvtemp;
    openCircuitVoltage = voltage;
    shortCircuitCurrent = current;

    stationDataRedy = 1;
    ackSentStation = 0;

    display.clear();
    display.drawString(0, 0, "Station data received");
    display.drawString(0,20, String(now.year())+"-"+String(now.month())+"-"+String(now.day())+
                              " "+String(now.hour())+":"+String(now.minute())+":"+String(now.second()));
    display.display();

    sendACK(STATION);
    lastStationData = now.unixtime();
    stationTime = now;
  }
  else {
    if(ackSentStation > 5) {
      esp_restart();
    }
    setupLoRa();
    sendACK(STATION);
    ackSentStation++;
  }
}

void readDataLoggerData(char* received) {
  DateTime now = decoder.getDate(received[1], received[2], received[3], received[4]);
  float data[48];
  int k_data = 0;
  int k_byte = 5;
  for(int i = 0; i < 6; i++){
    for(int j = 0; j < 8; j++){
      if((i < 2) || ((i == 2) && (j<4))) {
        data[k_data] = decoder.getCurrent(received[k_byte]);
        k_byte++;
      }
      else if(i < 5) {
        data[k_data] = decoder.getVoltage(received[k_byte], received[k_byte+1]);
        k_byte = k_byte + 2; 
      }
      else {
        data[k_data] = decoder.getPower(received[k_byte], received[k_byte+1], received[k_byte+2]);
        k_byte = k_byte + 3;
      }
      k_data++;
    }
  }

  Serial.println(String(now.year())+"-"+String(now.month())+"-"+String(now.day())+"T"+String(now.hour())+":"+String(now.minute())+":"+String(now.second())+".000000-03:00\"");
  Serial.println(data[0]);
  Serial.println(data[1]);
  Serial.println(data[20]);
  Serial.println(data[21]);
  Serial.println(data[40]);

  if(lastDLData != now.unixtime()) {
    // currentS1 = data[0]; //ADC0, porta 0
    // currentS2 = data[1]; //ADC0, porta 1
    // voltageS1 = data[20]; //ADC2, porta 4
    // voltageS2 = data[21]; //ADC2, porta 5
    powerAvg = data[40]; //ADC5, porta 0 

    dlDataReady = 1;
    ackSentDL = 0;

    display.clear();
    display.drawString(0, 0, "Data logger data received");
    display.drawString(0,20, String(now.year())+"-"+String(now.month())+"-"+String(now.day())+
                              " "+String(now.hour())+":"+String(now.minute())+":"+String(now.second()));
    display.display();

    sendACK(DATALOGGER);
    lastDLData = now.unixtime();
    dlTime = now;
  }
  else {
    if(ackSentDL > 5) {
      esp_restart();
    }
    setupLoRa();
    sendACK(STATION);
    ackSentDL++;
  }
}

void sendData() {
  Serial.println("Sending data");
  
  http.begin((url + "api/external/postdata/").c_str());
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Token " + token);

  DateTime now = stationTime;
  String timestamp = String(now.year())+"-"+String(now.month())+"-"+String(now.day())+"T"+String(now.hour())+":"+String(now.minute())+":"+String(now.second())+".000000-03:00\"";

  // Dados fake
  // float power = 5000 * (irradiance/1000) * (1 - 0.0045*(pvTemperature - 25)) * 0.90;
  // float voltage1 = 301 * (1 - 0.0035*(pvTemperature - 25));
  // float voltage2 = 270 * (1 - 0.0035*(pvTemperature - 25));
  // float current = 6.97 * (irradiance/1000) * (1 + 0.0006*(pvTemperature - 25));

  String payload = String("{") +
    "\"timestamp\": \"" + timestamp +
    ",\"irradiance\": " + String(irradiance) +
    ",\"temperature_pv\": " + String(pvTemperature) +
    ",\"temperature_amb\": " + String(ambTemperature) +
    ",\"humidity\": " + String(humidity) +
    ",\"wind_speed\": " + String(windSpeed) +
    ",\"wind_direction\": " + String(windDirection) +
    ",\"rain\": " + String(accumulatedRain) +
    ",\"ocv\": " + String(openCircuitVoltage) +
    ",\"scc\": " + String(shortCircuitCurrent) +
    ",\"power_avg\": " + String(powerAvg) +
    ",\"generation\": null}";

  int httpResponseCode = http.POST(payload);
  // int httpResponseCode = 200;
  Serial.println(httpResponseCode);
  if (httpResponseCode==200) {
    Serial.print("HTTP Response code: ");
    stationDataRedy = 0;
    dlDataReady = 0;

    display.clear();
    display.drawString(0, 0, "Data sent: " + timestamp);
    display.display();
    Serial.println("Data sent: " + timestamp);

  }
  else {
    Serial.print("Error code: ");
    Serial.println(http.getString());
  }
}

void loop() {
  int packetSize = LoRa.parsePacket();
 
  if (packetSize > 0) {
    char received[packetSize];
    int cursor = 0;
 
    while(LoRa.available()) {
      received[cursor] = (char) LoRa.read();
      cursor++;
    }

    if(decoder.getTo(received[0]) == GATEWAY) {
      timerWrite(timer, 0);
      if(decoder.getFrom(received[0]) == STATION) {
        digitalWrite(25, HIGH);   // indicative LED

        Serial.println("-------------------------------------------");
        Serial.println("Station data received");

        readStationData(received);
        digitalWrite(25, LOW);   // indicative LED
        Serial.println("-------------------------------------------\n");
      }

      if(decoder.getFrom(received[0]) == DATALOGGER) {
        digitalWrite(25, HIGH);   // indicative LED

        Serial.println("-------------------------------------------");
        Serial.println("Data Logger data received");

        readDataLoggerData(received);
        digitalWrite(25, LOW);   // indicative LED
        Serial.println("-------------------------------------------\n");
      }
    }
  }

  if(stationDataRedy && dlDataReady) {
    Serial.println("-------------------------------------------");
    sendData();
    // stationDataRedy = 0;
    // dlDataReady = 0;
    timerWrite(timer, 0);
    Serial.println("-------------------------------------------\n");
  }
}