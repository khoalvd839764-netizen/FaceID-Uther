========================================================================
       TÀI LIỆU CẬP NHẬT & TÓM TẮT LỖI HỆ THỐNG (README)
========================================================================

Tài liệu này tóm tắt các tính năng mới được phát triển so với phiên bản Vison 1.0.0, đồng thời chỉ ra các lỗi đã được sửa đổi trong phiên bản Vison mới.

---

## I. CÁC CHỨC NĂNG MỚI SO VỚI PHIÊN BẢN VISON 1.0.0

Phiên bản mới bổ sung các tính năng bảo mật, thương hiệu UTH chính thức, quản lý lớp học và báo cáo tuần:

1. **Hộp Chọn Lớp Học trước khi Quét (QComboBox)**:
   - Thay vì yêu cầu giáo viên nhập mã lớp thủ công như trước, hệ thống đã trang bị hộp chọn lớp (ComboBox) thông minh.
   - Hiển thị danh sách tất cả các lớp học hiện có trong CSDL dạng: `[Mã Lớp] ([Tên Môn Học])`. Lựa chọn đầu tiên luôn là "Tất cả lớp học" giúp điểm danh hàng loạt.
   - Khi chọn lớp nào, camera quét nhận diện sẽ chỉ lấy dữ liệu sinh viên của lớp học đó để thực hiện quét mặt điểm danh.
   - Danh sách lớp học trong ComboBox tự động cập nhật làm mới mỗi khi giáo viên chuyển hướng về trang quét mặt (Face Scan).

2. **Nút Reset điểm danh thông minh**:
   - Thêm nút **"Reset điểm danh"** màu xám đá cạnh nút điều khiển camera để xóa lịch sử điểm danh ngày hôm nay của tất cả các lớp hoặc lớp được chọn.

3. **Hiển thị hình ảnh Logo UTH chính thức trên Màn hình Đăng nhập**:
   - Thay thế hoàn toàn dòng chữ tiêu đề "FACIEID LOGIN" bằng hình ảnh logo UTH chính thức tải từ thư mục tài nguyên (`logo_uth.png`).

4. **Hiển thị hình ảnh Logo UTH chính thức trên Sidebar**:
   - Vùng đầu thanh bên sidebar hiển thị hình ảnh logo UTH nằm ngang chính thức được tải trực tiếp từ thư mục `assets/` hoặc `access/` của ứng dụng (`logo_uth.png`).

5. **Chụp lưu ảnh điểm danh khuôn mặt**:
   - Khi hệ thống nhận diện thành công sinh viên điểm danh, frame hình ảnh `QImage` thực tế tại thời điểm nhận dạng sẽ tự động được chụp lưu trữ tức thì.
   - Thư mục lưu trữ nằm trong thư mục chạy ứng dụng: `vison/checkin_images/` để giáo viên tiện quản lý và đối chiếu.

6. **Đồng bộ tên tài khoản động dựa trên đăng nhập thực tế**:
   - Tên giáo viên trên **Thẻ Profile Sidebar** và trên **Trang Cài đặt** tự động đồng bộ hiển thị bằng tài khoản thực tế vừa đăng nhập thành công.

7. **Cập nhật nhãn Sidebar thành "UTH"**:
   - Trạng thái phiên kết nối cơ sở dữ liệu ở thanh bên (Sidebar Profile Card Status) được sửa đổi thành nhãn **"UTH"** nổi bật.

8. **Khung Đăng ký tài khoản mới (Sign-up)**:
   - Trên hộp thoại đăng nhập, bổ sung nút liên kết **"Chưa có tài khoản? Đăng ký ngay"** cho phép tạo tài khoản giáo viên mới lưu trữ trực tiếp vào CSDL SQLite.

9. **Khung Đăng nhập (Login) xác thực cơ sở dữ liệu**:
   - Khi khởi động ứng dụng, hộp thoại yêu cầu đăng nhập sẽ xuất hiện đầu tiên, đối chiếu trực tiếp từ bảng tài khoản SQLite.
   - Khởi tạo sẵn tài khoản quản trị mặc định: **admin** | Mật khẩu: **123** nếu CSDL trống.
   - Nút "Thoát" cho phép đóng nhanh ứng dụng nếu không muốn đăng nhập.

10. **Giao diện trang Cài đặt (Settings - Trang 4)**:
   - Đã xóa bỏ hoàn toàn phần Phản hồi & Đóng góp (Feedback) cũ.
   - Bổ sung Logo đại diện tròn màu xanh dương "GV" bắt mắt và hiển thị tên tài khoản động.
   - Bổ sung nút **"Đăng xuất"** màu đỏ giúp chuyển đổi tài khoản.

11. **Trang Báo cáo Học viên Đi trễ / Vắng mặt trong tuần** (Đi trễ & Vắng):
   - Hiển thị danh sách học viên đi trễ hoặc vắng mặt trong vòng 7 ngày gần nhất, với các màu viền biểu thị trực quan (Vàng/Cam: Đi trễ, Đỏ: Vắng mặt).

12. **Giao diện lưới thẻ lớp học (Class Card Grid)**:
   - Hiển thị danh sách lớp dưới dạng ô lưới bo góc, kèm thông tin chi tiết: Mã lớp, Tên lớp, Phòng học, Sĩ số thực tế / Sĩ số dự kiến và nút Xem chi tiết.

13. **Thêm lớp học mới**:
   - Nút `+ Tạo lớp học mới` mở Dialog yêu cầu điền đầy đủ thông tin: Mã lớp, Tên lớp, Phòng học, Sĩ số, Ngày học (sử dụng `QDateEdit`), Giờ học (sử dụng `QTimeEdit`).

14. **Hộp thoại Xem Chi tiết Lớp học**:
   - Hiển thị danh sách sinh viên đã đăng ký trong lớp học đó, hỗ trợ thêm sinh viên nhanh và nút "Xóa lớp học" màu đỏ.

15. **Ràng buộc khi Xóa lớp học (Cascade Delete)**:
   - Khi xóa lớp học, tất cả sinh viên thuộc lớp và lịch sử điểm danh của họ cũng sẽ được tự động xóa sạch để đảm bảo toàn vẹn dữ liệu. Đồng thời, báo cáo của lớp học đó trên trang Đi trễ & Vắng cũng biến mất ngay tức thì.

16. **Ràng buộc dữ liệu mã lớp (Data Validation)**:
   - Chặn đăng ký sinh viên mới ở giao diện quét mặt nếu mã lớp nhập vào không tồn tại trong hệ thống.

---

## II. DANH SÁCH LỖI VÀ VỊ TRÍ CHI TIẾT (Đã fix ở thư mục "vison/")

Phiên bản lưu trữ `vison 1.1.0/` được giữ nguyên trạng thái bị lỗi để đối chiếu. Các lỗi đó đã được khắc phục hoàn toàn trong phiên bản mới tại thư mục `vison/`:

### 1. Lỗi không nạp được mô hình AI (ONNX models)
- **Tập tin bị lỗi**: `main.cpp`
- **Dòng lỗi**: Khoảng dòng 1201 - 1212 (vị trí cũ).
- **Mô tả lỗi**: Đường dẫn tìm kiếm file `.onnx` chỉ quét ở thư mục chạy của file thực thi (`appDir/models/`) và thư mục làm việc hiện hành. Khi chạy/debug ứng dụng bằng VSCode, file thực thi nằm trong thư mục con `build/Debug` nên không tìm được thư mục `models/` ở thư mục gốc, gây lỗi crash/lỗi nạp model.
- **Cách khắc phục**: Bổ sung các đường dẫn quét dự phòng lùi cấp thư mục tương đối (`../models`, `../../models`).

### 2. Lỗi nút "+" không mở được hộp thoại thêm lớp mới
- **Tập tin bị lỗi**: `main.cpp`
- **Dòng lỗi**: Dòng cuối cùng của hàm `showCreateClassDialog()`.
- **Mô tả lỗi**: Thiết lập đầy đủ các trường nhập dữ liệu, nút bấm của Dialog nhưng quên không gọi lệnh hiển thị hộp thoại `dialog.exec();`. Do đối tượng `dialog` được khởi tạo trên Stack (biến cục bộ của hàm), ngay khi hàm chạy xong, nó lập tức bị C++ giải phóng bộ nhớ tự động nên giáo viên bấm nút `+` không thấy phản hồi.
- **Cách khắc phục**: Bổ sung dòng lệnh chạy hộp thoại `dialog.exec();` ở cuối hàm.

---
Tài liệu được cập nhật ngày 02/07/2026.
