#include <Arduino.h>

const int   packet_size         = 22;                   // wielkość pakietu
const char  packet_header[]     = "snp";                // nagłówek pakietu
const int   packet_header_size = sizeof(packet_header); // wielkość nagłówka pakietu
uint8_t     packet[packet_size];                        // tablica bajtowado przechowywania pakietu
uint8_t     temp1_offset        = 4;                    // offset w pakiecie dla 1 temperatury
uint8_t     temp2_offset        = temp1_offset + 4;     // offset w pakiecie dla 2 temperatury
uint8_t     temp3_offset        = temp2_offset + 4;     // offset w pakiecie dla 3 temperatury
uint8_t     time_offset         = temp3_offset + 4;     // offset w pakiecie dla czasu od startu czujnika
uint8_t     received_byte       = 0;                    // odebrany bajt danych
uint16_t    calculated_checksum = 0;                    // obliczona suma kontrolna
uint16_t    received_checksum   = 0;                    // odebrana suma kontrolna
int         received_byte_index = 0;                    // index odebranego znaku pakietu
int         incomplete_packets  = 0;                    // "urwane" pakiety
int         broken_packets      = 0;                    // uszkodzone pakiety
int         received_packets    = 0;                    // ilość odebranych poprawnych pakietów
float       temp1, temp2, temp3, received_time;         // zmienne dla odebranych liczb zmiennoprzecinkowych
float       good_packets_percent = 0;                   // procent poprawnych pakietów
String      pc_input;                                   // zmienna dla danych wejściowych z PC

union byte_to_float_t {                                 // jednak unia :) - pozwala ona na łatwiejsze manipulowanie danymi w tym samym obszarze pamięci
    uint8_t bytes[4];                                   // tablica bajtowa dla kolejnych elementów liczby
    float value;                                        // wartość wyjściowa typu float
} byte_to_float;

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

  Serial.println("-----------------------------------------------");
  Serial.println("Dostępne opcje:");
  Serial.println("-> reset - zeruje liczniki odebranych i błędnych pakietów");
  Serial.println("-> info - wyświetlenie stanu liczników odebranych pakietów");
}

/**
 * \brief     Główna pętla programu (ta funkcja wykonuje się w nieskończoność)
*/
void loop() {
  if (Serial1.available()) {                // jeśli dostępny jest bajt na porcie szeregowym czujnika
    received_byte = Serial1.read();         // wpisanie odebranego bajtu do zmiennej
    if ((received_byte == packet_header[0]) && (received_byte_index != 0)) {  // jeśli odebrany znak to pierwszy znak nagłówka pakietu ale nie jest pierwszym bajtem pakietu        
      received_byte_index = 0;              // zresetuj index odbieranego bajtu
      incomplete_packets++;                 // podnieś ilość urwanych pakietów
      Serial.println("BŁĄD - Urwano ostatni pakiet danych. Liczba niekompletnych pakietów: " + String(incomplete_packets));
      good_packets_percent = ((float)(incomplete_packets + broken_packets) / (float)received_packets) * 100; // obliczanie procentu poprawnych pakietów
      Serial.println("INFO - Procent poprawnie odebranych pakietów: " +
                      String(good_packets_percent) + "%");
    }
  
    if (received_byte_index < packet_header_size) {                     // jeśli index odbieranego pakietu jest mniejszy niż wielkość nagłówka
      if (received_byte == packet_header[received_byte_index]) {        // jeśli odebrany bajt zgadza się z kolejnymi bajtami nagłówka
        packet[received_byte_index++] = received_byte;                  // wpisanie odebranego bajtu do tablicy pakietu
      } else {                                                          // jeśli odebrany bajt nie zgadza się z kolejnymi bajtami nagłówka
        received_byte_index = 0;                                        // zresetuj index odbieranego bajtu
        incomplete_packets++;                                           // podnieś ilość urwanych pakietów
        Serial.println("BŁĄD - Urwano ostatni pakiet danych. Liczba niekompletnych pakietów: " + String(incomplete_packets));
        good_packets_percent = ((float)(incomplete_packets + broken_packets) / (float)received_packets) * 100; // obliczanie procentu poprawnych pakietów
        Serial.println("INFO - Procent poprawnie odebranych pakietów: " +
                      String(good_packets_percent) + "%");
      }
    } else {                                                            // jeśli index odbieranego minął wielkość nagłówka
      packet[received_byte_index++] = received_byte;                    // przypisanie odebranego bajtu do kolejnego elementu tablicy pakietu
      
      if (received_byte_index == packet_size) {                         // jeśli odebrano pełny pakietu
        calculated_checksum = 0;
        for (int i = 0; i < 20 - 1; i++) {   // pętla służąca do "przejścia" po wszystkich elementach tablicy pakietu
          calculated_checksum += packet[i];             // sumowanie kolejnych wyrazów tablicy
        }

        received_checksum = (static_cast<uint16_t>(packet[20]) << 8) | packet[21]; // rzutowanie 2 odebranych bajtów sumy kontrolnej z przesunięciem bitowym

        if (calculated_checksum == received_checksum) { // jeśli obliczona suma kontrolna zgadza się z odebraną sumą
          for (int i = 0; i < 4; i++) {                 // przeliczenie temperatury z tablicy odebranego pakietu od zadanego offsetu
            byte_to_float.bytes[i] = packet[temp1_offset + i];
          }
          temp1 = byte_to_float.value;
                           
          for (int i = 0; i < 4; i++) {                 // przeliczenie temperatury z tablicy odebranego pakietu od zadanego offsetu
            byte_to_float.bytes[i] = packet[temp2_offset + i];
          }
          temp2 = byte_to_float.value;

          for (int i = 0; i < 4; i++) {                 // przeliczenie temperatury z tablicy odebranego pakietu od zadanego offsetu
            byte_to_float.bytes[i] = packet[temp3_offset + i];
          }
          temp3 = byte_to_float.value;

          for (int i = 0; i < 4; i++) {                 // przeliczenie czasu od włączenia czujnika z tablicy odebranego pakietu od zadanego offsetu
            byte_to_float.bytes[i] = packet[time_offset + i];
          }
          received_time = byte_to_float.value;
                                
          Serial.println("Czas pracy czujnika: " + String(received_time) +
                          " TEMP1: " + String(temp1) +
                          " TEMP2: " + String(temp2) +
                          " TEMP3: " + String(temp3));
          // ToDo: Zapisz odebrane dane na karcie

          received_packets++;
        } else {                                                        // jeśli odebrana suma kontrolna różni się od obliczonej
          broken_packets++;
          Serial.println("BŁĄD - Suma kontrolna pakietu różni się od obliczonej sumy. Liczba uszkodzonych pakietów: " + String(broken_packets));
          good_packets_percent = ((float)(incomplete_packets + broken_packets) / (float)received_packets) * 100; // obliczanie procentu poprawnych pakietów
          Serial.println("INFO - Procent poprawnie odebranych pakietów: " +
                          String(good_packets_percent) + "%");
        }
        received_byte_index = 0;                                        // wyzeruj index odebranego pakietu
      }
    }

  }

  if (Serial.available()) {                   // jeśli dostępny jest bajt na porcie szeregowym czujnika
    pc_input = Serial.readStringUntil('\n');  // odbierz z portu szeregoweg cały ciąg aż do znaku nowej linii
    pc_input.trim();                          // wyczyść ciąg z białych znaków na jego końcu
    if (pc_input == "help") {                 // jeżeli wprowadzono polecenie "help"
      Serial.println("Dostępne opcje:");
      Serial.println("-> reset - zeruje liczniki odebranych i błędnych pakietów");
      Serial.println("-> info - wyświetlenie stanu liczników odebranych pakietów");
    } else if (pc_input == "reset") {         // jeżeli wprowadzono polecenie "reset"
      incomplete_packets = 0;
      broken_packets = 0;
      received_packets = 0;
      Serial.println("INFO: Liczniki zostały wyzerowane");
    } else if (pc_input == "info") {          // jeżeli wprowadzono polecenie "info"
      good_packets_percent = ((float)(incomplete_packets + broken_packets) / (float)received_packets) * 100; // obliczanie procentu poprawnych pakietów
      Serial.println("INFO: Poprawne pakiety - " + String(received_packets) +
                      " Urwane pakiety: " + String(incomplete_packets) +
                      " Uszkodzone pakiety: " + String(broken_packets) +
                      " Procent poprawnych pakietów: " + String(good_packets_percent) + "%");
    } else {
      Serial.println("Nieznana komenda. Wpisz 'help' aby wyświetlić pomoc.");
    }
    pc_input = "";                            // wyczyść odebraną z PC komendę
    }
  delay(80);                                  // nwm czemu akurat 80ms, ale tak było w kodzie
}
