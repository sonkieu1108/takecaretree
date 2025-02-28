Hệ Thống Tưới Cây Thông Minh ESP32


Đây là dự án phát triển một Hệ Thống Tưới Cây Thông Minh sử dụng ESP32 với hai phiên bản điều khiển:

1.	Phiên bản Blynk: Điều khiển qua ứng dụng Blynk với nút bấm vật lý.

2.	Phiên bản Web Local: Điều khiển qua giao diện web cục bộ (Local Web Server).

Dự án này tích hợp cảm biến độ ẩm đất, cảm biến nhiệt độ/độ ẩm không khí (DHT11), màn hình LCD I2C, và relay để điều khiển bơm nước. Hệ thống hỗ trợ chế độ tự động (AUTO) và thủ công (MANUAL).

________________________________________
Nội dung kho lưu trữ

•	SmartIrrigation_Blynk.ino: Mã nguồn cho phiên bản sử dụng Blynk.

•	SmartIrrigation_WebLocal.ino: Mã nguồn cho phiên bản sử dụng Web Server cục bộ.

•	README.md: File hướng dẫn này.

________________________________________
Yêu cầu phần cứng

•	ESP32 Dev Module: Bo mạch điều khiển chính.

•	Cảm biến độ ẩm đất: Đo độ ẩm đất (kết nối GPIO34).

•	Cảm biến DHT11: Đo nhiệt độ và độ ẩm không khí (kết nối GPIO4).

•	Relay: Điều khiển bơm nước (kết nối GPIO2).

•	Màn hình LCD I2C (16x2): Hiển thị thông tin (địa chỉ I2C: 0x27).

•	Nút bấm (chỉ áp dụng cho phiên bản Blynk):
o	Nút chuyển chế độ AUTO/MANUAL (GPIO25).

o	Nút bật/tắt bơm trực tiếp (GPIO17).


•	Nguồn điện: 3.3V/5V cho ESP32 và các thiết bị ngoại vi.

________________________________________
Yêu cầu phần mềm

Thư viện cần cài đặt

1.	Arduino IDE: Môi trường lập trình ESP32.

2.	Thư viện cho cả hai phiên bản:
o	WiFi.h: Kết nối WiFi (có sẵn trong Arduino IDE).

o	DHT.h: Hỗ trợ cảm biến DHT11 (tải từ Library Manager).

o	LiquidCrystal_I2C.h: Hỗ trợ màn hình LCD I2C (tác giả Frank de Brabander).


3.	Thư viện bổ sung cho phiên bản Blynk:
o	BlynkSimpleEsp32.h: Thư viện Blynk cho ESP32 (tải từ Library Manager).

o	AceButton.h: Thư viện xử lý nút bấm (tải từ Library Manager).


4.	Thư viện bổ sung cho phiên bản Web Local:
o	WiFiServer.h và WiFiClient.h: Hỗ trợ Web Server (có sẵn trong Arduino IDE).


Cài đặt thư viện

•	Mở Arduino IDE.

•	Vào Sketch > Include Library > Manage Libraries.

•	Tìm và cài đặt các thư viện cần thiết theo danh sách trên.

________________________________________
Hướng dẫn cài đặt và sử dụng

1. Chuẩn bị phần cứng

•	Kết nối các cảm biến, relay, LCD và nút bấm theo sơ đồ chân được định nghĩa trong mã nguồn.

•	Đảm bảo nguồn điện ổn định cho ESP32 và các thiết bị ngoại vi.

2. Cấu hình mã nguồn

Phiên bản Blynk (SmartIrrigation_Blynk.ino)

1.	Cấu hình WiFi:
o	Sửa ssid và pass trong đoạn code sau với thông tin WiFi của bạn:
cpp

WrapCopy

char ssid[] = "Kieu Son"; // Thay bằng SSID WiFi của bạn
char pass[] = "123456789"; // Thay bằng mật khẩu WiFi của bạn

2.	

3.	Cấu hình Blynk:
o	Cập nhật thông tin Template ID, Template Name, và Auth Token từ ứng dụng Blynk:
cpp

WrapCopy

#define BLYNK_TEMPLATE_ID "TMPL6joqqjaJU"
#define BLYNK_TEMPLATE_NAME "tudongtuoi"
#define BLYNK_AUTH_TOKEN "eRhG-3uVpGKg9x83UKyGWQeHzuepf32M"

4.	

5.	Nạp mã nguồn:
o	Mở file SmartIrrigation_Blynk.ino trong Arduino IDE.

o	Chọn board ESP32 và cổng COM, sau đó nhấn Upload.


6.	Cấu hình Blynk Dashboard:
o	Tạo một dự án mới trên ứng dụng Blynk.

o	Thêm các widget sau và gán Virtual Pins:
	V0: Soil Moisture (Gauge).

	V1: Pump Status (LED hoặc Label).

	V2: Manual Water (Button - Switch mode).

	V3: Temperature (Gauge).

	V4: Humidity (Gauge).

	V5: Mode Switch (Switch).

	V6: Direct Pump Button Status (Label).



Phiên bản Web Local (SmartIrrigation_WebLocal.ino)

1.	Cấu hình WiFi:
o	Sửa ssid và pass tương tự như trên:
cpp

WrapCopy

char ssid[] = "Kieu Son"; // Thay bằng SSID WiFi của bạn
char pass[] = "123456789"; // Thay bằng mật khẩu WiFi của bạn

2.	

3.	Nạp mã nguồn:
o	Mở file SmartIrrigation_WebLocal.ino trong Arduino IDE.

o	Chọn board ESP32 và cổng COM, sau đó nhấn Upload.


4.	Truy cập giao diện web:
o	Sau khi nạp code, mở Serial Monitor (115200 baud) để xem địa chỉ IP của ESP32 (ví dụ: 192.168.x.x).

o	Mở trình duyệt và nhập địa chỉ IP để truy cập giao diện điều khiển.


3. Kiểm tra hoạt động

•	Phiên bản Blynk:
o	Quan sát dữ liệu trên LCD và ứng dụng Blynk.

o	Dùng nút bấm vật lý hoặc ứng dụng Blynk để chuyển chế độ và điều khiển bơm.


•	Phiên bản Web Local:
o	Giao diện web hiển thị dữ liệu cảm biến và trạng thái bơm.

o	Nhấn nút "Chuyển chế độ" hoặc "Bật/Tắt Bơm" để điều khiển.



