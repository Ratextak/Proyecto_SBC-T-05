#include <stdio.h>
#include <string.h>


int respuestas_bot(char pregunta[], char *mensaje, float datos[]) {
    int opcion = 0;
    char aux[100] = {0};

    if (strcmp(pregunta, "/datos") == 0) {
        sprintf(aux, "Temperatura: %0.1f ºC\nElectroconductividad: %f mS/cm\nTanque: %d", datos[0], datos[1], (int) datos[2]);
        opcion = 1;
    }
    else if (strcmp(pregunta, "/vaciar") == 0) {
        sprintf(aux, "Vaciando el tanque...");
        opcion = 2;
    }
    else if (strcmp(pregunta, "/dispensar") == 0) {
        sprintf(aux, "Dispensando nutrientes...");
        opcion = 3;
    }
    for (int i=0; i <100; i++)
        mensaje[i] = aux[i];

    return opcion;
}

void resp_dispensar(int nutriente, char *mensaje, float ec) {
    char aux[100];
    
    if (nutriente == 1)
        sprintf(aux, "Nitrógeno: %f mS/cm", ec);
    else if (nutriente == 2)
        sprintf(aux, "Fósforo: %f mS/cm", ec);
    else
        sprintf(aux, "Potasio: %f mS/cm", ec);

    for (int i=0; i <100; i++)
        mensaje[i] = aux[i];
}
