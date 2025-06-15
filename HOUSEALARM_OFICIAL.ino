#include <WiFi.h>                        // Biblioteca para conexão Wi-Fi
#include <WebServer.h>                   // Biblioteca para criar servidor web
#include <vector>                        // Biblioteca para usar vetor dinâmico
#include <ctime>                         // Biblioteca para manipulação de data e hora
#include <UniversalTelegramBot.h>       // Biblioteca para enviar mensagens via Telegram
#include <WiFiClientSecure.h>           // Biblioteca para conexões HTTPS
#include <ArduinoJson.h>                // Biblioteca para trabalhar com JSON (usada no Telegram)

#define BOTtoken "..."                  // Token do seu bot Telegram (substitua pelo seu)
#define CHAT_ID "..."                   // ID do chat do Telegram para onde as mensagens serão enviadas
const char* ssid = "";     // Nome da rede Wi-Fi
const char* password = "";     // Senha do Wi-Fi

WiFiClientSecure client;                // Cliente seguro para HTTPS
UniversalTelegramBot bot(BOTtoken, client); // Instancia o bot do Telegram com o token e o cliente seguro

// Pinos conectados aos sensores e buzzer
const int sensorMovimentoPin = 23;
const int sensorMC38Pin = 19;
const int buzzerPin = 22;

// Flags de controle
bool alarmActive = true;                // Alarme ligado por padrão
bool mc38Active = true;                 // Sensor de janela ativado por padrão
bool buzzerTriggered = false;           // Buzzer foi acionado?
bool lastMovimentoState = false;        // Último estado do sensor de movimento
bool lastMC38State = false;             // Último estado do sensor MC-38

WebServer server(80);                   // Cria servidor web na porta 80
std::vector<String> eventLog;           // Vetor para armazenar o log de eventos

// Função que retorna a data/hora atual como string
String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Data/Hora indisponível";
  }
  char buffer[64];
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

// Adiciona mensagem ao log, com data/hora
void addEventLog(String mensagem) {
  eventLog.push_back(mensagem + " em: " + getTimestamp());
  if (eventLog.size() > 20) { // Mantém até 20 eventos no log
    eventLog.erase(eventLog.begin());
  }
}

// Página principal HTML do sistema
void handleRoot() {
  String html = R"rawliteral(
    ... // [HTML omitido por brevidade, já que não há código C++ aqui, apenas HTML e JavaScript]
  )rawliteral";

  // Substitui os marcadores do HTML com o estado atual do sistema
  html.replace("%STATUS_CLASS%", alarmActive ? "ligado" : "desligado");
  html.replace("%STATUS_TEXT%", alarmActive ? "Ligado" : "Desligado");
  html.replace("%MC38_CLASS%", mc38Active ? "ligado" : "desligado");
  html.replace("%MC38_TEXT%", mc38Active ? "Ativado" : "Desativado");

  // Monta o log em formato HTML
  String logHtml;
  for (int i = eventLog.size() - 1; i >= 0; --i) {
    logHtml += "<li>" + eventLog[i] + "</li>";
  }
  html.replace("%LOG%", logHtml);

  server.send(200, "text/html", html); // Envia a página ao navegador
}

// Silencia o buzzer e envia mensagem no Telegram
void handleToggle() {
  noTone(buzzerPin);
  buzzerTriggered = false;
  bot.sendMessage(CHAT_ID, "🔇 Alarme foi silenciado.", "");
  server.sendHeader("Location", "/");  // Redireciona para a página principal
  server.send(303);
}

// Liga o alarme principal
void handleLigar() {
  alarmActive = true;
  bot.sendMessage(CHAT_ID, "🔔 Sistema de alarme foi ativado!", "");
  server.sendHeader("Location", "/");
  server.send(303);
}

// Desliga o alarme principal
void handleDesligar() {
  alarmActive = false;
  noTone(buzzerPin);
  buzzerTriggered = false;
  bot.sendMessage(CHAT_ID, "🔕 Sistema de alarme foi desligado.", "");
  server.sendHeader("Location", "/");
  server.send(303);
}

// Ativa o sensor de janela (MC-38)
void handleMC38Ligar() {
  mc38Active = true;
  bot.sendMessage(CHAT_ID, "✅ Sensor de janela foi ativado.", "");
  server.sendHeader("Location", "/");
  server.send(303);
}

// Desativa o sensor de janela (MC-38)
void handleMC38Desligar() {
  mc38Active = false;
  bot.sendMessage(CHAT_ID, "⛔ Sensor de janela foi desativado.", "");
  server.sendHeader("Location", "/");
  server.send(303);
}

// Retorna o log de eventos no formato HTML (atualizado a cada 1s via JavaScript)
void handleLog() {
  String logHtml;
  for (int i = eventLog.size() - 1; i >= 0; --i) {
    logHtml += "<li>" + eventLog[i] + "</li>";
  }
  server.send(200, "text/html", logHtml);
}

// Função de configuração inicial
void setup() {
  Serial.begin(19200);                         // Inicia comunicação serial
  pinMode(sensorMovimentoPin, INPUT);          // Define pino do sensor de movimento como entrada
  pinMode(sensorMC38Pin, INPUT_PULLUP);        // Define pino do MC-38 como entrada com pull-up
  pinMode(buzzerPin, OUTPUT);                  // Define pino do buzzer como saída
  client.setInsecure();                        // Ignora verificação de certificado SSL

  // Conecta ao Wi-Fi
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

  configTime(-3 * 3600, 0, "pool.ntp.org"); // Configura horário de Brasília via NTP

  // Define as rotas do servidor web
  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/ligar", handleLigar);
  server.on("/desligar", handleDesligar);
  server.on("/mc38_ligar", handleMC38Ligar);
  server.on("/mc38_desligar", handleMC38Desligar);
  server.on("/log", handleLog);

  server.begin();                       // Inicia o servidor web
  Serial.println("Servidor Web iniciado");
}

// Loop principal (executa continuamente)
void loop() {
  server.handleClient();               // Lida com requisições HTTP

  // Verifica se há movimento
  bool movimentoAtual = digitalRead(sensorMovimentoPin) == LOW;
  if (movimentoAtual && !lastMovimentoState) {
    Serial.println("Movimento detectado!");
    addEventLog("🚶 Movimento detectado");
    if (alarmActive && !buzzerTriggered) {
      tone(buzzerPin, 1000);           // Liga buzzer com tom de 1000 Hz
      buzzerTriggered = true;
      bot.sendMessage(CHAT_ID, "🚨 Movimento detectado!", "");
    }
  }
  lastMovimentoState = movimentoAtual;

  // Verifica abertura do sensor MC-38 (porta/janela)
  bool mc38Atual = digitalRead(sensorMC38Pin) == HIGH;
  if (mc38Active && mc38Atual && !lastMC38State) {
    Serial.println("Porta/janela aberta!");
    addEventLog("🚪 Porta ou janela aberta");
    if (alarmActive && !buzzerTriggered) {
      tone(buzzerPin, 1000);
      buzzerTriggered = true;
      bot.sendMessage(CHAT_ID, "🚨 Porta ou janela foi aberta!", "");
    }
  }
  lastMC38State = mc38Atual;
}
