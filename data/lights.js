// Konfiguracja świateł - funkcje pomocnicze

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

// Funkcja do wyświetlania powiadomień
function showNotification(message, type = 'info') {
    // Utwórz element powiadomienia
    const notification = document.createElement('div');
    notification.className = `notification ${type}`;
    notification.textContent = message;
    
    // Dodaj element do strony
    document.body.appendChild(notification);
    
    // Ustaw timeout do ukrycia i usunięcia powiadomienia
    setTimeout(() => {
        notification.classList.add('hide');
        setTimeout(() => notification.remove(), 500);
    }, 3000);
}

// Funkcja do zapisywania konfiguracji świateł
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
        
        // Pokaż dialog potwierdzenia
        if (!confirm("Czy na pewno chcesz zapisać ustawienia świateł?")) {
            return; // Anulowano zapis
        }
        
        // Dodaj wskaźnik ładowania
        const saveBtn = document.getElementById('save-light-config');
        const originalText = saveBtn.textContent;
        saveBtn.textContent = 'Zapisywanie...';
        saveBtn.disabled = true;

        // Wyślij dane jako czysty JSON
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
            // Pokaż powiadomienie
            showNotification('Zapisano ustawienia świateł!', 'success');
            
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
        showNotification('Błąd: ' + error.message, 'error');
    }
}

// Inicjalizacja po załadowaniu dokumentu
document.addEventListener('DOMContentLoaded', function() {
    // Ładowanie konfiguracji świateł
    loadLightConfig();
    
    // Przypisanie funkcji do przycisku zapisu
    const saveBtn = document.getElementById('save-light-config');
    if (saveBtn) {
        saveBtn.addEventListener('click', saveLightConfig);
    }
});
