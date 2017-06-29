//chrispus kamau
//ckn.io
//
//Requires PubSubClient found here: https://github.com/knolleary/pubsubclient
//
//ESP8266 Simple OLED display of MQTT messages

#include <Wire.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

//EDIT THESE LINES TO MATCH YOUR SETUP
#define MQTT_SERVER "192.168.43.250"
char* ssid = "subzero";
char* password = "@#$123456";

//WiFi scan properties buffers
char ip_buffer[20];                
char rssi_buffer[4]; 
char essid_buffer[20];

//MQTT subscription variables
char* eventTopic = "/test/event";
char event_buffer[16] = "Event: AH2017";
char speaker_buffer[16] = "Speaker: Wait";
char talk_buffer[16] = "Title: Wait";

//OLED parameters
#define OLED_address  0x3c

//generate unique name from MAC addr
String macToStr(const uint8_t* mac){

  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    }
  }

  return result;
}

//WiFi Scanning function
void scanWifi(){
  bool found = false;

  //set to station mode and disconnect if previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("scan start");

  int n = WiFi.scanNetworks();
  Serial.println("scan done");

  if (n == 0) {
    Serial.println("no networks found");
    clear_display();
    sendStrXY("  No networks  ", 2, 1);
    sendStrXY("    found!    ", 3, 1);
    sendStrXY("  Scanning...", 6, 1);
  }
  else {
    clear_display();
    sendStrXY("**WIFI SCANNER**", 0, 0);
    Serial.print(n);
    Serial.println(" networks found");

    for (int i = 0; (i < n) && !found; i++) {
      Serial.print(i + 1);
      Serial.print(": ");

            
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");

      String essid = WiFi.SSID(i);
      essid.toCharArray(essid_buffer, 20);
      sendStrXY((essid_buffer), i+1, 0);
      
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.print((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "* ");
      delay(10);
    }
  }
}

//WiFiconnection info

void connection() {
  //print out some more debug once connected
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    //Prep IP address for printing
    IPAddress ip = WiFi.localIP();   
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    ipStr.toCharArray(ip_buffer, 20);

    //Prep rssi for printing
    long rssi = WiFi.RSSI();
    String strRssi;
    strRssi=String(rssi);
    strRssi.toCharArray(rssi_buffer,4);

    clear_display();
    sendStrXY("Wireless Details", 0, 0);

    sendStrXY("ssid:", 2, 1);
    sendStrXY((ssid), 2, 6);

    sendStrXY("power:", 3, 1);
    sendStrXY((rssi_buffer), 3, 8);
    sendStrXY("dBm",3,11),
  
    sendStrXY("IP Address:", 5, 1);
    sendStrXY((ip_buffer), 6, 1);
  }
}

//No wifi connection display
void no_connection() {
  clear_display();
  sendStrXY(" Not connected", 2, 1);
  sendStrXY("to WiFi network", 3, 1);
  sendStrXY("Is ", 5, 0);
  sendStrXY((ssid), 5, 4);
  sendStrXY("available?", 6, 0);
}

WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);


void reconnect() {

  //loop while we wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    
    no_connection();
    delay(5000);
    
    scanWifi();
    
    //start wifi subsystem
    Serial.print("Connecting to ");
    Serial.println(ssid);
    Serial.print(".");
    WiFi.begin(ssid, password);
    delay(10000);  
  }
  
  connection();

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  // Loop until we're reconnected to the MQTT server
  while (!client.connected() && WiFi.status() == WL_CONNECTED) {
    Serial.print("Attempting MQTT connection...");
    // Generate client name based on MAC address and last 8 bits of microsecond counter
    String clientName;
    clientName += "esp8266-";
    uint8_t mac[6];
    WiFi.macAddress(mac);
    clientName += macToStr(mac);
    
    //if connected, subscribe to the topic(s) we want to be notified about
    //delay(5000);
    if (client.connect((char*) clientName.c_str())) {
      Serial.print("\tMTQQ Connected");
      clear_display();
      sendStrXY((event_buffer), 0, 1);
      sendStrXY((speaker_buffer), 3, 1);
      sendStrXY((talk_buffer), 6, 1); 
      client.subscribe(eventTopic);
    }
    //otherwise print failed for debugging
    else {
      Serial.println("\tFailed.");
      clear_display();
      sendStrXY("Attempting MQTT", 2, 0);
      sendStrXY("connection...", 3, 0);
      sendStrXY("     Failed!     ", 5, 0);
      delay(5000);
      connection();
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  String topicStr = topic; 
  String eventStr;
  String speakerStr;
  String talkStr;

  for (int i = 1; i < 18; i++) {
    eventStr += (char)payload[i];
  }

  for (int i = 19; i < 36; i++) {
    speakerStr += (char)payload[i];
  }

  for (int i = 37; i < 54; i++) {
    talkStr += (char)payload[i];
  }
  
  Serial.println();

  //Print out some debugging info
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);
  Serial.println(eventStr);
  Serial.println(speakerStr);
  Serial.println(talkStr);
  
  
  if((char)payload[0] == '['){
    clear_display();
    eventStr.toCharArray(event_buffer,16);
    speakerStr.toCharArray(speaker_buffer,16);
    talkStr.toCharArray(talk_buffer,16);
    sendStrXY((event_buffer), 0, 1);
    sendStrXY((speaker_buffer), 3, 1);
    sendStrXY((talk_buffer), 6, 1);
    client.publish("/test/confirm", "Event updated");
  }
}

void setup() {
  //start the serial line for debugging
  Serial.begin(115200);
  
  Wire.pins(0, 2);
  Wire.begin();                
   
  Serial.println("Initialising OLED...");
  StartUp_OLED();
  clear_display();
  Draw_Logo();
  delay(5000);
  clear_display();
  sendStrXY(" Africahackon ", 0, 1); // 16 characters per line
  sendStrXY(" Conference ", 2, 1);
  sendStrXY(" 2017 Badge  ", 4, 1);
  delay(2000);
  Serial.println("setup done");


  //scan for WiFi networks
  scanWifi();
  delay(5000);
  
  //attempt to connect to the WIFI network and then connect to the MQTT server
  WiFi.begin(ssid, password);
  delay(5000);
  
  reconnect();

  //wait a bit before starting the main loop
  delay(5000);
}

void loop(){

  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() != WL_CONNECTED) { reconnect(); }
  
  //maintain MQTT connection
  client.loop(); 

  clear_display();
  sendStrXY((event_buffer), 0, 1);
  sendStrXY((speaker_buffer), 3, 1);
  sendStrXY((talk_buffer), 6, 1); 

  delay(10000);
  
  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10); 
}











