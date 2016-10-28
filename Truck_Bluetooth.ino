#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_MOSI  A0
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define SOLENOID0 6
#define SOLENOID1 7
#define SOLENOID2 8
#define COMPRESSOR 9
int Button[6] = {20, 21, 2, 3, 4, 5}; //A6 and A7
boolean Buttstatus[6] = {false, false, false, false, false, false};
unsigned long time1 = millis();
unsigned long time2 = time1 + 1000;
float psi1 = 0;
float psi2 = 0;

int maxSeconds = 5; // send status message every maxSeconds
volatile int seconds = 0;
volatile boolean statusReport = false;

String inputString = "";
String command = "";
String value = "";
boolean stringComplete = false;

void buttWrite0(){
  if(analogRead(Button[0]) <= 512)
    return;
    
  while(analogRead(Button[0]) > 512)
  {
    OLED();
    digitalWrite(SOLENOID1, HIGH);
    digitalWrite(SOLENOID2, HIGH);
    digitalWrite(COMPRESSOR, LOW);
    digitalWrite(SOLENOID0, LOW);
  }
  digitalWrite(COMPRESSOR, HIGH);
  digitalWrite(SOLENOID0, HIGH);
  return;
}
void buttWrite1(){
  if(analogRead(Button[1]) <= 512)
    return;
    
  while(analogRead(Button[1]) > 512)
  {  
    OLED();
    digitalWrite(SOLENOID0, HIGH);
    digitalWrite(SOLENOID2, HIGH);
    digitalWrite(COMPRESSOR, LOW);
    digitalWrite(SOLENOID1, LOW);
  }
  digitalWrite(COMPRESSOR, HIGH);
  digitalWrite(SOLENOID1, HIGH);
  return;
}
void buttWrite2(){
  if(digitalRead(Button[2]) == LOW)
    return;
  
  while(digitalRead(Button[2]) == HIGH)
  {
    OLED();
    digitalWrite(SOLENOID2, HIGH);
    digitalWrite(COMPRESSOR, LOW);
    digitalWrite(SOLENOID0, LOW);
    digitalWrite(SOLENOID1, LOW);
  }
  digitalWrite(COMPRESSOR, HIGH);
  digitalWrite(SOLENOID0, HIGH);
  digitalWrite(SOLENOID1, HIGH);
  return;
}
void buttWrite3(){
  if(digitalRead(Button[3]) == LOW)
    return;
    
  while(digitalRead(Button[3]) == HIGH)
  {
    OLED();
    digitalWrite(COMPRESSOR, HIGH);
    digitalWrite(SOLENOID0, LOW);
    digitalWrite(SOLENOID1, LOW);
    digitalWrite(SOLENOID2, LOW);
  }
  digitalWrite(SOLENOID0, HIGH);
  digitalWrite(SOLENOID1, HIGH);
  digitalWrite(SOLENOID2, HIGH);
  
  return;
}
void buttWrite4(){
  if(digitalRead(Button[4]) == LOW)
    return;
    
  while(digitalRead(Button[4]) == HIGH)
  {
    OLED();
    digitalWrite(SOLENOID1, HIGH);
    digitalWrite(COMPRESSOR, HIGH);
    digitalWrite(SOLENOID0, LOW);
    digitalWrite(SOLENOID2, LOW);
  }
  digitalWrite(SOLENOID0, HIGH);
  digitalWrite(SOLENOID2, HIGH);
  return;
}
void buttWrite5(){
  if(digitalRead(Button[5]) == LOW)
    return;
    
  while(digitalRead(Button[5]) == HIGH)
  {
    OLED();
    digitalWrite(SOLENOID0, HIGH);
    digitalWrite(COMPRESSOR, HIGH);
    digitalWrite(SOLENOID1, LOW);
    digitalWrite(SOLENOID2, LOW);
  }
  digitalWrite(SOLENOID1, HIGH);
  digitalWrite(SOLENOID2, HIGH);
  return;
}

void setup(){
  //start serial connection
  Serial.begin(9600);
  inputString.reserve(50);
  command.reserve(50);
  value.reserve(50);
  
  pinMode(Button[0], INPUT);
  pinMode(Button[1], INPUT);
  pinMode(Button[2], INPUT);
  pinMode(Button[3], INPUT);
  pinMode(Button[4], INPUT);
  pinMode(Button[5], INPUT);
  pinMode(A0, OUTPUT);
  pinMode(SOLENOID0, OUTPUT);
  pinMode(SOLENOID1, OUTPUT);
  pinMode(SOLENOID2, OUTPUT);
  pinMode(COMPRESSOR, OUTPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  digitalWrite(SOLENOID0, HIGH);
  digitalWrite(SOLENOID1, HIGH);
  digitalWrite(SOLENOID2, HIGH);
  digitalWrite(COMPRESSOR, HIGH);
  display.begin(SSD1306_SWITCHCAPVCC);
  display.display();
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  
  cli();          // disable global interrupts
  
  // initialize Timer1 for interrupt @ 1000 msec
  TCCR1A = 0;     // set entire TCCR1A register to 0
  TCCR1B = 0;     // same for TCCR1B
 
  // set compare match register to desired timer count:
  OCR1A = 15624;
  // turn on CTC mode:
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler:
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
  // enable timer compare interrupt:
  TIMSK1 |= (1 << OCIE1A);
  
  sei();          // enable global interrupts
}

// timer interrupt routine
ISR(TIMER1_COMPA_vect)
{
  if (seconds++ >= maxSeconds){
    statusReport = true;
    seconds = 0;
  }
}

// interpret and execute command when received
// then report status if flag raised by timer interrupt
void loop(){
  OLED();
  commandProcess();
  buttWrite0();
  buttWrite1();
  buttWrite2();
  buttWrite3();
  buttWrite4();
  buttWrite5();
  if (statusReport) {
    Serial.print("STATUS PSI 1= ");
    Serial.println(psi1);
    Serial.print("STATUS PSI 2= ");
    Serial.println(psi2);
    statusReport = false;
  }

}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read(); 
    inputString += inChar;
    if (inChar == '\n' || inChar == '\r') {
      stringComplete = true;
      commandProcess();
    } 
  }
}

void commandProcess(){
  if (stringComplete) {
    Serial.println(inputString);
    boolean stringOK = false;
    if (inputString.startsWith("CMD ")) {
      inputString = inputString.substring(4);
      int pos = inputString.indexOf('=');
      if (pos > -1) {
        command = inputString.substring(0, 1);
        String commandVal = inputString.substring(3, 4);
        value = inputString.substring(pos+1, inputString.length()-1);  // extract command up to \n exluded
        if (command.equals("0")){
          if(commandVal.equals("N")){
            Buttstatus[0] = true;
            digitalWrite(COMPRESSOR, LOW);
            digitalWrite(SOLENOID0, LOW);
            Serial.println("Button 0 on");
          }
          
          else{
            Buttstatus[0] = false;
            digitalWrite(COMPRESSOR, HIGH);
            digitalWrite(SOLENOID0, HIGH);
            Serial.println("Button 0 off");
          }
            
          Buttstatus[1] = false;
          Buttstatus[2] = false;
          Buttstatus[3] = false;
          Buttstatus[4] = false;
          Buttstatus[5] = false;
          stringOK = true;
        }
        else if (command.equals("1")) {
          Buttstatus[0] = false;
          if(commandVal.equals("N")){
            Buttstatus[1] = true;
            digitalWrite(COMPRESSOR, LOW);
            digitalWrite(SOLENOID1, LOW);
            Serial.println("Button 1 on");
          }
          
          else{
            Buttstatus[1] = false;
            digitalWrite(COMPRESSOR, HIGH);
            digitalWrite(SOLENOID1, HIGH);
            Serial.println("Button 1 off");
          }
          Buttstatus[2] = false;
          Buttstatus[3] = false;
          Buttstatus[4] = false;
          Buttstatus[5] = false;
          stringOK = true;
        }
        else if (command.equals("2")) {
          Buttstatus[0] = false;
          Buttstatus[1] = false;
          if(commandVal.equals("N")){
            Buttstatus[2] = true;
            digitalWrite(COMPRESSOR, LOW);
            digitalWrite(SOLENOID0, LOW);
            digitalWrite(SOLENOID1, LOW);
            Serial.println("Button 2 on");
          }
          
          else{
            Buttstatus[2] = false;
            digitalWrite(COMPRESSOR, HIGH);
            digitalWrite(SOLENOID0, HIGH);
            digitalWrite(SOLENOID1, HIGH);
            Serial.println("Button 2 off");
          }
          Buttstatus[3] = false;
          Buttstatus[4] = false;
          Buttstatus[5] = false;
          stringOK = true;
        }
        else if (command.equals("3")) {
          Buttstatus[0] = false;
          Buttstatus[1] = false;
          Buttstatus[2] = false;
          if(commandVal.equals("N")){
            Buttstatus[3] = true;
            digitalWrite(SOLENOID0, LOW);
            digitalWrite(SOLENOID1, LOW);
            digitalWrite(SOLENOID2, LOW);
            Serial.println("Button 3 on");
          }
          
          else{
            Buttstatus[3] = false;
            digitalWrite(SOLENOID0, HIGH);
            digitalWrite(SOLENOID1, HIGH);
            digitalWrite(SOLENOID2, HIGH);
            Serial.println("Button 3 off");
          }
          Buttstatus[4] = false;
          Buttstatus[5] = false;
          stringOK = true;
        }
        else if (command.equals("4")) {
          Buttstatus[0] = false;
          Buttstatus[1] = false;
          Buttstatus[2] = false;
          Buttstatus[3] = false;
          if(commandVal.equals("N")){
            Buttstatus[4] = true;
            digitalWrite(SOLENOID0, LOW);
            digitalWrite(SOLENOID2, LOW);
            Serial.println("Button 4 on");
          }
          
          else{
            Buttstatus[4] = false;
            digitalWrite(SOLENOID0, HIGH);
            digitalWrite(SOLENOID2, HIGH);
            Serial.println("Button 4 off");
          }
          Buttstatus[5] = false;
          stringOK = true;
        }
        else if (command.equals("5")) {
          Buttstatus[0] = false;
          Buttstatus[1] = false;
          Buttstatus[2] = false;
          Buttstatus[3] = false;
          Buttstatus[4] = false;
          if(commandVal.equals("N")){
            Buttstatus[5] = true;
            digitalWrite(SOLENOID1, LOW);
            digitalWrite(SOLENOID2, LOW);
            Serial.println("Button 5 on");
          }
          
          else{
            Buttstatus[5] = false;
            digitalWrite(SOLENOID1, HIGH);
            digitalWrite(SOLENOID2, HIGH);
            Serial.println("Button 5 off");
          }
          stringOK = true;
        }
      }
      else if (inputString.startsWith("STATUS")) {
        Serial.print("STATUS PSI1= ");
        Serial.println(psi1);
        Serial.print("STATUS PSI 2= ");
        Serial.println(psi2);
        stringOK = true;
      }
    }
    stringOK ? Serial.println("Command Executed") : Serial.println("Invalid Command");
    inputString = "";
    stringComplete = false;
  }
  else
    return;
}

void OLED(){ //Drives the Oled Displays
  time1 = millis();
  if(time1 >= time2)
  {
    time2 = time1+300;
    int bag1 = analogRead(A1);
    int bag2 = analogRead(A2);
    int T1 = bag1 - 158;
    int T2 = bag2 - 158;
    psi1 = T1/7.9;
    psi2 = T2/7.9;
    if(psi1 < 0.0)
      psi1 = 0;
    if(psi2 < 0.0)
      psi2 = 0;
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.print("Bag 1:     ");display.println("Bag 2: ");
    display.setTextSize(2);
    display.setCursor(0,12);
    display.print(psi1,1);display.print(" ");display.print(psi2,1);
    display.display();
  }
  return;
}


