#ifndef CREDENTIALS_H
#define CREDENTIALS_H


#define PROYECTO    "SBC22-T-05 -> Huerto Hidropónico"

// Credenciales del WiFi.
#define WIFI_NAME       "SBC"
#define WIFI_PASSWORD   "SBCwifi$"

// Credenciales de ThingsBoard.
#define THINGSBOARD_TOKEN   "YYrBxdD2cfcKrVWhNj9x"
#define THINGSBOARD_SERVER  "mqtt://demo.thingsboard.io"

// Credenciales de Telegram y el bot.
#define TELEGRAM_SERVER "https://api.telegram.org"
#define BOT_TOKEN       "5885102192:AAGu2_2Dv7RexPQr33EMPs0QVtpbFunBqZw"
#define BOT_CHAT_ID     -843736526
#define BOT_URL         TELEGRAM_SERVER "/bot" BOT_TOKEN   // A falta del método.

#endif