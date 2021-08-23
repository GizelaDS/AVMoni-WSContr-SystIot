/*.......................Libraries and definitions...........................*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define trig D6 
#define echo D5

#define RELEPIN D1
/*..............................Variables...................................*/
const char* ssid = "mySSID";
const char* password = "myPASSWORD";

const char* mqtt_server = "192.168.100.28";
const int mqtt_port = 8883;
const char* clientID = "NodeMCU3";
const char* willTopic = "/estado3";
int willQoS = 0;
boolean willRetain = true;
const char* willMessage = "0"; // 0-disconnected

int contadorFallo = 0;

IPAddress wifiIp(192, 168, 100, 32);
IPAddress wifiNet(255, 255, 255, 0);
IPAddress wifiGw(192, 168, 100, 1);

const char* topicN = "/huerto/deposito/nivelAgua";  //empty water level
const char* topicR = "/huerto/deposito/releAgua";  //relay

char msgN[10]; //nivel o distancia vacio

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
void mqtt_callback(char* topic, byte* payload, unsigned int length) { 
  String mensajeR; //para recibir
  for (int i = 0; i < length; i++) {
    mensajeR = mensajeR + (char)payload[i];
  }
  
  if (String(topic) == topicR) {   //if there's a message in this topic, the water pump/solenoid valve is turned on or off
    if (mensajeR == "on") {digitalWrite(RELEPIN, LOW);}   //2CH-Relay, logical 0-on
    if (mensajeR == "off") {digitalWrite(RELEPIN, HIGH);}  //2CH-Relay, logical 1-off
  }
}

X509List caCertX509(caCert);
WiFiClientSecure esp3Client;
PubSubClient mqttClient3(mqtt_server, mqtt_port, mqtt_callback, esp3Client); //fully initialized client

void setup_wifi() {
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.config(wifiIp, wifiGw, wifiNet);
  WiFi.begin(ssid, password);
}

void setup_tls() {
  esp3Client.setTrustAnchors(&caCertX509);         //Upload the CA certificate
  esp3Client.allowSelfSignedCerts();               //Enable self-signed certificate 
  esp3Client.setFingerprint(mqttCertFingerprint);  //Upload the MQTT broker fingerprint certificate
}

void setup()
{
  setup_wifi();
  setup_tls();
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(RELEPIN, OUTPUT);
  digitalWrite(RELEPIN, HIGH); //2CH-Rele, initial state 1-off
}

void loop()
{
  if (!mqttClient3.connected()) {
    reconnect();
  }
  PubNivelAgua();   
  delay(1000);
  mqttClient3.loop(); 
  delay(1000);
}

/*...............................Functions...................................*/
void reconnect() {
  while (!mqttClient3.connected()) {
    if (mqttClient3.connect(clientID, "", "", willTopic, willQoS, willRetain, willMessage, true)) {
      mqttClient3.publish(willTopic, "1", true); 
      mqttClient3.subscribe(topicR);
      testHCSR04();
    } else {
      ++contadorFallo;
      if (contadorFallo > 180) ESP.restart();
      delay(5000);
    }
  }
}

float leerHCSR04() {
  digitalWrite(trig, LOW);
  delayMicroseconds(4);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long duracion = pulseIn(echo, HIGH);
  delayMicroseconds(6);
  return duracion/58.3;
}

void testHCSR04() {
  if (leerHCSR04() == 0) {
    mqttClient3.publish("/estadoHCSR04", "0"); //sensor doesn't work
  }
  else {
    mqttClient3.publish("/estadoHCSR04", "1"); //sensor works ok
  }
}

void PubNivelAgua() {
  float distanciaVacio = leerHCSR04();
  dtostrf(distanciaVacio, 3, 2, msgN);
  mqttClient3.publish(topicN, msgN);
}
