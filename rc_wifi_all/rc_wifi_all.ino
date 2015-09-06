/*
Bibli LED Matrix and iCreate Movement Control
Author: Qi LIU, qliu.hit@gmail.com
Date: 05/2015

Change the parameters below to work with your wifi
ssid[]: wifi network name
pass[]: wifi password
*/

#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library

/*************************WIFI Shield********************************/
//Static IP, update to assigned IP
IPAddress ip(192,168,43,100);

int status = WL_IDLE_STATUS;
char ssid[] = "Bibli"; // your network SSID (name) Your network name here, case sensitive!
char pass[] = "comiccon";  // if your network doesn't use WPA or WEP, change the line below that says "status = WiFi.begin(ssid, pass);"
unsigned int localPort = 2390; // local port to listen on

char packetBuffer[25]; //buffer to hold incoming packet
byte buffersize;
char  ReplyBuffer[] = "acknowledged"; // a string to send back

WiFiUDP Udp;
int counter = 0;

//Serial to iCreate
#define rxPin 48//5 , 48/49 with mega
#define txPin 49//6

// set up a new software serial port:
SoftwareSerial softSerial =  SoftwareSerial(rxPin, txPin);

/**************************LED Matrix********************************/
//PINs for LED matrix
#define CLK 11  // MUST be on PORTB, and 50-53 is for SCI with Wifi
#define LAT 10
#define OE  9
#define A   A3//A0
#define B   A4//A1
#define C   A5//A2
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, true);

//For spectrum analyzer shield, these three pins are used.
//You can move pinds 40 and 41, but you must cut the trace on the shield and re-route from the 2 jumpers. 
int spectrumReset=6;
int spectrumStrobe=4;
int spectrumAnalog=0;  //0 for left channel, 1 for right.

// Spectrum analyzer read values will be kept here.
int Spectrum[7];
int Barsize_au[16];
int Barsize_op[16];

int Band = 0, BarMax = 15, MaxLevel = 0, works = 0;

//Color scheme
int BLACK = matrix.Color333(0,0,0);
int GREEN = matrix.Color333(0,0, 3);
int ORANGE = matrix.Color333(3,3,0);
int RED = matrix.Color333(3,0,0);

int bt1 = 5;
int bt2 = 5;

//text scrolling
int    textX   = matrix.width();
int textMin = sizeof(packetBuffer) * -12;
long   hue     = 0;

int8_t ball[3][4] = {
  {  3,  0,  1,  1 }, // Initial X,Y pos & velocity for 3 bouncy balls
  { 17, 15,  1, -1 },
  { 27,  4, -1,  1 }
};

static const uint16_t PROGMEM ballcolor[3] = {
  0x0080, // Green=1
  0x0002, // Blue=1
  0x1000  // Red=1
};
/**************************Set-up************************************/
void setup() {
 /***step 0 - init LED matrix***/
  matrix.begin();
  matrix.setTextWrap(false); // Allow text to run off right edge
  //for debug
  draw_logo();
  show_status(0);//0-init wifi, 1-wifi init done, 2-
  
 /***step 1 - set up wifi connection***/
 //Initialize serial and wait for port to open:
 Serial.begin(115200); 
 Serial.println("WifiUdpSendReceiveString");

 // check for the presence of the shield:
 if (WiFi.status() == WL_NO_SHIELD) {
   Serial.println("WiFi shield not present"); 
   // don't continue:
   while(true);
 } 
 Serial.println("Wifi Shield present");
 
 //Config to use static IP
 
 WiFi.config(ip);
 
 // attempt to connect to Wifi network:
 while ( status != WL_CONNECTED) { 
   Serial.print("Attempting to connect to SSID: ");
   Serial.println(ssid);
   // Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
   status = WiFi.begin(ssid, pass); 

   // wait 10 seconds for connection:
   delay(10000);
 } 
 Serial.println("Connected to wifi");
 printWifiStatus();

 Serial.println("\nStarting connection to server...");
 Udp.begin(localPort);  
 
 //step 2-setup serial commnication with create
 // set the data rate for the SoftwareSerial port, this is the
 // iRobot's default rae of 57600 baud:
  softSerial.begin(57600);

  softSerial.write(128); // This command starts the OI. You must always send the Start command before sending any other commands to the OI
  softSerial.write(131); // safe mode 
  
 //step 3-step up spectrum shield
 //Spectrum
  byte Counter;
  //Setup pins to drive the spectrum analyzer. 
  pinMode(spectrumReset, OUTPUT);
  pinMode(spectrumStrobe, OUTPUT);

  //Init spectrum analyzer
  digitalWrite(spectrumStrobe,LOW);
    delay(1);
  digitalWrite(spectrumReset,HIGH);
    delay(1);
  digitalWrite(spectrumStrobe,HIGH);
    delay(1);
  digitalWrite(spectrumStrobe,LOW);
    delay(1);
  digitalWrite(spectrumReset,LOW);
    delay(5);
  // Reading the analyzer now will read the lowest frequency.
}

void loop() {
 // task 1 spectrum from mic
 showSpectrum();
 //scrolling text testing
 if (packetBuffer[0] == 'M') 
  {
      //grab message
      show_msg(25);
      Serial.println("user text message");
      Serial.println(packetBuffer);
  }
 
 //task 2 - message handling if there's data available, read a packet
 int packetSize = Udp.parsePacket();
 if(packetSize)
 {   
   //Serial.print("Received packet of size ");
   //Serial.println(packetSize);
   //Serial.print("From ");
   IPAddress remoteIp = Udp.remoteIP();
   //Serial.print(remoteIp);
   //Serial.print(", port ");
   //Serial.println(Udp.remotePort());

   // read the packet into packetBufffer
   int len = Udp.read(packetBuffer,255);
   if (len >0) packetBuffer[len]=0;
   //p-audio message 
   if (packetBuffer[0] == 'p') 
   {
      //grab all 16 bands, testing, need update when > 10
      int k = 0;
      for (int i = 1; i<34; i=i+2){
        //Serial.println(packetBuffer[i]);
        Barsize_op[k]  = 1 + packetBuffer[i] - '0';
        k++;
        //Serial.println(Barsize_op[k++]);
      }
   }
   //user input text for scrolling display on LED
   if (packetBuffer[0] == 'M') 
   {
      //grab message
      Serial.println("user text message");
      Serial.println(packetBuffer);
   }
   
   //f,b,l,r,s->ctrl command
   if (packetBuffer[0] == 'f') 
   {
     goForward();
     Serial.println("forward");
   }
   
   if (packetBuffer[0] == 'b') 
   {
     back();
     Serial.println("back");
   }
   
   if (packetBuffer[0] == 'l') 
   {
     leftTurn();
     Serial.println("left");
   }
   
   if (packetBuffer[0] == 'r') 
   {
     rightTurn();
     Serial.println("right");
   }
   
   if (packetBuffer[0] == 's') 
   {
     rstop();
     Serial.println("stop");
   }
   
   // send a reply, to the IP address and port that sent us the packet we received
   Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
   Udp.write(ReplyBuffer);
   Udp.endPacket();
 }
 else{
// this will show you that the sketch is actually running and not crashing
// remove this, it's useless.
   counter ++;
   if(counter > 6000){
     counter = 0;
     rstop(); 
   }
 }
}

//wifi status
void printWifiStatus() {
 // print the SSID of the network you're attached to:
 Serial.print("SSID: ");
 Serial.println(WiFi.SSID());

 // print your WiFi shield's IP address:
 IPAddress ip = WiFi.localIP();
 Serial.print("IP Address: ");
 Serial.println(ip);

 // print the received signal strength:
 long rssi = WiFi.RSSI();
 Serial.print("signal strength (RSSI):");
 Serial.print(rssi);
 Serial.println(" dBm");
}

//Robot control
void goForward() {
  softSerial.write(145);
  softSerial.write((byte)0);
  softSerial.write((byte)200);
  softSerial.write((byte)0);
  softSerial.write((byte)200);
}

void rightTurn() {
  softSerial.write(145);
  softSerial.write((byte)255);
  softSerial.write((byte)56);
  softSerial.write((byte)0);
  softSerial.write((byte)200);
}

void leftTurn() {
  softSerial.write(145);
  softSerial.write((byte)0);
  softSerial.write((byte)200);
  softSerial.write((byte)255);
  softSerial.write((byte)56);
}
 
void back() {
  softSerial.write(145);
  softSerial.write((byte)255);
  softSerial.write((byte)56);
  softSerial.write((byte)255);
  softSerial.write((byte)56);
}

void rstop() {
  softSerial.write(145);
  softSerial.write((byte)0);
  softSerial.write((byte)0);
  softSerial.write((byte)0);
  softSerial.write((byte)0);
}

//LED Matrix show system status
//0-init wifi, 1-wifi init done, 2-
void show_status(byte st){
 char* myStrings[]={"Setup", "Wifi", "Done", "BiBli"};
 // Clear background
 matrix.fillScreen(0);
 switch (st) {
    case 0://setup wifi
      // draw some text!
      matrix.setCursor(1, 1);   // start at top left, with one pixel of spacing
      matrix.setTextSize(1);    // size 1 == 8 pixels high
      for (int i = 0; i < 5; i++){
        matrix.setTextColor(matrix.Color333(0,0,bt1));
        matrix.print(myStrings[0][i]);
      }
      
      matrix.setCursor(1, 9);   // start at top left, with one pixel of spacing
      matrix.setTextSize(1);    // size 1 == 8 pixels high
      for (int i = 0; i < 4; i++){
        matrix.setTextColor(matrix.Color333(0,0,bt1));
        matrix.print(myStrings[1][i]);
      }
      break;
  }
 matrix.swapBuffers(false);   // Update display
}

//
//draw when power up
void draw_logo(){
// Clear background
  matrix.fillScreen(0);
  // draw some text!
  matrix.setCursor(1, 4);   // start at top left, with one pixel of spacing
  matrix.setTextSize(1);    // size 1 == 8 pixels high
 
//  // print each letter with a rainbow color
  matrix.setTextColor(matrix.Color333(0,2,6));
  matrix.print('B');
  
  matrix.setTextColor(matrix.Color333(bt1,0,0));
  matrix.print('i');
  
  matrix.setTextColor(matrix.Color333(0,2,6));
  matrix.print('B');
  
  matrix.setTextColor(matrix.Color333(bt1,0,0));
  matrix.print('l');
  
  matrix.setTextColor(matrix.Color333(bt1,0,0));
  matrix.print('i');
  
  matrix.swapBuffers(false);   // Update display
  delay(2000);

  matrix.swapBuffers(false); 
}

// Read 7 band equalizer.
void readSpectrum()
{
  // Band 0 = Lowest Frequencies.
  byte Band;
  for(Band=0;Band <7; Band++)
  {
    Spectrum[Band] = analogRead(spectrumAnalog);// + analogRead(spectrumAnalog) ) >>1; //Read twice and take the average by dividing by 2
    digitalWrite(spectrumStrobe,HIGH);
    digitalWrite(spectrumStrobe,LOW);     
  }
}


void showSpectrum()
{
  readSpectrum();
//  show_values();
//   byte Band, BarSize, MaxLevel;
   static unsigned int  Divisor = 20, ChangeTimer=0; 
//   unsigned int works, Remainder; 
  for(Band=0;Band<7;Band++)
  {
  //If value is 0, we don;t show anything on graph
     works = Spectrum[Band]/Divisor;    //Bands are read in as 10 bit values. Scale them down to be 0 - 15
     if(works > MaxLevel)  //Check if this value is the largest so far.
       MaxLevel = works;
      if( works >= BarMax) {
        Barsize_au[Band] = BarMax;
      }
      else {
        Barsize_au[Band] = works;
      }

  }
  show_spectrum(1);//start display
  //Adjust Divisor
//  if (MaxLevel >= 15)
//  {
//    Divisor=Divisor+1;
//  }
//  else
//    if(MaxLevel < 14)
//    {
//      if(Divisor > 65)
//        if(ChangeTimer++ > 20)
//        {
//          Divisor--;
//          ChangeTimer=0;
//        }
//    }
//    else
//    {
//      ChangeTimer=0; 
//    }
  }

//scrolling text message
void show_msg(int packesize)
{
  byte i;
  // Clear background
  matrix.fillScreen(0);

  // Bounce three balls around
  for(i=0; i<3; i++) {
    // Draw 'ball'
    matrix.fillCircle(ball[i][0], ball[i][1], 5, pgm_read_word(&ballcolor[i]));
    // Update X, Y position
    ball[i][0] += ball[i][2];
    ball[i][1] += ball[i][3];
    // Bounce off edges
    if((ball[i][0] == 0) || (ball[i][0] == (matrix.width() - 1)))
      ball[i][2] *= -1;
    if((ball[i][1] == 0) || (ball[i][1] == (matrix.height() - 1)))
      ball[i][3] *= -1;
  }

  // Draw big scrolly text on top
  matrix.setTextColor(matrix.ColorHSV(hue, 255, 255, true));
  matrix.setCursor(textX, 1);
  matrix.print(packetBuffer);

  // Move text left (w/wrap), increase hue
  if((--textX) < textMin) textX = matrix.width();
  hue += 7;

  // Update display
  matrix.swapBuffers(false);

}

//draw spectrum
void show_spectrum(int user){
  // Clear background
  matrix.fillScreen(0);
  int j = 0;
  int barlen = 0;
  int Barsize[16];
  if (user == 1){ //audience
    barlen = 7;
    //memcpy( Barsize, Barsize_au, 16);
  }
  
  if (user == 2) {//operator
    barlen = 16;
    //memcpy( Barsize, Barsize_op, 16);
  }
  for(j=0;j<barlen;j++)
  {
    //Serial.println(Barsize_au[j]);
    switch (Barsize_au[j]) {
          case 0:
            matrix.drawLine(2 * j, 0, 2 * j, 15, BLACK);
            break;
          case 1:
            matrix.drawLine(2 * j, 0, 2 * j, 15, BLACK);
            matrix.drawPixel(2 * j, 15, GREEN);
            break;
          case 2:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 14, GREEN);
            break;
          case 3:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 13, GREEN);
            break;
          case 4:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 12, GREEN);
            break;
          case 5:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 11, GREEN);
            break;
          case 6:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 10, GREEN);
            break;
          case 7:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 9, GREEN);
            break;
          case 8:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 8, GREEN);
            break;
          case 9:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 7, GREEN);
            break;
          case 10:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 6, GREEN);
            break;
          case 11:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 5, GREEN);
            break;
          case 12:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 4, ORANGE);
            matrix.drawLine(2 * j, 15, 2 * j, 5, GREEN);
            break;
          case 13:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 3, ORANGE);
            matrix.drawLine(2 * j, 15, 2 * j, 5, GREEN);
            break;
          case 14:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 2, ORANGE);
            matrix.drawLine(2 * j, 15, 2 * j, 5, GREEN);
            break;
          case 15:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 1, RED);
            matrix.drawLine(2 * j, 15, 2 * j, 2, ORANGE);
            matrix.drawLine(2 * j, 15, 2 * j, 5, GREEN);
            break;
          case 16:
            matrix.drawLine(2 * j, 15, 2 * j, 0, BLACK);
            matrix.drawLine(2 * j, 15, 2 * j, 0, RED);
            matrix.drawLine(2 * j, 15, 2 * j, 2, ORANGE);
            matrix.drawLine(2 * j, 15, 2 * j, 5, GREEN);
            break;
  }
  }
  matrix.swapBuffers(false);   // Update display
}

//for debugging
void show_values()
{
  Serial.print("Current values - ");

  Serial.print("63HZ: ");
  Serial.print(Spectrum[0]);
  Serial.print(" 160HZ: ");
  Serial.print(Spectrum[1]);
  Serial.print(" 400HZ: ");
  Serial.print(Spectrum[2]);
  Serial.print(" 1kHZ: ");
  Serial.print(Spectrum[3]);
  Serial.print(" 2.5kHZ: ");
  Serial.print(Spectrum[4]);
  Serial.print(" 6.25kHZ: ");
  Serial.print(Spectrum[5]);
  Serial.print(" 16kHZ: ");
  Serial.print(Spectrum[6]);
  Serial.println(); 
}


