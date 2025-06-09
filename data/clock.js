// Funkcja do aktualizacji zegara na stronie www
function updateClock() {
    console.log("Aktualizacja zegara...");
    
    fetch('/api/time')
        .then(response => {
            if (!response.ok) {
                throw new Error('Błąd sieci: ' + response.status);
            }
            return response.json();
        })
        .then(data => {
            if (data && data.time) {
                const time = data.time;
                
                // Format czasu HH:MM:SS
                const hours = String(time.hours).padStart(2, '0');
                const minutes = String(time.minutes).padStart(2, '0');
                const seconds = String(time.seconds).padStart(2, '0');
                const timeString = `${hours}:${minutes}:${seconds}`;
                
                // Format daty DD/MM/YYYY (zmieniono kolejność wyświetlania)
                const year = time.year;
                const month = String(time.month).padStart(2, '0');
                const day = String(time.day).padStart(2, '0');
                const dateString = `${day}/${month}/${year}`; // Zmieniono format daty
                
                // Aktualizuj pola na stronie
                updateElementValue('rtc-time', timeString);
                updateElementValue('rtc-date', dateString);
                
                console.log('Zaktualizowano czas na:', timeString);
                console.log('Zaktualizowano datę na:', dateString);
            } else {
                console.error('Nieprawidłowe dane zwrócone z serwera:', data);
            }
        })
        .catch(error => {
            handleError(error, 'Błąd podczas pobierania czasu');
        });
}

// Funkcja do ustawiania czasu
function saveRTCConfig() {
    console.log("Zapisywanie aktualnego czasu...");
    
    const now = new Date();
    
    const data = {
        year: now.getFullYear(),
        month: now.getMonth() + 1, // Miesiące są liczone od 0 w JavaScript
        day: now.getDate(),
        hour: now.getHours(),
        minute: now.getMinutes(),
        second: now.getSeconds()
    };
    
    fetch('/api/time', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
    })
    .then(response => response.json())
    .then(result => {
        if (result.status === 'ok') {
            showNotification('Czas został zaktualizowany!', 'success');
            // Odśwież wyświetlany czas
            updateClock();
        } else {
            throw new Error(result.error || 'Nieznany błąd');
        }
    })
    .catch(error => {
        handleError(error, 'Błąd podczas aktualizacji czasu');
    });
}

// Funkcja inicjalizacji zegara
function initializeClock() {
    console.log('Inicjalizacja zegara');
    
    // Pierwsze pobranie
    updateClock();
    
    // Ustawienie przycisku do aktualizacji czasu
    const saveButton = document.querySelector('.rtc-row .btn-save');
    if (saveButton) {
        saveButton.addEventListener('click', saveRTCConfig);
    }
    
    // Aktualizacja co 1 sekundę (zamiast co minutę) dla bieżącego czasu
    return setInterval(updateClock, 1000);  // Zmieniono z 60000 na 1000
}
