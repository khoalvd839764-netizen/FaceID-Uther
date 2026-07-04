#include "Database.h"
#include <QSqlDatabase>  // Qt SQL: Quản lý kết nối cơ sở dữ liệu
#include <QSqlQuery>     // Qt SQL: Thực thi truy vấn SQL
#include <QSqlError>     // Qt SQL: Đọc thông báo lỗi từ SQL
#include <QStringList>   // Qt: Danh sách chuỗi (dùng để ghép/tách vector float)
#include <QDebug>        // Qt: Xuất log ra console để debug
#include <QDate>         // Qt: Xử lý ngày tháng (so sánh ngày điểm danh)

// ============================================================================
// CHI TIẾT TRIỂN KHAI MODULE: DATABASE (SQLite Procedural Version)
// ============================================================================
// - Module này triển khai các hàm đã khai báo trong Database.h.
// - Một biến tĩnh static QSqlDatabase s_db được định nghĩa ở đây để lưu giữ kết nối
//   cơ sở dữ liệu SQLite trong suốt quá trình chạy của ứng dụng.
// - Các truy vấn SQL trong file này sử dụng SQLite làm hệ quản trị:
//   * Bảng "Students": Lưu thông tin sinh viên và FaceVector (lưu dạng TEXT phân tách bằng dấu phẩy).
//   * Bảng "AttendanceHistory": Lưu thông tin điểm danh, có khóa ngoại (FOREIGN KEY) tham chiếu
//     đến bảng Students với chế độ ON DELETE CASCADE (khi xóa sinh viên thì tự động xóa lịch sử điểm danh).
//   * Do SQLite không lưu ngày giờ động như MySQL, chúng ta định dạng ngày giờ điểm danh trực tiếp 
//     từ C++ bằng QDateTime sang định dạng ISO8601 ("yyyy-MM-dd hh:mm:ss") để lưu vào kiểu TEXT,
//     điều này giúp việc so sánh lọc dữ liệu "date(CheckInTime) = date('now', 'localtime')" hoạt động chính xác.
// ============================================================================
namespace Database {

    // -----------------------------------------------------------------------
    // BIẾN TĨNH NỘI BỘ - Đối tượng kết nối SQLite duy nhất trong toàn bộ app
    // Sử dụng 'static' để giới hạn phạm vi trong file Database.cpp (không lộ ra ngoài)
    // -----------------------------------------------------------------------
    static QSqlDatabase s_db;

    // -----------------------------------------------------------------------
    // connectToDatabase: Kết nối tới file SQLite và khởi tạo cấu trúc bảng
    //
    // Hoạt động:
    //   1. Kiểm tra xem kết nối mặc định đã tồn tại chưa để tránh tạo trùng
    //   2. Đăng ký driver QSQLITE và chỉ định đường dẫn file .db
    //   3. Mở kết nối (sẽ tự tạo file .db nếu chưa tồn tại)
    //   4. Gọi initializeDatabase() để tạo bảng nếu cần
    //
    // Tham số:
    //   dbPath: Đường dẫn file SQLite (mặc định: "attendance.db" trong thư mục hiện tại)
    // Trả về: true nếu kết nối và khởi tạo thành công
    // -----------------------------------------------------------------------
    bool connectToDatabase(const QString &dbPath)
    {
        // Kiểm tra xem connection mặc định đã tồn tại chưa
        // (tránh tạo nhiều connection, gây lỗi "Connection already exists")
        if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
            // Lấy connection hiện có thay vì tạo mới
            s_db = QSqlDatabase::database(QSqlDatabase::defaultConnection);
        } else {
            // Thêm database mới với driver QSQLITE (SQLite driver của Qt)
            s_db = QSqlDatabase::addDatabase("QSQLITE");
        }

        // Đặt tên file cơ sở dữ liệu (SQLite sẽ tạo file này nếu chưa tồn tại)
        s_db.setDatabaseName(dbPath);

        // Thực hiện mở kết nối
        if (!s_db.open()) {
            qCritical() << "[DB] Lỗi không thể mở SQLite Database:" << s_db.lastError().text();
            return false;
        }

        qDebug() << "[DB] Kết nối SQLite thành công. File database:" << dbPath;

        // Sau khi kết nối, khởi tạo bảng (CREATE TABLE IF NOT EXISTS)
        return initializeDatabase();
    }

    // -----------------------------------------------------------------------
    // disconnectDatabase: Đóng kết nối SQLite một cách an toàn
    // Được gọi khi thoát ứng dụng để tránh mất dữ liệu hoặc file bị hỏng
    // -----------------------------------------------------------------------
    void disconnectDatabase()
    {
        if (s_db.isOpen()) {
            s_db.close(); // Flush cache và đóng file SQLite
            qDebug() << "[DB] Đã ngắt kết nối an toàn với SQLite.";
        }
    }

    // -----------------------------------------------------------------------
    // isConnected: Kiểm tra trạng thái kết nối hiện tại
    // Trả về: true nếu SQLite đang mở và sẵn sàng truy vấn
    // -----------------------------------------------------------------------
    bool isConnected()
    {
        return s_db.isOpen();
    }

    // -----------------------------------------------------------------------
    // initializeDatabase: Tạo tất cả bảng và chỉ mục nếu chưa tồn tại
    //
    // Các bảng được tạo:
    //   - Classes: Thông tin lớp học (Mã lớp, Tên lớp, Phòng, Sĩ số, Giờ học...)
    //   - Students: Thông tin sinh viên + FaceVector (vector 128 chiều lưu dạng TEXT)
    //   - AttendanceHistory: Lịch sử điểm danh (MSSV, Thời gian, Ảnh, Độ tin cậy)
    //   - Accounts: Tài khoản giáo viên (Username, Password - plaintext cơ bản)
    //
    // Trả về: true nếu khởi tạo thành công tất cả bảng
    // -----------------------------------------------------------------------
    bool initializeDatabase()
    {
        // Kiểm tra kết nối trước khi thực thi truy vấn
        if (!s_db.isOpen()) {
            qWarning() << "[DB] CSDL chưa mở, không thể khởi tạo bảng.";
            return false;
        }

        QSqlQuery query; // Đối tượng thực thi truy vấn SQL

        // ============================================================
        // QUAN TRỌNG: Bật ràng buộc FOREIGN KEY trong SQLite
        // SQLite mặc định TẮT kiểm tra khóa ngoại để tương thích ngược.
        // Cần bật để ON DELETE CASCADE hoạt động (xóa sinh viên -> xóa lịch sử).
        // ============================================================
        query.exec("PRAGMA foreign_keys = ON;");

        // ============================================================
        // TẠO BẢNG Classes (Lớp học)
        // Cấu trúc:
        //   MaLop TEXT PRIMARY KEY: Mã lớp là khóa chính duy nhất (ví dụ: "CS-101")
        //   TenLop TEXT NOT NULL:   Tên môn học / lớp học
        //   PhongHoc TEXT:          Phòng học (ví dụ: "A2-302")
        //   SiSoDuKien INTEGER:     Sĩ số tối đa dự kiến
        //   NgayHoc TEXT:           Ngày học định dạng YYYY-MM-DD
        //   GioBatDau, GioKetThuc:  Giờ học định dạng HH:mm
        //   TrangThai INTEGER:      0=Chưa hoàn thành, 1=Đã hoàn thành
        // ============================================================
        QString createClasses = R"(
            CREATE TABLE IF NOT EXISTS Classes (
                MaLop TEXT PRIMARY KEY,
                TenLop TEXT NOT NULL,
                PhongHoc TEXT,
                SiSoDuKien INTEGER DEFAULT 0,
                NgayHoc TEXT,
                GioBatDau TEXT,
                GioKetThuc TEXT,
                TrangThai INTEGER DEFAULT 0
            );
        )";
        if (!query.exec(createClasses)) {
            qCritical() << "[DB] Lỗi tạo bảng Classes:" << query.lastError().text();
            return false;
        }

        // ============================================================
        // MIGRATION: Tự động nâng cấp cấu trúc bảng Classes
        // Nếu database cũ (tạo trước khi có các cột thời gian/trạng thái),
        // các lệnh ALTER TABLE này sẽ thêm cột còn thiếu.
        // SQLite sẽ trả lỗi (nhưng không crash) nếu cột đã tồn tại -> bỏ qua.
        // ============================================================
        query.exec("ALTER TABLE Classes ADD COLUMN NgayHoc TEXT;");
        query.exec("ALTER TABLE Classes ADD COLUMN GioBatDau TEXT;");
        query.exec("ALTER TABLE Classes ADD COLUMN GioKetThuc TEXT;");
        query.exec("ALTER TABLE Classes ADD COLUMN TrangThai INTEGER DEFAULT 0;");

        qDebug() << "[DB] Bảng Classes đã sẵn sàng.";

        // ============================================================
        // TẠO BẢNG Students (Sinh viên)
        // Cấu trúc:
        //   MSSV TEXT PRIMARY KEY: Mã số sinh viên là khóa chính
        //   HoTen TEXT NOT NULL:   Họ và tên đầy đủ
        //   MaLop TEXT NOT NULL:   Mã lớp (liên kết với bảng Classes)
        //   FaceVector TEXT:       Vector 128 chiều lưu dạng chuỗi số phân cách bởi dấu phẩy
        //                          Ví dụ: "0.123456,-0.987654,0.234567,..."
        //   AvatarPath TEXT:       Đường dẫn tuyệt đối tới ảnh chân dung gốc
        //
        // LƯU Ý: SQLite lưu FaceVector dưới dạng TEXT thay vì BLOB vì:
        //   - Dễ debug và kiểm tra bằng mắt
        //   - Kích thước nhỏ (128 * ~10 ký tự ≈ 1.28KB/sinh viên)
        //   - Hiệu năng đủ tốt cho quy mô lớp học
        // ============================================================
        QString createStudents = R"(
            CREATE TABLE IF NOT EXISTS Students (
                MSSV TEXT PRIMARY KEY,
                HoTen TEXT NOT NULL,
                MaLop TEXT NOT NULL,
                FaceVector TEXT,
                AvatarPath TEXT
            );
        )";

        if (!query.exec(createStudents)) {
            qCritical() << "[DB] Lỗi tạo bảng Students:" << query.lastError().text();
            return false;
        }

        // Tạo index trên cột MaLop để tăng tốc truy vấn tìm sinh viên theo lớp
        // Không có index: O(n) quét toàn bảng
        // Có index: O(log n) tìm kiếm theo B-tree của SQLite
        query.exec("CREATE INDEX IF NOT EXISTS idx_malop ON Students (MaLop);");

        qDebug() << "[DB] Bảng Students đã sẵn sàng.";

        // ============================================================
        // TẠO BẢNG AttendanceHistory (Lịch sử điểm danh)
        // Cấu trúc:
        //   ID INTEGER PRIMARY KEY AUTOINCREMENT: ID tự tăng (SQLite rowid)
        //   MSSV TEXT NOT NULL:          MSSV sinh viên điểm danh
        //   CheckInTime TEXT NOT NULL:   Thời gian định dạng "yyyy-MM-dd hh:mm:ss" (ISO8601)
        //   CapturedImagePath TEXT:      Đường dẫn ảnh chụp lúc điểm danh
        //   Confidence REAL:             Độ tin cậy nhận diện (0.0 - 1.0)
        //   FOREIGN KEY:                 Khóa ngoại tham chiếu MSSV trong Students
        //     ON UPDATE CASCADE:          Khi MSSV sinh viên thay đổi, lịch sử cũng cập nhật
        //     ON DELETE CASCADE:          Khi xóa sinh viên, lịch sử điểm danh TỰ ĐỘNG bị xóa
        // ============================================================
        QString createHistory = R"(
            CREATE TABLE IF NOT EXISTS AttendanceHistory (
                ID INTEGER PRIMARY KEY AUTOINCREMENT,
                MSSV TEXT NOT NULL,
                CheckInTime TEXT NOT NULL,
                CapturedImagePath TEXT,
                Confidence REAL DEFAULT 0.0,
                FOREIGN KEY (MSSV) REFERENCES Students(MSSV)
                    ON UPDATE CASCADE ON DELETE CASCADE
            );
        )";

        if (!query.exec(createHistory)) {
            qCritical() << "[DB] Lỗi tạo bảng AttendanceHistory:" << query.lastError().text();
            return false;
        }

        // Tạo index cho tốc độ lọc theo ngày (truy vấn "hôm nay" rất phổ biến)
        query.exec("CREATE INDEX IF NOT EXISTS idx_checkin_date ON AttendanceHistory (CheckInTime);");
        // Tạo index cho MSSV trong lịch sử (truy vấn theo sinh viên cụ thể)
        query.exec("CREATE INDEX IF NOT EXISTS idx_history_mssv ON AttendanceHistory (MSSV);");

        qDebug() << "[DB] Bảng AttendanceHistory đã sẵn sàng.";

        // ============================================================
        // TẠO BẢNG Accounts (Tài khoản giáo viên)
        // Cấu trúc đơn giản:
        //   Username TEXT PRIMARY KEY: Tên đăng nhập duy nhất
        //   Password TEXT NOT NULL:    Mật khẩu (hiện lưu plaintext - có thể nâng cấp thành hash)
        // ============================================================
        QString createAccounts = R"(
            CREATE TABLE IF NOT EXISTS Accounts (
                Username TEXT PRIMARY KEY,
                Password TEXT NOT NULL
            );
        )";
        if (!query.exec(createAccounts)) {
            qCritical() << "[DB] Lỗi tạo bảng Accounts:" << query.lastError().text();
            return false;
        }

        // ============================================================
        // SEED DATA: Tạo tài khoản admin mặc định nếu bảng chưa có tài khoản nào
        // Tài khoản mặc định: username="admin", password="123"
        // Người dùng có thể đổi mật khẩu hoặc tạo tài khoản mới qua giao diện
        // ============================================================
        QSqlQuery checkQuery("SELECT COUNT(*) FROM Accounts");
        if (checkQuery.next() && checkQuery.value(0).toInt() == 0) {
            // Bảng rỗng -> tạo tài khoản admin mặc định
            QSqlQuery insertQuery;
            insertQuery.prepare("INSERT INTO Accounts (Username, Password) VALUES (:user, :pass)");
            insertQuery.bindValue(":user", "admin");   // Tên đăng nhập mặc định
            insertQuery.bindValue(":pass", "123");     // Mật khẩu mặc định đơn giản
            if (!insertQuery.exec()) {
                qCritical() << "[DB] Lỗi tạo tài khoản admin mặc định:" << insertQuery.lastError().text();
            } else {
                qDebug() << "[DB] Đã khởi tạo tài khoản admin/123 mặc định thành công.";
            }
        }

        qDebug() << "[DB] Bảng Accounts đã sẵn sàng.";
        return true; // Tất cả bảng đã được tạo/kiểm tra thành công
    }

    // -----------------------------------------------------------------------
    // getLastError: Lấy thông báo lỗi SQL gần nhất
    // Dùng để debug khi truy vấn thất bại
    // -----------------------------------------------------------------------
    QString getLastError()
    {
        return s_db.lastError().text();
    }

    // -----------------------------------------------------------------------
    // vectorToString: Chuyển vector<float> 128 chiều thành chuỗi TEXT cho SQLite
    //
    // Định dạng output: "0.1234567,-0.2345678,0.3456789,..."
    // (Mỗi số float được làm tròn đến 7 chữ số thập phân để đảm bảo độ chính xác)
    //
    // Tại sao lưu dạng TEXT thay vì BLOB?
    //   - BLOB: Nhỏ hơn nhưng khó debug, cần xử lý endianness
    //   - TEXT: Dễ đọc, dễ backup, dễ inspect bằng SQLite Browser
    //   - 128 * 15 ký tự ≈ 1.9KB/sinh viên -> chấp nhận được
    // -----------------------------------------------------------------------
    QString vectorToString(const std::vector<float> &faceVector)
    {
        QStringList strList;
        strList.reserve(static_cast<int>(faceVector.size())); // Pre-allocate để tăng hiệu năng

        for (float val : faceVector) {
            // 'f' = định dạng fixed-point (không dùng ký hiệu khoa học e+)
            // 7 = số chữ số thập phân (đủ chính xác cho float 32-bit)
            strList.append(QString::number(val, 'f', 7));
        }

        // Ghép các số thành 1 chuỗi, phân cách bằng dấu phẩy
        return strList.join(",");
    }

    // -----------------------------------------------------------------------
    // stringToVector: Chuyển chuỗi TEXT từ SQLite về vector<float>
    // (Hàm nghịch đảo của vectorToString)
    //
    // Tham số: vectorString - Chuỗi "0.123,-0.456,0.789,..."
    // Trả về: std::vector<float> với 128 phần tử
    // -----------------------------------------------------------------------
    std::vector<float> stringToVector(const QString &vectorString)
    {
        std::vector<float> faceVector;
        if (vectorString.isEmpty()) return faceVector; // Trả về rỗng nếu chuỗi rỗng

        // Tách chuỗi bằng dấu phẩy, bỏ qua các phần tử rỗng
        QStringList strList = vectorString.split(",", Qt::SkipEmptyParts);
        faceVector.reserve(strList.size()); // Pre-allocate để tránh realloc nhiều lần

        for (const QString &str : strList) {
            faceVector.push_back(str.toFloat()); // Chuyển từng chuỗi số thành float
        }
        return faceVector;
    }

    // -----------------------------------------------------------------------
    // syncStudent: Đồng bộ thông tin sinh viên vào SQLite (THÊM MỚI hoặc CẬP NHẬT)
    //
    // Sử dụng kỹ thuật UPSERT của SQLite:
    //   "ON CONFLICT(MSSV) DO UPDATE SET ..."
    //   - Nếu MSSV chưa tồn tại -> INSERT (thêm mới)
    //   - Nếu MSSV đã tồn tại -> UPDATE (cập nhật thông tin và FaceVector mới)
    //
    // Lợi ích: 1 truy vấn duy nhất xử lý cả 2 tình huống, không cần kiểm tra trước.
    //
    // Tham số:
    //   mssv:       Mã số sinh viên (khóa chính)
    //   hoTen:      Họ và tên đầy đủ
    //   maLop:      Mã lớp học
    //   faceVector: Vector đặc trưng 128 chiều từ SFace
    //   avatarPath: Đường dẫn ảnh chân dung (tùy chọn)
    // Trả về: true nếu UPSERT thành công
    // -----------------------------------------------------------------------
    bool syncStudent(const QString &mssv,
                     const QString &hoTen,
                     const QString &maLop,
                     const std::vector<float> &faceVector,
                     const QString &avatarPath)
    {
        if (!s_db.isOpen()) {
            qWarning() << "[DB] Lỗi: Chưa kết nối CSDL.";
            return false;
        }

        // Chuyển vector float thành chuỗi TEXT để lưu vào SQLite
        QString vectorStr = vectorToString(faceVector);
        QSqlQuery query;

        // ============================================================
        // UPSERT QUERY: Insert hoặc Update tùy theo MSSV có tồn tại không
        // "excluded.ColumnName" trong phần UPDATE tham chiếu đến giá trị
        // mới được truyền vào (không phải giá trị cũ trong bảng)
        // ============================================================
        query.prepare(R"(
            INSERT INTO Students (MSSV, HoTen, MaLop, FaceVector, AvatarPath)
            VALUES (:mssv, :hoten, :malop, :facevector, :avatarpath)
            ON CONFLICT(MSSV) DO UPDATE SET
                HoTen = excluded.HoTen,
                MaLop = excluded.MaLop,
                FaceVector = excluded.FaceVector,
                AvatarPath = excluded.AvatarPath
        )");

        // Bind các giá trị vào placeholder (tránh SQL Injection)
        query.bindValue(":mssv", mssv);
        query.bindValue(":hoten", hoTen);
        query.bindValue(":malop", maLop);
        query.bindValue(":facevector", vectorStr);
        // Nếu avatarPath rỗng -> lưu NULL thay vì chuỗi rỗng vào SQLite
        query.bindValue(":avatarpath", avatarPath.isEmpty() ? QVariant() : avatarPath);

        if (!query.exec()) {
            qCritical() << "[DB] Lỗi đồng bộ sinh viên (" << mssv << "):" << query.lastError().text();
            return false;
        }

        qDebug() << "[DB] Đồng bộ SQLite thành công sinh viên:" << mssv << "-" << hoTen;
        return true;
    }

    // -----------------------------------------------------------------------
    // loadStudentsByClass: Tải danh sách sinh viên thuộc 1 lớp học cụ thể
    // Được dùng khi: bắt đầu camera với bộ lọc lớp, hiển thị thành viên lớp
    //
    // Tham số: maLop - Mã lớp học cần tải sinh viên
    // Trả về: Vector các StudentData, rỗng nếu lớp không có sinh viên hoặc lỗi
    // -----------------------------------------------------------------------
    std::vector<StudentData> loadStudentsByClass(const QString &maLop)
    {
        std::vector<StudentData> students;
        if (!s_db.isOpen()) return students; // Trả về rỗng nếu chưa kết nối

        QSqlQuery query;
        // Truy vấn có tham số để tránh SQL Injection
        query.prepare("SELECT MSSV, HoTen, MaLop, FaceVector, AvatarPath FROM Students WHERE MaLop = :malop");
        query.bindValue(":malop", maLop);

        if (!query.exec()) {
            qCritical() << "[DB] Lỗi tải sinh viên lớp" << maLop << ":" << query.lastError().text();
            return students;
        }

        // Duyệt qua từng hàng kết quả và đóng gói vào StudentData
        while (query.next()) {
            StudentData s;
            s.mssv       = query.value("MSSV").toString();    // MSSV sinh viên
            s.hoTen      = query.value("HoTen").toString();   // Họ và tên
            s.maLop      = query.value("MaLop").toString();   // Mã lớp
            // Chuyển đổi chuỗi TEXT về vector<float> khi đọc từ SQLite
            s.faceVector = stringToVector(query.value("FaceVector").toString());
            s.avatarPath = query.value("AvatarPath").toString(); // Đường dẫn ảnh
            students.push_back(s);
        }

        qDebug() << "[DB] Tải thành công" << students.size() << "sinh viên lớp" << maLop << "từ SQLite.";
        return students;
    }

    // -----------------------------------------------------------------------
    // loadAllStudents: Tải TOÀN BỘ danh sách sinh viên từ tất cả các lớp
    // Được dùng khi: Camera chạy chế độ "Tất cả lớp học", hoặc xem toàn bộ SV
    // -----------------------------------------------------------------------
    std::vector<StudentData> loadAllStudents()
    {
        std::vector<StudentData> students;
        if (!s_db.isOpen()) return students;

        QSqlQuery query;
        if (!query.exec("SELECT MSSV, HoTen, MaLop, FaceVector, AvatarPath FROM Students")) {
            qCritical() << "[DB] Lỗi tải toàn bộ sinh viên:" << query.lastError().text();
            return students;
        }

        while (query.next()) {
            StudentData s;
            s.mssv       = query.value("MSSV").toString();
            s.hoTen      = query.value("HoTen").toString();
            s.maLop      = query.value("MaLop").toString();
            s.faceVector = stringToVector(query.value("FaceVector").toString());
            s.avatarPath = query.value("AvatarPath").toString();
            students.push_back(s);
        }

        qDebug() << "[DB] Tải thành công" << students.size() << "sinh viên từ SQLite.";
        return students;
    }

    // -----------------------------------------------------------------------
    // recordCheckIn: Ghi nhận 1 lượt điểm danh thành công vào lịch sử
    //
    // Được gọi khi AI nhận diện thành công 1 khuôn mặt và sinh viên chưa điểm danh hôm nay.
    //
    // Tham số:
    //   mssv:              MSSV sinh viên vừa điểm danh
    //   capturedImagePath: Đường dẫn ảnh chụp lúc check-in (lưu làm bằng chứng)
    //   confidence:        Độ tin cậy nhận diện (0.0 - 1.0)
    // Trả về: true nếu ghi thành công
    // -----------------------------------------------------------------------
    bool recordCheckIn(const QString &mssv,
                       const QString &capturedImagePath,
                       double confidence)
    {
        if (!s_db.isOpen()) return false;

        QSqlQuery query;
        query.prepare(R"(
            INSERT INTO AttendanceHistory (MSSV, CheckInTime, CapturedImagePath, Confidence)
            VALUES (:mssv, :checkintime, :imgpath, :confidence)
        )");

        // Lấy thời gian hiện tại và định dạng thành ISO8601
        // Định dạng này cho phép SQLite so sánh ngày bằng hàm date()
        // Ví dụ: "2024-01-15 08:30:45"
        QString nowStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

        query.bindValue(":mssv", mssv);
        query.bindValue(":checkintime", nowStr);         // Thời gian điểm danh
        query.bindValue(":imgpath", capturedImagePath);   // Đường dẫn ảnh bằng chứng
        query.bindValue(":confidence", confidence);       // Độ tin cậy nhận diện

        if (!query.exec()) {
            qCritical() << "[DB] Lỗi ghi nhận lịch sử điểm danh cho MSSV (" << mssv << "):" << query.lastError().text();
            return false;
        }

        qDebug() << "[DB] Đã ghi điểm danh vào SQLite: MSSV" << mssv << "| Độ khớp:" << confidence;
        return true;
    }

    // -----------------------------------------------------------------------
    // hasCheckedInToday: Kiểm tra sinh viên đã điểm danh trong ngày hôm nay chưa
    //
    // Mục đích: Ngăn chặn ghi nhận trùng lặp khi camera phát hiện cùng 1 người
    //           nhiều lần trong 1 buổi học.
    //
    // SQL logic: So sánh date(CheckInTime) với date('now', 'localtime')
    //   - date(): Hàm SQLite trích xuất phần ngày từ chuỗi datetime
    //   - 'localtime': Modifier chuyển UTC về giờ địa phương (quan trọng ở VN +7)
    //
    // Tham số: mssv - Mã số sinh viên cần kiểm tra
    // Trả về: true nếu đã có ít nhất 1 lần điểm danh hôm nay
    // -----------------------------------------------------------------------
    bool hasCheckedInToday(const QString &mssv)
    {
        if (!s_db.isOpen()) return false;

        QSqlQuery query;
        query.prepare(R"(
            SELECT COUNT(*) FROM AttendanceHistory
            WHERE MSSV = :mssv AND date(CheckInTime) = date('now', 'localtime')
        )");
        query.bindValue(":mssv", mssv);

        // Nếu truy vấn thất bại hoặc không có kết quả -> trả về false (an toàn)
        if (!query.exec() || !query.next()) return false;

        // COUNT(*) > 0 nghĩa là đã điểm danh ít nhất 1 lần hôm nay
        return query.value(0).toInt() > 0;
    }

    // -----------------------------------------------------------------------
    // loadTodayAttendance: Tải danh sách tất cả lượt điểm danh trong ngày hôm nay
    //
    // Sử dụng LEFT JOIN giữa AttendanceHistory và Students để lấy tên sinh viên.
    // Sắp xếp theo thời gian giảm dần (mới nhất hiển thị đầu tiên).
    //
    // Trả về: Vector các AttendanceRecord sắp xếp mới -> cũ
    // -----------------------------------------------------------------------
    std::vector<AttendanceRecord> loadTodayAttendance()
    {
        std::vector<AttendanceRecord> records;
        if (!s_db.isOpen()) return records;

        QSqlQuery query;
        // LEFT JOIN để lấy HoTen từ bảng Students kết hợp với lịch sử điểm danh
        // WHERE date(...) = date('now', 'localtime'): Chỉ lấy dữ liệu hôm nay
        // ORDER BY CheckInTime DESC: Sắp xếp mới nhất lên đầu (để hiển thị trên UI)
        QString qStr = R"(
            SELECT a.ID, a.MSSV, s.HoTen, a.CheckInTime, a.CapturedImagePath, a.Confidence
            FROM AttendanceHistory a
            LEFT JOIN Students s ON a.MSSV = s.MSSV
            WHERE date(a.CheckInTime) = date('now', 'localtime')
            ORDER BY a.CheckInTime DESC
        )";

        if (!query.exec(qStr)) {
            qCritical() << "[DB] Lỗi tải lịch sử điểm danh hôm nay:" << query.lastError().text();
            return records;
        }

        while (query.next()) {
            AttendanceRecord r;
            r.id                = query.value("ID").toInt();
            r.mssv              = query.value("MSSV").toString();
            r.hoTen             = query.value("HoTen").toString();
            // Phân tích chuỗi datetime từ SQLite sang QDateTime
            r.checkInTime       = QDateTime::fromString(query.value("CheckInTime").toString(), "yyyy-MM-dd hh:mm:ss");
            r.capturedImagePath = query.value("CapturedImagePath").toString();
            r.confidence        = query.value("Confidence").toDouble();
            records.push_back(r);
        }

        qDebug() << "[DB] Đã tải" << records.size() << "lượt điểm danh hôm nay từ SQLite.";
        return records;
    }

    // -----------------------------------------------------------------------
    // resetTodayAttendance: Xóa toàn bộ lịch sử điểm danh trong ngày hôm nay
    //
    // Có 2 chế độ:
    //   1. maLop rỗng -> Xóa TẤT CẢ lịch sử hôm nay (tất cả lớp)
    //   2. maLop có giá trị -> Chỉ xóa lịch sử của sinh viên thuộc lớp đó
    //
    // Tham số: maLop - Mã lớp cần reset (để trống = reset toàn bộ)
    // Trả về: true nếu xóa thành công
    // -----------------------------------------------------------------------
    bool resetTodayAttendance(const QString &maLop)
    {
        if (!s_db.isOpen()) return false;
        QSqlQuery query;

        if (maLop.isEmpty()) {
            // Chế độ 1: Xóa toàn bộ lịch sử hôm nay
            query.prepare(R"(
                DELETE FROM AttendanceHistory 
                WHERE date(CheckInTime) = date('now', 'localtime')
            )");
        } else {
            // Chế độ 2: Chỉ xóa lịch sử sinh viên trong lớp được chọn
            // Subquery: Lấy MSSV của tất cả sinh viên trong lớp maLop
            query.prepare(R"(
                DELETE FROM AttendanceHistory 
                WHERE date(CheckInTime) = date('now', 'localtime')
                  AND MSSV IN (SELECT MSSV FROM Students WHERE MaLop = :malop)
            )");
            query.bindValue(":malop", maLop);
        }

        if (!query.exec()) {
            qCritical() << "[DB] Lỗi reset điểm danh hôm nay:" << query.lastError().text();
            return false;
        }
        qDebug() << "[DB] Đã reset điểm danh hôm nay cho lớp:" << (maLop.isEmpty() ? "Tất cả" : maLop);
        return true;
    }

    // ============================================================
    // TRIỂN KHAI CÁC HÀM QUẢN LÝ LỚP HỌC (CLASS MANAGEMENT)
    // Các hàm này xử lý CRUD (Create, Read, Update, Delete) cho lớp học
    // ============================================================

    // -----------------------------------------------------------------------
    // createClass: Tạo lớp học mới hoặc cập nhật thông tin lớp đã tồn tại (UPSERT)
    //
    // Tham số:
    //   maLop:       Mã lớp (khóa chính, ví dụ: "CS-101")
    //   tenLop:      Tên môn học (ví dụ: "Lập trình C++")
    //   phongHoc:    Phòng học (ví dụ: "A2-302")
    //   siSoDuKien:  Sĩ số tối đa dự kiến
    //   ngayHoc:     Ngày học định dạng "yyyy-MM-dd"
    //   gioBatDau:   Giờ bắt đầu "HH:mm"
    //   gioKetThuc:  Giờ kết thúc "HH:mm"
    // -----------------------------------------------------------------------
    bool createClass(const QString &maLop,
                     const QString &tenLop,
                     const QString &phongHoc,
                     int siSoDuKien,
                     const QString &ngayHoc,
                     const QString &gioBatDau,
                     const QString &gioKetThuc)
    {
        if (!s_db.isOpen()) return false;
        QSqlQuery query;
        // UPSERT tương tự syncStudent: Thêm mới hoặc cập nhật nếu MaLop trùng
        query.prepare(R"(
            INSERT INTO Classes (MaLop, TenLop, PhongHoc, SiSoDuKien, NgayHoc, GioBatDau, GioKetThuc)
            VALUES (:malop, :tenlop, :phonghoc, :sisodukien, :ngayhoc, :giobatdau, :giokethuc)
            ON CONFLICT(MaLop) DO UPDATE SET
                TenLop = excluded.TenLop,
                PhongHoc = excluded.PhongHoc,
                SiSoDuKien = excluded.SiSoDuKien,
                NgayHoc = excluded.NgayHoc,
                GioBatDau = excluded.GioBatDau,
                GioKetThuc = excluded.GioKetThuc
        )");
        query.bindValue(":malop", maLop);
        query.bindValue(":tenlop", tenLop);
        query.bindValue(":phonghoc", phongHoc);
        query.bindValue(":sisodukien", siSoDuKien);
        query.bindValue(":ngayhoc", ngayHoc);
        query.bindValue(":giobatdau", gioBatDau);
        query.bindValue(":giokethuc", gioKetThuc);

        if (!query.exec()) {
            qCritical() << "[DB] Lỗi tạo/đồng bộ lớp học (" << maLop << "):" << query.lastError().text();
            return false;
        }
        qDebug() << "[DB] Đã tạo/đồng bộ thành công lớp học:" << maLop << "-" << tenLop;
        return true;
    }

    // -----------------------------------------------------------------------
    // classExists: Kiểm tra mã lớp đã tồn tại trong CSDL chưa
    // Dùng để validate trước khi tạo lớp mới hoặc đăng ký sinh viên vào lớp
    //
    // Tham số: maLop - Mã lớp cần kiểm tra
    // Trả về: true nếu lớp đã tồn tại
    // -----------------------------------------------------------------------
    bool classExists(const QString &maLop)
    {
        if (!s_db.isOpen()) return false;
        QSqlQuery query;
        // Dùng COUNT(*) thay vì SELECT * để tối ưu hiệu năng (không cần đọc dữ liệu)
        query.prepare("SELECT COUNT(*) FROM Classes WHERE MaLop = :malop");
        query.bindValue(":malop", maLop);
        if (query.exec() && query.next()) {
            return query.value(0).toInt() > 0; // true nếu COUNT > 0
        }
        return false;
    }

    // -----------------------------------------------------------------------
    // loadAllClasses: Tải toàn bộ danh sách lớp học, sắp xếp theo ngày và giờ học
    //
    // Sắp xếp: ORDER BY NgayHoc ASC, GioBatDau ASC
    //   -> Lớp học sớm nhất hiển thị đầu tiên (tiện cho báo cáo đi trễ/vắng)
    //
    // Trả về: Vector các ClassData với đầy đủ thông tin lớp học
    // -----------------------------------------------------------------------
    std::vector<ClassData> loadAllClasses()
    {
        std::vector<ClassData> classes;
        if (!s_db.isOpen()) return classes;

        QSqlQuery query;
        if (query.exec("SELECT MaLop, TenLop, PhongHoc, SiSoDuKien, NgayHoc, GioBatDau, GioKetThuc, TrangThai FROM Classes ORDER BY NgayHoc ASC, GioBatDau ASC")) {
            while (query.next()) {
                ClassData c;
                c.maLop      = query.value("MaLop").toString();
                c.tenLop     = query.value("TenLop").toString();
                c.phongHoc   = query.value("PhongHoc").toString();
                c.siSoDuKien = query.value("SiSoDuKien").toInt();
                c.ngayHoc    = query.value("NgayHoc").toString();
                c.gioBatDau  = query.value("GioBatDau").toString();
                c.gioKetThuc = query.value("GioKetThuc").toString();
                c.trangThai  = query.value("TrangThai").toInt(); // 0=Chưa xong, 1=Đã xong
                classes.push_back(c);
            }
        } else {
            qCritical() << "[DB] Lỗi tải danh sách lớp học:" << query.lastError().text();
        }
        return classes;
    }

    // -----------------------------------------------------------------------
    // countStudentsInClass: Đếm số sinh viên thực tế đã đăng ký FaceID trong lớp
    // Dùng để hiển thị "X/Y sinh viên" trên thẻ lớp học trong giao diện
    //
    // Tham số: maLop - Mã lớp cần đếm
    // Trả về: Số sinh viên đã đăng ký FaceID trong lớp đó
    // -----------------------------------------------------------------------
    int countStudentsInClass(const QString &maLop)
    {
        if (!s_db.isOpen()) return 0;
        QSqlQuery query;
        query.prepare("SELECT COUNT(*) FROM Students WHERE MaLop = :malop");
        query.bindValue(":malop", maLop);
        if (query.exec() && query.next()) {
            return query.value(0).toInt();
        }
        return 0;
    }

    // -----------------------------------------------------------------------
    // deleteClass: Xóa lớp học cùng TOÀN BỘ dữ liệu liên quan
    //
    // Thực hiện trong 1 TRANSACTION để đảm bảo tính toàn vẹn dữ liệu:
    //   Nếu bất kỳ bước nào thất bại -> ROLLBACK toàn bộ, không xóa nửa chừng
    //
    // Thứ tự xóa (quan trọng vì ràng buộc khóa ngoại):
    //   1. Xóa lịch sử điểm danh của sinh viên trong lớp (AttendanceHistory)
    //   2. Xóa tất cả sinh viên thuộc lớp (Students)
    //   3. Xóa bản ghi lớp học (Classes)
    //
    // Tham số: maLop - Mã lớp cần xóa
    // Trả về: true nếu xóa thành công hoàn toàn
    // -----------------------------------------------------------------------
    bool deleteClass(const QString &maLop)
    {
        if (!s_db.isOpen()) return false;
        QSqlQuery query;
        
        // Bắt đầu transaction - các thao tác sau đây là 1 khối nguyên tử (atomic)
        s_db.transaction();
        
        // Bước 1: Xóa lịch sử điểm danh của tất cả sinh viên trong lớp
        // Subquery: Lấy MSSV từ bảng Students với điều kiện MaLop
        query.prepare("DELETE FROM AttendanceHistory WHERE MSSV IN (SELECT MSSV FROM Students WHERE MaLop = :malop)");
        query.bindValue(":malop", maLop);
        if (!query.exec()) {
            s_db.rollback(); // Hoàn tác mọi thay đổi nếu bước này thất bại
            qCritical() << "[DB] Lỗi xóa lịch sử điểm danh khi xóa lớp:" << query.lastError().text();
            return false;
        }

        // Bước 2: Xóa tất cả sinh viên thuộc lớp học này
        query.prepare("DELETE FROM Students WHERE MaLop = :malop");
        query.bindValue(":malop", maLop);
        if (!query.exec()) {
            s_db.rollback(); // Hoàn tác nếu bước 2 thất bại
            qCritical() << "[DB] Lỗi xóa sinh viên khi xóa lớp:" << query.lastError().text();
            return false;
        }

        // Bước 3: Xóa bản ghi lớp học khỏi bảng Classes
        query.prepare("DELETE FROM Classes WHERE MaLop = :malop");
        query.bindValue(":malop", maLop);
        if (!query.exec()) {
            s_db.rollback(); // Hoàn tác nếu bước 3 thất bại
            qCritical() << "[DB] Lỗi xóa lớp học:" << query.lastError().text();
            return false;
        }
        
        // Commit transaction: Xác nhận ghi tất cả 3 bước vào đĩa
        s_db.commit();
        qDebug() << "[DB] Đã xóa thành công lớp học" << maLop << "và các sinh viên/lịch sử liên quan.";
        return true;
    }

    // -----------------------------------------------------------------------
    // updateClassStatus: Cập nhật trạng thái hoàn thành của lớp học
    //
    // Tham số:
    //   maLop:  Mã lớp cần cập nhật
    //   status: 0 = Chưa hoàn thành, 1 = Đã hoàn thành
    // -----------------------------------------------------------------------
    bool updateClassStatus(const QString &maLop, int status)
    {
        if (!s_db.isOpen()) return false;
        QSqlQuery query;
        query.prepare("UPDATE Classes SET TrangThai = :status WHERE MaLop = :malop");
        query.bindValue(":status", status);
        query.bindValue(":malop", maLop);
        if (!query.exec()) {
            qCritical() << "[DB] Lỗi cập nhật trạng thái lớp học:" << query.lastError().text();
            return false;
        }
        qDebug() << "[DB] Đã cập nhật trạng thái lớp học" << maLop << "thành" << status;
        return true;
    }

    // -----------------------------------------------------------------------
    // getCheckInTimeForDate: Lấy thời gian điểm danh của sinh viên vào ngày cụ thể
    //
    // Dùng trong báo cáo "Đi trễ & Vắng mặt" để kiểm tra sinh viên có mặt không
    // và nếu có thì điểm danh lúc mấy giờ.
    //
    // Tham số:
    //   mssv:    MSSV sinh viên cần tra cứu
    //   dateStr: Ngày cần kiểm tra dạng "yyyy-MM-dd" (ví dụ: "2024-01-15")
    //
    // Trả về: QDateTime của lần điểm danh (hợp lệ nếu có, invalid nếu vắng mặt)
    // -----------------------------------------------------------------------
    QDateTime getCheckInTimeForDate(const QString &mssv, const QString &dateStr)
    {
        if (!s_db.isOpen()) return QDateTime(); // Invalid QDateTime = vắng mặt

        QSqlQuery query;
        // Dùng LIKE với prefix ngày để tìm bất kỳ record nào trong ngày đó
        // Ví dụ: dateStr = "2024-01-15" -> LIKE "2024-01-15%"
        query.prepare("SELECT CheckInTime FROM AttendanceHistory WHERE MSSV = :mssv AND CheckInTime LIKE :datePrefix");
        query.bindValue(":mssv", mssv);
        query.bindValue(":datePrefix", dateStr + "%"); // Pattern matching: ngày + bất kỳ giờ nào

        if (query.exec() && query.next()) {
            // Tìm thấy bản ghi -> parse chuỗi datetime và trả về
            return QDateTime::fromString(query.value(0).toString(), "yyyy-MM-dd hh:mm:ss");
        }
        // Không tìm thấy -> trả về QDateTime không hợp lệ (biểu thị vắng mặt)
        return QDateTime();
    }

    // -----------------------------------------------------------------------
    // createAccount: Tạo tài khoản giáo viên mới
    //
    // Tham số:
    //   username: Tên đăng nhập (phải duy nhất trong bảng Accounts)
    //   password: Mật khẩu (hiện lưu plaintext, nên nâng cấp thành bcrypt/sha256)
    // Trả về: true nếu tạo thành công (false nếu username đã tồn tại)
    // -----------------------------------------------------------------------
    bool createAccount(const QString &username, const QString &password)
    {
        if (!s_db.isOpen()) return false;
        QSqlQuery query;
        // INSERT sẽ thất bại nếu Username đã tồn tại (PRIMARY KEY constraint)
        query.prepare("INSERT INTO Accounts (Username, Password) VALUES (:user, :pass)");
        query.bindValue(":user", username);
        query.bindValue(":pass", password);
        if (!query.exec()) {
            qCritical() << "[DB] Lỗi tạo tài khoản mới:" << query.lastError().text();
            return false;
        }
        return true;
    }

    // -----------------------------------------------------------------------
    // validateLogin: Xác thực thông tin đăng nhập của giáo viên
    //
    // Logic: Tìm tài khoản theo username, so sánh password
    // LƯU Ý BẢO MẬT: Password hiện đang lưu plaintext. Trong thực tế nên:
    //   1. Hash password với salt trước khi lưu (bcrypt, scrypt, argon2)
    //   2. Chỉ lưu hash vào CSDL
    //   3. Khi login: hash password nhập vào và so sánh với hash trong DB
    //
    // Tham số:
    //   username: Tên đăng nhập
    //   password: Mật khẩu người dùng nhập vào
    // Trả về: true nếu tài khoản tồn tại và mật khẩu khớp
    // -----------------------------------------------------------------------
    bool validateLogin(const QString &username, const QString &password)
    {
        if (!s_db.isOpen()) return false;
        QSqlQuery query;
        // Chỉ tìm theo username trước, sau đó so sánh password bằng C++
        // (không dùng WHERE Password=:pass để tránh timing attack đơn giản)
        query.prepare("SELECT Password FROM Accounts WHERE Username = :user");
        query.bindValue(":user", username);
        if (query.exec() && query.next()) {
            // So sánh mật khẩu (trực tiếp vì đang lưu plaintext)
            return query.value(0).toString() == password;
        }
        return false; // Username không tồn tại trong hệ thống
    }


} // namespace Database
