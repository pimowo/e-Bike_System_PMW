/*
01＿info Throttle Abnormality
03＿info Motor Hall Signal Abnormality
04＿info Torque sensor Signal Abnormality
05＿info Axis speed sensor Abnormality(only applied to torque sensor )
06＿info Motor or controller has short circuit Abnormality

01_info - Problem z Manetką
    W skrócie: Manetka nie działa prawidłowo
    Możliwe przyczyny: uszkodzenie manetki, złe połączenie, problemy z przewodami

03_info - Problem z Czujnikami w Silniku
    W skrócie: Silnik nie otrzymuje prawidłowych sygnałów od swoich wewnętrznych czujników
    Możliwe przyczyny: uszkodzenie czujników, problemy z okablowaniem silnika

04_info - Problem z Czujnikiem Siły Nacisku
    W skrócie: System nie wykrywa prawidłowo siły pedałowania
    Możliwe przyczyny: uszkodzenie czujnika, problemy z kalibracją

05_info - Problem z Czujnikiem Prędkości
    W skrócie: System ma problem z pomiarem prędkości
    Uwaga: Ten błąd pojawia się tylko w systemach z czujnikiem siły nacisku
    Możliwe przyczyny: uszkodzony czujnik, złe ustawienie magnesów

06_info - Wykryto Zwarcie
    W skrócie: Poważny problem elektryczny w silniku lub kontrolerze
    Możliwe przyczyny: uszkodzenie przewodów, zalanie wodą, wewnętrzne uszkodzenie
    UWAGA: Ten błąd wymaga natychmiastowej kontroli, aby uniknąć poważniejszych uszkodzeń!
*/

/********************************************************************
 * BIBLIOTEKI
 ********************************************************************/

// --- Podstawowe biblioteki systemowe ---
#include <Wire.h>               // Biblioteka do komunikacji I2C

// --- Biblioteki wyświetlacza ---
#include <U8g2lib.h>            // Biblioteka do obsługi wyświetlacza OLED

// --- Biblioteki czasu ---
#include <RTClib.h>             // Biblioteka do obsługi zegara czasu rzeczywistego (RTC)

// --- Biblioteki czujników temperatury ---
#include <OneWire.h>            // Biblioteka do komunikacji z czujnikami OneWire
#include <DallasTemperature.h>  // Biblioteka do obsługi czujników temperatury DS18B20

// --- Biblioteki Bluetooth ---
#include <BLEDevice.h>          // Główna biblioteka BLE
#include <BLEUtils.h>           // Narzędzia BLE
#include <BLE2902.h>            // Deskryptor powiadomień BLE

// --- Biblioteki sieciowe ---
#include <WiFi.h>               // Biblioteka do obsługi WiFi
#include <AsyncTCP.h>           // Biblioteka do asynchronicznej komunikacji TCP
#include <ESPAsyncWebServer.h>  // Biblioteka do obsługi serwera WWW

// --- Biblioteki systemu plików i JSON ---
#include <LittleFS.h>           // System plików dla ESP32
#include <ArduinoJson.h>        // Biblioteka do obsługi formatu JSON
#include <map>                  // Biblioteka do obsługi map (kontenerów)

// --- Biblioteki systemowe ESP32 ---
#include <esp_partition.h>    // Biblioteka do obsługi partycji ESP32
#include <Preferences.h>      // Biblioteka do stałej pamięci ESP32
#include <nvs_flash.h>

// --- Komunikaty ---
#include "DebugUtils.h"

// --- Oświetlenie ---
#include "LightManager.h"

/********************************************************************
 * DEFINICJE I STAŁE GLOBALNE
 ********************************************************************/

// Wersja oprogramowania
const char* VERSION = "11.6.25-B";

// Nazwy plików konfiguracyjnych
const char* CONFIG_FILE = "/display_config.json";
const char* LIGHT_CONFIG_FILE = "/light_config.json";

// Definicje pinów
// przyciski
#define BTN_UP 13
#define BTN_DOWN 14
#define BTN_SET 12
// światła
#define FrontDayPin 18  // światła dzienne (DRL)
#define FrontPin 19     // światła zwykłe (przednie główne)
#define RearPin 23      // tylne światło
// ładowarka USB
#define UsbPin 32  // ładowarka USB
// czujniki temperatury
#define TEMP_AIR_PIN 15        // temperatutra powietrza (DS18B20)
#define TEMP_CONTROLLER_PIN 4  // temperatura sterownika (DS18B20)
//#define TEMP_MOTOR_PIN         // temperatura silnika (NTC10k)
// kadencja
#define CADENCE_SENSOR_PIN 27
// hamulec
#define BRAKE_SENSOR_PIN 26  // czujnik hamulca

// Konfiguracja pinów UART
#define CONTROLLER_RX_PIN 16 // ESP32 RX (połącz z TX sterownika)
#define CONTROLLER_TX_PIN 17 // ESP32 TX (połącz z RX sterownika)

// Parametry komunikacji
#define CONTROLLER_UART_BAUD 9600     // Prędkość komunikacji na podstawie OSKD

// Stałe protokołu komunikacyjnego
#define CONTROLLER_PACKET_SIZE 12

// RPM i przelicznik prędkości
#define RPM_CONSTANT 0.1885

// Typy parametrów
#define KT_PARAM_P 1
#define KT_PARAM_C 2
#define KT_PARAM_L 3

// Stałe czasowe
const unsigned long DEBOUNCE_DELAY = 25;
const unsigned long BUTTON_DELAY = 200;
const unsigned long LONG_PRESS_TIME = 1000;
const unsigned long DOUBLE_CLICK_TIME = 300;
const unsigned long GOODBYE_DELAY = 3000;
const unsigned long SET_LONG_PRESS = 2000;
const unsigned long TEMP_REQUEST_INTERVAL = 1000;
const unsigned long DS18B20_CONVERSION_DELAY_MS = 750;

#define TEMP_ERROR -999.0

/********************************************************************
 * STRUKTURY I TYPY WYLICZENIOWE
 ********************************************************************/

// Struktury konfiguracyjne
struct TimeSettings {
    bool ntpEnabled;
    int8_t hours;
    int8_t minutes;
    int8_t seconds;
    int8_t day;
    int8_t month;
    int16_t year;
};

struct LightSettings {
    enum LightMode {
        NONE,
        FRONT,
        REAR,
        BOTH
    };
    
    LightMode dayLights;      // Konfiguracja świateł dziennych
    LightMode nightLights;    // Konfiguracja świateł nocnych
    bool dayBlink;            // Miganie w trybie dziennym
    bool nightBlink;          // Miganie w trybie nocnym
    uint16_t blinkFrequency;  // Częstotliwość migania
};

struct BacklightSettings {
    int Brightness;        // Podstawowa jasność w trybie manualnym
    int dayBrightness;    // Jasność dzienna w trybie auto
    int nightBrightness;  // Jasność nocna w trybie auto
    bool autoMode;        // Tryb automatyczny włączony/wyłączony
};

struct WiFiSettings {
    char ssid[32];
    char password[64];
};

struct ControllerSettings {
    String type;         // "kt-lcd" lub "s866"
    int ktParams[23];    // P1-P5, C1-C15, L1-L3
    int s866Params[20];  // P1-P20
};

struct GeneralSettings {
    uint8_t wheelSize;  // Wielkość koła w calach (lub 0 dla 700C)
    
    // Konstruktor z wartościami domyślnymi
    GeneralSettings() : wheelSize(26) {} // Domyślnie 26 cali
};

struct BluetoothConfig {
    bool bmsEnabled;
    bool tpmsEnabled;
    char bmsMac[18];         // Dodaj to pole (już może istnieć)
    char frontTpmsMac[18];   // Dodaj to pole
    char rearTpmsMac[18];    // Dodaj to pole
    
    BluetoothConfig() : bmsEnabled(false), tpmsEnabled(false) {
        bmsMac[0] = '\0';
        frontTpmsMac[0] = '\0';
        rearTpmsMac[0] = '\0';
    }
};

struct BmsData {
    float voltage;            // Napięcie całkowite [V]
    float current;            // Prąd [A]
    float remainingCapacity;  // Pozostała pojemność [Ah]
    float totalCapacity;      // Całkowita pojemność [Ah]
    uint8_t soc;              // Stan naładowania [%]
    uint8_t cycles;           // Liczba cykli
    float cellVoltages[16];   // Napięcia cel [V]
    float temperatures[4];    // Temperatury [°C]
    bool charging;            // Status ładowania
    bool discharging;         // Status rozładowania
};

struct TpmsData {
    float pressure;       // Ciśnienie w bar
    float temperature;    // Temperatura w °C
    int batteryPercent;   // Poziom baterii w %
    bool alarm;           // Status alarmu
    uint8_t sensorNumber; // Numer czujnika (1-4)
    bool isActive;        // Czy czujnik jest aktywny
    unsigned long lastUpdate; // Czas ostatniego odczytu
    char address[20];     // Adres czujnika jako string
};

// Klasa obsługująca licznik kilometrów
class OdometerManager {
private:
    const char* filename = "/odometer.json";
    float currentTotal = 0;

    void saveToFile() {
        // Upewnij się, że system plików jest zamontowany
        if (!LittleFS.begin(false)) {
            DEBUG_ERROR("Blad montowania LittleFS przed zapisem licznika");
            return;
        }

        File file = LittleFS.open(filename, "w");
        if (!file) {
            DEBUG_ERROR("Blad otwarcia pliku licznika do zapisu");
            return;
        }

        StaticJsonDocument<128> doc;
        doc["total"] = currentTotal;
        
        if (serializeJson(doc, file) == 0) {
            DEBUG_ERROR("Blad zapisu do pliku licznika");
        } else {
            DEBUG_INFO("Zapisano licznik: %.2f", currentTotal);
        }
        
        file.close();
    }

    void loadFromFile() {
        DEBUG_INFO("Wczytywanie licznika z pliku...");

        File file = LittleFS.open(filename, "r");
        if (!file) {
            DEBUG_INFO("Brak pliku licznika - tworze nowy");
            currentTotal = 0;
            saveToFile();
            return;
        }

        StaticJsonDocument<128> doc;
        DeserializationError error = deserializeJson(doc, file);
        
        if (!error) {
            currentTotal = doc["total"] | 0.0f;
            DEBUG_INFO("Wczytano licznik: %.2f", currentTotal);
        } else {
            DEBUG_ERROR("Blad odczytu pliku licznika");
            currentTotal = 0;
        }
        
        file.close();
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
            DEBUG_ERROR("Bledna wartosc poczatkowa (ujemna)");
            return false;
        }
        
        DEBUG_INFO("Ustawianie poczatkowej wartosci licznika: %.2f", value);

        currentTotal = value;
        saveToFile();
        return true;
    }

    void updateTotal(float newValue) {
        if (newValue > currentTotal) {
            //DEBUG_INFO("Aktualizacja licznika z %.2f na %.2f", currentTotal, newValue);

            currentTotal = newValue;
            saveToFile();
        }
    }

    bool isValid() const {
        return true; // Zawsze zwraca true, bo nie ma już inicjalizacji Preferences
    }
};

/********************************************************************
 * TYPY WYLICZENIOWE
 ********************************************************************/

// Ekrany główne
enum MainScreen {
    RANGE_SCREEN,      // Ekran zasięgu
    CADENCE_SCREEN,    // Ekran kadencji
    SPEED_SCREEN,      // Ekran prędkości
    POWER_SCREEN,      // Ekran mocy
    BATTERY_SCREEN,    // Ekran baterii
    TEMP_SCREEN,       // Ekran temperatur
    PRESSURE_SCREEN,   // Ekran ciśnienia
    USB_SCREEN,        // Ekran sterowania USB
    MAIN_SCREEN_COUNT  // Liczba głównych ekranów
};

// Podekrany
enum SpeedSubScreen {
    SPEED_KMH,       // Aktualna prędkość
    SPEED_AVG_KMH,   // Średnia prędkość
    SPEED_MAX_KMH,   // Maksymalna prędkość
    SPEED_SUB_COUNT  // Liczba ekranów
};

enum CadenceSubScreen {
    CADENCE_RPM,       // Aktualna kadencja
    CADENCE_AVG_RPM,   // Średnia kadencja
    CADENCE_MAX_RPM,   // Maksymalna kadencja
    CADENCE_SUB_COUNT  // Liczba ekranów  
};

enum TempSubScreen {
    TEMP_AIR,         // Temperatura powietrza
    TEMP_CONTROLLER,  // Temperatura sterownika
    TEMP_MOTOR,       // Temperatura silnika
    TEMP_SUB_COUNT    // Liczba ekranów
};

enum RangeSubScreen {
    ODOMETER_KM,     // Przebieg
    DISTANCE_KM,     // Dystans
    RANGE_KM,        // Zasięg
    RANGE_SUB_COUNT  // Liczba ekranów
};

enum BatterySubScreen {
    BATTERY_CAPACITY_AH,       // Pojemność w Ah
    BATTERY_CAPACITY_WH,       // Pojemność w Wh
    BATTERY_CAPACITY_PERCENT,  // Poziom naładowania w %
    BATTERY_VOLTAGE,           // Napięcie baterii
    BATTERY_CURRENT,           // Prąd baterii
    BATTERY_SUB_COUNT          // Liczba ekranów
};

enum PowerSubScreen {
    POWER_W,         // Aktualna moc
    POWER_AVG_W,     // Średnia moc
    POWER_MAX_W,     // Maksymalna moc
    POWER_SUB_COUNT  // Liczba ekranów
};

enum PressureSubScreen {
    PRESSURE_BAR,       // Ciśnienie w bar
    PRESSURE_VOLTAGE,   // Napięcie czujnika
    PRESSURE_TEMP,      // Temperatura czujnika
    PRESSURE_SUB_COUNT  // Liczba ekranów
};

/********************************************************************
 * ZMIENNE GLOBALNE
 ********************************************************************/

// Obiekty główne
OdometerManager odometer;
Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Zmienne stanu systemu
bool configModeActive = false;
bool legalMode = false;
bool welcomeAnimationDone = false;
bool displayActive = false;
bool showingWelcome = false;
bool cruiseControlActive = false;

// Zmienne stanu ekranu
MainScreen currentMainScreen = RANGE_SCREEN;
int currentSubScreen = 0;
bool inSubScreen = false;
uint8_t displayBrightness = 16;  // Wartość od 0 do 255 (jasność wyświetlacza)

// Zmienne dla automatycznego wyłączania
int autoOffTime = 0;            // Czas do automatycznego wyłączenia w minutach (0 = funkcja wyłączona)
unsigned long lastActivityTime = 0;  // Czas ostatniej aktywności
bool autoOffEnabled = false;    // Czy funkcja auto-off jest włączona
bool webConfigActive = false;

// Zmienne pomiarowe
float speed_kmh;
float temp_air;
float temp_controller;
float temp_motor;
float range_km;
float distance_km;
float battery_voltage;
float battery_current;
float battery_capacity_wh;
float battery_capacity_ah;
float speed_sum = 0;
int speed_count = 0;
int battery_capacity_percent;
int power_w;
int power_avg_w;
int power_max_w;
float speed_avg_kmh;
float speed_max_kmh;
// kadencja
#define CADENCE_SAMPLES_WINDOW 4
#define CADENCE_OPTIMAL_MIN 75
#define CADENCE_OPTIMAL_MAX 95
#define CADENCE_HYSTERESIS 2
enum CadenceArrow { ARROW_NONE, ARROW_UP, ARROW_DOWN };
CadenceArrow cadence_arrow_state = ARROW_NONE;
volatile unsigned long cadence_pulse_times[CADENCE_SAMPLES_WINDOW] = {0};
volatile unsigned long cadence_last_pulse_time = 0;
int cadence_rpm = 0;
int cadence_avg_rpm = 0;
int cadence_max_rpm = 0;
uint8_t cadence_pulses_per_revolution = 1;  // Zakres 1-24
uint32_t cadence_sum = 0;
uint32_t cadence_samples = 0;
unsigned long last_avg_max_update = 0;
const unsigned long AVG_MAX_UPDATE_INTERVAL = 5000; // 5s
const unsigned long cadenceArrowTimeout = 1000; // czas wyświetlania strzałek w milisekundach (1 sekunda)

// Zmienne dla czujników ciśnienia kół
float pressure_bar;           // przednie koło
float pressure_rear_bar;      // tylne koło
float pressure_voltage;       // napięcie przedniego czujnika
float pressure_rear_voltage;  // napięcie tylnego czujnika
float pressure_temp;          // temperatura przedniego czujnika
float pressure_rear_temp;     // temperatura tylnego czujnika

TpmsData frontTpms;
TpmsData rearTpms;
bool tpmsScanning = false;
unsigned long lastTpmsScanTime = 0;
const unsigned long TPMS_SCAN_INTERVAL = 5000; // 5 sekund
const char* FRONT_TPMS_ADDRESS = "XX:XX:XX:XX:XX:XX"; // Adres przedniej opony
const char* REAR_TPMS_ADDRESS = "YY:YY:YY:YY:YY:YY";  // Adres tylnej opony

// Zmienne dla czujnika temperatury
float currentTemp = DEVICE_DISCONNECTED_C;
bool conversionRequested = false;
unsigned long lastTempRequest = 0;
unsigned long ds18b20RequestTime;

// Zmienne dla przycisków
unsigned long lastDebounceTime = 0;
unsigned long upPressStartTime = 0;
unsigned long downPressStartTime = 0;
unsigned long setPressStartTime = 0;
unsigned long messageStartTime = 0;
bool upLongPressExecuted = false;
bool downLongPressExecuted = false;
bool setLongPressExecuted = false;
unsigned long lastSpecialComboTime = 0; // Czas ostatniej obsługi specjalnej kombinacji
const unsigned long SPECIAL_COMBO_LOCKOUT = 500; // Czas blokady po kombinacji (500ms)

// Zmienne konfiguracyjne
int assistLevel = 3;
bool assistLevelAsText = false;
int assistMode = 0;       // 0=PAS, 1=STOP, 2=GAZ, 3=P+G
bool usbEnabled = false;  // Stan wyjścia USB

// Zmienne dla trybu prowadzenia roweru
bool walkAssistActive = false;

// Zmienna do śledzenia stanu hamulca
bool brakeActive = false;

// Czcionki
const uint8_t* czcionka_mala = u8g2_font_profont11_mf;      // opis ekranów
const uint8_t* czcionka_srednia = u8g2_font_pxplusibmvga9_mf; // górna belka
const uint8_t* czcionka_duza = u8g2_font_fub20_tr;

// Stałe BMS
const uint8_t BMS_BASIC_INFO[] = {0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77};
const uint8_t BMS_CELL_INFO[] = {0xDD, 0xA5, 0x04, 0x00, 0xFF, 0xFC, 0x77};
const uint8_t BMS_TEMP_INFO[] = {0xDD, 0xA5, 0x08, 0x00, 0xFF, 0xF8, 0x77};

// Instancje obiektów globalnych
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
RTC_DS3231 rtc;
OneWire oneWireAir(TEMP_AIR_PIN);
OneWire oneWireController(TEMP_CONTROLLER_PIN);
DallasTemperature sensorsAir(&oneWireAir);
DallasTemperature sensorsController(&oneWireController);

// Obiekty BLE
BLEClient* bleClient;
BLEClient* tpmsClient = nullptr;
BLEAddress bmsMacAddress("a5:c2:37:05:8b:86");
BLERemoteService* bleService;
BLERemoteCharacteristic* bleCharacteristicTx;
BLERemoteCharacteristic* bleCharacteristicRx;

// Instancje struktur konfiguracyjnych
ControllerSettings controllerSettings;
TimeSettings timeSettings;
BacklightSettings backlightSettings;
WiFiSettings wifiSettings;
GeneralSettings generalSettings;
BluetoothConfig bluetoothConfig;
BmsData bmsData;
LightManager lightManager(FrontPin, FrontDayPin, RearPin);

/********************************************************************
 * KLASY POMOCNICZE
 ********************************************************************/

// Klasa obsługująca timeout
class TimeoutHandler {
    private:
        uint32_t startTime;
        uint32_t timeoutPeriod;
        bool isRunning;

    public:
        TimeoutHandler(uint32_t timeout_ms = 0)
            : startTime(0),
                timeoutPeriod(timeout_ms),
                isRunning(false) {}

        void start(uint32_t timeout_ms = 0) {
            if (timeout_ms > 0) timeoutPeriod = timeout_ms;
            startTime = millis();
            isRunning = true;
        }

        bool isExpired() {
            if (!isRunning) return false;
            return (millis() - startTime) >= timeoutPeriod;
        }

        void stop() {
            isRunning = false;
        }

        uint32_t getElapsed() {
            if (!isRunning) return 0;
            return (millis() - startTime);
      }
};

// Klasa obsługująca czujnik temperatury
class TemperatureSensor {
    private:
        static constexpr float INVALID_TEMP = -999.0f;
        static constexpr float MIN_VALID_TEMP = -50.0f;
        static constexpr float MAX_VALID_TEMP = 100.0f;
        bool conversionRequested = false;
        unsigned long lastRequestTime = 0;
        DallasTemperature* airSensor;
        DallasTemperature* controllerSensor;

    public:
        TemperatureSensor(DallasTemperature* air, DallasTemperature* controller) 
            : airSensor(air), controllerSensor(controller) {}

        void requestTemperature() {
            if (millis() - lastRequestTime >= TEMP_REQUEST_INTERVAL) {
                airSensor->requestTemperatures();
                controllerSensor->requestTemperatures();
                conversionRequested = true;
                lastRequestTime = millis();
            }
        }

        bool isValidTemperature(float temp) {
            return temp >= MIN_VALID_TEMP && temp <= MAX_VALID_TEMP;
        }

        float readAirTemperature() {
            if (!conversionRequested) return INVALID_TEMP;

            if (millis() - lastRequestTime < DS18B20_CONVERSION_DELAY_MS) {
                return INVALID_TEMP;  // Konwersja jeszcze trwa
            }

            float temp = airSensor->getTempCByIndex(0);
            return isValidTemperature(temp) ? temp : INVALID_TEMP;
        }

        float readControllerTemperature() {
            if (!conversionRequested) return INVALID_TEMP;

            if (millis() - lastRequestTime < DS18B20_CONVERSION_DELAY_MS) {
                return INVALID_TEMP;  // Konwersja jeszcze trwa
            }

            float temp = controllerSensor->getTempCByIndex(0);
            return isValidTemperature(temp) ? temp : INVALID_TEMP;
        }
};



/********************************************************************
 * DEKLARACJE I IMPLEMENTACJE FUNKCJI
 ********************************************************************/

// --- Deklaracje funkcji obsługi świateł ---
void setLights();
void saveLightSettings();
void loadLightSettings();
void applyBacklightSettings();
void printLightConfigFile();

// --- Deklaracje funkcji obsługi ekranu ---
void drawHorizontalLine();
void drawVerticalLine();
void drawTopBar();
void drawLightStatus();
void drawAssistLevel();
void drawValueAndUnit(const char* valueStr, const char* unitStr);
void drawUpArrow();
void drawCircleIcon();
void drawDownArrow();
void drawMainDisplay();
void drawCadenceArrowsAndCircle();
void drawCenteredText(const char* text, int y, const uint8_t* font);
void showWalkAssistMode(bool sendBuffer);
void showWelcomeMessage();
void clearTripData();

// --- Deklaracje funkcji obsługi przycisków ---
void handleButtons();
void checkConfigMode();
void activateConfigMode();
void deactivateConfigMode();
void toggleLegalMode();

// --- Deklaracje funkcji pomocniczych ---
bool hasSubScreens(MainScreen screen);
int getSubScreenCount(MainScreen screen);
void resetTripData();
void setCadencePulsesPerRevolution(uint8_t pulses);
void goToSleep();
bool isValidTemperature(float temp);
void handleTemperature();
void saveLightMode();
void loadLightMode();
void updateActivityTime();

// --- Deklaracje funkcji konfiguracyjnych ---
void loadSettings();
void saveSettings();
int getParamIndex(const String& param);
void updateControllerParam(const String& param, int value);
const char* getLightModeString(LightSettings::LightMode mode);
void setupWebServer();
bool initLittleFS();
void listFiles();
bool loadConfig();
void initializeDefaultSettings();
void setDisplayBrightness(uint8_t brightness);
void saveBacklightSettingsToFile();
void loadBacklightSettingsFromFile();
void saveGeneralSettingsToFile();
void saveBluetoothConfigToFile();
void loadBluetoothConfigFromFile();
void loadGeneralSettingsFromFile();
void saveAutoOffSettings(); // Dodaj te dwie linijki
void loadAutoOffSettings(); //

// --- Deklaracje funkcji TPMS ---
void updateTpmsData(const char* address, uint8_t sensorNumber, float pressure, float temperature, uint8_t batteryPercent, bool alarm);
void saveTpmsAddresses();
void startTpmsScan();
void stopTpmsScan();
void loadTpmsAddresses();
void checkTpmsTimeout();

// --- Deklaracje funkcji BMS ---
void requestBmsData(const uint8_t* command, size_t length);
void updateBmsData();
void connectToBms();       

float getOdometerValue();
bool saveOdometerValue(float value);

// --- UART kontroler

void IRAM_ATTR cadence_ISR() {
    unsigned long now = millis();
    unsigned long minDelay = max(8, 200 / cadence_pulses_per_revolution); // Minimum 8ms
    
    if (now - cadence_last_pulse_time > minDelay) {
        for (int i = CADENCE_SAMPLES_WINDOW - 1; i > 0; i--) {
            cadence_pulse_times[i] = cadence_pulse_times[i - 1];
        }
        cadence_pulse_times[0] = now;
        cadence_last_pulse_time = now;

        // Resetuj timer aktywności przy wykryciu pedałowania
        updateActivityTime();
    }
}

// Funkcja wysyłająca komendę włączenia/wyłączenia świateł do sterownika KT
void sendLightCommandToKT(bool lightsOn) {
    // Przykładowy format komendy (musisz dostosować do faktycznego protokołu KT)
    if (lightsOn) {
        Serial2.write(0xA5); // Przykładowy nagłówek
        Serial2.write(0x03); // Przykładowa długość danych
        Serial2.write(0x01); // Przykładowy kod komendy "włącz światła"
        Serial2.write(0xA9); // Przykładowa suma kontrolna
    } else {
        Serial2.write(0xA5); // Przykładowy nagłówek
        Serial2.write(0x03); // Przykładowa długość danych
        Serial2.write(0x00); // Przykładowy kod komendy "wyłącz światła"
        Serial2.write(0xA8); // Przykładowa suma kontrolna
    }
}

// --- Funkcje BLE ---

// callback dla BLE
void notificationCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (length < 2) return;  // Sprawdzenie minimalnej długości pakietu

    switch (pData[1]) {  // Sprawdź typ pakietu
        case 0x03:  // Basic info
            if (length >= 34) {
                // Napięcie całkowite (0.1V)
                bmsData.voltage = (float)((pData[4] << 8) | pData[5]) / 10.0;
                
                // Prąd (0.1A, wartość ze znakiem)
                int16_t current = (pData[6] << 8) | pData[7];
                bmsData.current = (float)current / 10.0;
                
                // Pozostała pojemność (0.1Ah)
                bmsData.remainingCapacity = (float)((pData[8] << 8) | pData[9]) / 10.0;
                
                // SOC (%) - dodane zabezpieczenie przed indeksem out-of-bounds
                if (length > 23) {
                    bmsData.soc = pData[23];
                
                    // Status ładowania/rozładowania - dodane zabezpieczenie
                    if (length > 22) {
                        uint8_t status = pData[22];
                        bmsData.charging = (status & 0x01);
                        bmsData.discharging = (status & 0x02);
                    }
                }
            }
            break;

        case 0x04:  // Cell info
            if (length >= 34) {  // Sprawdź czy mamy kompletny pakiet
                const int maxCells = std::min(16, (int)((length - 4) / 2));
                for (int i = 0; i < maxCells; i++) {
                    bmsData.cellVoltages[i] = (float)((pData[4 + i*2] << 8) | pData[5 + i*2]) / 1000.0;
                }
            }
            break;

        case 0x08:  // Temperature info
            if (length >= 12) {
                const int maxTemps = std::min(4, (int)((length - 4) / 2));
                for (int i = 0; i < maxTemps; i++) {
                    // Konwersja z K na °C
                    int16_t temp = ((pData[4 + i*2] << 8) | pData[5 + i*2]) - 2731;
                    bmsData.temperatures[i] = (float)temp / 10.0;
                }
            }
            break;
    }
}

// Dodaj po callbacku dla BMS
class TpmsAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        // Jeśli TPMS nie jest włączone, nie rób nic
        if (!bluetoothConfig.tpmsEnabled) {
            return;
        }
        
        // Pobierz adres urządzenia
        String deviceAddress = advertisedDevice.getAddress().toString().c_str();
        
        // Sprawdź czy to jeden z naszych czujników
        bool isFrontSensor = (strcmp(deviceAddress.c_str(), bluetoothConfig.frontTpmsMac) == 0);
        bool isRearSensor = (strcmp(deviceAddress.c_str(), bluetoothConfig.rearTpmsMac) == 0);
        
        // Jeśli to nie jest żaden z naszych czujników, ignorujemy
        if (!isFrontSensor && !isRearSensor) {
            return;
        }
        
        if (advertisedDevice.haveManufacturerData()) {
            // Tutaj jest problem - zamienimy na użycie String zamiast std::string
            String manufacturerData = advertisedDevice.getManufacturerData();
            
            // Sprawdź czy dane mają właściwy format (przynajmniej 18 bajtów)
            if (manufacturerData.length() >= 18) {
                // Sprawdź czy pierwsze dwa bajty to 0001 (ID producenta)
                uint8_t id1 = manufacturerData[0];
                uint8_t id2 = manufacturerData[1];
                
                if (id1 == 0x00 && id2 == 0x01) {
                    // Czytamy numer czujnika z trzeciego bajtu
                    uint8_t sensorNumber = manufacturerData[2];
                    
                    // Adres czujnika (bajty 5-7)
                    char sensorAddressStr[20] = "";
                    for (int i = 5; i <= 7; i++) {
                        char hexStr[3];
                        sprintf(hexStr, "%02X", (uint8_t)manufacturerData[i]);
                        strcat(sensorAddressStr, hexStr);
                    }
                    
                    // Obliczamy ciśnienie (bajty 8-11)
                    uint32_t pressureValue = 
                        (uint8_t)manufacturerData[8] | 
                        ((uint8_t)manufacturerData[9] << 8) | 
                        ((uint8_t)manufacturerData[10] << 16) | 
                        ((uint8_t)manufacturerData[11] << 24);
                    float pressureBar = pressureValue / 100000.0; // Konwersja na bar
                    
                    // Obliczamy temperaturę (bajty 12-15)
                    uint32_t tempValue = 
                        (uint8_t)manufacturerData[12] | 
                        ((uint8_t)manufacturerData[13] << 8) | 
                        ((uint8_t)manufacturerData[14] << 16) | 
                        ((uint8_t)manufacturerData[15] << 24);
                    float temperatureC = tempValue / 100.0; // Konwersja na stopnie Celsjusza
                    
                    // Poziom baterii (bajt 16)
                    uint8_t batteryPercentage = (uint8_t)manufacturerData[16];
                    
                    // Status alarmu (bajt 17)
                    bool alarmActive = ((uint8_t)manufacturerData[17] != 0);
                    
                    // Utwórz pełny adres czujnika łącząc prefiks (bajty 3-4) i adres (bajty 5-7)
                    char fullAddress[20] = "";
                    for (int i = 3; i <= 7; i++) {
                        char hexStr[3];
                        sprintf(hexStr, "%02X", (uint8_t)manufacturerData[i]);
                        strcat(fullAddress, hexStr);
                        if (i < 7 && i != 4) strcat(fullAddress, ":");
                    }
                    
                    // Dodaj informację o rodzaju czujnika
                    if (isFrontSensor) {
                        DEBUG_BLE("Znaleziono dane z przedniego czujnika");
                        sensorNumber = 0x80; // Wymuszamy przedni czujnik
                    } else if (isRearSensor) {
                        DEBUG_BLE("Znaleziono dane z tylnego czujnika");
                        sensorNumber = 0x81; // Wymuszamy tylny czujnik
                    }
                    
                    // Aktualizuj dane
                    updateTpmsData(fullAddress, sensorNumber, pressureBar, temperatureC, batteryPercentage, alarmActive);
                }
            }
        }
    }
};

// wysyłanie zapytania do BMS
void requestBmsData(const uint8_t* command, size_t length) {
    if (bleClient && bleClient->isConnected() && bleCharacteristicTx) {
    //bleCharacteristicTx->writeValue(command, length);
    }
}

// aktualizacja danych BMS
void updateBmsData() {
    static unsigned long lastBmsUpdate = 0;
    static uint8_t requestState = 0; // Stan sekwencji zapytań
    const unsigned long BMS_UPDATE_INTERVAL = 1000; // Aktualizuj co 1 sekundę
    const unsigned long SEQUENTIAL_DELAY = 100; // Opóźnienie między krokami sekwencji

    if (!bleClient || !bleClient->isConnected()) {
        return;
    }

    unsigned long currentTime = millis();
    if (currentTime - lastBmsUpdate >= SEQUENTIAL_DELAY) {
        switch(requestState) {
            case 0:
                requestBmsData(BMS_BASIC_INFO, sizeof(BMS_BASIC_INFO));
                requestState = 1;
                break;
            case 1:
                requestBmsData(BMS_CELL_INFO, sizeof(BMS_CELL_INFO));
                requestState = 2;
                break;
            case 2:
                requestBmsData(BMS_TEMP_INFO, sizeof(BMS_TEMP_INFO));
                requestState = 0;
                lastBmsUpdate = currentTime; // Reset głównego timera tylko po pełnej sekwencji
                break;
        }
    }
}

// połączenie z BMS
void connectToBms() {
    if (!bleClient->isConnected()) {
    
        DEBUG_BLE("Próba połączenia z BMS...");

        if (bleClient->connect(bmsMacAddress)) {
            DEBUG_BLE("Połączono z BMS");

            bleService = bleClient->getService("0000ff00-0000-1000-8000-00805f9b34fb");
        
            if (bleService == nullptr) {
                DEBUG_BLE("Nie znaleziono uslugi BMS");
                bleClient->disconnect();
                return;
            }

            bleCharacteristicTx = bleService->getCharacteristic("0000ff02-0000-1000-8000-00805f9b34fb");
        
            if (bleCharacteristicTx == nullptr) {
                DEBUG_BLE("Nie znaleziono charakterystyki Tx");
                bleClient->disconnect();
                return;
            }

            bleCharacteristicRx = bleService->getCharacteristic("0000ff01-0000-1000-8000-00805f9b34fb");
        
            if (bleCharacteristicRx == nullptr) {
                DEBUG_BLE("Nie znaleziono charakterystyki Rx");
                bleClient->disconnect();
                return;
            }

            // Rejestracja funkcji obsługi powiadomień BLE
            if (bleCharacteristicRx->canNotify()) {
                bleCharacteristicRx->registerForNotify(notificationCallback);
                DEBUG_BLE("Zarejestrowano powiadomienia dla Rx");
            } else {
                DEBUG_BLE("Charakterystyka Rx nie obsługuje powiadomień");
                bleClient->disconnect();
                return;
            }

        } else {

            DEBUG_BLE("Nie udalo sie polaczyc z BMS");
        }
    }
}

void saveTpmsAddresses() {

    if (!LittleFS.begin(false)) {
        DEBUG_BLE("Błąd montowania LittleFS przy zapisie adresów TPMS");
        return;
    }

    StaticJsonDocument<256> doc;

    // Zapisz adresy czujników
    if (frontTpms.isActive && frontTpms.address[0] != '\0') {
        doc["front_tpms"] = frontTpms.address;
    }

    if (rearTpms.isActive && rearTpms.address[0] != '\0') {
        doc["rear_tpms"] = rearTpms.address;
    }

    File file = LittleFS.open("/tpms_config.json", "w");

    if (!file) {
        DEBUG_BLE("Nie można otworzyć pliku konfiguracji TPMS do zapisu");
        return;
    }

    serializeJson(doc, file);
    file.close();
    DEBUG_BLE("Zapisano konfigurację TPMS");
}

// Dodaj do sekcji funkcji
void updateTpmsData(const char* address, uint8_t sensorNumber, float pressure, float temperature, uint8_t batteryPercent, bool alarm) {

    DEBUG_BLE("Odebrano dane TPMS: ");
    DEBUG_BLE("Adres=%s", address);
    DEBUG_BLE("Czujnik=%d", sensorNumber);
    DEBUG_BLE("Ciśnienie=%.2f", pressure);
    DEBUG_BLE("Temperatura=%.2f", temperature);
    DEBUG_BLE("Bateria=%d", batteryPercent);
    DEBUG_BLE("Alarm=%s", alarm ? "TAK" : "NIE");
    
    // Sprawdź, czy to przedni czy tylny czujnik na podstawie numeru czujnika lub adresu
    bool isFrontSensor = false;
    bool isRearSensor = false;
    
    // Sprawdź numer czujnika (0x80 = 1, 0x81 = 2, itd.)
    if (sensorNumber == 0x80) {
        isFrontSensor = true;
    } else if (sensorNumber == 0x81) {
        isRearSensor = true;
    } else {
        // Jeśli nie mamy jeszcze żadnego aktywnego czujnika
        if (!frontTpms.isActive && !rearTpms.isActive) {
            // Pierwszy wykryty czujnik to przedni
            isFrontSensor = true;
        }
        // Jeśli mamy aktywny przedni ale nie tylny
        else if (frontTpms.isActive && !rearTpms.isActive) {
            // To nowy czujnik jest tylny
            isRearSensor = true;
        }
    }
    
    if (isFrontSensor) {
        frontTpms.pressure = pressure;
        frontTpms.temperature = temperature;
        frontTpms.batteryPercent = batteryPercent;
        frontTpms.alarm = alarm;
        frontTpms.sensorNumber = sensorNumber;
        frontTpms.lastUpdate = millis();
        frontTpms.isActive = true;

        // Aktualizacja zmiennych globalnych używanych przez interfejs
        pressure_bar = pressure;
        pressure_temp = temperature;
        pressure_voltage = batteryPercent / 100.0; // Konwersja % na napięcie 0-1
        
        DEBUG_BLE("Zaktualizowano przedni czujnik");
    } else if (isRearSensor) {
        rearTpms.pressure = pressure;
        rearTpms.temperature = temperature;
        rearTpms.batteryPercent = batteryPercent;
        rearTpms.alarm = alarm;
        rearTpms.sensorNumber = sensorNumber;
        rearTpms.lastUpdate = millis();
        rearTpms.isActive = true;
        
        // Aktualizacja zmiennych globalnych używanych przez interfejs
        pressure_rear_bar = pressure;
        pressure_rear_temp = temperature;
        pressure_rear_voltage = batteryPercent / 100.0; // Konwersja % na napięcie 0-1
        
        DEBUG_BLE("Zaktualizowano tylny czujnik");
    }
}

void startTpmsScan() {
    if (tpmsScanning || !bluetoothConfig.tpmsEnabled) return;
    
    DEBUG_BLE("Rozpoczynam nasluchiwanie TPMS...");
    DEBUG_BLE("Przedni czujnik MAC: %s", bluetoothConfig.frontTpmsMac);
    DEBUG_BLE("Tylny czujnik MAC: %s", bluetoothConfig.rearTpmsMac);
    
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new TpmsAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(false); // Pasywne skanowanie (mniej energii)
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    pBLEScan->start(0, true); // Skanowanie ciągłe w trybie pasywnym
    tpmsScanning = true;
    lastTpmsScanTime = millis();
}

void stopTpmsScan() {
    if (!tpmsScanning) return;
    
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->stop();
    tpmsScanning = false;
    
    DEBUG_BLE("Zatrzymano skanowanie TPMS");
}

void loadTpmsAddresses() {
    DEBUG_BLE("Funkcja loadTpmsAddresses() jeszcze nie jest zaimplementowana");
    // Na razie pusta implementacja
    // Docelowo będzie wczytywać adresy czujników z pliku konfiguracyjnego
}

void checkTpmsTimeout() {

    unsigned long currentTime = millis();
    const unsigned long TPMS_TIMEOUT = 30000; // 30 sekund
    
    // Sprawdź, czy czujniki nie wysłały danych przez dłuższy czas
    if (frontTpms.isActive && (currentTime - frontTpms.lastUpdate > TPMS_TIMEOUT)) {
        DEBUG_BLE("Przedni czujnik nie odpowiada - oznaczam jako nieaktywny");
        frontTpms.isActive = false;
    }
    
    if (rearTpms.isActive && (currentTime - rearTpms.lastUpdate > TPMS_TIMEOUT)) {
        DEBUG_BLE("Tylny czujnik nie odpowiada - oznaczam jako nieaktywny");
        rearTpms.isActive = false;
    }
}

// --- Funkcje konfiguracji ---

void setLights() {}
void saveLightSettings() {}
void loadLightSettings() {}
void saveLightMode() {}
void loadLightMode() {}

// Funkcja aktualizująca czas ostatniej aktywności
void updateActivityTime() {
    static unsigned long lastUpdateTime = 0;
    unsigned long currentTime = millis();
    
    // Ograniczenie do max 1 aktualizacji na sekundę (1000ms)
    if (currentTime - lastUpdateTime >= 1000) {
        lastActivityTime = currentTime;
        lastUpdateTime = currentTime;
        DEBUG_INFO("Aktywnosc wykryta: czas = %u ms", lastActivityTime);
    }
}

// Funkcja sprawdzająca czy należy automatycznie wyłączyć system
void checkAutoOff() {
    // Nie wykonuj automatycznego wyłączenia, gdy:
    // 1. Auto-off jest wyłączone (autoOffTime <= 0)
    // 2. Jesteśmy w trybie konfiguracji (configModeActive)
    // 3. Aktywna jest konfiguracja przez WWW (webConfigActive)
    if (autoOffTime <= 0 || configModeActive || webConfigActive) {
        return;
    }
    
    // Oblicz czas nieaktywności z obsługą przepełnienia millis()
    unsigned long currentTime = millis();
    unsigned long inactiveTime;
    
    // Obsługa przepełnienia millis()
    if (currentTime < lastActivityTime) {
        // Przepełnienie - oblicz prawidłowy czas
        inactiveTime = (0xFFFFFFFFUL - lastActivityTime) + currentTime;
    } else {
        inactiveTime = currentTime - lastActivityTime;
    }
    
    // Oblicz próg wyłączenia (autoOffTime w minutach * 60000 ms)
    uint32_t threshold = (uint32_t)autoOffTime * 60000UL;
    
    // Debugowanie co 10 sekund
    static unsigned long lastCheckTime = 0;
    if (currentTime - lastCheckTime > 10000) {
        lastCheckTime = currentTime;
        DEBUG_INFO("Auto-off: nieaktywnosc = %u s, prog = %u s, auto-off = %d min", inactiveTime/1000, threshold/1000, autoOffTime);
    }
    
    // Sprawdź czy przekroczono próg
    if (inactiveTime >= threshold) {
        DEBUG_INFO("Auto-off: wyłaczanie po %d min nieaktywnosci (%u s)", autoOffTime, inactiveTime/1000);
        
        // Powiadom użytkownika przed wyłączeniem (opcjonalnie)
        display.clearBuffer();
        //display.setFont(czcionka_srednia);
        drawCenteredText("Automatyczne", 25, czcionka_srednia);
        drawCenteredText("wylaczenie", 45, czcionka_srednia);
        display.sendBuffer();
        
        // Krótkie opóźnienie aby komunikat został wysłany i wyświetlony
        delay(2500);
        
        // Wyłącz system
        goToSleep();
    }
}

void printLightConfig() {

    if (!LittleFS.begin(false)) {
        DEBUG_ERROR("Blad montowania systemu plikow");
        return;
    }
    
    if (LittleFS.exists("/light_config.json")) {

        File file = LittleFS.open("/light_config.json", "r");

        if (file) {
            DEBUG_LIGHT("Zawartosc pliku konfiguracyjnego swiatel:");
            
            // Wczytanie całego pliku do bufora
            String fileContent = "";
            while (file.available()) {
                fileContent += (char)file.read();
            }
            DEBUG_LIGHT("%s", fileContent.c_str());
            
            file.close();
        } else {
            DEBUG_ERROR("Nie mozna otworzyc pliku konfiguracyjnego");
        }

    } else {

        DEBUG_INFO("Plik konfiguracyjny nie istnieje");
    }
}

// ustawianie jasności wyświetlacza
void setDisplayBrightness(uint8_t brightness) {
    displayBrightness = brightness;
    display.setContrast(displayBrightness);
}

// zapis ustawień podświetlenia
void saveBacklightSettingsToFile() {
    // Nie montujemy systemu plików ponownie - zakładamy, że jest już zamontowany
    // Jeśli nie jest, to inne operacje i tak by nie działały
    
    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file) {
        DEBUG_LIGHT("Nie można otworzyć pliku do zapisu");
        return;
    }

    StaticJsonDocument<200> doc;
    doc["brightness"] = backlightSettings.Brightness;
    doc["dayBrightness"] = backlightSettings.dayBrightness;
    doc["nightBrightness"] = backlightSettings.nightBrightness;
    doc["autoMode"] = backlightSettings.autoMode;

    // Zapisz JSON do pliku
    if (serializeJson(doc, file)) {
        DEBUG_LIGHT("Zapisano ustawienia do pliku");
    } else {
        DEBUG_LIGHT("Błąd podczas zapisu do pliku");
    }
    
    file.close(); // Zawsze zamykaj plik po operacjach
    
    // Zastosuj nowe ustawienia
    applyBacklightSettings();
}

// wczytywanie ustawień podświetlenia


// zapis ustawień ogólnych
void saveGeneralSettingsToFile() {
    File file = LittleFS.open("/general_config.json", "w");
    if (!file) {
        DEBUG_INFO("Nie można otworzyć pliku ustawień ogólnych do zapisu");
        return;
    }

    StaticJsonDocument<64> doc;
    doc["wheelSize"] = generalSettings.wheelSize;

    if (serializeJson(doc, file) == 0) {
        DEBUG_INFO("Błąd podczas zapisu ustawień ogólnych");
    }

    file.close();
}

// zapis konfiguracji Bluetooth
void saveBluetoothConfigToFile() {
    File file = LittleFS.open("/bluetooth_config.json", "w");
    if (!file) {
        DEBUG_BLE("Nie można otworzyć pliku konfiguracji Bluetooth");
        return;
    }

    StaticJsonDocument<256> doc; // Zwiększ rozmiar dokumentu
    doc["bmsEnabled"] = bluetoothConfig.bmsEnabled;
    doc["tpmsEnabled"] = bluetoothConfig.tpmsEnabled;
    doc["bmsMac"] = bluetoothConfig.bmsMac;
    doc["frontTpmsMac"] = bluetoothConfig.frontTpmsMac; // Dodaj to
    doc["rearTpmsMac"] = bluetoothConfig.rearTpmsMac;   // Dodaj to

    serializeJson(doc, file);
    file.close();
}

// wczytywanie konfiguracji Bluetooth
void loadBluetoothConfigFromFile() {
    File file = LittleFS.open("/bluetooth_config.json", "r");
    if (!file) {
        DEBUG_BLE("Nie znaleziono pliku konfiguracji Bluetooth, używam domyślnych");
        return;
    }

    StaticJsonDocument<256> doc; // Zwiększ rozmiar dokumentu
    DeserializationError error = deserializeJson(doc, file);
    
    if (!error) {
        bluetoothConfig.bmsEnabled = doc["bmsEnabled"] | false;
        bluetoothConfig.tpmsEnabled = doc["tpmsEnabled"] | false;
        
        // Dodaj obsługę MAC adresów
        if (doc.containsKey("bmsMac")) {
            strlcpy(bluetoothConfig.bmsMac, doc["bmsMac"], sizeof(bluetoothConfig.bmsMac));
        }
        
        if (doc.containsKey("frontTpmsMac")) {
            strlcpy(bluetoothConfig.frontTpmsMac, doc["frontTpmsMac"], sizeof(bluetoothConfig.frontTpmsMac));
        }
        
        if (doc.containsKey("rearTpmsMac")) {
            strlcpy(bluetoothConfig.rearTpmsMac, doc["rearTpmsMac"], sizeof(bluetoothConfig.rearTpmsMac));
        }
    }
    
    file.close();
}

// wczytywanie ustawień ogólnych
void loadGeneralSettingsFromFile() {
    File file = LittleFS.open("/general_config.json", "r");
    if (!file) {
        DEBUG_INFO("Nie znaleziono pliku ustawień ogólnych, używam domyślnych");
        generalSettings.wheelSize = 26; // Wartość domyślna
        saveGeneralSettingsToFile(); // Zapisz domyślne ustawienia
        return;
    }

    StaticJsonDocument<64> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        DEBUG_INFO("Błąd podczas parsowania JSON ustawień ogólnych");
        generalSettings.wheelSize = 26; // Wartość domyślna
        saveGeneralSettingsToFile(); // Zapisz domyślne ustawienia
        return;
    }

    generalSettings.wheelSize = doc["wheelSize"] | 26; // Domyślnie 26 cali jeśli nie znaleziono

    DEBUG_INFO("Loaded wheel size: %d", generalSettings.wheelSize);
}

// Funkcja zapisująca ustawienia automatycznego wyłączania
void saveAutoOffSettings() {
    if (!LittleFS.begin()) {
        DEBUG_ERROR("Nie mozna zamontowac systemu plikow!");
        return;
    }

    StaticJsonDocument<256> doc;
    doc["autoOffTime"] = autoOffTime;

    File file = LittleFS.open("/auto_off.json", "w");
    if (!file) {
        DEBUG_ERROR("Nie mozna otworzyc pliku auto_off.json do zapisu!");
        return;
    }

    if (serializeJson(doc, file) == 0) {
        DEBUG_ERROR("Blad podczas zapisu do pliku auto_off.json!");
    } else {
        DEBUG_INFO("Zapisano ustawienia auto-off: %d minut", autoOffTime);
    }
    file.close();
    
    // Sprawdź, czy plik został poprawnie zapisany
    File testFile = LittleFS.open("/auto_off.json", "r");
    if (testFile) {
        String content = testFile.readString();
        DEBUG_INFO("Zawartosc pliku auto_off.json: %s", content.c_str());
        testFile.close();
    } else {
        DEBUG_ERROR("Nie mozna odczytac zapisanego pliku auto_off.json!");
    }
}

void loadAutoOffSettings() {
    if (!LittleFS.begin()) {
        DEBUG_ERROR("Nie można zamontować systemu plików!");
        return;
    }

    File file = LittleFS.open("/auto_off.json", "r");
    if (!file) {
        DEBUG_INFO("Brak pliku auto_off.json, używam domyślnych ustawień");
        autoOffTime = 0; // Domyślnie wyłączone
        return;
    }

    String content = file.readString();
    DEBUG_INFO("Wczytany plik auto_off.json: %s", content.c_str());
    
    file.seek(0);
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        DEBUG_ERROR("Błąd parsowania auto_off.json: %s", error.c_str());
        return;
    }

    autoOffTime = doc["autoOffTime"] | 0;
    DEBUG_INFO("Wczytano ustawienia auto-off: %d minut", autoOffTime);
}

// --- Funkcje wyświetlacza ---

// rysowanie linii poziomej
void drawHorizontalLine() {
    display.drawHLine(4, 12, 122);
    display.drawHLine(4, 48, 122);
}

// rysowanie linii pionowej
void drawVerticalLine() {
    display.drawVLine(24, 16, 28);
    display.drawVLine(68, 16, 28);
}

// rysowanie górnego paska
void drawTopBar() {
    static bool colonVisible = true;
    static unsigned long lastColonToggle = 0;
    const unsigned long COLON_TOGGLE_INTERVAL = 500;  // Miganie co 500ms (pół sekundy)

    display.setFont(czcionka_srednia);

    // Pobierz aktualny czas z RTC
    DateTime now = rtc.now();

    // Czas z migającym dwukropkiem
    char timeStr[6];
    if (colonVisible) {
        sprintf(timeStr, "%02d:%02d", now.hour(), now.minute());
    } else {
        sprintf(timeStr, "%02d %02d", now.hour(), now.minute());
    }
    display.drawStr(0, 10, timeStr);

    // Przełącz stan dwukropka co COLON_TOGGLE_INTERVAL
    if (millis() - lastColonToggle >= COLON_TOGGLE_INTERVAL) {
        colonVisible = !colonVisible;
        lastColonToggle = millis();
    }

    // Bateria
    char battStr[5];
    sprintf(battStr, "%d%%", battery_capacity_percent);
    display.drawStr(58, 10, battStr);

    // Napięcie
    char voltStr[6];
    sprintf(voltStr, "%.0fV", battery_voltage);
    display.drawStr(100, 10, voltStr);
}

// wyświetlanie statusu świateł
void drawLightStatus() {
    display.setFont(czcionka_mala);

    if (lightManager.getControlMode() == LightManager::SMART_CONTROL) {
        // Dla trybu Smart wyświetlamy "Dzien" lub "Noc"
        switch (lightManager.getMode()) {
            case LightManager::DAY:
                display.drawStr(28, 45, "Dzien");
                break;
            case LightManager::NIGHT:
                display.drawStr(28, 45, "Noc");
                break;
        }
    } else {
        // Dla trybu Sterownik wyświetlamy "Lampy" jeśli są włączone
        if (lightManager.getMode() != LightManager::OFF) {
            display.drawStr(28, 45, "Lampy");
        }
    }
}

// wyświetlanie poziomu wspomagania
void drawAssistLevel() {
    display.setFont(czcionka_duza);

    // Wyświetl "T" gdy tempomat jest aktywny
    if (cruiseControlActive) {
        display.drawStr(2, 40, "T");
    } else {
        // Wyświetlanie poziomu asysty
        if (legalMode) {
            // Wymiary cyfry i tła
            const int digit_height = 20;  // wysokość czcionki
            const int padding = 3;        // margines wewnętrzny
            const int total_width = 16 + (padding * 2);  // szerokość cyfry + marginesy
            const int total_height = digit_height + (padding * 2);  // wysokość + marginesy
            
            int x = 2;
            int y = 40;
            
            // Pozycja tła (box)
            int box_x = x - padding;
            int box_y = y - digit_height - padding;  // przesunięcie w górę o wysokość + padding
            
            // Rysuj białe tło
            display.setDrawColor(1);
            display.drawBox(box_x, box_y, total_width, total_height);
            
            // Wyświetl tekst w negatywie
            display.setDrawColor(0);
            char levelStr[2];
            sprintf(levelStr, "%d", assistLevel);
            display.drawStr(x, y, levelStr);
            display.setDrawColor(1);
        } else {
            // Normalny tryb
            char levelStr[2];
            sprintf(levelStr, "%d", assistLevel);
            display.drawStr(2, 40, levelStr);
        }
    }    

    display.setFont(czcionka_mala);
    const char* modeText = "";  // Domyślna wartość
    const char* modeText2 = ""; // Domyślna wartość
    
    // Sprawdź czy jest aktywna kadencja (pedałowanie)
    unsigned long now = millis();
    bool isPedaling = (now - cadence_last_pulse_time < 2000) && (cadence_rpm > 0);
    
    // Ustawienie trybu PAS gdy jest kadencja
    if (isPedaling) {
        modeText = "PAS";
    }
    
    // Wyświetl STOP gdy hamulec jest aktywny
    if (brakeActive) {
        modeText2 = "STOP";
    }
    
    display.drawStr(28, 23, modeText);  // wyświetl PAS gdy pedałowanie aktywne
    display.drawStr(28, 34, modeText2);  // wyświetl STOP przy aktywnym hamulcu
}

// wyświetlanie wartości i jednostki
void drawValueAndUnit(const char* valueStr, const char* unitStr) {
    // Najpierw oblicz szerokość jednostki małą czcionką
    display.setFont(czcionka_mala);
    int unitWidth = display.getStrWidth(unitStr);
    
    // Następnie oblicz szerokość wartości średnią czcionką
    display.setFont(czcionka_srednia);
    int valueWidth = display.getStrWidth(valueStr);
    
    // Całkowita szerokość = wartość + jednostka
    int totalWidth = valueWidth + unitWidth;
    
    // Pozycja początkowa dla wartości (od prawej strony)
    int xPosValue = 128 - totalWidth;
    
    // Pozycja początkowa dla jednostki
    int xPosUnit = xPosValue + valueWidth;
    
    // Rysowanie wartości średnią czcionką
    display.setFont(czcionka_srednia);
    display.drawStr(xPosValue, 62, valueStr);
    
    // Rysowanie jednostki małą czcionką
    display.setFont(czcionka_mala);
    display.drawStr(xPosUnit, 62, unitStr);
}

// --- Funkcje rysujące ---
void drawUpArrow() {
    display.drawTriangle(56, 23, 61, 15, 66, 23);
}

void drawCircleIcon() {
    display.drawCircle(61, 25, 6);
}

void drawDownArrow() {
    display.drawTriangle(56, 35, 61, 43, 66, 35);
}

// logika kadencji
void updateCadenceLogic() {

    // Sprawdź stan hamulca - aktywny gdy pin jest w stanie niskim (LOW)
    brakeActive = !digitalRead(BRAKE_SENSOR_PIN);

    // Wyłącz tempomat jeśli hamulec jest aktywny
    if (brakeActive && cruiseControlActive) {
        cruiseControlActive = false;
        DEBUG_INFO("Dezaktywacja tempomatu przez hamulec");
    }

    unsigned long now = millis();
    static unsigned long lastStateChange = 0;
    const unsigned long MIN_STATE_DURATION = 500; // Minimalny czas (ms) utrzymania tego samego stanu
    
    bool cadenceActive = (now - cadence_last_pulse_time < 2000);
    
    if (!cadenceActive) {
        // Jeśli kadencja nie jest aktywna, nie pokazujemy żadnych strzałek
        cadence_arrow_state = ARROW_NONE;
        return;
    }
    
    // Sprawdź czy minimalny czas utrzymania stanu upłynął
    if (now - lastStateChange < MIN_STATE_DURATION) {
        return; // Zbyt wcześnie na zmianę stanu
    }
    
    // Aktualizuj stan strzałek tylko gdy kadencja jest aktywna i minął minimalny czas
    CadenceArrow oldState = cadence_arrow_state;
    
    switch (cadence_arrow_state) {
        case ARROW_NONE:
            // ZMIANA: Przy niskiej kadencji pokazuj strzałkę w dół (sugerując niższy bieg)
            if (cadence_rpm > 0 && cadence_rpm < CADENCE_OPTIMAL_MIN)
                cadence_arrow_state = ARROW_DOWN;
            // ZMIANA: Przy wysokiej kadencji pokazuj strzałkę w górę (sugerując wyższy bieg)
            else if (cadence_rpm > CADENCE_OPTIMAL_MAX)
                cadence_arrow_state = ARROW_UP;
            break;
        case ARROW_DOWN: // ZMIANA: Był ARROW_UP
            if (cadence_rpm >= CADENCE_OPTIMAL_MIN + CADENCE_HYSTERESIS)
                cadence_arrow_state = ARROW_NONE;
            break;
        case ARROW_UP: // ZMIANA: Był ARROW_DOWN
            if (cadence_rpm <= CADENCE_OPTIMAL_MAX - CADENCE_HYSTERESIS)
                cadence_arrow_state = ARROW_NONE;
            break;
    }
    
    // Jeśli zmieniliśmy stan, zapisz czas zmiany
    if (cadence_arrow_state != oldState) {
        lastStateChange = now;
    }
}

// Implementacja głównego ekranu
void drawMainDisplay() {
    display.setFont(czcionka_mala);
    char valueStr[10];
    const char* unitStr;
    const char* descText;

    if (inSubScreen) {
        switch (currentMainScreen) {
            case RANGE_SCREEN: // Teraz pierwszy ekran
                switch (currentSubScreen) {
                    case ODOMETER_KM: // Nowa kolejność
                        sprintf(valueStr, "%4.0f", odometer.getRawTotal());
                        unitStr = "km";
                        descText = ">Przebieg";
                        break;
                    case DISTANCE_KM:
                        sprintf(valueStr, "%4.1f", distance_km);
                        unitStr = "km";
                        descText = ">Dystans";
                        break;
                    case RANGE_KM:
                        sprintf(valueStr, "%4.1f", range_km);
                        unitStr = "km";
                        descText = ">Zasieg";
                        break;
                }
                break;

            case CADENCE_SCREEN: // Teraz drugi ekran
                switch (currentSubScreen) {
                    case CADENCE_RPM:
                        sprintf(valueStr, "%4d", cadence_rpm);
                        unitStr = "RPM";
                        descText = ">Kadencja";
                        break;
                    case CADENCE_AVG_RPM:
                        sprintf(valueStr, "%4d", cadence_avg_rpm);
                        unitStr = "RPM";
                        descText = ">Kadencja AVG";
                        break;
                    case CADENCE_MAX_RPM: 
                        sprintf(valueStr, "%4d", cadence_max_rpm);
                        unitStr = "RPM";
                        descText = ">Kadencja MAX";
                        break;
                }
                break;

            case SPEED_SCREEN: // Teraz trzeci ekran
                switch (currentSubScreen) {
                    case SPEED_KMH:
                        sprintf(valueStr, "%4.1f", speed_kmh);
                        unitStr = "km/h";
                        descText = ">Predkosc";
                        break;
                    case SPEED_AVG_KMH:
                        sprintf(valueStr, "%4.1f", speed_avg_kmh);
                        unitStr = "km/h";
                        descText = ">Pred. AVG";
                        break;
                    case SPEED_MAX_KMH:
                        sprintf(valueStr, "%4.1f", speed_max_kmh);
                        unitStr = "km/h";
                        descText = ">Pred. MAX";
                        break;
                }
                break;

            case POWER_SCREEN: // Teraz czwarty ekran
                switch (currentSubScreen) {
                    case POWER_W:
                        sprintf(valueStr, "%4d", power_w);
                        unitStr = "W";
                        descText = ">Moc";
                        break;
                    case POWER_AVG_W:
                        sprintf(valueStr, "%4d", power_avg_w);
                        unitStr = "W";
                        descText = ">Moc AVG";
                        break;
                    case POWER_MAX_W:
                        sprintf(valueStr, "%4d", power_max_w);
                        unitStr = "W";
                        descText = ">Moc MAX";
                        break;
                }
                break;

            case BATTERY_SCREEN: // Teraz piąty ekran z nową kolejnością podekranów
                switch (currentSubScreen) {
                    case BATTERY_CAPACITY_AH:
                        sprintf(valueStr, "%4.1f", battery_capacity_ah);
                        unitStr = "Ah";
                        descText = ">Pojemnosc";
                        break;
                    case BATTERY_CAPACITY_WH:
                        sprintf(valueStr, "%4.0f", battery_capacity_wh);
                        unitStr = "Wh";
                        descText = ">Energia";
                        break;
                    case BATTERY_CAPACITY_PERCENT:
                        sprintf(valueStr, "%3d", battery_capacity_percent);
                        unitStr = "%";
                        descText = ">Bateria";
                        break;
                    case BATTERY_VOLTAGE:
                        sprintf(valueStr, "%4.1f", battery_voltage);
                        unitStr = "V";
                        descText = ">Napiecie";
                        break;
                    case BATTERY_CURRENT:
                        sprintf(valueStr, "%4.1f", battery_current);
                        unitStr = "A";
                        descText = ">Natezenie";
                        break;
                }
                break;

            case TEMP_SCREEN: // Teraz szósty ekran
                switch (currentSubScreen) {
                    case TEMP_AIR:
                        if (currentTemp != TEMP_ERROR && currentTemp != DEVICE_DISCONNECTED_C) {
                            sprintf(valueStr, "%4.1f", currentTemp);
                        } else {
                            strcpy(valueStr, "---");
                        }
                        unitStr = "C";
                        descText = ">Powietrze";
                        break;
                    case TEMP_CONTROLLER:
                        sprintf(valueStr, "%4.1f", temp_controller);
                        unitStr = "C";
                        descText = ">Sterownik";
                        break;
                    case TEMP_MOTOR:
                        sprintf(valueStr, "%4.1f", temp_motor);
                        unitStr = "C";
                        descText = ">Silnik";
                        break;
                }
                break;

            case PRESSURE_SCREEN: // Teraz siódmy ekran
                char combinedStr[16];
                switch (currentSubScreen) {
                    case PRESSURE_BAR:
                        if (frontTpms.isActive && rearTpms.isActive) {
                            sprintf(combinedStr, "%.2f|%.2f", pressure_bar, pressure_rear_bar);
                        } else if (frontTpms.isActive) {
                            sprintf(combinedStr, "%.2f|---", pressure_bar);
                        } else if (rearTpms.isActive) {
                            sprintf(combinedStr, "---|%.2f", pressure_rear_bar);
                        } else {
                            strcpy(combinedStr, "---|---");
                        }
                        strcpy(valueStr, combinedStr);
                        unitStr = "bar";
                        descText = ">Cis";
                        break;
                    case PRESSURE_VOLTAGE:
                        sprintf(combinedStr, "%.2f|%.2f", pressure_voltage, pressure_rear_voltage);
                        strcpy(valueStr, combinedStr);
                        unitStr = "V";
                        descText = ">Bat";
                        break;
                    case PRESSURE_TEMP:
                        sprintf(combinedStr, "%.1f|%.1f", pressure_temp, pressure_rear_temp);
                        strcpy(valueStr, combinedStr);
                        unitStr = "C";
                        descText = ">Temp";
                        break;
                }
                break;
        }

    } else {
        // Wyświetlanie głównych ekranów
        switch (currentMainScreen) {
            case RANGE_SCREEN: // Teraz pierwszy ekran
                sprintf(valueStr, "%4.0f", odometer.getRawTotal());
                unitStr = "km";
                descText = " Przebieg";
                break;

            case CADENCE_SCREEN: // Teraz drugi ekran
                sprintf(valueStr, "%4d", cadence_rpm);
                unitStr = "RPM";
                descText = " Kadencja";
                break;  

            case SPEED_SCREEN: // Teraz trzeci ekran
                sprintf(valueStr, "%4.1f", speed_kmh);
                unitStr = "km/h";
                descText = " Predkosc";
                break;

            case POWER_SCREEN: // Teraz czwarty ekran
                sprintf(valueStr, "%4d", power_w);
                unitStr = "W";
                descText = " Moc";
                break;

            case BATTERY_SCREEN: // Teraz piąty ekran
                sprintf(valueStr, "%4.1f", battery_capacity_ah);
                unitStr = "Ah";
                descText = " Bateria";
                break;

            case TEMP_SCREEN: // Teraz szósty ekran
                if (currentTemp != TEMP_ERROR && currentTemp != DEVICE_DISCONNECTED_C) {
                    sprintf(valueStr, "%4.1f", currentTemp);
                } else {
                    strcpy(valueStr, "---");
                }
                unitStr = "C";
                descText = " Temperatura";
                break;

            case PRESSURE_SCREEN: // Teraz siódmy ekran
                sprintf(valueStr, "%.1f/%.1f", pressure_bar, pressure_rear_bar);
                unitStr = "bar";
                descText = " Kola";
                break;

            case USB_SCREEN: // Teraz ósmy ekran
                display.setFont(czcionka_srednia);
                display.drawStr(48, 61, usbEnabled ? "Wlaczone" : "Wylaczone");
                descText = " USB";
                break;
        }
    }
   
    char speedStr[10]; // Bufor na sformatowaną prędkość
    if (speed_kmh < 10.0) {
        sprintf(speedStr, "  %2.1f", speed_kmh);  // Dodaj spację przed liczbą
    } else {
        sprintf(speedStr, "%2.1f", speed_kmh);   // Bez spacji
    }

    // Wyświetl prędkość dużą czcionką
    display.setFont(czcionka_duza);
    display.drawStr(72, 35, speedStr);

    // Wyświetl jednostkę małą czcionką pod prędkością
    display.setFont(czcionka_mala);
    display.drawStr(105, 45, "km/h");

    // Wyświetl wartości tylko jeśli nie jesteśmy na ekranie USB
    if (currentMainScreen != USB_SCREEN) {
        drawValueAndUnit(valueStr, unitStr);
    }

    display.setFont(czcionka_mala);
    display.drawStr(0, 62, descText);
}

// rysowanie strzałek
void drawCadenceArrowsAndCircle() {
    unsigned long now = millis();
    
    // Statyczna zmienna przechowująca czas ostatniego wykrycia kadencji
    static unsigned long lastCadenceDetection = 0;
    
    // Jeśli wykryto kadencję, aktualizuj czas
    if (now - cadence_last_pulse_time < 500) { // Kadencja wykryta w ostatnich 500ms
        lastCadenceDetection = now;
    }
    
    // Wyświetlaj strzałki przez 2 sekundy od ostatniego wykrycia kadencji
    bool showArrows = (now - lastCadenceDetection < 2000);
    
    // Tylko jeśli powinniśmy pokazywać strzałki
    if (showArrows) {
        // ZMIANA: Strzałka w dół - pokazuj przy niskiej kadencji (sugerując niższy bieg)
        if (cadence_rpm > 0 && cadence_rpm < CADENCE_OPTIMAL_MIN) {
            drawDownArrow();
        }
        
        // ZMIANA: Strzałka w górę - pokazuj przy wysokiej kadencji (sugerując wyższy bieg)
        else if (cadence_rpm > CADENCE_OPTIMAL_MAX) {
            drawUpArrow();
        }
    }
}

// wyświetlanie wycentrowanego tekstu
void drawCenteredText(const char* text, int y, const uint8_t* font) {
    // Ustawia wybraną czcionkę dla tekstu
    display.setFont(font);
    
    // Oblicza szerokość tekstu w pikselach dla wybranej czcionki
    int textWidth = display.getStrWidth(text);
    
    // Oblicza pozycję X dla wycentrowania tekstu
    // display.getWidth() zwraca szerokość wyświetlacza (zazwyczaj 128 pikseli)
    // Odejmujemy szerokość tekstu i dzielimy przez 2, aby uzyskać lewy margines
    int x = (display.getWidth() - textWidth) / 2;
    
    // Rysuje tekst w obliczonej pozycji
    // x - pozycja pozioma (wycentrowana)
    // y - pozycja pionowa (określona przez parametr)
    display.drawStr(x, y, text);
}

// Wyświetlanie komunikatu "Prowadzenie roweru"
void showWalkAssistMode(bool sendBuffer = false) {
    // Wyczyść bufor tylko jeśli funkcja została wywołana samodzielnie
    if (sendBuffer) {
        display.clearBuffer();
        drawTopBar();
        drawHorizontalLine();
        drawVerticalLine();
        drawAssistLevel();
    }
    
    // Wyświetl duży napis na środku ekranu
    drawCenteredText("Prowadzenie", 32, czcionka_srednia);
    drawCenteredText("roweru", 45, czcionka_srednia);
    
    // Wysyłamy bufor tylko jeśli funkcja została wywołana samodzielnie
    if (sendBuffer) {
        display.sendBuffer();
    }
}

// wyświetlanie wiadomości powitalnej
void showWelcomeMessage() {
    display.clearBuffer();
    display.setFont(czcionka_srednia); // Ustaw domyślną czcionkę na początku

    // Tekst "Witaj!" na środku
    String welcomeText = "Witaj!";
    int welcomeWidth = display.getStrWidth(welcomeText.c_str());
    int welcomeX = (128 - welcomeWidth) / 2;

    // Tekst przewijany
    String scrollText = "e-Bike System PMW  ";
    int messageWidth = display.getStrWidth(scrollText.c_str());
    int x = 128; // Start poza prawą krawędzią

    // Przygotuj tekst wersji
    String versionText = "System ";
    versionText += VERSION;

    unsigned long lastUpdate = millis();
    while (x > -messageWidth) { // Przewijaj aż tekst zniknie z lewej strony
        unsigned long currentMillis = millis();
        if (currentMillis - lastUpdate >= 2) {
            display.clearBuffer();
            
            // Statyczny tekst "Witaj!" dużą czcionką
            display.setFont(czcionka_duza);
            display.drawStr(welcomeX, 20, welcomeText.c_str());
            
            // Przewijany tekst średnią czcionką
            display.setFont(czcionka_srednia);
            display.drawStr(x, 43, scrollText.c_str());

            // Tekst wersji małą czcionką
            display.setFont(czcionka_mala);
            int versionWidth = display.getStrWidth(versionText.c_str());
            int versionX = (128 - versionWidth) / 2;
            display.drawStr(versionX, 60, versionText.c_str());
            
            display.sendBuffer();
            
            //x--; // jeden px na krok
            x -= 2; // dwa px na krok
            lastUpdate = currentMillis;
        }
    }

    welcomeAnimationDone = true;
}

// wyświetlanie komunikatu kasowania danych
void clearTripData() {
    display.clearBuffer();

    drawCenteredText("Reset danych", 25, czcionka_srednia);
    drawCenteredText("przejazdu", 40, czcionka_srednia);

    display.sendBuffer();
    delay(1500);

    display.clearBuffer();
    display.sendBuffer();
}

// --- Funkcje obsługi przycisków ---

// obsługa przycisków
void handleButtons() {

    if (configModeActive) {
        return; // W trybie konfiguracji nie obsługuj normalnych funkcji przycisków
    }

    unsigned long currentTime = millis();
    bool setState = digitalRead(BTN_SET);
    bool upState = digitalRead(BTN_UP);
    bool downState = digitalRead(BTN_DOWN);

    // Wywołuj updateActivityTime() TYLKO gdy naprawdę jest wciśnięty jakiś przycisk
    if (!setState || !upState || !downState) {
        updateActivityTime(); // Aktualizuj czas aktywności TYLKO gdy przycisk jest wciśnięty
    }

    static unsigned long lastButtonCheck = 0;
    const unsigned long buttonDebounce = 50;
    static unsigned long legalModeStart = 0;
    static unsigned long resetTripStart = 0;

    // Sprawdź najpierw czy nie jesteśmy w czasie blokady po specjalnej kombinacji
    if (currentTime - lastSpecialComboTime < SPECIAL_COMBO_LOCKOUT) {
        // Jesteśmy w okresie blokady po kombinacji klawiszy - ignorujemy wszystkie klawisze
        // Resetujemy też stany, aby uniknąć niepożądanych akcji
        upPressStartTime = 0;
        downPressStartTime = 0;
        setPressStartTime = 0;
        upLongPressExecuted = false;
        downLongPressExecuted = false;
        setLongPressExecuted = false;
        return;
    }

    // Obsługa włączania/wyłączania wyświetlacza
    if (!displayActive) {
        if (!setState && (currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
            if (!setPressStartTime) {
                setPressStartTime = currentTime;
            } else if (!setLongPressExecuted && (currentTime - setPressStartTime) > SET_LONG_PRESS) {
                if (!welcomeAnimationDone) {
                    showWelcomeMessage();  // Pokaż animację powitania
                } 
                messageStartTime = currentTime;
                setLongPressExecuted = true;
                showingWelcome = true;
                displayActive = true;
            }
        } else if (setState && setPressStartTime) {
            setPressStartTime = 0;
            setLongPressExecuted = false;
            lastDebounceTime = currentTime;
        }
        return;
    }

    // Obsługa przycisków gdy wyświetlacz jest aktywny
    if (!showingWelcome) {

        // Sprawdzanie trybu legal (UP + SET) - przełączanie trybu legalnego
        if (displayActive && !showingWelcome && !upState && !setState) {
            if (legalModeStart == 0) {
                legalModeStart = currentTime;
            } else if ((currentTime - legalModeStart) > 500) { // 0.5 sekundy przytrzymania
                toggleLegalMode();
                while (!digitalRead(BTN_UP) || !digitalRead(BTN_SET)) {
                    delay(10); // Czekaj na puszczenie przycisków
                }
                legalModeStart = 0;
                lastDebounceTime = currentTime;
                
                // Ustaw czas ostatniej kombinacji
                lastSpecialComboTime = currentTime;
                
                // Reset wszystkich stanów przycisków
                upPressStartTime = 0;
                downPressStartTime = 0;
                setPressStartTime = 0;
                upLongPressExecuted = false;
                downLongPressExecuted = false;
                setLongPressExecuted = false;
                
                return; // Zakończ funkcję handleButtons po obsłudze kombinacji SET+UP
            }
        } else {
            legalModeStart = 0;
        }

        // Sprawdzanie kombinacji SET + DOWN - reset danych przejazdu
        if (displayActive && !showingWelcome && !setState && !downState) {
            if (resetTripStart == 0) {
                resetTripStart = currentTime;
            } else if ((currentTime - resetTripStart) > 500) { // 0.5 sekundy przytrzymania
                resetTripData();                
                clearTripData();
                
                // Czekaj na puszczenie przycisków
                while (!digitalRead(BTN_DOWN) || !digitalRead(BTN_SET)) {
                    delay(10);
                }
                resetTripStart = 0;
                lastDebounceTime = currentTime;
                
                // Ustaw czas ostatniej kombinacji
                lastSpecialComboTime = currentTime;
                
                // Reset wszystkich stanów przycisków
                upPressStartTime = 0;
                downPressStartTime = 0;
                setPressStartTime = 0;
                upLongPressExecuted = false;
                downLongPressExecuted = false;
                setLongPressExecuted = false;
                
                return; // Zakończ funkcję handleButtons po obsłudze kombinacji SET+DOWN
            }
        } else {
            resetTripStart = 0;
        }

        if (!upState && (currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
            if (!upPressStartTime) {
                upPressStartTime = currentTime;
            } else if (!upLongPressExecuted && (currentTime - upPressStartTime) > LONG_PRESS_TIME) {
                // Długie naciśnięcie przycisku UP - sterowanie światłami
                if (lightManager.getControlMode() == LightManager::SMART_CONTROL) {
                    // Cykliczne przełączanie: dzień -> noc -> wyłączone
                    lightManager.cycleMode();
                    applyBacklightSettings();
                } else {
                    // Tryb sterownika: włącz/wyłącz światła
                    if (lightManager.getMode() == LightManager::OFF) {
                        // Włącz światła (wysyłając komendę UART do sterownika)
                        lightManager.setMode(LightManager::DAY); // Tymczasowo ustawiam DAY jako "włączone" w trybie sterownika
                        // TODO: Dodać wysyłanie komendy UART do sterownika KT
                        applyBacklightSettings();
                    } else {
                        // Wyłącz światła
                        lightManager.setMode(LightManager::OFF);
                        // TODO: Dodać wysyłanie komendy UART do sterownika KT
                        applyBacklightSettings();
                    }
                }
                upLongPressExecuted = true;
            }
        } else if (upState && upPressStartTime) {
            if (!upLongPressExecuted && (currentTime - upPressStartTime) < LONG_PRESS_TIME) {
                if (assistLevel < 5) assistLevel++;
            }
            upPressStartTime = 0;
            upLongPressExecuted = false;
            lastDebounceTime = currentTime;
        }

        // Obsługa przycisku DOWN (zmiana asysty/tryb prowadzenia/tempomat)
        if (!downState && (currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
            if (!downPressStartTime) {
                downPressStartTime = currentTime;
            } else if (!downLongPressExecuted && (currentTime - downPressStartTime) > LONG_PRESS_TIME) {
                // Długie przytrzymanie przycisku DOWN
                if (speed_kmh >= 10.0) {
                    // Prędkość >= 10 km/h - włącz tempomat
                    cruiseControlActive = !cruiseControlActive; // Przełącz stan tempomatu
                    assistLevelAsText = cruiseControlActive; // Pokaż "T" gdy tempomat aktywny
                    
                    if (cruiseControlActive) {
                        DEBUG_INFO("Aktywacja tempomatu");
                    } else {
                        DEBUG_INFO("Dezaktywacja tempomatu");
                    }
                    
                    downLongPressExecuted = true;
                } else if (speed_kmh < 8.0) {
                    // Prędkość < 8 km/h - włącz tryb prowadzenia roweru
                    walkAssistActive = true;
                    showWalkAssistMode(true);  // Wyślij bufor, żeby od razu pokazać ekran
                    
                    DEBUG_INFO("Aktywacja trybu prowadzenia roweru");
                    
                    downLongPressExecuted = true;
                }
            }
        } else if (downState && downPressStartTime) {
            // Przycisk puszczony
            if (walkAssistActive) {
                // Wyłącz tryb prowadzenia roweru gdy przycisk DOWN jest puszczony
                walkAssistActive = false;
                DEBUG_INFO("Dezaktywacja trybu prowadzenia roweru");
            }
            
            // UWAGA: Usunięto wyłączanie tempomatu przyciskiem DOWN - teraz tylko hamulec może go wyłączyć
            
            if (!downLongPressExecuted && (currentTime - downPressStartTime) < LONG_PRESS_TIME) {
                // Krótkie kliknięcie DOWN - zmniejszenie asysty
                if (assistLevel > 0) assistLevel--;
            }
            
            downPressStartTime = 0;
            downLongPressExecuted = false;
            lastDebounceTime = currentTime;
        }

        // Obsługa przycisku SET
        static unsigned long lastSetRelease = 0;
        static bool waitingForSecondClick = false;

        if (!setState) {  // Przycisk wciśnięty
            if (!setPressStartTime) {
                setPressStartTime = currentTime;
            } else if (!setLongPressExecuted && (currentTime - setPressStartTime) > SET_LONG_PRESS) {
                // Długie przytrzymanie (>3s) - wyłączenie
                display.clearBuffer();
                display.setFont(czcionka_srednia);
                display.drawStr(5, 32, "Do widzenia ;)");
                display.sendBuffer();
                messageStartTime = currentTime;
                setLongPressExecuted = true;
            }
        } else if (setPressStartTime) {  // Przycisk puszczony
            if (!setLongPressExecuted) {
                unsigned long releaseTime = currentTime;

                if (waitingForSecondClick && (releaseTime - lastSetRelease) < DOUBLE_CLICK_TIME) {
                    // Podwójne kliknięcie
                    if (currentMainScreen == USB_SCREEN) {
                        // Przełącz stan USB
                        usbEnabled = !usbEnabled;
                        digitalWrite(UsbPin, usbEnabled ? HIGH : LOW);
                    } else if (inSubScreen) {
                        inSubScreen = false;  // Wyjście z pod-ekranów
                    } else if (hasSubScreens(currentMainScreen)) {
                        inSubScreen = true;  // Wejście do pod-ekranów
                        currentSubScreen = 0;
                    }
                    waitingForSecondClick = false;
                } else {
                    // Pojedyncze kliknięcie
                    if (!waitingForSecondClick) {
                        waitingForSecondClick = true;
                        lastSetRelease = releaseTime;
                    } else if ((releaseTime - lastSetRelease) >= DOUBLE_CLICK_TIME) {
                        // Przełączanie ekranów/pod-ekranów
                        if (inSubScreen) {
                            currentSubScreen = (currentSubScreen + 1) % getSubScreenCount(currentMainScreen);
                        } else {
                            currentMainScreen = (MainScreen)((currentMainScreen + 1) % MAIN_SCREEN_COUNT);
                        }
                        waitingForSecondClick = false;
                    }
                }
            }
            setPressStartTime = 0;
            setLongPressExecuted = false;
            lastDebounceTime = currentTime;
        }

        // Reset flagi oczekiwania na drugie kliknięcie po upływie czasu
        if (waitingForSecondClick && (currentTime - lastSetRelease) >= DOUBLE_CLICK_TIME) {
            // Wykonaj akcję pojedynczego kliknięcia
            if (inSubScreen) {
                currentSubScreen = (currentSubScreen + 1) % getSubScreenCount(currentMainScreen);
            } else {
                currentMainScreen = (MainScreen)((currentMainScreen + 1) % MAIN_SCREEN_COUNT);
            }
            waitingForSecondClick = false;
        }
    }

    // Obsługa komunikatów powitalnych/pożegnalnych
    if (messageStartTime > 0 && (currentTime - messageStartTime) >= GOODBYE_DELAY) {
        if (!showingWelcome) {
            displayActive = false;
            goToSleep();
        }
        messageStartTime = 0;
        showingWelcome = false;
    }
}

// sprawdzanie trybu konfiguracji
void checkConfigMode() {
    // Dodaj sprawdzenie czy wyświetlacz jest aktywny
    if (!displayActive) {
        return; // Jeśli system jest wyłączony, nie aktywuj trybu konfiguracji
    }

    static unsigned long upDownPressTime = 0;
    static bool bothButtonsPressed = false;

    if (!digitalRead(BTN_UP) && !digitalRead(BTN_DOWN)) {
        if (!bothButtonsPressed) {
            bothButtonsPressed = true;
            upDownPressTime = millis();
        } else if ((millis() - upDownPressTime > 500) && !configModeActive) {
            activateConfigMode();
        }
    } else {
        bothButtonsPressed = false;
    }
}

// Implementacja aktywacji trybu konfiguracji
void activateConfigMode() {
    configModeActive = true;

    // Wyłącz wszystkie światła przy wejściu w tryb konfiguracji
    digitalWrite(FrontDayPin, LOW);
    digitalWrite(FrontPin, LOW);
    digitalWrite(RearPin, LOW);
    //lightMode = 0; // Wymuś tryb świateł na "wyłączone"
    lightManager.setMode(LightManager::OFF);

    // 1. Inicjalizacja LittleFS
    if (!LittleFS.begin(true)) {
        DEBUG_INFO("Błąd montowania LittleFS");
        return;
    }
    DEBUG_INFO("LittleFS zainicjalizowany");

    // 2. Włączenie WiFi w trybie AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP("e-Bike System PMW", "#mamrower");
    DEBUG_INFO("Tryb AP aktywny");
    
    // 3. Konfiguracja serwera - najpierw pliki statyczne
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    
    // Dodanie obsługi typów MIME
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/style.css", "text/css");
    });
    
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/script.js", "application/javascript");
    });

    server.on("/api/reset-filesystem", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool success = false;
        
        DEBUG_INFO("Próba resetowania systemu plików");
        
        if (LittleFS.format()) {
            if (LittleFS.begin(false)) {
                success = true;
                DEBUG_INFO("System plików zresetowany pomyślnie");
            } else {
                DEBUG_INFO("Nie udało się zamontować systemu plików po formatowaniu");
            }
        } else {
            DEBUG_INFO("Formatowanie systemu plików nie powiodło się");
        }
        
        StaticJsonDocument<128> doc;
        doc["success"] = success;
        doc["message"] = success ? "System plikow zresetowany pomyslnie" : "Blad resetowania systemu plikow";
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/api/version", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<200> doc;
        doc["version"] = VERSION;
        
        String response;
        serializeJson(doc, response);
        
        request->send(200, "application/json", response);
    });

    // 4. Dodanie endpointów API
    setupWebServer();

    // 5. Uruchomienie serwera
    server.begin();
    DEBUG_INFO("Serwer WWW uruchomiony");
}

// dezaktywacja trybu konfiguracji
void deactivateConfigMode() {    
    if (!configModeActive) return;  // Jeśli tryb konfiguracji nie jest aktywny, przerwij
    server.end();                   // Zatrzymaj serwer HTTP
    WiFi.softAPdisconnect(true);    // Wyłącz punkt dostępowy WiFi
    WiFi.mode(WIFI_OFF);            // Wyłącz moduł WiFi
    LittleFS.end();                 // Odmontuj system plików
    
    configModeActive = false;
    
    display.clearBuffer();
    display.sendBuffer();
}

// przełączanie trybu legal
void toggleLegalMode() {
    legalMode = !legalMode;
    
    display.clearBuffer();
        
    drawCenteredText("Tryb legalny", 20, czcionka_srednia);
    drawCenteredText("zostal", 35, czcionka_srednia);
    drawCenteredText(legalMode ? "wlaczony" : "wylaczony", 50, czcionka_srednia);
    
    display.sendBuffer();
    delay(1500);
    
    display.clearBuffer();
    display.sendBuffer();
}

// --- Funkcje pomocnicze ---

// sprawdzanie pod-ekranów
bool hasSubScreens(MainScreen screen) {
    switch (screen) {
        case SPEED_SCREEN: return SPEED_SUB_COUNT > 1;
        case CADENCE_SCREEN: return CADENCE_SUB_COUNT > 1;
        case TEMP_SCREEN: return TEMP_SUB_COUNT > 1;
        case RANGE_SCREEN: return RANGE_SUB_COUNT > 1;
        case BATTERY_SCREEN: return BATTERY_SUB_COUNT > 1;
        case POWER_SCREEN: return POWER_SUB_COUNT > 1;
        case PRESSURE_SCREEN: return PRESSURE_SUB_COUNT > 1;
        case USB_SCREEN: return false;
        default: return false;
    }
}

// liczenie pod-ekranów
int getSubScreenCount(MainScreen screen) {
    switch (screen) {
        case SPEED_SCREEN: return SPEED_SUB_COUNT;
        case CADENCE_SCREEN: return CADENCE_SUB_COUNT;
        case TEMP_SCREEN: return TEMP_SUB_COUNT;
        case RANGE_SCREEN: return RANGE_SUB_COUNT;
        case BATTERY_SCREEN: return BATTERY_SUB_COUNT;
        case POWER_SCREEN: return POWER_SUB_COUNT;
        case PRESSURE_SCREEN: return PRESSURE_SUB_COUNT;
        default: return 0;
    }
}

void resetTripData() {
    speed_avg_kmh = 0;
    speed_max_kmh = 0;
    distance_km = 0;
    cadence_avg_rpm = 0;
    cadence_max_rpm = 0;
    cadence_sum = 0;
    cadence_samples = 0;
    power_avg_w = 0;
    power_max_w = 0;
    
    // Zresetuj liczniki używane do obliczania średnich
    speed_sum = 0;
    speed_count = 0;
    
    DEBUG_INFO("Zresetowano dane przejazdu");
}

//
void setCadencePulsesPerRevolution(uint8_t pulses) {
    if (pulses >= 1 && pulses <= 24) {
        cadence_pulses_per_revolution = pulses;
        preferences.begin("cadence", false);
        preferences.putUChar("pulses", pulses);
        preferences.end();
        DEBUG_INFO("Ustawiono %d impulsow na obrot korby", pulses);
    } else {
        DEBUG_ERROR("Bledna wartosc dla impulsow na obrot (dozwolony zakres: 1-24)");
    }
}

// tryb uśpienia
void goToSleep() {
    DEBUG_INFO("Wchodze w tryb glebokiego uspienia (DEEP SLEEP)...");
    //DEBUG_INFO("Aktualny tryb swiatel: %d", (int)lightManager.getMode());

    // Wyłącz wszystkie LEDy
    digitalWrite(FrontDayPin, LOW);
    digitalWrite(FrontPin, LOW);
    digitalWrite(RearPin, LOW);
    digitalWrite(UsbPin, LOW);

    delay(50);

    // Wyłącz OLED
    display.clearBuffer();
    display.sendBuffer();
    display.setPowerSave(1);  // Wprowadź OLED w tryb oszczędzania energii

    // Zapisz stan trybu świateł przed uśpieniem
    // Już nie jest potrzebne - LightManager zapisuje stan automatycznie

    //DEBUG_INFO("Konfiguracja wybudzania przez przycisk SET (GPIO12)");
    DEBUG_INFO("Przechodze do DEEP SLEEP. Do zobaczenia po wybudzeniu!");
    Serial.flush(); // Upewnij się, że wszystkie dane zostały wysłane

    // Konfiguracja wybudzania przez przycisk SET
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, 0);  // GPIO12 (BTN_SET) stan niski

    // Wejście w deep sleep
    esp_deep_sleep_start();
}

void applyBacklightSettings() {
    // Wartości domyślne na wypadek niezainicjalizowanej struktury
    int targetBrightness = 70; // domyślna jasność
    
    DEBUG_LIGHT("Aktualizacja podswietlenia:");
    DEBUG_LIGHT("  Tryb automatyczny: %s", backlightSettings.autoMode ? "TAK" : "NIE");
    DEBUG_LIGHT("  Tryb swiatel: %s", lightManager.getModeString());
    DEBUG_LIGHT("  Tryb sterowania: %s", lightManager.getControlMode() == LightManager::SMART_CONTROL ? "SMART" : "STEROWNIK");
    
    if (!backlightSettings.autoMode) {
        // Tryb manualny - użyj podstawowej jasności
        targetBrightness = backlightSettings.Brightness > 0 ? backlightSettings.Brightness : 70;
        DEBUG_LIGHT("  Tryb manualny, jasnosc: %d", targetBrightness);
    } else {
        // Tryb auto - sprawdź tryb sterowania i stan świateł
        if (lightManager.getControlMode() == LightManager::SMART_CONTROL) {
            // W trybie SMART
            if (lightManager.getMode() == LightManager::NIGHT) {
                // Tryb NOC
                targetBrightness = backlightSettings.nightBrightness > 0 ? backlightSettings.nightBrightness : 50;
                DEBUG_LIGHT("  Tryb automatyczny/SMART/NOC, jasnosc: %d", targetBrightness);
            } else {
                // Tryb DZIEŃ lub wyłączone
                targetBrightness = backlightSettings.dayBrightness > 0 ? backlightSettings.dayBrightness : 100;
                DEBUG_LIGHT("  Tryb automatyczny/SMART/DZIEN, jasnosc: %d", targetBrightness);
            }
        } else {
            // W trybie STEROWNIK
            if (lightManager.getMode() != LightManager::OFF) {
                // Światła włączone (LAMPY)
                targetBrightness = backlightSettings.nightBrightness > 0 ? backlightSettings.nightBrightness : 50;
                DEBUG_LIGHT("  Tryb automatyczny/STEROWNIK/LAMPY, jasnosc: %d", targetBrightness);
            } else {
                // Światła wyłączone
                targetBrightness = backlightSettings.dayBrightness > 0 ? backlightSettings.dayBrightness : 100;
                DEBUG_LIGHT("  Tryb automatyczny/STEROWNIK/Wylaczone, jasnosc: %d", targetBrightness);
            }
        }
    }
    
    // Zabezpieczenie przed nieprawidłowymi wartościami
    if (targetBrightness < 0) targetBrightness = 0;  // Teraz pozwalamy na 0%
    if (targetBrightness > 100) targetBrightness = 100;
    
    // PROSTE MAPOWANIE LINIOWE: 0-100% -> 0-255
    // Bezpośrednie przełożenie procentu jasności na wartość kontrastu
    displayBrightness = map(targetBrightness, 0, 100, 0, 255);
    
    // Zastosuj jasność do wyświetlacza
    display.setContrast(displayBrightness);
    
    DEBUG_LIGHT("  Zastosowano jasnosc: %d (kontrast: %d)", targetBrightness, displayBrightness);
}

void printLightConfigFile() {
    if (!LittleFS.begin(false)) {
        DEBUG_ERROR("Blad montowania systemu plikow");
        return;
    }
    
    if (LittleFS.exists("/light_config.json")) {
        File file = LittleFS.open("/light_config.json", "r");
        if (file) {
            DEBUG_INFO("=== ZAWARTOSC PLIKU KONFIGURACYJNEGO SWIATEL ===");
            while (file.available()) {
                Serial.write(file.read());
            }

            file.close();
        } else {
            DEBUG_INFO("Nie można otworzyc pliku konfiguracyjnego");
        }
    } else {
        DEBUG_ERROR("Plik konfiguracyjny /light_config.json nie istnieje");
    }
    
    // Sprawdź też stary plik
    if (LittleFS.exists("/lights.json")) {
        File file = LittleFS.open("/lights.json", "r");
        if (file) {
            DEBUG_INFO("=== ZAWARTOSC STAREGO PLIKU KONFIGURACYJNEGO ===");
            while (file.available()) {
                Serial.write(file.read());
            }

            file.close();
        }
    }
}

// sprawdzanie poprawności temperatury
bool isValidTemperature(float temp) {
    return (temp >= -50.0 && temp <= 100.0);
}

// obsługa temperatury
void handleTemperature() {
    unsigned long currentMillis = millis();

    if (!conversionRequested && (currentMillis - lastTempRequest >= TEMP_REQUEST_INTERVAL)) {
        // Żądanie konwersji z obu czujników
        sensorsAir.requestTemperatures();
        sensorsController.requestTemperatures();
        conversionRequested = true;
        lastTempRequest = currentMillis;
    }

    if (conversionRequested && (currentMillis - lastTempRequest >= 750)) {
        // Odczyt z obu czujników
        currentTemp = sensorsAir.getTempCByIndex(0);
        temp_controller = sensorsController.getTempCByIndex(0);
        conversionRequested = false;
    }
}

// --- Funkcje konfiguracji systemu ---

// wczytywanie wszystkich ustawień
void loadSettings() {
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        DEBUG_INFO("Nie udalo sie otworzyc pliku konfiguracyjnego");
        return;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, configFile);

    if (error) {
        DEBUG_INFO("Nie udalo sie przetworzyc pliku konfiguracyjnego");
        return;
    }

    // Wczytywanie ustawień świateł - teraz to obsługuje LightManager
    // Nie musimy już ładować lightSettings z pliku

    // Wczytywanie ustawień podświetlenia
    if (doc.containsKey("backlight")) {
        backlightSettings.dayBrightness = doc["backlight"]["dayBrightness"] | 100;
        backlightSettings.nightBrightness = doc["backlight"]["nightBrightness"] | 50;
        backlightSettings.autoMode = doc["backlight"]["autoMode"] | false;
    }

    // Wczytywanie ustawień WiFi
    if (doc.containsKey("wifi")) {
        strlcpy(wifiSettings.ssid, doc["wifi"]["ssid"] | "", sizeof(wifiSettings.ssid));
        strlcpy(wifiSettings.password, doc["wifi"]["password"] | "", sizeof(wifiSettings.password));
    }

    // Wczytywanie ustawień sterownika
    if (doc.containsKey("controller")) {
        controllerSettings.type = doc["controller"]["type"] | "kt-lcd";
        
        if (controllerSettings.type == "kt-lcd") {
            for (int i = 1; i <= 5; i++) {
                controllerSettings.ktParams[i-1] = doc["controller"]["p"][String(i)] | 0;
            }
            for (int i = 1; i <= 15; i++) {
                controllerSettings.ktParams[i+4] = doc["controller"]["c"][String(i)] | 0;
            }
            for (int i = 1; i <= 3; i++) {
                controllerSettings.ktParams[i+19] = doc["controller"]["l"][String(i)] | 0;
            }
        } else {
            for (int i = 1; i <= 20; i++) {
                controllerSettings.s866Params[i-1] = doc["controller"]["p"][String(i)] | 0;
            }
        }
    }

    configFile.close();
}

// zapis wszystkich ustawień
void saveSettings() {
    StaticJsonDocument<1024> doc;

    // Zapisywanie ustawień czasu
    JsonObject timeObj = doc.createNestedObject("time");
    timeObj["ntpEnabled"] = timeSettings.ntpEnabled;
    timeObj["hours"] = timeSettings.hours;
    timeObj["minutes"] = timeSettings.minutes;
    timeObj["seconds"] = timeSettings.seconds;
    timeObj["day"] = timeSettings.day;
    timeObj["month"] = timeSettings.month;
    timeObj["year"] = timeSettings.year;

    // Zapisywanie ustawień świateł - teraz to obsługuje LightManager
    JsonObject lightObj = doc.createNestedObject("light");
    // Użyj wartości z lightManager
    lightObj["dayLights"] = (int)lightManager.getDayConfig();
    lightObj["nightLights"] = (int)lightManager.getNightConfig();
    lightObj["dayBlink"] = lightManager.getDayBlink();
    lightObj["nightBlink"] = lightManager.getNightBlink();
    lightObj["blinkFrequency"] = lightManager.getBlinkFrequency();

    // Zapisywanie ustawień podświetlenia
    JsonObject backlightObj = doc.createNestedObject("backlight");
    backlightObj["dayBrightness"] = backlightSettings.dayBrightness;
    backlightObj["nightBrightness"] = backlightSettings.nightBrightness;
    backlightObj["autoMode"] = backlightSettings.autoMode;

    // Zapisywanie ustawień WiFi
    JsonObject wifiObj = doc.createNestedObject("wifi");
    wifiObj["ssid"] = wifiSettings.ssid;
    wifiObj["password"] = wifiSettings.password;

    // Zapisywanie ustawień sterownika
    JsonObject controllerObj = doc.createNestedObject("controller");
    controllerObj["type"] = controllerSettings.type;
    
    JsonObject paramsObj = controllerObj.createNestedObject("params");
    if (controllerSettings.type == "kt-lcd") {
      for (int i = 1; i <= 5; i++) {
        paramsObj["p"][String(i)] = controllerSettings.ktParams[i-1];
      }
      for (int i = 1; i <= 15; i++) {
        paramsObj["c"][String(i)] = controllerSettings.ktParams[i+4];
      }
      for (int i = 1; i <= 3; i++) {
        paramsObj["l"][String(i)] = controllerSettings.ktParams[i+19];
      }
    } else {
      for (int i = 1; i <= 20; i++) {
        paramsObj["p"][String(i)] = controllerSettings.s866Params[i-1];
      }
    }

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        DEBUG_INFO("Nie udalo sie otworzyc pliku konfiguracyjnego do zapisu");
        return;
    }

    if (serializeJson(doc, configFile) == 0) {
        DEBUG_INFO("Nie udalo sie zapisac pliku konfiguracyjnego");
    }

    configFile.close();
}

// konwersja parametru na indeks
int getParamIndex(const String& param) {
    if (param.startsWith("p")) {
        return param.substring(1).toInt() - 1;
    } else if (param.startsWith("c")) {
        return param.substring(1).toInt() + 4;  // P1-P5 zajmują indeksy 0-4
    } else if (param.startsWith("l")) {
        return param.substring(1).toInt() + 19; // P1-P5 i C1-C15 zajmują indeksy 0-19
    }
    return -1;
}

// aktualizacja parametrów kontrolera
void updateControllerParam(const String& param, int value) {
    if (controllerSettings.type == "kt-lcd") {
        int index = getParamIndex(param);
        if (index >= 0 && index < 23) { // 5 (P) + 15 (C) + 3 (L) = 23 parametry
            controllerSettings.ktParams[index] = value;
        }
    } else if (controllerSettings.type == "s866") {
        if (param.startsWith("p")) {
            int index = param.substring(1).toInt() - 1;
            if (index >= 0 && index < 20) {
                controllerSettings.s866Params[index] = value;
            }
        }
    }
    saveSettings();
}

// konwersja trybu świateł na string
const char* getLightModeString(LightSettings::LightMode mode) {
    switch (mode) {
        case LightSettings::FRONT: return "FRONT";
        case LightSettings::REAR: return "REAR";
        case LightSettings::BOTH: return "BOTH";
        case LightSettings::NONE:
        default: return "NONE";
    }
}

// --- Funkcje serwera WWW ---

// Funkcja do pobierania wartości licznika
float getOdometerValue() {
    if (!LittleFS.begin(false)) {
        DEBUG_ERROR("Blad montowania LittleFS");
        return 0.0;
    }
    
    if (!LittleFS.exists("/odometer.json")) {
        DEBUG_ERROR("Nie znaleziono pliku licznika");
        return 0.0;
    }
    
    File file = LittleFS.open("/odometer.json", "r");
    if (!file) {
        DEBUG_ERROR("Nie udalo sie otworzyc pliku licznika do odczytu");
        return 0.0;
    }
    
    StaticJsonDocument<64> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        DEBUG_ERROR("Nie udalo sie przetworzyc pliku licznika");
        return 0.0;
    }
    
    return doc["value"];
}

// Funkcja do zapisywania wartości licznika
bool saveOdometerValue(float value) {
    if (!LittleFS.begin(false)) {
        DEBUG_ERROR("Blad montowania LittleFS");
        return false;
    }
    
    // Usuń istniejący plik
    if (LittleFS.exists("/odometer.json")) {
        LittleFS.remove("/odometer.json");
    }
    
    File file = LittleFS.open("/odometer.json", "w");
    if (!file) {
        DEBUG_ERROR("Nie udalo sie otworzyc pliku licznika do zapisu");
        return false;
    }
    
    StaticJsonDocument<64> doc;
    doc["value"] = value;
    
    size_t bytes = serializeJson(doc, file);
    file.close();
    
    if (bytes == 0) {
        DEBUG_ERROR("Nie udalo sie zapisac pliku licznika");
        return false;
    }
    
    return true;
}

// konfiguracja serwera WWW
void setupWebServer() {
    // Serwowanie plików statycznych
    server.serveStatic("/", LittleFS, "/");

    server.on("/api/filesystem/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        StaticJsonDocument<512> doc;
        
        // Informacje o systemie plików
        doc["totalBytes"] = LittleFS.totalBytes();
        doc["usedBytes"] = LittleFS.usedBytes();
        doc["freeBytes"] = LittleFS.totalBytes() - LittleFS.usedBytes();
        
        // Lista plików
        JsonArray files = doc.createNestedArray("files");
        File root = LittleFS.open("/");
        File file = root.openNextFile();
        
        while (file) {
            JsonObject fileObj = files.createNestedObject();
            fileObj["name"] = file.name();
            fileObj["size"] = file.size();
            fileObj["isDir"] = file.isDirectory();
            file = root.openNextFile();
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Dodaj endpoint do otrzymania informacji diagnostycznych
    server.on("/api/debug-odometer", HTTP_GET, [](AsyncWebServerRequest *request) {
        #ifdef DEBUG_INFO_ENABLED
        //odometer.debugPreferences();
        #endif
        
        StaticJsonDocument<256> doc;
        doc["isValid"] = odometer.isValid();
        doc["currentValue"] = odometer.getRawTotal();
        
        // Dodaj informacje o preferencjach
        preferences.begin("system", true);
        JsonObject prefsInfo = doc.createNestedObject("preferences");
        prefsInfo["freeEntries"] = preferences.freeEntries();
        preferences.end();
        
        // Dodaj informacje o systemie plików
        JsonObject fsInfo = doc.createNestedObject("filesystem");
        fsInfo["totalBytes"] = LittleFS.totalBytes();
        fsInfo["usedBytes"] = LittleFS.usedBytes();
        fsInfo["freeBytes"] = LittleFS.totalBytes() - LittleFS.usedBytes();
        fsInfo["odometerFileExists"] = LittleFS.exists("/odometer.json");
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Licznik całkowity 
    // Endpoint do pobierania wartości licznika
    server.on("/api/odometer", HTTP_GET, [](AsyncWebServerRequest *request) {
        float value = getOdometerValue();
        request->send(200, "text/plain", String(value));
    });

    // Endpoint do ustawiania wartości licznika
    server.on("/api/setOdometer", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("value", true)) {
            String valueStr = request->getParam("value", true)->value();
            float value = valueStr.toFloat();
            
            bool success = saveOdometerValue(value);
            
            if (success) {
                request->send(200, "text/plain", "OK");
            } else {
                request->send(500, "text/plain", "ERROR");
            }
        } else {
            request->send(400, "text/plain", "Missing value parameter");
        }
    });

    // Światła
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        DynamicJsonDocument doc(1024);
        JsonObject root = doc.to<JsonObject>();
        
        // Dodaj obiekt lights z konfiguracją
        JsonObject lights = doc.createNestedObject("lights");
        lights["dayLights"] = lightManager.getConfigString(lightManager.getDayConfig());
        lights["nightLights"] = lightManager.getConfigString(lightManager.getNightConfig());
        lights["dayBlink"] = lightManager.getDayBlink();
        lights["nightBlink"] = lightManager.getNightBlink();
        lights["blinkFrequency"] = lightManager.getBlinkFrequency();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/api/lights/file", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!LittleFS.begin(false)) {
            request->send(500, "application/json", "{\"error\":\"Failed to mount filesystem\"}");
            return;
        }
        
        if (LittleFS.exists("/light_config.json")) {
            File file = LittleFS.open("/light_config.json", "r");
            if (!file) {
                request->send(500, "application/json", "{\"error\":\"Failed to open config file\"}");
                return;
            }
            
            // Odczytaj zawartość pliku
            String content = file.readString();
            file.close();
            
            // Analizuj JSON, aby wyświetlić go w bardziej czytelnej formie
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, content);
            
            if (error) {
                request->send(200, "text/plain", content); // Pokaż surową zawartość w przypadku błędu parsowania
            } else {
                // Dodaj dodatkowe informacje
                JsonObject debug = doc.createNestedObject("_debug");
                if (doc.containsKey("dayConfig")) {
                    uint8_t dayConfigValue = doc["dayConfig"];
                    debug["dayConfig_hex"] = "0x" + String(dayConfigValue, HEX);
                    debug["dayConfig_bin"] = "0b" + String(dayConfigValue, BIN);
                    debug["dayConfig_FRONT"] = (dayConfigValue & LightManager::FRONT) != 0;
                    debug["dayConfig_DRL"] = (dayConfigValue & LightManager::DRL) != 0;
                    debug["dayConfig_REAR"] = (dayConfigValue & LightManager::REAR) != 0;
                }
                
                if (doc.containsKey("nightConfig")) {
                    uint8_t nightConfigValue = doc["nightConfig"];
                    debug["nightConfig_hex"] = "0x" + String(nightConfigValue, HEX);
                    debug["nightConfig_bin"] = "0b" + String(nightConfigValue, BIN);
                    debug["nightConfig_FRONT"] = (nightConfigValue & LightManager::FRONT) != 0;
                    debug["nightConfig_DRL"] = (nightConfigValue & LightManager::DRL) != 0;
                    debug["nightConfig_REAR"] = (nightConfigValue & LightManager::REAR) != 0;
                }
                
                String enriched;
                serializeJsonPretty(doc, enriched);
                request->send(200, "application/json", enriched);
            }
        } else {
            request->send(404, "application/json", "{\"error\":\"Config file not found\"}");
        }
    });

    // Endpoint GET dla konfiguracji świateł
    server.on("/api/lights/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<512> doc;
        
        doc["status"] = "ok";
        JsonObject lights = doc.createNestedObject("lights");
        lights["dayLights"] = lightManager.getConfigString(lightManager.getDayConfig());
        lights["nightLights"] = lightManager.getConfigString(lightManager.getNightConfig());
        lights["dayBlink"] = lightManager.getDayBlink();
        lights["nightBlink"] = lightManager.getNightBlink();
        lights["blinkFrequency"] = lightManager.getBlinkFrequency();
        lights["currentMode"] = (int)lightManager.getMode();
        lights["controlMode"] = (int)lightManager.getControlMode(); // Dodaj informację o trybie sterowania
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Endpoint POST dla konfiguracji świateł
    server.on("/api/lights/config", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Ten handler zostanie wywołany po zakończeniu przetwarzania danych
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index + len != total) {
            return; // Czekamy na wszystkie dane
        }
        
        DEBUG_LIGHT("Processing request");
        DEBUG_LIGHT("Content-Type: %s\n", request->contentType().c_str());
        DEBUG_LIGHT("Data length: %d bytes\n", len);
        
        // Zmienna na dane JSON
        String jsonString;
        
        // Określ źródło danych na podstawie Content-Type
        if (request->contentType() == "application/x-www-form-urlencoded") {
            // Obsługa form-encoded
            if (request->hasParam("data", true)) {
                jsonString = request->getParam("data", true)->value();
                DEBUG_LIGHT("Form data param: %s\n", jsonString.c_str());
            } else {
                DEBUG_LIGHT("Missing data parameter in form data");
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing data parameter\"}");
                return;
            }
        } 
        else if (request->contentType() == "application/json") {
            // Obsługa czystego JSON
            if (len > 0) {
                data[len] = '\0'; // Dodaj null terminator
                jsonString = String((char*)data);
                DEBUG_LIGHT("Direct JSON data: %s\n", jsonString.c_str());
            } else {
                DEBUG_LIGHT("Empty JSON data");
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Empty JSON data\"}");
                return;
            }
        }
        else {
            // Nieznany format danych
            DEBUG_LIGHT("Unsupported Content-Type: %s\n", request->contentType().c_str());
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Unsupported content type\"}");
            return;
        }
        
        // Parsowanie JSON
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, jsonString);
        
        if (error) {
            DEBUG_LIGHT("JSON parsing error: %s\n", error.c_str());
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON format\"}");
            return;
        }
        
        // Przetwarzanie danych
        bool configSuccess = false;
        
        // Zapisz poprzednie wartości dla porównania
        uint8_t oldDayConfig = lightManager.getDayConfig();
        uint8_t oldNightConfig = lightManager.getNightConfig();
        bool oldDayBlink = lightManager.getDayBlink();
        bool oldNightBlink = lightManager.getNightBlink();
        uint16_t oldBlinkFrequency = lightManager.getBlinkFrequency();
        
        DEBUG_DETAIL("Aktualna konfiguracja:"); 
        DEBUG_DETAIL("Konfiguracja dzienna: 0x%02X (%s)", oldDayConfig, lightManager.getConfigString(oldDayConfig).c_str());
        DEBUG_DETAIL("Konfiguracja nocna: 0x%02X (%s)", oldNightConfig, lightManager.getConfigString(oldNightConfig).c_str());
        DEBUG_DETAIL("Miganie dzienne: %d", oldDayBlink);
        DEBUG_DETAIL("Miganie nocne: %d", oldNightBlink);
        DEBUG_DETAIL("Czestotliwosc migania: %d", oldBlinkFrequency);
        
        // Zmień tylko te wartości, które są w żądaniu
        if (doc.containsKey("dayLights")) {
            const char* dayLightsStr = doc["dayLights"];
            uint8_t dayConfig = lightManager.parseConfigString(dayLightsStr);
            bool dayBlink = doc.containsKey("dayBlink") ? doc["dayBlink"].as<bool>() : oldDayBlink;
            
            lightManager.setDayConfig(dayConfig, dayBlink);
            DEBUG_LIGHT("Ustawiono konfiguracje dzienna: '%s' => 0x%02X, miganie: %d", dayLightsStr, dayConfig, dayBlink);
        }
        
        if (doc.containsKey("nightLights")) {
            const char* nightLightsStr = doc["nightLights"];
            uint8_t nightConfig = lightManager.parseConfigString(nightLightsStr);
            bool nightBlink = doc.containsKey("nightBlink") ? doc["nightBlink"].as<bool>() : oldNightBlink;
            
            lightManager.setNightConfig(nightConfig, nightBlink);
            DEBUG_LIGHT("Ustawiono konfiguracje nocna: '%s' => 0x%02X, miganie: %d", nightLightsStr, nightConfig, nightBlink);
        }
        
        if (doc.containsKey("blinkFrequency")) {
            lightManager.setBlinkFrequency(doc["blinkFrequency"] | oldBlinkFrequency);
            DEBUG_LIGHT("Ustawiono czestotliwosc migania: %d", (int)doc["blinkFrequency"]);
        }
        
        // Zapisz konfigurację
        configSuccess = lightManager.saveConfig();
        DEBUG_LIGHT("Zapis konfiguracji %s", configSuccess ? "powiodl sie" : "nie powiodl sie");
        
        // Przygotuj odpowiedź
        StaticJsonDocument<512> responseDoc;
        
        if (configSuccess) {
            responseDoc["status"] = "ok";
            responseDoc["message"] = "Konfiguracja zapisana pomyślnie";
            
            // Dodaj aktualne ustawienia świateł do odpowiedzi
            JsonObject lights = responseDoc.createNestedObject("lights");
            lights["dayLights"] = lightManager.getConfigString(lightManager.getDayConfig());
            lights["nightLights"] = lightManager.getConfigString(lightManager.getNightConfig());
            lights["dayBlink"] = lightManager.getDayBlink();
            lights["nightBlink"] = lightManager.getNightBlink();
            lights["blinkFrequency"] = lightManager.getBlinkFrequency();
            lights["currentMode"] = (int)lightManager.getMode();
            
            // Zastosuj nowe ustawienia natychmiast
            LightManager::LightMode currentMode = lightManager.getMode();
            lightManager.setMode(currentMode);  // To wymusi ponowną konfigurację świateł
        } else {
            responseDoc["status"] = "error";
            responseDoc["message"] = "Błąd podczas zapisu konfiguracji świateł";
        }

        if (doc.containsKey("controlMode")) {
            int controlMode = doc["controlMode"].as<int>();
            lightManager.setControlMode((LightManager::ControlMode)controlMode);
            applyBacklightSettings();
        }

        String responseStr;
        serializeJson(responseDoc, responseStr);
        DEBUG_LIGHT("Wysylam odpowiedz: %s", responseStr.c_str());
        
        request->send(200, "application/json", responseStr);
    });

    // Dodaj endpoint do testowania konfiguracji
    server.on("/api/lights/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<512> doc;
        
        doc["dayConfig"] = lightManager.getDayConfig();
        doc["dayConfigString"] = lightManager.getConfigString(lightManager.getDayConfig());
        doc["nightConfig"] = lightManager.getNightConfig();
        doc["nightConfigString"] = lightManager.getConfigString(lightManager.getNightConfig());
        doc["dayBlink"] = lightManager.getDayBlink();
        doc["nightBlink"] = lightManager.getNightBlink();
        doc["blinkFrequency"] = lightManager.getBlinkFrequency();
        doc["currentMode"] = (int)lightManager.getMode();
        
        // Dodaj binarne reprezentacje
        doc["dayConfig_binary"] = String(lightManager.getDayConfig(), BIN);
        doc["nightConfig_binary"] = String(lightManager.getNightConfig(), BIN);
        
        // Sprawdź poszczególne bity (flagi)
        doc["dayConfig_FRONT"] = (lightManager.getDayConfig() & LightManager::FRONT) != 0;
        doc["dayConfig_DRL"] = (lightManager.getDayConfig() & LightManager::DRL) != 0;
        doc["dayConfig_REAR"] = (lightManager.getDayConfig() & LightManager::REAR) != 0;
        
        doc["nightConfig_FRONT"] = (lightManager.getNightConfig() & LightManager::FRONT) != 0;
        doc["nightConfig_DRL"] = (lightManager.getNightConfig() & LightManager::DRL) != 0;
        doc["nightConfig_REAR"] = (lightManager.getNightConfig() & LightManager::REAR) != 0;
        
        String response;
        serializeJson(doc, response);
        
        DEBUG_LIGHT("Konfiguracja swiatel: %s", response.c_str());
        
        request->send(200, "application/json", response);
    });

    // Endpoint do pobierania czasu (GET)
    server.on("/api/time", HTTP_GET, [](AsyncWebServerRequest* request) {
        DateTime now = rtc.now();
        
        StaticJsonDocument<200> doc;
        JsonObject time = doc.createNestedObject("time");
        time["year"] = now.year();
        time["month"] = now.month();
        time["day"] = now.day();
        time["hours"] = now.hour();
        time["minutes"] = now.minute();
        time["seconds"] = now.second();
        
        String response;
        serializeJson(doc, response);
        
        //DEBUG_INFO("Wysylam aktualny czas: %s", response.c_str());

        request->send(200, "application/json", response);
    });

    // Endpoint do ustawiania czasu (POST)
    server.on("/api/time", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
        [](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, (char*)data);

            if (!error) {
                int year = doc["year"] | 2024;
                int month = doc["month"] | 1;
                int day = doc["day"] | 1;
                int hour = doc["hour"] | 0;
                int minute = doc["minute"] | 0;
                int second = doc["second"] | 0;

                // Dodaj walidację
                if (year >= 2000 && year <= 2099 &&
                    month >= 1 && month <= 12 &&
                    day >= 1 && day <= 31 &&
                    hour >= 0 && hour <= 23 &&
                    minute >= 0 && minute <= 59 &&
                    second >= 0 && second <= 59) {
                    
                    rtc.adjust(DateTime(year, month, day, hour, minute, second));
                    
                    DEBUG_INFO("Czas zostal zaktualizowany: %d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
                    
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                } else {
                    DEBUG_ERROR("Bledne wartosci daty/czasu");
                    request->send(400, "application/json", "{\"error\":\"Invalid date/time values\"}");
                }
            } else {
                DEBUG_ERROR("Bledny format JSON");
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            }
    });
    
    // Obsługa POST dla konfiguracji wyświetlacza
    server.on("/api/display/config", HTTP_POST, [](AsyncWebServerRequest *request) {
        // Obsługa POST
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, data, len);
        
        if (error) {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }
        
        // Aktualizacja ustawień podświetlenia
        backlightSettings.autoMode = doc["autoMode"] | false;
        backlightSettings.dayBrightness = doc["dayBrightness"] | 100;
        backlightSettings.nightBrightness = doc["nightBrightness"] | 50;
        
        // Pobierz jasność i dostosuj jeśli potrzeba
        int newBrightness = doc["brightness"] | 70;
        if (newBrightness <= 100) {
            // Wartość procentowa (0-100), zachowaj jak jest
            displayBrightness = newBrightness;
        } else {
            // Wartość surowa (>100), użyj bezpośrednio
            displayBrightness = newBrightness;
        }
        
        DEBUG_INFO("Zapisuje ustawienia wyswietlacza: autoMode = %d, jasnosc = %d, dzien = %d, noc = %d", 
            backlightSettings.autoMode, displayBrightness, backlightSettings.dayBrightness, backlightSettings.nightBrightness);
        
        // Aktualizacja ustawień auto-off
        if (doc.containsKey("autoOffTime")) {
            autoOffTime = doc["autoOffTime"] | 0;
            DEBUG_INFO("Aktualizuje autoOffTime: %d minut", autoOffTime);
            saveAutoOffSettings();
        }
        
        // Zastosuj ustawienia
        applyBacklightSettings();
        saveBacklightSettingsToFile();
        
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // Endpoint do obsługi konfiguracji wyświetlacza
    server.on("/api/display/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<512> doc;
        
        // Przekształć wartość jasności na procenty, jeśli jest poza zakresem 0-100
        int brightnessToSend = displayBrightness;
        if (brightnessToSend > 100) {
            brightnessToSend = map(displayBrightness, 0, 255, 0, 100);
        }
        
        doc["brightness"] = brightnessToSend;
        doc["dayBrightness"] = backlightSettings.dayBrightness;
        doc["nightBrightness"] = backlightSettings.nightBrightness;
        doc["autoMode"] = backlightSettings.autoMode;
        // Dodaj bezpośrednio wartość autoOffTime
        doc["autoOffTime"] = autoOffTime;

        String response;
        serializeJson(doc, response);
        
        DEBUG_INFO("Wysyłam konfigurację wyświetlacza: %s", response.c_str());
        
        request->send(200, "application/json", response);
    });

    // Endpoint GET dla ustawień auto-off
    server.on("/api/display/auto-off", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<64> doc;
        doc["autoOffTime"] = autoOffTime;
        doc["enabled"] = autoOffEnabled;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Endpoint POST dla ustawień auto-off
    server.on("/api/display/auto-off", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index + len != total) {
                return; // Czekamy na wszystkie dane
            }

            data[len] = '\0'; // Dodaj null terminator
            StaticJsonDocument<64> doc;
            DeserializationError error = deserializeJson(doc, (const char*)data);
            
            if (!error) {
                if (doc.containsKey("autoOffTime")) {
                    autoOffTime = doc["autoOffTime"].as<int>();
                    
                    // Ogranicz wartość do sensownego zakresu
                    if (autoOffTime < 0) autoOffTime = 0;
                    if (autoOffTime > 60) autoOffTime = 60;  // Max 60 minut
                }
                
                if (doc.containsKey("enabled")) {
                    autoOffEnabled = doc["enabled"].as<bool>();
                }
                                
                // Zapisz ustawienia do pliku
                saveAutoOffSettings();
                
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            }
        }
    );

    // Endpoint do zapisywania ustawień ogólnych
    server.on("/save-general-settings", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index + len != total) {
                return; // Nie wszystkie dane zostały otrzymane
            }

            // Dodaj null terminator do danych
            data[len] = '\0';
            
            StaticJsonDocument<64> doc;
            DeserializationError error = deserializeJson(doc, (const char*)data);

            if (!error) {
                // Konwersja wartości "700C" na 0 lub liczby na odpowiednie wartości
                if (doc.containsKey("wheelSize")) {
                    String wheelSizeStr = doc["wheelSize"].as<String>();
                    if (wheelSizeStr == "700C") {
                        generalSettings.wheelSize = 0; // 0 oznacza 700C
                    } else {
                        generalSettings.wheelSize = doc["wheelSize"].as<uint8_t>();
                    }
                    
                    // Zapisz ustawienia od razu po zmianie
                    saveGeneralSettingsToFile();
                    DEBUG_INFO("Zapisano ustawienia ogolne");
                    DEBUG_INFO("Rozmiar kola: %d", generalSettings.wheelSize);
                }

                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
            }
    });

    // Dodaj w setupWebServer():
    server.on("/get-bluetooth-config", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<256> doc; // Zwiększ rozmiar dokumentu
        doc["bmsEnabled"] = bluetoothConfig.bmsEnabled;
        doc["tpmsEnabled"] = bluetoothConfig.tpmsEnabled;
        doc["bmsMac"] = bluetoothConfig.bmsMac;
        doc["frontTpmsMac"] = bluetoothConfig.frontTpmsMac; // Dodaj to
        doc["rearTpmsMac"] = bluetoothConfig.rearTpmsMac;   // Dodaj to
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/save-bluetooth-config", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index + len != total) {
            return; // Nie wszystkie dane zostały otrzymane
        }

        // Dodaj null terminator do danych
        data[len] = '\0';
        
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, (const char*)data);
        
        if (!error) {
            bluetoothConfig.bmsEnabled = doc["bmsEnabled"] | false;
            bluetoothConfig.tpmsEnabled = doc["tpmsEnabled"] | false;
            
            // Dodaj obsługę MAC adresów
            if (doc.containsKey("bmsMac")) {
                strlcpy(bluetoothConfig.bmsMac, doc["bmsMac"], sizeof(bluetoothConfig.bmsMac));
            }
            
            if (doc.containsKey("frontTpmsMac")) {
                strlcpy(bluetoothConfig.frontTpmsMac, doc["frontTpmsMac"], sizeof(bluetoothConfig.frontTpmsMac));
            }
            
            if (doc.containsKey("rearTpmsMac")) {
                strlcpy(bluetoothConfig.rearTpmsMac, doc["rearTpmsMac"], sizeof(bluetoothConfig.rearTpmsMac));
            }
            
            saveBluetoothConfigToFile();
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        }
    });
    
    // Endpoint do pobierania aktualnych ustawień ogólnych
    server.on("/get-general-settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<64> doc;
        if (generalSettings.wheelSize == 0) {
            doc["wheelSize"] = "700C";
        } else {
            doc["wheelSize"] = generalSettings.wheelSize;
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/api/controller/config", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (request->hasParam("data", true)) {
            String jsonString = request->getParam("data", true)->value();
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, jsonString);

            if (!error) {
                controllerSettings.type = doc["type"].as<String>();
                
                if (controllerSettings.type == "kt-lcd") {
                    for (int i = 1; i <= 5; i++) {
                        if (doc["p"].containsKey(String(i))) {
                            controllerSettings.ktParams[i-1] = doc["p"][String(i)].as<int>();
                        }
                    }
                    for (int i = 1; i <= 15; i++) {
                        if (doc["c"].containsKey(String(i))) {
                            controllerSettings.ktParams[i+4] = doc["c"][String(i)].as<int>();
                        }
                    }
                    for (int i = 1; i <= 3; i++) {
                        if (doc["l"].containsKey(String(i))) {
                            controllerSettings.ktParams[i+19] = doc["l"][String(i)].as<int>();
                        }
                    }
                } else {
                    for (int i = 1; i <= 20; i++) {
                        if (doc["p"].containsKey(String(i))) {
                            controllerSettings.s866Params[i-1] = doc["p"][String(i)].as<int>();
                        }
                    }
                }
                
                saveSettings();
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            }
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data parameter\"}");
        }
    });

    ws.onEvent([](AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
        switch (type) {
            case WS_EVT_CONNECT:
                DEBUG_INFO("Klient WebSocket #%u polaczony z %s\n", client->id(), client->remoteIP().toString().c_str());
                webConfigActive = true; // Ustaw flagę aktywnej konfiguracji WWW
                updateActivityTime(); // Zresetuj timer aktywności
                break;
            case WS_EVT_DISCONNECT:
                DEBUG_INFO("Klient WebSocket #%u rozlaczony\n", client->id());
                if (ws.count() == 0) {
                    webConfigActive = false; // Jeśli nie ma już żadnych klientów, wyłącz flagę
                }
                break;
            case WS_EVT_DATA:
                updateActivityTime(); // Każda komunikacja to aktywność
                break;
        }
    });
    server.addHandler(&ws);

    // Start serwera
    server.begin();
}

// --- Funkcje systemu plików ---

bool testFileSystem() {
    DEBUG_INFO("=== Diagnostyka systemu plikow ===");
    
    if (!LittleFS.begin(false)) {
        DEBUG_INFO("Blad montowania LittleFS - proba formatowania");
        
        if (LittleFS.format()) {
            DEBUG_INFO("LittleFS sformatowany pomyslnie");
            
            if (!LittleFS.begin(false)) {
                DEBUG_INFO("Nadal nie można zamontowac LittleFS");
                return false;
            }
        } else {
            DEBUG_ERROR("Blad formatowania LittleFS");
            return false;
        }
    }
    
    DEBUG_INFO("Calkowita przestrzen: %d bajtow", LittleFS.totalBytes());
    DEBUG_INFO("Uzyta przestrzen: %d bajtow", LittleFS.usedBytes());
    DEBUG_INFO("Wolna przestrzen: %d bajtow", LittleFS.totalBytes() - LittleFS.usedBytes());
    
    // Test zapisu
    DEBUG_INFO("Test zapisu pliku...");
    File testFile = LittleFS.open("/test_fs.txt", "w");
    if (!testFile) {
        DEBUG_ERROR("Nie mozna utworzyc pliku testowego");
        return false;
    }
    
    if (testFile.print("Test zapisu") == 0) {
        DEBUG_ERROR("Blad zapisu do pliku testowego");
        testFile.close();
        return false;
    }
    
    testFile.close();
    
    // Test odczytu
    DEBUG_INFO("Test odczytu pliku...");
    testFile = LittleFS.open("/test_fs.txt", "r");
    if (!testFile) {
        DEBUG_ERROR("Nie mozna otworzyc pliku testowego do odczytu");
        return false;
    }
    
    String content = testFile.readString();
    testFile.close();
    
    if (content != "Test zapisu") {
        DEBUG_ERROR("Blad odczytu - zawartosc nie zgadza sie z zapisana");
        return false;
    }
    
    // Usuń plik testowy
    if (!LittleFS.remove("/test_fs.txt")) {
        DEBUG_ERROR("Nie udalo sie usunac pliku testowego");
    }
    
    // Lista plików
    File root = LittleFS.open("/", "r");
    if (!root) {
        DEBUG_ERROR("Blad otwarcia katalogu glownego");
        return false;
    }
    
    if (!root.isDirectory()) {
        DEBUG_ERROR("Katalog glowny nie jest katalogiem!");
        return false;
    }
    
    DEBUG_INFO("=== Lista plikow ===");
    File entry = root.openNextFile();
    while (entry) {
        DEBUG_INFO("  %s (%d bajtow)", entry.name(), entry.size());
        entry = root.openNextFile();
    }
    
    DEBUG_INFO("Test systemu plikow zakonczony pomyslnie");
    return true;
}

// Sprawdzenie i formatowanie systemu plików przy starcie
// Lepsza obsługa błędów w funkcji initLittleFS
bool initLittleFS() {
    if (!LittleFS.begin(false)) { // Najpierw spróbuj bez formatowania
        DEBUG_ERROR("Blad montowania LittleFS - proba formatowania");
        
        if (!LittleFS.format()) {
            DEBUG_ERROR("Blad formatowania LittleFS");
            return false;
        }
        
        if (!LittleFS.begin()) {
            DEBUG_ERROR("Blad montowania LittleFS po formatowaniu");
            return false;
        }
    }
    
    DEBUG_INFO("LittleFS zamontowany pomyslnie");
    return true;
}

// listowanie plików
void listFiles() {
    DEBUG_INFO("Pliki w LittleFS:");
    File root = LittleFS.open("/");
    if (!root) {
        DEBUG_ERROR("- Nie udalo sie otworzyc katalogu");
        return;
    }
    if (!root.isDirectory()) {
        DEBUG_ERROR(" - To nie jest katalog");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            DEBUG_INFO("KATALOG: %s", file.name());
        } else {
            DEBUG_INFO("PLIK: %s\tROZMIAR: %d", file.name(), file.size());
        }
        file = root.openNextFile();
    }
}

// wczytywanie konfiguracji
bool loadConfig() {
    if(!LittleFS.exists("/config.json")) {
        DEBUG_INFO("Tworzenie domyslnego pliku konfiguracyjnego...");
        // Tworzymy domyślną konfigurację
        StaticJsonDocument<512> defaultConfig;
        defaultConfig["version"] = "1.0.0";
        defaultConfig["timezone"] = "Europe/Warsaw";
        defaultConfig["lastUpdate"] = "";
        
        File configFile = LittleFS.open("/config.json", "w");
        if(!configFile) {
            DEBUG_ERROR("Nie udalo sie utworzyc pliku konfiguracyjnego");
            return false;
        }
        serializeJson(defaultConfig, configFile);
        configFile.close();
    }
    
    // Czytamy konfigurację
    File configFile = LittleFS.open("/config.json", "r");
    if(!configFile) {
        DEBUG_ERROR("Nie udalo sie otworzyc pliku konfiguracyjnego");
        return false;
    }
    
    DEBUG_INFO("Plik konfiguracyjny wczytany pomyslnie");
    return true;
}

/********************************************************************
 * DEKLARACJE I IMPLEMENTACJE FUNKCJI
 ********************************************************************/

// --- Funkcje inicjalizacyjne ---
void initializeDefaultSettings() {
    // Czas
    timeSettings.ntpEnabled = false;
    timeSettings.hours = 0;
    timeSettings.minutes = 0;
    timeSettings.seconds = 0;
    timeSettings.day = 1;
    timeSettings.month = 1;
    timeSettings.year = 2025;

    // Ustawienia świateł - teraz obsługiwane przez LightManager
    // lightManager zainicjalizowany jest w konstruktorze

    // Podświetlenie
    backlightSettings.Brightness = 70;      // Dodana linia inicjalizująca podstawową jasność
    backlightSettings.dayBrightness = 100;
    backlightSettings.nightBrightness = 50;
    backlightSettings.autoMode = false;

    // WiFi - początkowo puste
    memset(wifiSettings.ssid, 0, sizeof(wifiSettings.ssid));
    memset(wifiSettings.password, 0, sizeof(wifiSettings.password));
}

// --- Główne funkcje programu ---

// Funkcje pomocnicze do setup()

void initializeTemperatureSensors() {
    // Inicjalizacja DS18B20
    sensorsAir.begin();
    sensorsController.begin();
    // Ustawienie trybu nieblokującego
    sensorsAir.setWaitForConversion(false);
    sensorsController.setWaitForConversion(false);
    // Ustawienie najwyższej rozdzielczości
    sensorsAir.setResolution(12);
    sensorsController.setResolution(12);
    
    // Pierwsze żądanie pomiaru
    sensorsAir.requestTemperatures();
    sensorsController.requestTemperatures();
}

void initializeRTC() {
    if (!rtc.begin()) {
        DEBUG_ERROR("Nie znaleziono RTC");
        delay(1000); // Krótkie opóźnienie przed kontynuacją
    } else if (rtc.lostPower()) {
        DEBUG_ERROR("RTC utracil zasilanie, ustawiam aktualny czas");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

void initializePins() {
    // Przyciski
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);

    // Światła
    pinMode(FrontDayPin, OUTPUT);
    pinMode(FrontPin, OUTPUT);
    pinMode(RearPin, OUTPUT);
    
    // Inicjalizacja menedżera świateł
    lightManager.begin(FrontPin, FrontDayPin, RearPin);
    lightManager.setMode(LightManager::OFF); // Wyłącz światła po włączeniu ESP
    
    // Hamulec
    pinMode(BRAKE_SENSOR_PIN, INPUT_PULLUP);

    // Kadencja
    pinMode(CADENCE_SENSOR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(CADENCE_SENSOR_PIN), cadence_ISR, FALLING);
    resetCadenceData();

    // Ładowarka USB
    pinMode(UsbPin, OUTPUT);
    digitalWrite(UsbPin, LOW);
}

void resetCadenceData() {
    cadence_rpm = 0;
    cadence_avg_rpm = 0;
    cadence_max_rpm = 0;
    cadence_sum = 0;
    cadence_samples = 0;

    // Wczytaj liczbę impulsów na obrót z pamięci preferences
    preferences.begin("cadence", false);
    cadence_pulses_per_revolution = preferences.getUChar("pulses", 1); // Domyślnie 1
    preferences.end();
}

// wczytywanie ustawień podświetlenia
void loadBacklightSettingsFromFile() {
    // Sprawdź czy plik istnieje
    if (!LittleFS.exists(CONFIG_FILE)) {
        DEBUG_LIGHT("Brak pliku konfiguracyjnego, używam ustawień domyślnych");
        // Ustaw wartości domyślne
        backlightSettings.Brightness = 70;
        backlightSettings.dayBrightness = 100;
        backlightSettings.nightBrightness = 50;
        backlightSettings.autoMode = false;
        // Zapisz domyślne ustawienia do pliku
        saveBacklightSettingsToFile();
        return;
    }

    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        DEBUG_LIGHT("Nie można otworzyć pliku konfiguracyjnego, używam ustawień domyślnych");
        // Ustaw wartości domyślne
        backlightSettings.Brightness = 70;
        backlightSettings.dayBrightness = 100;
        backlightSettings.nightBrightness = 50;
        backlightSettings.autoMode = false;
        return;
    }

    // Sprawdź rozmiar pliku
    size_t fileSize = file.size();
    if (fileSize == 0) {
        DEBUG_LIGHT("Plik konfiguracyjny jest pusty, używam ustawień domyślnych");
        file.close();
        // Ustaw wartości domyślne
        backlightSettings.Brightness = 70;
        backlightSettings.dayBrightness = 100;
        backlightSettings.nightBrightness = 50;
        backlightSettings.autoMode = false;
        saveBacklightSettingsToFile(); // Zapisz domyślne ustawienia
        return;
    }

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        DEBUG_LIGHT("Błąd podczas parsowania JSON: %s, używam ustawień domyślnych", error.c_str());
        // Ustaw wartości domyślne
        backlightSettings.Brightness = 70;
        backlightSettings.dayBrightness = 100;
        backlightSettings.nightBrightness = 50;
        backlightSettings.autoMode = false;
        saveBacklightSettingsToFile(); // Zapisz poprawne ustawienia
        return;
    }

    // Wczytaj ustawienia z pliku z wartościami domyślnymi jeśli pole nie istnieje
    backlightSettings.Brightness = doc["brightness"] | 70;
    backlightSettings.dayBrightness = doc["dayBrightness"] | 100;
    backlightSettings.nightBrightness = doc["nightBrightness"] | 50;
    backlightSettings.autoMode = doc["autoMode"] | false;

    // Walidacja wczytanych wartości - zapewnienie, że mieszczą się w dopuszczalnym zakresie
    backlightSettings.Brightness = constrain(backlightSettings.Brightness, 10, 100);
    backlightSettings.dayBrightness = constrain(backlightSettings.dayBrightness, 10, 100);
    backlightSettings.nightBrightness = constrain(backlightSettings.nightBrightness, 10, 100);

    DEBUG_LIGHT("Wczytano ustawienia z pliku:");
    DEBUG_LIGHT("Brightness: %d", backlightSettings.Brightness);
    DEBUG_LIGHT("Day Brightness: %d", backlightSettings.dayBrightness);
    DEBUG_LIGHT("Night Brightness: %d", backlightSettings.nightBrightness);
    DEBUG_LIGHT("Auto Mode: %d", backlightSettings.autoMode ? 1 : 0);

    // Zastosuj wczytane ustawienia
    applyBacklightSettings();
}

void initializeFileSystemAndSettings() {
    // Najpierw sprawdź i inicjalizuj system plików
    if (!LittleFS.begin(true)) {
        DEBUG_ERROR("Blad montowania LittleFS");
        // Ustaw domyślne wartości w przypadku błędu
        initializeDefaultSettings();
        return;
    } 
    
    DEBUG_INFO("LittleFS zamontowany pomyslnie");
    
    // Wczytaj ustawienia z plików
    loadBacklightSettingsFromFile();
    loadGeneralSettingsFromFile();
    loadBluetoothConfigFromFile();
    loadAutoOffSettings(); // Dodaj tę linijkę
    
    // Sprawdź i utwórz konfigurację Bluetooth jeśli nie istnieje
    if (!LittleFS.exists("/bluetooth_config.json")) {
        DEBUG_BLE("Tworze domyslny plik konfiguracyjny Bluetooth");
        createDefaultBluetoothConfig();
    }

    // Obsługa licznika
    cleanupOldOdometer();
    initializeOdometer();
    
    // Zastosuj wczytane ustawienia podświetlenia
    applyBacklightSettings();
}

void createDefaultBluetoothConfig() {
    bluetoothConfig.bmsEnabled = false;
    bluetoothConfig.tpmsEnabled = false;
    strcpy(bluetoothConfig.bmsMac, "");
    strcpy(bluetoothConfig.frontTpmsMac, "");
    strcpy(bluetoothConfig.rearTpmsMac, "");
    saveBluetoothConfigToFile();
}

void cleanupOldOdometer() {
    if (LittleFS.exists("/odometer.json")) {
        DEBUG_INFO("Usuwanie starego pliku licznika...");
        
        if (LittleFS.remove("/odometer.json")) {
            DEBUG_INFO("Stary plik licznika usuniety");
        } else {
            DEBUG_ERROR("Nie udalo sie usunac starego pliku licznika");
        }
    }
}

void initializeOdometer() {
    if (!odometer.isValid()) {
        DEBUG_INFO("Inicjalizacja licznika...");
        
        if (!odometer.isValid()) {
            DEBUG_INFO("Blad inicjalizacji licznika!");
        } else {
            DEBUG_INFO("Licznik zainicjalizowany pomyslnie");
        }
    }

    DEBUG_INFO("Stan licznika: %s, Wartosc: %.2f", odometer.isValid() ? "OK" : "BLAD", odometer.getRawTotal());
}

void initializeBluetooth() {
    BLEDevice::init("e-Bike System PMW");
    
    if (bluetoothConfig.bmsEnabled) {
        bleClient = BLEDevice::createClient();
        connectToBms();
    }
    
    if (bluetoothConfig.tpmsEnabled) {
        // Inicjalizacja struktur TPMS
        resetTpmsData();
        loadTpmsAddresses();
        startTpmsScan();
    }
}

void resetTpmsData() {
    memset(&frontTpms, 0, sizeof(frontTpms));
    memset(&rearTpms, 0, sizeof(rearTpms));
    frontTpms.isActive = false;
    rearTpms.isActive = false;
}

void printSystemInfo() {
    DEBUG_INFO("=== Informacje o systemie ===");
    DEBUG_INFO("Pamiec: %d KB calosc, %d KB wolne", ESP.getHeapSize()/1024, ESP.getFreeHeap()/1024);
    DEBUG_INFO("PSRAM: %d KB calosc, %d KB wolne", ESP.getPsramSize()/1024, ESP.getFreePsram()/1024);
    DEBUG_INFO("Flash: %d MB, Szkic: %d KB", ESP.getFlashChipSize()/(1024*1024), ESP.getSketchSize()/1024);
}

void handleInitialSetButton() {
    unsigned long startTime = millis();
    while (!digitalRead(BTN_SET)) {  // Czekaj na puszczenie przycisku
        if ((millis() - startTime) > SET_LONG_PRESS) {
            displayActive = true;
            showingWelcome = true;
            messageStartTime = millis();
            
            if (!welcomeAnimationDone) {
                showWelcomeMessage();  // Pokaż animację powitania
            }
            
            // Czekaj na puszczenie przycisku
            while (!digitalRead(BTN_SET)) {
                delay(10);
            }
            break;
        }
        delay(10);
    }
}

// SETUP - zoptymalizowana wersja
void setup() { 
    // Sprawdź przyczynę wybudzenia
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    // Inicjalizacja UART dla Serial Monitor i komunikacji z KT
    Serial.begin(115200);
    Serial2.begin(CONTROLLER_UART_BAUD, SERIAL_8N1, CONTROLLER_RX_PIN, CONTROLLER_TX_PIN);
    
    DEBUG_INFO("=== Inicjalizacja systemu ===");
    DEBUG_INFO("Przyczyna wybudzenia: ");
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: DEBUG_INFO("Przycisk SET"); break;
        case ESP_SLEEP_WAKEUP_UNDEFINED: DEBUG_INFO("Normalne uruchomienie"); break;
        default: DEBUG_INFO("Inna (%d)\n", wakeup_reason);
    }
    
    // Inicjalizacja podstawowych komponentów
    Wire.begin();
    display.begin();
    display.setFontDirection(0);
    display.clearBuffer();
    display.sendBuffer();
    
    // Konfiguracja pinu przycisku SET (niezbędnego do wybudzenia)
    pinMode(BTN_SET, INPUT_PULLUP);

    // Jeśli nie zostaliśmy wybudzeni przez przycisk, natychmiast przechodzimy do trybu uśpienia
    if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT0) {
        #ifdef DEBUG_INFO_ENABLED
        DEBUG_INFO("Normalne uruchomienie - przechodze do trybu uspienia");
        Serial.flush();
        #endif
        goToSleep();
        return; // Ten kod nigdy nie zostanie wykonany
    }
    
    // Od tego momentu wiemy, że zostaliśmy wybudzeni przez przycisk SET
    
    // Inicjalizacja NVS (połączone operacje czyszczenia i inicjalizacji)
    esp_err_t err = nvs_flash_erase();
    if (err != ESP_OK) {
        DEBUG_INFO("Blad podczas czyszczenia NVS: %d\n", err);
    }
    
    if ((err = nvs_flash_init()) != ESP_OK) {
        DEBUG_INFO("Blad podczas inicjalizacji NVS: %d\n", err);
        return;
    }
    
    DEBUG_INFO("NVS zainicjalizowane pomyslnie");
    
    // Diagnostyka NVS (tylko w trybie DEBUG)
    nvs_stats_t nvs_stats;
    if (nvs_get_stats(NULL, &nvs_stats) == ESP_OK) {
        DEBUG_INFO("=== Statystyki NVS ===");
        DEBUG_INFO("Wpisy uzyte/wolne/calkowite: %d/%d/%d", nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);
    }

    // Inicjalizacja czujników temperatury
    initializeTemperatureSensors();
    
    // Inicjalizacja RTC
    initializeRTC();

    // Inicjalizacja pinów
    initializePins();
    
    // Inicjalizacja systemu plików i wczytanie ustawień
    initializeFileSystemAndSettings();

    // Zastosuj wczytane ustawienia
    applyBacklightSettings();

    // Inicjalizacja BLE jeśli potrzebne
    if (bluetoothConfig.bmsEnabled || bluetoothConfig.tpmsEnabled) {
        initializeBluetooth();
    }

    // Zastosuj wczytane ustawienia
    setLights();  
    applyBacklightSettings();

    // Ustaw początkowy czas aktywności - dodaj po inicjalizacji innych zmiennych
    lastActivityTime = millis();
    webConfigActive = false;
    
    // Załaduj ustawienia auto-off - upewnij się, że to się dzieje
    loadAutoOffSettings();

    #if DEBUG_INFO_ENABLED
    printSystemInfo();
    #endif

    // Obsługa przycisku SET po wybudzeniu
    handleInitialSetButton();
}

// Implementacja funkcji loop
void loop() {
    // Zmienne statyczne do śledzenia czasu
    static unsigned long lastButtonCheck = 0;
    static unsigned long lastUpdate = 0;
    static unsigned long lastAutoOffCheck = 0;
    static unsigned long lastDebugTime = 0;
    static unsigned long prevActivityTime = 0;
    static unsigned long lastAutoSaveTime = 0;
    static unsigned long lastWebSocketUpdate = 0;
    static unsigned long lastBlinkToggle = 0;
    static unsigned long last_rpm_calc = 0;

    // Stałe czasowe
    const unsigned long buttonInterval = 5;          // Interwał sprawdzania przycisków
    const unsigned long updateInterval = 2000;       // Interwał ogólnej aktualizacji danych
    const unsigned long AUTO_OFF_CHECK_INTERVAL = 5000;  // Sprawdzanie auto-off co 5s
    const unsigned long DEBUG_INTERVAL = 10000;      // Debugowanie co 10s
    const unsigned long AUTO_SAVE_INTERVAL = 60000;  // Automatyczny zapis co minutę
    const unsigned long WEBSOCKET_UPDATE = 1000;     // Aktualizacja WebSocket co sekundę
    const unsigned long rpm_calc_interval = 100;     // Obliczanie kadencji co 100ms

    // Zmienne do śledzenia stanu świateł
    static LightManager::LightMode lastLightMode = lightManager.getMode();
    static LightManager::ControlMode lastControlMode = lightManager.getControlMode();

    unsigned long currentTime = millis();

    // Sprawdzanie aktywności WebSocket i aktualizacja flagi
    if (currentTime - lastWebSocketUpdate >= WEBSOCKET_UPDATE) {
        if (ws.count() > 0) {
            webConfigActive = true;  // Konfiguracja WWW jest aktywna
            updateActivityTime();    // Aktualizuj czas aktywności

            // Wyślij dane statusu do klientów WebSocket
            DynamicJsonDocument statusDoc(512);
            statusDoc["speed"] = speed_kmh;
            statusDoc["temperature"] = currentTemp;
            statusDoc["battery"] = battery_capacity_percent;
            statusDoc["power"] = power_w;
            
            // Informacje o światłach
            JsonObject lightStatus = statusDoc.createNestedObject("lights");
            lightStatus["mode"] = lightManager.getModeString();
            lightStatus["dayConfig"] = lightManager.getConfigString(lightManager.getDayConfig());
            lightStatus["nightConfig"] = lightManager.getConfigString(lightManager.getNightConfig());
            
            String jsonStr;
            serializeJson(statusDoc, jsonStr);
            ws.textAll(jsonStr);
        } else {
            webConfigActive = false; // Brak aktywnych połączeń WebSocket
        }
        lastWebSocketUpdate = currentTime;
    }

    // Sprawdzanie automatycznego wyłączania (tylko gdy nie ma aktywnej konfiguracji)
    if (currentTime - lastAutoOffCheck >= AUTO_OFF_CHECK_INTERVAL) {
        lastAutoOffCheck = currentTime;
        if (displayActive && !configModeActive && !showingWelcome && !webConfigActive) {
            checkAutoOff();
        }
    }

    // Debugowanie stanu auto-off
    if (currentTime - lastDebugTime > DEBUG_INTERVAL) {
        lastDebugTime = currentTime;
        
        // Sprawdź czy lastActivityTime się zmienia (wskazuje na aktywność)
        bool activityDetected = (lastActivityTime != prevActivityTime);
        prevActivityTime = lastActivityTime;
        
        DEBUG_INFO("Auto-off status: czas=%d min, lastActivity=%u s, konfiguracja=%s, WWW=%s, aktywnosc=%s", 
            autoOffTime, 
            (currentTime - lastActivityTime) / 1000, 
            configModeActive ? "TAK" : "NIE",
            webConfigActive ? "TAK" : "NIE",
            activityDetected ? "WYKRYTA" : "BRAK");
    }

    // Automatyczny zapis danych
    if (currentTime - lastAutoSaveTime >= AUTO_SAVE_INTERVAL) {
        // Zapisz dane odometru i inne ważne dane
        lastAutoSaveTime = currentTime;
    }

    // Sprawdzanie zmian trybu świateł
    if (lightManager.getMode() != lastLightMode || lightManager.getControlMode() != lastControlMode) {
        DEBUG_LIGHT("Wykryto zmiane trybu swiatel lub kontroli");
        lastLightMode = lightManager.getMode();
        lastControlMode = lightManager.getControlMode();
        
        // Zaktualizuj podświetlenie
        applyBacklightSettings();
        updateActivityTime(); // Zmiana świateł to też aktywność
    }

    // Tryb konfiguracji - wyświetlanie ekranu AP
    if (configModeActive) {      
        display.clearBuffer();

        // Wycentruj każdą linię tekstu
        drawCenteredText("e-Bike System", 12, czcionka_srednia);
        drawCenteredText("Konfiguracja on-line", 25, czcionka_mala);
        drawCenteredText("siec: e-Bike System", 40, czcionka_mala);
        drawCenteredText("haslo: #mamrower", 51, czcionka_mala);
        drawCenteredText("IP: 192.168.4.1", 62, czcionka_mala);

        display.sendBuffer();

        // Sprawdź długie przytrzymanie SET do wyjścia
        static unsigned long setPressStartTime = 0;
        if (!digitalRead(BTN_SET)) { 
            if (setPressStartTime == 0) {
                setPressStartTime = millis();
                updateActivityTime(); // Naciśnięcie przycisku to aktywność
            } else if (millis() - setPressStartTime > 50) { // Zwiększono z 50ms na 1000ms dla pewności
                deactivateConfigMode();
                setPressStartTime = 0;
            }
        } else {
            setPressStartTime = 0;
        }
        return; // Wyjdź z pętli loop gdy w trybie konfiguracji
    }

    // Sprawdzaj czy nie trzeba włączyć trybu konfiguracji
    checkConfigMode();

    // Obsługa przycisków
    if (currentTime - lastButtonCheck >= buttonInterval) {
        handleButtons(); // Ta funkcja powinna wewnątrz wywoływać updateActivityTime()
        lastButtonCheck = currentTime;
    }

    // Obsługa migania świateł
    lightManager.update(); // Aktualizacja stanu świateł

    // Aktualizuj wyświetlacz tylko jeśli jest aktywny i nie wyświetla komunikatów
    if (displayActive && messageStartTime == 0) {
        display.clearBuffer();
        
        // Sprawdzanie trybu prowadzenia roweru
        if (walkAssistActive) {
            showWalkAssistMode(false);
        } else {
            drawTopBar();
            drawHorizontalLine();
            drawVerticalLine();
            drawAssistLevel();
            drawMainDisplay();
            drawLightStatus();
            handleTemperature();
            updateBmsData();
        }

        // Obliczanie kadencji
        unsigned long now = millis();
        if (now - last_rpm_calc >= rpm_calc_interval) {
            unsigned long dt_sum = 0;
            int valid_intervals = 0;
            for (int i = 0; i < CADENCE_SAMPLES_WINDOW - 1; i++) {
                unsigned long dt = cadence_pulse_times[i] - cadence_pulse_times[i + 1];
                if (dt > 0 && cadence_pulse_times[i + 1] != 0) {
                    dt_sum += dt;
                    valid_intervals++;
                }
            }
            
            int rpm = 0;
            if (valid_intervals > 0 && (now - cadence_pulse_times[0]) < 2000) {
                float avg_period = (float)dt_sum / valid_intervals;
                rpm = (60000.0 / avg_period) / cadence_pulses_per_revolution;
            }
            cadence_rpm = rpm;

            // Histereza dla strzałki kadencji
            switch (cadence_arrow_state) {
                case ARROW_NONE:
                    if (cadence_rpm > 0 && cadence_rpm < CADENCE_OPTIMAL_MIN)
                        cadence_arrow_state = ARROW_DOWN;
                    else if (cadence_rpm > CADENCE_OPTIMAL_MAX)
                        cadence_arrow_state = ARROW_UP;
                    break;
                case ARROW_DOWN:
                    if (cadence_rpm >= CADENCE_OPTIMAL_MIN + CADENCE_HYSTERESIS)
                        cadence_arrow_state = ARROW_NONE;
                    break;
                case ARROW_UP:
                    if (cadence_rpm <= CADENCE_OPTIMAL_MAX - CADENCE_HYSTERESIS)
                        cadence_arrow_state = ARROW_NONE;
                    break;
            }

            // Liczenie średniej/maksymalnej kadencji
            cadence_sum += cadence_rpm;
            cadence_samples++;
            if (cadence_rpm > cadence_max_rpm) cadence_max_rpm = cadence_rpm;

            last_rpm_calc = now;
        }

        // Co 5s aktualizuj wyświetlaną średnią i max
        if (now - last_avg_max_update >= AVG_MAX_UPDATE_INTERVAL) {
            if (cadence_samples > 0)
                cadence_avg_rpm = cadence_sum / cadence_samples;
            else
                cadence_avg_rpm = 0;
            last_avg_max_update = now;
        }

        updateCadenceLogic();
        drawCadenceArrowsAndCircle();
        display.sendBuffer();

        // Obsługa TPMS
        if (bluetoothConfig.tpmsEnabled) {
            if (tpmsScanning && (currentTime - lastTpmsScanTime > 5000)) {
                stopTpmsScan();
            }
            
            if (!tpmsScanning && (currentTime - lastTpmsScanTime > TPMS_SCAN_INTERVAL)) {
                startTpmsScan();
            }
            
            checkTpmsTimeout();
        }

        // Aktualizacja danych co określony interwał
        if (currentTime - lastUpdate >= updateInterval) {
            // Aktualizacja wszystkich danych (prędkość, temperatura, itp.)
            speed_kmh = (speed_kmh >= 35.0) ? 0.0 : speed_kmh + 0.1;
            temp_motor = 30.0 + random(20);
            range_km = 50.0 - (random(20) / 10.0);
            odometer.updateTotal(distance_km);
            distance_km += 0.1;
            power_w = 100 + random(300);
            power_avg_w = power_w * 0.8;
            power_max_w = power_w * 1.2;
            battery_current = random(50, 150) / 10.0;
            battery_capacity_wh = battery_voltage * battery_capacity_ah;
            battery_capacity_wh = 14.5 - (random(20) / 10.0);
            battery_capacity_percent = (battery_capacity_percent <= 0) ? 100 : battery_capacity_percent - 1;
            battery_voltage = (battery_voltage <= 42.0) ? 50.0 : battery_voltage - 0.1;
            
            // Aktualizacja średniej i maksymalnej prędkości
            static float speed_sum = 0;
            static int speed_count = 0;
            speed_sum += speed_kmh;
            speed_count++;
            speed_avg_kmh = speed_sum / speed_count;
            if (speed_kmh > speed_max_kmh) {
                speed_max_kmh = speed_kmh;
            }
            
            lastUpdate = currentTime;
        }
    }
}


/*
Piny przycisków
GPIO 12 SET
GPIO 13 UP
GPIO 14 DOWN

GPIO 18 FrontDay
GPIO 19 Front
GPIO 23 Rear

GPIO 32 ładowarka USB

GPIO 15 temperatura powietrza
GPIO 4 temperatura sterownika
GPIO  temperatura silnika

GPIO 26 czujnik hamulca
GPIO 27 czujnik kadencji

GPIO 21 SDA
GPIO 22 SCL

GPIO 12 (GPIO_NUM_12) - skonfigurowany jako pin wybudzający z trybu głębokiego snu (wakeup pin)
*/
