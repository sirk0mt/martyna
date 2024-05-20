#include <Arduino.h>

const int   packet_size         = 22;                   // wielkość pakietu
const char  packet_header[]     = "snp";                // nagłówek pakietu
const int   packet_header_size = sizeof(packet_header); // wielkość nagłówka pakietu
uint8_t     packet[packet_size];                        // tablica bajtowado przechowywania pakietu
uint8_t     temp1_offset        = 4;                    // offset w pakiecie dla 1 temperatury
uint8_t     temp2_offset        = temp1_offset + 4;     // offset w pakiecie dla 2 temperatury
uint8_t     temp3_offset        = temp2_offset + 4;     // offset w pakiecie dla 3 temperatury
uint8_t     time_offset         = temp3_offset + 4;     // offset w pakiecie dla czasu od startu czujnika
int         packet_byte_index   = 0;                    // index odebranego znaku pakietu
int         incomplete_packets  = 0;                    // "urwane" pakiety
int         broken_packets      = 0;                    // uszkodzone pakiety
char        incoming_byte;                              // odebrany bajt
float       temp1, temp2, temp3, received_time;         // zmienne dla odebranych liczb zmiennoprzecinkowych

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
  temp1 = parse_float(temp1_offset);        // przeliczenie temperatury z tablicy odebranego pakietu od zadanego offsetu
  temp2 = parse_float(temp2_offset);
  temp3 = parse_float(temp3_offset);
  received_time = parse_float(time_offset); // przeliczenie czasu od włączenia czujnika z tablicy odebranego pakietu od zadanego offsetu
}

void setup() {
  Serial.begin(9600);                     // inicjalizacja portu do komunikacji z PC
  Serial1.begin(9600);                    // inicjalizacja portu do komunikacji z czujnikiem temp
}

void loop() {
  if (Serial1.available()) {              // jeśli dostępny jest bajt na porcie szeregowym
    incoming_byte = Serial.read();        // przypisanie odebranego bajtu do zmiennej
    
    if (packet_byte_index < packet_header_size) {                       // jeśli index odbieranego pakietu jest mniejszy niż wielkość nagłówka
      if (incoming_byte == packet_header[packet_byte_index]) {          // jeśli odebrany bajt zgadza się z kolejnymi bajtami nagłówka
        packet[packet_byte_index++] = incoming_byte;                    // wpisanie odebranego bajtu do tablicy pakietu
      } else {                                                          // jeśli odebrany bajt nie zgadza się z kolejnymi bajtami nagłówka
        packet_byte_index = 0;                                          // zresetuj index odbieranego bajtu
        incomplete_packets++;                                           // podnieś ilość urwanych pakietów
      }
    } else {                                                            // jeśli index odbieranego minął wielkość nagłówka
      packet[packet_byte_index++] = incoming_byte;                      // przypisanie odebranego bajtu do kolejnego elementu tablicy pakietu
      
      if (packet_byte_index == packet_size) {                           // jeśli odebrano pełny pakietu
        if (calc_checksum() == create_uint16(packet[20], packet[21])) { // jeśli obliczona suma kontrolna zgadza się z odebraną sumą
          handle_whole_packet();                                         
        } else {                                                        // jeśli odebrana suma kontrolna różni się od obliczonej
          broken_packets++;
        }

        packet_byte_index = 0;                                          // wyzeruj index odebranego pakietu
      }
    }
  }
}
