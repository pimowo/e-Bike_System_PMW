#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define DEBUG

class LightManager {
public:
    // Dostępne tryby
    enum LightMode {
        OFF = 0,
        DAY = 1,
        NIGHT = 2
    };
    
    // Flagi konfiguracji świateł (pozwalają na dowolne kombinacje)
    static const uint8_t NONE = 0;
    static const uint8_t FRONT = 1;
    static const uint8_t DRL = 2;
    static const uint8_t REAR = 4;
    
private:
    // Piny
    uint8_t frontPin;    // Światło przednie główne
    uint8_t drlPin;      // Światła do jazdy dziennej
    uint8_t rearPin;     // Światło tylne
    
    // Stan
    LightMode currentMode;
    
    // Konfiguracja
    uint8_t dayConfig;   // Kombinacja flag dla trybu dziennego
    uint8_t nightConfig; // Kombinacja flag dla trybu nocnego
    bool dayBlink;       // Czy tylne światło ma migać w trybie dziennym
    bool nightBlink;     // Czy tylne światło ma migać w trybie nocnym
    uint16_t blinkFrequency; // Częstotliwość migania w ms
    bool blinkState;     // Aktualny stan migania
    
    // Zmienne czasowe
    unsigned long lastBlinkTime;
    
    // Ścieżka do pliku konfiguracyjnego
    const char* configPath = "/light_config.json";
    
    // Funkcje pomocnicze
    void updateLights() {
        // Wyłącz wszystkie światła najpierw
        digitalWrite(frontPin, LOW);
        digitalWrite(drlPin, LOW);
        digitalWrite(rearPin, LOW);
        
        if (currentMode == OFF) {
            return;
        }
        
        // Sprawdź czy aktywny jest tryb migania
        bool shouldBlink = false;
        if (currentMode == DAY && dayBlink) {
            shouldBlink = true;
        } else if (currentMode == NIGHT && nightBlink) {
            shouldBlink = true;
        }
        
        // Zastosuj konfigurację dla aktualnego trybu
        uint8_t config = (currentMode == DAY) ? dayConfig : nightConfig;
        
        // Zastosuj ustawienia dla każdego światła
        if (config & FRONT) {
            digitalWrite(frontPin, HIGH);
        }
        
        if (config & DRL) {
            digitalWrite(drlPin, HIGH);
        }
        
        if (config & REAR) {
            // Jeśli miganie jest włączone, sprawdź stan migania
            if (!shouldBlink || blinkState) {
                digitalWrite(rearPin, HIGH);
            }
        }
        
        #ifdef DEBUG
        Serial.printf("[LightManager] Lights updated: Mode=%d, Config=%d, Blink=%d\n", 
                      currentMode, config, shouldBlink ? blinkState : -1);
        #endif
    }
    
public:
    // Konstruktor
    LightManager(uint8_t front, uint8_t drl, uint8_t rear) : 
        frontPin(front), 
        drlPin(drl),
        rearPin(rear),
        currentMode(OFF),
        dayConfig(REAR),      // Zmień z FRONT | REAR na REAR - tylko tylne
        nightConfig(FRONT | REAR),   // Domyślnie przód + tył (zostaw jak jest)
        dayBlink(false),
        nightBlink(false),
        blinkFrequency(500),
        blinkState(false),
        lastBlinkTime(0) {
        
        // Konfiguracja pinów
        pinMode(frontPin, OUTPUT);
        pinMode(drlPin, OUTPUT);
        pinMode(rearPin, OUTPUT);
        
        // Wyłącz światła
        digitalWrite(frontPin, LOW);
        digitalWrite(drlPin, LOW);
        digitalWrite(rearPin, LOW);
    }
    
    // Inicjalizacja
    void begin() {
        // Wczytaj konfigurację
        loadConfig();
        
        // Wymuś tryb wyłączony przy starcie
        currentMode = OFF;
        
        // Zastosuj ustawienia
        updateLights();
        
        #ifdef DEBUG
        Serial.println("[LightManager] Initialized");
        printStatus();
        #endif
    }
       
    // Zmiana trybu w sekwencji (OFF -> DAY -> NIGHT -> OFF)
    void cycleMode() {
        currentMode = static_cast<LightMode>((currentMode + 1) % 3);
        
        #ifdef DEBUG
        Serial.printf("[LightManager] Cycling mode to %d\n", currentMode);
        #endif
        
        updateLights();
    }
    
    // Pobranie aktualnego trybu
    LightMode getMode() {
        return currentMode;
    }
    
    // Konfiguracja trybów
    void setDayConfig(uint8_t config, bool blink = false) {
        dayConfig = config;
        dayBlink = blink;
        updateLights();
        saveConfig();
    }
    
    void setNightConfig(uint8_t config, bool blink = false) {
        nightConfig = config;
        nightBlink = blink;
        updateLights();
        saveConfig();
    }
    
    // Ustawienie częstotliwości migania
    void setBlinkFrequency(uint16_t freq) {
        blinkFrequency = freq;
        saveConfig();
    }
    
    // Aktualizacja stanu (wywołuj w pętli głównej)
    void update() {
        // Obsługa migania
        unsigned long currentTime = millis();
        
        // Sprawdź czy miganie jest aktywne
        bool blinkingActive = false;
        if (currentMode == DAY && dayBlink && (dayConfig & REAR)) {
            blinkingActive = true;
        } else if (currentMode == NIGHT && nightBlink && (nightConfig & REAR)) {
            blinkingActive = true;
        }
        
        if (blinkingActive && (currentTime - lastBlinkTime >= blinkFrequency)) {
            blinkState = !blinkState;
            lastBlinkTime = currentTime;
            updateLights();
        }
    }
    
    // Zapisz konfigurację
    void saveConfig() {
        // Nie resetuj LittleFS przed każdym zapisem
        if (!LittleFS.begin(false)) {
            #ifdef DEBUG
            Serial.println("[LightManager] Failed to mount filesystem for saving");
            #endif
            return;
        }
        
        File file = LittleFS.open(configPath, "w");
        if (!file) {
            #ifdef DEBUG
            Serial.println("[LightManager] Failed to open config file for writing");
            #endif
            return;
        }
        
        // Przygotuj dokument JSON
        StaticJsonDocument<256> doc;
        doc["dayConfig"] = dayConfig;
        doc["nightConfig"] = nightConfig;
        doc["dayBlink"] = dayBlink;
        doc["nightBlink"] = nightBlink;
        doc["blinkFrequency"] = blinkFrequency;
        
        // Zapisz do pliku
        if (serializeJson(doc, file) == 0) {
            #ifdef DEBUG
            Serial.println("[LightManager] Failed to write config file");
            #endif
        }
        
        // Zamknij plik i wyloguj sukces
        file.close();
        
        #ifdef DEBUG
        Serial.println("[LightManager] Configuration saved");
        
        // Weryfikacja zapisu
        File checkFile = LittleFS.open(configPath, "r");
        if (checkFile) {
            String content = checkFile.readString();
            Serial.println("[LightManager] Saved file content: " + content);
            checkFile.close();
        }
        #endif
    }
    
    // Wczytaj konfigurację
    void loadConfig() {
        if (!LittleFS.begin(false)) {
            #ifdef DEBUG
            Serial.println("[LightManager] Failed to mount filesystem for loading");
            #endif
            return;
        }
        
        if (!LittleFS.exists(configPath)) {
            #ifdef DEBUG
            Serial.println("[LightManager] Config file doesn't exist, using defaults");
            #endif
            saveConfig();
            return;
        }
        
        File file = LittleFS.open(configPath, "r");
        if (!file) {
            #ifdef DEBUG
            Serial.println("[LightManager] Failed to open config file for reading");
            #endif
            return;
        }
        
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        
        if (error) {
            #ifdef DEBUG
            Serial.println("[LightManager] Failed to parse config file");
            #endif
            return;
        }
        
        // Wczytaj konfigurację
        dayConfig = doc["dayConfig"] | REAR;  // Zmień domyślną wartość na REAR
        nightConfig = doc["nightConfig"] | (FRONT | REAR);  // Domyślnie przód + tył
        dayBlink = doc["dayBlink"] | false;
        nightBlink = doc["nightBlink"] | false;
        blinkFrequency = doc["blinkFrequency"] | 500;
        
        // currentMode zawsze ustawiamy na OFF przy starcie
        currentMode = OFF;  // Odkomentuj lub dodaj tę linię
        
        #ifdef DEBUG
        Serial.println("[LightManager] Configuration loaded");
        #endif
    }
    
    // Wyświetl status (debug)
    void printStatus() {
        #ifdef DEBUG
        Serial.println("[LightManager] Status:");
        Serial.printf("  Mode: %d\n", currentMode);
        Serial.printf("  Day config: %d (blink: %d)\n", dayConfig, dayBlink);
        Serial.printf("  Night config: %d (blink: %d)\n", nightConfig, nightBlink);
        Serial.printf("  Blink frequency: %d ms\n", blinkFrequency);
        #endif
    }

    // Pobranie konfiguracji jako string dla interfejsu
    const char* getModeString() {
        switch (currentMode) {
            case DAY: return "DAY";
            case NIGHT: return "NIGHT";
            default: return "OFF";
        }
    }
    
    // Konwersja konfiguracji na czytelny string dla interfejsu webowego
    String getConfigString(uint8_t config) {
        if (config == NONE) return "NONE";
        
        String result = "";
        if (config & FRONT) {
            result += "FRONT";
        }
        if (config & DRL) {
            if (result.length() > 0) result += "+";
            result += "DRL";
        }
        if (config & REAR) {
            if (result.length() > 0) result += "+";
            result += "REAR";
        }
        
        return result;
    }
    
    // Metody pomocnicze dla interfejsu webowego
    uint8_t getDayConfig() { return dayConfig; }
    uint8_t getNightConfig() { return nightConfig; }
    bool getDayBlink() { return dayBlink; }
    bool getNightBlink() { return nightBlink; }
    uint16_t getBlinkFrequency() { return blinkFrequency; }
    
    // Parsowanie konfiguracji z stringów dla interfejsu webowego
    static uint8_t parseConfigString(const char* configStr) {
        String config(configStr);
        uint8_t result = NONE;
        
        if (config == "NONE") return NONE;
        
        if (config.indexOf("FRONT") >= 0) result |= FRONT;
        if (config.indexOf("DRL") >= 0) result |= DRL;
        if (config.indexOf("REAR") >= 0) result |= REAR;
        
        return result;
    }
};

#endif // LIGHT_MANAGER_H
