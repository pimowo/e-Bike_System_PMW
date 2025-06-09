#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// Odkomentuj, aby włączyć debugowanie
#define DEBUG

class LightManager {
public:
    // Stałe dla konfiguracji świateł
    static const uint8_t NONE = 0;
    static const uint8_t FRONT = 1;
    static const uint8_t DRL = 2;
    static const uint8_t REAR = 4;
    
    enum ControlMode {
        SMART_CONTROL,    // Sterowanie przez ESP32 (Smart)
        CONTROLLER_CONTROL // Sterowanie przez sterownik KT
    };

    // Tryby działania
    enum LightMode {
        OFF = 0,  // Światła wyłączone
        DAY = 1,  // Tryb dzienny
        NIGHT = 2 // Tryb nocny
    };
    
    // Konstruktor i inicjalizacja
    LightManager();
    LightManager(uint8_t frontPin, uint8_t drlPin, uint8_t rearPin); // Nowy konstruktor z parametrami
    
    // Inicjalizacja - przypisanie pinów GPIO
    void begin(uint8_t frontPin, uint8_t drlPin, uint8_t rearPin);
    
    // Ustawianie trybu
    void setMode(LightMode mode);

    //
    void cycleMode(); // Przełącza między trybami OFF -> DAY -> NIGHT -> OFF
    
    // Gettery
    uint8_t getDayConfig() const { return dayConfig; }
    uint8_t getNightConfig() const { return nightConfig; }
    bool getDayBlink() const { return dayBlink; }
    bool getNightBlink() const { return nightBlink; }
    uint16_t getBlinkFrequency() const { return blinkFrequency; }
    LightMode getMode() const { return currentMode; }  // Metoda do pobrania aktualnego trybu
    
    // Settery
    void setDayConfig(uint8_t config, bool blink);
    void setNightConfig(uint8_t config, bool blink);
    void setBlinkFrequency(uint16_t frequency);
    
    // Aktywacja/Deaktywacja trybu konfiguracji
    void activateConfigMode();
    void deactivateConfigMode();
    
    // Przetworzyć stan - wywołaj w pętli loop
    void update();
    
    // Zapisz i wczytaj konfigurację
    bool saveConfig();
    bool loadConfig();
    
    // Konwersja między uint8_t i string
    String getConfigString(uint8_t config) const;
    static uint8_t parseConfigString(const char* configStr);

    String getModeString() const; // Zwraca nazwę aktualnego trybu jako string
    
    // gettery i settery
    ControlMode getControlMode() const {
        return controlMode;
    }
    
    void setControlMode(ControlMode mode) {
        controlMode = mode;
    }
    
    const char* getControlModeString() const {
        switch (controlMode) {
            case SMART_CONTROL: return "Smart";
            case CONTROLLER_CONTROL: return "Sterownik";
            default: return "Unknown";
        }
    }

private:
    // Piny GPIO
    uint8_t frontPin;
    uint8_t drlPin;
    uint8_t rearPin;
    
    // Stan świateł
    bool frontState;
    bool drlState;
    bool rearState;
    
    // Domyślnie sterowanie Smart
    ControlMode controlMode = SMART_CONTROL; 

    // Tryb działania
    LightMode currentMode;
    
    // Konfiguracja dla różnych trybów
    uint8_t dayConfig;   // Kombinacja flag dla trybu dziennego
    uint8_t nightConfig; // Kombinacja flag dla trybu nocnego
    bool dayBlink;       // Czy tylne światło ma migać w trybie dziennym
    bool nightBlink;     // Czy tylne światło ma migać w trybie nocnym
    uint16_t blinkFrequency; // Częstotliwość migania w ms
    
    // Zmienne do migania
    bool blinkState;
    unsigned long lastBlinkTime;
    bool configMode; // Tryb konfiguracji - wszystkie światła włączone
    
    // Aktualizacja stanu świateł
    void updateLights();
};

#endif // LIGHT_MANAGER_H
