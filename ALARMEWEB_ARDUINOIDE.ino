#include <WiFi.h>
#include <WebServer.h>
#include <vector>
#include <ctime>

const char* ssid = "erikclr";
const char* password = "erikwifi";

const int sensorPin = 23;
const int buzzerPin = 22;

bool alarmActive = true;        // Estado do alarme (ativado/desativado)

bool buzzerTriggered = false;   // Se o buzzer está atualmente ligado
bool lastSensorState = false;   // Estado antigo do sensor

WebServer server(80);
std::vector<String> eventLog;

String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Data/Hora indisponível";
  }
  char buffer[64];
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

void addEventLog() {
  eventLog.push_back("Movimento detectado em: " + getTimestamp());
  if (eventLog.size() > 20) {
    eventLog.erase(eventLog.begin());
  }
}

void handleRoot() {
  String html = "<html><body><h1>Monitor de Movimento</h1>";
  html += "<p>Estado do Alarme: " + String(alarmActive ? "ATIVADO" : "DESATIVADO") + "</p>";
  html += "<p>Buzzer: " + String(buzzerTriggered ? "TOCANDO" : "SILENCIOSO") + "</p>";
  html += "<a href=\"/toggle\"><button>Desativar Alarme</button></a>";
  html += "<h2>Log de movimento</h2><ul>";

  for (int i = eventLog.size() - 1; i >= 0; --i) {
    html += "<li>" + eventLog[i] + "</li>";
  }

  html += "</ul></body></html>";
  server.send(200, "text/html", html);
}

void handleToggle() {
  
  noTone(buzzerPin);
  buzzerTriggered = false;
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(19200);
  pinMode(sensorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.print("Conectando-se ao Wi-Fi");
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado!");
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nErro ao conectar ao Wi-Fi.");
  }

  configTime(-3 * 3600, 0, "pool.ntp.org"); 

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();
  Serial.println("Servidor Web iniciado");
}

void loop() {
  server.handleClient();

  bool sensorState = digitalRead(sensorPin) == LOW; 

  if (sensorState && !lastSensorState) {
    Serial.println("Movimento detectado!");
    addEventLog();

    if (alarmActive && !buzzerTriggered) {
      tone(buzzerPin, 1000);  
      buzzerTriggered = true;
    }
  }

  lastSensorState = sensorState;
}