Aby przeanalizować złożoność komunikacyjną i czasową podanych podalgorytmów, musimy przyjrzeć się poszczególnym krokom wykonywanym przez procesy oraz wymianie wiadomości pomiędzy nimi. Poniżej omówienie złożoności dla obu podalgorytmów:

### Podalgorytm 1 (MPC)

1. **Zgłoszenie chęci dostępu do MPC (wysyłanie REQ MPC):**
   - Każdy z bibliotekarzy wysyła wiadomość REQ MPC do wszystkich innych bibliotekarzy.
   - Złożoność komunikacyjna: \(O(B)\), gdzie \(B\) to liczba bibliotekarzów.
   - Złożoność czasowa: Otrzymanie wszystkich odpowiedzi (ACK/FIN MPC) od pozostałych \(B-1\) bibliotekarzy zajmuje \(O(B)\) czasu.

2. **Odesłanie zgody na dostęp (ACK):**
   - Każdy bibliotekarz otrzymuje REQ MPC i musi zdecydować, czy wysłać ACK.
   - Złożoność komunikacyjna: \(O(B)\) (każdy bibliotekarz może wysłać ACK do wszystkich innych).
   - Złożoność czasowa: \(O(1)\) dla wysłania pojedynczego ACK.

3. **Uruchomienie MPC przez proces, który otrzymał wszystkie ACK:**
   - Uruchomienie jest lokalne dla procesu, który otrzymał wszystkie ACK.
   - Złożoność komunikacyjna: \(O(1)\) (brak dodatkowej komunikacji).
   - Złożoność czasowa: \(O(1)\) (zależne od samego działania MPC).

4. **Wysłanie FIN MPC do wszystkich procesów:**
   - Bibliotekarz, który kończy pracę z MPC, wysyła FIN MPC do wszystkich innych bibliotekarzy.
   - Złożoność komunikacyjna: \(O(B)\).
   - Złożoność czasowa: \(O(1)\) dla wysłania pojedynczego FIN MPC.

### Podalgorytm 2 (Serwis)

1. **Zmiana stanu na WAIT SVC i wysłanie sygnału do innych bibliotekarzy:**
   - Bibliotekarz wysyła żądanie serwisu do wszystkich innych bibliotekarzy.
   - Złożoność komunikacyjna: \(O(B)\).
   - Złożoność czasowa: \(O(B)\) (oczekiwanie na odpowiedzi).

2. **Odesłanie ACK SVC lub wstawienie żądania do svcQueue:**
   - Każdy proces musi odpowiedzieć na żądanie ACK SVC lub dodać żądanie do kolejki.
   - Złożoność komunikacyjna: \(O(B)\) (potencjalnie każdemu bibliotekarzowi).
   - Złożoność czasowa: \(O(1)\) dla pojedynczego wysłania ACK SVC lub dodania do kolejki.

3. **Przejście do użycia serwisanta po otrzymaniu n odpowiedzi ACK SVC:**
   - Proces przechodzi do użycia serwisanta po otrzymaniu wystarczającej liczby zgód.
   - Złożoność komunikacyjna: \(O(1)\) (nie wymaga dodatkowej komunikacji).
   - Złożoność czasowa: \(O(1)\) (zależne od działania serwisanta).

4. **Odesłanie ACK SVC po zakończeniu użycia serwisanta:**
   - Proces wysyła ACK SVC do wszystkich bibliotekarzy, które oczekiwały na odpowiedź.
   - Złożoność komunikacyjna: \(O(B)\).
   - Złożoność czasowa: \(O(1)\) dla wysłania pojedynczego ACK SVC.

### Podsumowanie

- **Złożoność komunikacyjna:**
  - Podalgorytm 1: \(O(B)\) dla każdego etapu komunikacji.
  - Podalgorytm 2: \(O(B)\) dla każdego etapu komunikacji.

- **Złożoność czasowa:**
  - Podalgorytm 1: \(O(B)\) dla etapu otrzymywania wszystkich odpowiedzi (ACK/FIN MPC).
  - Podalgorytm 2: \(O(B)\) dla etapu otrzymywania odpowiedzi (ACK SVC).

Zarówno złożoność komunikacyjna, jak i czasowa w obu podalgorytmach jest liniowa względem liczby procesów \(B\).