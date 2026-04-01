#include <stdlib.h>

#define STATE_RED    0
#define STATE_YELLOW 1
#define STATE_GREEN  2

#define GREEN_TIME_DEFAULT   5000
#define GREEN_TIME_EXTENDED  9000
#define YELLOW_TIME          2000
#define DEBOUNCE_MS          50
#define NUM_INTERSECTIONS    2

typedef struct {
    int redPin;
    int yellowPin;
    int greenPin;
    int buttonPin;

    int           state;
    unsigned long stateStartTime;
    unsigned long stateDuration;

    int  vehicleCount;
    bool trafficWaiting;
    bool buttonHeld;

    const char* name;
} Intersection;


Intersection* intersections = NULL;
int activeIntersection = 0;

void initIntersection(Intersection* inter, int red, int yellow, int green,
                      int btn, const char* name);
void setLightState(Intersection* inter, int newState, unsigned long duration);
void updateIntersection(int index);
void checkButton(Intersection* inter);
void switchActiveIntersection();
void printStatus();
void showMenu();
void handleSerial();

void setup() {
    Serial.begin(9600);

    intersections = (Intersection*) malloc(NUM_INTERSECTIONS * sizeof(Intersection));
    if (intersections == NULL) {
        Serial.println("[FATAL] Memory allocation failed!");
        while (1);
    }

    initIntersection(&intersections[0], 2, 3, 4, 5, "North-South");
    initIntersection(&intersections[1], 6, 7, 8, 9, "East-West");

    setLightState(&intersections[0], STATE_GREEN, GREEN_TIME_DEFAULT);
    setLightState(&intersections[1], STATE_RED,   9999999UL);
    activeIntersection = 0;

    Serial.println("===========================================");
    Serial.println("  Smart Traffic Light Controller Ready");
    Serial.println("  Type 'm' and press Send for the menu.");
    Serial.println("===========================================");
}

void loop() {
    for (int i = 0; i < NUM_INTERSECTIONS; i++) checkButton(&intersections[i]);
    for (int i = 0; i < NUM_INTERSECTIONS; i++) updateIntersection(i);
    if (Serial.available() > 0) handleSerial();
}

void initIntersection(Intersection* inter, int red, int yellow, int green,
                      int btn, const char* name) {
    inter->redPin    = red;
    inter->yellowPin = yellow;
    inter->greenPin  = green;
    inter->buttonPin = btn;
    inter->name      = name;

    inter->state          = STATE_RED;
    inter->stateStartTime = millis();
    inter->stateDuration  = 9999999UL;
    inter->vehicleCount   = 0;
    inter->trafficWaiting = false;
    inter->buttonHeld     = false;

    pinMode(red,    OUTPUT);
    pinMode(yellow, OUTPUT);
    pinMode(green,  OUTPUT);
    pinMode(btn,    INPUT_PULLUP);  // LOW when pressed, HIGH when released

    digitalWrite(red,    LOW);
    digitalWrite(yellow, LOW);
    digitalWrite(green,  LOW);
}

// Always turns all LEDs off first to prevent two being on at once
void setLightState(Intersection* inter, int newState, unsigned long duration) {
    digitalWrite(inter->redPin,    LOW);
    digitalWrite(inter->yellowPin, LOW);
    digitalWrite(inter->greenPin,  LOW);

    switch (newState) {
        case STATE_RED:    digitalWrite(inter->redPin,    HIGH); break;
        case STATE_YELLOW: digitalWrite(inter->yellowPin, HIGH); break;
        case STATE_GREEN:  digitalWrite(inter->greenPin,  HIGH); break;
        default:
            Serial.print("[ERROR] Invalid state for ");
            Serial.print(inter->name);
            Serial.println(". Defaulting to RED.");
            digitalWrite(inter->redPin, HIGH);
            newState = STATE_RED;
    }

    inter->state          = newState;
    inter->stateStartTime = millis();
    inter->stateDuration  = duration;
}

void updateIntersection(int index) {
    Intersection* inter = &intersections[index];

    if (millis() - inter->stateStartTime < inter->stateDuration) return;

    switch (inter->state) {
        case STATE_GREEN:
            setLightState(inter, STATE_YELLOW, YELLOW_TIME);
            Serial.print("["); Serial.print(inter->name); Serial.println("] GREEN -> YELLOW");
            break;

        case STATE_YELLOW:
            setLightState(inter, STATE_RED, 9999999UL);
            Serial.print("["); Serial.print(inter->name); Serial.println("] YELLOW -> RED");
            inter->trafficWaiting = false;
            switchActiveIntersection();
            break;

        case STATE_RED:
            break;
    }
}


void switchActiveIntersection() {
    activeIntersection = 1 - activeIntersection;
    Intersection* next = &intersections[activeIntersection];

    unsigned long greenDuration = next->trafficWaiting ? GREEN_TIME_EXTENDED : GREEN_TIME_DEFAULT;

    Serial.print("["); Serial.print(next->name);
    if (next->trafficWaiting)
        Serial.println("] -> GREEN (EXTENDED - vehicles waiting)");
    else
        Serial.println("] -> GREEN");

    setLightState(next, STATE_GREEN, greenDuration);
}

void checkButton(Intersection* inter) {
    bool pressed = (digitalRead(inter->buttonPin) == LOW);

    if (pressed && !inter->buttonHeld) {
        delay(DEBOUNCE_MS);
        if (digitalRead(inter->buttonPin) == LOW) {
            inter->buttonHeld = true;
            inter->vehicleCount++;

            Serial.print("[VEHICLE] "); Serial.print(inter->name);
            if (inter->state == STATE_GREEN) {
                Serial.print(" - passing vehicle #");
                Serial.println(inter->vehicleCount);
            } else {
                inter->trafficWaiting = true;
                Serial.print(" - vehicle waiting (total: ");
                Serial.print(inter->vehicleCount);
                Serial.println(")");
            }
        }
    }

    if (!pressed) inter->buttonHeld = false;
}

void printStatus() {
    const char* stateNames[] = { "RED", "YELLOW", "GREEN" };

    Serial.println("\n===========================================");
    Serial.println("          TRAFFIC SYSTEM STATUS");
    Serial.println("===========================================");

    for (int i = 0; i < NUM_INTERSECTIONS; i++) {
        Intersection* inter = &intersections[i];
        unsigned long elapsed   = millis() - inter->stateStartTime;
        unsigned long remaining = (inter->stateDuration > elapsed)
                                  ? (inter->stateDuration - elapsed) / 1000 : 0;

        Serial.print("Intersection "); Serial.print(i);
        Serial.print(" ("); Serial.print(inter->name); Serial.println("):");
        Serial.print("  Light State    : "); Serial.println(stateNames[inter->state]);
        Serial.print("  Time Remaining : "); Serial.print(remaining); Serial.println("s");
        Serial.print("  Vehicles Seen  : "); Serial.println(inter->vehicleCount);
        Serial.print("  Traffic Waiting: "); Serial.println(inter->trafficWaiting ? "YES" : "No");
        Serial.println("-------------------------------------------");
    }
}

void showMenu() {
    Serial.println("\n===========================================");
    Serial.println("  SERIAL MENU:");
    Serial.println("  m  - Show this menu");
    Serial.println("  s  - Show system status");
    Serial.println("  0  - Manual override: force Intersection 0 GREEN");
    Serial.println("  1  - Manual override: force Intersection 1 GREEN");
    Serial.println("  r  - Reset all vehicle counts");
    Serial.println("===========================================");
}

void handleSerial() {
    char cmd = Serial.read();
    while (Serial.available()) Serial.read();

    switch (cmd) {
        case 'm': case 'M': showMenu();      break;
        case 's': case 'S': printStatus();   break;

        case '0':
            Serial.println("[OVERRIDE] Forcing Intersection 0 to GREEN");
            activeIntersection = 0;
            setLightState(&intersections[0], STATE_GREEN, GREEN_TIME_DEFAULT);
            setLightState(&intersections[1], STATE_RED,   9999999UL);
            break;

        case '1':
            Serial.println("[OVERRIDE] Forcing Intersection 1 to GREEN");
            activeIntersection = 1;
            setLightState(&intersections[1], STATE_GREEN, GREEN_TIME_DEFAULT);
            setLightState(&intersections[0], STATE_RED,   9999999UL);
            break;

        case 'r': case 'R':
            for (int i = 0; i < NUM_INTERSECTIONS; i++) {
                intersections[i].vehicleCount   = 0;
                intersections[i].trafficWaiting = false;
            }
            Serial.println("[RESET] All vehicle counts cleared.");
            break;

        case '\n': case '\r': case ' ': break;

        default:
            Serial.print("[ERROR] Unknown command '");
            Serial.print(cmd);
            Serial.println("'. Type 'm' for the menu.");
    }
}
