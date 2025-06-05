#include "LightManager.h"

// Definicje stałych
const uint8_t LightManager::NONE;
const uint8_t LightManager::FRONT;
const uint8_t LightManager::DRL;
const uint8_t LightManager::REAR;

// Konstruktor
LightManager::LightManager() :
    frontPin(0),
    drlPin(0),
    rearPin(0),
    frontState(false),
    drlState(false),
    rearState(false),
    currentMode(OFF),
    dayConfig(NONE),
    nightConfig(NONE),
    dayBlink(false),
    nightBlink(false),
    blinkFrequency(500),
    blinkState(false),
    lastBlinkTime(0),
    configMode(false)
{
}

// Dodaj po istniejącym konstruktorze
LightManager::LightManager(uint8_t frontPin, uint8_t drlPin, uint8_t rearPin) :
    frontPin(frontPin),
    drlPin(drlPin),
    rearPin(rearPin),
    frontState(false),
    drlState(false),
    rearState(false),
    currentMode(OFF),
    dayConfig(NONE),
    nightConfig(NONE),
    dayBlink(false),
    nightBlink(false),
    blinkFrequency(500),
    blinkState(false),
    lastBlinkTime(0),
    configMode(false)
{
    // Ustaw piny jako wyjścia
    pinMode(frontPin, OUTPUT);
    pinMode(drlPin, OUTPUT);
    pinMode(rearPin, OUTPUT);
    
    // Wyłącz wszystkie światła na start
    digitalWrite(frontPin, LOW);
    digitalWrite(drlPin, LOW);
    digitalWrite(rearPin, LOW);
    
    // Wczytaj konfigurację
    if (!loadConfig()) {
        // Domyślna konfiguracja jeśli nie udało się wczytać
        dayConfig = DRL | REAR;
        nightConfig = FRONT | REAR;
        dayBlink = true;
        nightBlink = false;
        blinkFrequency = 500;
        saveConfig();  // Zapisz domyślną konfigurację
    }
    
    #ifdef DEBUG
    Serial.println("[LightManager] Initialized with pins");
    Serial.printf("[LightManager] Day config: %s, blink: %d\n", getConfigString(dayConfig).c_str(), dayBlink);
    Serial.printf("[LightManager] Night config: %s, blink: %d\n", getConfigString(nightConfig).c_str(), nightBlink);
    #endif
}

// Inicjalizacja - przypisanie pinów GPIO
void LightManager::begin(uint8_t frontPin, uint8_t drlPin, uint8_t rearPin) {
    this->frontPin = frontPin;
    this->drlPin = drlPin;
    this->rearPin = rearPin;
    
    // Ustaw piny jako wyjścia
    pinMode(frontPin, OUTPUT);
    pinMode(drlPin, OUTPUT);
    pinMode(rearPin, OUTPUT);
    
    // Wyłącz wszystkie światła na start
    digitalWrite(frontPin, LOW);
    digitalWrite(drlPin, LOW);
    digitalWrite(rearPin, LOW);
    
    // Wczytaj konfigurację
    if (!loadConfig()) {
        // Domyślna konfiguracja jeśli nie udało się wczytać
        dayConfig = DRL | REAR;
        nightConfig = FRONT | REAR;
        dayBlink = true;
        nightBlink = false;
        blinkFrequency = 500;
        saveConfig();  // Zapisz domyślną konfigurację
    }
    
    #ifdef DEBUG
    Serial.println("[LightManager] Initialized");
    Serial.printf("[LightManager] Day config: %s, blink: %d\n", getConfigString(dayConfig).c_str(), dayBlink);
    Serial.printf("[LightManager] Night config: %s, blink: %d\n", getConfigString(nightConfig).c_str(), nightBlink);
    #endif
}

// Ustawianie trybu
void LightManager::setMode(LightMode mode) {
    #ifdef DEBUG
    Serial.printf("[LightManager] Setting mode %d\n", mode);
    #endif
    
    if (mode >= OFF && mode <= NIGHT) {
        currentMode = mode;
        updateLights();
    }
}

// Settery
void LightManager::setDayConfig(uint8_t config, bool blink) {
    dayConfig = config;
    dayBlink = blink;
    
    #ifdef DEBUG
    Serial.printf("[LightManager] Set day config: %s, blink: %d\n", getConfigString(dayConfig).c_str(), dayBlink);
    #endif
    
    if (currentMode == DAY) {
        updateLights();
    }
}

void LightManager::setNightConfig(uint8_t config, bool blink) {
    nightConfig = config;
    nightBlink = blink;
    
    #ifdef DEBUG
    Serial.printf("[LightManager] Set night config: %s, blink: %d\n", getConfigString(nightConfig).c_str(), nightBlink);
    #endif
    
    if (currentMode == NIGHT) {
        updateLights();
    }
}

void LightManager::setBlinkFrequency(uint16_t frequency) {
    if (frequency >= 100 && frequency <= 2000) {
        blinkFrequency = frequency;
        
        #ifdef DEBUG
        Serial.printf("[LightManager] Set blink frequency: %d ms\n", blinkFrequency);
        #endif
    }
}

// Przełączanie trybów
void LightManager::cycleMode() {
    #ifdef DEBUG
    Serial.println("[LightManager] Cycling mode");
    #endif
    
    switch (currentMode) {
        case OFF:
            setMode(DAY);
            break;
        case DAY:
            setMode(NIGHT);
            break;
        case NIGHT:
        default:
            setMode(OFF);
            break;
    }
}

// Aktywacja/Deaktywacja trybu konfiguracji
void LightManager::activateConfigMode() {
    configMode = true;
    
    #ifdef DEBUG
    Serial.println("[LightManager] Config mode activated");
    #endif
    
    // Włącz wszystkie światła
    digitalWrite(frontPin, HIGH);
    digitalWrite(drlPin, HIGH);
    digitalWrite(rearPin, HIGH);
    frontState = true;
    drlState = true;
    rearState = true;
}

void LightManager::deactivateConfigMode() {
    configMode = false;
    
    #ifdef DEBUG
    Serial.println("[LightManager] Config mode deactivated");
    #endif
    
    // Wróć do normalnego stanu
    updateLights();
}

// Aktualizacja stanu świateł
void LightManager::updateLights() {
    if (configMode) return; // W trybie konfiguracji nie aktualizujemy świateł
    
    uint8_t currentConfig = NONE;
    bool shouldBlink = false;
    
    // Wybierz konfigurację w zależności od trybu
    if (currentMode == DAY) {
        currentConfig = dayConfig;
        shouldBlink = dayBlink;
    } else if (currentMode == NIGHT) {
        currentConfig = nightConfig;
        shouldBlink = nightBlink;
    } else {
        // W trybie OFF wyłącz wszystko
        frontState = false;
        drlState = false;
        rearState = false;
    }
    
    // Ustaw stany świateł na podstawie konfiguracji
    if (currentMode != OFF) {
        frontState = (currentConfig & FRONT) != 0;
        drlState = (currentConfig & DRL) != 0;
        rearState = (currentConfig & REAR) != 0;
    }
    
    // Aktualizuj piny
    digitalWrite(frontPin, frontState ? HIGH : LOW);
    digitalWrite(drlPin, drlState ? HIGH : LOW);
    
    // Dla tylnego światła, jeśli ma migać, stan będzie aktualizowany w update()
    if (!shouldBlink) {
        digitalWrite(rearPin, rearState ? HIGH : LOW);
    }
    
    #ifdef DEBUG
    if (currentMode != OFF) {
        Serial.printf("[LightManager] Lights updated: front=%d, drl=%d, rear=%d, blink=%d\n", 
            frontState, drlState, rearState, shouldBlink);
    } else {
        Serial.println("[LightManager] All lights turned off");
    }
    #endif
}

// Przetworzyć stan - wywołaj w pętli loop
void LightManager::update() {
    if (configMode) return; // W trybie konfiguracji nie przetwarzamy migania
    
    bool shouldBlink = false;
    
    // Sprawdź, czy powinniśmy migać w zależności od trybu
    if (currentMode == DAY) {
        shouldBlink = dayBlink;
    } else if (currentMode == NIGHT) {
        shouldBlink = nightBlink;
    }
    
    // Jeśli powinniśmy migać i tylne światło jest włączone
    if (shouldBlink && rearState) {
        unsigned long currentTime = millis();
        
        // Czas na zmianę stanu migania
        if (currentTime - lastBlinkTime >= blinkFrequency) {
            lastBlinkTime = currentTime;
            blinkState = !blinkState;
            digitalWrite(rearPin, blinkState ? HIGH : LOW);
            
            #ifdef DEBUG
            Serial.printf("[LightManager] Rear light blink: %d\n", blinkState);
            #endif
        }
    } else if (rearState) {
        // Jeśli nie migamy, ale światło ma być włączone
        digitalWrite(rearPin, HIGH);
    } else {
        // Światło wyłączone
        digitalWrite(rearPin, LOW);
    }
}

// Zapisz konfigurację
bool LightManager::saveConfig() {
    #ifdef DEBUG
    Serial.println("[LightManager] Saving config...");
    #endif
    
    if (!LittleFS.begin(false)) {
        #ifdef DEBUG
        Serial.println("[LightManager] Error mounting LittleFS");
        #endif
        return false;
    }
    
    File configFile = LittleFS.open("/lights.json", "w");
    if (!configFile) {
        #ifdef DEBUG
        Serial.println("[LightManager] Failed to open config file for writing");
        #endif
        return false;
    }
    
    StaticJsonDocument<256> doc;
    doc["dayConfig"] = dayConfig;
    doc["nightConfig"] = nightConfig;
    doc["dayBlink"] = dayBlink;
    doc["nightBlink"] = nightBlink;
    doc["blinkFrequency"] = blinkFrequency;
    
    if (serializeJson(doc, configFile) == 0) {
        #ifdef DEBUG
        Serial.println("[LightManager] Failed to write config to file");
        #endif
        configFile.close();
        return false;
    }
    
    configFile.close();
    
    #ifdef DEBUG
    Serial.println("[LightManager] Config saved successfully");
    #endif
    
    return true;
}

// Wczytaj konfigurację
bool LightManager::loadConfig() {
    #ifdef DEBUG
    Serial.println("[LightManager] Loading config...");
    #endif
    
    if (!LittleFS.begin(false)) {
        #ifdef DEBUG
        Serial.println("[LightManager] Error mounting LittleFS");
        #endif
        return false;
    }
    
    if (!LittleFS.exists("/lights.json")) {
        #ifdef DEBUG
        Serial.println("[LightManager] Config file not found");
        #endif
        return false;
    }
    
    File configFile = LittleFS.open("/lights.json", "r");
    if (!configFile) {
        #ifdef DEBUG
        Serial.println("[LightManager] Failed to open config file for reading");
        #endif
        return false;
    }
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        #ifdef DEBUG
        Serial.printf("[LightManager] Failed to parse config file: %s\n", error.c_str());
        #endif
        return false;
    }
    
    // Wczytaj konfigurację
    dayConfig = doc["dayConfig"] | (DRL | REAR);
    nightConfig = doc["nightConfig"] | (FRONT | REAR);
    dayBlink = doc["dayBlink"] | true;
    nightBlink = doc["nightBlink"] | false;
    blinkFrequency = doc["blinkFrequency"] | 500;
    
    #ifdef DEBUG
    Serial.println("[LightManager] Config loaded successfully");
    Serial.printf("[LightManager] Day config: %s, blink: %d\n", getConfigString(dayConfig).c_str(), dayBlink);
    Serial.printf("[LightManager] Night config: %s, blink: %d\n", getConfigString(nightConfig).c_str(), nightBlink);
    #endif
    
    return true;
}

// Konwersja config (uint8_t) na string
String LightManager::getConfigString(uint8_t config) const {
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

// Konwersja trybu na string
String LightManager::getModeString() const {
    switch (currentMode) {
        case OFF:
            return "OFF";
        case DAY:
            return "DAY";
        case NIGHT:
            return "NIGHT";
        default:
            return "UNKNOWN";
    }
}

// Konwersja string na config (uint8_t)
uint8_t LightManager::parseConfigString(const char* configStr) {
    String config(configStr);
    uint8_t result = NONE;
    
    if (config == "NONE") return NONE;
    
    if (config.indexOf("FRONT") >= 0) result |= FRONT;
    if (config.indexOf("DRL") >= 0) result |= DRL;
    if (config.indexOf("REAR") >= 0) result |= REAR;
    
    return result;
}
