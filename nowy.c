#include <Arduino.h>

const int   packet_size         = 22;                   // wielkość pakietu
const char  packet_header[]     = "snp";                // nagłówek pakietu
const int   packet_header_size = sizeof(packet_header); // wielkość nagłówka pakietu
uint8_t     packet[packet_size];                        // tablica bajtowado przechowywania pakietu
uint8_t     temp1_offset        = 4;                    // offset w pakiecie dla 1 temperatury
uint8_t     temp2_offset        = temp1_offset + 4;     // offset w pakiecie dla 2 temperatury
uint8_t     temp3_offset        = temp2_offset + 4;     // offset w pakiecie dla 3 temperatury
uint8_t     time_offset         = temp3_offset + 4;     // offset w pakiecie dla czasu od startu czujnika
int         received_byte_index   = 0;                  // index odebranego znaku pakietu
int         incomplete_packets  = 0;                    // "urwane" pakiety
int         broken_packets      = 0;                    // uszkodzone pakiety
int         received_packets    = 0;                    // ilość odebranych poprawnych pakietów
float       temp1, temp2, temp3, received_time;         // zmienne dla odebranych liczb zmiennoprzecinkowych
String      pc_input;                                   // zmienna dla danych wejściowych z PC

union byte_to_float_t {                                 // jednak unia :) - pozwala ona na łatwiejsze manipulowanie danymi w tym samym obszarze pamięci
    uint8_t bytes[4];                                   // tablica bajtowa dla kolejnych elementów liczby
    float value;                                        // wartość wyjściowa typu float
} byte_to_float;

/**
 * \brief             Funkcja służąca do konwersji 4 kolejnych wyrazów tablicy pakietu od określonej pozycji na liczbę zmiennoprzecinkową (znów możnaby się pokusić o wskaźniki, a nie globale)
 * \param[offset]     Pozycja startu liczby do przeliczenia
 * \return            Liczba zmiennoprzecinkowa złożona z 4 kolejnych bajtów z tablicy od określonej pozycji
*/
float parse_float(uint8_t offset) {
  for (int i = 0; i < 4; i++) {
    byte_to_float.bytes[i] = packet[offset + i];
  }
  return byte_to_float.value;
}

/**
 * \brief   Funkcja licząca sumę kontrolną (to mały projekt, więc w uproszczeniu, bo liczy ze zmiennych globalnych, a kusi użyć parametrów)
 * \return  Obliczona suma z 20 bajtów odebranego pakietu
*/
uint16_t calc_checksum() {
  uint16_t  checksum = 0;              // suma kontrolna
  for (int i = 0; i < 20 - 1; i++) {   // pętla służąca do "przejścia" po wszystkich elementach tablicy pakietu
    checksum += packet[i];             // sumowanie kolejnych wyrazów tablicy
  }
  return checksum;                     // zwrócenie obliczonej wartości
}

/**
 * \brief               Funkcja licząca procenty
 * \param[numerator]    Licznik
 * \param[denominator]  Mianownik
 * \return              Obliczony procent
*/
float calc_percentage(int numerator, int denominator) {
  if (denominator == 0) {     // Sprawdź czy nie dzielimy przez 0
    return 0;
  }
  return ((float)numerator / (float)denominator) * 100; // zwrócenie obliczonego procentu
}

/**
 * \brief       Funkcja służąca do parsowania zmiennej typu uint16_t z 2 zmiennych typu uint8_t
 * \param[msb]  Najbardziej znaczący bajt
 * \param[lsb]  Mniej znaczący bajt
 * \return      Liczba typu uint16_t złożona z 2 podanych liczb
*/
uint16_t create_uint16(uint8_t msb, uint8_t lsb) {
  return (static_cast<uint16_t>(msb) << 8) | lsb; // rzutowanie 2 liczb uint8_t z przesunięciem bitowym
}

/**
 * \brief     Funkcja do obsługi całego poprawnie odebranego pakietu danych (znów można by się pokusić o przekazanie wskaźnika i sługości tablicy jako parametrów zamiast praca na globalach)
*/
void handle_whole_packet() {
  if (calc_checksum() == create_uint16(packet[20], packet[21])) { // jeśli obliczona suma kontrolna zgadza się z odebraną sumą
    temp1 = parse_float(temp1_offset);                            // przeliczenie temperatury z tablicy odebranego pakietu od zadanego offsetu
    temp2 = parse_float(temp2_offset);
    temp3 = parse_float(temp3_offset);
    received_time = parse_float(time_offset);                     // przeliczenie czasu od włączenia czujnika z tablicy odebranego pakietu od zadanego offsetu                                 
    Serial.println("Czas pracy czujnika: " + String(received_time) +
                    " TEMP1: " + String(temp1) +
                    " TEMP2: " + String(temp2) +
                    " TEMP3: " + String(temp3));
    // ToDo: Zapisz odebrane dane na karcie

    received_packets++;
  } else {                                                        // jeśli odebrana suma kontrolna różni się od obliczonej
    broken_packets++;
    Serial.println("BŁĄD - Suma kontrolna pakietu różni się od obliczonej sumy. Liczba uszkodzonych pakietów: " + String(broken_packets));
    Serial.println("INFO - Procent poprawnie odebranych pakietów: " +
                    String(calc_percentage((incomplete_packets + broken_packets), received_packets)) + "%");
  }
}

/**
 * \brief                 Funkcja obsługująca odebrane pakiety z czujnika
 * \param[received_byte]  Odebrany z czujnika bajt danych
*/
void handle_received_packet(uint8_t received_byte) {
  if ((received_byte == packet_header[0]) && (received_byte_index != 0)) {  // jeśli odebrany znak to pierwszy znak nagłówka pakietu ale nie jest pierwszym bajtem pakietu        
    received_byte_index = 0;                // zresetuj index odbieranego bajtu
    incomplete_packets++;                 // podnieś ilość urwanych pakietów
    Serial.println("BŁĄD - Urwano ostatni pakiet danych. Liczba niekompletnych pakietów: " + String(incomplete_packets));
    Serial.println("INFO - Procent poprawnie odebranych pakietów: " +
                    String(calc_percentage((incomplete_packets + broken_packets), received_packets)) + "%");
  }
  
  if (received_byte_index < packet_header_size) {                     // jeśli index odbieranego pakietu jest mniejszy niż wielkość nagłówka
    if (received_byte == packet_header[received_byte_index]) {        // jeśli odebrany bajt zgadza się z kolejnymi bajtami nagłówka
      packet[received_byte_index++] = received_byte;                  // wpisanie odebranego bajtu do tablicy pakietu
    } else {                                                          // jeśli odebrany bajt nie zgadza się z kolejnymi bajtami nagłówka
      received_byte_index = 0;                                        // zresetuj index odbieranego bajtu
      incomplete_packets++;                                           // podnieś ilość urwanych pakietów
      Serial.println("BŁĄD - Urwano ostatni pakiet danych. Liczba niekompletnych pakietów: " + String(incomplete_packets));
      Serial.println("INFO - Procent poprawnie odebranych pakietów: " +
                    String(calc_percentage((incomplete_packets + broken_packets), received_packets)) + "%");
    }
  } else {                                                            // jeśli index odbieranego minął wielkość nagłówka
    packet[received_byte_index++] = received_byte;                    // przypisanie odebranego bajtu do kolejnego elementu tablicy pakietu
    
    if (received_byte_index == packet_size) {                         // jeśli odebrano pełny pakietu
      handle_whole_packet();                                          // obsłuż cały poprawnie odebrany pakiet
      received_byte_index = 0;                                        // wyzeruj index odebranego pakietu
    }
  }
}

/**
 * \brief     Funkcja wyświetlająca pomoc
*/
void print_help() {
  Serial.println("Dostępne opcje:");
  Serial.println("-> reset - zeruje liczniki odebranych i błędnych pakietów");
  Serial.println("-> info - wyświetlenie stanu liczników odebranych pakietów");
  // ToDo
}

/**
 * \brief     Funkcja resetująca liczniki odebranych i błędnych pakietów
*/
void handle_reset() {
  incomplete_packets = 0;
  broken_packets = 0;
  received_packets = 0;
  Serial.println("INFO: Liczniki zostały wyzerowane");
}

/**
 * \brief     Funkcja wyświetlająca stan liczników odebranych pakietów
*/
void handle_info() {
  Serial.println("INFO: Poprawne pakiety - " + String(received_packets) +
                  " Urwane pakiety: " + String(incomplete_packets) +
                  " Uszkodzone pakiety: " + String(broken_packets) +
                  " Procent poprawnych pakietów: " + String(calc_percentage(
                                                            (incomplete_packets + broken_packets),
                                                              received_packets)) + "%");
}


/**
 * \brief     Funkcja do obsługi odebranego z PC polecenia
*/
void handle_pc_command() {
  if (pc_input == "help") {
    print_help();
  } else if (pc_input == "reset") {
    handle_reset();
  } else if (pc_input == "info") {
    handle_info();
  } else {
    Serial.println("Nieznana komenda. Wpisz 'help' aby wyświetlić pomoc.");
  }
  pc_input = "";
}

/**
 * \brief     Domyślna funkcja arduino wykonywana na początku uruchamiania urządzenia
*/
void setup() {
  Serial.begin(9600);                     // inicjalizacja portu do komunikacji z PC
  Serial1.begin(9600);                    // inicjalizacja portu do komunikacji z czujnikiem temp

  Serial.println();
  Serial.println("-----------------------------------------------");
  Serial.println("Rejestrator temperatury został zainicjalizowany");  // debug info wysłane to terminala PC
  Serial.println("-----------------------------------------------");
  print_help();
  Serial.println("-----------------------------------------------");
}

/**
 * \brief     Główna pętla programu (ta funkcja wykonuje się w nieskończoność)
*/
void loop() {
  if (Serial1.available()) {                // jeśli dostępny jest bajt na porcie szeregowym czujnika
    handle_received_packet(Serial1.read()); // obsłuż odebrany bajt danych
  }

  if (Serial.available()) {                   // jeśli dostępny jest bajt na porcie szeregowym czujnika
    pc_input = Serial.readStringUntil('\n');  // odbierz z portu szeregoweg cały ciąg aż do znaku nowej linii
    pc_input.trim();                          // wyczyść ciąg z białych znaków na jego końcu
    handle_pc_command();                      // obsłóż odebrane z PC polecenie
  }
  delay(80);                                  // nwm czemu akurat 80ms, ale tak było w kodzie
}
