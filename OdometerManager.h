class OdometerManager {
private:
    Preferences preferences;
    const char* NAMESPACE = "odometer";
    const char* TOTAL_KEY = "total";
    float currentTotal = 0.0f;
    bool initialized = false;

public:
    OdometerManager() {
        loadFromPreferences();
    }

    ~OdometerManager() {
        // Upewnij się, że Preferences są zamknięte
        if (preferences.isOpen()) {
            preferences.end();
        }
    }

    void loadFromPreferences() {
        #ifdef DEBUG
        Serial.println("[OdometerManager] Wczytywanie licznika z preferencji...");
        #endif

        if (!preferences.begin(NAMESPACE, false)) {
            #ifdef DEBUG
            Serial.println("[OdometerManager] Nie można otworzyć przestrzeni nazw Preferences");
            #endif
            currentTotal = 0.0f;
            initialized = false;
            return;
        }

        currentTotal = preferences.getFloat(TOTAL_KEY, 0.0f);
        initialized = true;
        preferences.end();

        #ifdef DEBUG
        Serial.printf("[OdometerManager] Wczytano licznik: %.2f\n", currentTotal);
        #endif
    }

    bool saveToPreferences() {
        #ifdef DEBUG
        Serial.println("[OdometerManager] Zapisywanie licznika do preferencji...");
        #endif

        if (!preferences.begin(NAMESPACE, false)) {
            #ifdef DEBUG
            Serial.println("[OdometerManager] Nie można otworzyć przestrzeni nazw Preferences");
            #endif
            return false;
        }

        preferences.putFloat(TOTAL_KEY, currentTotal);
        bool success = preferences.isKey(TOTAL_KEY); // Sprawdź czy zapis się powiódł
        preferences.end();

        if (success) {
            #ifdef DEBUG
            Serial.printf("[OdometerManager] Zapisano licznik: %.2f\n", currentTotal);
            #endif
            initialized = true;
        } else {
            #ifdef DEBUG
            Serial.println("[OdometerManager] Błąd zapisu licznika");
            #endif
        }

        return success;
    }

    float getRawTotal() const {
        return currentTotal;
    }

    bool setInitialValue(float value) {
        if (value < 0.0f) {
            #ifdef DEBUG
            Serial.println("[OdometerManager] Błąd: ujemna wartość licznika");
            #endif
            return false;
        }

        #ifdef DEBUG
        Serial.printf("[OdometerManager] Ustawianie początkowej wartości licznika: %.2f\n", value);
        #endif

        currentTotal = value;
        return saveToPreferences();
    }

    void updateTotal(float newValue) {
        if (newValue > currentTotal) {
            #ifdef DEBUG
            Serial.printf("[OdometerManager] Aktualizacja licznika z %.2f na %.2f\n", currentTotal, newValue);
            #endif
            
            currentTotal = newValue;
            saveToPreferences();
        }
    }

    bool isValid() const {
        return initialized;
    }

    // Metoda do ręcznej inicjalizacji (jeśli konstruktor nie zadziała)
    bool initialize() {
        loadFromPreferences();
        return initialized;
    }

    // Metoda do resetowania licznika
    bool resetOdometer() {
        currentTotal = 0.0f;
        return saveToPreferences();
    }

    // Metoda do debugowania stanu Preferences
    void debugPreferences() {
        #ifdef DEBUG
        Serial.println("\n--- Diagnostyka OdometerManager ---");
        
        if (!preferences.begin(NAMESPACE, true)) { // Tryb tylko do odczytu
            Serial.println("Nie można otworzyć przestrzeni nazw Preferences");
            return;
        }
        
        Serial.printf("Klucze w przestrzeni nazw %s:\n", NAMESPACE);
        Serial.printf("- Licznik (total): %.2f\n", preferences.getFloat(TOTAL_KEY, -1.0f));
        
        preferences.end();
        Serial.println("-----------------------------\n");
        #endif
    }
};
