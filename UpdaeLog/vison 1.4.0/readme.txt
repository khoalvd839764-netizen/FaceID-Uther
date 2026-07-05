========================================================================
       BẢN TÓM TẮT LỖI VÀ PHƯƠNG ÁN KHẮC PHỤC (PHIÊN BẢN VISON CỐ ĐỊNH)
========================================================================

Thư mục 'vison/' này chứa phiên bản ứng dụng FacieID mới nhất đã được:
1. FIX HOÀN TOÀN CÁC LỖI biên dịch và giao diện.
2. THAY THẾ LỊCH TRÌNH THÀNH TRANG BÁO CÁO HỌC VIÊN ĐI TRỄ / VẮNG MẶT TRONG TUẦN.
3. BỔ SUNG KHUNG ĐĂNG NHẬP, ĐĂNG KÝ TÀI KHOẢN MỚI.
4. GỠ BỎ FEEDBACK, THÊM LOGO/AVATAR CHO GIÁO VIÊN VÀ ĐỒNG BỘ DỰA TRÊN TÀI KHOẢN ĐĂNG NHẬP THỰC TẾ.
5. THAY THẾ LOGO TIÊU ĐỀ ĐĂNG NHẬP VÀ LOGO SIDEBAR BẰNG HÌNH ẢNH LOGO UTH CHÍNH THỨC.
6. TỐI ƯU HÓA TỰ ĐỘNG CHỤP LẠI ẢNH MẶT KHI ĐIỂM DANH THÀNH CÔNG.
7. XÓA BỎ DÒNG GỢI Ý MẬT KHẨU MẶC ĐỊNH TRÊN MÀN HÌNH ĐĂNG NHẬP.
8. BỔ SUNG NÚT "RESET ĐIỂM DANH" CHO PHÉP XÓA NHANH LỊCH SỬ ĐIỂM DANH TRONG NGÀY.

------------------------------------------------------------------------
CÁC ĐIỂM NÂNG CẤP MỚI:
------------------------------------------------------------------------
- **Nút Reset điểm danh (Reset Today's Attendance)**:
  * Nút **"Reset điểm danh"** màu xám đá nằm cạnh nút "Dừng quét" ở trang Quét mặt (Face Scan).
  * Khi bấm, hệ thống hiển thị hộp thoại xác nhận để tránh bấm nhầm.
  * Tự động lọc thông minh: Nếu ô "Lọc lớp" trống, hệ thống sẽ xóa toàn bộ lịch sử điểm danh của ngày hôm nay. Nếu điền mã lớp, hệ thống chỉ xóa lịch sử điểm danh ngày hôm nay của riêng lớp đó.
  * Giao diện danh sách quét mặt và các chỉ số thống kê sĩ số (Có mặt/Vắng mặt) tự động cập nhật về trạng thái ban đầu ngay lập tức.

- **Hiển thị hình ảnh Logo UTH chính thức trên Màn hình Đăng nhập (Login Header Logo)**:
  * Thay thế tiêu đề chữ "FACIEID LOGIN" bằng hình ảnh logo UTH chính thức.

- **Hiển thị hình ảnh Logo UTH chính thức trên Sidebar (Sidebar Logo Image)**:
  * Load trực tiếp hình ảnh logo UTH chính thức thay cho chữ "UTH" ở đầu sidebar.
  
- **Chụp ảnh điểm danh trực tiếp từ QImage (Face capture)**:
  * Tối ưu hóa logic lưu ảnh điểm danh thực tế trực tiếp từ frame QImage khi AI nhận dạng thành công.
  * Đường dẫn lưu ảnh điểm danh tại thư mục chạy của ứng dụng (`checkin_images/`).

- Trang 3: Trang báo cáo **"Đi trễ & Vắng"** (Weekly Late & Absent Report) kiểm tra dữ liệu 7 ngày gần nhất.
- Chức năng XÓA LỚP HỌC hỗ trợ cascade tự động xóa sạch sinh viên và điểm danh liên quan.

------------------------------------------------------------------------
CÁC LỖI ĐÃ ĐƯỢC FIX:
------------------------------------------------------------------------
1. LỖI KHÔNG NẠP ĐƯỢC MÔ HÌNH AI:
   - Vị trí file: backend/main.cpp.
   - Giải pháp khắc phục: Bổ sung các đường dẫn quét đệ quy lùi cấp thư mục (../models, ../../models).

2. LỖI KHÔNG HIỂN THỊ HỘP THOẠI THÊM LỚP:
   - Vị trí file: backend/main.cpp.
   - Giải pháp khắc phục: Thêm lệnh dialog.exec() để hiển thị.

========================================================================
Lưu ý: Thư mục 'vison 1.1.0/' được giữ nguyên trạng thái bị lỗi như cũ để làm mẫu so sánh/lưu trữ theo yêu cầu.
