
#define BLYNK_TEMPLATE_ID "TMPL6joqqjaJU"
#define BLYNK_TEMPLATE_NAME "tudongtuoi"
#define BLYNK_AUTH_TOKEN "eRhG-3uVpGKg9x83UKyGWQeHzuepf32M"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// --- AceButton Library ---
#include <AceButton.h>
using namespace ace_button;

/* Điền thông tin Template ID và Template Name (nếu dùng Blynk.Cloud) */
//#define BLYNK_TEMPLATE_ID          "YOUR_TEMPLATE_ID"   // Điền Template ID của bạn vào đây // BỎ COMMENT ĐỂ TÙY CHỈNH
//#define BLYNK_TEMPLATE_NAME        "YOUR_TEMPLATE_NAME" // Điền Template Name của bạn vào đây // BỎ COMMENT ĐỂ TÙY CHỈNH
//#define BLYNK_AUTH_TOKEN             "YOUR_AUTH_TOKEN"    // Điền Auth Token của bạn vào đây // BỎ COMMENT ĐỂ TÙY CHỈNH

#define BLYNK_PRINT Serial // Vẫn giữ lại để in debug ra Serial Monitor nếu cần
//#define APP_DEBUG // Bạn có thể uncomment dòng này để bật debug app Blynk nếu cần

// --- Thông tin WiFi - ĐIỀN THÔNG TIN WIFI CỦA BẠN ---
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Kieu Son";          // Thay bằng SSID WiFi của bạn
char pass[] = "123456789";   // Điền mật khẩu WiFi của bạn vào đây

// --- Định nghĩa chân cảm biến và thiết bị (ESP32 Pins) - GIỮ NGUYÊN ---
const int moistureSensorPin = 34;          // GPIO34 - Analog pin cho cảm biến độ ẩm
const int pumpRelayPin = 2;                // GPIO2 - Digital pin cho relay bơm
const int dhtPin = 4;                      // GPIO4 - Digital pin cho DHT11

// --- Định nghĩa chân cho nút nhấn - SỬ DỤNG #define - GIỮ NGUYÊN ---
#define modeButtonPin 25             // GPIO25 - Digital pin cho nút chuyển chế độ AUTO/MANUAL
#define directPumpButtonPin 17        // GPIO17 - Digital pin cho nút BẬT/TẮT bơm trực tiếp (chế độ MANUAL)

#define DHTTYPE DHT11
const int lcdColumns = 16;
const int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// --- Định nghĩa ngưỡng và thời gian tưới - GIỮ NGUYÊN ---
const int drySoilThreshold = 2630;
const int wetSoilThreshold = 1150;
const int targetSoilMoisturePercentage = 80;   // Ngưỡng độ ẩm đất để ngừng bơm ở chế độ tự động
const unsigned long wateringDuration = 5000;

// --- Biến - GIỮ NGUYÊN ---
int soilMoistureValue;
int soilMoisturePercentage;
bool pumpState = false;                  // Trạng thái bơm (true=BẬT, false=TẮT)
unsigned long pumpStartTime = 0;
float temperature = 0.0;
float humidity = 0.0;
bool automaticMode = true;              // Chế độ mặc định LÀ TỰ ĐỘNG
bool directPumpControlButton = false;     // Biến trạng thái cho nút điều khiển bơm trực tiếp (chế độ MANUAL)

// --- AceButton cho nút chế độ - GIỮ NGUYÊN ---
ButtonConfig modeButtonConfig;
AceButton modeButton(&modeButtonConfig);

// --- **THÊM AceButton cho nút bơm trực tiếp** ---
ButtonConfig directPumpButtonConfig;
AceButton directPumpButton(&directPumpButtonConfig);

// --- Biến thời gian cho millis() - GIỮ NGUYÊN (nếu bạn đã tích hợp millis() trước đó) ---
unsigned long previousWateringTime = 0; // Thời điểm thực hiện logic tưới tự động lần trước
const long wateringInterval = 2000;     // Chu kỳ thực hiện logic tưới tự động (2 giây)

unsigned long previousLCDUpdateTime = 0;    // Thời điểm cập nhật LCD lần trước (tùy chọn)
const long lcdUpdateInterval = 1000;        // Chu kỳ cập nhật LCD (1 giây - tùy chọn, có thể điều chỉnh)

// --- Blynk Virtual Pins - KHÔI PHỤC và ĐIỀU CHỈNH VPIN CHO PHÙ HỢP VỚI DASHBOARD BLYNK CỦA BẠN ---
#define VPIN_SOIL_MOISTURE V0
#define VPIN_PUMP_STATUS        V1
#define VPIN_MANUAL_WATER      V2  // VPIN cho nút BẬT/TẮT bơm bằng tay trên Blynk // ĐÃ SỬA TÊN THÀNH VPIN_MANUAL_WATER
#define VPIN_TEMPERATURE        V3
#define VPIN_HUMIDITY           V4
#define VPIN_MODE_SWITCH         V5  // **ĐÃ ĐỔI VPIN_MODE_STATUS -> VPIN_MODE_SWITCH VÌ DÙNG SWITCH WIDGET**
#define VPIN_DIRECT_PUMP_BUTTON_STATUS V6 // VPIN hiển thị trạng thái nút bơm cơ trên Blynk

DHT dht(dhtPin, DHTTYPE);
BlynkTimer timer;


int getSoilMoisturePercentage() { // **ĐÃ SỬA LẠI HÀM getSoilMoisturePercentage() - ĐÚNG VỚI CODE GỐC**
    soilMoistureValue = analogRead(moistureSensorPin);
    return map(soilMoistureValue, drySoilThreshold,wetSoilThreshold, 0, 100);
}

void readDHTData() { // **ĐÃ SỬA LẠI HÀM readDHTData() - ĐÚNG VỚI CODE GỐC**
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println(F("Lỗi đọc DHT sensor!"));
        return;
    }
}

void updateLCDReadings() { // Đổi tên hàm để phù hợp (chỉ cập nhật LCD)
    soilMoisturePercentage = getSoilMoisturePercentage();
    readDHTData();

    // --- Cập nhật Blynk Virtual Pins - KHÔI PHỤC GỬI DỮ LIỆU LÊN BLYNK ---
    Blynk.virtualWrite(VPIN_SOIL_MOISTURE, soilMoisturePercentage);
    Blynk.virtualWrite(VPIN_PUMP_STATUS, pumpState ? "BẬT" : "TẮT");
    Blynk.virtualWrite(VPIN_TEMPERATURE, temperature);
    Blynk.virtualWrite(VPIN_HUMIDITY, humidity);
    Blynk.virtualWrite(VPIN_MODE_SWITCH, automaticMode); // **ĐÃ ĐỔI VPIN_MODE_STATUS -> VPIN_MODE_SWITCH VÀ GỬI TRẠNG THÁI automaticMode (true/false)**
    Blynk.virtualWrite(VPIN_DIRECT_PUMP_BUTTON_STATUS, directPumpControlButton ? "BẬT-Cơ" : "TẮT-Cơ");

    // --- Cập nhật LCD - GIỮ LẠI HIỂN THỊ THÔNG TIN CƠ BẢN VÀ CHẾ ĐỘ, TRẠNG THÁI NÚT BƠM TRỰC TIẾP ---
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("D:"); lcd.print(soilMoisturePercentage); lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("T:"); lcd.print(temperature); lcd.print("C");
    lcd.setCursor(8,1);
    lcd.print("H:"); lcd.print(humidity); lcd.print("%");
    lcd.setCursor(13,0);
    lcd.print(pumpState ? "ON" : "OFF");
    lcd.setCursor(6,0);
    lcd.print(automaticMode ? "AUTO" : "MAN"); // Hiển thị chế độ AUTO/MANUAL
    if (!automaticMode) { // Chỉ hiển thị trạng thái nút bơm cơ khi ở chế độ MANUAL
        lcd.setCursor(12, 1);
        lcd.print(directPumpControlButton ? "B-C" : "T-C"); // B-C: Bơm-Cơ (Bật Cơ), T-C: Tắt-Cơ
    } else {
        lcd.setCursor(12, 1);
        lcd.print("    "); // Xóa trạng thái nút bơm cơ khi ở chế độ AUTO
    }
}

void controlPump(bool enablePump) {
    pumpState = enablePump;
    digitalWrite(pumpRelayPin, pumpState ? HIGH : LOW);
    updateLCDReadings(); // Gọi hàm cập nhật LCD
    Blynk.virtualWrite(VPIN_PUMP_STATUS, pumpState ? "BẬT" : "TẮT"); // Gửi trạng thái bơm lên Blynk
}

// --- Hàm xử lý sự kiện nút chế độ (AceButton) - GIỮ NGUYÊN ---
void modeButtonHandler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
    Serial.println("Nut che do duoc nhan (AceButton)"); // In debug

    switch (eventType) {
        case AceButton::kEventReleased: // Xử lý khi nút được NHẢ RA
            Serial.println("Su kien: Nút chế độ NHẢ RA (kEventReleased)"); // In debug

            // --- Đảo ngược chế độ ---
            automaticMode = !automaticMode; // Chuyển đổi AUTO <-> MANUAL
            Serial.print("Che do chuyen sang (AceButton): ");
            Serial.println(automaticMode ? "Tu dong" : "Thu cong");

            updateLCDReadings(); // Cập nhật LCD để hiển thị chế độ mới
            Blynk.virtualWrite(VPIN_MODE_SWITCH, automaticMode); // **ĐÃ ĐỔI VPIN_MODE_STATUS -> VPIN_MODE_SWITCH VÀ GỬI TRẠNG THÁI automaticMode (true/false)**
            break;
    }
}

// --- **THÊM: Hàm xử lý sự kiện nút bơm trực tiếp (AceButton)** ---
void directPumpButtonHandler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
    Serial.println("Nut bom truc tiep duoc nhan (AceButton)"); // In debug

    switch (eventType) {
        case AceButton::kEventReleased: // Xử lý khi nút được NHẢ RA
            Serial.println("Su kien: Nút bơm trực tiếp NHẢ RA (kEventReleased)"); // In debug

            if (!automaticMode) { // CHỈ KHI Ở CHẾ ĐỘ MANUAL
                directPumpControlButton = !directPumpControlButton; // Đảo trạng thái bơm cơ
                controlPump(directPumpControlButton); // Gọi hàm điều khiển bơm dựa trên trạng thái nút bơm cơ
                Serial.print("Bom truc tiep chuyen sang (AceButton - Che do THU CONG): ");
                Serial.println(directPumpControlButton ? "BAT" : "TAT");
                updateLCDReadings(); // Cập nhật LCD
            } else {
                Serial.println("Nut bom truc tiep duoc nhan nhung o che do TU DONG - KHONG PHAN UNG.");
            }
            break;
    }
}

void checkBlynkStatus() { // Hàm kiểm tra kết nối Blynk - KHÔI PHỤC
    bool isconnected = Blynk.connected();
    if (isconnected == false) {
        Serial.print("Blynk Not Connected ");
        // --- **ĐÃ LOẠI BỎ digitalWrite(wifiLed, LOW); // Bật LED báo mất kết nối (tùy chọn)** ---
    }
    if (isconnected == true) {
        // --- **ĐÃ LOẠI BỎ digitalWrite(wifiLed, HIGH); // Tắt LED báo mất kết nối (tùy chọn)** ---
        //Serial.println("Blynk Connected");
    }
}

// --- Hàm BLYNK_CONNECTED() - KHÔI PHỤC ---
BLYNK_CONNECTED() {
    Blynk.syncVirtual(VPIN_SOIL_MOISTURE);
    Blynk.syncVirtual(VPIN_PUMP_STATUS);
    Blynk.syncVirtual(VPIN_TEMPERATURE);
    Blynk.syncVirtual(VPIN_HUMIDITY);
    Blynk.syncVirtual(VPIN_MODE_SWITCH); // **ĐÃ ĐỔI VPIN_MODE_STATUS -> VPIN_MODE_SWITCH**
    Blynk.virtualWrite(VPIN_MODE_SWITCH, automaticMode); // **ĐÃ ĐỔI VPIN_MODE_STATUS -> VPIN_MODE_SWITCH VÀ ĐỒNG BỘ TRẠNG THÁI automaticMode**
}

// --- Hàm BLYNK_WRITE(VPIN_RELAY) - KHÔI PHỤC và CHỈNH SỬA - **THÊM DEBUG PRINTS** ---
BLYNK_WRITE(VPIN_MANUAL_WATER) { // ĐÃ SỬA VPIN_RELAY -> VPIN_MANUAL_WATER - CHO PHÙ HỢP VỚI ĐỊNH NGHĨA
    Serial.println("--- Blynk_WRITE(VPIN_MANUAL_WATER) ---"); // Debug
    Serial.print("Che do AUTO/MANUAL: "); Serial.println(automaticMode ? "AUTO" : "MANUAL"); // Debug
    if(!automaticMode){ // CHỈ CHO PHÉP điều khiển bơm qua Blynk KHI Ở CHẾ ĐỘ MANUAL
        bool relayValueFromBlynk = param.asInt();
        Serial.print("Gia tri relay tu Blynk: "); Serial.println(relayValueFromBlynk); // Debug
        Serial.print("Trang thai bom hien tai: "); Serial.println(pumpState ? "BAT" : "TAT"); // Debug
        if (relayValueFromBlynk != pumpState) {
            controlPump(relayValueFromBlynk);
            directPumpControlButton = relayValueFromBlynk;
            updateLCDReadings();
            Serial.print("--> Da dieu khien bom qua Blynk. Trang thai bom moi: "); Serial.println(pumpState ? "BAT" : "TAT"); // Debug
        } else {
            Serial.println("--> Trang thai bom Blynk yeu cau KHONG thay doi so voi trang thai hien tai."); // Debug
        }
    } else {
        Blynk.virtualWrite(VPIN_PUMP_STATUS, pumpState);
        Serial.println("Che do AUTO - Khong dieu khien bom qua Blynk, chi dong bo trang thai."); // Debug
    }
    Serial.println("--- Ket thuc Blynk_WRITE(VPIN_MANUAL_WATER) ---"); // Debug
}

// --- Hàm BLYNK_WRITE(VPIN_MODE_SWITCH) - KHÔI PHỤC và CHỈNH SỬA ---
BLYNK_WRITE(VPIN_MODE_SWITCH) {
    bool modeValueFromBlynk = param.asInt();
    if (modeValueFromBlynk != automaticMode) { // Chỉ cập nhật nếu giá trị Blynk khác với chế độ hiện tại
        automaticMode = modeValueFromBlynk; // Cập nhật chế độ theo giá trị từ Blynk
        Serial.print("Che do chuyen sang (Blynk): ");
        Serial.println(automaticMode ? "Tu dong" : "Thu cong");
        updateLCDReadings(); // Cập nhật LCD để hiển thị chế độ mới
        Blynk.virtualWrite(VPIN_MODE_SWITCH, automaticMode); // **VẪN GIỮ NGUYÊN VPIN_MODE_SWITCH ĐỂ ĐỒNG BỘ TRẠNG THÁI LÊN WIDGET SWITCH**
    }
}


void setup() {
    Serial.begin(115200);
    delay(100);

    pinMode(pumpRelayPin, OUTPUT);
    // --- **ĐÃ LOẠI BỎ pinMode(wifiLed, OUTPUT); // Chân LED báo WiFi (tùy chọn)** ---
    digitalWrite(pumpRelayPin, LOW);
    // --- **ĐÃ LOẠI BỎ digitalWrite(wifiLed, LOW); // Tắt LED WiFi khi khởi động (tùy chọn)** ---


    // --- KHAI BÁO CHÂN NÚT BẤM - GIỮ NGUYÊN ---
    pinMode(modeButtonPin, INPUT_PULLUP);
    pinMode(directPumpButtonPin, INPUT_PULLUP);

    // --- Khởi tạo AceButton cho nút chế độ - GIỮ NGUYÊN ---
    modeButtonConfig.setEventHandler(modeButtonHandler); // Gán hàm xử lý sự kiện
    modeButton.init(modeButtonPin); // Khởi tạo nút với chân modeButtonPin

    // --- **THÊM: Khởi tạo AceButton cho nút bơm trực tiếp** ---
    directPumpButtonConfig.setEventHandler(directPumpButtonHandler); // Gán hàm xử lý sự kiện
    directPumpButton.init(directPumpButtonPin); // Khởi tạo nút với chân directPumpButtonPin

    dht.begin();
    lcd.init();
    lcd.backlight();
    lcd.print("Khoi dong...");

    // --- KẾT NỐI WIFI VÀ BLYNK - KHÔI PHỤC ---
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi da ket noi");
    Serial.print("Dia chi IP: ");
    Serial.println(WiFi.localIP());
    Blynk.begin(auth, ssid, pass);
    Serial.println("Da ket noi Blynk");

    // --- Định thời kiểm tra kết nối Blynk mỗi 3 giây - KHÔI PHỤC ---
    timer.setInterval(3000L, checkBlynkStatus); // Kiểm tra kết nối Blynk sau mỗi 3 giây

    // --- Định thời gửi dữ liệu cảm biến lên Blynk và cập nhật LCD mỗi 3 giây - KHÔI PHỤC (điều chỉnh chu kỳ nếu muốn) ---
    timer.setInterval(3000L, sendSensorData); // Gửi dữ liệu cảm biến mỗi 3 giây (có thể điều chỉnh)


    Serial.println("He thong tuoi cay da bat dau (Blynk - CO NUT BAM - ACEBUTTON MODE BUTTON - KHONG WIFI LED - DA SUA LOI BIEN - CAI THIEN NUT BAM & BLYNK CONTROL - CHE DO QUA SWITCH BLYNK)!"); // Thông báo khởi động
}

void loop() {
    Blynk.run(); // Chạy Blynk - KHÔI PHỤC
    timer.run(); // Chạy Blynk timer - KHÔI PHỤC

    updateLCDReadings(); // Gọi hàm cập nhật LCD - GIỮ NGUYÊN

    // --- KIỂM TRA NÚT CHẾ ĐỘ BẰNG ACEBUTTON - GIỮ NGUYÊN ---
    modeButton.check(); // Kiểm tra nút chế độ bằng AceButton

    // --- **THAY THẾ: KIỂM TRA NÚT BƠM TRỰC TIẾP BẰNG ACEBUTTON** ---
    directPumpButton.check(); // Kiểm tra nút bơm trực tiếp bằng AceButton

    // --- **ĐÃ LOẠI BỎ HOÀN TOÀN ĐOẠN CODE XỬ LÝ NÚT BƠM TRỰC TIẾP CŨ** ---

    // --- Định thời cho logic tưới tự động bằng millis() - GIỮ NGUYÊN (nếu bạn đã tích hợp millis() trước đó) ---
    unsigned long currentMillis = millis();
    if (currentMillis - previousWateringTime >= wateringInterval) {
        previousWateringTime = currentMillis; // Cập nhật thời điểm thực hiện logic tưới tự động

        // --- Logic tưới cây tự động (chỉ hoạt động ở chế độ tự động) ---
        if (automaticMode) { // Kiểm tra chế độ tự động
            if (!pumpState) {
                if (soilMoisturePercentage < 20) {
                    Serial.println("Dat qua kho! Bat bom (Tu dong)...");
                    controlPump(true);
                    pumpStartTime = millis();
                }
            } else {
                // --- Dừng bơm khi độ ẩm đạt trên 80% HOẶC hết thời gian tưới ---
                if ((soilMoisturePercentage >= targetSoilMoisturePercentage) || (millis() - pumpStartTime >= wateringDuration)) {
                    Serial.println("Do am dat dat nguong hoac het gio. Tat bom (Tu dong)...");
                    controlPump(false);
                }
            }
        } else {
            // Chế độ thủ công - Tưới tự động TẮT
            Serial.println("Che do thu cong. Tuoi tu dong TAT.");
        }
    }

    // --- Định thời cập nhật LCD bằng millis() - TÙY CHỌN - GIỮ NGUYÊN (nếu bạn đã tích hợp millis() trước đó) ---
    if (currentMillis - previousLCDUpdateTime >= lcdUpdateInterval) {
        previousLCDUpdateTime = currentMillis; // Cập nhật thời điểm cập nhật LCD
        updateLCDReadings(); // Gọi hàm cập nhật LCD
    }

   
    delay(1000); // GIỮ LẠI DELAY 100ms - BẠN CÓ THỂ THỬ LOẠI BỎ HOẶC GIẢM XUỐNG NẾU MUỐN
}

// --- Hàm gửi dữ liệu cảm biến lên Blynk (được gọi định kỳ bởi timer Blynk) - KHÔI PHỤC ---
void sendSensorData() {
    

    updateLCDReadings(); // Cập nhật LCD (bao gồm cả gửi dữ liệu lên Blynk trong hàm này, **updateLCDReadings() ĐÃ BAO GỒM ĐỌC CẢM BIẾN**)
}