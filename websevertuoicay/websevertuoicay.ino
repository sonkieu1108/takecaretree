/* Hệ Thống Tưới Cây Thông Minh ESP32 - WEB SERVER CÓ GIAO DIỆN CẢI TIẾN */
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#define BLYNK_PRINT Serial 

char ssid[] = "Kieu Son";
char pass[] = "123456789";

const int moistureSensorPin = 34;
const int pumpRelayPin = 2;
const int dhtPin = 4;

#define DHTTYPE DHT11
DHT dht(dhtPin, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int drySoilThreshold = 2630;
const int wetSoilThreshold = 1150;
const unsigned long wateringDuration = 5000;

int soilMoisturePercentage;
bool pumpState = false;
bool automaticMode = true;
float temperature = 0.0;
float humidity = 0.0;

WiFiServer server(80);

int getSoilMoisturePercentage() {
    int soilMoistureValue = analogRead(moistureSensorPin);
    return map(soilMoistureValue, drySoilThreshold, wetSoilThreshold, 0, 100);
}

void readDHTData() {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
}

void updateLCDReadings() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("D: "); lcd.print(soilMoisturePercentage); lcd.print("%");
    lcd.setCursor(8, 0);
    lcd.print(automaticMode ? "A" : "M");
    lcd.setCursor(0, 1);
    lcd.print("T: "); lcd.print(temperature); lcd.print("C H:"); lcd.print(humidity); lcd.print("%");
    lcd.setCursor(13, 0);
    lcd.print(pumpState ? "ON" : "OFF");
}

void controlPump(bool enablePump) {
    pumpState = enablePump;
    digitalWrite(pumpRelayPin, pumpState ? HIGH : LOW);
    updateLCDReadings();
}

void setup() {
    Serial.begin(115200);
    pinMode(pumpRelayPin, OUTPUT);
    digitalWrite(pumpRelayPin, LOW);
    dht.begin();
    lcd.init();
    lcd.noBacklight();

    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi Connected");
    server.begin();
}

void loop() {
    soilMoisturePercentage = getSoilMoisturePercentage();
    readDHTData();
    updateLCDReadings();
     if (automaticMode) {
        if (soilMoisturePercentage < 20 && !pumpState) {
            controlPump(true); // Bật bơm nếu độ ẩm dưới 20% và bơm đang tắt
        } else if (soilMoisturePercentage > 50 && pumpState) {
            controlPump(false); // Tắt bơm nếu độ ẩm trên 50% và bơm đang bật
        }
    }

    WiFiClient client = server.available();
    if (client) {
        String request = client.readStringUntil('\r');
        client.flush();
        
        if (request.indexOf("/toggleMode") >= 0) {
            automaticMode = !automaticMode;
        }
        if (request.indexOf("/togglePump") >= 0 && !automaticMode) {
            controlPump(!pumpState);
        }
        if (request.indexOf("/getData") >= 0) {
            String jsonData = "{\"soilMoisture\":" + String(soilMoisturePercentage) +
                              ", \"temperature\":" + String(temperature) +
                              ", \"humidity\":" + String(humidity) +
                              ", \"pumpState\":" + String(pumpState) +
                              ", \"automaticMode\":" + String(automaticMode) + "}";
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.println(jsonData);
            client.stop();
            return;
        }

        String htmlContent = "<!DOCTYPE html><html><head><title>Hệ Thống Tưới Cây</title><meta charset='UTF-8'><style>";
        htmlContent += "body{font-family:Arial;text-align:center;background:#f4f4f4;}";
        htmlContent += "h1{color:#333;}button{padding:15px 30px;margin:10px;font-size:18px;border:none;border-radius:5px;cursor:pointer;}";
        htmlContent += ".on{background:#28a745;color:white;}.off{background:#dc3545;color:white;}.mode{background:#007bff;color:white;}";
        htmlContent += "</style></head><body>";
        htmlContent += "<h1>Hệ Thống Tưới Cây Thông Minh</h1>";
        htmlContent += "<p>Chế độ: <b id='mode' style='color:" + String(automaticMode ? "blue" : "red") + "'>" + (automaticMode ? "TỰ ĐỘNG" : "THỦ CÔNG") + "</b></p>";
        htmlContent += "<p>Độ ẩm đất: <span id='soil'>" + String(soilMoisturePercentage) + "</span>%</p>";
        htmlContent += "<p>Nhiệt độ: <span id='temp'>" + String(temperature) + "</span>°C</p>";
        htmlContent += "<p>Độ ẩm không khí: <span id='hum'>" + String(humidity) + "</span>%</p>";
        htmlContent += "<p>Trạng thái bơm: <b id='pump' style='color:" + String(pumpState ? "green" : "red") + "'>" + (pumpState ? "BẬT" : "TẮT") + "</b></p>";
        htmlContent += "<button class='mode' onclick=\"window.location.href='/toggleMode'\">Chuyển chế độ</button>";
        htmlContent += "<button id='pumpBtn' class='" + String(pumpState ? "off" : "on") + "' onclick=\"window.location.href='/togglePump'\" " + String(automaticMode ? "disabled" : "") + ">" + (pumpState ? "Tắt Bơm" : "Bật Bơm") + "</button>";
        htmlContent += "<script>";
        htmlContent += "function updateData() {fetch('/getData').then(response => response.json()).then(data => {";
        htmlContent += "document.getElementById('soil').innerText = data.soilMoisture;";
        htmlContent += "document.getElementById('temp').innerText = data.temperature;";
        htmlContent += "document.getElementById('hum').innerText = data.humidity;";
        htmlContent += "document.getElementById('mode').innerText = data.automaticMode ? 'TỰ ĐỘNG' : 'THỦ CÔNG';";
        htmlContent += "document.getElementById('mode').style.color = data.automaticMode ? 'blue' : 'red';";
        htmlContent += "document.getElementById('pump').innerText = data.pumpState ? 'BẬT' : 'TẮT';";
        htmlContent += "document.getElementById('pump').style.color = data.pumpState ? 'green' : 'red';";
        htmlContent += "setTimeout(updateData, 2000);}).catch(error => console.error('Error:', error));}";
        htmlContent += "updateData();</script>";
        htmlContent += "</body></html>";
        
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();
        client.println(htmlContent);
        client.stop();
    }
    delay(1000);
}
