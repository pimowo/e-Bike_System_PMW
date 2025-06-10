// DebugUtils.h
#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <Arduino.h>

/********************************************************************
 * SYSTEM LOGOWANIA Z POZIOMAMI
 ********************************************************************/

// Główny przełącznik debugowania - zakomentuj aby wyłączyć całe logowanie
#define DEBUG

// Definiuje poziomy logowania (ustaw 1 aby włączyć, 0 aby wyłączyć)
#define DEBUG_ERROR_ENABLED   1  // Błędy krytyczne
#define DEBUG_WARN_ENABLED    1  // Ostrzeżenia
#define DEBUG_INFO_ENABLED    1  // Informacje ogólne
#define DEBUG_LIGHT_ENABLED   1  // Logi dotyczące oświetlenia
#define DEBUG_TEMP_ENABLED    1  // Logi czujników temperatury
#define DEBUG_BLE_ENABLED     1  // Logi komunikacji Bluetooth
#define DEBUG_DETAIL_ENABLED  1  // Szczegółowe logi

// Makra do logowania z tagami
#ifdef DEBUG
    // Aktywne tylko gdy odpowiedni poziom jest włączony (1)
    #if DEBUG_ERROR_ENABLED
        #define DEBUG_ERROR(...) Serial.printf("[ERROR] " __VA_ARGS__); Serial.println()
    #else
        #define DEBUG_ERROR(...) ((void)0)
    #endif
    
    #if DEBUG_WARN_ENABLED
        #define DEBUG_WARN(...) Serial.printf("[WARN] " __VA_ARGS__); Serial.println()
    #else
        #define DEBUG_WARN(...) ((void)0)
    #endif
    
    #if DEBUG_INFO_ENABLED
        #define DEBUG_INFO(...) Serial.printf("[INFO] " __VA_ARGS__); Serial.println()
    #else
        #define DEBUG_INFO(...) ((void)0)
    #endif
    
    #if DEBUG_LIGHT_ENABLED
        #define DEBUG_LIGHT(...) Serial.printf("[LIGHT] " __VA_ARGS__); Serial.println()
    #else
        #define DEBUG_LIGHT(...) ((void)0)
    #endif
    
    #if DEBUG_TEMP_ENABLED
        #define DEBUG_TEMP(...) Serial.printf("[TEMP] " __VA_ARGS__); Serial.println()
    #else
        #define DEBUG_TEMP(...) ((void)0)
    #endif
    
    #if DEBUG_BLE_ENABLED
        #define DEBUG_BLE(...) Serial.printf("[BLE] " __VA_ARGS__); Serial.println()
    #else
        #define DEBUG_BLE(...) ((void)0)
    #endif
    
    #if DEBUG_DETAIL_ENABLED
        #define DEBUG_DETAIL(...) Serial.printf("[DETAIL] " __VA_ARGS__); Serial.println()
    #else
        #define DEBUG_DETAIL(...) ((void)0)
    #endif
#else
    // Gdy DEBUG jest wyłączony, wszystkie makra są puste
    #define DEBUG_ERROR(...) ((void)0)
    #define DEBUG_WARN(...) ((void)0)
    #define DEBUG_INFO(...) ((void)0)
    #define DEBUG_LIGHT(...) ((void)0)
    #define DEBUG_TEMP(...) ((void)0)
    #define DEBUG_BLE(...) ((void)0)
    #define DEBUG_DETAIL(...) ((void)0)
#endif

#endif // DEBUG_UTILS_H
