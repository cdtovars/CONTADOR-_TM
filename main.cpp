//CRISTIAN DANIEL TOVAR SUAREZ CODIGO: 20202673113
//JULIAN ENRIQUE SIMBAQUEBA CORREA CODIGO : 20221673021

#include "mbed.h"
// Defines
#define escritura       0x40
#define poner_brillo    0x88
#define dir_display     0xC0

// Prototipos
void condicion_start(void);
void condicion_stop(void);
void send_byte(char data);
void send_data(int number);
void leer_pin(void);

// Hilos
Thread T_leer_pin(osPriorityNormal, 4096, NULL, NULL);

// Variables globales
volatile int contador = 0;  // Volátil porque puede ser modificada por interrupciones
volatile bool reset = false;  // Bandera para el reset
volatile bool increment = false;  // Bandera para incrementar el contador
volatile bool automatico = false;  // Bandera para el modo automático
volatile bool decrementar = false;  // Bandera para determinar si se incrementa o decrementa
volatile bool mostrar_dos_digitos = false;  // Bandera para determinar si mostrar solo dos dígitos

// Pines
DigitalOut sclk(D2);
DigitalInOut dio(D3);
DigitalIn boton_reset(D4);  // Botón para resetear el contador
DigitalIn boton_increment(D5);  // Botón para incrementar el contador
DigitalIn boton_modo(D6);  // Botón para cambiar entre modo automático y manual
DigitalIn dip_switch(D7);  // DIP switch para decrementar el contador
DigitalIn dip_switch_dos_digitos(D8);  // DIP switch para mostrar solo dos dígitos

const char digitToSegment[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };

// Tiempo de rebote para los botones
#define Tiempo_rebote 200ms

int main() {
    // Inicializar los botones de interrupción para detectar las pulsaciones
    T_leer_pin.start(leer_pin);  // Iniciar el hilo para la detección de botones

    // Ciclo principal
    while (true) {
        if (reset) {
            contador = 0;  // Resetea el contador si la bandera está activada
            reset = false;  // Reinicia la bandera
            printf("Contador reseteado a 0\n");  // Mensaje de depuración
        }

        if (increment && !automatico) {
            if (decrementar) {
                contador--;  // Decrementar el contador en modo manual
                if (contador < 0) {
                    contador = 9999;  // Si el contador llega a ser negativo, reinícialo a 9999
                }
            } else {
                contador++;  // Incrementar el contador en modo manual
                if (contador > 9999) {
                    contador = 0;  // Reiniciar si supera el límite
                }
            }
            increment = false;  // Reiniciar la bandera
            printf("Contador incrementado manualmente: %d\n", contador);  // Mensaje de depuración
        }

        if (automatico) {
            send_data(contador);  // Muestra el número en el display

            if (decrementar) {
                contador--;  // Decrementa el contador en modo automático
                if (contador < 0) {
                    contador = 9999;  // Si el contador llega a ser negativo, reinícialo a 9999
                }
            } else {
                contador++;  // Incrementa el contador en modo automático
                if (contador > 9999) {
                    contador = 0;  // Reiniciar el contador si supera 9999
                }
            }

            ThisThread::sleep_for(1s);  // Espera 1 segundo antes de la siguiente actualización
        } else {
            send_data(contador);  // Muestra el número en el display
        }
    }
}

void leer_pin(void) {
    bool Q0_reset = 0;
    bool Q1_reset = 0;
    bool Q2_reset = 0;

    bool Q0_increment = 0;
    bool Q1_increment = 0;
    bool Q2_increment = 0;

    bool Q0_modo = 0;
    bool Q1_modo = 0;
    bool Q2_modo = 0;

    while (true) {
        // Leer el estado del botón de reset
        Q2_reset = Q1_reset;
        Q1_reset = Q0_reset;
        Q0_reset = boton_reset.read();
        if (!Q0_reset && Q1_reset && Q2_reset) {
            reset = true;  // Activar la bandera de reset
        }

        // Leer el estado del botón de incremento (solo en modo manual)
        if (!automatico) {
            Q2_increment = Q1_increment;
            Q1_increment = Q0_increment;
            Q0_increment = boton_increment.read();
            if (!Q0_increment && Q1_increment && Q2_increment) {
                increment = true;  // Activar la bandera de incremento
            }
        }

        // Leer el estado del botón de modo
        Q2_modo = Q1_modo;
        Q1_modo = Q0_modo;
        Q0_modo = boton_modo.read();
        if (!Q0_modo && Q1_modo && Q2_modo) {
            automatico = !automatico;  // Alternar el modo automático/manual
            printf("Modo %s\n", automatico ? "Automático" : "Manual");  // Mensaje de depuración
        }

        // Leer el estado del DIP switch para decrementar
        decrementar = dip_switch.read();  // Determina si el DIP switch está activado o desactivado

        // Leer el estado del DIP switch para mostrar solo dos dígitos
        mostrar_dos_digitos = dip_switch_dos_digitos.read();  // Determina si mostrar solo 2 dígitos o los 4

        ThisThread::sleep_for(Tiempo_rebote);  // Esperar para evitar rebotes
    }
}

void condicion_start(void) {
    sclk = 1;
    dio.output();
    dio = 1;
    wait_us(1);
    dio = 0;
    sclk = 0;
}

void condicion_stop(void) {
    sclk = 0;
    dio.output();
    dio = 0;
    wait_us(1);
    sclk = 1;
    dio = 1;
}

void send_byte(char data) {
    dio.output();
    for (int i = 0; i < 8; i++) {
        sclk = 0;
        dio = (data & 0x01) ? 1 : 0;
        data >>= 1;
        sclk = 1;
    }
    // Esperar el ack
    dio.input();
    sclk = 0;
    wait_us(1);

    if (dio == 0) {
        sclk = 1;
        sclk = 0;
    }
}

void send_data(int number) {
    int digits[4];  // Arreglo para almacenar los dígitos

    // Descomponer el número en dígitos
    for (int i = 3; i >= 0; i--) {
        digits[i] = number % 10;
        number /= 10;
    }

    condicion_start();
    send_byte(escritura);  // Modo de escritura
    condicion_stop();
    condicion_start();
    send_byte(dir_display);  // Dirección de inicio del display

    // Enviar los dígitos al display
    // Si el DIP switch está activado, solo muestra los primeros 2 dígitos
    if (mostrar_dos_digitos) {
        for (int i = 2; i < 4; i++) {
            send_byte(digitToSegment[digits[i]]);
        }
    } else {
        // Si está desactivado, muestra los 4 dígitos
        for (int i = 0; i < 4; i++) {
            send_byte(digitToSegment[digits[i]]);
        }
    }

    condicion_stop();
    condicion_start();
    send_byte(poner_brillo + 1);  // Configurar el brillo
    condicion_stop();
}
