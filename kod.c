uint8_t Tablica[22];                                    // co to za mienna i do czego?
uint16_t chksum;                                        // co to za mienna i do czego?
float incoming;                                         // co to za mienna i do czego?

void setup() {
    Serial.begin(9600);                                 // port do komunikacji z PC
    Serial1.begin(9600);                                // port do komunikacji z czujnikiem temp
}

void loop() {
    /**
     * Serial.available()
     * @brief   Get the number of bytes (characters) available for reading from the serial port.
     *          This is data that’s already arrived and stored in the serial receive buffer (which holds 64 bytes).
     * @return  The number of bytes available to read.
    */
    if (Serial1.available()) {                          // jeśli różne od zera
        /**
         * Serial.peek()
         * @brief   Returns the next byte (character) of incoming serial data without removing it from the internal serial buffer.
         *          That is, successive calls to peek() will return the same character, as will the next call to read().
         * @return  The first byte of incoming serial data available (or -1 if no data is available). Data type: int.
        */
        byte liczba = Serial1.peek();                   // odczyt pierwszego bajtu danych z kolejki
        Serial.println(liczba);                         // wyrzucenie zmiennej na konsolę do PC w oddzielnej linii
        
        if(liczba != 's') {                             // jeśli liczba nie równa 's' - nie wiem czy nie powinno się zrobić rzutowania na inny typ
            //Serial.println("s niezgodne");            // debug info

            /**
             * Serial.read()
             * @brief   Reads incoming serial data.
             * @return  The first byte of incoming serial data available (or -1 if no data is available). Data type: int.
            */
            Serial1.read();                             // brak przypisania gdzieś odebranego bajtu
        } else {                                        // jeśli liczba równa 's'
            Serial.println("s zgodne");                 // debug info
            Tablica[0] = Serial1.read();                // przypisanie odebranego bajtu do pierwszego elementu tablicy (nie wiem czy nie należy zrobić rzutowania na inny typ)

            byte liczba = Serial1.peek();               // ponowne zadeklarowanie istniejącej zmiennej i przypisanie do niej pierwszego bajtu danych z kolejki
            //Serial.print(liczba);

            if (liczba !='n') {                         // jeśli odebrany bajt to nie 'n' (jak wyżej)
                //Serial.println("n niezgodne");
                Serial1.read();                         // jak wyżej
            } else {                                    // jeśli odebrany bajt to n
                Serial.println("n zgodne");             // debug info
                Tablica[1] = Serial1.read();            // przypisanie do drugiego elementu tablicy odebranego bajtu
                
                byte liczba=Serial1.peek();             // jak wyżej

                if (liczba != 'p') {                    // jeśli odebrany bajt to nie 'p' (jak wyżej)
                    //Serial.println("p niezgodne");    // debug info
                    Serial1.read();                     // jak wyżej
                } else {                                // jeśli odebrany bajt to 'p'
                    Serial.println("p zgodne");         // debug info
                    Tablica[2] = Serial1.read();        // przypisanie do 3 elementu tablicy odebranego bajtu
                }

                /**
                 * Serial.readBytes()
                 * @brief           reads characters from the serial port into a buffer. The function terminates if the determined length has been read, or it times out (see Serial.setTimeout()).
                 *                  returns the number of characters placed in the buffer. A 0 means no valid data was found.
                 * @param[buffer]   the buffer to store the bytes in. Allowed data types: array of char or byte.
                 * @param[length]   the number of bytes to read. Allowed data types: int.
                */
                Serial1.readBytes(&(Tablica[3]), 19);   // IMO spoko, że wskaźnik, ale tablica jest innego typu niż odbierane dane
                uint16_t sum = 0;                       // to na sumę wszystkich wyrazów branych do chck sumy?
                for (uint8_t i = 0; i < 20; ++i) {      // for bez klamerki był, więc nie wiem gdzie się zaczyna, a gdzie kończy
                    sum += Tablica[i];                  // sumowanie kolejnych wyrazów tablicy z zadanego w pętli zakresu
                }
                Serial.print("sum ");                   // debug info
                Serial.println(sum);                    // można to zrobić za pomocą jednej linijki
                uint16_t chksum = (Tablica[20] << 8) + Tablica[21]; // do sprawdzenia i stestowania, bo na szybko i w nocy nie mam głowy :p
                //Serial.println(Tablica[20]);          // debug info
                //Serial.println(Tablica[21]);          // debug info
                Serial.println(sum);                    // debug info
                Serial.println(chksum);                 // debug info

                if (sum == chksum) {                    // jeśli policzona suma zgadza się z odebraną
                    Serial.println("suma zgodna");      // debug info
                    union temp1 {                       // czemu unia? i czemu akurat tak nazwany jej typ oraz jej nazwa?
                        uint8_t t[4];                   // kolejne bity odebranej liczby
                        float temp11;                   // to przeliczona wartość?
                    } temp1;
                    temp1.t[0] = Tablica[7];            // przypisanie kolejnych odebranych bitów do wyrazów tablicy unii
                    temp1.t[1] = Tablica[6];
                    temp1.t[2] = Tablica[5];
                    temp1.t[3] = Tablica[4];
                    Serial.println("temp1: ");          // debug info
                    Serial.println(temp1.temp11);

                    union temp2 {
                        uint8_t t[4];
                        float temp22;
                    } temp2;
                    temp2.t[0] = Tablica[11];
                    temp2.t[1] = Tablica[10];
                    temp2.t[2] = Tablica[9];
                    temp2.t[3] = Tablica[8];
                    Serial.println("temp2: ");
                    Serial.println(temp2.temp22);

                    union temp3 {
                        uint8_t t[4];
                        float temp33;
                    } temp3;
                    temp3.t[0] = Tablica[15];
                    temp3.t[1] = Tablica[14];
                    temp3.t[2] = Tablica[13];
                    temp3.t[3] = Tablica[12];
                    Serial.println("temp3: ");
                    Serial.println(temp3.temp33);

                    union time {
                        uint8_t t[4];
                        float time1;
                    } time;
                    time.t[0] = Tablica[19];
                    time.t[1] = Tablica[18];
                    time.t[2] = Tablica[17];
                    time.t[3] = Tablica[16];
                    Serial.println("czas: ");
                    Serial.println(time.time1);
                } else {                                // jeśli policzona suma nie zgadza się z odebraną
                    Serial.println("bledna");           // debug info
                }
            }
        }
    }

    delay(80);
}