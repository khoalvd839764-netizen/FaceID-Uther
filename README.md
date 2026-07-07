# 🖥️ FacieID Uther Dashboard - Hệ Thống Điểm Danh Sinh Viên Qua FaceID

![FacieID Uther Banner](github_promo_banner.png)

Hệ thống điểm danh sinh viên thời gian thực sử dụng trí tuệ nhân tạo (mạng nơ-ron YuNet phát hiện và SFace nhận diện khuôn mặt). Ứng dụng được xây dựng trên nền tảng **C++ / Qt6** và cơ sở dữ liệu **SQLite** cục bộ, tối ưu hóa độ trễ thấp và bảo mật dữ liệu.

---

## ✨ Các tính năng nổi bật ở bản v2.0.0
* **📅 Quản lý lịch học tối giản:** Tách biệt biểu mẫu tạo lớp và thiết lập lịch học tuần bằng checkbox. Cho phép 1 lớp học nhiều ca trong tuần linh hoạt.
* **📄 Xuất báo cáo PDF đối chứng:** Tự động truy vấn ngày điểm danh thực tế, xuất báo cáo PDF khổ ngang chứa đầy đủ thông tin học viên kèm ảnh đối chứng (ảnh thẻ gốc vs ảnh chụp thực tế lúc camera quét mặt).
* **⏰ Đồng bộ múi giờ 24h:** Khắc phục triệt để lỗi lệch giờ điểm danh sáng/tối và tính toán số phút đi trễ chuẩn xác.
* **🔧 Tương thích tối ưu:** Hoạt động tốt trên các thư mục chứa ký tự Tiếng Việt có dấu và tự động chẩn đoán lỗi mô hình AI thông minh.

---

## 🛠️ HƯỚNG DẪN CÀI ĐẶT & CHẠY ỨNG DỤNG (SETUP)

Bạn có thể lựa chọn 1 trong 2 cách tải và chạy dưới đây tùy nhu cầu sử dụng:

### 📦 CÁCH 1: Chạy ngay lập tức (Dành cho người dùng cuối / Giảng viên)
*Khuyên dùng nếu bạn chỉ muốn chạy phần mềm để điểm danh mà không cần lập trình.*

1. **Tải về:** Truy cập mục **[Releases](https://github.com/khoalvd839764-netizen/FaceID-Uther/releases)** ở cột bên phải trang GitHub này và tải tệp **`FacieID_vison_Release.zip`** về máy.
2. **Giải nén:** Nhấp chuột phải vào tệp ZIP vừa tải và chọn **Extract All** (Giải nén toàn bộ) ra một thư mục.
3. **Cài đặt hỗ trợ (Nếu máy tính chưa có):**
   - Nếu máy báo lỗi thiếu file hệ thống hoặc không chạy được, hãy tải và cài đặt gói hỗ trợ của Microsoft: **[Visual C++ Redistributable 2015-2022 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe)**.
4. **Khởi chạy:** Nhấp đúp chuột vào tệp **`FacieID_UtherDashbbord.exe`** bên trong thư mục giải nén để mở ứng dụng điểm danh ngay!

---

### 💻 CÁCH 2: Chạy từ Source Code giải nén ZIP (Dành cho Lập trình viên)
*Sử dụng khi bạn muốn xem mã nguồn, chỉnh sửa giao diện hoặc tự biên dịch bằng C++.*

1. **Tải mã nguồn:** Nhấp vào nút **Code** màu xanh ở đầu trang GitHub này và chọn **Download ZIP**.
2. **Giải nén:** Giải nén tệp ZIP vừa tải về máy.
3. **Chạy ứng dụng:**
   - Phiên bản mới đã được cấu hình chuyển tệp mô hình sang Git thông thường.
   - Do đó, trong thư mục giải nén, các tệp mô hình trong thư mục **`models/`** đã chứa đầy đủ tệp tin thật (~232KB và ~38MB).
   - Bạn chỉ cần mở dự án bằng **Qt Creator** hoặc **VS Code**, cấu hình trình biên dịch **CMake / MinGW 64-bit** và bấm **Run** để biên dịch chạy dự án.

---

## ❓ Xử lý một số sự cố thường gặp (Troubleshooting)

* **Lỗi camera không hiển thị hoặc bị đen màn hình:**
  - Hãy kiểm tra xem camera của máy tính có đang bị ứng dụng khác sử dụng không (ví dụ: Zoom, Teams, Zalo). Hãy tắt các ứng dụng đó trước khi mở FacieID.
  - Kiểm tra xem camera có bị che hoặc bị tắt nút gạt vật lý trên laptop hay không.
* **Lỗi nạp mô hình AI ONNX:**
  - Ứng dụng v2.0.0 đã tích hợp chẩn đoán lỗi thông minh. Nếu gặp lỗi này, hãy đọc kỹ thông báo hiện trên màn hình để kiểm tra xem tệp mô hình của bạn có bị thiếu hoặc dung lượng quá nhỏ (do lỗi Git LFS cũ) hay không.

---
*Chúc bạn có trải nghiệm tuyệt vời cùng FacieID Uther!*
