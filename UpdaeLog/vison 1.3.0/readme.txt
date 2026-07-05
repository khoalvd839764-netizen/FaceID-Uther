========================================================================
       BẢN TÓM TẮT LỖI VÀ PHƯƠNG ÁN KHẮC PHỤC (PHIÊN BẢN VISON CỐ ĐỊNH)
========================================================================

Thư mục 'vison/' này chứa phiên bản ứng dụng FacieID mới nhất đã được:
1. FIX HOÀN TOÀN CÁC LỖI biên dịch và giao diện.
2. THAY THẾ LỊCH TRÌNH THÀNH TRANG BÁO CÁO HỌC VIÊN ĐI TRỄ / VẮNG MẶT TRONG TUẦN.
3. BỔ SUNG KHUNG ĐĂNG NHẬP, ĐĂNG KÝ TÀI KHOẢN MỚI.
4. GỠ BỎ FEEDBACK, THÊM LOGO/AVATAR CHO GIÁO VIÊN VÀ ĐỒNG BỘ DỰA TRÊN TÀI KHOẢN ĐĂNG NHẬP THỰC TẾ.
5. ĐỔI NHÃN TRÊN SIDEBAR THÀNH "UTH".

------------------------------------------------------------------------
CÁC ĐIỂM NÂNG CẤP MỚI:
------------------------------------------------------------------------
- **Đồng bộ tên tài khoản động dựa trên đăng nhập thực tế**:
  * Tên tài khoản hiển thị trên **Thẻ Profile Sidebar** và trên **Trang Cài đặt** giờ đây sẽ được cập nhật tự động bằng tên tài khoản mà giáo viên đã sử dụng để đăng nhập thành công (thay vì cố định là "Administrator" và "admin" như trước).
  * Hình đại diện hình tròn (Avatar) trên sidebar cũng tự động cập nhật chữ cái đầu viết hoa tương ứng theo tên tài khoản của giáo viên đó (ví dụ: đăng nhập "teacher1" thì avatar hiện chữ "T").
  * Khi giáo viên click "Đăng xuất" và đăng nhập lại bằng một tài khoản khác, toàn bộ thông tin tên, avatar và cài đặt của tài khoản mới sẽ lập tức được cập nhật đồng bộ theo thời gian thực (Real-time).

- **Đổi nhãn trên Sidebar**:
  * Trạng thái phiên cơ sở dữ liệu ở thanh bên sidebar (Sidebar Profile Status) được cập nhật nhãn hiển thị thành **"UTH"** (thay cho "LOCAL DB SESSION" cũ).

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
