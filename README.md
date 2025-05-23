
# 🔔 Monitor de Movimento com ESP32 + Interface Web

Este projeto utiliza um **ESP32**, um **sensor infravermelho TCRT5000** e um **buzzer** para detectar movimento, ativar um alarme sonoro e exibir os eventos registrados em uma página web acessível via Wi-Fi.

---

## 🚀 Funcionalidades

- Conexão à rede Wi-Fi configurada no código
- Detecção de movimento via sensor infravermelho
- Ativação de alarme sonoro com buzzer
- Interface web com:
  - Status do alarme (ativado/desativado)
  - Log dos últimos 20 eventos de movimento
  - Botão para silenciar o alarme

---

## 🔧 Componentes Utilizados

- ESP32
- Sensor infravermelho TCRT5000 (ou similar)
- Buzzer passivo
- Conexão Wi-Fi

---

## 📲 Como Usar

1. **Configure seu Wi-Fi no código:**

```cpp
const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA";
```

2. **Carregue o código no ESP32 usando a IDE Arduino**

3. **Abra o monitor serial (baud rate 19200)** e anote o IP fornecido

4. **Acesse o IP pelo navegador** para visualizar a interface web

---

## 🌐 Interface Web

A página mostra:

- Estado do alarme (ativado/desativado)
- Estado do buzzer (tocando ou silencioso)
- Lista dos últimos eventos de movimento detectado
- Botão para silenciar o alarme

---

## 📝 Exemplo de Log

```text
Movimento detectado em: 23/05/2025 14:32:17
Movimento detectado em: 23/05/2025 14:30:05
...
```

---

## 📁 Estrutura do Código

- `setup()`: conecta ao Wi-Fi, configura pinos e inicia o servidor web
- `loop()`: monitora o sensor, aciona o alarme e atualiza a interface
- `getTimestamp()`: obtém data/hora atual via NTP
- `addEventLog()`: salva eventos recentes (máximo de 20)
- `handleRoot()`: renderiza a página HTML principal
- `handleToggle()`: desativa o alarme/buzzer via interface

---

## ⚠️ Observações

- O sensor está configurado como **ativo em nível baixo** (`LOW`).
- O horário é obtido com base no fuso UTC-3 (`configTime(-3 * 3600, 0, ...)`).
- O botão da interface não desativa o alarme, apenas silencia o buzzer.

---

## 📜 Licença

Este projeto é de uso livre para fins educacionais e pode ser adaptado conforme sua necessidade.
