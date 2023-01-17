#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <string.h>

#define PROYECTO    "SBC22-T-05 -> Huerto Hidropónico"

// Credenciales del WiFi.
#define WIFI_NAME       "SBC"
#define WIFI_PASSWORD   "SBCwifi$"

// Credenciales de ThingsBoard.
#define THINGSBOARD_TOKEN   "Cx9cvuphrFV6K37zaCox"
#define THINGSBOARD_SERVER  "mqtt://demo.thingsboard.io"

// Credenciales de Telegram y el bot.
#define TELEGRAM_SERVER "https://api.telegram.org"
#define BOT_TOKEN       "5885102192:AAGu2_2Dv7RexPQr33EMPs0QVtpbFunBqZw"
#define BOT_URL         ""   // A falta del método.

//strcat(BOT_URL, TELEGRAM_SERVER);
//strcat(BOT_URL, "/bot");
//strcat(BOT_URL, BOT_TOKEN);

#endif