#include "MenuBackend.h" // attach the library
#include <NewPing.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> // handle the LCD display

// Light pins
#define LIGHT_1 6 // Glowne
#define LIGHT_2 7 // Prawe
#define LIGHT_3 13 // Lewe
#define PRINTER_PIN A0 // Printer

//Sensors
#define TRIGGER_PIN  2  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     3  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define TRIGGER_PIN2  4  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN2     5  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

// --- Define their own characters for the LCD arrow: down, left, right, the top-down and back ---
uint8_t arrowUpDown[8] = {0x4, 0xe, 0x15, 0x4, 0x15, 0xe, 0x4};
uint8_t arrowDown[8] = {0x4, 0x4, 0x4, 04, 0x15, 0xe, 0x4};
uint8_t arrowRight[8] = {0x0, 0x4, 0x2, 0x1f, 0x2, 0x4, 0x0};
uint8_t arrowLeft[8] = {0x0, 0x4, 0x8, 0x1f, 0x8, 0x4, 0x0};
uint8_t arrowBack[8] = {0x1, 0x1, 0x5, 0x9, 0x1f, 0x8, 0x4};

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // definicja pinów dla LCD
volatile int keyboardchange = -1; // to dla kontroli zmiany stanu klawiatury
volatile int button = -1; // zmienna pomocnicza

char *Line_1; // First LCD line
char *Line_2; // 2nd LCD line

String receivedstring;
volatile char command;
volatile boolean check = false;
boolean mainmenu = 1;
boolean statemenu = 0;
boolean exitmenu = 0;

/////////////////////////////////////////////////////////////////////////////////////////

class LcdLight
{
private: long OffTime;    // milliseconds of off-time

  // These maintain the current state
private: unsigned long previousMillis;   // will store last time LED was updated
private: unsigned long currentMillis = millis();

  // Constructor
public: LcdLight(long off) {
    OffTime = off;
    previousMillis = 0;
  }

  void BackLightOn() {
    lcd.setBacklight(255);
    currentMillis = millis();
    previousMillis = currentMillis - 1;  
  }

  void Update() {
    // check to see if it's time to change the state of the LED
    currentMillis = millis();
    
    if(previousMillis > 0 && (currentMillis - previousMillis >= OffTime)) {
      lcd.setBacklight(0);  
      previousMillis = currentMillis;  // Remember the time
    }
  }
};

class SendDelay
{
private: long interval;    // milliseconds of interval

  // These maintain the current state
private: unsigned long previousMillis2;   // will store last time send was updated
private: unsigned long currentMillis = millis();

  // Constructor
public: SendDelay(long interval_val) {
    interval = interval_val;
    previousMillis2 = 0;
  }

  void ShowTime() {
    Serial.print(previousMillis2);
    Serial.print("  ");
    Serial.println(currentMillis);
  }

  bool Update2() {
    // check to see if it's time to change the state of the LED
    currentMillis = millis();
    
    if(currentMillis - previousMillis2 >= interval) {
      previousMillis2 = currentMillis;  // Remember the time
      return true;
    }
    else {
      currentMillis = millis();
      return false;
    }
  }
};

class Lights
{
private: unsigned char pin;

  // Constructor
public: Lights(unsigned char outputpin) {
    pin = outputpin;
  }

  void SetState(bool state) {
    digitalWrite(pin, state);
    delay(10);
  }

  bool GetState() {
    return digitalRead(pin);
  }
};

//Objects declaration
LcdLight backlight1(30000);
LcdLight backlight2(10000);
LcdLight backlight3(5000);
SendDelay senddelay1(1000);
Lights light1(LIGHT_1);
Lights light2(LIGHT_2);
Lights light3(LIGHT_3);
Lights printer(PRINTER_PIN);

/////////////////////////////////////////////////////////////////////////////////////////

// ----- Sensors ------------------------------------------------------
NewPing sonar1(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing sonar2(TRIGGER_PIN2, ECHO_PIN2, MAX_DISTANCE);
// --- Create all menu options: ---------------------------------------
// Create a de facto class MenuItem objects that inherit from the class MenuBackend
MenuBackend menu = MenuBackend(menuUseEvent, menuChangeEvent); // konstruktor

MenuItem P1 = MenuItem("   Swiatlo    ", 1);
MenuItem P11 = MenuItem("   Glowne     ", 3);
MenuItem P111 = MenuItem("   Wylacz G   ", 4);
MenuItem P112 = MenuItem("   Wlacz G    ", 4);
MenuItem P12 = MenuItem("   Prawe P    ", 3);
MenuItem P121 = MenuItem("   Wylacz P   ", 4);
MenuItem P122 = MenuItem("   Wlacz P    ", 4);
MenuItem P13 = MenuItem("   Lewe R     ", 3);
MenuItem P131 = MenuItem("   Wylacz L   ", 4);
MenuItem P132 = MenuItem("   Wlacz L    ", 4);

MenuItem P2 = MenuItem(" Sprawdz Stan ", 1);

MenuItem P3 = MenuItem("   Wyjscie    ", 1);

void menuSetup() // function class MenuBackend
{
  /////////////////////////////////////////////
  menu.getRoot().add(P1); // Set the root menu, which is the first option
  P1.add(P11); // parent has a child new so adds them vertically
  P11.addRight(P111);
  P111.add(P112);
  P111.addLeft(P11);
  P112.addLeft(P11);
  P112.add(P111); // Close the loop, etc. ..
  P11.addLeft(P1);
  P11.add(P11); // Loop closure
  P11.addLeft(P1);
  P11.add(P12);
  P12.addRight(P121);
  P121.add(P122);
  P121.addLeft(P12);
  P122.addLeft(P12);
  P122.add(P121); // Loop closure, etc. ..
  P12.addLeft(P1);
  P12.add(P11); // Loop closure
  P12.addLeft(P1);
  P12.add(P13);
  P13.addRight(P131);
  P131.add(P132);
  P131.addLeft(P13);
  P132.addLeft(P13);
  P132.add(P131); // Loop closure, etc. ..
  P13.addLeft(P1);
  P13.add(P11); // Loop closure
  P1.addRight(P2); // Right for the EDIT FILE

  P2.addRight(P3); // Wyjscie
  P3.addRight(P1); // Main loop closure, ie, the horizontal - the AID is FILE

}

void menuUseEvent(MenuUseEvent used)
// funkcja klasy MenuBackend - reButton na wciśnięcie OK
// tutaj właśnie oddajemy menu na rzecz akcji obsługi klawisza OK
{

  //Serial.print("wybrano:  ");
  //Serial.println(used.item.getName()); // only for tests

  /************FUNKCJE PODMENU************/

  if (used.item.getName() == "   Glowne     ") {
    menu.moveRight();
  }
  if (used.item.getName() == "   Prawe P    ") {
    menu.moveRight();
  }
  if (used.item.getName() == "   Lewe R     ") {
    menu.moveRight();
  }

  if (used.item.getName() == "   Wlacz G    ") {
    if (light1.GetState() == HIGH) {
      lcd.setCursor(1,0);
      lcd.print(" Juz wlaczone!");
    }
    else {
      light1.SetState(HIGH);
      lcd.setCursor(1,0);
      lcd.print("  Wlaczono!   ");
    }
    delay(2000); 
    menu.moveLeft();
  }
  if (used.item.getName() == "   Wylacz G   ") {
    if (light1.GetState() == LOW) {
      lcd.setCursor(1,0);
      lcd.print("Juz wylaczone!");
    }
    else {
      light1.SetState(LOW);
      lcd.setCursor(1,0);
      lcd.print("  Wylaczono!  ");
    }
    delay(2000); 
    menu.moveLeft();
  }

  if (used.item.getName() == "   Wlacz P    ") {
    if (light2.GetState() == HIGH) {
      lcd.setCursor(1,0);
      lcd.print(" Juz wlaczone!");
    }
    else {
      light2.SetState(HIGH);
      lcd.setCursor(1,0);
      lcd.print("  Wlaczono!   ");
    }
    delay(2000); 
    menu.moveLeft();
  }
  if (used.item.getName() == "   Wylacz P   ") {
    if (light2.GetState() == LOW) {
      lcd.setCursor(1,0);
      lcd.print("Juz wylaczone!");
    }
    else {
      light2.SetState(LOW);
      lcd.setCursor(1,0);
      lcd.print("  Wylaczono!  ");
    }
    delay(2000); 
    menu.moveLeft();
  }

  if (used.item.getName() == "   Wlacz L    ") {
    if (light3.GetState() == HIGH) {
      lcd.setCursor(1,0);
      lcd.print(" Juz wlaczone!");
    }
    else {
      light3.SetState(HIGH);
      lcd.setCursor(1,0);
      lcd.print("  Wlaczono!   ");
    }
    delay(2000); 
    menu.moveLeft();
  }
  if (used.item.getName() == "   Wylacz L   ") {
    if (light3.GetState() == LOW) {
      lcd.setCursor(1,0);
      lcd.print("Juz wylaczone!");
    }
    else {
      light1.SetState(LOW);;
      lcd.setCursor(1,0);
      lcd.print("  Wylaczono!  ");
    }
    delay(2000); 
    menu.moveLeft();
  }

  /***************SPRAWDZ STAN*****************/

  if (used.item.getName() == " Sprawdz Stan ") {
    mainmenu = 0;
    statemenu = 1;
    exitmenu = 0;
    delay(200);
    backlight3.BackLightOn();
  }

  /***************WYJSCIE*****************/

  if (used.item.getName() == "   Wyjscie    ") {  // strona glowna
    mainmenu = 0;
    statemenu = 0;
    exitmenu = 1;
    delay(200);
    backlight2.BackLightOn();
  }

  /***************PODSWIETLENIE****************

// A teraz coś ambitniejszego :-), bo przekazujemy sterowanie klawiaturką do innej procedury,
// w tym przykładzie programik czeka aż ustawisz jakąś temperaturę i po wciśnięciu OK wraca do pętli głównej
if (used.item.getName() == "Podswietlenie ")
// dokładnie taki sam ciąg " Temperatura"
{
int lightLevel = 250; // przykładowo 250
lcd.setCursor(0, 0);
lcd.write(7); // wyswietlamy nasz symbol strzałki góra-dół
lcd.print("              ");
lcd.setCursor(1, 0);
lcd.print("Ust.pods.= "); // tekst dla użytkownika
lcd.setCursor(12, 0);
lcd.print(lightLevel); // wyświetlamy akt. stan
int button = -1;
delay(1000); // zmienna pomocnicza, sterująca dla petli while
// jesli nie puścisz klawisza OK w ciągu 1 sek. to powrót do menu
while (button != 4) // ta pętla trwa tak długo aż wciśniesz klawisz OK
{
keyboardchange = -1;
button = Read_3(8, 9, 10, 11, 12);
// delay(300);   // odczyt stanu klawiatury - funkcja Read_1 lub Read_2 lub Read_3
// opis poniżej przy 3 różnych definicjach funkcji Read
if (keyboardchange != button)
// ruszamy do pracy tylko wtedy gdy zmienił sie stan klawiatury
{
  if (button == 1) {
  lightLevel += 10;
  if (lightLevel > 250)
  lightLevel = 250;
  lcd.setCursor(12, 0);
  lcd.print(lightLevel);
  lcd.setBacklight(lightLevel);
  if (lightLevel < 100) {
  lcd.setCursor(14, 0);
  lcd.print(" ");
  }
  delay(300);
  } // jesli button=1 (czyli wciśnieto klawisz w górę to zwiększono temperaturę
  // ustawiono max próg i wyświetlono obecną temperaturę
  if (button == 2) {
  lightLevel -= 10;
  if (lightLevel < 0)
  lightLevel = 0;
  lcd.setCursor(12, 0);
  lcd.print(lightLevel);
  lcd.setBacklight(lightLevel);
  if (lightLevel < 100) {
  lcd.setCursor(14, 0);
  lcd.print(" ");
  }
  delay(300);
  } // jesli button=2 (czyli wciśnieto klawisz w dół to mniejszono temperaturę
  // ustawiono min próg i wyświetlono obecną temperaturę
  if (button == 4) // jeśli wciśnieto OK
  {
  lcd.setCursor(0, 0);
  lcd.print("Podswietlenie OK");
  delay(2000); // pokazujemy OK przez 2 sek.
  lcd.setCursor(1, 0);
  lcd.print("              "); // czyścimy linię
  lcd.setCursor(1, 0);
  lcd.print(Line_1); // odtwarzamy poprzedni stan na LCD
  }
}
}
keyboardchange = button;
// aktualizacja zmiennej keyboardchange, po to aby reagować tylko na zmiany stanu klawiatury
// tu WAŻNY MOMENT - kończy się pętla while i zwracamy sterowanie do głównej pętli loop()
}*/
}

// --- Response to pressing ----------------------------------------- ------------------------
void menuChangeEvent(MenuChangeEvent changed) // function class MenuBackend
{
  /* So really it is only here that shortkey useful and is used primarily to enrich the menu
of arrow symbols, whichever is selected. Everything here is going on is displayed on the LCD.
*/
  int c = changed.to.getShortkey(); // Fetch shortkey (1,2,3, LUB4)
  lcd.clear(); // No comment
  lcd.setCursor(0, 0);
  if (c == 1) { // if the menu Main contacts (shortkey = 1) are:
    lcd.write(3); // Left arrow
    strcpy(Line_1, changed.to.getName());
    // Create a string in the first line
    lcd.print(Line_1); // Display it
    lcd.setCursor(15, 0);
    lcd.write(4); // Right arrow
    lcd.setCursor(0, 1);
    lcd.write(5); // Down arrow
    lcd.setCursor(15, 1);
    lcd.write(5); // Down arrow
  }
  if (c == 2)  { // if the submenu for the child - (shortkey = 2) are:
    lcd.print("*"); // Draw a star
    strcpy(Line_2, changed.to.getName());
    // Create a string in the first line
    lcd.print(Line_1); // Display it
    lcd.setCursor(15, 0);
    lcd.print("*"); // Star
    lcd.setCursor(0, 1);
    lcd.write(6); // Second line and arrow return (arrowBack)
    lcd.print(changed.to.getName()); // Display name of "child"
    lcd.setCursor(15, 1);
    lcd.write(7); // Arrow up and down
  }
  if (c == 3) { // if the child has a child - (shortkey = 3) are:
    lcd.print("*"); // Star
    strcpy(Line_2, changed.to.getName());
    // Copy the files. the name of the menu options to the variable line 2
    lcd.print(Line_1); // And display the first line of
    lcd.setCursor(15, 0);
    lcd.print("*"); // Star
    lcd.setCursor(0, 1);
    lcd.write(6); // Second line and arrow arrowBack
    lcd.print(changed.to.getName());
    // Display the grandson of the second line
    lcd.setCursor(15, 1);
    lcd.write(4); // Right arrow as they are the grandchildren
  }

  if (c == 4) { // if this grandson (shortkey = 4) are:
    lcd.print("*"); // Gwaizdka
    lcd.print(Line_2);
    // Display the first line of the child (or parent grandchild)
    lcd.setCursor(15, 0);
    lcd.print("*"); // Gwaizdka
    lcd.setCursor(0, 1);
    lcd.write(6); // Second line and arrow arrowBack
    lcd.print(changed.to.getName()); // Display grandson
    lcd.setCursor(15, 1);
    lcd.write(7); // Arrow up and down
  }
}

// For example if you are using pins: 1,2,3,11 and 12 are calling: Read_2(8,4,7,8,11)
int Read_3(int up, int left, int ok, int right, int down)
{
// up - no digital pin that is connected to the up
// Left - no digital pin that is connected to the left buttons
// Ok - no digital pin that is connected to the OK
// Right - no digital pin that is connected to the right buttons
// Lower - no digital pin that is connected to the buttons down
  if (digitalRead(up) == LOW)
  return 1;
  if (digitalRead(left) == LOW)
  return 3;
  if (digitalRead(ok) == LOW)
  return 4;
  if (digitalRead(right) == LOW)
  return 0;
  if (digitalRead(down) == LOW)
  return 2;
  return -1;
}

// ============================================================================================
//
void setup()
{
  Line_1 = new char[16]; // Initialize a pointer to a dynamic text
  Line_2 = new char[16];
  // Is VERY IMPORTANT because dynamic indicator must indicate
  // Pre-defined location in memory. If we did not do
  // This sooner or later applet could jump in indefinite
  // Close the memory area, which can result in irreversible consequences
  // Including switching Fuse Bits!
  // Please be careful at all dynamic indicators, SUCH GOOD ADVICE :-)
  Serial.begin(9600); // Initialize serial, mainly for test
  lcd.begin(16, 2); // Initialize the LCD
  // Create a memory of your LCD 5 characters for arrows
  lcd.createChar(3, arrowLeft);
  lcd.createChar(4, arrowRight);
  lcd.createChar(5, arrowDown);
  lcd.createChar(6, arrowBack);
  lcd.createChar(7, arrowUpDown);
  //Ustawianie pinów wyjściowych
  pinMode(LIGHT_1, OUTPUT);
  pinMode(LIGHT_2, OUTPUT);
  pinMode(LIGHT_3, OUTPUT);
  pinMode(PRINTER_PIN, OUTPUT);
  // Here exemplary digital pins for the 3 versions feature Read_3 (1,2,3,11,12)
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);

  light1.SetState(LOW);
  light2.SetState(LOW);
  light3.SetState(LOW);
  printer.SetState(LOW);

  // pinMode (0, OUTPUT); digitalWrite (0, LOW); // For testing
  menuSetup();
  // Function MenuBackend class - there really are creating our menu
  lcd.setCursor(0,0);
  lcd.print(" Light Control  ");
  lcd.setCursor(0,1);
  lcd.print("Rafal Olszewski");
  delay(2000);
  backlight1.BackLightOn();
  menu.moveDown();
  menu.moveLeft();
  menu.use();
  BluetoothControl();
}

// --- And it's time for neverending story :-) -------------------------------------------------
///////////////////////////////MAIN LOOOOOOOOOOOP///////////////////////////////////////////////

void loop()
{
  if (mainmenu == 1) {
    ButtonRead();
  }
  if(exitmenu == 1) {
    ExitMenu();
  }
  if(statemenu == 1) {
    StateMenu();
  }
  //Sensors();
  BluetoothControl();
  SendAndroidValues();
}

///////////////////////////////END OF LOOOOOOOOOOOP///////////////////////////////////////////////

void Stan(bool swiatlo)
{
  if(swiatlo) lcd.print("ON ");
  else lcd.print("OFF"); 
}

void ButtonRead()
{
  backlight1.Update();
  button = Read_3(8, 9, 10, 11, 12);
  delay(10); // Read the state of the keyboard:
  if (keyboardchange != button) { // if it was the change in the state are:
    backlight1.BackLightOn(); // wlaczenie podswietlenia
    switch (button) { // check to see what was pressed
    case 0:
      menu.moveRight();
      break; // If pressed, move it in the right menu to the right
    case 1:
      menu.moveUp();
      break; // Menu to top
    case 2:
      menu.moveDown();
      break; // Menu Down
    case 3:
      menu.moveLeft();\
      break; // Menu to the left
    case 4:
      menu.use();
      break; // Pressed OK, so jump to the function menuUseEvent (MenuUseEvend used)
      // This function is just serve our menu, check here
      // Which option is selected, and here we create code to handle the event.
    }
  }
  keyboardchange = button;
  // Assign the value of x variable amended so that the long pressing the
  // Same key did not result in the re-generation event.
  // Program responds to a change in the keyboard.
}

void BluetoothControl()
{
  if (Serial.available() > 0) {
    receivedstring = "";
  }

  while(Serial.available() > 0) {
    command = (Serial.read());
    
    if(command == ':') {
      break;
    }
    
    else {
      receivedstring += command;
    }
    
    //delay(10);
  }
  if(receivedstring == "G" && light1.GetState() == LOW) {
    light1.SetState(HIGH);
  }

  if(receivedstring =="g" && light1.GetState() == HIGH) {
    light1.SetState(LOW);
  }  

  if(receivedstring == "S" && light2.GetState() == LOW) {
    light2.SetState(HIGH);
  }

  if(receivedstring == "s" && light2.GetState() == HIGH) {
    light2.SetState(LOW);
  }  

  if(receivedstring == "L" && light3.GetState() == LOW) {
    light3.SetState(HIGH);
  }

  if(receivedstring =="l" && light3.GetState() == HIGH) {
    light3.SetState(LOW);
  } 
  if(receivedstring == "P" && printer.GetState() == LOW) {
    printer.SetState(HIGH);
  }

  if(receivedstring =="p" && printer.GetState() == HIGH) {
    printer.SetState(LOW);
  } 
}

void Sensors()
{
  //delay(10);                     // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
  Serial.print("Sensor1: ");
  Serial.print(sonar1.convert_cm(sonar1.ping_median())); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.print("cm Sensor2: ");
  Serial.print(sonar2.convert_cm(sonar2.ping_median()));
  Serial.println("cm");
}

void SendAndroidValues()
{
  //senddelay1.ShowTime();
  volatile int delaystate = senddelay1.Update2(); //added a delay to eliminate missed transmissions
  if(delaystate == 1)  {
    //puts # before the values so our app knows what to do with the data
    Serial.print('#');
    //for loop cycles through 4 sensors and sends values via serial

    Serial.print(light1.GetState());
    Serial.print('+');
    Serial.print(light2.GetState());
    Serial.print('+');
    Serial.print(light3.GetState());
    Serial.print('+');  
    Serial.print(printer.GetState());
    //technically not needed but I prefer to break up data values
    //so they are easier to see when debugging
    Serial.print('~'); //used as an end of transmission character - used in app for string length
    //Serial.println();
  }
}

void ExitMenu()
{
  int button = -1;
  //delay(100);
  lcd.setCursor(0, 0);
  lcd.print("Ilosc osob:     ");
  lcd.setCursor(12, 0);
  lcd.print("x");
  lcd.setCursor(0, 1);
  lcd.print("Menu - hold OK  ");

  backlight2.Update(); // BACKLIGHT
  keyboardchange = -1;
  button = Read_3(8, 9, 10, 11, 12);
  if (keyboardchange != button){ // ruszamy do pracy tylko wtedy gdy zmienił sie stan klawiatury    
    backlight2.BackLightOn();
    if (button == 4){ // jeśli wciśnieto OK
      mainmenu = 1;
      statemenu = 0;
      exitmenu = 0;

      keyboardchange = button;

      lcd.setCursor(1, 0);
      lcd.print("              ");
      lcd.setCursor(1, 0);
      lcd.print(Line_1); // poprzedni stan LCD
      lcd.setCursor(1, 1);
      lcd.print("              ");
      menu.moveRight();
    }
  }
}

void StateMenu()
{
  int button = -1;
  lcd.setCursor(0, 0);
  lcd.print("  Glowne:       ");
  lcd.setCursor(10, 0);
  Stan(light1.GetState()); // G
  lcd.setCursor(0, 1);
  lcd.print(" P:     L:      ");
  lcd.setCursor(4, 1); // P
  Stan(light2.GetState());
  lcd.setCursor(11, 1); // L
  Stan(light3.GetState());
  
  backlight3.Update(); // BACKLIGHT
  keyboardchange = -1;
  button = Read_3(8, 9, 10, 11, 12);

  if (keyboardchange != button){ // ruszamy do pracy tylko wtedy gdy zmienił sie stan klawiatury      
    backlight3.BackLightOn();
    if (button == 4){ // jeśli wciśnieto OK
      mainmenu = 1;
      statemenu = 0;
      exitmenu = 0;
      lcd.setCursor(1, 0);
      lcd.print("              ");
      lcd.setCursor(1, 0);
      lcd.print(Line_1); // poprzedni stan LCD
      lcd.setCursor(1, 1);
      lcd.print("              ");
      menu.moveLeft();
      keyboardchange = button;
    }
  }
}
// === END ==========================================================

