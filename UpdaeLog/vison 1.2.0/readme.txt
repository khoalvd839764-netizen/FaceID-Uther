========================================================================
       BẢN TÓM TẮT LỖI VÀ PHƯƠNG ÁN KHẮC PHỤC (PHIÊN BẢN VISON CỐ ĐỊNH)
========================================================================

Thư mục 'vison/' này chứa phiên bản ứng dụng FacieID mới nhất đã được:
1. FIX HOÀN TOÀN CÁC LỖI biên dịch và giao diện.
2. THAY THẾ LỊCH TRÌNH THÀNH TRANG BÁO CÁO HỌC VIÊN ĐI TRỄ / VẮNG MẶT TRONG TUẦN.

------------------------------------------------------------------------
CÁC ĐIỂM NÂNG CẤP MỚI:
------------------------------------------------------------------------
- Trang 3 cũ được thay thế bằng trang **"Đi trễ & Vắng"** (Weekly Late & Absent Report).
- **Cách thức hoạt động của Báo cáo Đi trễ / Vắng**:
  * Hệ thống tự động kiểm tra lịch sử điểm danh của học viên trong vòng 7 ngày gần nhất (tính từ ngày hiện tại lùi về sau).
  * Đối với các buổi học của lớp học đã diễn ra:
    - Nếu học viên **vắng mặt** (không quét mặt điểm danh): Thẻ báo cáo hiển thị badge đỏ **"VẮNG MẶT"** kèm giờ học của lớp và thông báo không ghi nhận điểm danh.
    - Nếu học viên **đi trễ** (quét mặt sau giờ bắt đầu của lớp học): Thẻ báo cáo hiển thị badge vàng/cam **"ĐI TRỄ"** kèm thông tin so sánh chi tiết thời điểm quét mặt thực tế và giờ vào lớp.
- Chức năng XÓA LỚP HỌC (được tích hợp ở nút "..." trên card lớp và nút đỏ trong Dialog Chi tiết).
  * Khi thực hiện xóa lớp học, tất cả sinh viên thuộc lớp và lịch sử điểm danh của họ cũng sẽ được tự động xóa sạch để đảm bảo toàn vẹn dữ liệu.
  * Đồng thời, tất cả thông tin đi trễ / vắng mặt của lớp đó cũng sẽ tự động được xóa bỏ và làm mới ngay lập tức trên trang Báo cáo tuần.

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
