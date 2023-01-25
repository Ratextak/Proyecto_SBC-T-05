idf_component_register(SRCS "respuestas_bot.c" "conexion_mqtt_thingsboard.c" "conexion_http_telegram.c" "main.c" "ds18b20.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES telegram_certificate.pem)