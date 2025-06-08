// Główny plik JavaScript zawierający wspólne funkcje i inicjalizację

// Funkcja pomocnicza do debugowania
function debug(...args) {
    if (typeof console !== 'undefined') {
        console.log('[DEBUG]', ...args);
    }
}

// Główna inicjalizacja po załadowaniu DOM
document.addEventListener('DOMContentLoaded', async function() {
    debug('Inicjalizacja aplikacji...');

    try {
        // Najpierw zainicjalizuj rozwijane sekcje
        initializeCollapsibleSections();
        
        // Inicjalizacja modalu informacyjnego
        setupModal();
        
        // Inicjalizacja zegara
        let clockInterval = initializeClock();

        // Poczekaj na pewność załadowania DOM
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

        // Inicjalizacja WebSocket
        setupWebSocket();
        
        // Inicjalizacja formularzy
        setupFormListeners();

        debug('Inicjalizacja zakończona pomyślnie');
    } catch (error) {
        console.error('Błąd podczas inicjalizacji:', error);
    }
});

// Funkcja inicjalizująca rozwijane sekcje
function initializeCollapsibleSections() {
    console.log('Inicjalizacja rozwijanch sekcji...');
    
    // Pobierz wszystkie przyciski zwijania
    const collapseButtons = document.querySelectorAll('.collapse-btn');
    console.log('Znaleziono przycisków zwijania:', collapseButtons.length);
    
    collapseButtons.forEach(button => {
        button.addEventListener('click', function(event) {
            console.log('Kliknięto przycisk zwijania');
            event.stopPropagation(); // Zatrzymaj propagację zdarzenia
            
            // Znajdź rodzica (.card)
            const card = this.closest('.card');
            
            if (card) {
                // Znajdź zawartość karty (.card-content)
                const content = card.querySelector('.card-content');
                
                if (content) {
                    // Przełącz widoczność zawartości
                    const isCollapsed = content.style.display === 'none' || content.style.display === '';
                    content.style.display = isCollapsed ? 'block' : 'none';
                    
                    // Dodaj/usuń klasę dla animacji obrotu przycisku
                    this.classList.toggle('rotated', isCollapsed);
                    
                    console.log('Przełączono sekcję na:', isCollapsed ? 'rozwiniętą' : 'zwiniętą');
                } else {
                    console.warn('Nie znaleziono zawartości karty');
                }
            } else {
                console.warn('Nie znaleziono karty rodzica');
            }
        });
    });

    // Domyślnie zwiń wszystkie sekcje
    const cardContents = document.querySelectorAll('.card-content');
    console.log('Znaleziono sekcji do zwinięcia:', cardContents.length);
    
    cardContents.forEach(content => {
        content.style.display = 'none';
    });
    
    console.log('Zwinięto wszystkie sekcje domyślnie');
}

// Funkcja do wyświetlania powiadomień
function showNotification(message, type = 'info') {
    // Sprawdź czy element powiadomienia istnieje
    let notification = document.getElementById('notification');
    if (!notification) {
        notification = document.createElement('div');
        notification.id = 'notification';
        document.body.appendChild(notification);
    }
    
    // Ustaw typ powiadomienia
    notification.className = 'notification ' + type;
    notification.textContent = message;
    notification.style.display = 'block';
    
    // Ukryj powiadomienie po 3 sekundach
    setTimeout(() => {
        notification.style.opacity = '0';
        setTimeout(() => {
            notification.style.display = 'none';
            notification.style.opacity = '1';
        }, 500);
    }, 3000);
}

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

// Funkcja inicjalizująca modal informacyjny
function setupModal() {
    console.log('Inicjalizacja modala...');
    
    const modal = document.getElementById('info-modal');
    const modalTitle = document.getElementById('modal-title');
    const modalDescription = document.getElementById('modal-description');
    
    if (!modal || !modalTitle || !modalDescription) {
        console.error('Brak elementów modala!');
        return;
    }
    
    console.log('Znaleziono elementy modala');
    
    // Otwieranie modala przez info-icons
    document.querySelectorAll('.info-icon').forEach(button => {
        button.addEventListener('click', function(event) {
            event.stopPropagation(); // Zatrzymaj propagację zdarzenia
            const infoId = this.getAttribute('data-info');
            console.log('Kliknięto przycisk info dla:', infoId);
            
            const info = infoContent[infoId];
            
            if (info) {
                modalTitle.textContent = info.title;
                modalDescription.textContent = info.description;
                modal.style.display = 'block';
                console.log('Wyświetlono modal dla:', infoId);
            } else {
                console.warn('Nie znaleziono opisu dla:', infoId);
            }
        });
    });
    
    // Zamykanie modala
    const closeButton = document.querySelector('.close-modal');
    if (closeButton) {
        closeButton.addEventListener('click', () => {
            console.log('Zamknięto modal przyciskiem X');
            modal.style.display = 'none';
        });
    }
    
    // Zamykanie po kliknięciu poza modalem
    window.addEventListener('click', (event) => {
        if (event.target === modal) {
            console.log('Zamknięto modal kliknięciem poza nim');
            modal.style.display = 'none';
        }
    });
    
    console.log('Inicjalizacja modala zakończona');
}

// Funkcja do pokazywania/ukrywania pól MAC adresów TPMS
function toggleTpmsFields() {
    const tpmsEnabled = document.getElementById('tpms-enabled').value === 'true';
    const tpmsFields = document.getElementById('tpms-fields');
    
    if (tpmsFields) {
        tpmsFields.style.display = tpmsEnabled ? 'block' : 'none';
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

// Funkcja do obsługi nasłuchiwaczy zdarzeń formularzy
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

    // Dodaj listener do przycisku zapisu świateł
    const saveButton = document.getElementById('save-light-config');
    if (saveButton) {
        saveButton.addEventListener('click', saveLightConfig);
        debug('Przycisk zapisywania ustawień świateł zainicjalizowany');
    } else {
        console.error('Nie znaleziono przycisku save-light-config');
    }
    
    // Walidacja dla pól numerycznych wyświetlacza
    document.querySelectorAll('#day-brightness, #night-brightness').forEach(input => {
        input.addEventListener('input', function() {
            let value = parseInt(this.value);
            if (value < 0) this.value = 0;
            if (value > 100) this.value = 100;
        });
    });
	
	const controllerTypeSelect = document.getElementById('controller-type');
	if (controllerTypeSelect) {
		controllerTypeSelect.addEventListener('change', toggleControllerParams);
	}
	
	const displayAutoSelect = document.getElementById('display-auto');
	if (displayAutoSelect) {
		displayAutoSelect.addEventListener('change', toggleAutoBrightness);
	}
	
	const tpmsEnabledSelect = document.getElementById('tpms-enabled');
	if (tpmsEnabledSelect) {
		tpmsEnabledSelect.addEventListener('change', toggleTpmsFields);
	}
	
	const displaySaveButton = document.getElementById('save-display-config');
	if (displaySaveButton) {
		displaySaveButton.addEventListener('click', saveDisplayConfig);
	}
	
	const controllerSaveButton = document.getElementById('save-controller-config');
	if (controllerSaveButton) {
		controllerSaveButton.addEventListener('click', saveControllerConfig);
	}
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

// Funkcja do zapisywania ustawień ogólnych
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
            //alert('Zapisano ustawienia ogólne');
			showNotification('Zapisano ustawienia ogólne', 'success');
            // Odśwież wyświetlane wartości
            await loadGeneralSettings();
        } else {
            throw new Error('Nie udało się zapisać wszystkich ustawień');
        }
    } catch (error) {
        //console.error('Błąd:', error);
        //alert('Błąd podczas zapisywania ustawień: ' + error.message);
        handleError(error, 'Błąd podczas zapisywania ustawień ogólnych');
	}
}

// Funkcja wczytująca ustawienia ogólne
async function loadGeneralSettings() {
    try {
        // Pobierz rozmiar koła
        const response = await fetch('/get-general-settings');
        const data = await response.json();
        
        if (data) {
            // Ustaw rozmiar koła
            updateElementValue('wheel-size', data.wheelSize);
        }
        
        // Pobierz stan licznika
        const odometerResponse = await fetch('/api/odometer');
        if (odometerResponse.ok) {
            const odometerValue = await odometerResponse.text();
            updateElementValue('total-odometer', Math.floor(parseFloat(odometerValue)));
        }
    } catch (error) {
        console.error('Błąd podczas wczytywania ustawień ogólnych:', error);
    }
}

// Funkcja do wczytywania wartości licznika
async function loadOdometerValue() {
    try {
        const response = await fetch('/api/odometer');
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        const value = await response.text();
        //document.getElementById('total-odometer').value = Math.floor(parseFloat(value));
        updateElementValue('total-odometer', Math.floor(parseFloat(value)));
	} catch (error) {
        console.error('Error loading odometer:', error);
    }
}

// Funkcja do zapisywania wartości licznika
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
        handleError(error, 'Błąd podczas zapisywania licznika kilometrów');
    }
}

// Funkcja do wczytywania konfiguracji Bluetooth
async function loadBluetoothConfig() {
    try {
        console.log("Wczytywanie konfiguracji Bluetooth...");
        const response = await fetch('/get-bluetooth-config');
        const data = await response.json();
        console.log("Otrzymane dane z serwera:", data);
        
        if (data) {
            //document.getElementById('bms-enabled').value = data.bmsEnabled.toString();
            //document.getElementById('tpms-enabled').value = data.tpmsEnabled.toString();
            updateElementValue('bms-enabled', data.bmsEnabled.toString());
            updateElementValue('tpms-enabled', data.tpmsEnabled.toString());			
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

// Funkcja do zapisywania konfiguracji Bluetooth
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
            showNotification('Zapisano konfigurację Bluetooth', 'success');
        } else {
            showNotification('Błąd podczas zapisywania konfiguracji', 'error');
        }
    })
    .catch(error => {
        handleError(error, 'Błąd podczas zapisywania konfiguracji Bluetooth');
    });
}

// Funkcja pobierająca i aktualizująca konfigurację wyświetlacza
async function fetchDisplayConfig() {
    try {
        const response = await fetch('/api/status');
        const data = await response.json();
        if (data.backlight) {
            //document.getElementById('day-brightness').value = data.backlight.dayBrightness;
            //document.getElementById('night-brightness').value = data.backlight.nightBrightness;
            //document.getElementById('display-auto').value = data.backlight.autoMode.toString();
            // Ustawienie jasności normalnej na podstawie jasności dziennej w trybie manualnym
            //document.getElementById('brightness').value = data.backlight.dayBrightness;
            updateElementValue('day-brightness', data.backlight.dayBrightness);
            updateElementValue('night-brightness', data.backlight.nightBrightness);
            updateElementValue('display-auto', data.backlight.autoMode.toString());
            // Ustawienie jasności normalnej na podstawie jasności dziennej w trybie manualnym
            updateElementValue('brightness', data.backlight.dayBrightness);
			
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

        console.log('Wysyłane dane:', data);

        const response = await fetch('/api/display/config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(data)
        });

        const result = await response.json();
        console.log('Odpowiedź serwera:', result);

        if (result.status === 'ok') {
            showNotification('Zapisano ustawienia wyświetlacza', 'success');
            await fetchDisplayConfig(); // odśwież wyświetlane ustawienia
        } else {
            throw new Error(result.message || 'Błąd odpowiedzi serwera');
        }
    } catch (error) {
        handleError(error, 'Błąd podczas zapisywania ustawień wyświetlacza');
    }
}

// Inicjalizacja WebSocket
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

// Funkcja do aktualizacji statusu świateł
function updateLightStatus(lights) {
    try {
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

// Funkcja do aktualizacji formularza na podstawie otrzymanego stanu
function updateLightForm(lights) {
    debug('Aktualizacja formularza, otrzymane dane:', lights);
    
    try {
        const dayLightsElement = document.getElementById('day-lights');
        const nightLightsElement = document.getElementById('night-lights');
        const dayBlinkElement = document.getElementById('day-blink');
        const nightBlinkElement = document.getElementById('night-blink');
        const blinkFrequencyElement = document.getElementById('blink-frequency');
        
        //if (dayLightsElement) dayLightsElement.value = lights.dayLights || 'NONE';
		//if (nightLightsElement) nightLightsElement.value = lights.nightLights || 'NONE';
        //if (dayBlinkElement) dayBlinkElement.checked = Boolean(lights.dayBlink);
        //if (nightBlinkElement) nightBlinkElement.checked = Boolean(lights.nightBlink);
        //if (blinkFrequencyElement) blinkFrequencyElement.value = lights.blinkFrequency || 500;
        updateElementValue('day-lights', lights.dayLights, 'NONE');
		updateElementValue('night-lights', lights.nightLights, 'NONE');
		updateElementValue('day-blink', lights.dayBlink);
		updateElementValue('night-blink', lights.nightBlink);
		updateElementValue('blink-frequency', lights.blinkFrequency, 500);

        debug('Formularz zaktualizowany pomyślnie');
    } catch (error) {
        console.error('Błąd podczas aktualizacji formularza:', error);
    }
}

// Funkcja pobierająca aktualny stan
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
            data.p = {};
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
            showNotification('Zapisano ustawienia sterownika', 'success');
            await fetchControllerConfig(); // Odśwież widok
        } else {
            throw new Error('Błąd odpowiedzi serwera');
        }
    } catch (error) {
        handleError(error, 'Błąd podczas zapisywania ustawień sterownika');
    }
}

// Inicjuj po załadowaniu strony
window.onload = function() {
    loadGeneralSettings();
    loadBluetoothConfig();
};

function updateElementValue(elementId, value, defaultValue = '') {
    const element = document.getElementById(elementId);
    if (element) {
        if (element.type === 'checkbox') {
            element.checked = Boolean(value);
        } else {
            element.value = value !== undefined ? value : defaultValue;
        }
        return true;
    }
    return false;
}

function handleError(error, userMessage = 'Wystąpił błąd') {
    console.error(error);
    showNotification(userMessage + ': ' + error.message, 'error');
}
