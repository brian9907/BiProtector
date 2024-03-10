#include <ESP32Servo.h>
#include <SoftwareSerial.h>
#include <WiFi.h>

//핀 상수
#define LEDR      13
#define LEDG      12
#define LEDB      14
#define DETECTOR  27
#define GPS_TX    16
#define GPS_RX    17
#define SERVO     25
#define TILT      26
#define BUZZ      5

//제어 상수
#define CONNECTION_TIMEOUT  1000
#define SERVO_OPEN          180
#define SERVO_CLOSE         0
#define STEPS               2037
#define TILT_TIMEOUT        1000
#define TILT_THRESHOLD      200

//상태 상수
#define OPENED    0
#define LOCKED    1
#define DETECTED  2

const char* ssid     = "<WiFi SSID>";
const char* password = "<WiFi PW>";

String str = "";
String currentTime, currentMin, currentSec;
String latitude, latD, latM, longitude, longD, longM;
double latitude2, longitude2, latS, longS;
double lastlat = 37.557880, lastlon = 126.998541;

int state = LOCKED;
int tiltCount = 0;
long tstime = 0, tetime = 0;
int preState = LOW;

WiFiServer server(80);
SoftwareSerial gpsSerial(GPS_TX, GPS_RX);
Servo sv;

void digitalRGB(int r, int g, int b) {
  digitalWrite(LEDR, r);
  digitalWrite(LEDG, g);
  digitalWrite(LEDB, b);
}

void getGPS()
{
  char r = gpsSerial.read();
  str += r;
  if (r == '\n') {
    int index = str.indexOf("GPGGA"); //위치 line
    if (index != -1) {
      int first = str.indexOf(",");
      int two = str.indexOf(",", first + 1);
      int three = str.indexOf(",", two + 1);
      int four = str.indexOf(",", three + 1);
      int five = str.indexOf(",", four + 1);
      currentTime = str.substring(first+1, first+3).toInt() + 9;
      currentMin = str.substring(first+3, first+5).toInt();
      currentSec = str.substring(first+5, first+7).toInt();
      String t = currentTime + "h " + currentMin + "m " + currentSec + "s";
      Serial.println(t);
      Serial.println(str);
      latitude = str.substring(two+1, three);
      longitude = str.substring(four+1, five);
      latD = latitude.substring(0, 2);
      longD = longitude.substring(0, 3);
      latM = latitude.substring(2);
      longM = longitude.substring(3);
      latS = latM.toDouble() / 60;
      longS = longM.toDouble() / 60;
      latitude2 = latD.toDouble() + latS;
      longitude2 = longD.toDouble() + longS;
      if (latitude2 != 0) lastlat = latitude2;
      if (longitude2 != 0) lastlon = longitude2;
      Serial.print("lat: ");
      Serial.print(latitude2, 6);
      Serial.print(", lon: ");
      Serial.println(longitude2, 6);
      Serial.print("lastlat: ");
      Serial.print(lastlat, 6);
      Serial.print(", lastlon: ");
      Serial.println(lastlon, 6);
    }
    str = "";
  }
}

void check_server() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    long stime = millis(); //set timeout
    while (client.connected()) {
      long etime = millis();
      long tdiff = etime - stime;
      if (tdiff < 0 || tdiff >= CONNECTION_TIMEOUT) {
        Serial.println("Connection Timeout");
        break;
      }
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Access-Control-Allow-Origin: *");
            client.println();
            client.print(String(lastlat, 7));
            client.print(",");
            client.print(String(lastlon, 7));
            client.print(",");
            client.print(state);
            client.println();
            break;
          }
          else {
            currentLine = "";
          }
        }
        else if (c != '\r') {
          currentLine += c;
        }
        if (currentLine.endsWith("GET /O")) {
          sv.write(SERVO_OPEN);
          state = OPENED;
        }
        if (currentLine.endsWith("GET /C")) {
          sv.write(SERVO_CLOSE);
          state = LOCKED;
        }
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

void check_detected() {
  if (state == DETECTED) {
    digitalWrite(LEDR, HIGH);
    tone(BUZZ, 1800);
  }
  else {
    digitalWrite(LEDR, LOW);
    noTone(BUZZ);
  }
}

void check_tilt() {
  if (state != LOCKED) return;

  int tiltState = digitalRead(TILT);
  if (tiltState ^ preState) { //XOR: 서로 다르면 상태변화 감지
    tiltCount++;
  }
  preState = tiltState;
  
  tetime = millis();
  if (tetime - tstime >= TILT_TIMEOUT || tetime < tstime) {
    tiltCount = 0;
    tstime = millis();
  }
  
  if (tiltCount >= TILT_THRESHOLD) {
    state = DETECTED;
    tiltCount = 0;
  }
}

void setup()
{
    Serial.begin(9600);
    pinMode(LEDR, OUTPUT);      // set the LED pin mode
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
    pinMode(TILT, INPUT);
    pinMode(DETECTOR, INPUT_PULLUP);
    sv.attach(SERVO);
    gpsSerial.begin(9600);

    delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    sv.write(SERVO_CLOSE);
    state = LOCKED;

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        digitalRGB(HIGH, LOW, LOW);
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    digitalRGB(LOW, HIGH, LOW);
    server.begin();

    //♬♪♩~
    tone(BUZZ, 466);
    delay(200);
    tone(BUZZ, 932);
    delay(200);
    tone(BUZZ, 1865);
    delay(200);
    noTone(BUZZ);
}

void loop() {
  if (gpsSerial.available()) {
    getGPS();
  }

  if (state == LOCKED && digitalRead(DETECTOR) == HIGH) {
    state = DETECTED;
  }

  check_detected();

  check_tilt();

  check_server();
}
