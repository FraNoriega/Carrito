#include <Bluepad32.h>

ControllerPtr myControllers[BP32_MAX_GAMEPADS];
int lastAngulo = -1;
int lastVelocidad = -1;
unsigned long lastSendTime = 0;

// Cabecera estricta para evitar que el ruido se lea como comando
const uint8_t HEADER = 0xFF; 

void onConnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            myControllers[i] = ctl;
            break;
        }
    }
}

void setup() {
    Serial.begin(115200);
    // Comunicación con el XBee
    Serial2.begin(115200, SERIAL_8N1, 16, 17); 
    BP32.setup(&onConnectedController, nullptr);
}

void loop() {
    BP32.update();
    unsigned long now = millis();

    for (auto ctl : myControllers) {
        if (ctl && ctl->isConnected() && ctl->hasData()) {
            
            // --- EJE Y: ADELANTE / ATRÁS (Joystick Izquierdo) ---
            int rawY = ctl->axisY(); 
            if (abs(rawY) < 40) rawY = 0; // Zona muerta mecánica
            
            // IMPORTANTE: En el mando PS4, empujar hacia arriba da valores negativos (-512).
            // Mapeamos invertido para que Arriba sea 253 (Máxima velocidad) y Abajo sea 0 (Reversa máxima).
            int velocidad = map(rawY, -508, 512, 253, 0); 
            velocidad = constrain(velocidad, 0, 253);

            // --- EJE X: IZQUIERDA / DERECHA (Joystick Izquierdo) ---
            int rawX = ctl->axisX();
            if (abs(rawX) < 40) rawX = 0; // Zona muerta mecánica
            
            // Mapeamos el giro a una escala de 0 a 180
            int angulo = map(rawX, -508, 512, 0, 180);
            angulo = constrain(angulo, 0, 180);

            // --- ENVÍO DE DATOS ---
            // Enviamos si hay cambios o cada 50ms para mantener el carrito activo (Keep-Alive)
            if ((angulo != lastAngulo || velocidad != lastVelocidad) || (now - lastSendTime > 50)) {
                
                uint8_t checksum = (angulo + velocidad) & 0xFE; 
                
                Serial2.write(HEADER);
                Serial2.write((uint8_t)angulo);
                Serial2.write((uint8_t)velocidad);
                Serial2.write(checksum);
                
                lastAngulo = angulo;
                lastVelocidad = velocidad;
                lastSendTime = now;
            }
        }
    }
}