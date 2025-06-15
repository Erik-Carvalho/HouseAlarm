🔔 Sistema de Alarme com ESP32, Web Interface e Notificações via Telegram
Este projeto implementa um sistema de alarme residencial usando o ESP32, sensores de movimento e magnético, um buzzer, e uma interface web acessível via navegador. Ele também envia alertas em tempo real para o Telegram sempre que um evento for detectado.

📦 Recursos
📡 Conexão Wi-Fi para acesso remoto

🌐 Interface web com:

Estado do sistema

Botões para ligar/desligar o alarme

Habilitar/desabilitar o sensor MC-38 (janela/porta)

Visualização do log de eventos em tempo real

🛎️ Buzzer que soa ao detectar invasão

📲 Notificações via Telegram ao detectar:

Movimento (TCRT5000 ou similar)

Porta/janela aberta (MC-38)

Buzzer silenciado manualmente

🕒 Log de eventos com data e hora (via NTP)

🧰 Componentes Utilizados
ESP32

Sensor de movimento (ex: TCRT5000)

Sensor magnético MC-38 (porta/janela)

Buzzer piezoelétrico

Conexão Wi-Fi

Conta no Telegram com Bot configurado

🖥️ Interface Web
A interface é acessível pelo IP local do ESP32 e fornece:

Estado atual do alarme

Botões para ativar/desativar o alarme e sensor MC-38

Log de eventos atualizando a cada 1 segundo

📲 Configuração do Telegram
Crie um bot via BotFather e anote o token.

Envie uma mensagem qualquer ao bot.

Use a URL https://api.telegram.org/bot<SEU_TOKEN>/getUpdates para obter seu chat_id.

Preencha as constantes BOTtoken e CHAT_ID no código com os dados do seu bot.

⚙️ Como Usar
Conecte os sensores e o buzzer aos pinos definidos no código:

sensorMovimentoPin → Pino 23

sensorMC38Pin → Pino 19

buzzerPin → Pino 22

Configure o nome e a senha do Wi-Fi:

const char* ssid = "SEU_SSID";
const char* password = "SUA_SENHA";
Compile e envie o código para o ESP32 usando a Arduino IDE.

Abra o monitor serial para visualizar o IP atribuído.

Acesse http://<ip_do_esp32> no navegador.

📝 Exemplo de Log
🚶 Movimento detectado em: 15/06/2025 17:34:12

🚪 Porta ou janela aberta em: 15/06/2025 17:34:45

🔇 Alarme foi silenciado em: 15/06/2025 17:35:03

📷 Imagens (opcional)
Você pode adicionar capturas de tela da interface web ou do protótipo montado.

🔒 Segurança
O cliente HTTPS ignora certificados (client.setInsecure()), ideal para protótipos locais.

Em produção, recomenda-se usar certificados válidos.

🛠️ Dependências
Certifique-se de instalar as seguintes bibliotecas na Arduino IDE:

WiFi

WebServer

WiFiClientSecure

UniversalTelegramBot

ArduinoJson

📌 Observações
O log armazena os últimos 20 eventos.

O horário é sincronizado via servidor NTP (pool.ntp.org) com fuso de Brasília (-3 GMT).

Para testes sem sensores físicos, é possível simular entradas com digitalWrite() ou jumpers.
