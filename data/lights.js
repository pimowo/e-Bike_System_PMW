/**
 * e-Bike System PMW - Moduł konfiguracji świateł
 * Zoptymalizowana wersja kodu
 */

// Stałe konfiguracyjne
const DEBUG = false;
const DEFAULT_BLINK_FREQUENCY = 500;
const DEFAULT_CONFIG = {
    controlMode: 0, // 0 = smart, 1 = controller
    dayLights: 'DRL+REAR',
    nightLights: 'FRONT+REAR',
    dayBlink: true,
    nightBlink: false,
    blinkFrequency: DEFAULT_BLINK_FREQUENCY
};

// Funkcja do logowania diagnostycznego
function log(message, level = 'log') {
    if (DEBUG || level === 'error') {
        console[level](message);
    }
}

// Funkcja do pobierania referencji do elementów formularza
function getLightFormElements() {
    return {
        controlMode: document.getElementById('light-control-mode'),
        dayLights: document.getElementById('day-lights'),
        nightLights: document.getElementById('night-lights'),
        dayBlink: document.getElementById('day-blink'),
        nightBlink: document.getElementById('night-blink'),
        blinkFrequency: document.getElementById('blink-frequency'),
        saveButton: document.getElementById('save-light-config'),
        smartConfigSection: document.getElementById('smart-config-section')
    };
}

// Funkcja do sprawdzania, czy wszystkie elementy formularza istnieją
function validateFormElements(elements) {
    let valid = true;
    
    for (const [id, element] of Object.entries(elements)) {
        if (!element) {
            log(`Element #${id.replace('elements.', '')} nie został znaleziony w DOM!`, 'warn');
            valid = false;
        }
    }
    
    return valid;
}

// Funkcja do ustawienia wartości formularza na podstawie danych konfiguracyjnych
function updateFormFromConfig(config, elements) {
    if (!config || !elements) return;
    
    if (elements.controlMode) {
        elements.controlMode.value = config.controlMode === 1 ? 'controller' : 'smart';
    }
    
    if (elements.dayLights) elements.dayLights.value = config.dayLights || DEFAULT_CONFIG.dayLights;
    if (elements.nightLights) elements.nightLights.value = config.nightLights || DEFAULT_CONFIG.nightLights;
    
    if (elements.dayBlink) {
        elements.dayBlink.checked = config.dayBlink !== undefined ? config.dayBlink : DEFAULT_CONFIG.dayBlink;
    }
    
    if (elements.nightBlink) {
        elements.nightBlink.checked = config.nightBlink !== undefined ? config.nightBlink : DEFAULT_CONFIG.nightBlink;
    }
    
    if (elements.blinkFrequency) {
        elements.blinkFrequency.value = config.blinkFrequency || DEFAULT_CONFIG.blinkFrequency;
    }
    
    log('Zaktualizowano formularz z konfiguracją');
}

// Funkcja do aktualizacji widoczności sekcji konfiguracji światła
function updateLightConfigVisibility() {
    const elements = getLightFormElements();
    
    if (elements.controlMode && elements.smartConfigSection) {
        const isSmartMode = elements.controlMode.value === 'smart';
        elements.smartConfigSection.style.display = isSmartMode ? 'block' : 'none';
        log(`Zaktualizowano widoczność sekcji konfiguracji - tryb: ${elements.controlMode.value}, widoczność: ${isSmartMode ? 'pokazana' : 'ukryta'}`);
    } else {
        log('Nie znaleziono wymaganych elementów UI dla aktualizacji widoczności', 'warn');
    }
}

// Funkcja do pokazywania/ukrywania stanu ładowania
function showLoading(element, isLoading = true) {
    if (!element) return;
    
    if (isLoading) {
        element.disabled = true;
        element.dataset.originalText = element.textContent;
        element.textContent = 'Ładowanie...';
    } else {
        element.disabled = false;
        element.textContent = element.dataset.originalText || element.textContent;
    }
}

// Funkcja pomocnicza do obsługi żądań HTTP
async function fetchWithTimeout(url, options = {}, timeout = 5000) {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), timeout);
    
    try {
        const response = await fetch(url, {
            ...options,
            signal: controller.signal
        });
        clearTimeout(timeoutId);
        return response;
    } catch (error) {
        clearTimeout(timeoutId);
        if (error.name === 'AbortError') {
            throw new Error('Przekroczono czas oczekiwania na odpowiedź serwera');
        }
        throw error;
    }
}

// Funkcja do ładowania konfiguracji świateł
async function loadLightConfig() {
    log('Ładowanie konfiguracji świateł...');
    const elements = getLightFormElements();
    
    try {
        if (elements.saveButton) showLoading(elements.saveButton, true);
        
        const response = await fetchWithTimeout('/api/lights/config');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        log('Pobrana konfiguracja świateł:', data);
        
        if (data.status === 'ok' && data.lights) {
            // Aktualizacja interfejsu
            updateFormFromConfig(data.lights, elements);
        } else {
            log('Odpowiedź serwera nie zawiera danych o światłach, używam domyślnych wartości', 'warn');
            // Ustawienia domyślne
            updateFormFromConfig(DEFAULT_CONFIG, elements);
        }
        
        // Zaktualizuj widoczność sekcji
        updateLightConfigVisibility();
        
    } catch (error) {
        log('Błąd podczas ładowania konfiguracji świateł: ' + error, 'error');
        // Ustawienia domyślne w przypadku błędu
        updateFormFromConfig(DEFAULT_CONFIG, elements);
        showNotification('Błąd podczas ładowania konfiguracji: ' + error.message, 'error');
    } finally {
        if (elements.saveButton) showLoading(elements.saveButton, false);
    }
}

// Funkcja do walidacji konfiguracji świateł
function validateLightConfig(config) {
    const validLightOptions = ['NONE', 'DRL', 'FRONT', 'REAR', 'FRONT+REAR', 'DRL+REAR', 'FRONT+DRL', 'ALL'];
    
    if (!validLightOptions.includes(config.dayLights)) {
        throw new Error(`Nieprawidłowa opcja świateł dziennych: ${config.dayLights}`);
    }
    
    if (!validLightOptions.includes(config.nightLights)) {
        throw new Error(`Nieprawidłowa opcja świateł nocnych: ${config.nightLights}`);
    }
    
    if (isNaN(config.blinkFrequency) || config.blinkFrequency < 100 || config.blinkFrequency > 2000) {
        throw new Error(`Częstotliwość migania poza zakresem: ${config.blinkFrequency}ms (zakres 100-2000ms)`);
    }
    
    if (config.controlMode !== 0 && config.controlMode !== 1) {
        throw new Error(`Nieprawidłowy tryb sterowania: ${config.controlMode}`);
    }
    
    return true;
}

// Funkcja do pobierania konfiguracji z formularza
function getLightConfigFromForm() {
    const elements = getLightFormElements();
    
    if (!validateFormElements(elements)) {
        throw new Error('Nie można pobrać konfiguracji - brakujące elementy formularza');
    }
    
    // Konwersja wartości z formularza
    const controlMode = elements.controlMode.value === 'controller' ? 1 : 0;
    const dayLights = elements.dayLights.value;
    const nightLights = elements.nightLights.value;
    const dayBlink = elements.dayBlink.checked;
    const nightBlink = elements.nightBlink.checked;
    const blinkFrequency = parseInt(elements.blinkFrequency.value) || DEFAULT_CONFIG.blinkFrequency;
    
    const config = {
        controlMode,
        dayLights,
        nightLights,
        dayBlink,
        nightBlink,
        blinkFrequency
    };
    
    validateLightConfig(config);
    return config;
}

// Funkcja do zapisywania konfiguracji świateł - zmodyfikowana
async function saveLightConfig() {
    log('Rozpoczynam zapisywanie konfiguracji świateł...');
    const elements = getLightFormElements();
    
    // WAŻNE: Zapamiętaj wybrany przez użytkownika tryb przed wysłaniem do serwera
    const userSelectedMode = elements.controlMode ? elements.controlMode.value : null;
    log(`Zapisuję tryb wybrany przez użytkownika: ${userSelectedMode}`);
    
    try {
        const lightConfig = getLightConfigFromForm();
        log('Konfiguracja do zapisu:', lightConfig);
        
        // Pokaż wskaźnik ładowania
        showLoading(elements.saveButton, true);
        
        // Wyślij dane jako JSON
        const response = await fetchWithTimeout('/api/lights/config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(lightConfig)
        });
        
        log(`Status odpowiedzi: ${response.status}`);
        const responseText = await response.text();
        log(`Odpowiedź serwera: ${responseText}`);
        
        // Parsuj odpowiedź JSON
        let result;
        try {
            result = JSON.parse(responseText);
        } catch (e) {
            log('Błąd parsowania odpowiedzi JSON:', 'error');
            throw new Error('Nieprawidłowy format odpowiedzi serwera');
        }
        
        if (result.status === 'ok') {
            showNotification('Zapisano ustawienia świateł!', 'success');
            
            if (result.lights) {
                // DEBUG: Wyświetl otrzymaną wartość controlMode
                log(`Otrzymana wartość controlMode z serwera: ${result.lights.controlMode}`);
                
                // ROZWIĄZANIE: Uwzględnij wybór użytkownika zamiast wartości zwróconej przez serwer
                const updatedConfig = {...result.lights};
                
                // Ustawienie trybu sterowania zgodnie z wyborem użytkownika
                if (userSelectedMode) {
                    updatedConfig.controlMode = userSelectedMode === 'controller' ? 1 : 0;
                    log(`Wymuszono tryb użytkownika: ${userSelectedMode} (controlMode: ${updatedConfig.controlMode})`);
                }
                
                // Aktualizacja interfejsu z uwzględnieniem wyboru użytkownika
                updateFormFromConfig(updatedConfig, elements);
                
                // Zaktualizuj widoczność sekcji po zmianie trybu
                updateLightConfigVisibility();
            }
        } else {
            throw new Error(result.message || 'Nieznany błąd podczas zapisu');
        }
    } catch (error) {
        log('Błąd podczas zapisywania: ' + error, 'error');
        showNotification('Błąd: ' + error.message, 'error');
    } finally {
        // Przywróć przycisk
        showLoading(elements.saveButton, false);
    }
}

// Funkcja do inicjalizacji obsługi formularza konfiguracji świateł
function initLightConfigForm() {
    log('Inicjalizacja formularza konfiguracji świateł');
    const elements = getLightFormElements();
    
    // Sprawdź, czy wszystkie elementy istnieją
    validateFormElements(elements);
    
    // Dodaj nasłuchiwanie na zmianę trybu sterowania światłami
    if (elements.controlMode) {
        elements.controlMode.addEventListener('change', function() {
            log('Zmieniono tryb sterowania na: ' + this.value);
            updateLightConfigVisibility();
        });
    }
    
    // Dodaj nasłuchiwanie na przycisk zapisu
    if (elements.saveButton) {
        elements.saveButton.addEventListener('click', saveLightConfig);
        log('Dodano nasłuchiwacz do przycisku save-light-config');
    }
    
    // Ustaw początkowy stan widoczności
    updateLightConfigVisibility();
}

// Dodaj tooltips (jeśli masz system tooltipów w głównym skrypcie)
function initTooltips() {
    if (typeof tooltips !== 'undefined') {
        // Dodanie nowego tooltipa dla trybu sterowania światłami
        tooltips["light-control-info"] = `
            <h3>Tryb sterowania światłami</h3>
            <p><strong>Smart</strong> - Zaawansowane sterowanie świateł przez system e-Bike. Dostępne są opcje konfiguracji świateł dziennych, nocnych oraz opcji migania.</p>
            <p><strong>Sterownik</strong> - Podstawowe sterowanie świateł przez kontroler KT. System e-Bike będzie tylko włączał/wyłączał światła, a kontroler KT będzie decydował o szczegółach ich działania.</p>
        `;
    }
}

// Inicjalizacja przy załadowaniu dokumentu
document.addEventListener('DOMContentLoaded', function() {
    log('DOM załadowany, inicjalizacja komponentów...');
    
    // Inicjalizuj tooltips
    initTooltips();
    
    // Załaduj konfigurację świateł
    loadLightConfig();
    
    // Inicjalizuj obsługę formularza
    initLightConfigForm();
});
