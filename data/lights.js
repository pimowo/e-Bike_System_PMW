// Funkcja do ładowania konfiguracji świateł
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
            const lightControlMode = document.getElementById('light-control-mode');
            const dayLights = document.getElementById('day-lights');
            const nightLights = document.getElementById('night-lights');
            const dayBlink = document.getElementById('day-blink');
            const nightBlink = document.getElementById('night-blink');
            const blinkFrequency = document.getElementById('blink-frequency');
            
            // Ustawienie trybu sterowania światłami
            if (lightControlMode) {
                lightControlMode.value = data.lights.controlMode === 1 ? 'controller' : 'smart';
                // Wywołanie funkcji aktualizującej widoczność sekcji konfiguracyjnych
                updateLightConfigVisibility();
            }
            
            if (dayLights) dayLights.value = data.lights.dayLights || 'DRL+REAR';
            if (nightLights) nightLights.value = data.lights.nightLights || 'FRONT+REAR';
            if (dayBlink) dayBlink.checked = data.lights.dayBlink !== undefined ? data.lights.dayBlink : true;
            if (nightBlink) nightBlink.checked = data.lights.nightBlink !== undefined ? data.lights.nightBlink : false;
            if (blinkFrequency) blinkFrequency.value = data.lights.blinkFrequency || 500;
            
            console.log('Zaktualizowano formularz z konfiguracji z serwera');
        } else {
            console.warn('Odpowiedź serwera nie zawiera danych o światłach, używam domyślnych wartości');
            // Ustawienia domyślne
            const lightControlMode = document.getElementById('light-control-mode');
            const dayLights = document.getElementById('day-lights');
            const nightLights = document.getElementById('night-lights');
            const dayBlink = document.getElementById('day-blink');
            const nightBlink = document.getElementById('night-blink');
            const blinkFrequency = document.getElementById('blink-frequency');
            
            if (lightControlMode) lightControlMode.value = 'smart';
            if (dayLights) dayLights.value = 'DRL+REAR';
            if (nightLights) nightLights.value = 'FRONT+REAR';
            if (dayBlink) dayBlink.checked = true;
            if (nightBlink) nightBlink.checked = false;
            if (blinkFrequency) blinkFrequency.value = 500;
            
            // Zaktualizuj widoczność sekcji konfiguracyjnych
            updateLightConfigVisibility();
        }
    } catch (error) {
        console.error('Błąd podczas ładowania konfiguracji świateł:', error);
        // Ustawienia domyślne w przypadku błędu
    }
}

// Funkcja do aktualizacji widoczności sekcji konfiguracji światła
function updateLightConfigVisibility() {
    const lightControlMode = document.getElementById('light-control-mode');
    const smartConfigSection = document.getElementById('smart-config-section');
    
    if (lightControlMode && smartConfigSection) {
        if (lightControlMode.value === 'smart') {
            smartConfigSection.style.display = 'block';
        } else {
            smartConfigSection.style.display = 'none';
        }
    }
}

// Funkcja do inicjalizacji obsługi formularza konfiguracji świateł
function initLightConfigForm() {
    // Dodaj nasłuchiwanie na zmianę trybu sterowania światłami
    const lightControlMode = document.getElementById('light-control-mode');
    if (lightControlMode) {
        lightControlMode.addEventListener('change', updateLightConfigVisibility);
    }
    
    // Ustaw początkowy stan widoczności
    updateLightConfigVisibility();
    
    // Dodaj nasłuchiwanie na przycisk zapisu
    const saveButton = document.getElementById('save-light-config');
    if (saveButton) {
        saveButton.addEventListener('click', saveLightConfig);
    }
}

// Funkcja do zapisywania konfiguracji świateł
async function saveLightConfig() {
    console.log('Rozpoczynam zapisywanie konfiguracji świateł...');
    
    try {
        // Pobierz wartość trybu sterowania
        const lightControlMode = document.getElementById('light-control-mode').value;
        // Konwersja na wartość liczbową dla API
        const controlMode = lightControlMode === 'controller' ? 1 : 0;
        
        const dayLights = document.getElementById('day-lights').value;
        const nightLights = document.getElementById('night-lights').value;
        const dayBlink = document.getElementById('day-blink').checked;
        const nightBlink = document.getElementById('night-blink').checked;
        const blinkFrequency = parseInt(document.getElementById('blink-frequency').value) || 500;
        
        console.log(`Konfiguracja do zapisu: tryb=${lightControlMode}, dzienne=${dayLights}, nocne=${nightLights}, `+
                    `dzienBlink=${dayBlink}, nocBlink=${nightBlink}, częst.=${blinkFrequency}`);
        
        // Sprawdź, czy tylne światło jest wyłączone i zaloguj to
        if (dayLights === 'NONE' || dayLights === 'DRL' || dayLights === 'FRONT' || dayLights === 'FRONT+DRL') {
            console.log('Tylne światło jest WYŁĄCZONE w konfiguracji dziennej');
        } else {
            console.log('Tylne światło jest WŁĄCZONE w konfiguracji dziennej');
        }
        
        if (nightLights === 'NONE' || nightLights === 'DRL' || nightLights === 'FRONT' || nightLights === 'FRONT+DRL') {
            console.log('Tylne światło jest WYŁĄCZONE w konfiguracji nocnej');
        } else {
            console.log('Tylne światło jest WŁĄCZONE w konfiguracji nocnej');
        }
        
        const lightConfig = {
            controlMode: controlMode, // Dodano nowe pole
            dayLights: dayLights,
            nightLights: nightLights,
            dayBlink: dayBlink,
            nightBlink: nightBlink,
            blinkFrequency: blinkFrequency
        };
        
        // Pokaż wskaźnik ładowania
        const saveBtn = document.getElementById('save-light-config');
        const originalText = saveBtn.textContent;
        saveBtn.textContent = 'Zapisywanie...';
        saveBtn.disabled = true;

        // Wyślij dane jako JSON
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
            showNotification('Zapisano ustawienia świateł!', 'success');
            
            // Sprawdź, czy zapisane wartości odpowiadają wysłanym
            if (result.lights) {
                console.log('Otrzymane ustawienia z serwera:', result.lights);
                
                // Porównaj wartości
                if (result.lights.controlMode !== controlMode) {
                    console.warn(`Ostrzeżenie: Zapisany tryb sterowania światłami (${result.lights.controlMode}) `+
                                 `różni się od wysłanego (${controlMode})`);
                }
                
                if (result.lights.dayLights !== dayLights) {
                    console.warn(`Ostrzeżenie: Zapisana konfiguracja dziennych świateł (${result.lights.dayLights}) `+
                                 `różni się od wysłanej (${dayLights})`);
                }
                
                if (result.lights.nightLights !== nightLights) {
                    console.warn(`Ostrzeżenie: Zapisana konfiguracja nocnych świateł (${result.lights.nightLights}) `+
                                 `różni się od wysłanej (${nightLights})`);
                }
                
                // Zaktualizuj formularz z otrzymanych danych
                document.getElementById('light-control-mode').value = result.lights.controlMode === 1 ? 'controller' : 'smart';
                document.getElementById('day-lights').value = result.lights.dayLights;
                document.getElementById('night-lights').value = result.lights.nightLights;
                document.getElementById('day-blink').checked = result.lights.dayBlink;
                document.getElementById('night-blink').checked = result.lights.nightBlink;
                document.getElementById('blink-frequency').value = result.lights.blinkFrequency;
                
                // Zaktualizuj widoczność sekcji po zmianie trybu
                updateLightConfigVisibility();
                
                console.log('Zaktualizowano formularz z odpowiedzi serwera');
                
                // Sprawdź ponownie status tylnego światła po aktualizacji
                if (result.lights.dayLights.includes('REAR')) {
                    console.log('Po zapisie: Tylne światło jest WŁĄCZONE w konfiguracji dziennej');
                } else {
                    console.log('Po zapisie: Tylne światło jest WYŁĄCZONE w konfiguracji dziennej');
                }
                
                if (result.lights.nightLights.includes('REAR')) {
                    console.log('Po zapisie: Tylne światło jest WŁĄCZONE w konfiguracji nocnej');
                } else {
                    console.log('Po zapisie: Tylne światło jest WYŁĄCZONE w konfiguracji nocnej');
                }
            }
        } else {
            throw new Error(result.message || 'Nieznany błąd podczas zapisu');
        }
    } catch (error) {
        console.error('Błąd podczas zapisywania:', error);
        showNotification('Błąd: ' + error.message, 'error');
    }
}

// Dodaj do tooltips (jeśli masz system tooltipów w głównym skrypcie)
if (typeof tooltips !== 'undefined') {
    // Dodanie nowego tooltipa dla trybu sterowania światłami
    tooltips["light-control-info"] = `
        <h3>Tryb sterowania światłami</h3>
        <p><strong>Smart</strong> - Zaawansowane sterowanie świateł przez system e-Bike. Dostępne są opcje konfiguracji świateł dziennych, nocnych oraz opcji migania.</p>
        <p><strong>Sterownik</strong> - Podstawowe sterowanie świateł przez kontroler KT. System e-Bike będzie tylko włączał/wyłączał światła, a kontroler KT będzie decydował o szczegółach ich działania.</p>
    `;
}

// Inicjalizacja przy załadowaniu dokumentu
document.addEventListener('DOMContentLoaded', function() {
    // Załaduj konfigurację świateł
    loadLightConfig();
    // Inicjalizuj obsługę formularza
    initLightConfigForm();
});
