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
                
                // Format daty YYYY-MM-DD
                const year = time.year;
                const month = String(time.month).padStart(2, '0');
                const day = String(time.day).padStart(2, '0');
                const dateString = `${year}-${month}-${day}`;
                
                // Aktualizuj pola na stronie
                document.getElementById('rtc-time').value = timeString;
                document.getElementById('rtc-date').value = dateString;
                
                console.log('Zaktualizowano czas na:', timeString);
                console.log('Zaktualizowano datę na:', dateString);
            } else {
                console.error('Nieprawidłowe dane zwrócone z serwera:', data);
            }
        })
        .catch(error => {
            console.error('Błąd podczas pobierania czasu:', error);
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
            alert('Czas został zaktualizowany!');
            // Odśwież wyświetlany czas
            updateClock();
        } else {
            alert('Błąd podczas aktualizacji czasu: ' + (result.error || 'Nieznany błąd'));
        }
    })
    .catch(error => {
        console.error('Błąd:', error);
        alert('Wystąpił błąd podczas aktualizacji czasu.');
    });
}

// Inicjalizacja po załadowaniu strony
document.addEventListener('DOMContentLoaded', function() {
    console.log("Inicjalizacja zegara...");
    
    // Aktualizuj zegar natychmiast
    updateClock();
    
    // Ustawienie przycisku do aktualizacji czasu
    const saveButton = document.querySelector('.btn-save');
    if (saveButton) {
        saveButton.addEventListener('click', saveRTCConfig);
    }
    
    // Aktualizuj zegar co minutę
    setInterval(updateClock, 60000);
});
