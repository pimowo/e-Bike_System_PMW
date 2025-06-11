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
        // Upewnij sie, ze Preferences sa zamkniete
        if (preferences.isOpen()) {
            preferences.end();
        }
    }

    void loadFromPreferences() {
        DEBUG_INFO("Wczytywanie licznika z preferencji...");

        if (!preferences.begin(NAMESPACE, false)) {
            DEBUG_ERROR("Nie mozna otworzyc przestrzeni nazw Preferences");
            currentTotal = 0.0f;
            initialized = false;
            return;
        }

        currentTotal = preferences.getFloat(TOTAL_KEY, 0.0f);
        initialized = true;
        preferences.end();

        DEBUG_INFO("Wczytano licznik: %.2f", currentTotal);
    }

    bool saveToPreferences() {
        DEBUG_INFO("Zapisywanie licznika do preferencji...");

        if (!preferences.begin(NAMESPACE, false)) {
            DEBUG_ERROR("Nie mozna otworzyc przestrzeni nazw Preferences");
            return false;
        }

        preferences.putFloat(TOTAL_KEY, currentTotal);
        bool success = preferences.isKey(TOTAL_KEY); // Sprawdz czy zapis sie powiodl
        preferences.end();

        if (success) {
            DEBUG_INFO("Zapisano licznik: %.2f", currentTotal);
            initialized = true;
        } else {
            DEBUG_ERROR("Blad zapisu licznika");
        }

        return success;
    }

    float getRawTotal() const {
        return currentTotal;
    }

    bool setInitialValue(float value) {
        if (value < 0.0f) {
            DEBUG_ERROR("Blad: ujemna wartosc licznika");
            return false;
        }

        DEBUG_INFO("Ustawianie poczatkowej wartosci licznika: %.2f", value);

        currentTotal = value;
        return saveToPreferences();
    }

    void updateTotal(float newValue) {
        if (newValue > currentTotal) {
            DEBUG_INFO("Aktualizacja licznika z %.2f na %.2f", currentTotal, newValue);
            
            currentTotal = newValue;
            saveToPreferences();
        }
    }

    bool isValid() const {
        return initialized;
    }

    // Metoda do recznej inicjalizacji (jesli konstruktor nie zadziala)
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
        DEBUG_DETAIL("=== Diagnostyka OdometerManager ===");
        
        if (!preferences.begin(NAMESPACE, true)) { // Tryb tylko do odczytu
            DEBUG_ERROR("Nie mozna otworzyc przestrzeni nazw Preferences");
            return;
        }
        
        DEBUG_DETAIL("Klucze w przestrzeni nazw %s:", NAMESPACE);
        DEBUG_DETAIL("- Licznik (total): %.2f", preferences.getFloat(TOTAL_KEY, -1.0f));
        
        preferences.end();
    }
};
