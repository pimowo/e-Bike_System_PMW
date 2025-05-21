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
//#include <TimeLib.h>            // Biblioteka do obsługi funkcji czasowych

// --- Biblioteki czujników temperatury ---
#include <OneWire.h>            // Biblioteka do komunikacji z czujnikami OneWire
#include <DallasTemperature.h>  // Biblioteka do obsługi czujników temperatury DS18B20

// --- Biblioteki Bluetooth ---
#include <BLEDevice.h>          // Główna biblioteka BLE
//#include <BLEServer.h>          // Biblioteka do tworzenia serwera BLE
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

// --- Biblioteki własne ---
#include "Odometer.h"         // Własna biblioteka do obsługi licznika kilometrów

/********************************************************************
 * DEFINICJE I STAŁE GLOBALNE
 ********************************************************************/

//#define DEBUG

// Wersja oprogramowania
const char* VERSION = "21.5.25";

// Nazwy plików konfiguracyjnych
const char* CONFIG_FILE = "/display_config.json";
const char* LIGHT_CONFIG_FILE = "/light_config.json";

// Definicje pinów
// przyciski
#define BTN_UP 13
#define BTN_DOWN 14
#define BTN_SET 12
// światła
#define FrontDayPin 5  // światła dzienne
#define FrontPin 18    // światła zwykłe
#define RearPin 19     // tylne światło
// ładowarka USB
#define UsbPin 32  // ładowarka USB
// czujniki temperatury
#define TEMP_AIR_PIN 15        // temperatutra powietrza (DS18B20)
#define TEMP_CONTROLLER_PIN 4  // temperatura sterownika (DS18B20)
// kadencja
#define CADENCE_SENSOR_PIN 27

// Stałe czasowe
const unsigned long DEBOUNCE_DELAY = 25;
const unsigned long BUTTON_DELAY = 200;
const unsigned long LONG_PRESS_TIME = 1000;
const unsigned long DOUBLE_CLICK_TIME = 300;
const unsigned long GOODBYE_DELAY = 3000;
const unsigned long SET_LONG_PRESS = 2000;
const unsigned long TEMP_REQUEST_INTERVAL = 1000;
const unsigned long DS18B20_CONVERSION_DELAY_MS = 750;

// Stałe wyświetlacza
// #define PRESSURE_LEFT_MARGIN 70
// #define PRESSURE_TOP_LINE 62
// #define PRESSURE_BOTTOM_LINE 62
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
    
    BluetoothConfig() : bmsEnabled(false), tpmsEnabled(false) {}
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

/********************************************************************
 * TYPY WYLICZENIOWE
 ********************************************************************/

// Ekrany główne
enum MainScreen {
    SPEED_SCREEN,      // Ekran prędkości
    CADENCE_SCREEN,    // Ekran kadencji
    TEMP_SCREEN,       // Ekran temperatur
    RANGE_SCREEN,      // Ekran zasięgu
    BATTERY_SCREEN,    // Ekran baterii
    POWER_SCREEN,      // Ekran mocy
    PRESSURE_SCREEN,   // Ekran ciśnienia
    USB_SCREEN,        // Ekran sterowania USB
    MAIN_SCREEN_COUNT  // Liczba głównych ekranów
};

// Podekrany
enum SpeedSubScreen {
    SPEED_KMH,       // Aktualna prędkość
    SPEED_AVG_KMH,   // Średnia prędkość
    SPEED_MAX_KMH,   // Maksymalna prędkość
    SPEED_SUB_COUNT  
};

enum CadenceSubScreen {
    CADENCE_RPM,       // Aktualna kadencja
    CADENCE_AVG_RPM,   // Średnia kadencja
    CADENCE_SUB_COUNT  
};

enum TempSubScreen {
    TEMP_AIR,
    TEMP_CONTROLLER,
    TEMP_MOTOR,
    TEMP_SUB_COUNT
};

enum RangeSubScreen {
    RANGE_KM,
    DISTANCE_KM,
    ODOMETER_KM,
    RANGE_SUB_COUNT
};

enum BatterySubScreen {
    BATTERY_VOLTAGE,
    BATTERY_CURRENT,
    BATTERY_CAPACITY_WH,
    BATTERY_CAPACITY_AH,
    BATTERY_CAPACITY_PERCENT,
    BATTERY_SUB_COUNT
};

enum PowerSubScreen {
    POWER_W,
    POWER_AVG_W,
    POWER_MAX_W,
    POWER_SUB_COUNT
};

enum PressureSubScreen {
    PRESSURE_BAR,
    PRESSURE_VOLTAGE,
    PRESSURE_TEMP,
    PRESSURE_SUB_COUNT
};

/********************************************************************
 * ZMIENNE GLOBALNE
 ********************************************************************/

// Obiekty główne
OdometerManager odometer;
Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
//AsyncEventSource events("/events");

// Zmienne stanu systemu
bool configModeActive = false;
bool legalMode = false;
bool welcomeAnimationDone = false;
bool displayActive = false;
bool showingWelcome = false;

// Zmienne stanu ekranu
MainScreen currentMainScreen = SPEED_SCREEN;
int currentSubScreen = 0;
bool inSubScreen = false;
uint8_t displayBrightness = 16;  // Wartość od 0 do 255 (jasność wyświetlacza)

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
int battery_capacity_percent;
int power_w;
int power_avg_w;
int power_max_w;
float speed_avg_kmh;
float speed_max_kmh;
// kadencja
int cadence_rpm;
int cadence_avg_rpm;
int cadence_max_rpm = 0;
volatile uint32_t cadence_pulse_count = 0;
volatile unsigned long last_cadence_pulse_time = 0;

// Zmienne dla czujników ciśnienia kół
float pressure_bar;           // przednie koło
float pressure_rear_bar;      // tylne koło
float pressure_voltage;       // napięcie przedniego czujnika
float pressure_rear_voltage;  // napięcie tylnego czujnika
float pressure_temp;          // temperatura przedniego czujnika
float pressure_rear_temp;     // temperatura tylnego czujnika

// Zmienne dla czujnika temperatury
float currentTemp = DEVICE_DISCONNECTED_C;
//bool temperatureReady = false;
bool conversionRequested = false;
unsigned long lastTempRequest = 0;
unsigned long ds18b20RequestTime;

// Zmienne dla przycisków
unsigned long lastClickTime = 0;
unsigned long lastButtonPress = 0;
unsigned long lastDebounceTime = 0;
unsigned long upPressStartTime = 0;
unsigned long downPressStartTime = 0;
unsigned long setPressStartTime = 0;
unsigned long messageStartTime = 0;
//bool firstClick = false;
bool upLongPressExecuted = false;
bool downLongPressExecuted = false;
bool setLongPressExecuted = false;

// Zmienne konfiguracyjne
int assistLevel = 3;
bool assistLevelAsText = false;
int lightMode = 0;        // 0=off, 1=dzień, 2=noc
int assistMode = 0;       // 0=PAS, 1=STOP, 2=GAZ, 3=P+G
bool usbEnabled = false;  // Stan wyjścia USB

// Zmienne dla świateł
unsigned long lastBlinkTime = 0;  // Czas ostatniego mrugania
bool blinkState = false;          // Stan mrugania (włączone/wyłączone)

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
BLEAddress bmsMacAddress("a5:c2:37:05:8b:86");
BLERemoteService* bleService;
BLERemoteCharacteristic* bleCharacteristicTx;
BLERemoteCharacteristic* bleCharacteristicRx;

// Instancje struktur konfiguracyjnych
ControllerSettings controllerSettings;
TimeSettings timeSettings;
LightSettings lightSettings;
BacklightSettings backlightSettings;
WiFiSettings wifiSettings;
GeneralSettings generalSettings;
BluetoothConfig bluetoothConfig;
BmsData bmsData;

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

void IRAM_ATTR cadence_ISR() {
    static unsigned long last_isr_time = 0;
    unsigned long now = millis();
    if (now - last_isr_time > 20) { // filtr drgań styków ~20ms
        cadence_pulse_count++;
        last_cadence_pulse_time = now;
        last_isr_time = now;
    }
}

/********************************************************************
 * DEKLARACJE I IMPLEMENTACJE FUNKCJI
 ********************************************************************/

// --- Funkcje BLE ---

// callback dla BLE
void notificationCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                        uint8_t* pData, size_t length, bool isNotify) {
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
                
                // SOC (%)
                bmsData.soc = pData[23];
                
                // Status ładowania/rozładowania
                uint8_t status = pData[22];
                bmsData.charging = (status & 0x01);
                bmsData.discharging = (status & 0x02);
                
                #ifdef DEBUG
                Serial.printf("Voltage: %.1fV, Current: %.1fA, SOC: %d%%\n", 
                            bmsData.voltage, bmsData.current, bmsData.soc);
                #endif
            }
            break;

        case 0x04:  // Cell info
            if (length >= 34) {  // Sprawdź czy mamy kompletny pakiet
                for (int i = 0; i < 16; i++) {
                    bmsData.cellVoltages[i] = (float)((pData[4 + i*2] << 8) | pData[5 + i*2]) / 1000.0;
                }
                #ifdef DEBUG
                Serial.println("Cell voltages updated");
                #endif
            }
            break;

        case 0x08:  // Temperature info
            if (length >= 12) {
                for (int i = 0; i < 4; i++) {
                    // Konwersja z K na °C
                    int16_t temp = ((pData[4 + i*2] << 8) | pData[5 + i*2]) - 2731;
                    bmsData.temperatures[i] = (float)temp / 10.0;
                }
                #ifdef DEBUG
                Serial.println("Temperatures updated");
                #endif
            }
            break;
    }
}

// wysyłanie zapytania do BMS
void requestBmsData(const uint8_t* command, size_t length) {
    if (bleClient && bleClient->isConnected() && bleCharacteristicTx) {
//        bleCharacteristicTx->writeValue(command, length);
    }
}

// aktualizacja danych BMS
void updateBmsData() {
    static unsigned long lastBmsUpdate = 0;
    const unsigned long BMS_UPDATE_INTERVAL = 1000; // Aktualizuj co 1 sekundę

    if (millis() - lastBmsUpdate >= BMS_UPDATE_INTERVAL) {
        if (bleClient && bleClient->isConnected()) {
            requestBmsData(BMS_BASIC_INFO, sizeof(BMS_BASIC_INFO));
            delay(100);  // Krótkie opóźnienie między zapytaniami
            requestBmsData(BMS_CELL_INFO, sizeof(BMS_CELL_INFO));
            delay(100);
            requestBmsData(BMS_TEMP_INFO, sizeof(BMS_TEMP_INFO));
            lastBmsUpdate = millis();
        }
    }
}

// połączenie z BMS
void connectToBms() {
    if (!bleClient->isConnected()) {
        #ifdef DEBUG
        Serial.println("Próba połączenia z BMS...");
        #endif

        if (bleClient->connect(bmsMacAddress)) {
            #ifdef DEBUG
            Serial.println("Połączono z BMS");
            #endif

            bleService = bleClient->getService("0000ff00-0000-1000-8000-00805f9b34fb");
            if (bleService == nullptr) {
                #ifdef DEBUG
                Serial.println("Nie znaleziono usługi BMS");
                #endif
                bleClient->disconnect();
                return;
            }

            bleCharacteristicTx = bleService->getCharacteristic("0000ff02-0000-1000-8000-00805f9b34fb");
            if (bleCharacteristicTx == nullptr) {
                #ifdef DEBUG
                Serial.println("Nie znaleziono charakterystyki Tx");
                #endif
                bleClient->disconnect();
                return;
            }

            bleCharacteristicRx = bleService->getCharacteristic("0000ff01-0000-1000-8000-00805f9b34fb");
            if (bleCharacteristicRx == nullptr) {
                #ifdef DEBUG
                Serial.println("Nie znaleziono charakterystyki Rx");
                #endif
                bleClient->disconnect();
                return;
            }

            // Rejestracja funkcji obsługi powiadomień BLE
            if (bleCharacteristicRx->canNotify()) {
                bleCharacteristicRx->registerForNotify(notificationCallback);
                #ifdef DEBUG
                Serial.println("Zarejestrowano powiadomienia dla Rx");
                #endif
            } else {
                #ifdef DEBUG
                Serial.println("Charakterystyka Rx nie obsługuje powiadomień");
                #endif
                bleClient->disconnect();
                return;
            }
        } else {
          #ifdef DEBUG
          Serial.println("Nie udało się połączyć z BMS");
          #endif
        }
    }
}

// --- Funkcje konfiguracji ---

// zapis ustawień świateł
void saveLightSettings() {
    #ifdef DEBUG
    Serial.println("Zapisywanie ustawień świateł");
    #endif

    // Przygotuj dokument JSON
    StaticJsonDocument<256> doc;
    
    // Zapisz ustawienia
    doc["dayLights"] = lightSettings.dayLights;
    doc["nightLights"] = lightSettings.nightLights;
    doc["dayBlink"] = lightSettings.dayBlink;
    doc["nightBlink"] = lightSettings.nightBlink;
    doc["blinkFrequency"] = lightSettings.blinkFrequency;

    // Otwórz plik do zapisu
    File file = LittleFS.open("/lights.json", "w");
    if (!file) {
        #ifdef DEBUG
        Serial.println("Błąd otwarcia pliku do zapisu");
        #endif
        return;
    }

    // Zapisz JSON do pliku
    if (serializeJson(doc, file) == 0) {
        #ifdef DEBUG
        Serial.println("Błąd podczas zapisu do pliku");
        #endif
    }

    file.close();

    #ifdef DEBUG
    Serial.println("Ustawienia świateł zapisane");
    Serial.print("dayLights: "); Serial.println(lightSettings.dayLights);
    Serial.print("nightLights: "); Serial.println(lightSettings.nightLights);
    #endif

    // Od razu zastosuj nowe ustawienia
    setLights();
}

// wczytywanie ustawień świateł
void loadLightSettings() {
    #ifdef DEBUG
    Serial.println("Wczytywanie ustawień świateł");
    #endif

    if (LittleFS.exists("/lights.json")) {
        File file = LittleFS.open("/lights.json", "r");
        if (file) {
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, file);
            file.close();

            if (!error) {
                const char* dayLightsStr = doc["dayLights"] | "FRONT";
                const char* nightLightsStr = doc["nightLights"] | "BOTH";

                // Konwersja stringów na enum
                if (strcmp(dayLightsStr, "FRONT") == 0) 
                    lightSettings.dayLights = LightSettings::FRONT;
                else if (strcmp(dayLightsStr, "REAR") == 0) 
                    lightSettings.dayLights = LightSettings::REAR;
                else if (strcmp(dayLightsStr, "BOTH") == 0) 
                    lightSettings.dayLights = LightSettings::BOTH;
                else 
                    lightSettings.dayLights = LightSettings::NONE;

                if (strcmp(nightLightsStr, "FRONT") == 0) 
                    lightSettings.nightLights = LightSettings::FRONT;
                else if (strcmp(nightLightsStr, "REAR") == 0) 
                    lightSettings.nightLights = LightSettings::REAR;
                else if (strcmp(nightLightsStr, "BOTH") == 0) 
                    lightSettings.nightLights = LightSettings::BOTH;
                else 
                    lightSettings.nightLights = LightSettings::NONE;

                lightSettings.dayBlink = doc["dayBlink"] | false;
                lightSettings.nightBlink = doc["nightBlink"] | false;
                lightSettings.blinkFrequency = doc["blinkFrequency"] | 500;
            }
        }
    } else {
        // Ustawienia domyślne
        lightSettings.dayLights = LightSettings::FRONT;
        lightSettings.nightLights = LightSettings::BOTH;
        lightSettings.dayBlink = false;
        lightSettings.nightBlink = false;
        lightSettings.blinkFrequency = 500;
    }
}

// ustawianie jasności wyświetlacza
void setDisplayBrightness(uint8_t brightness) {
    displayBrightness = brightness;
    display.setContrast(displayBrightness);
}

// zapis ustawień podświetlenia
void saveBacklightSettingsToFile() {
    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file) {
        #ifdef DEBUG
        Serial.println("Nie można otworzyć pliku do zapisu");
        #endif
        return;
    }

    StaticJsonDocument<200> doc;
    doc["dayBrightness"] = backlightSettings.dayBrightness;
    doc["nightBrightness"] = backlightSettings.nightBrightness;
    doc["autoMode"] = backlightSettings.autoMode;

    // Zapisz JSON do pliku
    if (serializeJson(doc, file)) {
        #ifdef DEBUG
        Serial.println("Zapisano ustawienia do pliku");
        #endif
    } else {
        #ifdef DEBUG
        Serial.println("Błąd podczas zapisu do pliku");
        #endif
    }
    
    file.close();
}

// wczytywanie ustawień podświetlenia
void loadBacklightSettingsFromFile() {
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        #ifdef DEBUG
        Serial.println("Brak pliku konfiguracyjnego, używam ustawień domyślnych");
        #endif
        // Ustaw wartości domyślne
        backlightSettings.dayBrightness = 100;
        backlightSettings.nightBrightness = 50;
        backlightSettings.autoMode = false;
        // Zapisz domyślne ustawienia do pliku
        saveBacklightSettingsToFile();
        return;
    }

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        #ifdef DEBUG
        Serial.println("Błąd podczas parsowania JSON, używam ustawień domyślnych");
        #endif
        // Ustaw wartości domyślne
        backlightSettings.dayBrightness = 100;
        backlightSettings.nightBrightness = 50;
        backlightSettings.autoMode = false;
        return;
    }

    // Wczytaj ustawienia
    backlightSettings.dayBrightness = doc["dayBrightness"] | 100;
    backlightSettings.nightBrightness = doc["nightBrightness"] | 50;
    backlightSettings.autoMode = doc["autoMode"] | false;

    #ifdef DEBUG
    Serial.println("Wczytano ustawienia z pliku:");
    Serial.print("Day Brightness: "); Serial.println(backlightSettings.dayBrightness);
    Serial.print("Night Brightness: "); Serial.println(backlightSettings.nightBrightness);
    Serial.print("Auto Mode: "); Serial.println(backlightSettings.autoMode);
    #endif
}

// zapis ustawień ogólnych
void saveGeneralSettingsToFile() {
    File file = LittleFS.open("/general_config.json", "w");
    if (!file) {
        #ifdef DEBUG
        Serial.println("Nie można otworzyć pliku ustawień ogólnych do zapisu");
        #endif
        return;
    }

    StaticJsonDocument<64> doc;
    doc["wheelSize"] = generalSettings.wheelSize;

    if (serializeJson(doc, file) == 0) {
        #ifdef DEBUG
        Serial.println("Błąd podczas zapisu ustawień ogólnych");
        #endif
    }

    file.close();
}

// zapis konfiguracji Bluetooth
void saveBluetoothConfigToFile() {
    File file = LittleFS.open("/bluetooth_config.json", "w");
    if (!file) {
        #ifdef DEBUG
        Serial.println("Nie można otworzyć pliku konfiguracji Bluetooth");
        #endif
        return;
    }

    StaticJsonDocument<64> doc;
    doc["bmsEnabled"] = bluetoothConfig.bmsEnabled;
    doc["tpmsEnabled"] = bluetoothConfig.tpmsEnabled;

    serializeJson(doc, file);
    file.close();
}

// wczytywanie konfiguracji Bluetooth
void loadBluetoothConfigFromFile() {
    File file = LittleFS.open("/bluetooth_config.json", "r");
    if (!file) {
        #ifdef DEBUG
        Serial.println("Nie znaleziono pliku konfiguracji Bluetooth, używam domyślnych");
        #endif
        return;
    }

    StaticJsonDocument<64> doc;
    DeserializationError error = deserializeJson(doc, file);
    
    if (!error) {
        bluetoothConfig.bmsEnabled = doc["bmsEnabled"] | false;
        bluetoothConfig.tpmsEnabled = doc["tpmsEnabled"] | false;
    }
    
    file.close();
}

// wczytywanie ustawień ogólnych
void loadGeneralSettingsFromFile() {
    File file = LittleFS.open("/general_config.json", "r");
    if (!file) {
        #ifdef DEBUG
        Serial.println("Nie znaleziono pliku ustawień ogólnych, używam domyślnych");
        #endif
        generalSettings.wheelSize = 26; // Wartość domyślna
        saveGeneralSettingsToFile(); // Zapisz domyślne ustawienia
        return;
    }

    StaticJsonDocument<64> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        #ifdef DEBUG
        Serial.println("Błąd podczas parsowania JSON ustawień ogólnych");
        #endif
        generalSettings.wheelSize = 26; // Wartość domyślna
        saveGeneralSettingsToFile(); // Zapisz domyślne ustawienia
        return;
    }

    generalSettings.wheelSize = doc["wheelSize"] | 26; // Domyślnie 26 cali jeśli nie znaleziono

    #ifdef DEBUG
    Serial.print("Loaded wheel size: ");
    Serial.println(generalSettings.wheelSize);
    #endif
}

// --- Funkcje wyświetlacza ---

// rysowanie linii poziomej
void drawHorizontalLine() {
    display.drawHLine(4, 12, 122);
    display.drawHLine(4, 48, 122);
}

// rysowanie linii pionowej
void drawVerticalLine() {
    display.drawVLine(25, 16, 28);
    display.drawVLine(64, 16, 28);
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

    switch (lightMode) {
        case 1:
            display.drawStr(30, 45, "Dzien");
            break;
        case 2:
            display.drawStr(30, 45, "Noc");
            break;
    }
}

// wyświetlanie poziomu wspomagania
void drawAssistLevel() {
    display.setFont(czcionka_duza);

    if (assistLevelAsText) {
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
    switch (assistMode) {
        case 0:
            modeText = "";     // Pusty tekst dla trybu 0
            modeText2 = "STOP";
            break;
        case 1:
            modeText = "PAS";
            modeText2 = "";
            break;
        case 2:           
            modeText = "TENS";
            modeText2 = "";
            break;
        case 3:
            modeText = "GAZ";
            modeText2 = "";
            break;
        case 4:
            modeText = "MIX";
            modeText2 = "";
            break;
        default:
            modeText = "";
            modeText2 = "";
            break;
    }
    display.drawStr(30, 23, modeText);  // wyświetl rodzaj sterowania
    display.drawStr(30, 34, modeText2);  // wyświetl STOP przy hamowaniu
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

// Implementacja głównego ekranu
void drawMainDisplay() {
    display.setFont(czcionka_mala);
    char valueStr[10];
    const char* unitStr;
    const char* descText;

    if (inSubScreen) {

        switch (currentMainScreen) {
      
            case SPEED_SCREEN:
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

            case CADENCE_SCREEN:
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
                    case CADENCE_SUB_COUNT: 
                        sprintf(valueStr, "%4d", cadence_max_rpm);
                        unitStr = "RPM";
                        descText = ">Kadencja MAX";
                        break;
                }
                break;

            case TEMP_SCREEN:
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

            case RANGE_SCREEN:
                switch (currentSubScreen) {
                    case RANGE_KM:
                        sprintf(valueStr, "%4.1f", range_km);
                        unitStr = "km";
                        descText = ">Zasieg";
                        break;
                    case DISTANCE_KM:
                        sprintf(valueStr, "%4.1f", distance_km);
                        unitStr = "km";
                        descText = ">Dystans";
                        break;
                    case ODOMETER_KM:
                        sprintf(valueStr, "%4.0f", odometer.getRawTotal());
                        unitStr = "km";
                        descText = ">Przebieg";
                        break;
                }
                break;

            case BATTERY_SCREEN:
                switch (currentSubScreen) {
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
                    case BATTERY_CAPACITY_WH:
                        sprintf(valueStr, "%4.0f", battery_capacity_wh);
                        unitStr = "Wh";
                        descText = ">Energia";
                        break;
                    case BATTERY_CAPACITY_AH:
                        sprintf(valueStr, "%4.1f", battery_capacity_wh);
                        unitStr = "Ah";
                        descText = ">Pojemnosc";
                        break;
                    case BATTERY_CAPACITY_PERCENT:
                        sprintf(valueStr, "%3d", battery_capacity_percent);
                        unitStr = "%";
                        descText = ">Bateria";
                        break;
                }
                break;

            case POWER_SCREEN:
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

            case PRESSURE_SCREEN:
                char combinedStr[16];
                switch (currentSubScreen) {
                    case PRESSURE_BAR:
                        sprintf(combinedStr, "%.2f|%.2f", pressure_bar, pressure_rear_bar);
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

            case SPEED_SCREEN:
                sprintf(valueStr, "%4.1f", speed_kmh);
                unitStr = "km/h";
                descText = " Predkosc";
                break;
          
            case CADENCE_SCREEN:
                sprintf(valueStr, "%4d", cadence_rpm);
                unitStr = "RPM";
                descText = " Kadencja";
                break;

            case TEMP_SCREEN:
                if (currentTemp != TEMP_ERROR && currentTemp != DEVICE_DISCONNECTED_C) {
                    sprintf(valueStr, "%4.1f", currentTemp);
                } else {
                    strcpy(valueStr, "---");
                }
                unitStr = "C";
                descText = " Temperatura";
                break;

            case RANGE_SCREEN:
                sprintf(valueStr, "%4.1f", range_km);
                unitStr = "km";
                descText = " Zasieg";
                break;

            case BATTERY_SCREEN:
                sprintf(valueStr, "%3d", battery_capacity_percent);
                unitStr = "%";
                descText = " Bateria";
                break;

            case POWER_SCREEN:
                sprintf(valueStr, "%4d", power_w);
                unitStr = "W";
                descText = " Moc";
                break;

            case PRESSURE_SCREEN:
                sprintf(valueStr, "%.1f/%.1f", pressure_bar, pressure_rear_bar);
                unitStr = "bar";
                descText = " Kola";
                break;

            case USB_SCREEN:
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
    String versionText = "System ver. ";
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

    static unsigned long lastButtonCheck = 0;
    static unsigned long bothPressStart = 0;
    static bool upPressed = false;
    static bool downPressed = false;
    static unsigned long upPressTime = 0;
    static unsigned long downPressTime = 0;
    const unsigned long buttonDebounce = 50;
    static unsigned long legalModeStart = 0;

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
            } else if ((currentTime - legalModeStart) > 500) { // 2 sekundy  przytrzymania
                toggleLegalMode();
                while (!digitalRead(BTN_UP) || !digitalRead(BTN_SET)) {
                    delay(10); // Czekaj na puszczenie przycisków
                }
                legalModeStart = 0;
                return;
            }
        } else {
            legalModeStart = 0;
        }

        // Obsługa przycisku UP (zmiana asysty)
        if (!upState && (currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
            if (!upPressStartTime) {
                upPressStartTime = currentTime;
            } else if (!upLongPressExecuted && (currentTime - upPressStartTime) > LONG_PRESS_TIME) {
                lightMode = (lightMode + 1) % 3;
                
                #ifdef DEBUG
                Serial.print("Zmieniono tryb świateł na: ");
                Serial.println(lightMode);
                #endif
                
                setLights(); // Zastosuj ustawienia zgodnie z trybem
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

        // Obsługa przycisku DOWN (zmiana asysty)
        if (!downState && (currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
            if (!downPressStartTime) {
                downPressStartTime = currentTime;
            } else if (!downLongPressExecuted && (currentTime - downPressStartTime) > LONG_PRESS_TIME) {
                assistLevelAsText = !assistLevelAsText;
                downLongPressExecuted = true;
            }
        } else if (downState && downPressStartTime) {
            if (!downLongPressExecuted && (currentTime - downPressStartTime) < LONG_PRESS_TIME) {
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
    
    // 1. Inicjalizacja LittleFS
    if (!LittleFS.begin(true)) {
        #ifdef DEBUG
        Serial.println("Błąd montowania LittleFS");
        #endif
        return;
    }
    #ifdef DEBUG
    Serial.println("LittleFS zainicjalizowany");
    #endif

    // 2. Włączenie WiFi w trybie AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP("e-Bike System PMW", "#mamrower");
    #ifdef DEBUG
    Serial.println("Tryb AP aktywny");
    #endif

    // 3. Konfiguracja serwera - najpierw pliki statyczne
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    
    // Dodanie obsługi typów MIME
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/style.css", "text/css");
    });
    
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/script.js", "application/javascript");
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
    #ifdef DEBUG
    Serial.println("Serwer WWW uruchomiony");
    #endif
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

// tryb uśpienia
void goToSleep() {
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

    // Zapisz licznik całkowity
    //odometer.shutdown();
    //odometer.save();

    // Konfiguracja wybudzania przez przycisk SET
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, 0);  // GPIO12 (BTN_SET) stan niski

    // Wejście w deep sleep
    esp_deep_sleep_start();
}

// ustawiania świateł
void setLights() {
    // Wyłącz wszystkie światła
    digitalWrite(FrontDayPin, LOW);
    digitalWrite(FrontPin, LOW);
    digitalWrite(RearPin, LOW);

    // Jeśli światła wyłączone (lightMode == 0), kończymy
    if (lightMode == 0) {
        #ifdef DEBUG
        Serial.println("Światła wyłączone");
        #endif
        return;
    }

    // Zastosuj ustawienia zgodnie z trybem
    if (lightMode == 1) { // Tryb dzienny
        switch (lightSettings.dayLights) {
            case LightSettings::NONE:
                // Wszystkie światła pozostają wyłączone
                break;
            case LightSettings::FRONT:
                digitalWrite(FrontDayPin, HIGH);
                break;
            case LightSettings::REAR:
                digitalWrite(RearPin, HIGH);
                break;
            case LightSettings::BOTH:
                digitalWrite(FrontDayPin, HIGH);
                digitalWrite(RearPin, HIGH);
                break;
        }
    } else if (lightMode == 2) { // Tryb nocny
        switch (lightSettings.nightLights) {
            case LightSettings::NONE:
                // Wszystkie światła pozostają wyłączone
                break;
            case LightSettings::FRONT:
                digitalWrite(FrontPin, HIGH);
                break;
            case LightSettings::REAR:
                digitalWrite(RearPin, HIGH);
                break;
            case LightSettings::BOTH:
                digitalWrite(FrontPin, HIGH);
                digitalWrite(RearPin, HIGH);
                break;
        }
    }
    
    // Dodaj wywołanie funkcji aktualizującej jasność wyświetlacza
    applyBacklightSettings();
}

// ustawienia podświetlenia
void applyBacklightSettings() {
    int targetBrightness;
    
    if (!backlightSettings.autoMode) {
        // Tryb manualny - użyj podstawowej jasności
        targetBrightness = backlightSettings.Brightness;
    } else {
        // Tryb auto - sprawdź stan świateł
        if (lightMode == 2) {  // Światła nocne
            targetBrightness = backlightSettings.nightBrightness;
        } else {  // Światła dzienne (lightMode == 1) lub wyłączone (lightMode == 0)
            targetBrightness = backlightSettings.dayBrightness;
        }
    }
    
    // Nieliniowe mapowanie jasności
    // Używamy funkcji wykładniczej do lepszego rozłożenia jasności
    // Wzór: (x^2)/100 daje nam wartość od 0 do 100
    float normalized = (targetBrightness * targetBrightness) / 100.0;
    
    // Mapujemy wartość na zakres 16-255
    // Minimum ustawiamy na 16, bo niektóre wyświetlacze OLED mogą się wyłączać przy niższych wartościach
    displayBrightness = map(normalized, 0, 100, 16, 255);
    
    // Zastosuj jasność do wyświetlacza
    display.setContrast(displayBrightness);
    
    #ifdef DEBUG
    Serial.print("Target brightness: ");
    Serial.print(targetBrightness);
    Serial.print("%, Normalized: ");
    Serial.print(normalized);
    Serial.print("%, Display brightness: ");
    Serial.println(displayBrightness);
    #endif
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
        #ifdef DEBUG
        Serial.println("Failed to open config file");
        #endif
        return;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, configFile);

    if (error) {
        #ifdef DEBUG
        Serial.println("Failed to parse config file");
        #endif
        return;
    }

    // Wczytywanie ustawień świateł
    if (doc.containsKey("light")) {
        lightSettings.dayLights = static_cast<LightSettings::LightMode>(doc["light"]["dayLights"] | 0);
        lightSettings.nightLights = static_cast<LightSettings::LightMode>(doc["light"]["nightLights"] | 0);
        lightSettings.dayBlink = doc["light"]["dayBlink"] | false;
        lightSettings.nightBlink = doc["light"]["nightBlink"] | false;
        lightSettings.blinkFrequency = doc["light"]["blinkFrequency"] | 500;
    }

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

    // Zapisywanie ustawień świateł
    JsonObject lightObj = doc.createNestedObject("light");
    lightObj["dayLights"] = static_cast<int>(lightSettings.dayLights);
    lightObj["nightLights"] = static_cast<int>(lightSettings.nightLights);
    lightObj["dayBlink"] = lightSettings.dayBlink;
    lightObj["nightBlink"] = lightSettings.nightBlink;
    lightObj["blinkFrequency"] = lightSettings.blinkFrequency;

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
    #ifdef DEBUG
    Serial.println("Failed to open config file for writing");
    #endif
    return;
  }

  if (serializeJson(doc, configFile) == 0) {
    #ifdef DEBUG
    Serial.println("Failed to write config file");
    #endif
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

// konfiguracja serwera WWW
void setupWebServer() {
    // Serwowanie plików statycznych
    server.serveStatic("/", LittleFS, "/");

    // Licznik całkowity 
    server.on("/api/odometer", HTTP_GET, [](AsyncWebServerRequest *request) {
        #ifdef DEBUG
        Serial.print("Odczyt licznika: ");
        Serial.println(odometer.getRawTotal());
        #endif
        request->send(200, "text/plain", String(odometer.getRawTotal()));
    });

    server.on("/api/setOdometer", HTTP_POST, [](AsyncWebServerRequest *request) {
        #ifdef DEBUG
        Serial.println("Otrzymano żądanie POST /api/setOdometer");
        #endif

        if (request->hasParam("value", true)) {
            float newValue = request->getParam("value", true)->value().toFloat();
            
            #ifdef DEBUG
            Serial.print("Otrzymana wartość: ");
            Serial.println(newValue);
            Serial.print("Stan licznika przed zapisem: ");
            Serial.println(odometer.getRawTotal());
            Serial.print("Licznik zainicjalizowany: ");
            Serial.println(odometer.isValid() ? "TAK" : "NIE");
            #endif

            bool success = odometer.setInitialValue(newValue);
            
            #ifdef DEBUG
            Serial.print("Wynik zapisu: ");
            Serial.println(success ? "OK" : "BŁĄD");
            Serial.print("Stan licznika po zapisie: ");
            Serial.println(odometer.getRawTotal());
            #endif

            request->send(success ? 200 : 400, "text/plain", 
                        success ? "OK" : "Invalid value");
        } else {
            #ifdef DEBUG
            Serial.println("Błąd: brak parametru value");
            #endif
            request->send(400, "text/plain", "Missing value");
        }
    });

    // Światła
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        StaticJsonDocument<512> doc;
        JsonObject lightsObj = doc.createNestedObject("lights");
        
        lightsObj["dayLights"] = getLightModeString(lightSettings.dayLights);
        lightsObj["nightLights"] = getLightModeString(lightSettings.nightLights);
        lightsObj["dayBlink"] = lightSettings.dayBlink;
        lightsObj["nightBlink"] = lightSettings.nightBlink;
        lightsObj["blinkFrequency"] = lightSettings.blinkFrequency;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/api/lights/config", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (request->hasParam("data", true)) {
            String jsonString = request->getParam("data", true)->value();
            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, jsonString);

            if (!error) {
                const char* dayLightsStr = doc["dayLights"] | "OFF";
                const char* nightLightsStr = doc["nightLights"] | "OFF";
                
                // Konwersja stringów na enum
                if (strcmp(dayLightsStr, "OFF") == 0) 
                    lightSettings.dayLights = LightSettings::NONE;
                else if (strcmp(dayLightsStr, "FRONT") == 0) 
                    lightSettings.dayLights = LightSettings::FRONT;
                else if (strcmp(dayLightsStr, "REAR") == 0) 
                    lightSettings.dayLights = LightSettings::REAR;
                else if (strcmp(dayLightsStr, "BOTH") == 0) 
                    lightSettings.dayLights = LightSettings::BOTH;
                
                if (strcmp(nightLightsStr, "OFF") == 0)
                    lightSettings.nightLights = LightSettings::NONE;
                else if (strcmp(nightLightsStr, "FRONT") == 0)
                    lightSettings.nightLights = LightSettings::FRONT;
                else if (strcmp(nightLightsStr, "REAR") == 0)
                    lightSettings.nightLights = LightSettings::REAR;
                else if (strcmp(nightLightsStr, "BOTH") == 0)
                    lightSettings.nightLights = LightSettings::BOTH;
                
                lightSettings.dayBlink = doc["dayBlink"] | false;
                lightSettings.nightBlink = doc["nightBlink"] | false;
                lightSettings.blinkFrequency = doc["blinkFrequency"] | 500;
                
                // Zapisz do pliku
                saveLightSettings();
                
                // Od razu zastosuj nowe ustawienia jeśli jakiś tryb jest aktywny
                if (lightMode > 0) {
                    setLights();
                }
                
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            }
        }
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
        
        #ifdef DEBUG
        //Serial.print("Wysyłam aktualny czas: ");
        //Serial.println(response);
        #endif
        
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
                    
                    #ifdef DEBUG
                    Serial.println("Czas został zaktualizowany:");
                    Serial.print(year); Serial.print("-");
                    Serial.print(month); Serial.print("-");
                    Serial.print(day); Serial.print(" ");
                    Serial.print(hour); Serial.print(":");
                    Serial.print(minute); Serial.print(":");
                    Serial.println(second);
                    #endif
                    
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                } else {
                    #ifdef DEBUG
                    Serial.println("Błędne wartości daty/czasu");
                    #endif
                    request->send(400, "application/json", "{\"error\":\"Invalid date/time values\"}");
                }
            } else {
                #ifdef DEBUG
                Serial.println("Błędny format JSON");
                #endif
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            }
    });
    
    server.on("/api/display/config", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index + len != total) {
                // Jeśli to nie jest ostatni fragment danych, czekamy na więcej
                return;
            }

            // Dodaj null terminator do danych
            data[len] = '\0';
            
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, (const char*)data);
            
            if (!error) {
                // Aktualizacja ustawień
                backlightSettings.dayBrightness = doc["dayBrightness"] | backlightSettings.dayBrightness;
                backlightSettings.nightBrightness = doc["nightBrightness"] | backlightSettings.nightBrightness;
                backlightSettings.autoMode = doc["autoMode"] | backlightSettings.autoMode;
                
                // Zapisz do pliku
                saveBacklightSettingsToFile();
                
                // Zastosuj nowe ustawienia
                applyBacklightSettings();
                
                // Odpowiedz używając send()
                String response = "{\"status\":\"ok\"}";
                request->send(200, "application/json", response);
            } else {
                String response = "{\"status\":\"error\",\"message\":\"Invalid JSON\"}";
                request->send(400, "application/json", response);
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
                    #ifdef DEBUG
                    Serial.println("General settings saved");
                    Serial.print("Wheel size: ");
                    Serial.println(generalSettings.wheelSize);
                    #endif
                }

                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
            }
    });

    // Dodaj w setupWebServer():
    server.on("/get-bluetooth-config", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<64> doc;
        doc["bmsEnabled"] = bluetoothConfig.bmsEnabled;
        doc["tpmsEnabled"] = bluetoothConfig.tpmsEnabled;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/save-bluetooth-config", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("body", true)) {
            String json = request->getParam("body", true)->value();
            StaticJsonDocument<64> doc;
            DeserializationError error = deserializeJson(doc, json);

            if (!error) {
                bluetoothConfig.bmsEnabled = doc["bmsEnabled"] | false;
                bluetoothConfig.tpmsEnabled = doc["tpmsEnabled"] | false;
                
                saveBluetoothConfigToFile();
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
            }
        } else {
            request->send(400, "application/json", "{\"success\":false,\"error\":\"No data\"}");
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
                #ifdef DEBUG
                Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
                #endif
                break;
            case WS_EVT_DISCONNECT:
                #ifdef DEBUG
                Serial.printf("WebSocket client #%u disconnected\n", client->id());
                #endif
                break;
        }
    });
    server.addHandler(&ws);

    // Start serwera
    server.begin();
}

// zapis ustawień zegara
// void handleSaveClockSettings(AsyncWebServerRequest *request) {
//     if (request->hasParam("hour")) {
//         int hour = request->getParam("hour")->value().toInt();
//     }
//     request->redirect("/");
// }

// zapis ustawień roweru
// void handleSaveBikeSettings(AsyncWebServerRequest *request) {
//     if (request->hasParam("wheel")) {
//         bikeSettings.wheelCircumference = request->getParam("wheel")->value().toInt();
//     }
//     request->redirect("/");
// }

// --- Funkcje systemu plików ---

// Sprawdzenie i formatowanie systemu plików przy starcie
void initLittleFS() {
    if (!LittleFS.begin(true)) {
        #ifdef DEBUG
        Serial.println("LittleFS Mount Failed");
        #endif
        if (!LittleFS.format()) {
            #ifdef DEBUG
            Serial.println("LittleFS Format Failed");
            #endif
            return;
        }
        if (!LittleFS.begin()) {
            #ifdef DEBUG
            Serial.println("LittleFS Mount Failed After Format");
            #endif
            return;
        }
    }
    #ifdef DEBUG
    Serial.println("LittleFS Mounted Successfully");
    #endif
}

// listowanie plików
void listFiles() {
    #ifdef DEBUG
    Serial.println("Files in LittleFS:");
    #endif
    File root = LittleFS.open("/");
    if (!root) {
        #ifdef DEBUG
        Serial.println("- Failed to open directory");
        #endif
        return;
    }
    if (!root.isDirectory()) {
        #ifdef DEBUG
        Serial.println(" - Not a directory");
        #endif
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            #ifdef DEBUG
            Serial.print("  DIR : ");
            Serial.println(file.name());
            #endif
        } else {
            #ifdef DEBUG
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
            #endif
        }
        file = root.openNextFile();
    }
}

// wczytywanie konfiguracji
bool loadConfig() {
    if(!LittleFS.exists("/config.json")) {
        #ifdef DEBUG
        Serial.println("Creating default config file...");
        #endif
        // Tworzymy domyślną konfigurację
        StaticJsonDocument<512> defaultConfig;
        defaultConfig["version"] = "1.0.0";
        defaultConfig["timezone"] = "Europe/Warsaw";
        defaultConfig["lastUpdate"] = "";
        
        File configFile = LittleFS.open("/config.json", "w");
        if(!configFile) {
            #ifdef DEBUG
            Serial.println("Failed to create config file");
            #endif
            return false;
        }
        serializeJson(defaultConfig, configFile);
        configFile.close();
    }
    
    // Czytamy konfigurację
    File configFile = LittleFS.open("/config.json", "r");
    if(!configFile) {
        #ifdef DEBUG
        Serial.println("Failed to open config file");
        #endif
        return false;
    }
    
    #ifdef DEBUG
    Serial.println("Config file loaded successfully");
    #endif
    return true;
}

// --- Funkcje czasu ---

// Synchronizacja RTC z NTP
void syncRTCWithNTP() {
    configTime(0, 0, "pool.ntp.org");
    delay(1000);
    time_t now;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        rtc.adjust(DateTime(
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec
        ));
    }
}

// synchronizacja czasu NTP
void synchronizeTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    #ifdef DEBUG
    Serial.println("Waiting for NTP time sync...");
    #endif
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        #ifdef DEBUG
        Serial.print(".");
        #endif
        now = time(nullptr);
    }
    Serial.println();

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                        timeinfo.tm_mday, timeinfo.tm_hour,
                        timeinfo.tm_min, timeinfo.tm_sec));
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
    timeSettings.year = 2024;

    // Ustawienia świateł
    lightSettings.dayLights = LightSettings::FRONT;
    lightSettings.nightLights = LightSettings::BOTH;
    lightSettings.dayBlink = false;
    lightSettings.nightBlink = false;
    lightSettings.blinkFrequency = 500;

    // Podświetlenie
    backlightSettings.dayBrightness = 100;
    backlightSettings.nightBrightness = 50;
    backlightSettings.autoMode = false;

    // WiFi - początkowo puste
    memset(wifiSettings.ssid, 0, sizeof(wifiSettings.ssid));
    memset(wifiSettings.password, 0, sizeof(wifiSettings.password));
}

// --- Główne funkcje programu ---

// SETUP
void setup() { 
    // Sprawdź przyczynę wybudzenia
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    Serial.begin(115200);
    
    #ifdef DEBUG
    Serial.println("\n=== Inicjalizacja systemu ===");
    #endif
    
    // Najpierw wyczyść NVS
    esp_err_t err = nvs_flash_erase();
    if (err != ESP_OK) {
        #ifdef DEBUG
        Serial.printf("Błąd podczas czyszczenia NVS: %d\n", err);
        #endif
    }
    
    // Inicjalizacja NVS
    err = nvs_flash_init();
    if (err != ESP_OK) {
        #ifdef DEBUG
        Serial.printf("Błąd podczas inicjalizacji NVS: %d\n", err);
        #endif
        return;
    }
    
    #ifdef DEBUG
    Serial.println("NVS zainicjalizowane pomyślnie");
    
    // Sprawdź statystyki NVS
    nvs_stats_t nvs_stats;
    err = nvs_get_stats(NULL, &nvs_stats);
    if (err == ESP_OK) {
        Serial.println("\n=== Statystyki NVS ===");
        Serial.printf("Użyte wpisy: %d\n", nvs_stats.used_entries);
        Serial.printf("Wolne wpisy: %d\n", nvs_stats.free_entries);
        Serial.printf("Całkowita liczba wpisów: %d\n", nvs_stats.total_entries);
        Serial.println("===================\n");
    }
    #endif

    // Poczekaj chwilę przed dalszą inicjalizacją
    delay(100);

    // Inicjalizacja I2C
    Wire.begin();

    // Inicjalizacja wyświetlacza
    display.begin();
    display.setFontDirection(0);
    display.clearBuffer();
    display.sendBuffer();

    // Inicjalizacja DS18B20
    sensorsAir.begin();
    sensorsController.begin();
    sensorsAir.setWaitForConversion(false);      // Tryb nieblokujący
    sensorsController.setWaitForConversion(false);// Tryb nieblokujący
    sensorsAir.setResolution(12);                // Najwyższa rozdzielczość
    sensorsController.setResolution(12);         // Najwyższa rozdzielczość
    
    // Pierwsze żądanie pomiaru
    sensorsAir.requestTemperatures();
    sensorsController.requestTemperatures();

    // Inicjalizacja RTC
    if (!rtc.begin()) {
        #ifdef DEBUG
        Serial.println("Couldn't find RTC");
        #endif
        while (1);
    }

    if (rtc.lostPower()) {
        #ifdef DEBUG
        Serial.println("RTC lost power, lets set the time!");
        #endif
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    // Konfiguracja pinów
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_SET, INPUT_PULLUP);

    // Konfiguracja pinów LED
    pinMode(FrontDayPin, OUTPUT);
    pinMode(FrontPin, OUTPUT);
    pinMode(RearPin, OUTPUT);
    digitalWrite(FrontDayPin, LOW);
    digitalWrite(FrontPin, LOW);
    digitalWrite(RearPin, LOW);

    // kadencja
    pinMode(CADENCE_SENSOR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(CADENCE_SENSOR_PIN), cadence_ISR, FALLING);

    // Ładowarka USB
    pinMode(UsbPin, OUTPUT);
    digitalWrite(UsbPin, LOW);

    // Inicjalizacja LittleFS i wczytanie ustawień
    if (!LittleFS.begin(true)) {
        #ifdef DEBUG
        Serial.println("Błąd montowania LittleFS");
        #endif
    } else {
        #ifdef DEBUG
        Serial.println("LittleFS zamontowany pomyślnie");
        #endif
        // Wczytaj ustawienia z pliku
        loadSettings();              // Wczytaj główne ustawienia
        loadLightSettings();         // Wczytaj ustawienia świateł
        loadBacklightSettingsFromFile();
        loadGeneralSettingsFromFile();
        loadBluetoothConfigFromFile();
    }

    // Inicjalizacja licznika
    //odometer.initialize();
    //odometer.begin();
    #ifdef DEBUG
    Serial.println("Stan licznika po inicjalizacji:");
    Serial.print("Zainicjalizowany: ");
    Serial.println(odometer.isValid() ? "TAK" : "NIE");
    Serial.print("Wartość: ");
    Serial.println(odometer.getRawTotal());
    #endif

    // Inicjalizacja BLE
    if (bluetoothConfig.bmsEnabled || bluetoothConfig.tpmsEnabled) {
        BLEDevice::init("e-Bike System PMW");
        bleClient = BLEDevice::createClient();
        if (bluetoothConfig.bmsEnabled) {
            connectToBms();
        }
    }

    // Zastosuj wczytane ustawienia
    setLights();  
    applyBacklightSettings();

    #ifdef DEBUG
        Serial.println("\n--- Memory Info ---");
        Serial.printf("Total heap: %d\n", ESP.getHeapSize());
        Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
        Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
        Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());
        
        Serial.println("\n--- Flash Info ---");
        Serial.printf("Flash size: %d\n", ESP.getFlashChipSize());
        Serial.printf("Sketch size: %d\n", ESP.getSketchSize());
        Serial.printf("Free sketch space: %d\n", ESP.getFreeSketchSpace());
        
        Serial.println("\n--- Partition Info ---");
        esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
        while (pi != NULL) {
            const esp_partition_t* partition = esp_partition_get(pi);
            Serial.printf("Partition '%s': size %d\n", partition->label, partition->size);
            pi = esp_partition_next(pi);
        }
        esp_partition_iterator_release(pi);
        Serial.println("-------------------\n");
    #endif

    // Jeśli wybudzenie przez przycisk SET
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        unsigned long startTime = millis();
        while (!digitalRead(BTN_SET)) {  // Czekaj na puszczenie przycisku
            if ((millis() - startTime) > SET_LONG_PRESS) {
                displayActive = true;
                showingWelcome = true;
                messageStartTime = millis();
                if (!welcomeAnimationDone) {
                    showWelcomeMessage();  // Pokaż animację powitania
                }                
                while (!digitalRead(BTN_SET)) {  // Czekaj na puszczenie przycisku
                    delay(10);
                }
                break;
            }
            delay(10);
        }
    }
}

// Implementacja funkcji loop
void loop() {
    static unsigned long lastButtonCheck = 0;
    static unsigned long lastUpdate = 0;
    const unsigned long buttonInterval = 5;
    const unsigned long updateInterval = 2000;

    unsigned long currentTime = millis();

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
            } else if (millis() - setPressStartTime > 50) {
                deactivateConfigMode();
                setPressStartTime = 0;
            }
        } else {
            setPressStartTime = 0;
        }
        return;
    }

    // Sprawdzaj czy nie trzeba włączyć trybu konfiguracji
    checkConfigMode();

    if (currentTime - lastButtonCheck >= buttonInterval) {
        handleButtons();
        lastButtonCheck = currentTime;
    }

    static unsigned long lastWebSocketUpdate = 0;
    if (currentTime - lastWebSocketUpdate >= 1000) { // Aktualizuj co sekundę
        if (ws.count() > 0) {
            String json = "{";
            json += "\"speed\":" + String(speed_kmh) + ",";
            json += "\"temperature\":" + String(currentTemp) + ",";
            json += "\"battery\":" + String(battery_capacity_percent) + ",";
            json += "\"power\":" + String(power_w);
            json += "}";
            ws.textAll(json);
        }
        lastWebSocketUpdate = currentTime;
    }

    // Aktualizuj wyświetlacz tylko jeśli jest aktywny i nie wyświetla komunikatów
    if (displayActive && messageStartTime == 0) {
        display.clearBuffer();
        drawTopBar();
        drawHorizontalLine();
        drawVerticalLine();
        drawAssistLevel();
        drawMainDisplay();
        drawLightStatus();
        display.sendBuffer();
        handleTemperature();
        updateBmsData();

        static uint32_t last_cadence_pulse_count = 0;
        static unsigned long last_cadence_update_time = 0;
        static uint32_t cadence_sum = 0;
        static uint32_t cadence_samples = 0;

        if (currentTime - last_cadence_update_time >= 1000) { // co sekundę
            uint32_t pulses = cadence_pulse_count - last_cadence_pulse_count;
            last_cadence_pulse_count = cadence_pulse_count;
            cadence_rpm = pulses * 60; // 1 impuls = 1 obrót korby na minutę

            // Liczenie średniej
            cadence_sum += cadence_rpm;
            cadence_samples++;
            if (cadence_samples > 0) {
                cadence_avg_rpm = cadence_sum / cadence_samples;
            }

            // Liczenie maksymalnej kadencji
            if (cadence_rpm > cadence_max_rpm) {
                cadence_max_rpm = cadence_rpm;
            }

            last_cadence_update_time = currentTime;
        }

        if (currentTime - lastUpdate >= updateInterval) {
            speed_kmh = (speed_kmh >= 35.0) ? 0.0 : speed_kmh + 0.1;
            temp_motor = 30.0 + random(20);
            range_km = 50.0 - (random(20) / 10.0);
            // Aktualizacja dystansu            
            //odometer.update(distance_km);
            odometer.updateTotal(distance_km);
            distance_km += 0.1;
            power_w = 100 + random(300);
            power_avg_w = power_w * 0.8;
            power_max_w = power_w * 1.2;
            battery_current = random(50, 150) / 10.0;
            battery_capacity_wh = battery_voltage * battery_capacity_ah;
            pressure_bar = 2.0 + (random(20) / 10.0);
            pressure_voltage = 0.5 + (random(20) / 100.0);
            pressure_temp = 20.0 + (random(100) / 10.0);
            battery_capacity_wh = 14.5 - (random(20) / 10.0);
            battery_capacity_percent = (battery_capacity_percent <= 0) ? 100 : battery_capacity_percent - 1;
            battery_voltage = (battery_voltage <= 42.0) ? 50.0 : battery_voltage - 0.1;
            assistMode = (assistMode + 1) % 5;
            lastUpdate = currentTime;
            pressure_rear_bar = 2.0 + (random(20) / 10.0);
            pressure_rear_voltage = 0.5 + (random(20) / 100.0);
            pressure_rear_temp = 20.0 + (random(100) / 10.0);
            // Aktualizacja średniej prędkości (przykładowa implementacja)
            static float speed_sum = 0;
            static int speed_count = 0;
            speed_sum += speed_kmh;
            speed_count++;
            speed_avg_kmh = speed_sum / speed_count;
            // Aktualizacja maksymalnej prędkości
            if (speed_kmh > speed_max_kmh) {
                speed_max_kmh = speed_kmh;
            }

            // Aktualizacja średniej kadencji (przykładowa implementacja)
            static int cadence_sum = 0;
            static int cadence_count = 0;
            cadence_sum += cadence_rpm;
            cadence_count++;
            cadence_avg_rpm = cadence_sum / cadence_count;
        }
    }
}
