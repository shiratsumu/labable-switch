#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "secrets.h"

// Pin definitions
const int SWITCH_PIN = 14;
const int LED_PIN = 25;

// State tracking
int lastSwitchState = HIGH;
int currentSwitchState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Parsed URL components
String targetApiUrl = "";
String channelName = "";
String encryptedPostUrl = "";

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // LED off initially

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  
  // Set up NTP for accurate timestamps
  configTime(9 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync");
  time_t now = time(nullptr);
  int retryCount = 0;
  while (now < 8 * 3600 * 2 && retryCount < 20) { // Timeout after 10 seconds
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    retryCount++;
  }
  if (retryCount >= 20) {
    Serial.println("\nNTP sync timeout. Time might not be accurate.");
  } else {
    Serial.println("\nTime synchronized.");
  }

  // Parse the generated URL
  String urlStr = String(generatedUrl);
  int qMark = urlStr.indexOf('?');
  if (qMark != -1) {
    // Extract base URL and construct slack.php URL
    String baseUrl = urlStr.substring(0, qMark);
    if (!baseUrl.endsWith("/")) {
      baseUrl += "/";
    }
    targetApiUrl = baseUrl + "slack.php";

    // Extract channel
    int chStart = urlStr.indexOf("ch=") + 3;
    if (chStart >= 3) {
      int chEnd = urlStr.indexOf('&', chStart);
      if (chEnd == -1) chEnd = urlStr.length();
      channelName = urlStr.substring(chStart, chEnd);
    }

    // Extract posturl
    int postUrlStart = urlStr.indexOf("posturl=") + 8;
    if (postUrlStart >= 8) {
      int postUrlEnd = urlStr.indexOf('&', postUrlStart);
      if (postUrlEnd == -1) postUrlEnd = urlStr.length();
      encryptedPostUrl = urlStr.substring(postUrlStart, postUrlEnd);
    }
    
    Serial.println("--- Parsed URL Configuration ---");
    Serial.println("Target API: " + targetApiUrl);
    Serial.println("Channel: " + channelName);
    Serial.println("Encrypted Webhook length: " + String(encryptedPostUrl.length()));
    Serial.println("--------------------------------");
  } else {
    Serial.println("Error: generatedUrl does not contain '?' parameters. Please check secrets.h");
  }

  // Initialize state based on switch at startup
  currentSwitchState = digitalRead(SWITCH_PIN);
  lastSwitchState = currentSwitchState;
  
  // Set LED according to initial switch state (LOW means ON due to pullup)
  if (currentSwitchState == LOW) {
    digitalWrite(LED_PIN, HIGH);
  }
}

void loop() {
  // Read switch state
  int reading = digitalRead(SWITCH_PIN);

  // Check for state change (noise/debounce)
  if (reading != lastSwitchState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If state has been stable longer than debounce delay
    if (reading != currentSwitchState) {
      currentSwitchState = reading;

      // Handle the new state
      if (currentSwitchState == LOW) { // Switch turned ON
        Serial.println("Switch is ON - User Entered");
        digitalWrite(LED_PIN, HIGH);
        sendRequest("in");
      } else { // Switch turned OFF
        Serial.println("Switch is OFF - User Exited");
        digitalWrite(LED_PIN, LOW);
        sendRequest("out");
      }
    }
  }

  lastSwitchState = reading;
}

void sendRequest(String inOutAction) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot send request.");
    return;
  }
  
  if (targetApiUrl == "") {
    Serial.println("Error: target API URL not parsed successfully.");
    return;
  }

  HTTPClient http;
  WiFiClient* client = nullptr;
  
  Serial.print("Sending POST request to: ");
  Serial.println(targetApiUrl);
  
  if (targetApiUrl.startsWith("https")) {
    WiFiClientSecure* secureClient = new WiFiClientSecure();
    secureClient->setInsecure(); // SSL証明書の検証をスキップしてHTTPS接続を許可
    client = secureClient;
  } else {
    client = new WiFiClient();
  }
  
  http.begin(*client, targetApiUrl);
  http.addHeader("Content-Type", "application/json");

  // Construct JSON payload
  // Requirements: name, channel, icon_emoji, text, posturl, inout. For "in" requires timestamp
  
  time_t now = time(nullptr);
  unsigned long timestamp = static_cast<unsigned long>(now);
  
  String payload = "{";
  payload += "\"name\":\"" + String(userName) + "\",";
  payload += "\"channel\":\"" + channelName + "\",";
  payload += "\"icon_emoji\":\"" + String(iconEmoji) + "\",";
  payload += "\"text\":\"\",";  // テキストメッセージ（ブラウザ同様空にしないと名前のように出てしまう）
  payload += "\"posturl\":\"" + encryptedPostUrl + "\",";
  payload += "\"inout\":\"" + inOutAction + "\"";
  
  if (inOutAction == "in") {
    payload += ",\"timestamp\":" + String(timestamp);
  }
  payload += "}";

  Serial.println("Payload: " + payload);

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  
  if (client != nullptr) {
    delete client;
  }
}
