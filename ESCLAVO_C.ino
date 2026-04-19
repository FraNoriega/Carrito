#include <Arduino.h>

// --- MOTOR A (IZQUIERDO) ---
const int pinENA = 14; 
const int pinIN1 = 26; 
const int pinIN2 = 27;

// --- MOTOR B (DERECHO) ---
// PINES CORREGIDOS Y CONFIRMADOS
const int pinENB = 25; 
const int pinIN3 = 33; 
const int pinIN4 = 32;

const uint8_t HEADER = 0xFF;

// Variables Filtro Anti-Ruido
float anguloActual = 90.0;     
float velocidadActual = 126.0; 
const float alpha = 0.7;       

unsigned long ultimaVezRecibido = 0; 

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, 16, 17);
    
    pinMode(pinIN1, OUTPUT); pinMode(pinIN2, OUTPUT);
    pinMode(pinIN3, OUTPUT); pinMode(pinIN4, OUTPUT);
    
    ledcAttach(pinENA, 5000, 8); 
    ledcAttach(pinENB, 5000, 8); 
}

void controlarMotor(int in1, int in2, int pinEN, int velocidadCalculada) {
    if (velocidadCalculada > 20) { // ADELANTE
        int pwm = map(velocidadCalculada, 20, 255, 80, 255); 
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        ledcWrite(pinEN, pwm);
    } 
    else if (velocidadCalculada < -20) { // REVERSA
        int pwm = map(velocidadCalculada, -20, -255, 80, 255);
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        ledcWrite(pinEN, pwm);
    } 
    else { // STOP
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        ledcWrite(pinEN, 0); 
    }
}

void loop() {
    // --- 1. RECEPCIÓN SEGURA (Protocolo) ---
    while (Serial2.available() >= 4) {
        if (Serial2.read() == HEADER) {
            uint8_t angRecibido = Serial2.read();
            uint8_t velRecibida = Serial2.read();
            uint8_t chkRecibido = Serial2.read();

            if (chkRecibido == ((angRecibido + velRecibida) & 0xFE)) {
                anguloActual = (alpha * angRecibido) + ((1.0 - alpha) * anguloActual);
                velocidadActual = (alpha * velRecibida) + ((1.0 - alpha) * velocidadActual);
                ultimaVezRecibido = millis(); 
            }
        }
    }

    // Fail-Safe
    if (millis() - ultimaVezRecibido > 250) {
        anguloActual = 90.0;     
        velocidadActual = 126.0; 
    }

    // --- 2. LÓGICA ESTRICTA (Sin interferencia de ruido) ---
    int ejeY = map(round(velocidadActual), 0, 253, -255, 255); 
    int ejeX = map(round(anguloActual), 0, 180, -255, 255); 

    if (abs(ejeY) < 60) ejeY = 0;
    if (abs(ejeX) < 60) ejeX = 0;

    int velIzquierda = 0;
    int velDerecha   = 0;

    // Prioridad 1: Acelerar/Frenar
    if (ejeY != 0) { 
        velIzquierda = ejeY;
        velDerecha   = ejeY;
    }
    // Prioridad 2: Girar (Solo si no estás acelerando)
    else if (ejeX != 0) {
        if (ejeX < 0) { // JOYSTICK IZQUIERDA
            velIzquierda = map(abs(ejeX), 60, 255, 80, 255); 
            velDerecha   = 0; 
        } 
        else { // JOYSTICK DERECHA
            velIzquierda = 0; 
            velDerecha   = map(abs(ejeX), 60, 255, 80, 255); 
        }
    }

    // --- 3. EJECUCIÓN ---
    controlarMotor(pinIN1, pinIN2, pinENA, constrain(velIzquierda, -255, 255)); 
    controlarMotor(pinIN3, pinIN4, pinENB, constrain(velDerecha, -255, 255));   

    delay(2); 
}