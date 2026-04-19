#ifndef SECRETS_H
#define SECRETS_H

// Wi-Fi Configuration
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Labable URL Configuration
// index.htmlで発行され、ブラウザで実際に開いているURLをそのまま貼り付けてください。
// 例: "http://192.168.1.10:8000/?ch=research-lab&posturl=xxxxxxxxx..."
const char* generatedUrl = "YOUR_GENERATED_URL_HERE";

// User Configuration
const char* userName = "YOUR_NAME"; // 通知に表示されるあなたの名前
const char* iconEmoji = ":robot_face:"; // Slackのアイコン（例: :beginner: や :smile: など）

#endif // SECRETS_H
