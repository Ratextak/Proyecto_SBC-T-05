#include <stdio.h>
#include <string.h>


void respuestas_bot(char pregunta[], char *mensaje, float datos[]) {
    if (strcmp(pregunta, "/datos") == 0)
        sprintf(mensaje, "Temperatura: %0.1f ºC\nElectroconductividad: %f mS/cm\nTanque: %d", datos[0], datos[1], (int) datos[2]);
    else if (strcmp(pregunta, "/vaciar") == 0)
        sprintf(mensaje, "Vaciando el tanque...");
    else if (strcmp(pregunta, "/dispensar") == 0)
        sprintf(mensaje, "Dispensando nutrientes...");
    else {
        strcat(mensaje, "Las opciones disponibles:\n");
        strcat(mensaje, "/datos: Muestra los valores de los sensores.\n");
        strcat(mensaje, "/dispensar: Dispensa los nutrientes necesarios.\n");
        strcat(mensaje, "/vaciar: Vacia el tanque.");
    }
}

void resp_dispensar(int nutriente, char *mensaje, float ec) {
    if (nutriente == 1)
        sprintf(mensaje, "Nitrógeno: %f mS/cm", ec);
    else if (nutriente == 2)
        sprintf(mensaje, "Fósforo: %f mS/cm", ec);
    else
        sprintf(mensaje, "Potasio: %f mS/cm", ec);
}
