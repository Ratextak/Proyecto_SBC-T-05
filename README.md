# Huerto Hidropónico

Proyecto para la asignatura de Sistemas Basados en Computador (SBC).  
Grupo: SBC-T-05.  

Se ha desarrollado un prototipo para dispensar nutrientes (*N, P, K*) en un tanque anexado a un huerto hidropónico.  
Se medirá, mediante sensores, la electroconductividad y la temperatura del agua (*ds18b20*).  
Cuando la cantidad de nutrientes sea la deseada el tanque se vaciará en el circuito principal del huerto al que pertenezca.

## Conexión ThingsBoard

Se realiza mediante el protocolo **MQTT**.
La *ESP-32* mandará los datos de las mediciones, de manera periódica, a la plataforma IoT *ThingsBoard*.
Allí los datos podrán ser consultados por el usuario mediante un panel (dashboard).

## Conexión Telegram

Se realiza mediante el protocolo **HTTP**, tanto desde la *ESP-32* como desde *ThingsBoard*. 
A través de un bot de *Telegram* se recibirán alertas y datos, que podrán ser consultados por el usuario.  
También se podrá interactuar con el sistema: pidiendo datos, requiriendo la dispensión de los nutrientes al medio o vaciando el tanque.

## Configuraciones

### Credentials.h

Este fichero deberá ser editado por el usuario final; indicando:
- Los datos de su conexión WiFi.
- Credenciales de ThingsBoard.
- Credenciales de su bot de Telegram.
