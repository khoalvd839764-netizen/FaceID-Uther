#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QDateTime>
#include <vector>

// ============================================================================
// HƯỚNG DẪN HOẠT ĐỘNG MODULE: DATABASE (SQLite Procedural Version)
// ============================================================================
// 1. CHỨC NĂNG CHÍNH:
//    - Quản lý toàn bộ vòng đời lưu trữ của ứng dụng: Kết nối, tạo bảng, thêm/sửa sinh viên,
//      và lưu trữ lịch sử quét gương mặt điểm danh.
//    - Lưu trữ cục bộ thông qua SQLite (file cục bộ: 'attendance.db') bằng driver QSQLITE của Qt,
//      giúp chạy độc lập gọn nhẹ mà không cần bất kỳ tiến trình CSDL server nào (như MySQL/XAMPP).
//
// 2. PHƯƠNG THỨC HOẠT ĐỘNG & KIẾN TRÚC PHI OOP (PROCEDURAL):
//    - Module này được triển khai dưới dạng một Namespace chứa các hàm tự do (Free Functions)
//      và cấu trúc dữ liệu thuần túy (Plain Old Data structs), hoàn toàn không khai báo bất kỳ 
//      Class hay sử dụng cơ chế Kế thừa (Inheritance) nào.
//    - QSqlDatabase được khai báo tĩnh (static) nội bộ bên trong file cpp của module này để duy trì
//      phiên làm việc (Session) mặc định. Bất kỳ hàm nào gọi truy vấn QSqlQuery cũng sẽ tự động
//      liên kết tới session mặc định này mà không cần truyền đối tượng cơ sở dữ liệu qua lại.
//
// 3. CÁC HÀM CỐT LÕI:
//    - connectToDatabase: Đăng ký driver QSQLITE, chỉ định đường dẫn lưu file db và mở kết nối.
//    - initializeDatabase: Bật khóa ngoại SQLite (foreign_keys = ON), tạo bảng dữ liệu Students 
//      và AttendanceHistory nếu chưa tồn tại, đồng thời tạo các chỉ mục INDEX để tối ưu tốc độ đọc.
//    - syncStudent: Đồng bộ thông tin sinh viên bằng kỹ thuật UPSERT (ON CONFLICT DO UPDATE) của SQLite,
//      giúp tự động tạo mới hoặc cập nhật thông tin nếu trùng khóa chính (MSSV).
//    - recordCheckIn: Chụp thời gian hiện tại từ C++ chuyển thành chuỗi ISO8601 và chèn vào lịch sử SQLite.
// ============================================================================
namespace Database {

    // -----------------------------------------------------------------------
    // Cấu trúc dữ liệu
    // -----------------------------------------------------------------------

    // Cấu trúc lưu thông tin sinh viên
    struct StudentData {
        QString mssv;
        QString hoTen;
        QString maLop;
        std::vector<float> faceVector; // 128 số đặc trưng từ mô hình AI SFace
        QString avatarPath;            // Đường dẫn ảnh chân dung
    };

    // Cấu trúc lưu lịch sử điểm danh
    struct AttendanceRecord {
        int id;                    // ID tự tăng trong SQLite
        QString mssv;              // Mã sinh viên
        QString hoTen;             // Tên sinh viên (lấy từ bảng Students)
        QDateTime checkInTime;     // Thời gian quét mặt thành công
        QString capturedImagePath; // Đường dẫn ảnh chụp lúc check-in
        double confidence;         // Độ tin cậy nhận diện
    };

    // Cấu trúc lưu thông tin lớp học
    struct ClassData {
        QString maLop;       // Mã lớp (Khóa chính, ví dụ: CS-101)
        QString tenLop;      // Tên môn học / lớp học
        QString phongHoc;    // Phòng học (ví dụ: Phòng A2-302)
        int siSoDuKien;      // Sĩ số dự kiến
        QString ngayHoc;     // Ngày học (YYYY-MM-DD)
        QString gioBatDau;   // Giờ bắt đầu (HH:mm)
        QString gioKetThuc;  // Giờ kết thúc (HH:mm)
        int trangThai;       // Trạng thái lớp học (0: Chưa hoàn thành, 1: Đã hoàn thành)
    };


    // -----------------------------------------------------------------------
    // Các hàm chức năng (Free functions)
    // -----------------------------------------------------------------------

    // Kết nối tới SQLite Database (mặc định tạo file "attendance.db" cục bộ)
    bool connectToDatabase(const QString &dbPath = "attendance.db");

    // Ngắt kết nối cơ sở dữ liệu
    void disconnectDatabase();

    // Kiểm tra trạng thái kết nối
    bool isConnected();

    // Khởi tạo các bảng Students và AttendanceHistory nếu chưa tồn tại
    bool initializeDatabase();

    // Lấy thông báo lỗi cuối cùng
    QString getLastError();

    // Chuyển mảng vector<float> thành chuỗi TEXT (phân cách bằng dấu phẩy)
    QString vectorToString(const std::vector<float> &faceVector);

    // Chuyển chuỗi TEXT từ SQLite thành mảng vector<float>
    std::vector<float> stringToVector(const QString &vectorString);

    // Đồng bộ thông tin sinh viên (Thêm mới hoặc Cập nhật nếu trùng MSSV)
    bool syncStudent(const QString &mssv,
                     const QString &hoTen,
                     const QString &maLop,
                     const std::vector<float> &faceVector,
                     const QString &avatarPath = "");

    // Tải danh sách sinh viên theo lớp
    std::vector<StudentData> loadStudentsByClass(const QString &maLop);

    // Tải toàn bộ danh sách sinh viên
    std::vector<StudentData> loadAllStudents();

    // Ghi nhận lượt điểm danh mới vào CSDL
    bool recordCheckIn(const QString &mssv,
                       const QString &capturedImagePath,
                       double confidence);

    // Kiểm tra xem sinh viên đã điểm danh trong ngày chưa
    bool hasCheckedInToday(const QString &mssv);

    // Tải danh sách điểm danh hôm nay
    std::vector<AttendanceRecord> loadTodayAttendance();

    // Reset/Xóa dữ liệu điểm danh ngày hôm nay (tùy chọn theo mã lớp)
    bool resetTodayAttendance(const QString &maLop = "");

    // --- CÁC HÀM QUẢN LÝ LỚP HỌC (CLASS MANAGEMENT) ---
    // Tạo/Đăng ký lớp học mới
    bool createClass(const QString &maLop,
                     const QString &tenLop,
                     const QString &phongHoc,
                     int siSoDuKien,
                     const QString &ngayHoc = "",
                     const QString &gioBatDau = "",
                     const QString &gioKetThuc = "");

    // Kiểm tra lớp học đã tồn tại chưa
    bool classExists(const QString &maLop);

    // Tải toàn bộ danh sách lớp học
    std::vector<ClassData> loadAllClasses();

    // Đếm sĩ số thực tế (số sinh viên đã đăng ký FaceID trong lớp)
    int countStudentsInClass(const QString &maLop);

    // Xóa lớp học và tất cả sinh viên thuộc lớp học đó
    bool deleteClass(const QString &maLop);

    // Cập nhật trạng thái hoàn thành của lớp học (0: Chưa hoàn thành, 1: Đã hoàn thành)
    bool updateClassStatus(const QString &maLop, int status);

    // Lấy thời gian điểm danh của sinh viên vào ngày cụ thể (YYYY-MM-DD)
    QDateTime getCheckInTimeForDate(const QString &mssv, const QString &dateStr);

    // Quản lý tài khoản giáo viên
    bool createAccount(const QString &username, const QString &password);
    bool validateLogin(const QString &username, const QString &password);


} // namespace Database

#endif // DATABASE_H
