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
    // Ustaw piny jako wyjscia
    pinMode(frontPin, OUTPUT);
    pinMode(drlPin, OUTPUT);
    pinMode(rearPin, OUTPUT);
    
    // Wylacz wszystkie swiatla na start
    digitalWrite(frontPin, LOW);
    digitalWrite(drlPin, LOW);
    digitalWrite(rearPin, LOW);
    
    // Wczytaj konfiguracje
    if (!loadConfig()) {
        // Domyslna konfiguracja jesli nie udalo sie wczytac
        dayConfig = DRL | REAR;
        nightConfig = FRONT | REAR;
        dayBlink = true;
        nightBlink = false;
        blinkFrequency = 500;
        saveConfig();  // Zapisz domyslna konfiguracje
    }
    
    DEBUG_LIGHT("Zainicjalizowano z pinami");
    DEBUG_LIGHT("Konfiguracja dzienna: %s, miganie: %d", getConfigString(dayConfig).c_str(), dayBlink);
    DEBUG_LIGHT("Konfiguracja nocna: %s, miganie: %d", getConfigString(nightConfig).c_str(), nightBlink);
}

// Inicjalizacja - przypisanie pinow GPIO
void LightManager::begin(uint8_t frontPin, uint8_t drlPin, uint8_t rearPin) {
    this->frontPin = frontPin;
    this->drlPin = drlPin;
    this->rearPin = rearPin;
    
    // Ustaw piny jako wyjscia
    pinMode(frontPin, OUTPUT);
    pinMode(drlPin, OUTPUT);
    pinMode(rearPin, OUTPUT);
    
    // Wylacz wszystkie swiatla na start
    digitalWrite(frontPin, LOW);
    digitalWrite(drlPin, LOW);
    digitalWrite(rearPin, LOW);
    
    // Wczytaj konfiguracje
    if (!loadConfig()) {
        // Domyslna konfiguracja jesli nie udalo sie wczytac
        dayConfig = DRL | REAR;
        nightConfig = FRONT | REAR;
        dayBlink = true;
        nightBlink = false;
        blinkFrequency = 500;
        saveConfig();  // Zapisz domyslna konfiguracje
    }
    
    #ifdef DEBUG
    DEBUG_LIGHT("Zainicjalizowano");
    DEBUG_LIGHT("Konfiguracja dzienna: %s, miganie: %d", getConfigString(dayConfig).c_str(), dayBlink);
    DEBUG_LIGHT("Konfiguracja nocna: %s, miganie: %d", getConfigString(nightConfig).c_str(), nightBlink);
    #endif
}

// Ustawianie trybu
void LightManager::setMode(LightMode mode) {
    DEBUG_LIGHT("Ustawianie trybu %d", mode);
    
    if (mode >= OFF && mode <= NIGHT) {
        currentMode = mode;
        updateLights();
    }
}

// Settery
void LightManager::setDayConfig(uint8_t config, bool blink) {
    DEBUG_LIGHT("Ustawianie konfiguracji dziennej: przed=0x%02X, po=0x%02X, miganie: %d", dayConfig, config, blink);
    DEBUG_LIGHT("REAR wlaczone? %s", (config & REAR) ? "TAK" : "NIE");
    
    dayConfig = config;
    dayBlink = blink;
    
    DEBUG_LIGHT("Ustawiono konfiguracje dzienna: %s, miganie: %d", getConfigString(dayConfig).c_str(), dayBlink);
    
    if (currentMode == DAY) {
        updateLights();
    }
}

void LightManager::setNightConfig(uint8_t config, bool blink) {
    DEBUG_LIGHT("Ustawianie konfiguracji nocnej: przed=0x%02X, po=0x%02X, miganie: %d", nightConfig, config, blink);
    DEBUG_LIGHT("REAR wlaczone? %s", (config & REAR) ? "TAK" : "NIE");
        
    nightConfig = config;
    nightBlink = blink;
    
    DEBUG_LIGHT("Ustawiono konfiguracje nocna: %s, miganie: %d", getConfigString(nightConfig).c_str(), nightBlink);
    
    if (currentMode == NIGHT) {
        updateLights();
    }
}

void LightManager::setBlinkFrequency(uint16_t frequency) {
    if (frequency >= 100 && frequency <= 2000) {
        blinkFrequency = frequency;
        
        DEBUG_LIGHT("Ustawiono czestotliwosc migania: %d ms", blinkFrequency);
    }
}

// Przelaczanie trybow
void LightManager::cycleMode() {
    DEBUG_LIGHT("Przelaczanie trybu");
    
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
    
    DEBUG_LIGHT("Tryb konfiguracji aktywowany");
    
    // Wlacz wszystkie swiatla
    digitalWrite(frontPin, HIGH);
    digitalWrite(drlPin, HIGH);
    digitalWrite(rearPin, HIGH);
    frontState = true;
    drlState = true;
    rearState = true;
}

void LightManager::deactivateConfigMode() {
    configMode = false;
    
    DEBUG_LIGHT("Tryb konfiguracji dezaktywowany");
        
    // Wroc do normalnego stanu
    updateLights();
}

// Aktualizacja stanu swiatel
void LightManager::updateLights() {
    if (configMode) return; // W trybie konfiguracji nie aktualizujemy swiatel
    
    uint8_t currentConfig = NONE;
    bool shouldBlink = false;
    
    // Wybierz konfiguracje w zaleznosci od trybu
    if (currentMode == DAY) {
        currentConfig = dayConfig;
        shouldBlink = dayBlink;
    } else if (currentMode == NIGHT) {
        currentConfig = nightConfig;
        shouldBlink = nightBlink;
    } else {
        // W trybie OFF wylacz wszystko
        frontState = false;
        drlState = false;
        rearState = false;
    }
    
    // Ustaw stany swiatel na podstawie konfiguracji
    if (currentMode != OFF) {
        frontState = (currentConfig & FRONT) != 0;
        drlState = (currentConfig & DRL) != 0;
        rearState = (currentConfig & REAR) != 0;
    }
    
    // Aktualizuj piny
    digitalWrite(frontPin, frontState ? HIGH : LOW);
    digitalWrite(drlPin, drlState ? HIGH : LOW);
    //digitalWrite(rearPin, rearState ? HIGH : LOW);
    
    // Dla tylnego swiatla, jesli ma migac, stan bedzie aktualizowany w update()
    if (!shouldBlink) {
        digitalWrite(rearPin, rearState ? HIGH : LOW);
    }
    
    if (currentMode != OFF) {
        DEBUG_LIGHT("Swiatla zaktualizowane: przednie=%d, swiatlo dzienne=%d, tylne=%d, miganie=%d", frontState, drlState, rearState, shouldBlink);
    } else {
        DEBUG_LIGHT("Wszystkie swiatla wylaczone");
    }
}

// Przetworzyc stan - wywolaj w petli loop
void LightManager::update() {
    if (configMode) return; // W trybie konfiguracji nie przetwarzamy migania
    
    bool shouldBlink = false;
    
    // Sprawdz, czy powinnismy migac w zaleznosci od trybu
    if (currentMode == DAY) {
        shouldBlink = dayBlink;
    } else if (currentMode == NIGHT) {
        shouldBlink = nightBlink;
    }
    
    // Jesli powinnismy migac i tylne swiatlo jest wlaczone
    if (shouldBlink && rearState) {
        unsigned long currentTime = millis();
        
        // Czas na zmiane stanu migania
        if (currentTime - lastBlinkTime >= blinkFrequency) {
            lastBlinkTime = currentTime;
            blinkState = !blinkState;
            digitalWrite(rearPin, blinkState ? HIGH : LOW);
            
            //DEBUG_LIGHT("Miganie tylnego swiatla: %d", blinkState);
        }
    } else if (rearState) {
        // Jesli nie migamy, ale swiatlo ma byc wlaczone
        digitalWrite(rearPin, HIGH);
    } else {
        // Swiatlo wylaczone
        digitalWrite(rearPin, LOW);
    }
}

// Zapisz konfiguracje
bool LightManager::saveConfig() {
    DEBUG_LIGHT("Zapisywanie konfiguracji...");
    DEBUG_LIGHT("Konfiguracja dzienna: 0x%02X (%s)", dayConfig, getConfigString(dayConfig).c_str());
    DEBUG_LIGHT("Konfiguracja nocna: 0x%02X (%s)", nightConfig, getConfigString(nightConfig).c_str());
    DEBUG_LIGHT("Miganie dzienne: %d, Miganie nocne: %d", dayBlink, nightBlink);
    DEBUG_LIGHT("Czestotliwosc migania: %d", blinkFrequency);
    
    // Sprawdz poszczegolne bity
    DEBUG_LIGHT("Konfiguracja dzienna - FRONT: %d, DRL: %d, REAR: %d", (dayConfig & FRONT) != 0, (dayConfig & DRL) != 0, (dayConfig & REAR) != 0);
    DEBUG_LIGHT("Konfiguracja nocna - FRONT: %d, DRL: %d, REAR: %d", (nightConfig & FRONT) != 0, (nightConfig & DRL) != 0, (nightConfig & REAR) != 0);
        
    if (!LittleFS.begin(false)) {
        DEBUG_LIGHT("Blad montowania LittleFS");
        return false;
    }
    
    // Dla bezpieczenstwa usun istniejacy plik
    if (LittleFS.exists("/light_config.json")) {
        if (!LittleFS.remove("/light_config.json")) {
            DEBUG_LIGHT("Nie udalo sie usunac istniejacego pliku konfiguracyjnego");
            // Kontynuujemy mimo to
        }
    }
    
    // Otworz plik do zapisu
    File configFile = LittleFS.open("/light_config.json", "w");
    if (!configFile) {
        DEBUG_LIGHT("Nie mozna otworzyc pliku konfiguracyjnego do zapisu");
        return false;
    }
    
    // Przygotuj dokument JSON
    StaticJsonDocument<256> doc;
    doc["dayConfig"] = dayConfig;
    doc["nightConfig"] = nightConfig;
    doc["dayBlink"] = dayBlink;
    doc["nightBlink"] = nightBlink;
    doc["blinkFrequency"] = blinkFrequency;
    
    // Serializuj do pliku
    size_t bytes = serializeJson(doc, configFile);
    configFile.close();
    
    if (bytes == 0) {
        DEBUG_LIGHT("Nie udalo sie zapisac konfiguracji do pliku");
        return false;
    }
    
    #ifdef DEBUG_LIGHT_ENABLED
    DEBUG_LIGHT("Konfiguracja zapisana pomyslnie (%d bajtow)", bytes);
    
    // Dla pewnosci odczytaj zapisany plik i sprawdz jego zawartosc
    File readFile = LittleFS.open("/light_config.json", "r");
    if (readFile) {
        String content = readFile.readString();
        readFile.close();
        DEBUG_LIGHT("Zapisana zawartosc pliku: %s", content.c_str());
    }
    #endif
    
    return true;
}

// Wczytaj konfiguracje
bool LightManager::loadConfig() {
    DEBUG_LIGHT("Wczytywanie konfiguracji...");
        
    if (!LittleFS.begin(false)) {
        DEBUG_LIGHT("Blad montowania LittleFS");
        return false;
    }
    
    if (!LittleFS.exists("/light_config.json")) {
        DEBUG_LIGHT("Nie znaleziono pliku konfiguracyjnego");
        return false;
    }
    
    File configFile = LittleFS.open("/light_config.json", "r");
    if (!configFile) {
        DEBUG_LIGHT("Nie mozna otworzyc pliku konfiguracyjnego do odczytu");
        return false;
    }
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        DEBUG_LIGHT("Blad parsowania pliku konfiguracyjnego: %s", error.c_str());
        return false;
    }
    
    // UWAGA: Zmienione uzycie operatora | na sprawdzenie, czy wartosc istnieje
    // Wczytaj konfiguracje tylko jesli istnieje, w przeciwnym razie uzyj wartosci domyslnych
    if (doc.containsKey("dayConfig")) {
        dayConfig = doc["dayConfig"];
    } else {
        dayConfig = DRL | REAR; // Wartosc domyslna
    }
    
    if (doc.containsKey("nightConfig")) {
        nightConfig = doc["nightConfig"];
    } else {
        nightConfig = FRONT | REAR; // Wartosc domyslna
    }
    
    if (doc.containsKey("dayBlink")) {
        dayBlink = doc["dayBlink"];
    } else {
        dayBlink = true; // Wartosc domyslna
    }
    
    if (doc.containsKey("nightBlink")) {
        nightBlink = doc["nightBlink"];
    } else {
        nightBlink = false; // Wartosc domyslna
    }
    
    if (doc.containsKey("blinkFrequency")) {
        blinkFrequency = doc["blinkFrequency"];
    } else {
        blinkFrequency = 500; // Wartosc domyslna
    }
    
    #ifdef DEBUG_LIGHT_ENABLED
    DEBUG_LIGHT("Wczytana konfiguracja:");
    DEBUG_LIGHT("Konfiguracja dzienna: 0x%02X (%s)", dayConfig, getConfigString(dayConfig).c_str());
    DEBUG_LIGHT("Konfiguracja nocna: 0x%02X (%s)", nightConfig, getConfigString(nightConfig).c_str());
    DEBUG_LIGHT("Miganie dzienne: %d, Miganie nocne: %d", dayBlink, nightBlink);
    DEBUG_LIGHT("Czestotliwosc migania: %d", blinkFrequency);
    #endif
    
    return true;
}

// Konwersja config (uint8_t) na string
String LightManager::getConfigString(uint8_t config) const {
    DEBUG_LIGHT("getConfigString - wartsc wejsciowa: 0x%02X", config);
    
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
    
    DEBUG_LIGHT("getConfigString - rezultat: %s", result.c_str());
        
    return result;
}

// Konwersja trybu na string
String LightManager::getModeString() const {
    switch (currentMode) {
        case OFF:
            return "WYLACZONE";
        case DAY:
            return "DZIEN";
        case NIGHT:
            return "NOC";
        default:
            return "NIEZNANY";
    }
}

// Konwersja string na config (uint8_t)
uint8_t LightManager::parseConfigString(const char* configStr) {
    DEBUG_LIGHT("parseConfigString - wejsciowy string: '%s'", configStr);
        
    String configString(configStr); // Konwertujemy const char* na String dla wygody
    uint8_t result = NONE;
    
    if (strcmp(configStr, "NONE") == 0) {
        DEBUG_LIGHT("Wykryto konfiguracje NONE (0x00)");
        return NONE;
    }
    
    // Sprawdz czy string zawiera '+'
    if (strchr(configStr, '+') != NULL) {
        // Podziel string na czesci
        char* copy = strdup(configStr); // Stworz kopie, bo strtok modyfikuje string
        char* token = strtok(copy, "+");
        
        while (token != NULL) {
            // Usun biale znaki
            while (*token == ' ') token++;
            char* end = token + strlen(token) - 1;
            while (end > token && *end == ' ') end--;
            *(end + 1) = '\0';
            
            DEBUG_LIGHT("Analizuje token: '%s'", token);
                        
            if (strcmp(token, "FRONT") == 0) {
                DEBUG_LIGHT("Dodaje flage FRONT (0x01)");
                result |= FRONT;
            }
            else if (strcmp(token, "DRL") == 0) {
                DEBUG_LIGHT("Dodaje flage DRL (0x02)");
                result |= DRL;
            }
            else if (strcmp(token, "REAR") == 0) {
                DEBUG_LIGHT("Dodaje flage REAR (0x04)");
                result |= REAR;
            }
            
            token = strtok(NULL, "+");
        }
        
        free(copy); // Zwolnij pamiec
    } else {
        // Pojedyncza wartosc
        DEBUG_LIGHT("Analizuje pojedyncza wartosc: '%s'", configStr);
                
        if (strcmp(configStr, "FRONT") == 0) {
            DEBUG_LIGHT("Ustawiam flage FRONT (0x01)");
            result |= FRONT;
        }
        else if (strcmp(configStr, "DRL") == 0) {
            DEBUG_LIGHT("Ustawiam flage DRL (0x02)");
            result |= DRL;
        }
        else if (strcmp(configStr, "REAR") == 0) {
            DEBUG_LIGHT("Ustawiam flage REAR (0x04)");
            result |= REAR;
        }
    }
    
    DEBUG_LIGHT("parseConfigString - rezultat: 0x%02X", result);
    
    return result;
}
