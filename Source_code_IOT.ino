
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>

void displayDataLCD();
float calculateConcentration(long, long);

float conPM25;
int co2Value;
const int buzzer = 7;
int ledPin = 7;
int PM25PIN = 7;

unsigned long durationPM25;
unsigned long starttime;
unsigned long endtime;
unsigned long sampletime_ms = 15000;
unsigned long lowpulseoccupancyPM25 = 0;

const int rs = 9, en = 8, d4 = 5, d5 = 6, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

//set interval for sending messages (milliseconds)
const long interval = 1000;
unsigned long previousMillis = 0;

const char broker[] = "test.mosquitto.org";

void setup() {

  lcd.begin(16, 2);
  pinMode(buzzer, OUTPUT);
  pinMode(ledPin, OUTPUT); 
  pinMode(PM25PIN,INPUT);
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("MQTT Arduino: ");

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  }    
  // print your local IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());

  delay(1500);
  
  mqttClient.setServer( broker, 1883);

  // Attempt to connect to the server with the ID "myClientID"
  if (mqttClient.connect("ID13")) 
  {
    Serial.println("Connection has been established, well done");
 
    // Establish the subscribe event
//     mqttClient.setCallback(subscribeReceive);
  } 
  else 
  {
    Serial.println("Looks like the server connection failed...");
    Serial.println(mqttClient.state());
    while(1);
  }
  
}


void loop() {

  
  // call poll() regularly to allow the library to send MQTT keep alive which
  // avoids being disconnected by the broker
  mqttClient.loop();

  displayDataLCD();

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time a message was sent
    previousMillis = currentMillis;
    
//    char msg_out[20];
//    sprintf(msg_out, "%d",co2Value); 

    String msgCO2 = "CO2 concentration is " + String(co2Value) + " ppm.";
    char msgCO2AsCharArray[msgCO2.length()];
    msgCO2.toCharArray(msgCO2AsCharArray, msgCO2.length());

    String msgPM25 = "PM25 concentration is " + String(conPM25) + " ug/m3.";
    char msgPM25AsCharArray[msgPM25.length()];
    msgPM25.toCharArray(msgPM25AsCharArray, msgPM25.length());

    Serial.println("Publishing values to broker");
    // Attempt to publish a value to the topic "MakerIOTopic"
    if(mqttClient.publish("CO2concentration", msgCO2AsCharArray))
    {
      Serial.println("Publish message success");
    }
    else
    {
      Serial.println("Could not send message :(");
    }

    if(mqttClient.publish("PM25concentration", msgPM25AsCharArray))
    {
      Serial.println("Publish message success");
    }
    else
    {
      Serial.println("Could not send message :(");
    }
    
  }
}

void displayDataLCD()
{
  co2Value = analogRead(0);
  lcd.print("Conc.of ");
  lcd.setCursor(0, 1);
  lcd.print("Co2 is ");
  lcd.print(co2Value, DEC);
  lcd.print(" ppm");

  if(co2Value > 30) {
     digitalWrite(ledPin, HIGH);
     tone(buzzer, 1000); 
     delay(1000);
     digitalWrite(ledPin, LOW);
     noTone(buzzer);
     delay(1000);
  }

  starttime = millis();
  while ((millis() - starttime) < sampletime_ms){
    durationPM25 = pulseIn(PM25PIN, LOW);
    lowpulseoccupancyPM25 += durationPM25;
  
  }  
  mqttClient.loop();

   //Only after 30s has passed we calcualte the ratio
  
    conPM25 = calculateConcentration(lowpulseoccupancyPM25,15);
    lowpulseoccupancyPM25 = 0;

    int conPM25int = int(conPM25*100);
    lcd.clear(); //clears c02 value
    lcd.print("PM2.5 CONC. ");
    lcd.setCursor(0, 1);
    lcd.print("is ");
    lcd.print(conPM25int/100,DEC);
    lcd.print(".");
    lcd.print(conPM25int%100, DEC);
    lcd.print(" ug/m3");
    delay(10000);
    lcd.clear();
}

float calculateConcentration(long lowpulseInMicroSeconds, long durationinSeconds){
  
  float ratio = (lowpulseInMicroSeconds/1000000.0)/15.0*100.0; //Calculate the ratio
  float concentration = 0.001915 * pow(ratio,2) + 0.09522 * ratio - 0.04884;//Calculate the mg/m3
  Serial.print("lowpulseoccupancy:");
  Serial.print(lowpulseInMicroSeconds);
  Serial.print("    ratio:");
  Serial.print(ratio);
  Serial.print("    Concentration:");
  Serial.println(concentration);
  return concentration;
}

  
//void subscribeReceive(char* topic, byte* payload, unsigned int length)
//{
//  // Print the topic
//  Serial.print("Topic: ");
//  Serial.println(topic);
// 
//  // Print the message
//  Serial.print("Message: ");
//  for(int i = 0; i < length; i ++)
//  {
//    Serial.print(char(payload[i]));
//  }
// 
//  // Print a newline
//  Serial.println("");
//}  
