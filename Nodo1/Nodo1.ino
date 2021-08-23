/*.......................Libraries and definitions...........................*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define PinHumidSA A0
/*..............................Variables...................................*/
const char* ssid = "mySSID";
const char* password = "myPASSWORD";

const char* mqtt_server = "192.168.100.28";
const char* clientID = "NodeMCU1";
const char* willTopic = "/estado1";
int willQoS = 0;
boolean willRetain = true;
const char* willMessage = "0"; // 0-disconnected

int contadorFallo = 0;

IPAddress wifiIp(192, 168, 100, 30);  
IPAddress wifiNet(255, 255, 255, 0);
IPAddress wifiGw(192, 168, 100, 1);

const char* topicHA = "/huerto/plantas/humidSuelo";  //soil moisture ADC value

char msgHA[5];

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

/*................................Setups.....................................*/
X509List caCertX509(caCert);
WiFiClientSecure esp1Client;
PubSubClient mqttClient1(mqtt_server, 8883, esp1Client); //fully initialized client

void setup_wifi() {
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.config(wifiIp, wifiGw, wifiNet);
  WiFi.begin(ssid, password);
}

void setup_tls() {
  esp1Client.setTrustAnchors(&caCertX509);         //Upload the CA certificate
  esp1Client.allowSelfSignedCerts();               //Enable self-signed certificate 
  esp1Client.setFingerprint(mqttCertFingerprint);  //Upload the MQTT broker fingerprint certificate
}

void setup()
{
  setup_wifi();
  setup_tls();
}

void loop()
{
  if (!mqttClient1.connected()) {
    reconnect();
  }
  PubHumidSueloAnalogico();
  delay(1000);
}

/*...............................Functions....................................*/
void reconnect() {
  while (!mqttClient1.connected()) {
    if (mqttClient1.connect(clientID, "", "", willTopic, willQoS, willRetain, willMessage, true)) {
      mqttClient1.publish(willTopic, "1", true); 
      testCapacitivo();
    } else {
      ++contadorFallo;
      if (contadorFallo > 180) ESP.restart();
      delay(5000);
    }
  }
}

int leerCapacitivo() {
  return analogRead(PinHumidSA);
}

void testCapacitivo() {
  if (leerCapacitivo() < 10 ) {
    mqttClient1.publish("/estadoCapacitivo", "0"); //sensor doesn't work
  }
  else {
    mqttClient1.publish("/estadoCapacitivo", "1"); //sensor works ok
  }
}

void PubHumidSueloAnalogico() {
  int Alectura = leerCapacitivo();
  sprintf(msgHA, "%i", Alectura);  
  mqttClient1.publish(topicHA, msgHA);
}
