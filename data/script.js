// Funkcja konwersji wartości formularza na wartości API
function getLightMode(value) {
    debug('Konwersja wartości formularza:', value);
    switch(value) {
        case 'front-day':
        case 'front-normal':
            return "FRONT";
        case 'rear':
            return "REAR";
        case 'front-day-rear':
        case 'front-normal-rear':
            return "BOTH";
        case 'off':
        default:
            return "NONE";
    }
}

// Funkcja konwersji wartości z API na wartości formularza
function getFormValue(serverValue, isNightMode = false) {
    debug('Konwersja wartości z serwera:', serverValue);
    switch(serverValue) {
        case 'FRONT':
            return isNightMode ? 'front-normal' : 'front-day';
        case 'REAR':
            return 'rear';
        case 'BOTH':
            return isNightMode ? 'front-normal-rear' : 'front-day-rear';
        case 'NONE':
        default:
            return 'off';
    }
}

// Dodaj zmienną do kontroli debounce
let saveTimeout = null;

// Na początku pliku w sekcji funkcji inicjalizujących
async function loadOdometerValue() {
    try {
        const response = await fetch('/api/odometer');
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        const value = await response.text();
        document.getElementById('total-odometer').value = Math.floor(parseFloat(value));
    } catch (error) {
        console.error('Error loading odometer:', error);
    }
}

// Dodaj przed saveGeneralSettings()
async function saveOdometerValue() {
    const odometerInput = document.getElementById('total-odometer');
    if (!odometerInput) return;

    const value = odometerInput.value;
    if (value === '') return;

    try {
        const response = await fetch('/api/setOdometer', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            body: `value=${value}`
        });

        if (!response.ok) {
            throw new Error('Network response was not ok');
        }

        const result = await response.text();
        if (result === 'OK') {
            console.log('Odometer value saved successfully');
        } else {
            throw new Error('Failed to save odometer value');
        }
    } catch (error) {
        console.error('Error saving odometer:', error);
        alert('Error saving odometer value');
    }
}

// Funkcja zapisywania konfiguracji z debounce
async function saveLightConfig() {
    console.log('Rozpoczynam zapisywanie konfiguracji świateł...');
    
    try {
        const dayLights = document.getElementById('day-lights').value;
        const nightLights = document.getElementById('night-lights').value;
        const dayBlink = document.getElementById('day-blink').checked;
        const nightBlink = document.getElementById('night-blink').checked;
        const blinkFrequency = parseInt(document.getElementById('blink-frequency').value) || 500;
        
        console.log(`Konfiguracja do zapisu: dzienne=${dayLights}, nocne=${nightLights}, `+
                    `dzienBlink=${dayBlink}, nocBlink=${nightBlink}, częst.=${blinkFrequency}`);
        
        const lightConfig = {
            dayLights: dayLights,
            nightLights: nightLights,
            dayBlink: dayBlink,
            nightBlink: nightBlink,
            blinkFrequency: blinkFrequency
        };
        
        // Dodaj wskaźnik ładowania
        const saveBtn = document.getElementById('save-lights-btn');
        const originalText = saveBtn.textContent;
        saveBtn.textContent = 'Zapisywanie...';
        saveBtn.disabled = true;

        // Wyślij dane jako czysty JSON (ważne!)
        const response = await fetch('/api/lights/config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(lightConfig)
        });
        
        console.log(`Status odpowiedzi: ${response.status}`);
        const responseText = await response.text();
        console.log(`Odpowiedź serwera: ${responseText}`);
        
        // Parsuj odpowiedź JSON
        let result;
        try {
            result = JSON.parse(responseText);
        } catch (e) {
            console.error('Błąd parsowania odpowiedzi JSON:', e);
            throw new Error('Nieprawidłowy format odpowiedzi serwera');
        }
        
        // Przywróć przycisk
        saveBtn.textContent = originalText;
        saveBtn.disabled = false;
        
        if (result.status === 'ok') {
            // Pokaż wyraźne potwierdzenie
            showToast('Zapisano ustawienia świateł!', 'success');
            
            // Jeśli przyszły zaktualizowane dane świateł, zaktualizuj formularz
            if (result.lights) {
                document.getElementById('day-lights').value = result.lights.dayLights;
                document.getElementById('night-lights').value = result.lights.nightLights;
                document.getElementById('day-blink').checked = result.lights.dayBlink;
                document.getElementById('night-blink').checked = result.lights.nightBlink;
                document.getElementById('blink-frequency').value = result.lights.blinkFrequency;
                console.log('Zaktualizowano formularz z odpowiedzi serwera');
            }
        } else {
            throw new Error(result.message || 'Nieznany błąd podczas zapisu');
        }
    } catch (error) {
        console.error('Błąd podczas zapisywania:', error);
        showToast('Błąd: ' + error.message, 'error');
    }
}

// Funkcja do wyświetlania powiadomień toast
// Funkcja do wyświetlania powiadomień toast
function showToast(message, type = 'info') {
    // Sprawdź czy kontener na toasty istnieje
    let toastContainer = document.getElementById('toast-container');
    if (!toastContainer) {
        // Utwórz kontener jeśli nie istnieje
        toastContainer = document.createElement('div');
        toastContainer.id = 'toast-container';
        toastContainer.className = 'toast-container position-fixed bottom-0 end-0 p-3';
        document.body.appendChild(toastContainer);
    }
    
    // Utwórz element toast
    const toastId = 'toast-' + Date.now();
    const toast = document.createElement('div');
    toast.id = toastId;
    toast.className = 'toast';
    toast.setAttribute('role', 'alert');
    toast.setAttribute('aria-live', 'assertive');
    toast.setAttribute('aria-atomic', 'true');
    
    // Ustaw różne kolory w zależności od typu
    let bgClass = 'bg-primary text-white';
    if (type === 'success') {
        bgClass = 'bg-success text-white';
    } else if (type === 'error') {
        bgClass = 'bg-danger text-white';
    } else if (type === 'warning') {
        bgClass = 'bg-warning text-dark';
    }
    
    // Struktura toast
    toast.innerHTML = `
        <div class="toast-header ${bgClass}">
            <strong class="me-auto">eBike System</strong>
            <button type="button" class="btn-close btn-close-white" data-bs-dismiss="toast" aria-label="Close"></button>
        </div>
        <div class="toast-body">
            ${message}
        </div>
    `;
    
    // Dodaj toast do kontenera
    toastContainer.appendChild(toast);
    
    // Inicjalizuj toast za pomocą Bootstrap
    const bsToast = new bootstrap.Toast(toast, {
        autohide: true,
        delay: 3000
    });
    
    // Pokaż toast
    bsToast.show();
    
    // Usuń element po ukryciu
    toast.addEventListener('hidden.bs.toast', function () {
        toast.remove();
    });
}

// Funkcja do pokazywania powiadomień
function showNotification(message, type = 'info') {
    // Sprawdź czy kontener na powiadomienia istnieje
    let notifContainer = document.getElementById('notification-container');
    if (!notifContainer) {
        // Utwórz kontener jeśli nie istnieje
        notifContainer = document.createElement('div');
        notifContainer.id = 'notification-container';
        notifContainer.style.position = 'fixed';
        notifContainer.style.top = '20px';
        notifContainer.style.right = '20px';
        notifContainer.style.zIndex = '9999';
        document.body.appendChild(notifContainer);
    }
    
    // Utwórz element powiadomienia
    const notification = document.createElement('div');
    notification.className = `notification ${type}`;
    notification.style.padding = '10px 20px';
    notification.style.marginBottom = '10px';
    notification.style.borderRadius = '5px';
    notification.style.boxShadow = '0 2px 4px rgba(0,0,0,0.2)';
    notification.style.transition = 'all 0.3s ease';
    notification.style.minWidth = '200px';
    
    // Ustaw kolor tła w zależności od typu
    if (type === 'success') {
        notification.style.backgroundColor = '#4CAF50';
        notification.style.color = 'white';
    } else if (type === 'error') {
        notification.style.backgroundColor = '#F44336';
        notification.style.color = 'white';
    } else {
        notification.style.backgroundColor = '#2196F3';
        notification.style.color = 'white';
    }
    
    notification.textContent = message;
    
    // Dodaj powiadomienie do kontenera
    notifContainer.appendChild(notification);
    
    // Usuń powiadomienie po 3 sekundach
    setTimeout(() => {
        notification.style.opacity = '0';
        setTimeout(() => {
            notifContainer.removeChild(notification);
        }, 300);
    }, 3000);
}

// Dodaj debounce dla loadLightConfig
let loadTimeout = null;

// Funkcja do ładowania konfiguracji świateł przy starcie
async function loadLightConfig() {
    console.log('Ładowanie konfiguracji świateł...');
    
    try {
        const response = await fetch('/api/lights/config');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        console.log('Pobrana konfiguracja świateł:', data);
        
        if (data.status === 'ok' && data.lights) {
            // Aktualizacja interfejsu
            document.getElementById('day-lights').value = data.lights.dayLights || 'DRL+REAR';
            document.getElementById('night-lights').value = data.lights.nightLights || 'FRONT+REAR';
            document.getElementById('day-blink').checked = data.lights.dayBlink !== undefined ? data.lights.dayBlink : true;
            document.getElementById('night-blink').checked = data.lights.nightBlink !== undefined ? data.lights.nightBlink : false;
            document.getElementById('blink-frequency').value = data.lights.blinkFrequency || 500;
            
            console.log('Zaktualizowano formularz z konfiguracji z serwera');
        } else {
            console.warn('Odpowiedź serwera nie zawiera danych o światłach, używam domyślnych wartości');
            // Ustawienia domyślne
            document.getElementById('day-lights').value = 'DRL+REAR';
            document.getElementById('night-lights').value = 'FRONT+REAR';
            document.getElementById('day-blink').checked = true;
            document.getElementById('night-blink').checked = false;
            document.getElementById('blink-frequency').value = 500;
        }
    } catch (error) {
        console.error('Błąd podczas ładowania konfiguracji świateł:', error);
        // Ustawienia domyślne w przypadku błędu
        document.getElementById('day-lights').value = 'DRL+REAR';
        document.getElementById('night-lights').value = 'FRONT+REAR';
        document.getElementById('day-blink').checked = true;
        document.getElementById('night-blink').checked = false;
        document.getElementById('blink-frequency').value = 500;
    }
}

// Wywołaj funkcję przy ładowaniu strony
document.addEventListener('DOMContentLoaded', function() {
    // Istniejący kod inicjalizacji...
    
    // Dodaj ładowanie konfiguracji świateł
    loadLightConfig();
    
    // Dodaj nasłuchiwanie zmian zakładek
    document.querySelectorAll('.nav-link').forEach(link => {
        link.addEventListener('click', function(event) {
            const targetTab = this.getAttribute('data-bs-target');
            if (targetTab === '#lights-tab') {
                // Odśwież konfigurację świateł gdy przechodzimy do zakładki świateł
                loadLightConfig();
            }
        });
    });
});

function verifyLightElements() {
    const requiredElements = [
        'day-lights', 'night-lights', 'day-blink', 'night-blink', 'blink-frequency', 'save-light-config'
    ];
    
    const missingElements = [];
    
    for (const id of requiredElements) {
        if (!document.getElementById(id)) {
            missingElements.push(id);
        }
    }
    
    if (missingElements.length > 0) {
        console.error('Brakuje następujących elementów formularza świateł:', missingElements.join(', '));
        return false;
    }
    
    return true;
}

// Funkcja pomocnicza do debugowania
function debug(...args) {
    if (typeof console !== 'undefined') {
        console.log('[DEBUG]', ...args);
    }
}

document.addEventListener('DOMContentLoaded', async function() {
    debug('Inicjalizacja aplikacji...');

    let clockInterval;
    
    try {
        // Sprawdź czy wszystkie elementy formularza świateł istnieją
        if (!verifyLightElements()) {
            console.error('Nie wszystkie wymagane elementy formularza świateł są dostępne');
        }

        // Inicjalizacja zegara
        clockInterval = initializeClock();

        // Dodajemy inicjalizację sekcji zwijanych
        initializeCollapsibleSections();
        
        // Poczekaj na załadowanie DOM
        await new Promise(resolve => setTimeout(resolve, 200));

        // Dodaj wczytywanie licznika
        await loadOdometerValue();

        // Inicjalizacja pozostałych modułów
        if (document.querySelector('.light-config')) {
            await loadLightConfig();
        }

        await Promise.all([
            fetchDisplayConfig(),
            fetchControllerConfig(),
            fetchSystemVersion()
        ]);

        // Inicjalizacja WebSocket i UI
        setupWebSocket();
        setupModal();
        setupFormListeners(); // Prawidłowa inicjalizacja nasłuchiwania zdarzeń formularza

        debug('Inicjalizacja zakończona pomyślnie');
    } catch (error) {
        console.error('Błąd podczas inicjalizacji:', error);
        // W przypadku błędu, zatrzymaj interval zegara
        if (clockInterval) clearInterval(clockInterval);
    }
});

function checkAPIResponse(response, errorMessage = 'Błąd API') {
    if (!response.ok) {
        throw new Error(`${errorMessage}: ${response.status}`);
    }
    return response.json();
}

function showModal(title, description) {
    const modal = document.getElementById('info-modal');
    const modalTitle = document.getElementById('modal-title');
    const modalDescription = document.getElementById('modal-description');
    
    if (modal && modalTitle && modalDescription) {
        modalTitle.textContent = title;
        modalDescription.textContent = description;
        modal.style.display = 'block';
    }
}

let ws = null;

function setupWebSocket() {
    debug('Inicjalizacja WebSocket...');
    function connect() {
        ws = new WebSocket('ws://' + window.location.hostname + '/ws');
        
        ws.onopen = () => {
            debug('WebSocket połączony');
            // Pobierz aktualny stan po połączeniu
            fetchCurrentState();
        };

        ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                debug('Otrzymano dane WebSocket:', data);
                if (data.lights) {
                    updateLightStatus(data.lights);
                    updateLightForm(data.lights); // Aktualizuj też formularz
                }
            } catch (error) {
                console.error('Błąd podczas przetwarzania danych WebSocket:', error);
            }
        };

        ws.onclose = () => {
            debug('WebSocket rozłączony, próba ponownego połączenia za 5s');
            setTimeout(connect, 5000);
        };

        ws.onerror = (error) => {
            console.error('Błąd WebSocket:', error);
        };
    }

    connect();
}

async function fetchRTCTime() {
    try {
        const timeElement = document.getElementById('rtc-time');
        const dateElement = document.getElementById('rtc-date');

        if (!timeElement || !dateElement) {
            debug('Elementy czasu nie są gotowe');
            return;
        }

        const response = await fetch('/api/time');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        if (data && data.time) {
            const { hours, minutes, seconds, year, month, day } = data.time;
            
            const timeStr = `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
            const dateStr = `${String(day).padStart(2, '0')}/${String(month).padStart(2, '0')}/${year}`;
            
            timeElement.value = timeStr;
            dateElement.value = dateStr;
            
            debug('Zaktualizowano czas:', { time: timeStr, date: dateStr });
        }
    } catch (error) {
        console.error('Błąd podczas pobierania czasu RTC:', error);
    }
}

// Funkcja inicjalizacji zegara
function initializeClock() {
    debug('Inicjalizacja zegara');
    fetchRTCTime(); // Pierwsze pobranie
    return setInterval(fetchRTCTime, 1000); // Aktualizacja co sekundę
}

function checkElements(...ids) {
    const missing = [];
    const elements = {};

    for (const id of ids) {
        const element = document.getElementById(id);
        if (!element) {
            missing.push(id);
        }
        elements[id] = element;
    }

    if (missing.length > 0) {
        throw new Error(`Brak elementów: ${missing.join(', ')}`);
    }

    return elements;
}

// Funkcja zapisująca konfigurację RTC
async function saveRTCConfig() {
    // Pobierz aktualny czas z przeglądarki
    const now = new Date();
    
    try {
        const response = await fetch('/api/time', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                year: now.getFullYear(),
                month: now.getMonth() + 1, // getMonth() zwraca 0-11
                day: now.getDate(),
                hour: now.getHours(),
                minute: now.getMinutes(),
                second: now.getSeconds()
            })
        });

        const data = await response.json();
        if (data.status === 'ok') {
            alert('Ustawiono aktualny czas');
            fetchRTCTime(); // Pobierz zaktualizowany czas
        } else {
            alert('Błąd podczas ustawiania czasu');
        }
    } catch (error) {
        console.error('Błąd podczas ustawiania czasu RTC:', error);
        alert('Błąd podczas ustawiania czasu');
    }
}

// Funkcja pobierająca konfigurację świateł
async function fetchLightConfig() {
    try {
        const response = await fetch('/api/status');
        const data = await response.json();
        if (data.lights) {
            document.getElementById('day-lights').value = data.lights.dayLights;
            document.getElementById('night-lights').value = data.lights.nightLights;
            document.getElementById('day-blink').checked = data.lights.dayBlink;
            document.getElementById('night-blink').checked = data.lights.nightBlink;
            document.getElementById('blink-frequency').value = data.lights.blinkFrequency;
        }
    } catch (error) {
        console.error('Błąd podczas pobierania konfiguracji świateł:', error);
    }
}

function setupFormListeners() {
    const formElements = [
        'day-lights',
        'night-lights',
        'day-blink',
        'night-blink',
        'blink-frequency'
    ];

    formElements.forEach(elementId => {
        const element = document.getElementById(elementId);
        if (element) {
            element.addEventListener('change', () => {
                debug(`Zmieniono wartość ${elementId}:`, 
                    element.type === 'checkbox' ? element.checked : element.value);
            });
        }
    });

    // Dodaj listener do przycisku zapisu
    const saveButton = document.getElementById('save-light-config'); // Poprawne ID przycisku
    if (saveButton) {
        saveButton.addEventListener('click', saveLightConfig);
        debug('Przycisk zapisywania ustawień świateł zainicjalizowany (setupFormListeners)');
    } else {
        console.error('Nie znaleziono przycisku save-light-config w setupFormListeners');
    }
}

// Funkcja inicjalizacji formularza świateł
function initializeLightForm() {
    debug('Inicjalizacja formularza świateł');
    const form = document.getElementById('lights-form');
    if (!form) {
        console.error('Nie znaleziono formularza świateł');
        return false;
    }
    return true;
}

// Funkcja sprawdzająca elementy formularza
function getLightFormElements() {
    const elements = {
        dayLights: document.getElementById('day-lights'),
        nightLights: document.getElementById('night-lights'),
        dayBlink: document.getElementById('day-blink'),
        nightBlink: document.getElementById('night-blink'),
        blinkFrequency: document.getElementById('blink-frequency')
    };

    const missing = Object.entries(elements)
        .filter(([_, element]) => !element)
        .map(([name]) => name);

    if (missing.length > 0) {
        throw new Error(`Brak elementów formularza: ${missing.join(', ')}`);
    }

    return elements;
}

async function fetchCurrentState(retries = 3) {
    for (let i = 0; i < retries; i++) {
        try {
            debug(`Próba pobrania stanu (${i + 1}/${retries})`);
            const response = await fetch('/api/status');
            
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            const data = await response.json();
            debug('Otrzymane dane statusu:', data);

            if (data.lights) {
                updateLightStatus(data.lights);
                updateLightForm(data.lights);
                return true;
            }
            
            await new Promise(resolve => setTimeout(resolve, 1000));
        } catch (error) {
            console.error(`Błąd podczas próby ${i + 1}:`, error);
            if (i === retries - 1) throw error;
            await new Promise(resolve => setTimeout(resolve, 1000));
        }
    }
    return false;
}

async function fetchCurrentState() {
    try {
        debug('Pobieranie aktualnego stanu...');
        const response = await fetch('/api/status');
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        debug('Otrzymane dane statusu:', data);

        if (data.lights) {
            // Aktualizacja interfejsu
            updateLightStatus(data.lights);
            // Aktualizacja formularza
            updateLightForm(data.lights);
        }
    } catch (error) {
        console.error('Błąd podczas pobierania stanu:', error);
    }
}

// Funkcja do aktualizacji formularza na podstawie otrzymanego stanu
function updateLightForm(lights) {
    debug('Aktualizacja formularza, otrzymane dane:', lights);
    
    try {
        document.getElementById('day-lights').value = lights.dayLights || 'NONE';
        document.getElementById('night-lights').value = lights.nightLights || 'NONE';
        document.getElementById('day-blink').checked = Boolean(lights.dayBlink);
        document.getElementById('night-blink').checked = Boolean(lights.nightBlink);
        document.getElementById('blink-frequency').value = lights.blinkFrequency || 500;
        
        debug('Formularz zaktualizowany pomyślnie');
    } catch (error) {
        console.error('Błąd podczas aktualizacji formularza:', error);
    }
}

function setupModal() {
    const modal = document.getElementById('info-modal');
    const modalTitle = document.getElementById('modal-title');
    const modalDescription = document.getElementById('modal-description');
    
    // Otwieranie modala przez info-icons
    document.querySelectorAll('.info-icon').forEach(button => {
        button.addEventListener('click', function() {
            const infoId = this.dataset.info;
            const info = infoContent[infoId];
            
            if (info) {
                modalTitle.textContent = info.title;
                modalDescription.textContent = info.description;               
                modal.style.display = 'block';
            } else {
                console.error('Nie znaleziono opisu dla:', infoId);
            }
        });
    });
    
    // Zamykanie modala
    document.querySelector('.close-modal').addEventListener('click', () => {
        modal.style.display = 'none';
    });
    
    // Zamykanie po kliknięciu poza modalem
    window.addEventListener('click', (event) => {
        if (event.target === modal) {
            modal.style.display = 'none';
        }
    });
}

// Funkcja aktualizacji statusu świateł
function updateLightStatus(lights) {
    try {
        const elements = getLightFormElements();
        if (!elements) return;

        // Aktualizacja klas CSS dla wskaźników świateł (jeśli są)
        const indicators = {
            frontDay: document.querySelector('.light-indicator.front-day'),
            front: document.querySelector('.light-indicator.front'),
            rear: document.querySelector('.light-indicator.rear')
        };

        for (const [key, indicator] of Object.entries(indicators)) {
            if (indicator) {
                indicator.classList.toggle('active', Boolean(lights[key]));
            }
        }

        debug('Status świateł zaktualizowany');
    } catch (error) {
        console.error('Błąd podczas aktualizacji statusu świateł:', error);
    }
}

// Funkcja pobierająca konfigurację wyświetlacza
async function fetchDisplayConfig() {
    try {
        const response = await fetch('/api/status');
        const data = await response.json();
        if (data.backlight) {
            document.getElementById('day-brightness').value = data.backlight.dayBrightness;
            document.getElementById('night-brightness').value = data.backlight.nightBrightness;
            document.getElementById('display-auto').value = data.backlight.autoMode.toString();
            // Ustawienie jasności normalnej na podstawie jasności dziennej w trybie manualnym
            document.getElementById('brightness').value = data.backlight.dayBrightness;
            // Wywołaj funkcję przełączania, aby odpowiednio pokazać/ukryć sekcje
            toggleAutoBrightness();
        }
    } catch (error) {
        console.error('Błąd podczas pobierania konfiguracji wyświetlacza:', error);
    }
}

// Funkcja zapisująca konfigurację wyświetlacza
async function saveDisplayConfig() {
    try {
        const autoMode = document.getElementById('display-auto').value === 'true';
        const data = {
            dayBrightness: parseInt(autoMode ? 
                document.getElementById('day-brightness').value : 
                document.getElementById('brightness').value),
            nightBrightness: parseInt(document.getElementById('night-brightness').value),
            autoMode: autoMode
        };

        console.log('Wysyłane dane:', data); // dla debugowania

        const response = await fetch('/api/display/config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(data)
        });

        const result = await response.json();
        console.log('Odpowiedź serwera:', result); // dla debugowania

        if (result.status === 'ok') {
            alert('Zapisano ustawienia wyświetlacza');
            await fetchDisplayConfig(); // odśwież wyświetlane ustawienia
        } else {
            throw new Error(result.message || 'Błąd odpowiedzi serwera');
        }
    } catch (error) {
        console.error('Błąd podczas zapisywania:', error);
        alert('Błąd podczas zapisywania ustawień: ' + error.message);
    }
}

function toggleAutoBrightness() {
    const autoMode = document.getElementById('display-auto').value === 'true';
    const autoBrightnessSection = document.getElementById('auto-brightness-section');
    const normalBrightness = document.getElementById('brightness').parentElement.parentElement;
    
    if (autoMode) {
        autoBrightnessSection.style.display = 'block';
        normalBrightness.style.display = 'none';
        // Ustaw jasność dzienną jako domyślną jasność
        document.getElementById('day-brightness').value = document.getElementById('brightness').value;
    } else {
        autoBrightnessSection.style.display = 'none';
        normalBrightness.style.display = 'flex';
        // Ustaw normalną jasność na wartość jasności dziennej
        document.getElementById('brightness').value = document.getElementById('day-brightness').value;
    }
}

// Walidacja dla pól numerycznych wyświetlacza
document.querySelectorAll('#day-brightness, #night-brightness').forEach(input => {
    input.addEventListener('input', function() {
        let value = parseInt(this.value);
        if (value < 0) this.value = 0;
        if (value > 100) this.value = 100;
    });
});

// Funkcja przełączająca widoczność parametrów w zależności od wybranego sterownika
function toggleControllerParams() {
    const controllerType = document.getElementById('controller-type').value;
    const ktLcdParams = document.getElementById('kt-lcd-params');
    const s866Params = document.getElementById('s866-lcd-params');

    if (controllerType === 'kt-lcd') {
        ktLcdParams.style.display = 'block';
        s866Params.style.display = 'none';
    } else if (controllerType === 's866') {
        ktLcdParams.style.display = 'none';
        s866Params.style.display = 'block';
    }
}

// Funkcja pobierająca konfigurację sterownika
async function fetchControllerConfig() {
    try {
        const response = await fetch('/api/status');
        const data = await response.json();
        if (data.controller) {
            // Ustaw typ sterownika
            document.getElementById('controller-type').value = data.controller.type;
            toggleControllerParams();

            // Wypełnij parametry dla KT-LCD
            if (data.controller.type === 'kt-lcd') {
                // Parametry P
                for (let i = 1; i <= 5; i++) {
                    document.getElementById(`kt-p${i}`).value = data.controller[`p${i}`] || '';
                }
                // Parametry C
                for (let i = 1; i <= 15; i++) {
                    document.getElementById(`kt-c${i}`).value = data.controller[`c${i}`] || '';
                }
                // Parametry L
                for (let i = 1; i <= 3; i++) {
                    document.getElementById(`kt-l${i}`).value = data.controller[`l${i}`] || '';
                }
            }
            // Wypełnij parametry dla S866
            else if (data.controller.type === 's866') {
                document.getElementById('controller-type').value = 's866';
                for (let i = 1; i <= 20; i++) {
                    const input = document.getElementById(`s866-p${i}`);
                    if (input) {
                        input.value = data.controller[`p${i}`] || '';
                    }
                }
            }
        }
    } catch (error) {
        console.error('Błąd podczas pobierania konfiguracji sterownika:', error);
    }
}

// Funkcja zapisująca konfigurację sterownika
async function saveControllerConfig() {
    try {
        const controllerType = document.getElementById('controller-type').value;
        let data = {
            type: controllerType,
        };

        // Zbierz parametry w zależności od typu sterownika
        if (controllerType === 'kt-lcd') {
            // Parametry P
            for (let i = 1; i <= 5; i++) {
                const value = document.getElementById(`kt-p${i}`).value;
                if (value !== '') data[`p${i}`] = parseInt(value);
            }
            // Parametry C
            for (let i = 1; i <= 15; i++) {
                const value = document.getElementById(`kt-c${i}`).value;
                if (value !== '') data[`c${i}`] = parseInt(value);
            }
            // Parametry L
            for (let i = 1; i <= 3; i++) {
                const value = document.getElementById(`kt-l${i}`).value;
                if (value !== '') data[`l${i}`] = parseInt(value);
            }
        } else if (controllerType === 's866') {
            // Zbierz wszystkie parametry S866 (P1-P20)
            for (let i = 1; i <= 20; i++) {
                const value = document.getElementById(`s866-p${i}`).value;
                if (value !== '') {
                    data.p[i] = parseInt(value);
                }
            }
        }

        const response = await fetch('/api/controller/config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(data)
        });

        const result = await response.json();
        if (result.status === 'ok') {
            alert('Zapisano ustawienia sterownika');
            await fetchControllerConfig(); // Odśwież widok
            fetchControllerConfig();
        } else {
            throw new Error('Błąd odpowiedzi serwera');
        }
    } catch (error) {
        console.error('Błąd podczas zapisywania konfiguracji sterownika:', error);
        alert('Błąd podczas zapisywania ustawień: ' + error.message);
    }
}

// Obiekt z informacjami dla każdego parametru
const infoContent = {

    // Sekcja zegara //

    'rtc-info': {
        title: '⏰ Konfiguracja zegara',
        description: `Panel konfiguracji zegara czasu rzeczywistego (RTC)

    ⌚ Funkcje:
      - Synchronizacja czasu systemowego
      - Podtrzymanie bateryjne
      - Format 24-godzinny
      - Kalendarz z datą

    🔄 Synchronizacja:
      1. Sprawdź czas na swoim urządzeniu
      2. Kliknij "Ustaw aktualny czas"
      3. System automatycznie:
          • Pobierze czas z twojego urządzenia
          • Zaktualizuje zegar systemowy
          • Potwierdzi synchronizację

    💡 WSKAZÓWKI:
      - Synchronizuj czas po wymianie baterii
      - Sprawdzaj dokładność co kilka miesięcy
      - Używaj dokładnego źródła czasu

    ⚠️ WAŻNE: 
      - Zegar działa nawet po odłączeniu głównego zasilania
      - Bateria podtrzymująca wystarcza na około 2-3 lata
      - Wymień baterię gdy zauważysz rozbieżności w czasie`
    },

    // Sekcja świateł //

    'light-config-info': {
        title: '💡 Konfiguracja świateł',
        description: `Panel konfiguracji systemu oświetlenia.

    🌞 Tryb dzienny:
      - Światła do jazdy dziennej
      - Zwiększona widoczność

    🌙 Tryb nocny:
      - Pełne oświetlenie drogi
      - Dostosowanie do warunków

    ⚙️ Opcje konfiguracji:
      - Przód: światła dzienne/zwykłe
      - Tył: światło pozycyjne
      - Tryb pulsacyjny (mruganie)
      - Częstotliwość mrugania

    💡 WSKAZÓWKI:
      - Używaj świateł nawet w dzień
      - Dostosuj jasność do warunków
      - Regularnie sprawdzaj działanie

    ⚠️ WAŻNE:
      - Sprawdź lokalne przepisy
      - Utrzymuj światła w czystości
      - Wymień uszkodzone elementy`
    },

    'day-lights-info': {
        title: '☀️ Światła dzienne',
        description: `Wybór konfiguracji świateł dla jazdy w dzień:

      - Wyłączone: wszystkie światła wyłączone 
      - Przód dzień: przednie światło w trybie dziennym 
      - Przód zwykłe: przednie światło w trybie normalnym 
      - Tył: tylko tylne światło 
      - Przód dzień + tył: przednie światło dzienne i tylne 
      - Przód zwykłe + tył: przednie światło normalne i tylne`
    },

    'day-blink-info': {
        title: 'Mruganie tylnego światła (dzień)',
        description: `Włącza lub wyłącza funkcję mrugania tylnego światła podczas jazdy w dzień. 

    Mrugające światło może być bardziej widoczne 
    dla innych uczestników ruchu.`
    },

    'night-lights-info': {
        title: '🌙 Światła nocne',
        description: `Wybór konfiguracji świateł dla jazdy w nocy:
 
      - Wyłączone: wszystkie światła wyłączone 
      - Przód dzień: przednie światło w trybie dziennym 
      - Przód zwykłe: przednie światło w trybie normalnym 
      - Tył: tylko tylne światło 
      - Przód dzień + tył: przednie światło dzienne i tylne 
      - Przód zwykłe + tył: przednie światło normalne i tylne`
    },

    'night-blink-info': {
        title: 'Mruganie tylnego światła (noc)',
        description: `Włącza lub wyłącza funkcję mrugania tylnego światła podczas jazdy w nocy. 
    
    Należy rozważnie używać tej funkcji, gdyż w niektórych warunkach migające światło może być bardziej dezorientujące niż pomocne.`
    },

    'blink-frequency-info': {
        title: '⚡ Częstotliwość mrugania',
        description: `Określa częstotliwość mrugania tylnego światła w milisekundach. 
        
    Mniejsza wartość oznacza szybsze mruganie, a większa - wolniejsze. Zakres: 100-2000ms.`
    },

    // Sekcja wyświetlacza //

    'display-config-info': {
        title: '📱 Konfiguracja wyświetlacza',
        description: `Panel konfiguracji wyświetlacza LCD.

    Dostępne opcje:
    🔆 Jasność:
      - Tryb automatyczny: automatyczne dostosowanie jasności
      - Jasność dzienna: poziom w trybie dziennym (0-100%)
      - Jasność nocna: poziom w trybie nocnym (0-100%)
    
    💡 WSKAZÓWKI:
      - W nocy zalecana jasność 30-50%
      - W dzień zalecana jasność 70-100%
    
    ⚠️ UWAGA: 
    Zbyt niska jasność może utrudnić odczyt w silnym świetle słonecznym`
    },

    'brightness-info': {
        title: '🔆 Podświetlenie wyświetlacza',
        description: `Ustaw jasność podświetlenia wyświetlacza w zakresie od 0% do 100%. 
        
    Wyższa wartość oznacza jaśniejszy wyświetlacz. Zalecane ustawienie to 50-70% dla optymalnej widoczności.`
    },

    'auto-mode-info': {
        title: '🤖 Tryb automatyczny',
        description: `Automatycznie przełącza jasność wyświetlacza w zależności od ustawionych świateł dzień/noc. W trybie dziennym używa jaśniejszego podświetlenia, a w nocnym - przyciemnionego. Gdy światła nie są włączone to jasność jest ustawiona jak dla dnia`
    },

    'day-brightness-info': {
        title: '☀️ Jasność dzienna',
        description: `Poziom jasności wyświetlacza używany w ciągu dnia (0-100%). Zalecana wyższa wartość dla lepszej widoczności w świetle słonecznym.`
    },

    'night-brightness-info': {
        title: '🌙 Jasność nocna',
        description: `Poziom jasności wyświetlacza używany w nocy (0-100%). Zalecana niższa wartość dla komfortowego użytkowania w ciemności.`
    },

    'auto-off-time-info': {
        title: '⏰ Czas automatycznego wyłączenia',
        description: `Określa czas bezczynności, po którym system automatycznie się wyłączy.

        Zakres: 0-60 minut
        0: Funkcja wyłączona (system nie wyłączy się automatycznie)
        1-60: Czas w minutach do automatycznego wyłączenia

        💡 WSKAZÓWKA:
          - Krótszy czas oszczędza baterię
          - Dłuższy czas jest wygodniejszy przy dłuższych postojach
        
        ⚠️ UWAGA:
        System zawsze zapisze wszystkie ustawienia przed wyłączeniem`
    },

    // Sekcja sterownika //

    'controller-config-info': {
        title: '🎮 Konfiguracja sterownika',
        description: `Panel konfiguracji sterownika silnika.

    Obsługiwane sterowniki:
    🔷 KT-LCD:
      - Parametry P1-P5: podstawowa konfiguracja
      - Parametry C1-C15: zaawansowane ustawienia
      - Parametry L1-L3: specjalne funkcje
    
    🔶 S866:
      - Parametry P1-P20: pełna konfiguracja
    
    ⚠️ WAŻNE:
      - Nieprawidłowa konfiguracja może wpłynąć na:
        • Działanie silnika
        • Zużycie energii
        • Żywotność komponentów
      - W razie wątpliwości użyj ustawień domyślnych
    
    💡 WSKAZÓWKA:
    Każdy parametr ma szczegółowy opis dostępny
    pod ikoną informacji (ℹ️)`
    },

    'display-type-info': {
        title: '🔍 Wybór typu wyświetlacza',
        description: `Wybierz odpowiedni model wyświetlacza LCD zainstalowanego w Twoim rowerze.

        🟦 KT-LCD:
        • Standardowy wyświetlacz z serii KT
        • Obsługuje parametry P1-P5, C1-C15, L1-L3
        • Kompatybilny z większością kontrolerów KT
        
        🟨 S866:
        • Wyświetlacz z serii Bigstone/S866
        • Obsługuje parametry P1-P20
        • Posiada dodatkowe funkcje konfiguracyjne
        
        ⚠️ UWAGA: 
        Wybór niewłaściwego typu wyświetlacza może 
        spowodować nieprawidłowe działanie systemu.
        Upewnij się, że wybrany model odpowiada 
        fizycznie zainstalowanemu wyświetlaczowi.`
    },

    // Parametry sterownika KT-LCD //

    // Parametry P sterownika //

    'kt-p1-info': {
        title: '⚙️ P1 - Przełożenie silnika',
        description: `Obliczane ze wzoru: ilość magnesów X przełożenie

    Dla silników bez przekładni (np. 30H): przełożenie = 1 (P1 = 46)
    Dla silników z przekładnią (np. XP07): przełożenie > 1 (P1 = 96)

    Parametr wpływa tylko na wyświetlanie prędkości - nieprawidłowa wartość nie wpłynie na jazdę, jedynie na wskazania prędkościomierza`
    },

    'kt-p2-info': {
        title: 'P2 - Sposób odczytu prędkości',
        description: `Wybierz:
        
    0: Dla silnika bez przekładni
      - Prędkość z czujników halla silnika
      - Biały przewód do pomiaru temperatury

    1: Dla silnika z przekładnią
      - Prędkość z dodatkowego czujnika halla
      - Biały przewód do pomiaru prędkości

    2-6: Dla silników z wieloma magnesami pomiarowymi
      - Prędkość z dodatkowego czujnika halla
      - Biały przewód do pomiaru prędkości
      *używane rzadko, ale gdy pokazuje zaniżoną prędkość spróbuj tej opcji`
    },

    'kt-p3-info': {
        title: 'P3 - Tryb działania czujnika PAS',
        description: `Pozwala ustawić jak ma się zachowywać wspomaganie z czujnikiem PAS podczas używania biegów 1-5
      – 0: Tryb sterowania poprzez prędkość
      – 1: Tryb sterowania momentem obrotowym`
    },

    'kt-p4-info': {
        title: 'P4 - Ruszanie z manetki',
        description: `Pozwala ustawić sposób ruszania rowerem:

    0: Można ruszyć od zera używając samej manetki
    1: Manetka działa dopiero po ruszeniu z PAS/nóg`
    },

    'kt-p5-info': {
        title: 'P5 - Sposób obliczania poziomu naładowania akumulatora',
        description: `Pozwala dostosować czułość wskaźnika naładowania akumulatora
      - niższa wartość: szybsza reakcja na spadki napięcia
      - wyższa wartość: wolniejsza reakcja, uśrednianie wskazań

    Zalecane zakresy wartości:
      - 24V: 4-11
      - 36V: 5-15
      - 48V: 6-20
      - 60V: 7-30

    Uwaga: Zbyt wysokie wartości mogą opóźnić ostrzeżenie o niskim poziomie baterii.

    Jeśli wskaźnik pokazuje stale 100%, wykonaj:
    1. Reset do ustawień fabrycznych
    2. Ustaw podstawowe parametry
    3. Wykonaj pełny cykl ładowania-rozładowania`
    },

    // Parametry C sterownika //

    'kt-c1-info': {
        title: 'C1 - Czujnik PAS',
        description: `Konfiguracja czułości czujnika asysty pedałowania (PAS). Wpływa na to, jak szybko system reaguje na pedałowanie.`
    },

    'kt-c2-info': {
        title: 'C2 - Typ silnika',
        description: `Ustawienia charakterystyki silnika i jego podstawowych parametrów pracy.`
    },

    'kt-c3-info': {
        title: 'C3 - Tryb wspomagania',
        description: `Konfiguracja poziomów wspomagania i ich charakterystyki (eco, normal, power).`
    },

    'kt-c4-info': {
        title: 'C4 - Manetka i PAS',
        description: `Określa sposób współdziałania manetki z czujnikiem PAS i priorytety sterowania.`
    },

    'kt-c5-info': {
        title: '⚠️ C5 - Regulacja prądu sterownika',
        description: `Pozwala dostosować maksymalny prąd sterownika do możliwości akumulatora.
    
    Wartości:
    3:  Prąd zmniejszony o 50% (÷2.0)
    4:  Prąd zmniejszony o 33% (÷1.5) 
    5:  Prąd zmniejszony o 25% (÷1.33)
    6:  Prąd zmniejszony o 20% (÷1.25)
    7:  Prąd zmniejszony o 17% (÷1.20)
    8:  Prąd zmniejszony o 13% (÷1.15)
    9:  Prąd zmniejszony o 9%  (÷1.10)
    10: Pełny prąd sterownika

    Przykład dla sterownika 25A:
      - C5=3 → max 12.5A
      - C5=5 → max 18.8A
      - C5=10 → max 25A

    ⚠️ WAŻNE
    Używaj niższych wartości gdy:
      - Masz słaby akumulator z mocnym silnikiem
      - Chcesz wydłużyć żywotność akumulatora
      - Występują spadki napięcia podczas przyśpieszania`
    },

    'kt-c6-info': {
        title: 'C6 - Jasność wyświetlacza',
        description: `Ustawienie domyślnej jasności podświetlenia wyświetlacza LCD.`
    },

    'kt-c7-info': {
        title: 'C7 - Tempomat',
        description: `Konfiguracja tempomatu - utrzymywania stałej prędkości.`
    },

    'kt-c8-info': {
        title: 'C8 - Silnik',
        description: `Dodatkowe parametry silnika, w tym temperatura i zabezpieczenia.`
    },

    'kt-c9-info': {
        title: 'C9 - Zabezpieczenia',
        description: `Ustawienia kodów PIN i innych zabezpieczeń systemowych.`
    },

    'kt-c10-info': {
        title: 'C10 - Ustawienia fabryczne',
        description: `Opcje przywracania ustawień fabrycznych i kalibracji systemu.`
    },

    'kt-c11-info': {
        title: 'C11 - Komunikacja',
        description: `Parametry komunikacji między kontrolerem a wyświetlaczem.`
    },

    'kt-c12-info': {
        title: '🔋 C12 - Regulacja minimalnego napięcia wyłączenia (LVC)',
        description: `Pozwala dostosować próg napięcia, przy którym sterownik się wyłącza (Low Voltage Cutoff).

    Wartości względem napięcia domyślnego:
    0: -2.0V     
    1: -1.5V     
    2: -1.0V     
    3: -0.5V
    4: domyślne (40V dla 48V, 30V dla 36V, 20V dla 24V)
    5: +0.5V
    6: +1.0V
    7: +1.5V

    Przykład dla sterownika 48V:
      - Domyślnie (C12=4): wyłączenie przy 40V
      - C12=0: wyłączenie przy 38V
      - C12=7: wyłączenie przy 41.5V

    ⚠️ WAŻNE WSKAZÓWKI:
    1. Obniżenie progu poniżej 42V w sterowniku 48V może spowodować:
      - Błędne wykrycie systemu jako 36V
      - Nieprawidłowe wskazania poziomu naładowania (stałe 100%)
    2. Przy częstym rozładowywaniu akumulatora:
      - Zalecane ustawienie C12=7
      - Zapobiega przełączaniu na tryb 36V
      - Chroni ostatnie % pojemności akumulatora

    ZASTOSOWANIE:
      - Dostosowanie do charakterystyki BMS
      - Optymalizacja wykorzystania pojemności akumulatora
      - Ochrona przed głębokim rozładowaniem`
    },

    'kt-c13-info': {
        title: '🔄 C13 - Hamowanie regeneracyjne',
        description: `Pozwala ustawić siłę hamowania regeneracyjnego i efektywność odzysku energii.

    USTAWIENIA:
    0: Wyłączone (brak hamowania i odzysku)
    1: Słabe hamowanie + Najwyższy odzysk energii
    2: Umiarkowane hamowanie + Średni odzysk
    3: Średnie hamowanie + Umiarkowany odzysk
    4: Mocne hamowanie + Niski odzysk
    5: Najmocniejsze hamowanie + Minimalny odzysk

    ZASADA DZIAŁANIA:
      - Niższe wartości = lepszy odzysk energii
      - Wyższe wartości = silniejsze hamowanie
      - Hamowanie działa na klamki hamulcowe
      - W niektórych modelach działa też na manetkę

    ⚠️ WAŻNE OSTRZEŻENIA:
    1. Hamowanie regeneracyjne może powodować obluzowanie osi silnika
      - ZAWSZE używaj 2 blokad osi
      - Regularnie sprawdzaj dokręcenie
    2. Wybór ustawienia:
      - Priorytet odzysku energii → ustaw C13=1
      - Priorytet siły hamowania → ustaw C13=5
      - Kompromis → ustaw C13=2 lub C13=3

    💡 WSKAZÓWKA: Zacznij od niższych wartości i zwiększaj stopniowo, obserwując zachowanie roweru i efektywność odzysku energii.`
    },

    'kt-c14-info': {
        title: 'C14 - Poziomy PAS',
        description: `Konfiguracja poziomów wspomagania i ich charakterystyk.`
    },

    'kt-c15-info': {
        title: 'C15 - Prowadzenie',
        description: `Ustawienia trybu prowadzenia roweru (walk assist).`
    },

    // Parametry L sterownika //
    
    'kt-l1-info': {
        title: '🔋 L1 - Napięcie minimalne (LVC)',
        description: `Ustawienie minimalnego napięcia pracy sterownika (Low Voltage Cutoff).

    Dostępne opcje:
    0: Automatyczny dobór progu przez sterownik
      - 24V → wyłączenie przy 20V
      - 36V → wyłączenie przy 30V      
      - 48V → wyłączenie przy 40V
      
    Wymuszenie progu wyłączenia:
    1: 20V
    2: 30V
    3: 40V

    ⚠️ UWAGA: 
    Ustawienie zbyt niskiego progu może prowadzić do uszkodzenia akumulatora!`
    },

    'kt-l2-info': {
        title: '⚡ L2 - Silniki wysokoobrotowe',
        description: `Parametr dla silników o wysokich obrotach (>5000 RPM).

    Wartości:
    0: Tryb normalny
    1: Tryb wysokoobrotowy - wartość P1 jest mnożona ×2

    📝 UWAGA:
      - Parametr jest powiązany z ustawieniem P1
      - Używaj tylko dla silników > 5000 RPM`
    },

    'kt-l3-info': {
        title: '🔄 L3 - Tryb DUAL',
        description: `Konfiguracja zachowania dla sterowników z podwójnym kompletem czujników halla.

    Opcje:
    0: Tryb automatyczny
      - Automatyczne przełączenie na sprawny komplet czujników
      - Kontynuacja pracy po awarii jednego kompletu

    1: Tryb bezpieczny
      - Wyłączenie przy awarii czujników
      - Sygnalizacja błędu

    ⚠️ WAŻNE: 
    Dotyczy tylko sterowników z funkcją DUAL (2 komplety czujników halla)`
    },

    // Parametry sterownika S866 //

    's866-p1-info': {
        title: 'P1 - Jasność podświetlenia',
        description: `Regulacja poziomu podświetlenia wyświetlacza.

    Dostępne poziomy:
    1: Najciemniejszy
    2: Średni
    3: Najjaśniejszy`
    },

    's866-p2-info': {
        title: 'P2 - Jednostka pomiaru',
        description: `Wybór jednostki wyświetlania dystansu i prędkości.

    Opcje:
    0: Kilometry (km)
    1: Mile`
    },

    's866-p3-info': {
        title: 'P3 - Napięcie nominalne',
        description: `Wybór napięcia nominalnego systemu.

    Dostępne opcje:
    - 24V
    - 36V
    - 48V
    - 60V`
    },

    's866-p4-info': {
        title: 'P4 - Czas automatycznego uśpienia',
        description: `Czas bezczynności po którym wyświetlacz przejdzie w stan uśpienia.

    Zakres: 0-60 minut
    0: Funkcja wyłączona (brak auto-uśpienia)
    1-60: Czas w minutach do przejścia w stan uśpienia`
    },

    's866-p5-info': {
        title: 'P5 - Tryb wspomagania PAS',
        description: `Wybór liczby poziomów wspomagania.

    Opcje:
    0: Tryb 3-biegowy
    1: Tryb 5-biegowy`
    },

    's866-p6-info': {
        title: 'P6 - Rozmiar koła',
        description: `Ustawienie średnicy koła dla prawidłowego obliczania prędkości.

    Zakres: 5.0 - 50.0 cali
    Dokładność: 0.1 cala

    ⚠️ WAŻNE 
    Ten parametr jest kluczowy dla prawidłowego wyświetlania prędkości.`
    },

    's866-p7-info': {
        title: 'P7 - Liczba magnesów czujnika prędkości',
        description: `Konfiguracja czujnika prędkości.

    Zakres: 1-100 magnesów

    Dla silnika z przekładnią:
    Wartość = Liczba magnesów × Przełożenie

    Przykład:
    - 20 magnesów, przełożenie 4.3
    - Wartość = 20 × 4.3 = 86`
    },

    's866-p8-info': {
        title: 'P8 - Limit prędkości',
        description: `Ustawienie maksymalnej prędkości pojazdu.

    Zakres: 0-100 km/h
    100: Brak limitu prędkości

    ⚠️ UWAGA: 
    - Dokładność: ±1 km/h
    - Limit dotyczy zarówno mocy jak i skrętu
    - Wartości są zawsze w km/h, nawet przy wyświetlaniu w milach`
    },

    's866-p9-info': {
        title: 'P9 - Tryb startu',
        description: `Wybór sposobu uruchamiania wspomagania.

    0: Start od zera (zero start)
    1: Start z rozbiegu (non-zero start)`
    },

    's866-p10-info': {
        title: 'P10 - Tryb jazdy',
        description: `Wybór trybu wspomagania.

    0: Wspomaganie PAS (moc zależna od siły pedałowania)
    1: Tryb elektryczny (sterowanie manetką)
    2: Tryb hybrydowy (PAS + manetka)`
    },

    's866-p11-info': {
        title: 'P11 - Czułość PAS',
        description: `Regulacja czułości czujnika wspomagania.

    Zakres: 1-24
    - Niższe wartości = mniejsza czułość
    - Wyższe wartości = większa czułość`
    },

    's866-p12-info': {
        title: 'P12 - Siła startu PAS',
        description: `Intensywność wspomagania przy rozpoczęciu pedałowania.

    Zakres: 1-5
    1: Najsłabszy start
    5: Najmocniejszy start`
    },

    's866-p13-info': {
        title: 'P13 - Typ czujnika PAS',
        description: `Wybór typu czujnika PAS według liczby magnesów.

    Dostępne opcje:
    - 5 magnesów
    - 8 magnesów
    - 12 magnesów`
    },

    's866-p14-info': {
        title: 'P14 - Limit prądu kontrolera',
        description: `Ustawienie maksymalnego prądu kontrolera.

    Zakres: 1-20A`
    },

    's866-p15-info': {
        title: 'P15 - Napięcie odcięcia',
        description: `Próg napięcia przy którym kontroler wyłączy się.`
    },

    's866-p16-info': {
        title: 'P16 - Reset licznika ODO',
        description: `Resetowanie licznika całkowitego przebiegu.

    Aby zresetować:
    Przytrzymaj przycisk przez 5 sekund`
    },

    's866-p17-info': {
        title: 'P17 - Tempomat',
        description: `Włączenie/wyłączenie funkcji tempomatu.

    0: Tempomat wyłączony
    1: Tempomat włączony

    ⚠️ Uwaga
    Działa tylko z protokołem 2`
    },

    's866-p18-info': {
        title: 'P18 - Kalibracja prędkości',
        description: `Współczynnik korekcji wyświetlanej prędkości.

    Zakres: 50% - 150%`
    },

    's866-p19-info': {
        title: 'P19 - Bieg zerowy PAS',
        description: `Konfiguracja biegu zerowego w systemie PAS.

    0: Z biegiem zerowym
    1: Bez biegu zerowego`
    },

    's866-p20-info': {
        title: 'P20 - Protokół komunikacji',
        description: `Wybór protokołu komunikacji sterownika.

    0: Protokół 2
    1: Protokół 5S
    2: Protokół Standby
    3: Protokół Standby alternatywny`
    },

    // Sekcja ustawień ogólnych //

    'general-settings-info': {
        title: '⚙️ Ustawienia ogólne',
        description: `Podstawowa konfiguracja systemu.

    🚲 Parametry roweru:
      - Rozmiar koła: wpływa na pomiar prędkości
      - Limit prędkości: zgodnie z przepisami
      - Jednostki: km/h lub mph
    
    ⏰ Automatyczne wyłączanie:
      - Czas do uśpienia: 0-60 minut
      - 0 = funkcja wyłączona
        
    💾 Opcje konfiguracji:
      - Reset do ustawień fabrycznych
      - Kopia zapasowa konfiguracji
    
    ⚠️ UWAGA:
    Reset ustawień usuwa wszystkie
    spersonalizowane konfiguracje!`
    },

    // Licznik całkowity
    'total-odometer-info': {
        title: 'Przebieg całkowity',
        description: `Całkowity przebieg roweru w kilometrach. Można ustawić wartość początkową, np. przy przeniesieniu z innego licznika.`
    },

    // Rozmiar koła
    'wheel-size-info': {
        title: 'Rozmiar koła',
        description: `Wybierz rozmiar koła swojego roweru. Jest to ważne dla prawidłowego obliczania prędkości i dystansu.`
    },

    // Sekcja Bluetooth
    'bluetooth-config-info': {
        title: '📶 Konfiguracja Bluetooth',
        description: `Panel konfiguracji połączeń bezprzewodowych.

	'front-tpms-mac-info': {
		title: 'Adres MAC przedniego czujnika TPMS',
		description: `Wprowadź adres MAC przedniego czujnika TPMS w formacie XX:XX:XX:XX:XX:XX.    
		Możesz znaleźć adres MAC używając aplikacji do skanowania Bluetooth na telefonie podczas kalibracji czujników.    
		Przykład: A1:B2:C3:D4:E5:F6`
	},

	'rear-tpms-mac-info': {
		title: 'Adres MAC tylnego czujnika TPMS',
		description: `Wprowadź adres MAC tylnego czujnika TPMS w formacie XX:XX:XX:XX:XX:XX.
		
		Możesz znaleźć adres MAC używając aplikacji do skanowania Bluetooth na telefonie podczas kalibracji czujników.
		
		Przykład: A1:B2:C3:D4:E5:F6`
	},

    🔋 BMS (Battery Management System):
      - Monitoring stanu baterii
      - Pomiar temperatury ogniw
      - Kontrola napięcia
      - Statystyki ładowania
    
    🌡️ TPMS (Tire Pressure Monitoring):
      - Ciśnienie w oponach
      - Temperatura opon
      - Stan baterii czujników
    
    📱 Opcje połączenia:
      - Auto-łączenie ze znanymi urządzeniami
      - Skanowanie nowych czujników
      - Parowanie urządzeń
    
    💡 WSKAZÓWKI:
      - Utrzymuj czujniki w zasięgu 2-3m
      - Sprawdzaj stan baterii czujników
      - Regularnie aktualizuj oprogramowanie`
    },

    'bms-info': {
        title: 'System zarządzania baterią (BMS)',
        description: `BMS (Battery Management System) to system monitorujący stan baterii. Po włączeniu tej opcji, urządzenie będzie odbierać dane o stanie baterii przez Bluetooth, takie jak:
             
    • Pojemność (Ah)
    • Energia (Wh)
    • Temperatura ogniw (°C)
    • Stan naładowania (SOC)`
    },

    // Opis dla TPMS
    'tpms-info': {
        title: 'System monitorowania ciśnienia w oponach (TPMS)',
        description: `TPMS (Tire Pressure Monitoring System) to system monitorujący ciśnienie w oponach. Po włączeniu tej opcji, urządzenie będzie odbierać dane z czujników przez Bluetooth, takie jak:
                
    • Ciśnienie w oponach (bar)
    • Temperatura opon (°C)
    • Stan baterii czujników (V)`
    },
	
	'front-tpms-mac-info': {
		title: 'Adres MAC przedniego czujnika TPMS',
		description: `Wprowadź adres MAC przedniego czujnika TPMS w formacie XX:XX:XX:XX:XX:XX.    
		Możesz znaleźć adres MAC używając aplikacji do skanowania Bluetooth na telefonie podczas kalibracji czujników.    
		Przykład: A1:B2:C3:D4:E5:F6`
	},

	'rear-tpms-mac-info': {
		title: 'Adres MAC tylnego czujnika TPMS',
		description: `Wprowadź adres MAC tylnego czujnika TPMS w formacie XX:XX:XX:XX:XX:XX.		
		Możesz znaleźć adres MAC używając aplikacji do skanowania Bluetooth na telefonie podczas kalibracji czujników.		
		Przykład: A1:B2:C3:D4:E5:F6`
	}
};

function showRTCInfo() {
    showModal(infoContent['rtc-info'].title, infoContent['rtc-info'].description);
}

// Funkcja pobierająca wersję systemu
async function fetchSystemVersion() {
    try {
        const response = await fetch('/api/version');
        const data = await response.json();
        if (data.version) {
            document.getElementById('system-version').textContent = data.version;
        }
    } catch (error) {
        console.error('Błąd podczas pobierania wersji systemu:', error);
        document.getElementById('system-version').textContent = 'N/A';
    }
}

//function saveGeneralSettings() {
//    const wheelSize = document.getElementById('wheel-size').value;
//    
//    fetch('/save-general-settings', {
//        method: 'POST',
//        headers: {
//            'Content-Type': 'application/json',
//        },
//        body: JSON.stringify({
//            wheelSize: wheelSize
//        })
//    })
//    .then(response => response.json())
//    .then(data => {
//        if (data.success) {
//            console.log('Ustawienia ogólne zapisane pomyślnie');
//        }
//    })
//    .catch(error => {
//        console.error('Błąd podczas zapisywania ustawień ogólnych:', error);
//    });
//}

// Zmodyfikuj istniejącą funkcję saveGeneralSettings
function saveGeneralSettings() {
    const wheelSize = document.getElementById('wheel-size').value;
    const odometer = document.getElementById('total-odometer').value;
    
    // Najpierw zapisz ustawienia ogólne
    fetch('/save-general-settings', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({
            wheelSize: wheelSize
        })
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            console.log('Ustawienia ogólne zapisane pomyślnie');
            // Jeśli ustawienia ogólne zostały zapisane, zapisz również licznik
            return saveOdometerValue();
        }
    })
    .catch(error => {
        console.error('Błąd podczas zapisywania ustawień ogólnych:', error);
    });
}

async function loadBluetoothConfig() {
    try {
        console.log("Wczytywanie konfiguracji Bluetooth...");
        const response = await fetch('/get-bluetooth-config');
        const data = await response.json();
        console.log("Otrzymane dane z serwera:", data);
        
        if (data) {
            document.getElementById('bms-enabled').value = data.bmsEnabled.toString();
            document.getElementById('tpms-enabled').value = data.tpmsEnabled.toString();
            console.log("Ustawiono tpms-enabled na:", data.tpmsEnabled.toString());
            
            // Dodaj obsługę MAC adresów TPMS
            const tpmsFields = document.getElementById('tpms-fields');
            if (tpmsFields) {
                // Pokaż/ukryj pola w zależności od włączenia TPMS
                tpmsFields.style.display = data.tpmsEnabled ? 'block' : 'none';
                console.log("Ustawiono widoczność tpms-fields w loadBluetoothConfig na:", tpmsFields.style.display);
            } else {
                console.error("Element tpms-fields nie został znaleziony!");
            }
            
            // Ustaw wartości MAC adresów jeśli istnieją
            if (document.getElementById('front-tpms-mac')) {
                document.getElementById('front-tpms-mac').value = data.frontTpmsMac || '';
                console.log("Ustawiono front-tpms-mac na:", data.frontTpmsMac || '');
            } else {
                console.error("Element front-tpms-mac nie został znaleziony!");
            }
            
            if (document.getElementById('rear-tpms-mac')) {
                document.getElementById('rear-tpms-mac').value = data.rearTpmsMac || '';
                console.log("Ustawiono rear-tpms-mac na:", data.rearTpmsMac || '');
            } else {
                console.error("Element rear-tpms-mac nie został znaleziony!");
            }
            
            // Wywołaj funkcję toggleTpmsFields() aby upewnić się, że elementy są widoczne/ukryte
            toggleTpmsFields();
        }
    } catch (error) {
        console.error('Błąd podczas pobierania konfiguracji Bluetooth:', error);
    }
}

window.onload = function() {
    loadGeneralSettings();
    loadBluetoothConfig();
    // ... (pozostałe istniejące wywołania)
};

// Funkcja do pokazywania/ukrywania pól MAC adresów TPMS
function toggleTpmsFields() {
    const tpmsEnabled = document.getElementById('tpms-enabled').value === 'true';
    const tpmsFields = document.getElementById('tpms-fields');
    
    if (tpmsFields) {
        tpmsFields.style.display = tpmsEnabled ? 'block' : 'none';
    }
}

function saveBluetoothConfig() {
    const bmsEnabled = document.getElementById('bms-enabled').value;
    const tpmsEnabled = document.getElementById('tpms-enabled').value;
    
    // Przygotuj dane do wysłania
    const configData = {
        bmsEnabled: bmsEnabled === 'true',
        tpmsEnabled: tpmsEnabled === 'true'
    };
    
    // Dodaj MAC adresy tylko jeśli TPMS jest włączone
    if (tpmsEnabled === 'true') {
        if (document.getElementById('front-tpms-mac')) {
            configData.frontTpmsMac = document.getElementById('front-tpms-mac').value;
        }
        
        if (document.getElementById('rear-tpms-mac')) {
            configData.rearTpmsMac = document.getElementById('rear-tpms-mac').value;
        }
    }
    
    // Wyślij dane do serwera
    fetch('/save-bluetooth-config', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(configData)
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            console.log('Konfiguracja Bluetooth zapisana pomyślnie');
            alert('Zapisano konfigurację Bluetooth');
        } else {
            alert('Błąd podczas zapisywania konfiguracji');
        }
    })
    .catch(error => {
        console.error('Błąd podczas zapisywania konfiguracji Bluetooth:', error);
        alert('Błąd podczas zapisywania konfiguracji: ' + error.message);
    });
}

function initializeCollapsibleSections() {
    document.querySelectorAll('.collapsible').forEach(section => {
        const content = section.querySelector('.card-content');
        const collapseBtn = section.querySelector('.collapse-btn');
        
        // Ustaw początkowy stan (zwinięty)
        content.style.display = 'none';
        
        // Nasłuchuj tylko kliknięć w przycisk trybika
        collapseBtn.addEventListener('click', (e) => {
            e.stopPropagation(); // Zatrzymaj propagację zdarzenia
            
            // Przełącz widoczność zawartości
            const isCollapsed = content.style.display === 'none';
            content.style.display = isCollapsed ? 'block' : 'none';
            collapseBtn.classList.toggle('rotated', isCollapsed);
            
            // Zapisz stan w localStorage
            const sectionId = section.classList[1];
            localStorage.setItem(`section_${sectionId}`, isCollapsed ? 'expanded' : 'collapsed');
        });
        
        // Przywróć poprzedni stan z localStorage
        const sectionId = section.classList[1];
        const savedState = localStorage.getItem(`section_${sectionId}`);
        if (savedState === 'expanded') {
            content.style.display = 'block';
            collapseBtn.classList.add('rotated');
        }
    });
}

// Zastąp istniejącą funkcję saveGeneralSettings tą nową wersją
async function saveGeneralSettings() {
    try {
        const wheelSize = document.getElementById('wheel-size').value;
        const odometer = document.getElementById('total-odometer').value;
        
        // Najpierw zapisz ustawienia ogólne
        const generalResponse = await fetch('/save-general-settings', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                wheelSize: wheelSize
            })
        });

        if (!generalResponse.ok) {
            throw new Error('Błąd podczas zapisywania ustawień ogólnych');
        }

        const generalData = await generalResponse.json();
        
        // Następnie zapisz stan licznika
        const odometerResponse = await fetch('/api/setOdometer', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            body: `value=${odometer}`
        });

        if (!odometerResponse.ok) {
            throw new Error('Błąd podczas zapisywania stanu licznika');
        }

        const odometerResult = await odometerResponse.text();
        
        if (generalData.success && odometerResult === 'OK') {
            // Wyświetl potwierdzenie tylko jeśli oba zapisy się powiodły
            alert('Zapisano ustawienia ogólne');
            // Odśwież wyświetlane wartości
            await loadGeneralSettings();
        } else {
            throw new Error('Nie udało się zapisać wszystkich ustawień');
        }
    } catch (error) {
        console.error('Błąd:', error);
        alert('Błąd podczas zapisywania ustawień: ' + error.message);
    }
}

// Upewnij się, że ta funkcja jest obecna i prawidłowa
async function loadGeneralSettings() {
    try {
        // Pobierz rozmiar koła
        const response = await fetch('/get-general-settings');
        const data = await response.json();
        
        if (data) {
            // Ustaw rozmiar koła
            document.getElementById('wheel-size').value = data.wheelSize;
        }
        
        // Pobierz stan licznika
        const odometerResponse = await fetch('/api/odometer');
        if (odometerResponse.ok) {
            const odometerValue = await odometerResponse.text();
            document.getElementById('total-odometer').value = Math.floor(parseFloat(odometerValue));
        }
    } catch (error) {
        console.error('Błąd podczas wczytywania ustawień ogólnych:', error);
    }
}

/*
WAŻNE KOMUNIKATY:
⚠️ - Ważne ostrzeżenia
💡 - Wskazówka
📝 - Uwaga

PARAMETRY TECHNICZNE:
⚡ - Ustawienia mocy/elektryczne
🔋 - Ustawienia baterii
🔌 - Ustawienia elektryczne
🌡️ - Parametry temperatury
📊 - Parametry pomiarowe

USTAWIENIA MECHANICZNE:
🚲 - Ogólne ustawienia roweru
⚙️ - Ustawienia mechaniczne
🔄 - Funkcje regeneracji

INTERFEJS I CZAS:
📱 - Ustawienia interfejsu
⏰ - Ustawienia czasowe
💾 - Opcje zapisu/resetu

BEZPIECZEŃSTWO I WYDAJNOŚĆ:
🔒 - Ustawienia zabezpieczeń
📈 - Parametry wydajności
🛠️ - Ustawienia serwisowe
🔧 - KONFIGURACJA
*/
