#ifndef CREDENTIALS_H
#define CREDENTIALS_H


#define PROYECTO    "SBC22-T-05 -> Huerto Hidropónico"

// Credenciales del WiFi.
#define WIFI_NAME       "SBC"
#define WIFI_PASSWORD   "SBCwifi$"

// Credenciales de ThingsBoard.
#define THINGSBOARD_SERVER  "mqtt://demo.thingsboard.io"
#define THINGSBOARD_TOKEN   "Token del dispositivo de TB"

// Credenciales de Telegram y el bot.
#define TELEGRAM_SERVER "https://api.telegram.org"
#define BOT_TOKEN       "Token del bot creado en TG"
#define BOT_CHAT_ID     666  // Id del chat dónde recibiremos los mensajes del bot.
#define BOT_URL         TELEGRAM_SERVER "/bot" BOT_TOKEN   // A falta del método.

#endif