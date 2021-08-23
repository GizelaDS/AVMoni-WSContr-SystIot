/*.......................Libraries and definitions...........................*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h" 

#define DHTPIN D5
#define DHTTYPE DHT22

#define LDR A0
/*..............................Variables...................................*/
const char* ssid = "mySSID";
const char* password = "myPASSWORD";

const char* mqtt_server = "192.168.100.28";
const char* clientID = "NodeMCU2";
const char* willTopic = "/estado2";
int willQoS = 0;
boolean willRetain = true;
const char* willMessage = "0"; // 0-disconnected

int contadorFallo = 0;

IPAddress wifiIp(192, 168, 100, 31);
IPAddress wifiNet(255, 255, 255, 0);
IPAddress wifiGw(192, 168, 100, 1);

const char* topicT = "/huerto/plantas/tempAmb";  //temperture
const char* topicH = "/huerto/plantas/humidAmb"; //humidity
const char* topicL = "/huerto/plantas/nivelLuz"; //light ADC value

char msgT[10];
char msgH[10];
char msgL[5];

/* CA Certificate*/
const char caCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjzCCAnegAwIBAgIUaTaS51KrOAu9w8bsoXbFMYrqXZEwDQYJKoZIhvcNAQEL
BQAwVzELMAkGA1UEBhMCQk8xDTALBgNVBAgMBGNiYmExDTALBgNVBAcMBGNiYmEx
ETAPBgNVBAoMCFByb3llY3RvMRcwFQYDVQQDDA4xOTIuMTY4LjEwMC4yODAeFw0y
MTA3MjYxOTM5MjZaFw0zMTA3MjQxOTM5MjZaMFcxCzAJBgNVBAYTAkJPMQ0wCwYD
VQQIDARjYmJhMQ0wCwYDVQQHDARjYmJhMREwDwYDVQQKDAhQcm95ZWN0bzEXMBUG
A1UEAwwOMTkyLjE2OC4xMDAuMjgwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK
AoIBAQDplx7FQlTdQhEGB9GptOnHoIo4jZFb0rfAOVb60wmt+MD/OxBGUrEJQXhD
iuvNZv+3Ova3bvtl/1DDjUtzZFeg+cLlDDYx8FFeebD5K7JIRtcfFqYHbkuULMFB
cykm62Le2RuHkIG3MEenHc3vWVTfGczD+JJDNYjIySzv1oWC+qdjoEf2Y8mSfdfA
WRothjzgrUdx1ZQO9YfQCvKGq50/N63oNCh4OnDd72jRH04l/aJiKaiaZ4E9aWPq
WwX5felPIOeovG+3ClVxjww6DY6z5d/vvNT3gwvv/r2hGjqj3gfQkJn165nFADT2
3Zx0bsZ9WhKE9lLhQcNFeVMxNlCxAgMBAAGjUzBRMB0GA1UdDgQWBBSjwGkag9Xv
PY+0HSWxIGHg/FnKdjAfBgNVHSMEGDAWgBSjwGkag9XvPY+0HSWxIGHg/FnKdjAP
BgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAeequ/k534S6y9T1Lv
TllgE48srGtUe4DNiOtkrJAl1AbeousX12vGsl+PvXOUks/eEd4C2KGK1Dz97K4b
3WO5QvOgmx2+/p4TzFm8A3fWl9JAIVIviPRKLkq+5OUTDoPY5+9ohpXR+0UEBQmK
ep556oX+b47D8RiFwuSJIpm42KH/103NakclJiV6i22tOqyP2EuD3jhXplewqDWw
N0B28UGNWD0j7tvPsvJwUGLsK5gKkgIJIL1D/X3l5ZgbgcTBMoMnfzc70q7njvZw
UbGACTFlQSSop1LmzrKxTmvCLJPs7EZZBAw0pxxWdbqqTZlI8vNalGZTVDnDo5pp
oAnT
-----END CERTIFICATE-----
)EOF";

/*MQTT broker SHA1 fingerprint certificate*/
const uint8_t mqttCertFingerprint[] = {0x1C,0x0F,0x59,0xD6,0xFB,0xA6,0xA8,0x14,0x8A,0x73,0x52,0xF6,0x41,0x0F,0xAF,0xF4,0xC2,0x2D,0xA2,0x19};

/*................................Setups....................................*/
X509List caCertX509(caCert);
WiFiClientSecure esp2Client;
PubSubClient mqttClient2(mqtt_server, 8883, esp2Client); //fully initialized client

DHT dht(DHTPIN, DHTTYPE);

void setup_wifi() {
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.config(wifiIp, wifiGw, wifiNet);
  WiFi.begin(ssid, password);
}

void setup_tls() {
  esp2Client.setTrustAnchors(&caCertX509);         //Upload the CA certificate
  esp2Client.allowSelfSignedCerts();               //Enable self-signed certificate  
  esp2Client.setFingerprint(mqttCertFingerprint);  //Upload the MQTT broker fingerprint certificate
}

void setup()
{
  dht.begin(); 
  setup_wifi();
  setup_tls();
}

void loop()
{
  if (!mqttClient2.connected()) {
    reconnect();
  }
  PubTempHumidAmb();
  PubNivelLuz();
  delay(2000);
}

/*...............................Functions..................................*/
void reconnect() {
  while (!mqttClient2.connected()) {
    if (mqttClient2.connect(clientID, "", "", willTopic, willQoS, willRetain, willMessage, true)) {
      mqttClient2.publish(willTopic, "1", true); 
      testDHT();
      testLDR();
    } else {
      ++contadorFallo;
      if (contadorFallo > 180) ESP.restart();
      delay(5000);
    }
  }
}

float leerTempDHT() {
  return dht.readTemperature();
}

float leerHumidDHT() {
  return dht.readHumidity();
}

void testDHT() {
  if (isnan(leerTempDHT()) || isnan(leerHumidDHT())) {
    mqttClient2.publish("/estadoDHT", "0"); //sensor doesn't work
  }
  else {
    mqttClient2.publish("/estadoDHT", "1"); //sensor works ok
  }
}

void PubTempHumidAmb() {
  dtostrf(leerTempDHT(), 3, 2, msgT);
  dtostrf(leerHumidDHT(), 3, 2, msgH);
  
  mqttClient2.publish(topicT, msgT);
  mqttClient2.publish(topicH, msgH);
}

int leerLDR() {
  return analogRead(LDR);
}

void testLDR() {
  if (leerLDR() == 0) {
    mqttClient2.publish("/estadoLDR", "0"); //sensor doesn't work
  }
  else {
    mqttClient2.publish("/estadoLDR", "1"); //sensor works ok
  }
}

void PubNivelLuz() {
  int Alectura = leerLDR();
  sprintf(msgL, "%i", Alectura);
  mqttClient2.publish(topicL, msgL);
}
