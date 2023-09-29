#include <stdio.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
//
#define RST_PIN         9          // Configurable, see typical pin layout above
#define SS_1_PIN        10         // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 2
#define SS_2_PIN        8          // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 1
#define NR_OF_READERS   1
//
byte ssPins[] = {SS_1_PIN, SS_2_PIN};
MFRC522 mfrc522[NR_OF_READERS];   // Create MFRC522 instance.

// setting up lcd screen
LiquidCrystal lcd(3, 8, 4, 5, 6, 7);

// global variables // 
bool loopval = true; // used for scan() funciton to infinite loop until a card is scanned
bool Continue = false; // used in press2continue() function to get out of infinite while loop to go onto the next question
byte x = 0; // stores the card input
// questions:
char arr[5][100] = {"What is Ohm's law? A) v=tr B) v=i/r C) v=ir D) v=t/r ", "What acts as a reservoir in a circuit? A) resistor B) ground C) diode D) LED ", "Do resistors impede the flow of current though a circuit? T/F ", "How many bits are in a byte? A) 2 B) 4 C) 1 D) 8 ", "You don't need a resistor when connecting an LED to 5V. T/F "};
// the number number equivalent for each card to a letter: a = 170, b = 187, c = 204, d = 221
// answers using key from above
int arr2[5] = {204, 187, 170, 221, 187};
// array used to keep track of how many questions are incorrect and need to review after the first round
int incorrect_arr[5] = {-1, -1, -1, -1, -1};
int arr_size = (sizeof(incorrect_arr)/sizeof(incorrect_arr[0])); // finds the size of one of the arrays to use to iterrate through all of them as they are all the same length
int total_questions = 0; // stores amount of questions one needs to go through for each round
int roundnum = 1; // keeps track of round one is on
int line_num = 0; // used for lcd display to display on both lines
int Cursor = 0;   // cursor for lcd display to scroll the text

// setup //
void setup() {
  Serial.begin(9600); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  // setup for rfid scanner
  SPI.begin();     
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN);
    mfrc522[reader].PCD_DumpVersionToSerial();
  }
  // led pins
  pinMode(A0,OUTPUT);
  pinMode(A1,OUTPUT);
  pinMode(A2,OUTPUT);
  // lcd setup
  lcd.begin(16, 2);
  // interupt code
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), press, RISING);
}

// main loop //
void loop() {
  run_game(); // main game function
}

// runs whole game though one function
void run_game(){
  while(arr_check(incorrect_arr, arr_size)){ // while there are still incorrect questions or -1's in the incorrect_arr run through the loop
    int num_correct = 0; // stores how many correct questions one gets during current run
    lcd.clear();
    lcd.print("Round ");
    lcd.print(roundnum);
    delay(2000);
    lcd.clear();
    for(int i=0; i<arr_size; i++){ // iterating though incorrect_arr
      if(incorrect_arr[i] == -1){ // if there is a wrong question (-1 in incorrect_arr) that needs to go over ask the question again, or for the first time for the first run 
        Serial.println(arr[i]);
        scan(i);
        lcd.clear();
        Cursor = 0;
        line_num = 0;
        if(x == arr2[i]){ // if x (card input) is equal to arr2[i] (value of correct answer), then display correct
          num_correct += 1;
          incorrect_arr[i] = 0; // replace the -1 to a 0 in incorrect_arr signifying that the question was answered correctly and there is no need to review for next round
          setColor(0, 255, 0); // set green light
          lcd.print("Correct");
          delay(2000);
          lcd.clear();
          press2continue(num_correct,total_questions); // wait for user input of a button press to go onto the next question and also take in the two inputs to display stats while waiting
        }
        else{ // if x (card input) is not equal to arr2[i] (value of correct answer), then display incorrect 
          setColor(255, 0, 0); // set red light
          lcd.print("Incorrect");
          delay(2000);
          lcd.clear();
          press2continue(num_correct,total_questions);
        }
      }
    }
    roundnum += 1; // round counter
  }
  // when game ends //
  setColor(0,0,0); 
  lcd.clear();
  lcd.print("Press Arduino");
  lcd.setCursor(0,1);
  lcd.print("Button to reset");
  exit(0);
}

// checks all vals in the inputed array (incorrect_arr) and returns false if all vals = 0, else returns true, also updates int total_questions to as many -1's there are
bool arr_check(int arr[], int n){
  int temp = 0;
  for (int i=0; i<n; i++){
    if(arr[i] == -1){
      temp++;
    }
  }
  total_questions = temp;
  if (temp == 0){
    return false;
  }
  else{
    return true;
  }
}

// looks for card to scan, once scaned set value of card to x
void scan(int i){
  loopval = true; // used to infinite loop the while loop up next until a card is scanned so its constantly looking for a signal
  setColor(0, 0, 255); // set led to blue to signify that it is ready to scan
  while (loopval) {
    for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
      scroll(arr[i]); // display the question while waiting for a scan
      delay(500);
      if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {
      dump_byte_array(mfrc522[reader].uid.uidByte); // function which sets variable x to the value of the card
      mfrc522[reader].PICC_HaltA(); // stops scanning from the rc255
      } 
    } 
  }
}
// works with function scan()
void dump_byte_array(byte *c) {
  x = c[0]; 
  loopval = false; // stops scan loop
}

// set led light to rgb value
void setColor(int redValue, int greenValue, int blueValue) 
{
  analogWrite(A2, redValue);
  analogWrite(A1, greenValue);
  analogWrite(A0, blueValue);
}

// an event that pauses the code untill the user presses the button signifying they are ready for the next question, also shows current statistics correct questions/total questions
void press2continue(int i, int j){
  lcd.clear();
  lcd.print("Performance: ");
  lcd.print(i);
  lcd.print("/");
  lcd.print(j);
  lcd.setCursor(0, 1);
  lcd.print("Push to Continue");
  while(!Continue){
    delay(1);
  }
  Continue = false;
  lcd.clear();
  delay(2000);
}

// call when button is pressed
void press(){
  Continue = true;
}

// scrolls through the inputed array on the lcd screen
void scroll(char arr[]){
  int text_length = strlen(arr)+1;
  if(Cursor == (text_length - 1)){
    Cursor = 0;
  }
  lcd.setCursor(0, line_num);
  if(Cursor < text_length - 16){
    for(int Char = Cursor; Char < Cursor + 16; Char++){
      lcd.print(arr[Char]);
    }
  }
  else{
    for(int Char = Cursor; Char < (text_length -1); Char++){
      lcd.print(arr[Char]);
    }
    for(int Char = 0; Char <= 16 - (text_length - Cursor); Char++){
      lcd.print(arr[Char]);
    }
  }
  Cursor ++;
}
