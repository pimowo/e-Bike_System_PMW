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
            const dayLights = document.getElementById('day-lights');
            const nightLights = document.getElementById('night-lights');
            const dayBlink = document.getElementById('day-blink');
            const nightBlink = document.getElementById('night-blink');
            const blinkFrequency = document.getElementById('blink-frequency');
            
            if (dayLights) dayLights.value = data.lights.dayLights || 'DRL+REAR';
            if (nightLights) nightLights.value = data.lights.nightLights || 'FRONT+REAR';
            if (dayBlink) dayBlink.checked = data.lights.dayBlink !== undefined ? data.lights.dayBlink : true;
            if (nightBlink) nightBlink.checked = data.lights.nightBlink !== undefined ? data.lights.nightBlink : false;
            if (blinkFrequency) blinkFrequency.value = data.lights.blinkFrequency || 500;
            
            console.log('Zaktualizowano formularz z konfiguracji z serwera');
        } else {
            console.warn('Odpowiedź serwera nie zawiera danych o światłach, używam domyślnych wartości');
            // Ustawienia domyślne
            const dayLights = document.getElementById('day-lights');
            const nightLights = document.getElementById('night-lights');
            const dayBlink = document.getElementById('day-blink');
            const nightBlink = document.getElementById('night-blink');
            const blinkFrequency = document.getElementById('blink-frequency');
            
            if (dayLights) dayLights.value = 'DRL+REAR';
            if (nightLights) nightLights.value = 'FRONT+REAR';
            if (dayBlink) dayBlink.checked = true;
            if (nightBlink) nightBlink.checked = false;
            if (blinkFrequency) blinkFrequency.value = 500;
        }
    } catch (error) {
        console.error('Błąd podczas ładowania konfiguracji świateł:', error);
        // Ustawienia domyślne w przypadku błędu
    }
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
            //alert('Zapisano ustawienia świateł!');
			showNotification('Zapisano ustawienia świateł!', 'success');
            
            // Sprawdź, czy zapisane wartości odpowiadają wysłanym
            if (result.lights) {
                console.log('Otrzymane ustawienia z serwera:', result.lights);
                
                // Porównaj wartości
                if (result.lights.dayLights !== dayLights) {
                    console.warn(`Ostrzeżenie: Zapisana konfiguracja dziennych świateł (${result.lights.dayLights}) `+
                                 `różni się od wysłanej (${dayLights})`);
                }
                
                if (result.lights.nightLights !== nightLights) {
                    console.warn(`Ostrzeżenie: Zapisana konfiguracja nocnych świateł (${result.lights.nightLights}) `+
                                 `różni się od wysłanej (${nightLights})`);
                }
                
                // Zaktualizuj formularz z otrzymanych danych
                document.getElementById('day-lights').value = result.lights.dayLights;
                document.getElementById('night-lights').value = result.lights.nightLights;
                document.getElementById('day-blink').checked = result.lights.dayBlink;
                document.getElementById('night-blink').checked = result.lights.nightBlink;
                document.getElementById('blink-frequency').value = result.lights.blinkFrequency;
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
        //alert('Błąd: ' + error.message);
		showNotification('Błąd: ' + error.message, 'error');
    }
}
