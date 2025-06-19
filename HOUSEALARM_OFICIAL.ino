#include <WiFi.h>                       // Biblioteca para conectar o ESP32 à rede Wi-Fi
#include <WebServer.h>                  // Biblioteca para criar servidor web no ESP32
#include <vector>                       // Biblioteca para usar vetor dinâmico (armazenar logs)
#include <ctime>                        // Biblioteca para manipular data e hora
#include <UniversalTelegramBot.h>      // Biblioteca para integração com o Bot do Telegram
#include <WiFiClientSecure.h>          // Biblioteca para conexões HTTPS
#include <ArduinoJson.h>               // Biblioteca para manipular JSON (usada pelo Telegram)

#define BOTtoken ""  // Token do Bot do Telegram
#define CHAT_ID ""             // ID do chat para onde o bot vai enviar as mensagens

const char* ssid = "";     // Nome da rede Wi-Fi
const char* password = "";     // Senha do Wi-Fi

WiFiClientSecure client;                                // Cliente seguro para conexão HTTPS
UniversalTelegramBot bot(BOTtoken, client);             // Objeto do bot do Telegram

const int sensorMovimentoPin = 23;       // Pino do sensor de movimento (TCRT5000 ou outro)
const int sensorMC38Pin = 19;            // Pino do sensor magnético (MC-38)
const int buzzerPin = 22;                // Pino do buzzer

bool alarmActive = true;                 // Flag para saber se o alarme está ativado
bool mc38Active = true;                  // Flag para saber se o sensor MC-38 está ativado
bool movimentoActive = true;             // flag de controle do sensor de movimento
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
  String html = R"rawliteral(
    <html>
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <title>HouseAlarm Monitoramento </title>
      <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.0/css/all.min.css">
      <style>
        body {
          font-family: Arial, sans-serif;
          margin: 0;
          padding-top: 70px;
          background-color: #f4f4f4;
          color: #333;
          transition: background 0.3s, color 0.3s;
        }

        header {
          position: fixed;
          top: 0;
          width: 100%;
          background-color: #333;
          color: white;
          text-align: center;
          padding: 15px;
          font-size: 22px;
          z-index: 1000;
          box-shadow: 0 2px 5px rgba(0,0,0,0.2);
        }

        .container {
          max-width: 800px;
          margin: auto;
          padding: 10px;
        }

        .card {
          background: white;
          border-radius: 12px;
          padding: 20px;
          margin-bottom: 20px;
          box-shadow: 0 4px 10px rgba(0,0,0,0.1);
        }

        .status {
          font-size: 18px;
          margin: 10px 0;
        }

        .status span {
          font-weight: bold;
        }

        .ligado { color: green; }
        .desligado { color: red; }

        .button-group {
          display: flex;
          flex-wrap: wrap;
          justify-content: center;
          gap: 12px;
          margin-top: 10px;
        }

        .button {
          display: inline-flex;
          align-items: center;
          gap: 8px;
          padding: 10px 20px;
          font-size: 16px;
          border: none;
          border-radius: 8px;
          cursor: pointer;
          transition: all 0.2s ease;
        }

        .on-btn { background-color: #28a745; color: white; }
        .on-btn:hover { background-color: #218838; }

        .off-btn { background-color: #dc3545; color: white; }
        .off-btn:hover { background-color: #c82333; }

        .mute-btn { background-color: #007bff; color: white; }
        .mute-btn:hover { background-color: #0069d9; }

        .theme-btn {
          margin-top: 10px;
          background-color: #6c757d;
          color: white;
        }

        .theme-btn:hover {
          background-color: #5a6268;
        }

        ul#logList {
          list-style-type: none;
          padding: 10px;
          margin: 0;
          background: #ffffff;
          border: 1px solid #ccc;
          border-radius: 10px;
          height: 250px;
          overflow-y: auto;
          box-shadow: inset 0 1px 4px rgba(0,0,0,0.1);
        }

        li {
          padding: 6px 4px;
          border-bottom: 1px solid #ddd;
        }

        footer {
          text-align: center;
          font-size: 13px;
          color: #888;
          margin-top: 40px;
          margin-bottom: 20px;
        }

        /* Dark mode */
        .dark-mode {
          background-color: #1e1e1e;
          color: #f1f1f1;
        }

        .dark-mode .card {
          background-color: #2c2c2c;
        }

        .dark-mode ul#logList {
          background-color: #3a3a3a;
          border-color: #555;
        }

        .dark-mode footer {
          color: #aaa;
        }
      </style>
    </head>
    <body>
      <header>
        <i class="fas fa-shield-alt"></i> Painel de Monitoramento HouseAlarm
      </header>

      <div class="container">

        <div class="card">
          <div class="status">
            Sistema de alarme Principal: <span class="%STATUS_CLASS%">%STATUS_TEXT%</span>
          </div>
          <div class="status">
            Sensor de janela: <span class="%MC38_CLASS%">%MC38_TEXT%</span>
          </div>
          <div class="status">
            Sensor de movimento: <span class="%MOV_CLASS%">%MOV_TEXT%</span>
          </div>
          <div class="button-group">
            <a href="/ligar"><button class="button on-btn"><i class="fas fa-lock"></i> Ligar Alarme</button></a>
            <a href="/desligar"><button class="button off-btn"><i class="fas fa-unlock"></i> Desligar Alarme</button></a>
            <a href="/toggle"><button class="button mute-btn"><i class="fas fa-volume-mute"></i> Silenciar Buzzer</button></a>
          </div>
          <div class="button-group">
            <a href="/mc38_ligar"><button class="button on-btn"><i class="fas fa-door-open"></i> Ligar sensor de porta/janela</button></a>
            <a href="/mc38_desligar"><button class="button off-btn"><i class="fas fa-door-closed"></i> Desligar sensor de porta/janela</button></a>
            <a href="/movimento_ligar"><button class="button on-btn"><i class="fas fa-running"></i> Ligar sensor de movimento</button></a>
            <a href="/movimento_desligar"><button class="button off-btn"><i class="fas fa-person-walking-arrow-right"></i> Desligar sensor de movimento</button></a>
          </div>
          <div style="text-align: center;">
            <button class="button theme-btn" onclick="toggleTheme()"><i class="fas fa-moon"></i> Alternar Tema</button>
          </div>
        </div>

        <div class="card">
          <h2>Log de Eventos</h2>
          <ul id="logList">%LOG%</ul>
        </div>

        <footer>
          Sistema de Monitoramento HouseAlarm- ESP32 - IP: %IP_ADDRESS%
        </footer>
      </div>

      <script>
        function updateLog() {
          fetch("/log")
            .then(response => response.text())
            .then(data => {
              document.getElementById("logList").innerHTML = data;
            });
        }

        setInterval(updateLog, 1000); // Atualiza log a cada 1 segundo

        function toggleTheme() {
          document.body.classList.toggle('dark-mode');
        }
      </script>
    </body>
    </html>
  )rawliteral";

  html.replace("%STATUS_CLASS%", alarmActive ? "ligado" : "desligado");
  html.replace("%STATUS_TEXT%", alarmActive ? "Ligado" : "Desligado");
  html.replace("%MC38_CLASS%", mc38Active ? "ligado" : "desligado");
  html.replace("%MC38_TEXT%", mc38Active ? "Ativado" : "Desativado");
  html.replace("%MOV_CLASS%", movimentoActive ? "ligado" : "desligado");
  html.replace("%MOV_TEXT%", movimentoActive ? "Ativado" : "Desativado");


  // Preenche log
  String logHtml;
  for (int i = eventLog.size() - 1; i >= 0; --i) {
    logHtml += "<li>" + eventLog[i] + "</li>";
  }
  html.replace("%LOG%", logHtml);

  // Adiciona o IP local do ESP32 ao rodapé
  html.replace("%IP_ADDRESS%", WiFi.localIP().toString());

  server.send(200, "text/html", html);
}

// Ativa o sensor de movimento
void handleMovimentoLigar() {
  movimentoActive = true;
  bot.sendMessage(CHAT_ID, "✅ Sensor de movimento foi ativado.", "");
  server.sendHeader("Location", "/");
  server.send(303);
}

// Desativa o sensor de movimento
void handleMovimentoDesligar() {
  movimentoActive = false;
  bot.sendMessage(CHAT_ID, "⛔ Sensor de movimento foi desativado.", "");
  server.sendHeader("Location", "/");
  server.send(303);
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
  bot.sendMessage(CHAT_ID, "✅ Sensor de porta/janela foi ativado.", "");
  server.sendHeader("Location", "/");
  server.send(303);
}

// Desativa o sensor de janela (MC-38)
void handleMC38Desligar() {
  mc38Active = false;
  bot.sendMessage(CHAT_ID, "⛔ Sensor de porta/janela foi desativado.", "");
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
  Serial.begin(115200);                      // Inicializa a comunicação serial
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
  server.on("/movimento_ligar", handleMovimentoLigar);
  server.on("/movimento_desligar", handleMovimentoDesligar);
  server.on("/log", handleLog);

  server.begin();  // Inicia o servidor
  Serial.println("Servidor Web iniciado");
}

// Loop principal
void loop() {
  server.handleClient();  // Trata requisições dos navegadores

  // Verifica se houve detecção de movimento
  bool movimentoAtual = digitalRead(sensorMovimentoPin) == LOW;
    if (movimentoActive && movimentoAtual && !lastMovimentoState) {
      Serial.println("Movimento detectado!");
      addEventLog("🚶 Movimento detectado");     
      bot.sendMessage(CHAT_ID, "🚨 Movimento detectado!", "");  // Alerta no Telegram


    if (alarmActive && !buzzerTriggered) {
      tone(buzzerPin, 1000);                    // Toca buzzer
      buzzerTriggered = true;                   // Previne múltiplos toques
    }
  }
  lastMovimentoState = movimentoAtual;

  // Verifica abertura da janela/porta com o MC-38
  bool mc38Atual = digitalRead(sensorMC38Pin) == HIGH;
  if (mc38Active && mc38Atual && !lastMC38State) {
    Serial.println("Porta/janela aberta!");
    addEventLog("🚪 Porta ou janela aberta");
     bot.sendMessage(CHAT_ID, "🚨 Porta ou janela foi aberta!", "");
    
    if (alarmActive && !buzzerTriggered) {
      tone(buzzerPin, 1000);                    // Toca o buzzer
      buzzerTriggered = true;
    }
  }
  lastMC38State = mc38Atual;
}
