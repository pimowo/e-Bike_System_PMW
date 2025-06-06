class OdometerManager {
private:
    const char* filename = "/odometer.json";
    float currentTotal = 0;
    bool fileSystemMounted = false;

    bool ensureFilesystem() {
        if (fileSystemMounted) return true;
        
        if (!LittleFS.begin(false)) {
            #ifdef DEBUG
            Serial.println("[OdometerManager] Błąd montowania LittleFS");
            #endif
            return false;
        }
        
        fileSystemMounted = true;
        return true;
    }

    void saveToFile() {
        #ifdef DEBUG
        Serial.println("[OdometerManager] Zapisywanie licznika do pliku...");
        #endif

        // Upewnij się, że system plików jest zamontowany
        if (!ensureFilesystem()) {
            #ifdef DEBUG
            Serial.println("[OdometerManager] System plików niedostępny - nie można zapisać licznika");
            #endif
            return;
        }

        // Sprawdź czy plik licznika jest dostępny
        if (LittleFS.exists(filename)) {
            // Usuń istniejący plik, aby uniknąć problemów z zapisem
            if (!LittleFS.remove(filename)) {
                #ifdef DEBUG
                Serial.println("[OdometerManager] Nie można usunąć istniejącego pliku licznika");
                #endif
                // Kontynuujemy mimo to
            }
        }

        // Spróbuj otworzyć plik do zapisu
        File file = LittleFS.open(filename, "w");
        if (!file) {
            #ifdef DEBUG
            Serial.println("[OdometerManager] Błąd otwarcia pliku licznika do zapisu");
            
            // Sprawdź dostępność systemu plików
            if (!LittleFS.exists("/")) {
                Serial.println("[OdometerManager] Katalog główny nie istnieje - poważny problem z systemem plików");
            }
            
            Serial.printf("[OdometerManager] Wolne miejsce: %d bajtów\n", 
                       LittleFS.totalBytes() - LittleFS.usedBytes());
            #endif
            return;
        }

        // Przygotuj dane w formacie JSON i zapisz
        StaticJsonDocument<128> doc;
        doc["total"] = currentTotal;
        
        if (serializeJson(doc, file) == 0) {
            #ifdef DEBUG
            Serial.println("[OdometerManager] Błąd serializacji JSON do pliku licznika");
            #endif
        } else {
            #ifdef DEBUG
            Serial.printf("[OdometerManager] Zapisano licznik: %.2f\n", currentTotal);
            #endif
        }
        
        file.close();
    }

    void loadFromFile() {
        #ifdef DEBUG
        Serial.println("[OdometerManager] Wczytywanie licznika z pliku...");
        #endif

        // Upewnij się, że system plików jest zamontowany
        if (!ensureFilesystem()) {
            #ifdef DEBUG
            Serial.println("[OdometerManager] System plików niedostępny - używam domyślnej wartości 0");
            #endif
            currentTotal = 0;
            saveToFile();  // Spróbuj zapisać domyślną wartość
            return;
        }

        // Sprawdź czy plik istnieje
        if (!LittleFS.exists(filename)) {
            #ifdef DEBUG
            Serial.println("[OdometerManager] Brak pliku licznika - tworzę nowy z wartością 0");
            #endif
            currentTotal = 0;
            saveToFile();
            return;
        }

        // Otwórz plik do odczytu
        File file = LittleFS.open(filename, "r");
        if (!file) {
            #ifdef DEBUG
            Serial.println("[OdometerManager] Błąd otwarcia pliku licznika do odczytu");
            #endif
            currentTotal = 0;
            return;
        }

        // Wczytaj i sparsuj JSON
        StaticJsonDocument<128> doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        
        if (!error) {
            currentTotal = doc["total"] | 0.0f;
            #ifdef DEBUG
            Serial.printf("[OdometerManager] Wczytano licznik: %.2f\n", currentTotal);
            #endif
        } else {
            #ifdef DEBUG
            Serial.printf("[OdometerManager] Błąd odczytu pliku licznika: %s\n", error.c_str());
            #endif
            currentTotal = 0;
        }
    }

public:
    OdometerManager() {
        loadFromFile();
    }

    float getRawTotal() const {
        return currentTotal;
    }

    bool setInitialValue(float value) {
        if (value < 0) {
            #ifdef DEBUG
            Serial.println("[OdometerManager] Błędna wartość początkowa (ujemna)");
            #endif
            return false;
        }
        
        #ifdef DEBUG
        Serial.printf("[OdometerManager] Ustawianie początkowej wartości licznika: %.2f\n", value);
        #endif

        currentTotal = value;
        saveToFile();
        return true;
    }

    void updateTotal(float newValue) {
        if (newValue > currentTotal) {
            #ifdef DEBUG
            Serial.printf("[OdometerManager] Aktualizacja licznika z %.2f na %.2f\n", currentTotal, newValue);
            #endif

            currentTotal = newValue;
            saveToFile();
        }
    }

    bool isValid() const {
        return true; // Zawsze zwraca true, bo nie ma już inicjalizacji Preferences
    }
};
