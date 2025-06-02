// Zmienne globalne
let saveTimeout = null;
let loadTimeout = null;

// Funkcja pomocnicza do debugowania
function debug(...args) {
    if (typeof console !== 'undefined') {
        console.log('[DEBUG]', ...args);
    }
}

// Funkcja wczytywania konfiguracji świateł
async function loadLightConfig() {
    debug('Rozpoczynam wczytywanie konfiguracji świateł...');
    
    // Anuluj poprzedni timeout jeśli istnieje
    if (typeof loadTimeout !== 'undefined' && loadTimeout) {
        clearTimeout(loadTimeout);
    }

    // Ustaw nowy timeout
    loadTimeout = setTimeout(async () => {
        try {
            debug('Wysyłam żądanie GET do /api/status');
            const response = await fetch('/api/status', {cache: 'no-store'});
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            const data = await response.json();
            debug('Otrzymane dane:', data);

            if (data.lights) {
                debug('Aktualizacja formularza, otrzymane dane:', data.lights);
                
                // Parsowanie konfiguracji świateł dziennych
                const dayLights = data.lights.dayLights || 'NONE';
                document.getElementById('day-front').checked = dayLights.includes('FRONT');
                document.getElementById('day-drl').checked = dayLights.includes('DRL');
                document.getElementById('day-rear').checked = dayLights.includes('REAR');
                document.getElementById('day-blink').checked = Boolean(data.lights.dayBlink);
                
                // Parsowanie konfiguracji świateł nocnych
                const nightLights = data.lights.nightLights || 'NONE';
                document.getElementById('night-front').checked = nightLights.includes('FRONT');
                document.getElementById('night-drl').checked = nightLights.includes('DRL');
                document.getElementById('night-rear').checked = nightLights.includes('REAR');
                document.getElementById('night-blink').checked = Boolean(data.lights.nightBlink);
                
                // Ustawienie częstotliwości migania
                document.getElementById('blink-frequency').value = data.lights.blinkFrequency || 500;

                debug('Formularz zaktualizowany pomyślnie');
            }
        } catch (error) {
            console.error('Błąd podczas wczytywania konfiguracji świateł:', error);
        }
    }, 250);
}

// Funkcja zapisywania konfiguracji świateł
async function saveLightConfig() {
    debug('Rozpoczynam zapisywanie konfiguracji świateł');
    
    try {
        // Zbierz elementy formularza
        const elements = {
            dayFront: document.getElementById('day-front'),
            dayDRL: document.getElementById('day-drl'),
            dayRear: document.getElementById('day-rear'),
            dayBlink: document.getElementById('day-blink'),
            nightFront: document.getElementById('night-front'), 
            nightDRL: document.getElementById('night-drl'),
            nightRear: document.getElementById('night-rear'),
            nightBlink: document.getElementById('night-blink'),
            blinkFrequency: document.getElementById('blink-frequency')
        };
        
        // Sprawdź czy wszystkie elementy istnieją
        const missingElements = [];
        for (const [key, element] of Object.entries(elements)) {
            if (!element) {
                missingElements.push(key);
            }
        }
        
        if (missingElements.length > 0) {
            throw new Error(`Brak elementów: ${missingElements.join(', ')}`);
        }
        
        // Pobierz wartości
        const dayFront = elements.dayFront.checked;
        const dayDRL = elements.dayDRL.checked;
        const dayRear = elements.dayRear.checked;
        const dayBlink = elements.dayBlink.checked;
        
        const nightFront = elements.nightFront.checked;
        const nightDRL = elements.nightDRL.checked;
        const nightRear = elements.nightRear.checked;
        const nightBlink = elements.nightBlink.checked;
        
        const blinkFrequency = parseInt(elements.blinkFrequency.value) || 500;
        
        debug(`Stan checkboxów dziennych: dayFront=${dayFront}, dayDRL=${dayDRL}, dayRear=${dayRear}, dayBlink=${dayBlink}`);
        debug(`Stan checkboxów nocnych: nightFront=${nightFront}, nightDRL=${nightDRL}, nightRear=${nightRear}, nightBlink=${nightBlink}`);
        debug(`Częstotliwość migania: ${blinkFrequency}`);
        
        // Budowanie konfiguracji
        let dayLightsConfig = 'NONE';
        const dayParts = [];
        if (dayFront) dayParts.push('FRONT');
        if (dayDRL) dayParts.push('DRL');
        if (dayRear) dayParts.push('REAR');
        if (dayParts.length > 0) {
            dayLightsConfig = dayParts.join('+');
        }
        
        let nightLightsConfig = 'NONE';
        const nightParts = [];
        if (nightFront) nightParts.push('FRONT');
        if (nightDRL) nightParts.push('DRL');
        if (nightRear) nightParts.push('REAR');
        if (nightParts.length > 0) {
            nightLightsConfig = nightParts.join('+');
        }
        
        // Tworzenie obiektu konfiguracji
        const lightConfig = {
            dayLights: dayLightsConfig,
            nightLights: nightLightsConfig,
            dayBlink: dayBlink,
            nightBlink: nightBlink,
            blinkFrequency: blinkFrequency
        };
        
        debug('Przygotowane dane:', lightConfig);
        
        // Przygotuj dane do wysłania
        const formData = new URLSearchParams();
        formData.append('data', JSON.stringify(lightConfig));
        
        debug('Dane do wysłania:', formData.toString());
        debug('Wysyłam żądanie POST do /api/lights/config');
        
        // Wykonaj zapytanie z opcją cache: 'no-store'
        const response = await fetch('/api/lights/config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            body: formData.toString(),
            cache: 'no-store'  // Dodane aby zapobiec cachowaniu
        });
        
        debug(`Odpowiedź HTTP: ${response.status}`);
        if (!response.ok) {
            throw new Error(`Błąd HTTP: ${response.status}`);
        }
        
        const result = await response.json();
        debug('Otrzymana odpowiedź:', result);
        
        if (result.status === 'ok') {
            alert('Zapisano ustawienia świateł');
            // Odczekaj 1 sekundę przed odświeżeniem, aby serwer miał czas na aktualizację
            setTimeout(() => {
                loadLightConfig();
            }, 1000);
        } else {
            throw new Error(result.message || 'Nieznany błąd');
        }
    } catch (error) {
        console.error('Błąd podczas zapisywania:', error);
        alert('Błąd podczas zapisywania ustawień: ' + error.message);
    }
}

// Funkcja aktualizacji formularza na podstawie otrzymanego stanu
function updateLightForm(lights) {
    debug('Aktualizacja formularza, otrzymane dane:', lights);
    
    try {
        // Parsowanie konfiguracji świateł dziennych
        const dayLights = lights.dayLights || 'NONE';
        document.getElementById('day-front').checked = dayLights.includes('FRONT');
        document.getElementById('day-drl').checked = dayLights.includes('DRL');
        document.getElementById('day-rear').checked = dayLights.includes('REAR');
        document.getElementById('day-blink').checked = Boolean(lights.dayBlink);
        
        // Parsowanie konfiguracji świateł nocnych
        const nightLights = lights.nightLights || 'NONE';
        document.getElementById('night-front').checked = nightLights.includes('FRONT');
        document.getElementById('night-drl').checked = nightLights.includes('DRL');
        document.getElementById('night-rear').checked = nightLights.includes('REAR');
        document.getElementById('night-blink').checked = Boolean(lights.nightBlink);
        
        // Ustawienie częstotliwości migania
        document.getElementById('blink-frequency').value = lights.blinkFrequency || 500;

        debug('Formularz zaktualizowany pomyślnie');
    } catch (error) {
        console.error('Błąd podczas aktualizacji formularza:', error);
    }
}

// Funkcja aktualizacji statusu świateł
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

// Funkcja inicjalizująca
document.addEventListener('DOMContentLoaded', function() {
    debug('Inicjalizacja aplikacji...');
    
    // Przypisz funkcje do przycisku zapisu
    const saveLightButton = document.getElementById('save-light-config');
    if (saveLightButton) {
        debug('Znaleziono przycisk zapisu świateł, dodaję nasłuchiwacz...');
        saveLightButton.addEventListener('click', function() {
            debug('Kliknięto przycisk zapisu świateł');
            saveLightConfig();
        });
    } else {
        console.error('Nie znaleziono przycisku #save-light-config!');
    }
});

// Reszta kodu może pozostać bez zmian...
