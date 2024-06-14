# Kompilacja i uruchomienie
Do symulacji działania na sieci rozproszonej wykorzystujemy kontenery, które komunikują się każdy z każdym. Do zbudowania go - wystarczy uruchomić program dockerGenerator.sh. Pierwszym argumentem jest liczba kontenerów do utworzenia, drugim jest ścieżka do programu. Kolejne argumenty to argumenty do programu podanego w ścieżce.

# Opis problemu:

Bibliotekarze mają dosyć czytelników oddających książki z opóźnieniem. 
Postanowili więc zakupić Mechanicznych Ponaglaczy Czytelników (MPC), wyposażonych w perswazatory i naganatacze. 
Niestety, bibliotekarzy jest wielu (B - parametr).
Rozróżnialni MPC są bardzo drogie, więc można było ich kupić tylko M (parametr - B > M ).

Co pewien czas bibliotekarz autonomicznie decydują, że pewna liczba czytelników wymaga przypomnienia o terminie oddania książki. W tym celu bibliotekarz ubiega się o dostęp do MPC.

MPC co pewien czas wymagają serwisu. Kiedy MPC obsłuży K czytelników, powinien zostać oddany do serwisu. Nierozróżnialni serwisanci, o liczbie S (parametr, S << M ), podejmują się naprawy MPC, po czym MPC wraca do służby. Serwisanci są zasobem, nie aktywnymi procesami, bibliotekarze muszą więc konkurować o dostęp do serwisantów.

# Użyta technologia i biblioteki:
C++17

OpenMPI

# Opis algorytmu:

# Struktury i zmienne
### Zmienne
* B - liczba bibliotekarzy
* M - liczba mechanicznych ponaglaczy czytelników (rozróżnialne)
* S - liczba serwisantów (nierozróżnialne)
* K - liczba możliwych użyć MPC przed awarią
### Stuktury
&emsp;  MPCaccessQueue - każdy bibliotekarz posiada synchronizującą się kopię liczników dostępu do każdego MPC

&emsp;  MPCrepairGauge - każdy bibliotekarz posiada synchronizującą się kopię liczników - opisująych za ile operacji MPC ulegnie awarii.

&emsp; svcQueue - kolejka przechowująca żądania dostępu do serwisantów na które proces jeszcze nie odpowiedział

# Wiadomości
```
Wszystkie wiadomości oznaczone są znacznikiem czasowym,
modyfikowanym zgodnie z zasadami skalarnego zegara logicznego Lamporta.

Poniżej używane jest stwierdzenie "priorytet wiadomości" jest on
zegarem Lamporta wraz z rangą (identyfikatorem procesu)
```

* REQ MPC {MPC}- gdzie {MPC}, to numer identyfikacyjny maszyny MPC. Chęć użycia danego MPC
* ACK MPC {MPC} - zgoda na użycie MPC dla danego procesu, występuje po REQ MPC
* FIN {MPC} {USES} - koniec pracy z MPC wraz z informacją o pozostałych możliwych użyciach MPC

* REQ SVC - chęć użycia dowolnego z serwisantów
* ACK SVC - zgoda na użycie serwisanta

# Stany
    * IDLE - brak ubiegania się o dostęp lub przetwarzania jakichkolwiek danych
    ------ część bibliotekarska ------
    * WAIT MPC - oczekiwanie na dostęp/odpowiedzi innych bibliotekarzy o dostęp do mpc
    * IN SECTION MPC - używanie mpc przez bibliotekarza
    -------- część serwisowa  --------
    * WAIT SVC - oczekiwanie na dostęp/odpowiedzi innych bibliotekarzy o dostęp do serwisu
    * IN SECTION SCV - używanie serwisu przez bibliotekarza
```
                    ┌──────────┐                                                                      
     Złożenie pracy │          │  oczekiwanie na                                                      
    ┌───────────────┤ WAIT MPC ├─────────────────┐                                                    
    │  przez lib    │          │  dostęp do MPC  │                                                    
    │               └──────────┘                 ▼                         Oczekiwanie            
┌───┴──┐                                ┌────────────────┐      ┌──────────┐        ┌────────────────┐
│      │                                │                │AWARIA│          │   na   │                │
│ IDLE │                                │ IN_SECTION MPC ├─────►│ WAIT SVC ├───────►│ IN_SECTION SVC │
│      │◄───────────────────────────────┤                │      │          │ serwis │                │
└──────┘     Prawidłowe zakończenie     └────────────────┘      └──────────┘        └───────┬────────┘
                     pracy                       ▲                                          │         
                                                 └──────────────────────────────────────────┘         
```                                               
# Szkic algorytmu 

Algorytm można w klarowny sposób podzielić na dwa podalgorytmy - część dotycząca MPC i część dotycząca serwisu, gdzie bibliotekarz będzie się kolejno ubiegał o rozróżnialny zasób MPC lub o nierozróżnialny zasób serwisanta.

-----część MPC (podalgorytm 1)----
1. Bibliotekarz zgłasza chęć innym bibliotekarzom (procesom) dostępu do MPC - wybiera pierwszego z najkrótszą kolejką - wysyłając REQ MPC z określonym priorytetem
2. Inni bibliotekarze odsyłają zgodę na dostęp (ACK) pod warunkiem, że sami nie ubiegają się o sekcję krytyczną bądź ich żądanie ma niższy priorytet oraz umieszczają informację o kolejnym bibliotekarzu ubiegającym się o danego MPC
Zawsze tylko jeden proces może otrzymać wszystkie pozostałe ACK ze wzgęlu na priorytety zawsze znajdzie się proces dominujący w przypadku wielu oczekujących
&emsp; Uwaga: FIN MPC jest także traktowany jako ACK MPC - zawiera on dodatkowo informację o pozostałych użyciach MPC, po których będzie potrzebny serwis w przypadku braku takiej wiadomości (na początku) MPC ma wartość domyślną
3. Bibliotekarz, który otrzymał wszystkie ACK (bądź FIN), uruchamia MPC.                
&emsp; Uwaga: Jeżeli MPC potrzebuje serwisu - bibliotekarz przechodzi do podalgorytmu 2.
4. Bibliotekarz kończy pracę z MPC i wysyła FIN MPC do wszystkich pozostałych procesów, w którym zawiera stan MPC po pracy.
Inne procesy zmieniają stan danego MPC po otrzymaniu tej wiadomości
5. Jako iż kolejny proces dostał ostatnie potwierdzenie (FIN MPC) - algorytm ten może być kontynuowany dla kolejnego bibliotekarza.

-----część serwisowa (podalgorytm 2)----
1. Jeżeli w trakcie pracy (punkt 3 podalgorytmu 1) MPC będzie wymagał serwisu (stan możliwości obsłużenia czytelników będzie wynosił 0 - uruchamiany jest ten algrytm
2. Bibliotekarz zmienia stan na WAIT SVC i wysyła sygnał do wszystkich innych bibliotekarzy z prośbą o dostęp do serwisanta.
Inni serwisanci odsyłają ACK SVC jeżeli ich priorytet jest niższy bądź wstawiają żądanie do svcQueue w innym przypadku
4. Jeżeli otrzyma co najmniej n odpowiedzi zgody (ACK SVC) gdzie n=B-S przechodzi do użycia serwisanta.
5. Po zakończeniu użycia serwisanta proces odsyła ACK SVC dla wszystkich procesów dla których otrzymał żądanie ale nie odesłał natychmiast odpowiedzi

# Złożoność komunikacyjna i czasowa:
* Komunikacyjna
  
  O(n)

* Czasowa
  
  O(n)

# Opis szczegółowy algorytmu:

Wiadomości przekreślone oznaczają, że sytuacja jest ignorowana lub niemożliwa

1. Reakcje na wiadomości
* Stan początkowy **IDLE**:
    -   **REQ MPC {MPC}**
        ```
        ** Zwiększa licznik w MPCaccessQueue dla danego {MPC} **
        ** Odsyła ACK MPC {MPC} **
        ```
    -   **REQ SVC {SVC}**
        ```
        ** Odsyła ACK SVC **
        ```
    -   **FIN {MPC} {USES}**
        ```
        ** Zmniejsza licznik w MPCaccessQueue dla danego {MPC} **
        ** Zmienia MPCrepairGauge danego {MPC} na {USES}**
        ```
    - ~~**ACK MPC {MPC}**~~
    - ~~**ACK SVC**~~

* Oczekiwanie na dostęp do MPC **WAIT MPC**
    -   **REQ MPC {MPC}**
        ```
        ** Zwiększa licznik w MPCaccessQueue dla danego {MPC} **
        Jeżeli nie jest to {MPC} o który proces się ubiega bądź priorytet przychodzącego żądania jest wyższy:
            ** Odsyła ACK MPC {MPC} **
        ```
    -   **REQ SVC**
        ```
        ** Odsyła ACK SVC **
        ```
    -   **ACK MPC {MPC}**
        ```
        ** Zmniejsza licznik acksToRcv o 1 **
        ```
    -  **FIN {MPC} {USES}**
        ```
        ** Zmniejsza licznik w MPCaccessQueue dla danego {MPC} **
        ** Zmienia MPCrepairGauge danego {MPC} na {USES}**
        Jeżeli {MPC} to ten o którego zabiega bibliotekarz:
            **Zmniejsza licznik acksToRcv o 1**
        ```
    - ~~**ACK SVC**~~
* Przebywanie w sekcji krytycznej MPC **IN SECTION MPC**
    -   **REQ MPC {MPC}**
        ```
         ** Zwiększa licznik w MPCaccessQueue dla danego {MPC} **
        Jeżeli {MPC} jest inny niż ten do którego dostęp posiada proces
            ** Odsyła ACK MPC {MPC} **
        ```
    -   **REQ SVC**
        ```
        ** Odsyła ACK SVC **
        ```
    -   **FIN {MPC} {USES}**
        ```
        ** Zmniejsza licznik w MPCaccessQueue dla danego {MPC} **
        ** Zmienia MPCrepairGauge danego {MPC} na {USES}**
        ```
    - ~~**ACK MPC {MPC}**~~
    - ~~**ACK SVC**~~

* Oczekiwanie na dostęp do serwisanta **WAIT SVC**
    -   **REQ MPC {MPC}**
        ```
         ** Zwiększa licznik w MPCaccessQueue dla danego {MPC} **
        Jeżeli {MPC} jest inny niż ten do którego dostęp posiada proces
            ** Odsyła ACK MPC {MPC} **
        ```
    -   **REQ SVC** 
        ```
        Jeżeli priorytet przychodzącego żądania jest wyższy:
            ** Odsyła ACK SVC **
        W przeciwnym wypadku:
            ** Dodaje przychodzące żądanie do svcQueue w miejscu określonym priorytetem **
        ```
    -   **FIN {MPC} {USES}**
        ```
        ** Zmniejsza licznik w MPCaccessQueue dla danego {MPC} **
        ** Zmienia MPCrepairGauge danego {MPC} na {USES}**
        ```
    - ~~**ACK MPC {MPC}**~~

* Przebywanie w sekcji krytycznej serwisanta **IN SECTION SVC**
* Reakcje na wiadomości
    -   **REQ {MPC}**
        ```
         ** Zwiększa licznik w MPCaccessQueue dla danego {MPC} **
        Jeżeli {MPC} jest inny niż ten do którego dostęp posiada proces:
            ** Odsyła ACK MPC {MPC} **
        ```
    -   **REQ {SVC}**
        ```
        ** Dodaje przychodzące żądanie do svcQueue w miejscu określonym priorytetem **
        ```
    -   **FIN {MPC}**
        ```
        ** Zmniejsza licznik w MPCaccessQueue dla danego {MPC} **
        ** Zmienia MPCrepairGauge danego {MPC} na {USES}**
        ```
    - ~~**ACK MPC {MPC}**~~
    - ~~**ACK SVC**~~
 
2. Zmiany stanów      
    -  **IDLE -> WAIT MPC** 
        ```
        1- Bibliotekarz wybiera MPC z najmniejszą ilością próśb o dostęp na podstawie mpcAccessQueue    
        2- Bibliotekarz ustawia swoją zmienną lokalną acksToRcv = B - 1     
        3- Bibliotekarz rozysła wszystkim bibliotekarzom REQ MPC {MPC}
        ** Następuje zmiana stanu ** 
        ```
    - **WAIT MPC -> IN SECTION MPC**
        ```
        1- Każdorazowe otrzymanie wiadomości [ACK MPC {MPC}] lub [FIN {MPC} {USES}] od innego bibliotekarza zmniejsza zmienną lokalną acksToRcv o 1
        2- Jeżeli acksToRcv == 0:
            ** Następuje zmiana stanu **
        ```
    - **IN_SECTION MPC -> IDLE**
        ```
        1- Z ponagleniem każdego czytelnika, obniża licznik MPCrepairGauge dla konkretnego {MPC} o 1
        2- Bibliotekarz wychodzi z sekcji krytycznej po ponagleniu wszystkich swoich czytelników przez MPC
        3- Następuje wysłanie FIN {MPC} {USES} dla każdego bibliotekarza
        ** Następuje zmiana stanu **
        ```
     - **IN SECTION MPC -> WAIT SVC**
        ```
        1- Jeżeli licznik MPCrepairGauge dla używanego {MPC} zmienia się na 0 poszukiwany jest serwisant
        2- Do wszystkich bibliotekarzy rozsyłana jest wiadomość REQ SVC - chęć dostępu do serwisanta **
        3- Bibliotekarz ustawia swoją zmienną lokalną acksToRcv = B - S
        ** Następuje zmiana stanu **
        ```
    - **WAIT SVC -> IN SECTION SVC**
        ```
        1- Każdorazowe otrzymanie wiadomości [ACK SVC] od innego bibliotekarza zmniejsza zmienną lokalną acksToRcv o 1
        2- Jeżeli acksToRcv == 0:
            ** Następuje zmiana stanu **
        ```
   - **IN SECTION SVC -> IN SECTION MPC**
        ```
        1- Po wykonaniu pracy serwisanta MPCrepairGauge dla danego {MPC} wraca do poziomu K
        2- Następuje odesłanie ACK do wszystkich procesów z svcQueue i wyczyszczenie jej 
        ** Następuje powrót do IN SECTION MPC i kontynuacja ponaglania czytelników **
        ```
