========================================================================
       BẢN TÓM TẮT LỖI VÀ PHƯƠNG ÁN KHẮC PHỤC (PHIÊN BẢN VISON CỐ ĐỊNH)
========================================================================

Thư mục 'vison/' này chứa phiên bản ứng dụng FacieID mới nhất đã được FIX HOÀN TOÀN CÁC LỖI dưới đây:

------------------------------------------------------------------------
1. LỖI KHÔNG NẠP ĐƯỢC MÔ HÌNH AI (ĐÃ FIX TRONG THƯ MỤC NÀY)
------------------------------------------------------------------------
- Vị trí file: backend/main.cpp (khoảng dòng 1200 - 1215).
- Nguyên nhân lỗi: Trình biên dịch debug của VSCode tạo file chạy .exe nằm ở build/Debug nên không tìm được thư mục models/ ở thư mục gốc.
- Giải pháp khắc phục: Bổ sung các đường dẫn quét đệ quy lùi cấp thư mục (../models, ../../models) tương đối với file chạy. Hiện tại ứng dụng chạy ổn định trên mọi môi trường và nạp mô hình thành công.

------------------------------------------------------------------------
2. LỖI KHÔNG HIỂN THỊ HỘP THOẠI THÊM LỚP (ĐÃ FIX TRONG THƯ MỤC NÀY)
------------------------------------------------------------------------
- Vị trí file: backend/main.cpp (cuối hàm showCreateClassDialog()).
- Nguyên nhân lỗi: Thiếu lệnh thực thi dialog.exec() khiến dialog cục bộ bị hủy ngay khi hàm chạy xong.
- Giải pháp khắc phục: Thêm lệnh dialog.exec() giữ cho dialog được hiển thị trên giao diện và hoạt động bình thường.

========================================================================
Lưu ý: Thư mục 'vison 1.1.0/' được giữ nguyên trạng thái bị lỗi như cũ để làm mẫu so sánh/lưu trữ theo yêu cầu.
