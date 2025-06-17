#include <WiFi.h>                       // Biblioteca para conectar o ESP32 à rede Wi-Fi
#include <WebServer.h>                  // Biblioteca para criar servidor web no ESP32
#include <vector>                       // Biblioteca para usar vetor dinâmico (armazenar logs)
#include <ctime>                        // Biblioteca para manipular data e hora
#include <UniversalTelegramBot.h>      // Biblioteca para integração com o Bot do Telegram
#include <WiFiClientSecure.h>          // Biblioteca para conexões HTTPS
#include <ArduinoJson.h>               // Biblioteca para manipular JSON (usada pelo Telegram)

#define BOTtoken "7685826266:AAH9UlPxcRZoO6582Wu4WqKbMnW2yvfDAGo"  // Token do Bot do Telegram
#define CHAT_ID "5206958509"             // ID do chat para onde o bot vai enviar as mensagens

const char* ssid = "VIVOFIBRA-C9E8";     // Nome da rede Wi-Fi
const char* password = "2rB2HrmCx8";     // Senha do Wi-Fi

WiFiClientSecure client;                                // Cliente seguro para conexão HTTPS
UniversalTelegramBot bot(BOTtoken, client);             // Objeto do bot do Telegram

const int sensorMovimentoPin = 23;       // Pino do sensor de movimento (TCRT5000 ou outro)
const int sensorMC38Pin = 19;            // Pino do sensor magnético (MC-38)
const int buzzerPin = 22;                // Pino do buzzer

bool alarmActive = true;                 // Flag para saber se o alarme está ativado
bool mc38Active = true;                  // Flag para saber se o sensor MC-38 está ativado
bool buzzerTriggered = false;            // Flag para evitar múltiplos toques do buzzer
bool lastMovimentoState = false;         // Último estado do sensor de movimento
bool lastMC38State = false;              // Último estado do sensor MC-38

WebServer server(80);                    // Criação do servidor web na porta 80
std::vector<String> eventLog;            // Vetor que armazena as mensagens de log

// Função para obter o timestamp atual formatado
String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Data/Hora indisponível";
  }
  char buffer[64];
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

// Adiciona um novo evento ao log
void addEventLog(String mensagem) {
  eventLog.push_back(mensagem + " em: " + getTimestamp());  // Adiciona mensagem + horário
  if (eventLog.size() > 20) {                               // Mantém máximo 20 eventos
    eventLog.erase(eventLog.begin());                       // Remove o mais antigo
  }
}

// Manipula a página principal HTML (interface de controle)
void handleRoot() {
  String html = R"rawliteral( /* HTML com placeholders que serão substituídos */
    <html>
    <head>
      <style> /* CSS da página */
        body { font-family: Arial; background-color: #f4f4f4; color: #333; text-align: center; padding: 20px; }
        h1 { color: #222; }
        .status { font-size: 18px; margin: 10px 0; }
        .status span { font-weight: bold; }
        .ligado { color: green; }
        .desligado { color: red; }
        .button { display: inline-block; padding: 10px 20px; margin: 8px 10px; font-size: 16px; border: none; border-radius: 8px; cursor: pointer; transition: background-color 0.3s; }
        .on-btn { background-color: #28a745; color: white; }
        .on-btn:hover { background-color: #218838; }
        .off-btn { background-color: #dc3545; color: white; }
        .off-btn:hover { background-color: #c82333; }
        .mute-btn { background-color: #007bff; color: white; }
        .mute-btn:hover { background-color: #0069d9; }
        ul { text-align: left; max-width: 600px; margin: auto; background: #fff; padding: 15px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
        li { padding: 5px 0; border-bottom: 1px solid #ccc; }
      </style>
    </head>
    <body>
      <h1>Monitor de Movimento e Abertura</h1>

      <div class="status">
        Sistema de alarme Principal: <span class="%STATUS_CLASS%">%STATUS_TEXT%</span>
      </div>
      <div class="status">
        Sensor de janela: <span class="%MC38_CLASS%">%MC38_TEXT%</span>
      </div>

      <a href="/ligar"><button class="button on-btn">Ligar Alarme</button></a>
      <a href="/desligar"><button class="button off-btn">Desligar Alarme</button></a>
      <a href="/toggle"><button class="button mute-btn">Silenciar Buzzer</button></a>
      <br>
      <a href="/mc38_ligar"><button class="button on-btn">Ligar sensor de janela</button></a>
      <a href="/mc38_desligar"><button class="button off-btn">Desligar sensor de janela</button></a>

      <h2>Log de Eventos</h2>
      <ul id="logList">%LOG%</ul>

      <script>
        function updateLog() {
          fetch("/log")                               // Requisição para /log
            .then(response => response.text())
            .then(data => {
              document.getElementById("logList").innerHTML = data; // Atualiza a lista
            });
        }
        setInterval(updateLog, 1000); // Atualiza log a cada 1 segundo
      </script>
    </body>
    </html>
  )rawliteral";

  // Substitui os placeholders pelo status real
  html.replace("%STATUS_CLASS%", alarmActive ? "ligado" : "desligado");
  html.replace("%STATUS_TEXT%", alarmActive ? "Ligado" : "Desligado");
  html.replace("%MC38_CLASS%", mc38Active ? "ligado" : "desligado");
  html.replace("%MC38_TEXT%", mc38Active ? "Ativado" : "Desativado");

  // Monta os itens do log
  String logHtml;
  for (int i = eventLog.size() - 1; i >= 0; --i) {
    logHtml += "<li>" + eventLog[i] + "</li>";
  }
  html.replace("%LOG%", logHtml);  // Substitui o log na página

  server.send(200, "text/html", html);  // Envia a resposta HTML para o navegador
}

// Manipula clique em "Silenciar Buzzer"
void handleToggle() {
  noTone(buzzerPin);                        // Desliga o buzzer
  buzzerTriggered = false;                 // Reinicia o controle do buzzer
  bot.sendMessage(CHAT_ID, "🔇 Alarme foi silenciado.", "");  // Envia alerta no Telegram
  server.sendHeader("Location", "/");      // Redireciona de volta para a página
  server.send(303);                        // Código de redirecionamento
}

// Ativa o alarme principal
void handleLigar() {
  alarmActive = true;
  bot.sendMessage(CHAT_ID, "🔔 Sistema de alarme foi ativado!", "");
  server.sendHeader("Location", "/");
  server.send(303);
}

// Desativa o alarme
void handleDesligar() {
  alarmActive = false;
  noTone(buzzerPin);  // Desliga o buzzer ao desativar
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

// Envia o log em tempo real para o navegador
void handleLog() {
  String logHtml;
  for (int i = eventLog.size() - 1; i >= 0; --i) {
    logHtml += "<li>" + eventLog[i] + "</li>";
  }
  server.send(200, "text/html", logHtml);  // Envia somente o conteúdo do log
}

// Função de inicialização
void setup() {
  Serial.begin(19200);                      // Inicializa a comunicação serial
  pinMode(sensorMovimentoPin, INPUT);      // Define pino do sensor de movimento
  pinMode(sensorMC38Pin, INPUT_PULLUP);    // Define pino do sensor MC-38 com pull-up
  pinMode(buzzerPin, OUTPUT);              // Define pino do buzzer
  client.setInsecure();                    // Conexão HTTPS sem verificação de certificado

  WiFi.begin(ssid, password);              // Conecta ao Wi-Fi
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

  configTime(-3 * 3600, 0, "pool.ntp.org");  // Configura fuso horário e servidor NTP

  // Define as rotas que o servidor irá tratar
  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/ligar", handleLigar);
  server.on("/desligar", handleDesligar);
  server.on("/mc38_ligar", handleMC38Ligar);
  server.on("/mc38_desligar", handleMC38Desligar);
  server.on("/log", handleLog);

  server.begin();  // Inicia o servidor
  Serial.println("Servidor Web iniciado");
}

// Loop principal
void loop() {
  server.handleClient();  // Trata requisições dos navegadores

  // Verifica se houve detecção de movimento
  bool movimentoAtual = digitalRead(sensorMovimentoPin) == LOW;
  if (movimentoAtual && !lastMovimentoState) {
    Serial.println("Movimento detectado!");
    addEventLog("🚶 Movimento detectado");
    if (alarmActive && !buzzerTriggered) {
      tone(buzzerPin, 1000);                    // Toca buzzer
      buzzerTriggered = true;                   // Previne múltiplos toques
      bot.sendMessage(CHAT_ID, "🚨 Movimento detectado!", "");  // Alerta no Telegram
    }
  }
  lastMovimentoState = movimentoAtual;

  // Verifica abertura da janela/porta com o MC-38
  bool mc38Atual = digitalRead(sensorMC38Pin) == HIGH;
  if (mc38Active && mc38Atual && !lastMC38State) {
    Serial.println("Porta/janela aberta!");
    addEventLog("🚪 Porta ou janela aberta");
    if (alarmActive && !buzzerTriggered) {
      tone(buzzerPin, 1000);                    // Toca o buzzer
      buzzerTriggered = true;
      bot.sendMessage(CHAT_ID, "🚨 Porta ou janela foi aberta!", "");  // Telegram
    }
  }
  lastMC38State = mc38Atual;
}
