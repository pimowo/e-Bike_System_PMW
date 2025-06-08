// Funkcja do obsługi rozwijania/zwijania sekcji
function setupCollapsibleSections() {
    console.log("Inicjalizacja rozwijanch sekcji...");
    
    // Pobierz wszystkie przyciski zwijania
    const collapseButtons = document.querySelectorAll('.collapse-btn');
    
    collapseButtons.forEach(button => {
        button.addEventListener('click', function() {
            // Znajdź rodzica (.card)
            const card = this.closest('.card');
            
            if (card) {
                // Znajdź zawartość karty (.card-content)
                const content = card.querySelector('.card-content');
                
                if (content) {
                    // Przełącz widoczność zawartości
                    if (content.style.display === 'none' || content.style.display === '') {
                        content.style.display = 'block';
                        console.log("Rozwinięto sekcję");
                    } else {
                        content.style.display = 'none';
                        console.log("Zwinięto sekcję");
                    }
                }
            }
        });
    });

    // Domyślnie zwiń wszystkie sekcje
    const cardContents = document.querySelectorAll('.card-content');
    cardContents.forEach(content => {
        content.style.display = 'none';
    });
    
    console.log("Zwinięto wszystkie sekcje domyślnie");
}

// Inicjalizacja po załadowaniu strony
document.addEventListener('DOMContentLoaded', function() {
    console.log("Inicjalizacja UI...");
    
    // Skonfiguruj rozwijane sekcje
    setupCollapsibleSections();
});

// Funkcja do obsługi przycisków informacyjnych
function setupInfoButtons() {
    console.log("Inicjalizacja przycisków informacyjnych...");
    
    // Pobierz wszystkie przyciski informacyjne
    const infoButtons = document.querySelectorAll('.info-icon');
    
    infoButtons.forEach(button => {
        button.addEventListener('click', function() {
            // Pobierz identyfikator informacji
            const infoId = this.getAttribute('data-info');
            
            if (infoId) {
                // Pobierz tekst informacji z odpowiedniego miejsca
                const infoText = getInfoText(infoId);
                
                // Wyświetl informację w modalu
                showInfoModal(infoId, infoText);
            }
        });
    });
}

// Funkcja do pobierania tekstu informacji na podstawie identyfikatora
function getInfoText(infoId) {
    // Mapowanie identyfikatorów na teksty informacyjne
    const infoTexts = {
        // Informacje o zegarze
        'rtc-info': 'Zegar czasu rzeczywistego (RTC) pokazuje aktualny czas i datę. Możesz ustawić aktualny czas z przeglądarki klikając przycisk "Ustaw aktualny czas".',
        
        // Informacje o światłach
        'light-config-info': 'Konfiguracja świateł pozwala na ustawienie trybów dziennego i nocnego oraz włączenie/wyłączenie migania tylnego światła.',
        'day-lights-info': 'Wybierz, które światła mają być włączone w trybie dziennym.',
        'night-lights-info': 'Wybierz, które światła mają być włączone w trybie nocnym.',
        'day-blink-info': 'Zaznacz, jeśli tylne światło ma migać w trybie dziennym.',
        'night-blink-info': 'Zaznacz, jeśli tylne światło ma migać w trybie nocnym.',
        'blink-frequency-info': 'Ustaw częstotliwość migania tylnego światła w milisekundach (ms).',
        
        // Informacje o wyświetlaczu
        'display-config-info': 'Konfiguracja wyświetlacza pozwala na dostosowanie jasności i czasu automatycznego wyłączenia.',
        'auto-mode-info': 'Tryb AUTO automatycznie przełącza między dzienną i nocną jasnością wyświetlacza.',
        'brightness-info': 'Ustaw jasność wyświetlacza (0-100%).',
        'day-brightness-info': 'Ustaw jasność wyświetlacza w trybie dziennym (0-100%).',
        'night-brightness-info': 'Ustaw jasność wyświetlacza w trybie nocnym (0-100%).',
        'auto-off-time-info': 'Ustaw czas bezczynności, po którym system przejdzie w stan uśpienia (0-60 min).',
        
        // Informacje o sterowniku
        'controller-config-info': 'Konfiguracja sterownika pozwala na dostosowanie parametrów silnika i napędu.',
        'display-type-info': 'Wybierz typ wyświetlacza używanego w systemie.',
        
        // Informacje ogólne
        'general-settings-info': 'Ustawienia ogólne systemu rowerowego.',
        'wheel-size-info': 'Wybierz rozmiar koła rowerowego, aby zapewnić dokładne pomiary prędkości i odległości.',
        'total-odometer-info': 'Całkowity przebieg roweru w kilometrach.',
        
        // Informacje o Bluetooth
        'bluetooth-config-info': 'Konfiguracja połączeń Bluetooth z urządzeniami zewnętrznymi.',
        'bms-info': 'Włącz/wyłącz połączenie z systemem zarządzania baterią (BMS) poprzez Bluetooth.',
        'bms-mac-info': 'Adres MAC urządzenia BMS. Format: XX:XX:XX:XX:XX:XX',
        'tpms-info': 'Włącz/wyłącz system monitorowania ciśnienia w oponach (TPMS) poprzez Bluetooth.',
        'front-tpms-mac-info': 'Adres MAC przedniego czujnika TPMS. Format: XX:XX:XX:XX:XX:XX',
        'rear-tpms-mac-info': 'Adres MAC tylnego czujnika TPMS. Format: XX:XX:XX:XX:XX:XX'
    };
    
    // Zwróć tekst dla danego identyfikatora lub komunikat domyślny
    return infoTexts[infoId] || 'Brak dostępnych informacji dla tego elementu.';
}

// Funkcja do wyświetlania okna modalnego z informacją
function showInfoModal(title, content) {
    console.log("Wyświetlam informację:", title);
    
    // Pobierz elementy modalu
    const modal = document.getElementById('info-modal');
    const modalTitle = document.getElementById('modal-title');
    const modalDescription = document.getElementById('modal-description');
    
    // Ustaw tytuł i zawartość
    modalTitle.textContent = 'Informacja';
    modalDescription.textContent = content;
    
    // Wyświetl modal
    modal.style.display = 'block';
    
    // Zamykanie modalu po kliknięciu w X
    const closeBtn = modal.querySelector('.close-modal');
    if (closeBtn) {
        closeBtn.onclick = function() {
            modal.style.display = 'none';
        };
    }
    
    // Zamykanie modalu po kliknięciu poza nim
    window.onclick = function(event) {
        if (event.target == modal) {
            modal.style.display = 'none';
        }
    };
}

// Rozszerzona inicjalizacja po załadowaniu strony
document.addEventListener('DOMContentLoaded', function() {
    console.log("Inicjalizacja UI...");
    
    // Skonfiguruj rozwijane sekcje
    setupCollapsibleSections();
    
    // Skonfiguruj przyciski informacyjne
    setupInfoButtons();
});
