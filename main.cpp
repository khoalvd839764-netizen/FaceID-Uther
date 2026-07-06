// ============================================================================
// FACIEID DASHBOARD - Hệ thống điểm danh bằng nhận diện khuôn mặt
// Trường Đại học Giao thông Vận tải TP.HCM (UTH)
// ============================================================================
// FILE: main.cpp
// MÔ TẢ:
//   Đây là file nguồn duy nhất chứa toàn bộ logic giao diện người dùng (UI).
//   Toàn bộ ứng dụng được xây dựng theo phong cách lập trình THỦ TỤC (Procedural),
//   không sử dụng class hay kế thừa OOP. Thay vào đó:
//   - Các hàm helper tạo widget được định nghĩa là hàm tự do (free functions)
//   - Logic sự kiện (event handling) sử dụng Lambda C++11 kết nối trực tiếp vào signals của Qt
//   - Biến trạng thái ứng dụng (state) được khai báo cục bộ trong hàm main()
//
// CẤU TRÚC CHƯƠNG TRÌNH:
//   1. Các hàm helper tạo widget UI (createStatCard, createAttendanceItemWidget,...)
//   2. Các dialog (showAddStudentDialog, showCreateClassDialog,...)
//   3. Các hàm làm mới dữ liệu UI (refreshClassGrid, refreshLateAbsentReport,...)
//   4. Hàm showLoginDialog (xác thực đăng nhập)
//   5. Hàm main() - Điểm khởi đầu chương trình, xây dựng toàn bộ cửa sổ chính
// ============================================================================

// === THƯ VIỆN QT UI ===
#include <QApplication>     // Qt: Lớp quản lý vòng lặp sự kiện ứng dụng (event loop)
#include <QMainWindow>      // Qt: Cửa sổ chính với menu bar, toolbar, status bar
#include <QWidget>          // Qt: Widget cơ bản - lớp cha của mọi widget giao diện
#include <QHBoxLayout>      // Qt: Layout ngang (sắp xếp widget theo hàng ngang)
#include <QVBoxLayout>      // Qt: Layout dọc (sắp xếp widget theo cột dọc)
#include <QGridLayout>      // Qt: Layout lưới (sắp xếp widget theo hàng và cột)
#include <QLabel>           // Qt: Widget hiển thị văn bản hoặc hình ảnh
#include <QPushButton>      // Qt: Nút nhấn
#include <QListWidget>      // Qt: Danh sách có thể cuộn và chọn
#include <QListWidgetItem>  // Qt: Phần tử trong QListWidget
#include <QFrame>           // Qt: Widget khung có thể tùy chỉnh border
#include <QLineEdit>        // Qt: Ô nhập văn bản 1 dòng
#include <QComboBox>        // Qt: Danh sách thả xuống (dropdown)
#include <QCheckBox>        // Qt: Hộp kiểm (checkbox)
#include <QListWidget>      // Qt: Danh sách widget (khai báo lại - không ảnh hưởng)
#include <QFileDialog>      // Qt: Hộp thoại chọn file/thư mục
#include <QMessageBox>      // Qt: Hộp thoại thông báo (info, warning, error, question)
#include <QPixmap>          // Qt: Chứa và xử lý dữ liệu ảnh để hiển thị
#include <QDateTime>        // Qt: Xử lý ngày giờ (lấy thời gian hiện tại, format...)
#include "backend/Exporter.h" // Chức năng kết xuất xuất báo cáo PDF
#include <QScrollArea>      // Qt: Vùng có thể cuộn khi nội dung vượt kích thước
#include <QDialog>          // Qt: Cửa sổ dialog modal (chặn tương tác với cửa sổ cha)
#include <QFormLayout>      // Qt: Layout form (label + input field xếp theo cặp)
#include <QFileInfo>        // Qt: Lấy thông tin về file (tên, kích thước, đường dẫn...)
#include <QTimer>           // Qt: Bộ đếm thời gian (dùng khởi tạo trễ và FPS camera)
#include <QDir>             // Qt: Thao tác với thư mục (tạo, tìm kiếm, đường dẫn...)
#include <QSqlQuery>        // Qt: Thực thi truy vấn SQL
#include <QSqlError>        // Qt: Xử lý lỗi SQLite
#include <QDebug>           // Qt: Xuất log debug ra console
#include <QStackedWidget>   // Qt: Widget chứa nhiều trang, hiển thị 1 trang tại 1 thời điểm
#include <QDateEdit>        // Qt: Ô nhập ngày tháng với calendar popup
#include <QTimeEdit>        // Qt: Ô nhập giờ phút
#include <QMenu>            // Qt: Menu ngữ cảnh (context menu)
#include <QAction>          // Qt: Hành động trong menu hoặc toolbar
#include <QTextEdit>        // Qt: Ô nhập văn bản nhiều dòng
#include <QPainter>         // Qt: Vẽ đồ họa 2D (dùng để crop avatar hình tròn)

// === THƯ VIỆN BACKEND ===
#include <opencv2/core.hpp>     // OpenCV: Cấu trúc dữ liệu cốt lõi (Mat, Point, Rect...)
#include "backend/Database.h"  // Module quản lý cơ sở dữ liệu SQLite
#include "backend/Camera.h"    // Module điều khiển webcam và đọc frame
#include "backend/Face.h"      // Module AI nhận diện khuôn mặt (YuNet + SFace)

// ============================================================================
// CÁC HÀM HELPER ĐỂ TẠO CÁC WIDGET GIAO DIỆN THỦ TỤC (Không dùng OOP Class)
// ============================================================================

// -----------------------------------------------------------------------
// createStatCard: Tạo thẻ thống kê (Stat Card) phong cách Glassmorphism
//
// Mỗi thẻ hiển thị 1 chỉ số thống kê (Sĩ số, Có mặt, Đi muộn, Vắng mặt)
// Thẻ có màu border khác nhau để phân biệt trực quan.
//
// Tham số:
//   title:        Tiêu đề thẻ (ví dụ: "CÓ MẶT")
//   value:        Giá trị ban đầu (thường là "0")
//   borderStyle:  Màu CSS của đường viền (ví dụ: "#059669" - xanh lá)
//   textColor:    Màu CSS của chữ số lớn (ví dụ: "#10B981" - xanh nhạt)
//   lblValueOut:  [OUT] Con trỏ tham chiếu tới QLabel hiển thị số - dùng để cập nhật sau
//
// Trả về: QWidget* là thẻ thống kê hoàn chỉnh, sẵn sàng thêm vào layout
// -----------------------------------------------------------------------
QWidget* createStatCard(const QString &title, const QString &value,
                        const QString &borderStyle, const QString &textColor,
                        QLabel *&lblValueOut)
{
    // Tạo QFrame làm container của thẻ (có border-radius đẹp hơn QWidget)
    QFrame *card = new QFrame();
    card->setFixedSize(110, 80); // Kích thước cố định 110x80 pixel

    // Áp dụng style Dark Mode với border màu tùy theo loại thống kê
    card->setStyleSheet(QString(
        "QFrame {"
        "  background-color: #1E293B;"  // Màu nền tối (dark blue-grey)
        "  border: 1px solid %1;"        // Màu border được truyền vào
        "  border-radius: 10px;"         // Bo góc 10px
        "}"
    ).arg(borderStyle)); // Thay thế %1 bằng borderStyle

    // Layout dọc chứa tiêu đề (nhỏ) và số liệu (to)
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 10, 10, 10); // Padding 10px mỗi phía
    layout->setSpacing(4); // Khoảng cách giữa tiêu đề và số

    // Label tiêu đề thẻ (font nhỏ, màu mờ)
    QLabel *lblTitle = new QLabel(title, card);
    lblTitle->setStyleSheet("font-size: 10px; color: #94A3B8; font-weight: bold; border: none; background: transparent;");
    lblTitle->setAlignment(Qt::AlignLeft | Qt::AlignTop); // Căn trái trên

    // Label hiển thị số liệu lớn (font to, màu nổi bật)
    lblValueOut = new QLabel(value, card);
    lblValueOut->setStyleSheet(QString(
        "font-size: 24px; font-weight: bold; color: %1; border: none; background: transparent;"
    ).arg(textColor)); // Màu số được truyền vào
    lblValueOut->setAlignment(Qt::AlignLeft | Qt::AlignBottom); // Căn trái dưới

    // Thêm 2 label vào layout theo thứ tự từ trên xuống
    layout->addWidget(lblTitle);
    layout->addWidget(lblValueOut);

    return card; // Trả về widget thẻ hoàn chỉnh
}

// -----------------------------------------------------------------------
// createAttendanceItemWidget: Tạo 1 dòng hiển thị lịch sử điểm danh
//
// Mỗi dòng gồm: [Avatar hình tròn] [Tên + MSSV] [Badge trễ + Giờ]
// Hiển thị trong QListWidget ở cột phải của giao diện chính.
//
// Tham số:
//   name:         Tên sinh viên
//   id:           MSSV sinh viên
//   maLop:        Mã lớp học
//   time:         Chuỗi giờ điểm danh (ví dụ: "08:30:45")
//   status:       Chuỗi trạng thái (ví dụ: "ĐÃ XÁC THỰC")
//   statusColor:  Màu CSS của badge trạng thái (ví dụ: "#10B981")
//   lateMinutes:  Số phút đi trễ (0 = đúng giờ, > 0 = trễ)
//   capturedPath: Đường dẫn ảnh chụp lúc điểm danh (bằng chứng)
//   avatarPath:   Đường dẫn ảnh chân dung gốc (nếu có -> avatar tròn)
//
// Trả về: QWidget* là 1 dòng hoàn chỉnh để đặt vào QListWidgetItem
// -----------------------------------------------------------------------
QWidget* createAttendanceItemWidget(const QString &name, const QString &id,
                                    const QString &maLop,
                                    const QString &time, const QString &status,
                                    const QString &statusColor,
                                    int lateMinutes = 0,
                                    const QString &capturedPath = "",
                                    const QString &avatarPath = "")
{
    QWidget *widget = new QWidget();
    widget->setStyleSheet("background: transparent;");

    // Lưu metadata vào properties để popup chi tiết truy xuất khi click
    widget->setProperty("sv_name",    name);
    widget->setProperty("sv_id",      id);
    widget->setProperty("sv_maLop",   maLop);
    widget->setProperty("sv_time",    time);
    widget->setProperty("sv_late",    lateMinutes);
    widget->setProperty("sv_imgPath", capturedPath);

    // Layout ngang chứa: [Avatar] [Thông tin] [Thời gian/Trạng thái]
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(10);

    // ============================================================
    // AVATAR HÌNH TRÒN
    // Ưu tiên ảnh chụp lúc điểm danh (capturedPath), fallback ảnh chân dung
    // gốc (avatarPath), cuối cùng là chữ cái đầu tên nếu không có ảnh nào
    // ============================================================
    QLabel *avatar = new QLabel(widget);
    avatar->setFixedSize(38, 38);

    QString imgToShow = capturedPath;
    if (imgToShow.isEmpty() || !QFile::exists(imgToShow)) imgToShow = avatarPath;

    if (!imgToShow.isEmpty() && QFile::exists(imgToShow)) {
        // Hiển thị ảnh chụp điểm danh (ảnh bằng chứng)
        QPixmap pix(imgToShow);
        pix = pix.scaled(38, 38, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        // Crop hình tròn bằng mask
        QPixmap rounded(38, 38);
        rounded.fill(Qt::transparent);
        QPainter painter(&rounded);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QBrush(pix));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(0, 0, 38, 38);
        painter.end();
        avatar->setPixmap(rounded);
        // Border màu tùy theo trạng thái: xanh lá = đúng giờ, cam = trễ
        QString borderColor = (lateMinutes > 0) ? "#F59E0B" : "#10B981";
        avatar->setStyleSheet(QString("border-radius: 19px; border: 2px solid %1;").arg(borderColor));
    } else {
        // Fallback: Chữ cái đầu với gradient
        avatar->setStyleSheet(
            lateMinutes > 0
            ? "background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #D97706,stop:1 #B45309); border-radius: 19px; color: white; font-weight: bold; font-size: 14px; border: 2px solid #F59E0B;"
            : "background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #059669,stop:1 #047857); border-radius: 19px; color: white; font-weight: bold; font-size: 14px; border: 2px solid #10B981;"
        );
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setText(name.left(1).toUpper());
    }

    // ============================================================
    // THÔNG TIN SINH VIÊN (Cột giữa)
    // Dòng 1: Tên + badge "Trễ Xp" nếu đi trễ
    // Dòng 2: MSSV
    // ============================================================
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);

    // Hàng tên + badge trễ
    QHBoxLayout *nameRow = new QHBoxLayout();
    nameRow->setSpacing(5);
    QLabel *lblName = new QLabel(name, widget);
    lblName->setStyleSheet("font-weight: bold; font-size: 13px; color: #F1F5F9;");
    nameRow->addWidget(lblName);

    if (lateMinutes > 0) {
        // Badge hiển thị số phút trễ (màu cam nổi bật)
        QLabel *lateBadge = new QLabel(QString("Trễ %1p").arg(lateMinutes), widget);
        lateBadge->setStyleSheet(
            "background-color: #D97706; color: white; font-size: 8px; font-weight: bold;"
            " padding: 1px 5px; border-radius: 3px; border: none;"
        );
        nameRow->addWidget(lateBadge);
    }
    nameRow->addStretch();
    infoLayout->addLayout(nameRow);

    QLabel *lblId = new QLabel(QString("%1 · %2").arg(id, maLop.isEmpty() ? "—" : maLop), widget);
    lblId->setStyleSheet("font-size: 11px; color: #94A3B8;");
    infoLayout->addWidget(lblId);

    // ============================================================
    // THỜI GIAN & TRẠNG THÁI (Cột phải, căn phải)
    // ============================================================
    QVBoxLayout *timeLayout = new QVBoxLayout();
    timeLayout->setSpacing(2);
    timeLayout->setAlignment(Qt::AlignRight);

    QLabel *lblStatus = new QLabel(status, widget);
    lblStatus->setStyleSheet(QString("font-size: 9px; font-weight: bold; color: %1;").arg(statusColor));

    QLabel *lblTime = new QLabel(time, widget);
    QString timeColor = (lateMinutes > 0) ? "#F59E0B" : "#3B82F6";
    lblTime->setStyleSheet(QString("font-size: 12px; font-weight: bold; color: %1;").arg(timeColor));

    timeLayout->addWidget(lblStatus);
    timeLayout->addWidget(lblTime);

    layout->addWidget(avatar);
    layout->addLayout(infoLayout);
    layout->addStretch();
    layout->addLayout(timeLayout);

    return widget;
}

// -----------------------------------------------------------------------
// showAddStudentDialog: Dialog đăng ký sinh viên mới với trích xuất FaceID
//
// Quy trình đăng ký:
//   1. Người dùng nhập MSSV, Họ tên, Mã lớp
//   2. Chọn ảnh chân dung từ máy tính
//   3. Nhấn "Đăng ký FaceID":
//      a. Kiểm tra lớp học tồn tại
//      b. Phát hiện khuôn mặt trong ảnh (YuNet AI)
//      c. Trích xuất vector 128 chiều (SFace AI)
//      d. Lưu vào SQLite bằng syncStudent()
//
// Tham số:
//   parent:           Widget cha của dialog
//   preFilledClassCode: Nếu mở từ Chi tiết lớp, mã lớp được điền sẵn và khóa lại
//
// Trả về: true nếu đăng ký thành công
// -----------------------------------------------------------------------
bool showAddStudentDialog(QWidget *parent, const QString &preFilledClassCode = "")
{
    // Tạo dialog modal với tiêu đề và kích thước cố định
    QDialog dialog(parent);
    dialog.setWindowTitle("Đăng ký FaceID Sinh Viên mới");
    dialog.setFixedSize(400, 280);

    // Áp dụng theme Dark Mode cho dialog
    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #1E293B;
            border: 1px solid #334155;
        }
        QLabel {
            font-size: 12px;
            color: #E2E8F0;
            font-weight: bold;
        }
        QLineEdit {
            padding: 6px;
            border: 1px solid #475569;
            border-radius: 6px;
            font-size: 12px;
            background-color: #0F172A;
            color: #FFFFFF;
        }
        QLineEdit:focus {
            border: 1px solid #3B82F6;
        }
    )");

    // Layout chính dọc của dialog
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(14);
    layout->setContentsMargins(20, 20, 20, 20);
    
    // ============================================================
    // FORM NHẬP THÔNG TIN SINH VIÊN
    // QFormLayout tự động tạo cặp Label-Input theo hàng ngang
    // ============================================================
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    // Ô nhập MSSV (Mã số sinh viên)
    QLineEdit *inputMssv = new QLineEdit(&dialog);
    inputMssv->setPlaceholderText("Ví dụ: 20110123");

    // Ô nhập họ tên đầy đủ
    QLineEdit *inputHoTen = new QLineEdit(&dialog);
    inputHoTen->setPlaceholderText("Ví dụ: Nguyễn Văn A");

    // Ô nhập mã lớp
    QLineEdit *inputMaLop = new QLineEdit(&dialog);
    inputMaLop->setPlaceholderText("Ví dụ: KTPM1");

    // Nếu dialog được mở từ trang "Chi tiết lớp học":
    // -> Điền sẵn mã lớp và khóa lại để người dùng không thay đổi
    if (!preFilledClassCode.isEmpty()) {
        inputMaLop->setText(preFilledClassCode);
        inputMaLop->setReadOnly(true); // Chỉ đọc - không thể chỉnh sửa
        inputMaLop->setStyleSheet("background-color: #334155; color: #94A3B8; border: 1px solid #475569;");
    }

    formLayout->addRow("MSSV:", inputMssv);
    formLayout->addRow("Họ Tên:", inputHoTen);
    formLayout->addRow("Mã Lớp:", inputMaLop);
    layout->addLayout(formLayout);

    // ============================================================
    // KHU VỰC CHỌN ẢNH CHÂN DUNG
    // Nút "Chọn ảnh..." + Label hiển thị tên file đã chọn
    // ============================================================
    QHBoxLayout *fileLayout = new QHBoxLayout();

    QPushButton *btnSelectImg = new QPushButton("Chọn ảnh chân dung...", &dialog);
    btnSelectImg->setStyleSheet(R"(
        QPushButton {
            background-color: #334155;
            color: #E2E8F0;
            border: 1px solid #475569;
            padding: 6px 12px;
            border-radius: 6px;
            font-size: 11px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #475569;
        }
    )");

    // Label hiển thị tên file ảnh đã chọn (ban đầu = "Chưa chọn ảnh...")
    QLabel *lblImgPath = new QLabel("Chưa chọn ảnh...", &dialog);
    lblImgPath->setStyleSheet("color: #94A3B8; font-size: 11px; font-weight: normal;");

    fileLayout->addWidget(btnSelectImg);
    fileLayout->addWidget(lblImgPath);
    fileLayout->addStretch();
    layout->addLayout(fileLayout);

    // ============================================================
    // HÀNG NÚT HỦY / ĐĂng KÝ FACEID
    // ============================================================
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch(); // Đẩy các nút sang phải

    // Nút Hủy - đóng dialog không lưu
    QPushButton *btnCancel = new QPushButton("Hủy", &dialog);
    btnCancel->setStyleSheet(R"(
        QPushButton {
            background-color: #334155;
            color: #94A3B8;
            padding: 6px 16px;
            border-radius: 6px;
            border: none;
            font-size: 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #475569;
            color: #F8FAFC;
        }
    )");

    // Nút Đăng ký - thực hiện toàn bộ pipeline AI và lưu DB
    QPushButton *btnSave = new QPushButton("Đăng ký FaceID", &dialog);
    btnSave->setStyleSheet(R"(
        QPushButton {
            background-color: #2563EB;
            color: white;
            padding: 6px 16px;
            border-radius: 6px;
            border: none;
            font-size: 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1D4ED8;
        }
    )");

    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);
    layout->addLayout(btnLayout);

    // Biến lưu đường dẫn ảnh được chọn (capture vào lambda)
    QString selectedImgPath = "";

    // ============================================================
    // SỰ KIỆN: Nhấn nút "Chọn ảnh chân dung..."
    // Mở hộp thoại chọn file, lọc các định dạng ảnh phổ biến
    // ============================================================
    QObject::connect(btnSelectImg, &QPushButton::clicked, [&]() {
        QString file = QFileDialog::getOpenFileName(&dialog, "Chọn ảnh chân dung sinh viên", "", "Images (*.png *.jpg *.jpeg)");
        if (!file.isEmpty()) {
            selectedImgPath = file;          // Lưu đường dẫn đầy đủ
            QFileInfo info(file);
            lblImgPath->setText(info.fileName()); // Chỉ hiển thị tên file (không hiển thị đường dẫn đầy đủ)
            lblImgPath->setToolTip(file);    // Hiển thị đường dẫn đầy đủ khi hover
        }
    });

    // Nút Hủy: Đóng dialog và trả về QDialog::Rejected
    QObject::connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);

    // ============================================================
    // SỰ KIỆN: Nhấn nút "Đăng ký FaceID" - Pipeline AI hoàn chỉnh
    // ============================================================
    QObject::connect(btnSave, &QPushButton::clicked, [&]() {
        // Đọc và làm sạch dữ liệu nhập vào (trimmed loại bỏ khoảng trắng đầu/cuối)
        QString mssv = inputMssv->text().trimmed();
        QString hoTen = inputHoTen->text().trimmed();
        QString maLop = inputMaLop->text().trimmed();

        // Validate: Tất cả trường đều phải có dữ liệu
        if (mssv.isEmpty() || hoTen.isEmpty() || maLop.isEmpty() || selectedImgPath.isEmpty()) {
            QMessageBox::warning(&dialog, "Thông báo", "Vui lòng nhập đầy đủ thông tin và chọn ảnh chân dung!");
            return; // Dừng lambda, không đóng dialog
        }

        // Validate: Mã lớp phải tồn tại trong CSDL (ràng buộc toàn vẹn dữ liệu)
        if (!Database::classExists(maLop)) {
            QMessageBox::warning(&dialog, "Không tìm thấy lớp học",
                "Mã lớp học này chưa tồn tại trong danh sách lớp học!\n\n"
                "Vui lòng tạo lớp học trước trong mục Quản lý lớp học.");
            return;
        }

        // Load ảnh chân dung vào QImage để xử lý
        QImage img(selectedImgPath);
        if (img.isNull()) {
            QMessageBox::critical(&dialog, "Lỗi", "Không thể mở file ảnh chân dung.");
            return;
        }

        // ============================================================
        // AI STEP 1: Chuyển QImage -> cv::Mat để OpenCV xử lý
        // Face::qImageToMat thực hiện chuyển đổi định dạng RGB->BGR
        // ============================================================
        cv::Mat frame = Face::qImageToMat(img);
        if (frame.empty()) {
            QMessageBox::critical(&dialog, "Lỗi", "Dữ liệu pixel ảnh không hợp lệ.");
            return;
        }

        // ============================================================
        // AI STEP 2: Phát hiện khuôn mặt trong ảnh tĩnh (YuNet)
        // detectFaces trả về danh sách vị trí QRect của các mặt tìm thấy
        // ============================================================
        std::vector<QRect> faces = Face::detectFaces(frame);
        if (faces.empty()) {
            QMessageBox::warning(&dialog, "Không tìm thấy mặt",
                "Thuật toán không thể phát hiện bất kỳ khuôn mặt nào trong ảnh chân dung này!\n\n"
                "Vui lòng chọn ảnh chụp rõ góc thẳng, đủ ánh sáng.");
            return;
        }

        // Chỉ lấy khuôn mặt đầu tiên (lớn nhất, tin cậy nhất) nếu có nhiều mặt
        QRect faceRect = faces[0];
        // Chuyển QRect (Qt) sang cv::Rect (OpenCV)
        cv::Rect cvRect(faceRect.x(), faceRect.y(), faceRect.width(), faceRect.height());
        // Giới hạn rect trong biên ảnh (tránh tràn ra ngoài biên ảnh gây lỗi)
        cvRect &= cv::Rect(0, 0, frame.cols, frame.rows);
        if (cvRect.width <= 0 || cvRect.height <= 0) return; // Kiểm tra thêm

        // Cắt vùng khuôn mặt từ ảnh gốc
        cv::Mat faceROI = frame(cvRect);

        // ============================================================
        // AI STEP 3: Trích xuất vector đặc trưng 128 chiều (SFace)
        // Đây là "dấu vân tay số" của khuôn mặt - được lưu vào SQLite
        // ============================================================
        std::vector<float> faceVector = Face::extractEmbedding(faceROI);
        if (faceVector.empty()) {
            QMessageBox::critical(&dialog, "Lỗi AI", "Không thể trích xuất vector đặc trưng FaceID.");
            return;
        }

        // ============================================================
        // BƯỚC LƯU DATABASE: Ghi thông tin sinh viên + FaceVector vào SQLite
        // syncStudent sử dụng UPSERT: thêm mới hoặc cập nhật nếu MSSV đã tồn tại
        // ============================================================
        if (Database::syncStudent(mssv, hoTen, maLop, faceVector, selectedImgPath)) {
            QMessageBox::information(&dialog, "Thành công",
                QString("Đăng ký thành công FaceID cho sinh viên:\n\nHọ Tên: %1\nMSSV: %2\nMã Lớp: %3")
                .arg(hoTen, mssv, maLop));
            dialog.accept(); // Đóng dialog với kết quả Accepted
        } else {
            QMessageBox::critical(&dialog, "Lỗi Database", "Gặp lỗi khi ghi thông tin sinh viên vào SQLite.");
        }
    });

    // Hiển thị dialog và trả về true/false dựa vào kết quả người dùng
    return dialog.exec() == QDialog::Accepted;
}

// ============================================================================
// FORWARD DECLARATIONS - Khai báo trước các hàm để có thể gọi nhau tự do
// (Do không dùng class, các hàm phụ thuộc nhau cần được khai báo trước)
// ============================================================================
void refreshClassGrid(QGridLayout *gridLayout, QWidget *parentWindow);
void showCreateClassDialog(QWidget *parent, QGridLayout *gridLayout);
void showConfigureScheduleDialog(QWidget *parent, const QString &maLop, QGridLayout *gridLayout);
void showClassDetailsDialog(QWidget *parent, const QString &classCode, const QString &className, QGridLayout *gridLayout);
void showExportPdfDialog(QWidget *parent);
void refreshLateAbsentReport(QVBoxLayout *reportLayout, QWidget *parentWindow);
void triggerReportRefresh(QWidget *parentWindow);
bool showLoginDialog(QString &outUsername, QWidget *parent = nullptr);
void refreshClassComboBox(QComboBox *combo);

// -----------------------------------------------------------------------
// refreshClassGrid: Làm mới lưới hiển thị thẻ lớp học trên trang Class Management
//
// Hoạt động:
//   1. Xóa toàn bộ thẻ lớp cũ trong grid layout
//   2. Tải lại danh sách lớp từ SQLite
//   3. Tạo lại thẻ mới cho từng lớp, xếp theo lưới 3 cột
//
// Tham số:
//   gridLayout:   Grid layout cần cập nhật
//   parentWindow: Cửa sổ cha (dùng để truyền cho dialog con)
// -----------------------------------------------------------------------
void refreshClassGrid(QGridLayout *gridLayout, QWidget *parentWindow)
{
    // ============================================================
    // XÓA TẤT CẢ WIDGET CŨ TRONG GRID
    // Phải xóa theo đúng thứ tự để tránh memory leak:
    //   1. takeAt(0): Lấy và xóa phần tử khỏi layout
    //   2. widget()->deleteLater(): Lên lịch xóa widget (Qt event queue)
    //   3. delete child: Xóa QLayoutItem wrapper
    // ============================================================
    QLayoutItem *child;
    while ((child = gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater(); // Xóa an toàn trên main thread
        }
        delete child; // Xóa layout item wrapper
    }

    // Tải toàn bộ danh sách lớp học từ SQLite (đã sắp xếp theo ngày/giờ)
    auto classes = Database::loadAllClasses();
    int row = 0; // Hàng hiện tại trong grid
    int col = 0; // Cột hiện tại trong grid
    int colsCount = 3; // Số cột trong lưới (3 thẻ/hàng)

    // Tạo thẻ cho từng lớp học
    for (const auto &c : classes) {
        // Đếm sĩ số thực tế (số sinh viên đã đăng ký FaceID)
        int studentCount = Database::countStudentsInClass(c.maLop);

        // Tạo thẻ lớp học (QFrame làm container với bo góc và nền tối)
        QFrame *card = new QFrame();
        card->setFixedSize(260, 180); // Kích thước cố định mỗi thẻ
        card->setStyleSheet(R"(
            QFrame {
                background-color: #1E293B;
                border: 1px solid #334155;
                border-radius: 12px;
            }
        )");

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 16, 16, 16);
        cardLayout->setSpacing(8);

        // ============================================================
        // HEADER THẺ: [Badge mã lớp] ........... [Nút •••]
        // ============================================================
        QHBoxLayout *header = new QHBoxLayout();

        // Badge hiển thị mã lớp (ví dụ: "CS-101")
        QLabel *lblBadge = new QLabel(c.maLop, card);
        lblBadge->setStyleSheet("background-color: #334155; color: #E2E8F0; font-size: 10px; font-weight: bold; padding: 4px 8px; border-radius: 6px; border: none;");
        
        // Nút menu "•••" mở context menu với tùy chọn xóa
        QPushButton *btnMenu = new QPushButton("•••", card);
        btnMenu->setStyleSheet("QPushButton { color: #64748B; font-size: 14px; font-weight: bold; border: none; background: transparent; } QPushButton:hover { color: #E2E8F0; }");
        btnMenu->setCursor(Qt::PointingHandCursor); // Đổi con trỏ thành tay khi hover
        
        // Tạo context menu với action Thiết lập lịch học và Xóa
        QMenu *menu = new QMenu(card);
        menu->setStyleSheet(R"(
            QMenu { background-color: #1E293B; border: 1px solid #334155; color: #E2E8F0; padding: 4px; }
            QMenu::item:selected { background-color: #3B82F6; color: white; }
        )");

        QAction *actSchedule = menu->addAction("📅 Thiết lập lịch học");
        QAction *actDelete = menu->addAction("❌ Xóa lớp học này");

        // Đổi màu hover của nút xóa thành đỏ để cảnh báo trực quan
        actDelete->setProperty("isDelete", true);

        // Kết nối action thiết lập lịch học
        QObject::connect(actSchedule, &QAction::triggered, [c, parentWindow, gridLayout]() {
            showConfigureScheduleDialog(parentWindow, c.maLop, gridLayout);
        });

        // Kết nối action xóa với lambda yêu cầu xác nhận trước khi xóa
        QObject::connect(actDelete, &QAction::triggered, [c, parentWindow, gridLayout]() {
            // Hỏi xác nhận trước khi xóa (hành động không thể hoàn tác)
            auto reply = QMessageBox::question(parentWindow, "Xác nhận xóa",
                QString("Bạn có chắc chắn muốn xóa lớp học %1?\n"
                        "LƯU Ý: Hành động này sẽ xóa tất cả sinh viên thuộc lớp học này cùng với lịch sử điểm danh của họ!").arg(c.maLop),
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                if (Database::deleteClass(c.maLop)) {
                    QMessageBox::information(parentWindow, "Thành công", "Đã xóa lớp học thành công!");
                    // Làm mới cả grid lớp và báo cáo sau khi xóa
                    refreshClassGrid(gridLayout, parentWindow);
                    triggerReportRefresh(parentWindow);
                } else {
                    QMessageBox::critical(parentWindow, "Lỗi", "Không thể xóa lớp học khỏi SQLite.");
                }
            }
        });
        
        // Kết nối nút "•••" để hiện menu tại vị trí nút
        QObject::connect(btnMenu, &QPushButton::clicked, [btnMenu, menu]() {
            // mapToGlobal chuyển tọa độ cục bộ của nút sang tọa độ màn hình
            menu->exec(btnMenu->mapToGlobal(QPoint(0, btnMenu->height())));
        });
        
        header->addWidget(lblBadge);
        header->addStretch(); // Đẩy badge sang trái, nút sang phải
        header->addWidget(btnMenu);
        cardLayout->addLayout(header);

        // ============================================================
        // PHẦN GIỮA THẺ: Tên môn học + Phòng học
        // ============================================================
        QLabel *lblClassName = new QLabel(c.tenLop, card);
        lblClassName->setStyleSheet("font-size: 15px; font-weight: bold; color: #F8FAFC; border: none; background: transparent;");
        lblClassName->setWordWrap(true); // Tự động xuống dòng nếu tên quá dài
        cardLayout->addWidget(lblClassName);

        // Phòng học với icon ghim bản đồ
        QLabel *lblRoom = new QLabel("📍 " + c.phongHoc, card);
        lblRoom->setStyleSheet("font-size: 12px; color: #94A3B8; border: none; background: transparent;");
        cardLayout->addWidget(lblRoom);

        cardLayout->addStretch(); // Đẩy footer xuống dưới

        // ============================================================
        // FOOTER THẺ: [Sĩ số thực tế/dự kiến] .... [Nút Chi tiết →]
        // ============================================================
        QHBoxLayout *footer = new QHBoxLayout();

        // Hiển thị sĩ số: "👥 X/Y sinh viên" (X = thực tế đã đăng ký, Y = dự kiến)
        QLabel *lblStudents = new QLabel(QString("👥 %1/%2 sinh viên").arg(studentCount).arg(c.siSoDuKien), card);
        lblStudents->setStyleSheet("font-size: 12px; color: #94A3B8; border: none; background: transparent;");
        
        // Nút "Chi tiết →" mở dialog xem thành viên lớp
        QPushButton *btnDetail = new QPushButton("Chi tiết →", card);
        btnDetail->setStyleSheet(R"(
            QPushButton {
                color: #3B82F6;
                font-size: 12px;
                font-weight: bold;
                border: none;
                background: transparent;
                text-align: right;
            }
            QPushButton:hover {
                color: #60A5FA;
            }
        )");
        
        // Kết nối nút Chi tiết để mở dialog xem chi tiết lớp
        QObject::connect(btnDetail, &QPushButton::clicked, [c, parentWindow, gridLayout]() {
            showClassDetailsDialog(parentWindow, c.maLop, c.tenLop, gridLayout);
        });

        footer->addWidget(lblStudents);
        footer->addStretch();
        footer->addWidget(btnDetail);
        cardLayout->addLayout(footer);

        // Thêm thẻ vào grid tại vị trí (row, col)
        gridLayout->addWidget(card, row, col);
        col++;

        // Nếu đã đủ 3 cột -> xuống hàng mới
        if (col >= colsCount) {
            col = 0; // Đặt lại về cột đầu tiên
            row++;   // Tăng lên hàng tiếp theo
        }
    }
}

// -----------------------------------------------------------------------
// refreshClassComboBox: Làm mới danh sách lớp trong ComboBox chọn lớp
//
// Được gọi khi: Mở app, thêm lớp mới, xóa lớp, chuyển sang trang Face Scan
// Lưu lại lớp đang chọn để sau khi reload vẫn giữ nguyên selection.
//
// Tham số: combo - QComboBox cần cập nhật
// -----------------------------------------------------------------------
void refreshClassComboBox(QComboBox *combo)
{
    if (!combo) return; // Bảo vệ: không làm gì nếu combo là null

    // Lưu lại data (mã lớp) của lựa chọn hiện tại trước khi xóa
    QString currentSelected = combo->currentData().toString();
    
    // Xóa tất cả items cũ
    combo->clear();

    // Thêm item đầu tiên "Tất cả lớp học" với data rỗng (filter = không lọc)
    combo->addItem("Tất cả lớp học", "");
    
    // Tải danh sách lớp từ SQLite và thêm vào combo
    auto classes = Database::loadAllClasses();
    int selectIndex = 0; // Chỉ số sẽ được chọn sau khi reload
    int index = 1;       // Bắt đầu từ index 1 (index 0 là "Tất cả lớp học")

    for (const auto &c : classes) {
        // Hiển thị: "MaLop (TenLop)" - ví dụ: "CS-101 (Lập trình C++)"
        combo->addItem(QString("%1 (%2)").arg(c.maLop, c.tenLop), c.maLop);

        // Kiểm tra nếu lớp này là lớp đang được chọn trước khi reload
        if (!currentSelected.isEmpty() && c.maLop == currentSelected) {
            selectIndex = index; // Ghi nhớ index để restore selection
        }
        index++;
    }

    // Khôi phục lại lớp đã chọn trước đó
    combo->setCurrentIndex(selectIndex);
}


// -----------------------------------------------------------------------
// showCreateClassDialog: Dialog tạo lớp học mới
//
// Form nhập thông tin:
//   - Mã lớp (khóa chính, bắt buộc duy nhất)
//   - Tên lớp/môn học
//   - Phòng học
//   - Sĩ số dự kiến
//   - Ngày học (chọn từ calendar)
//   - Giờ bắt đầu và kết thúc
//
// Tham số:
//   parent:     Widget cha của dialog
//   gridLayout: Grid layout trang lớp học (để làm mới sau khi tạo)
// -----------------------------------------------------------------------
void showCreateClassDialog(QWidget *parent, QGridLayout *gridLayout)
{
    // Tạo dialog tạo lớp mới
    QDialog dialog(parent);
    dialog.setWindowTitle("Tạo Lớp Học Mới");
    dialog.setFixedSize(400, 260);
    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #1E293B;
            border: 1px solid #334155;
        }
        QLabel {
            font-size: 12px;
            color: #E2E8F0;
            font-weight: bold;
        }
        QLineEdit {
            padding: 6px;
            border: 1px solid #475569;
            border-radius: 6px;
            font-size: 12px;
            background-color: #0F172A;
            color: #FFFFFF;
        }
        QLineEdit:focus {
            border: 1px solid #3B82F6;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 20, 20, 20);

    // Form layout cho các trường thông tin
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    // Các ô nhập liệu thông tin lớp học
    QLineEdit *inputMaLop = new QLineEdit(&dialog);
    inputMaLop->setPlaceholderText("Ví dụ: CS-101");

    QLineEdit *inputTenLop = new QLineEdit(&dialog);
    inputTenLop->setPlaceholderText("Ví dụ: Lập trình cơ bản C++");

    QLineEdit *inputPhongHoc = new QLineEdit(&dialog);
    inputPhongHoc->setPlaceholderText("Ví dụ: Phòng A2-302");

    QLineEdit *inputSiSo = new QLineEdit(&dialog);
    inputSiSo->setPlaceholderText("Ví dụ: 45");

    formLayout->addRow("Mã lớp học:", inputMaLop);
    formLayout->addRow("Tên lớp/môn học:", inputTenLop);
    formLayout->addRow("Phòng học:", inputPhongHoc);
    formLayout->addRow("Sĩ số dự kiến:", inputSiSo);
    layout->addLayout(formLayout);

    // Hàng nút Hủy / Tạo lớp
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *btnCancel = new QPushButton("Hủy", &dialog);
    btnCancel->setStyleSheet(R"(
        QPushButton {
            background-color: #334155;
            color: #94A3B8;
            padding: 6px 16px;
            border-radius: 6px;
            border: none;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #475569;
        }
    )");
    
    QPushButton *btnSave = new QPushButton("Tạo lớp", &dialog);
    btnSave->setStyleSheet(R"(
        QPushButton {
            background-color: #2563EB;
            color: white;
            padding: 6px 16px;
            border-radius: 6px;
            border: none;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1D4ED8;
        }
    )");

    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);
    layout->addLayout(btnLayout);

    // Nút Hủy đóng dialog
    QObject::connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    // Sự kiện nút "Tạo lớp": Validate và lưu vào CSDL
    QObject::connect(btnSave, &QPushButton::clicked, [&]() {
        // Đọc dữ liệu từ các ô nhập
        QString maLop    = inputMaLop->text().trimmed();
        QString tenLop   = inputTenLop->text().trimmed();
        QString phongHoc = inputPhongHoc->text().trimmed();
        int siSo         = inputSiSo->text().trimmed().toInt(); // Chuyển chuỗi -> số nguyên

        // Kiểm tra các trường bắt buộc không được rỗng
        if (maLop.isEmpty() || tenLop.isEmpty() || phongHoc.isEmpty()) {
            QMessageBox::warning(&dialog, "Thông báo", "Vui lòng điền đầy đủ các trường thông tin!");
            return;
        }

        // Kiểm tra mã lớp chưa tồn tại (tránh tạo trùng)
        if (Database::classExists(maLop)) {
            QMessageBox::warning(&dialog, "Thông báo", "Mã lớp học này đã tồn tại!");
            return;
        }

        // Lưu lớp học mới vào SQLite với ngày học và giờ mặc định trống
        if (Database::createClass(maLop, tenLop, phongHoc, siSo, "", "", "")) {
            QMessageBox::information(&dialog, "Thành công", "Tạo lớp học mới thành công! Vui lòng cấu hình lịch học cho lớp.");
            // Làm mới giao diện sau khi tạo thành công
            refreshClassGrid(gridLayout, parent);  // Cập nhật lưới thẻ lớp
            triggerReportRefresh(parent);          // Cập nhật báo cáo vắng/trễ
            dialog.accept();
        } else {
            QMessageBox::critical(&dialog, "Lỗi", "Không thể lưu lớp học vào CSDL SQLite.");
        }
    });

    dialog.exec(); // Hiển thị dialog (blocking - chờ đến khi đóng)
}

// -----------------------------------------------------------------------
// showClassDetailsDialog: Dialog xem chi tiết và quản lý thành viên của 1 lớp học
//
// Chức năng:
//   - Hiển thị danh sách sinh viên đã đăng ký FaceID trong lớp
//   - Cho phép thêm sinh viên mới vào lớp này
//   - Cho phép xóa toàn bộ lớp học
//
// Tham số:
//   parent:     Widget cha
//   classCode:  Mã lớp cần xem
//   className:  Tên lớp (để hiển thị tiêu đề)
//   gridLayout: Grid layout trang lớp (để làm mới sau khi xóa)
// -----------------------------------------------------------------------
void showClassDetailsDialog(QWidget *parent, const QString &classCode, const QString &className, QGridLayout *gridLayout)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(QString("Danh sách lớp: %1").arg(classCode));
    dialog.setFixedSize(500, 420);
    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #1E293B;
            border: 1px solid #334155;
        }
        QLabel {
            color: #E2E8F0;
        }
        QListWidget {
            background-color: #0F172A;
            border: 1px solid #334155;
            border-radius: 8px;
            color: #FFFFFF;
            padding: 6px;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 20, 20, 20);

    // Tiêu đề dialog
    QLabel *lblTitle = new QLabel(QString("Lớp: %1 - %2").arg(classCode, className), &dialog);
    lblTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #F8FAFC;");
    layout->addWidget(lblTitle);

    QLabel *lblSub = new QLabel("Danh sách thành viên đăng ký FaceID trong lớp:", &dialog);
    lblSub->setStyleSheet("color: #94A3B8; font-size: 12px;");
    layout->addWidget(lblSub);

    // Danh sách thành viên lớp (QListWidget với custom widget)
    QListWidget *listWidget = new QListWidget(&dialog);
    listWidget->setFocusPolicy(Qt::NoFocus); // Không hiển thị focus border
    layout->addWidget(listWidget);

    // ============================================================
    // LAMBDA NỘI BỘ: refreshList - Cập nhật danh sách sinh viên trong dialog
    // Được gọi khi mở dialog và sau khi thêm sinh viên mới
    // ============================================================
    auto refreshList = [&]() {
        listWidget->clear(); // Xóa danh sách cũ

        // Tải danh sách sinh viên của lớp từ SQLite
        auto students = Database::loadStudentsByClass(classCode);
        for (const auto &s : students) {
            // Tạo QListWidgetItem với custom widget hiển thị tên và MSSV
            QListWidgetItem *item = new QListWidgetItem(listWidget);
            item->setSizeHint(QSize(0, 38)); // Chiều cao mỗi item

            QWidget *w = new QWidget();
            w->setStyleSheet("background: transparent;");
            QHBoxLayout *lay = new QHBoxLayout(w);
            lay->setContentsMargins(8, 4, 8, 4);

            QLabel *lblName = new QLabel(s.hoTen, w);
            lblName->setStyleSheet("font-weight: bold; color: #F1F5F9; font-size: 12px;");

            QLabel *lblMssv = new QLabel(QString("MSSV: %1").arg(s.mssv), w);
            lblMssv->setStyleSheet("color: #94A3B8; font-size: 11px;");

            lay->addWidget(lblName);
            lay->addStretch();
            lay->addWidget(lblMssv);

            listWidget->setItemWidget(item, w); // Gắn widget vào item
        }

        // Hiển thị thông báo nếu lớp chưa có sinh viên nào
        if (students.empty()) {
            listWidget->addItem("Lớp chưa có sinh viên nào đăng ký FaceID.");
        }
    };
    refreshList(); // Tải danh sách ngay khi mở dialog

    // ============================================================
    // HÀNG NÚT: [Xóa lớp] [Thêm sinh viên] ........... [Đóng]
    // ============================================================
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    // Nút Xóa lớp (màu đỏ, nguy hiểm - cảnh báo người dùng)
    QPushButton *btnDeleteClass = new QPushButton("Xóa lớp học", &dialog);
    btnDeleteClass->setStyleSheet(R"(
        QPushButton {
            background-color: #EF4444;
            color: white;
            padding: 8px 16px;
            border-radius: 6px;
            border: none;
            font-weight: bold;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #DC2626;
        }
    )");

    // Nút Thêm sinh viên (màu xanh lá, hành động tích cực)
    QPushButton *btnAddMember = new QPushButton("Thêm sinh viên", &dialog);
    btnAddMember->setStyleSheet(R"(
        QPushButton {
            background-color: #10B981;
            color: white;
            padding: 8px 16px;
            border-radius: 6px;
            border: none;
            font-weight: bold;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #059669;
        }
    )");

    // Nút Đóng
    QPushButton *btnClose = new QPushButton("Đóng", &dialog);
    btnClose->setStyleSheet(R"(
        QPushButton {
            background-color: #334155;
            color: #94A3B8;
            padding: 8px 16px;
            border-radius: 6px;
            border: none;
            font-weight: bold;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #475569;
            color: #F8FAFC;
        }
    )");

    btnLayout->addWidget(btnDeleteClass);
    btnLayout->addWidget(btnAddMember);
    btnLayout->addStretch();
    btnLayout->addWidget(btnClose);
    layout->addLayout(btnLayout);

    // Nút Đóng: Đóng dialog bình thường
    QObject::connect(btnClose, &QPushButton::clicked, &dialog, &QDialog::accept);

    // Nút Xóa lớp: Hỏi xác nhận rồi xóa toàn bộ (lớp + sinh viên + lịch sử)
    QObject::connect(btnDeleteClass, &QPushButton::clicked, [&]() {
        // Cảnh báo rõ ràng về hậu quả của hành động xóa
        auto reply = QMessageBox::question(&dialog, "Xác nhận xóa lớp học",
            QString("Bạn có chắc chắn muốn xóa lớp học %1?\n\n"
                    "LƯU Ý: Hành động này sẽ xóa vĩnh viễn tất cả sinh viên thuộc lớp học này cùng lịch sử điểm danh liên quan!").arg(classCode),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            if (Database::deleteClass(classCode)) {
                QMessageBox::information(&dialog, "Thành công", "Đã xóa lớp học thành công!");
                // Cập nhật lại giao diện trang lớp học
                refreshClassGrid(gridLayout, parent);
                triggerReportRefresh(parent);
                dialog.accept(); // Đóng dialog sau khi xóa thành công
            } else {
                QMessageBox::critical(&dialog, "Lỗi", "Không thể xóa lớp học khỏi SQLite.");
            }
        }
    });

    // Nút Thêm sinh viên: Mở AddStudent dialog với mã lớp được điền sẵn
    QObject::connect(btnAddMember, &QPushButton::clicked, [&]() {
        // Truyền classCode vào dialog để khóa ô mã lớp
        if (showAddStudentDialog(parent, classCode)) {
            refreshList();                          // Cập nhật danh sách sau khi thêm thành công
            refreshClassGrid(gridLayout, parent);   // Cập nhật sĩ số trên thẻ lớp
        }
    });

    dialog.exec(); // Hiển thị dialog (blocking)
}

// -----------------------------------------------------------------------
// refreshLateAbsentReport: Cập nhật báo cáo học viên đi trễ/vắng mặt trong 7 ngày
//
// Hoạt động:
//   1. Xóa tất cả thẻ báo cáo cũ
//   2. Duyệt qua 7 ngày gần đây (hôm nay đến 6 ngày trước)
//   3. Với mỗi ngày, kiểm tra các lớp học có ngày học trùng
//   4. Với mỗi lớp đã bắt đầu, kiểm tra từng sinh viên:
//      - Không có dữ liệu điểm danh -> VẮNG MẶT
//      - Điểm danh sau giờ bắt đầu -> ĐI TRỄ
//   5. Tạo thẻ báo cáo cho từng trường hợp vi phạm
//
// Tham số:
//   reportLayout: VBoxLayout chứa các thẻ báo cáo
//   parentWindow: Cửa sổ cha
// -----------------------------------------------------------------------
void refreshLateAbsentReport(QVBoxLayout *reportLayout, QWidget *parentWindow)
{
    // Xóa toàn bộ thẻ báo cáo cũ (tương tự refreshClassGrid)
    QLayoutItem *child;
    while ((child = reportLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    QDate today = QDate::currentDate();       // Ngày hôm nay
    QTime curTime = QTime::currentTime();     // Giờ hiện tại (để kiểm tra hôm nay)
    auto classes = Database::loadAllClasses(); // Tải tất cả lớp học

    int entryCount = 0; // Đếm số trường hợp vi phạm tìm được

    // ============================================================
    // DUYỆT QUA 7 NGÀY GẦN ĐÂY
    // d=0: Hôm nay, d=1: Hôm qua, ..., d=6: 6 ngày trước
    // ============================================================
    for (int d = 0; d < 7; ++d) {
        QDate date = today.addDays(-d);                    // Ngày cần kiểm tra
        QString dateStr = date.toString("yyyy-MM-dd");     // Định dạng để so sánh với SQLite

        // Kiểm tra từng lớp học xem có ngày học trùng với ngày đang duyệt không
        for (const auto &c : classes) {
            if (c.ngayHoc != dateStr) continue; // Bỏ qua nếu không phải ngày học của lớp này

            // Lấy giờ bắt đầu của lớp học
            QTime sTime = QTime::fromString(c.gioBatDau, "HH:mm");

            // ============================================================
            // KIỂM TRA BUỔI HỌC ĐÃ BẮT ĐẦU CHƯA
            // - Ngày trước hôm nay: Chắc chắn đã bắt đầu
            // - Hôm nay: Chỉ bắt đầu nếu giờ hiện tại >= giờ học
            // ============================================================
            bool hasStarted = false;
            if (date < today) {
                hasStarted = true; // Ngày đã qua -> buổi học đã diễn ra
            } else if (date == today) {
                if (curTime >= sTime) {
                    hasStarted = true; // Hôm nay + đã qua giờ học
                }
            }

            if (!hasStarted) continue; // Bỏ qua nếu buổi học chưa bắt đầu

            // Tải danh sách sinh viên của lớp này
            auto students = Database::loadStudentsByClass(c.maLop);
            for (const auto &s : students) {
                // Lấy thời gian điểm danh của sinh viên vào ngày này
                QDateTime checkInTime = Database::getCheckInTimeForDate(s.mssv, dateStr);

                // Phân tích trạng thái: Vắng mặt hay Đi trễ
                bool isAbsent = !checkInTime.isValid(); // Không có dữ liệu = vắng mặt
                bool isLate = false;
                QString timeStr = "";

                if (!isAbsent) {
                    // Có điểm danh - kiểm tra có đúng giờ không
                    QTime checkTime = checkInTime.time();
                    if (checkTime > sTime) {
                        // Điểm danh sau giờ bắt đầu = đi trễ
                        isLate = true;
                        timeStr = checkTime.toString("HH:mm:ss"); // Lưu giờ điểm danh thực tế
                    }
                }

                // Chỉ hiển thị thẻ báo cáo nếu sinh viên vắng hoặc trễ
                if (isAbsent || isLate) {
                    entryCount++;

                    // Tạo thẻ báo cáo
                    QFrame *card = new QFrame();
                    card->setStyleSheet(R"(
                        QFrame {
                            background-color: #1E293B;
                            border: 1px solid #334155;
                            border-radius: 10px;
                        }
                    )");

                    QHBoxLayout *cardLayout = new QHBoxLayout(card);
                    cardLayout->setContentsMargins(16, 12, 16, 12);
                    cardLayout->setSpacing(12);

                    QVBoxLayout *textLayout = new QVBoxLayout();
                    textLayout->setSpacing(4);

                    // ============================================================
                    // HEADER THẺ BÁO CÁO: [Badge trạng thái] [Ngày học]
                    // Badge màu đỏ = VẮNG MẶT, màu vàng = ĐI TRỄ
                    // Border-left 5px màu sắc để phân biệt nhanh
                    // ============================================================
                    QHBoxLayout *headerRow = new QHBoxLayout();
                    headerRow->setSpacing(10);

                    QLabel *lblBadge = new QLabel(card);
                    if (isAbsent) {
                        // Thẻ VẮNG MẶT: Badge đỏ + Border-left đỏ
                        lblBadge->setText("VẮNG MẶT");
                        lblBadge->setStyleSheet("background-color: #7F1D1D; color: #FCA5A5; font-size: 9px; font-weight: bold; padding: 2px 6px; border-radius: 4px; border: none;");
                        card->setStyleSheet("QFrame { background-color: #1E293B; border-left: 5px solid #EF4444; border-top: 1px solid #334155; border-right: 1px solid #334155; border-bottom: 1px solid #334155; border-radius: 10px; }");
                    } else {
                        // Thẻ ĐI TRỄ: Badge vàng + Border-left vàng
                        lblBadge->setText("ĐI TRỄ");
                        lblBadge->setStyleSheet("background-color: #78350F; color: #FCD34D; font-size: 9px; font-weight: bold; padding: 2px 6px; border-radius: 4px; border: none;");
                        card->setStyleSheet("QFrame { background-color: #1E293B; border-left: 5px solid #F59E0B; border-top: 1px solid #334155; border-right: 1px solid #334155; border-bottom: 1px solid #334155; border-radius: 10px; }");
                    }

                    // Nhãn ngày học
                    QLabel *lblDate = new QLabel(QString("📅 Ngày: %1").arg(date.toString("dd/MM/yyyy")), card);
                    lblDate->setStyleSheet("color: #94A3B8; font-size: 11px; font-weight: bold; border: none;");

                    headerRow->addWidget(lblBadge);
                    headerRow->addWidget(lblDate);
                    headerRow->addStretch();
                    textLayout->addLayout(headerRow);

                    // Tên học viên
                    QLabel *lblStudent = new QLabel(QString("Học viên: %1 (%2)").arg(s.hoTen).arg(s.mssv), card);
                    lblStudent->setStyleSheet("font-size: 14px; font-weight: bold; color: #F8FAFC; border: none;");
                    textLayout->addWidget(lblStudent);

                    // Thông tin lớp học
                    QLabel *lblClass = new QLabel(QString("Lớp: %1 (%2) - Phòng: %3").arg(c.tenLop).arg(c.maLop).arg(c.phongHoc), card);
                    lblClass->setStyleSheet("font-size: 12px; color: #94A3B8; border: none;");
                    textLayout->addWidget(lblClass);

                    // Chi tiết thời gian (khác nhau tùy vắng hay trễ)
                    QLabel *lblDetail = new QLabel(card);
                    if (isAbsent) {
                        // Vắng mặt: Hiển thị giờ học dự kiến
                        lblDetail->setText(QString("Giờ học: %1 - %2 (Không có dữ liệu điểm danh)").arg(c.gioBatDau).arg(c.gioKetThuc));
                        lblDetail->setStyleSheet("font-size: 11px; color: #EF4444; border: none;");
                    } else {
                        // Đi trễ: Hiển thị giờ điểm danh thực tế và so với giờ học
                        lblDetail->setText(QString("Điểm danh lúc: %1 (Trễ so với giờ vào lớp: %2)").arg(timeStr).arg(c.gioBatDau));
                        lblDetail->setStyleSheet("font-size: 11px; color: #F59E0B; border: none;");
                    }
                    textLayout->addWidget(lblDetail);

                    cardLayout->addLayout(textLayout, 1); // 1 = stretch factor, chiếm phần lớn

                    // Icon bên phải (❌ vắng, ⚠️ trễ)
                    QLabel *lblRightIcon = new QLabel(card);
                    lblRightIcon->setText(isAbsent ? "❌" : "⚠️");
                    lblRightIcon->setStyleSheet("font-size: 18px; border: none;");
                    cardLayout->addWidget(lblRightIcon, 0, Qt::AlignRight | Qt::AlignVCenter);

                    reportLayout->addWidget(card); // Thêm thẻ vào danh sách báo cáo
                }
            }
        }
    }

    // Nếu không có vi phạm nào -> hiển thị thông báo tích cực
    if (entryCount == 0) {
        QLabel *lblEmpty = new QLabel("Không có ghi nhận học viên đi trễ hay vắng mặt nào trong tuần qua.", reportLayout->parentWidget());
        lblEmpty->setStyleSheet("color: #64748B; font-size: 13px; padding: 40px 0;");
        lblEmpty->setAlignment(Qt::AlignCenter);
        reportLayout->addWidget(lblEmpty);
    }
}

// -----------------------------------------------------------------------
// triggerReportRefresh: Kích hoạt làm mới báo cáo từ bất kỳ đâu trong ứng dụng
//
// Vì báo cáo nằm trong widget con sâu bên trong cấu trúc UI, hàm này
// dùng findChild() để tìm widget đích theo objectName và gọi refresh.
//
// Tham số: parentWindow - Cửa sổ chính (nơi chứa schedScrollContent)
// -----------------------------------------------------------------------
void triggerReportRefresh(QWidget *parentWindow)
{
    // Tìm widget "schedScrollContent" theo objectName trong toàn bộ cây widget
    QWidget *content = parentWindow->findChild<QWidget*>("schedScrollContent");
    if (content) {
        // Lấy layout của widget đó (phải là QVBoxLayout)
        QVBoxLayout *reportLayout = qobject_cast<QVBoxLayout*>(content->layout());
        if (reportLayout) {
            // Gọi hàm làm mới báo cáo
            refreshLateAbsentReport(reportLayout, parentWindow);
        }
    }
}

// -----------------------------------------------------------------------
// showLoginDialog: Dialog đăng nhập hệ thống
//
// Bắt buộc người dùng phải đăng nhập trước khi sử dụng app.
// Dialog không có nút X (không thể tắt bằng phím Alt+F4 hoặc nút X).
// Người dùng chỉ có thể: Đăng nhập thành công hoặc nhấn "Thoát" để thoát app.
//
// Cũng cung cấp chức năng đăng ký tài khoản mới.
//
// Tham số:
//   outUsername: [OUT] Tên tài khoản đăng nhập thành công được ghi vào đây
//   parent:      Widget cha (optional)
// Trả về: true nếu đăng nhập thành công
// -----------------------------------------------------------------------
bool showLoginDialog(QString &outUsername, QWidget *parent)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Đăng Nhập Hệ Thống");
    dialog.setFixedSize(360, 310);

    // Loại bỏ nút X của cửa sổ: Bắt buộc người dùng phải đăng nhập hoặc Thoát
    // Qt::CustomizeWindowHint cho phép tùy chỉnh các nút cửa sổ
    dialog.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    
    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #0F172A;
            border: 1px solid #334155;
        }
        QLabel {
            color: #E2E8F0;
            font-size: 12px;
            font-weight: bold;
        }
        QLineEdit {
            padding: 8px;
            border: 1px solid #475569;
            border-radius: 6px;
            font-size: 12px;
            background-color: #1E293B;
            color: #FFFFFF;
        }
        QLineEdit:focus {
            border: 1px solid #3B82F6;
        }
        QPushButton {
            background-color: #2563EB;
            color: white;
            padding: 8px;
            border-radius: 6px;
            border: none;
            font-weight: bold;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #1D4ED8;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(10);
    layout->setContentsMargins(24, 20, 24, 20);

    // ============================================================
    // LOGO UTH: Tìm file logo theo nhiều đường dẫn fallback
    // Ứng dụng có thể chạy từ nhiều vị trí khác nhau nên cần thử nhiều path
    // ============================================================
    QLabel *lblTitle = new QLabel(&dialog);
    lblTitle->setAlignment(Qt::AlignCenter);
    lblTitle->setStyleSheet("margin-bottom: 10px; border: none; background: transparent;");

    // Lấy thư mục chứa file executable đang chạy
    QString appDir = QCoreApplication::applicationDirPath();
    QString logoPath = appDir + "/assets/logo_uth.png";

    // Thử lần lượt các đường dẫn khác nhau nếu không tìm thấy logo
    if (!QFile::exists(logoPath)) logoPath = appDir + "/../assets/logo_uth.png";
    if (!QFile::exists(logoPath)) logoPath = appDir + "/access/logo_uth.png";
    if (!QFile::exists(logoPath)) logoPath = appDir + "/../access/logo_uth.png";
    if (!QFile::exists(logoPath)) logoPath = QDir::currentPath() + "/assets/logo_uth.png";
    if (!QFile::exists(logoPath)) logoPath = QDir::currentPath() + "/access/logo_uth.png";

    QPixmap logoPix(logoPath);
    if (!logoPix.isNull()) {
        // Load logo thành công -> hiển thị ảnh
        lblTitle->setPixmap(logoPix.scaled(180, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        // Fallback: Hiển thị văn bản nếu không tìm thấy ảnh logo
        lblTitle->setText("UTH LOGIN");
        lblTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #3B82F6; margin-bottom: 8px;");
    }
    layout->addWidget(lblTitle);

    // Form đăng nhập: Tài khoản + Mật khẩu
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    QLineEdit *inputUser = new QLineEdit(&dialog);
    inputUser->setPlaceholderText("Nhập tài khoản");
    
    QLineEdit *inputPass = new QLineEdit(&dialog);
    inputPass->setPlaceholderText("Nhập mật khẩu");
    inputPass->setEchoMode(QLineEdit::Password); // Hiển thị dấu *** thay vì ký tự thực

    formLayout->addRow("Tài khoản:", inputUser);
    formLayout->addRow("Mật khẩu:", inputPass);
    layout->addLayout(formLayout);

    // Hàng nút Thoát / Đăng nhập
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnCancel = new QPushButton("Thoát", &dialog);
    btnCancel->setStyleSheet("background-color: #334155; color: #94A3B8;");
    
    QPushButton *btnLogin = new QPushButton("Đăng nhập", &dialog);
    
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnLogin);
    layout->addLayout(btnLayout);

    // Link đăng ký tài khoản mới (kiểu hyperlink)
    QPushButton *btnRegister = new QPushButton("Chưa có tài khoản? Đăng ký ngay", &dialog);
    btnRegister->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            color: #3B82F6;
            font-size: 11px;
            font-weight: bold;
            border: none;
            text-decoration: underline;
        }
        QPushButton:hover {
            color: #60A5FA;
        }
    )");
    layout->addWidget(btnRegister);

    // Nút Thoát: Đóng dialog, trả về Rejected -> ứng dụng sẽ thoát
    QObject::connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    // ============================================================
    // SỰ KIỆN: Mở dialog đăng ký tài khoản mới
    // ============================================================
    QObject::connect(btnRegister, &QPushButton::clicked, [&dialog]() {
        // Tạo dialog đăng ký con (modal, kế thừa style từ dialog cha)
        QDialog regDialog(&dialog);
        regDialog.setWindowTitle("Đăng Ký Tài Khoản");
        regDialog.setFixedSize(360, 270);
        regDialog.setStyleSheet(dialog.styleSheet()); // Kế thừa stylesheet

        QVBoxLayout *regLayout = new QVBoxLayout(&regDialog);
        regLayout->setSpacing(12);
        regLayout->setContentsMargins(24, 24, 24, 24);

        QLabel *lblRegTitle = new QLabel("ĐĂNG KÝ TÀI KHOẢN MỚI", &regDialog);
        lblRegTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #10B981; margin-bottom: 10px;");
        lblRegTitle->setAlignment(Qt::AlignCenter);
        regLayout->addWidget(lblRegTitle);

        QFormLayout *regForm = new QFormLayout();
        regForm->setSpacing(10);

        // Ba ô nhập: Username, Password, Confirm Password
        QLineEdit *inputNewUser = new QLineEdit(&regDialog);
        inputNewUser->setPlaceholderText("admin_new");
        
        QLineEdit *inputNewPass = new QLineEdit(&regDialog);
        inputNewPass->setPlaceholderText("Nhập mật khẩu");
        inputNewPass->setEchoMode(QLineEdit::Password);

        QLineEdit *inputConfirmPass = new QLineEdit(&regDialog);
        inputConfirmPass->setPlaceholderText("Xác nhận mật khẩu");
        inputConfirmPass->setEchoMode(QLineEdit::Password);

        regForm->addRow("Tài khoản:", inputNewUser);
        regForm->addRow("Mật khẩu:", inputNewPass);
        regForm->addRow("Nhập lại:", inputConfirmPass);
        regLayout->addLayout(regForm);

        QHBoxLayout *regBtnLayout = new QHBoxLayout();
        QPushButton *btnRegCancel = new QPushButton("Hủy", &regDialog);
        btnRegCancel->setStyleSheet("background-color: #334155; color: #94A3B8;");
        
        QPushButton *btnRegSubmit = new QPushButton("Đăng ký", &regDialog);
        btnRegSubmit->setStyleSheet("background-color: #10B981; color: white;");

        regBtnLayout->addWidget(btnRegCancel);
        regBtnLayout->addWidget(btnRegSubmit);
        regLayout->addLayout(regBtnLayout);

        QObject::connect(btnRegCancel, &QPushButton::clicked, &regDialog, &QDialog::reject);

        // Sự kiện Đăng ký: Validate và tạo tài khoản mới
        QObject::connect(btnRegSubmit, &QPushButton::clicked, [&]() {
            QString user    = inputNewUser->text().trimmed();
            QString pass    = inputNewPass->text();
            QString confirm = inputConfirmPass->text();

            // Validate: Không được để trống
            if (user.isEmpty() || pass.isEmpty()) {
                QMessageBox::warning(&regDialog, "Thông báo", "Vui lòng nhập đầy đủ tài khoản và mật khẩu!");
                return;
            }

            // Validate: Mật khẩu và xác nhận phải khớp nhau
            if (pass != confirm) {
                QMessageBox::warning(&regDialog, "Thông báo", "Mật khẩu nhập lại không khớp!");
                return;
            }

            // Ghi tài khoản mới vào SQLite
            if (Database::createAccount(user, pass)) {
                QMessageBox::information(&regDialog, "Thành công", "Đăng ký tài khoản giáo viên mới thành công!\nBạn có thể đăng nhập ngay.");
                regDialog.accept(); // Đóng dialog đăng ký
            } else {
                // Thất bại thường do username đã tồn tại (PRIMARY KEY constraint)
                QMessageBox::critical(&regDialog, "Thất bại", "Tài khoản đã tồn tại hoặc lỗi cơ sở dữ liệu!");
            }
        });

        regDialog.exec(); // Hiển thị dialog đăng ký (blocking)
    });
    
    // ============================================================
    // SỰ KIỆN: Nút Đăng nhập - Xác thực tài khoản với SQLite
    // Dùng con trỏ heap 'success' để truyền kết quả ra ngoài lambda
    // (vì lambda không thể capture return value của dialog.exec())
    // ============================================================
    bool *success = new bool(false); // Cấp phát trên heap để lambda có thể truy cập
    QObject::connect(btnLogin, &QPushButton::clicked, [&]() {
        QString user = inputUser->text().trimmed();
        QString pass = inputPass->text();

        // Xác thực với Database
        if (Database::validateLogin(user, pass)) {
            *success = true;
            outUsername = user; // Ghi tên tài khoản ra biến output
            dialog.accept();   // Đóng dialog với Accepted
        } else {
            // Đăng nhập thất bại: Không đóng dialog, hiển thị cảnh báo
            QMessageBox::warning(&dialog, "Thất bại", "Tài khoản hoặc mật khẩu không chính xác!");
        }
    });

    dialog.exec(); // Hiển thị dialog đăng nhập (blocking)

    // Đọc và giải phóng kết quả
    bool res = *success;
    delete success;
    return res;
}

// ============================================================================
// HÀM MAIN - ĐIỂM KHỞI ĐẦU CHƯƠNG TRÌNH
// ============================================================================
// Quy trình khởi động:
//   1. Kết nối SQLite (trước khi hiển thị UI)
//   2. Xây dựng toàn bộ cửa sổ chính (3 cột: Sidebar trái + Center + Sidebar phải)
//   3. Cài đặt logic sự kiện qua Lambda
//   4. Hiển thị dialog đăng nhập
//   5. Hiển thị cửa sổ chính và vào vòng lặp sự kiện (event loop)
//   6. Khi thoát: Dọn dẹp camera và database
// ============================================================================
int main(int argc, char *argv[])
{
    // Khởi tạo ứng dụng Qt (bắt buộc phải tạo trước khi dùng bất kỳ widget nào)
    QApplication app(argc, argv);

    // ============================================================
    // BIẾN TRẠNG THÁI ỨNG DỤNG (Application State)
    // Khai báo cục bộ trong main() và capture bởi lambda
    // ============================================================
    bool dbConnected = false;    // Trạng thái kết nối SQLite
    int attendanceCount = 0;     // Số lượt điểm danh trong phiên hiện tại

    // Kết nối SQLite trước (cần thiết cho màn hình đăng nhập)
    if (Database::connectToDatabase("attendance.db")) {
        dbConnected = true; // Kết nối thành công
    }

    // ====================================================================
    // CẤU HÌNH CỬA SỔ CHÍNH (MainWindow)
    // ====================================================================
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("FACIEID DASHBOARD - Hệ thống quét gương mặt");
    mainWindow.resize(1080, 660); // Kích thước khởi động 1080x660 pixel
    
    // Áp dụng theme Dark Mode toàn bộ ứng dụng
    mainWindow.setStyleSheet(R"(
        QMainWindow {
            background-color: #0F172A;
        }
        QWidget {
            font-family: 'Segoe UI', Arial, sans-serif;
        }
    )");

    // Widget trung tâm chứa toàn bộ layout chính (3 cột)
    QWidget *centralWidget = new QWidget(&mainWindow);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0); // Không có padding ngoài
    mainLayout->setSpacing(0);                   // Không có khoảng cách giữa các cột

    // ====================================================================
    // PHẦN 1: SIDEBAR TRÁI (MENU ĐIỀU HƯỚNG + TRẠNG THÁI HỆ THỐNG)
    // Chiều rộng cố định 220px
    // ====================================================================
    QWidget *sidebar = new QWidget(centralWidget);
    sidebar->setFixedWidth(220);
    sidebar->setStyleSheet("background-color: #1E293B; border-right: 1px solid #334155;");
    
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(16, 24, 16, 24);

    // ============================================================
    // LOGO UTH Ở ĐẦU SIDEBAR
    // Tìm logo theo nhiều đường dẫn fallback (giống như trong Login Dialog)
    // ============================================================
    QLabel *logoLabel = new QLabel(sidebar);
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setStyleSheet("margin-bottom: 12px; border: none; background: transparent;");

    QString appDir = QCoreApplication::applicationDirPath();
    QString logoPath = appDir + "/assets/logo_uth.png";
    if (!QFile::exists(logoPath)) logoPath = appDir + "/../assets/logo_uth.png";
    if (!QFile::exists(logoPath)) logoPath = appDir + "/access/logo_uth.png";
    if (!QFile::exists(logoPath)) logoPath = appDir + "/../access/logo_uth.png";
    if (!QFile::exists(logoPath)) logoPath = QDir::currentPath() + "/assets/logo_uth.png";
    if (!QFile::exists(logoPath)) logoPath = QDir::currentPath() + "/access/logo_uth.png";

    QPixmap logoPix(logoPath);
    if (!logoPix.isNull()) {
        // Logo tìm thấy: Scale về 190x60 giữ tỉ lệ chiều ngang/dọc
        logoLabel->setPixmap(logoPix.scaled(190, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        sidebarLayout->addWidget(logoLabel);
    } else {
        // Fallback: Hiển thị văn bản tên trường
        QLabel *logoTitle = new QLabel("UTH", sidebar);
        logoTitle->setStyleSheet("font-size: 24px; font-weight: bold; color: #3B82F6; letter-spacing: 1.5px;");
        QLabel *logoSub = new QLabel("ĐẠI HỌC GIAO THÔNG VẬN TẢI TPHCM", sidebar);
        logoSub->setStyleSheet("font-size: 9px; font-weight: bold; color: #94A3B8; margin-bottom: 24px;");
        sidebarLayout->addWidget(logoTitle);
        sidebarLayout->addWidget(logoSub);
    }

    // ============================================================
    // CÁC NÚT MENU ĐIỀU HƯỚNG
    // Mỗi nút tương ứng với 1 trang trong QStackedWidget
    // Nút đang active có màu xanh dương nổi bật, còn lại màu mờ
    // ============================================================
    QList<QPushButton*> menuButtons; // Danh sách tất cả nút menu để quản lý trạng thái active
    QStringList menuItems = {" Face Scan", " Class Management", " Đi trễ & Vắng", " Settings"};

    for (int i = 0; i < menuItems.size(); ++i) {
        QPushButton *btn = new QPushButton(menuItems[i], sidebar);
        btn->setFixedHeight(42);       // Chiều cao cố định 42px
        btn->setCheckable(true);       // Cho phép nút ở trạng thái checked/unchecked

        if (i == 0) {
            // Nút đầu tiên (Face Scan) active mặc định khi mở app
            btn->setChecked(true);
            btn->setStyleSheet(R"(
                QPushButton {
                    text-align: left;
                    padding-left: 14px;
                    font-size: 13px;
                    font-weight: bold;
                    color: #FFFFFF;
                    background-color: #2563EB;
                    border: none;
                    border-radius: 8px;
                }
            )");
        } else {
            // Các nút còn lại: Mặc định ở trạng thái inactive (màu mờ)
            btn->setStyleSheet(R"(
                QPushButton {
                    text-align: left;
                    padding-left: 14px;
                    font-size: 13px;
                    color: #94A3B8;
                    background-color: transparent;
                    border: none;
                }
                QPushButton:hover {
                    background-color: #334155;
                    color: #F8FAFC;
                    border-radius: 8px;
                }
            )");
        }

        // Đặt objectName để có thể tìm kiếm nút bằng findChild() nếu cần
        btn->setObjectName(QString("menuButton%1").arg(i));
        sidebarLayout->addWidget(btn);
        menuButtons.append(btn); // Thêm vào danh sách quản lý
    }

    sidebarLayout->addStretch(); // Đẩy phần panel trạng thái xuống dưới cùng

    // ============================================================
    // PANEL TRẠNG THÁI CƠ SỞ DỮ LIỆU
    // Hiển thị trạng thái kết nối SQLite ở góc dưới sidebar
    // ============================================================
    QFrame *statusPanel = new QFrame(sidebar);
    statusPanel->setStyleSheet("background-color: #0F172A; border-radius: 8px; border: 1px solid #334155;");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusPanel);
    statusLayout->setContentsMargins(10, 10, 10, 10);
    
    QLabel *lblDbLabel = new QLabel("CƠ SỞ DỮ LIỆU:", statusPanel);
    lblDbLabel->setStyleSheet("font-size: 9px; color: #64748B; font-weight: bold; border: none;");
    
    // Nhãn trạng thái - sẽ được cập nhật sau khi khởi tạo xong
    QLabel *lblDbStatus = new QLabel("Đang kết nối...", statusPanel);
    lblDbStatus->setStyleSheet("font-size: 11px; color: #F59E0B; font-weight: bold; border: none;");
    
    statusLayout->addWidget(lblDbLabel);
    statusLayout->addWidget(lblDbStatus);
    sidebarLayout->addWidget(statusPanel);

    // ============================================================
    // PROFILE CARD GIÁO VIÊN Ở GÓC DƯỚI SIDEBAR
    // Hiển thị avatar chữ cái + Tên tài khoản + Vai trò
    // Được cập nhật sau khi đăng nhập thành công
    // ============================================================
    QFrame *profileCard = new QFrame(sidebar);
    profileCard->setStyleSheet(R"(
        QFrame {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #0F172A,stop:1 #1E293B);
            border-radius: 10px;
            border: 1px solid #334155;
            margin-top: 12px;
        }
    )");
    QHBoxLayout *profileLayout = new QHBoxLayout(profileCard);
    profileLayout->setContentsMargins(10, 10, 10, 10);
    profileLayout->setSpacing(10);

    // ============================================================
    // AVATAR GIÁO VIÊN - Hình tròn gradient neon xanh dương
    // Kích thước 40x40px với border neon để nổi bật
    // ============================================================
    QLabel *profAvatar = new QLabel(profileCard);
    profAvatar->setFixedSize(40, 40);
    profAvatar->setStyleSheet(R"(
        QLabel {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #3B82F6,stop:1 #6366F1);
            border-radius: 20px;
            color: white;
            font-weight: bold;
            font-size: 16px;
            border: 2px solid #60A5FA;
        }
    )");
    profAvatar->setAlignment(Qt::AlignCenter);
    profAvatar->setText("A"); // Mặc định "A" cho "Admin"

    QVBoxLayout *profText = new QVBoxLayout();
    profText->setSpacing(2);

    // Tên tài khoản (sẽ cập nhật sau đăng nhập)
    QLabel *profName = new QLabel("Administrator", profileCard);
    profName->setStyleSheet("font-size: 12px; font-weight: bold; color: #F1F5F9; border: none; background: transparent;");

    // Nhãn vai trò + tổ chức
    QLabel *profStatus = new QLabel("🏫 Giáo viên · UTH", profileCard);
    profStatus->setStyleSheet("font-size: 9px; color: #60A5FA; font-weight: bold; border: none; background: transparent;");

    profText->addWidget(profName);
    profText->addWidget(profStatus);
    profileLayout->addWidget(profAvatar);
    profileLayout->addLayout(profText);
    profileLayout->addStretch();
    sidebarLayout->addWidget(profileCard);

    // ====================================================================
    // PHẦN 2: KHU VỰC TRUNG TÂM (QStackedWidget - Chứa 4 trang)
    // QStackedWidget hiển thị 1 trang tại 1 thời điểm, chuyển trang khi
    // người dùng nhấn các nút menu bên sidebar trái
    // ====================================================================
    QStackedWidget *centerStack = new QStackedWidget(centralWidget);
    centerStack->setObjectName("centerStack"); // Đặt tên để tìm kiếm nếu cần

    // ============================================================
    // TRANG 1: FACE SCAN (Camera điểm danh + Thống kê)
    // ============================================================
    QWidget *pageFaceScan = new QWidget(centerStack);
    QVBoxLayout *centerLayout = new QVBoxLayout(pageFaceScan);
    centerLayout->setContentsMargins(24, 20, 24, 20);
    centerLayout->setSpacing(18);

    // Header trang Face Scan: Tiêu đề + Bộ lọc lớp + Nút thêm SV
    QHBoxLayout *centerHeader = new QHBoxLayout();
    QLabel *lblMainTitle = new QLabel("HỆ THỐNG ĐIỂM DANH KHUÔN MẶT", pageFaceScan);
    lblMainTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #F8FAFC;");

    // Bộ lọc chọn lớp học (ComboBox)
    QHBoxLayout *classFilterLayout = new QHBoxLayout();
    classFilterLayout->setSpacing(8);
    QLabel *lblClassLabel = new QLabel("Chọn lớp:", pageFaceScan);
    lblClassLabel->setStyleSheet("font-size: 12px; color: #94A3B8; font-weight: bold;");
    
    // ComboBox chọn lớp - khi chọn 1 lớp, camera chỉ nhận diện SV lớp đó
    QComboBox *comboClass = new QComboBox(pageFaceScan);
    comboClass->setObjectName("comboClass"); // Đặt tên để tìm kiếm
    comboClass->setFixedWidth(200);
    comboClass->setStyleSheet(R"(
        QComboBox {
            background-color: #1E293B;
            border: 1px solid #334155;
            border-radius: 6px;
            color: #FFFFFF;
            padding: 6px 10px;
            font-size: 12px;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 24px;
            border-left: 1px solid #334155;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 6px solid #94A3B8;
            margin-right: 6px;
        }
        QComboBox QAbstractItemView {
            background-color: #1E293B;
            color: #FFFFFF;
            selection-background-color: #2563EB;
            border: 1px solid #334155;
            outline: none;
        }
    )");
    
    // Nạp danh sách lớp học vào ComboBox khi khởi tạo
    refreshClassComboBox(comboClass);

    // Nút thêm sinh viên mới (mở showAddStudentDialog)
    QPushButton *btnAddStudent = new QPushButton("Thêm sinh viên", pageFaceScan);
    btnAddStudent->setStyleSheet(R"(
        QPushButton {
            background-color: #2563EB;
            color: white;
            font-weight: bold;
            padding: 5px 12px;
            border-radius: 6px;
            border: none;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #1D4ED8;
        }
    )");

    classFilterLayout->addWidget(lblClassLabel);
    classFilterLayout->addWidget(comboClass);
    classFilterLayout->addWidget(btnAddStudent);
    
    centerHeader->addWidget(lblMainTitle);
    centerHeader->addStretch();
    centerHeader->addLayout(classFilterLayout);
    centerLayout->addLayout(centerHeader);

    // ============================================================
    // KHUNG HIỂN THỊ CAMERA
    // QLabel là widget hiển thị frame camera, được cập nhật 30 lần/giây
    // Border màu xanh dương (#3B82F6) tạo hiệu ứng HUD futuristic
    // ============================================================
    QFrame *cameraFrame = new QFrame(pageFaceScan);
    cameraFrame->setMinimumSize(480, 360); // Kích thước tối thiểu 480x360
    cameraFrame->setStyleSheet("background-color: #030712; border: 2px solid #3B82F6; border-radius: 12px;");

    QVBoxLayout *camInnerLayout = new QVBoxLayout(cameraFrame);
    camInnerLayout->setContentsMargins(0, 0, 0, 0);

    // QLabel này là nơi hiển thị frame video từ camera
    // Mỗi frame từ camera sẽ được chuyển thành QPixmap và hiển thị lên đây
    QLabel *lblCameraView = new QLabel(cameraFrame);
    lblCameraView->setAlignment(Qt::AlignCenter);
    lblCameraView->setStyleSheet("color: #64748B; font-weight: bold; font-size: 14px; border: none; background: transparent;");
    lblCameraView->setText("CAMERA CHƯA KHỞI ĐỘNG\nNhấn 'Bắt đầu quét' ở dưới");
    camInnerLayout->addWidget(lblCameraView);

    // ============================================================
    // CÁC NÚT ĐIỀU KHIỂN CAMERA
    // ============================================================
    QHBoxLayout *camControlLayout = new QHBoxLayout();
    
    // Nút Bắt đầu quét: Mở camera và bắt đầu nhận diện khuôn mặt
    QPushButton *btnStartCam = new QPushButton("Bắt đầu quét", pageFaceScan);
    btnStartCam->setObjectName("btnStartCam");
    btnStartCam->setStyleSheet(R"(
        QPushButton {
            background-color: #10B981;
            color: white;
            font-weight: bold;
            padding: 8px 18px;
            border-radius: 6px;
            border: none;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #059669;
        }
    )");

    // Nút Dừng quét: Tắt camera và dừng nhận diện
    QPushButton *btnStopCam = new QPushButton("Dừng quét", pageFaceScan);
    btnStopCam->setStyleSheet(R"(
        QPushButton {
            background-color: #EF4444;
            color: white;
            font-weight: bold;
            padding: 8px 18px;
            border-radius: 6px;
            border: none;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #DC2626;
        }
        QPushButton:disabled {
            background-color: #334155;
            color: #64748B;
        }
    )");
    btnStopCam->setEnabled(false); // Ban đầu disabled vì camera chưa bật

    // Nút Reset điểm danh: Xóa lịch sử điểm danh hôm nay
    QPushButton *btnResetAttendance = new QPushButton("Reset điểm danh", pageFaceScan);
    btnResetAttendance->setStyleSheet(R"(
        QPushButton {
            background-color: #64748B;
            color: white;
            font-weight: bold;
            padding: 8px 18px;
            border-radius: 6px;
            border: none;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #475569;
        }
    )");

    camControlLayout->addWidget(btnStartCam);
    camControlLayout->addWidget(btnStopCam);
    camControlLayout->addWidget(btnResetAttendance);
    camControlLayout->addStretch();
    
    centerLayout->addWidget(cameraFrame);
    centerLayout->addLayout(camControlLayout);

    // ============================================================
    // HÀNG THẺ THỐNG KÊ (Stat Cards)
    // 4 thẻ: Sĩ số | Có mặt | Đi muộn | Vắng mặt
    // Các con trỏ lblVal* được lưu lại để cập nhật số liệu sau
    // ============================================================
    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(12);
    
    // Khai báo con trỏ output sẽ được gán bởi createStatCard
    QLabel *lblValTotal   = nullptr; // Sẽ trỏ tới label hiển thị tổng sĩ số
    QLabel *lblValPresent = nullptr; // Sẽ trỏ tới label hiển thị số có mặt
    QLabel *lblValLate    = nullptr; // Sẽ trỏ tới label hiển thị số đi muộn
    QLabel *lblValAbsent  = nullptr; // Sẽ trỏ tới label hiển thị số vắng mặt

    // Tạo 4 thẻ với màu sắc khác nhau
    QWidget *cardTotal   = createStatCard("SĨ SỐ",    "0", "#334155", "#F8FAFC", lblValTotal);   // Xám - trung tính
    QWidget *cardPresent = createStatCard("CÓ MẶT",   "0", "#059669", "#10B981", lblValPresent); // Xanh lá - tích cực
    QWidget *cardLate    = createStatCard("ĐI MUỘN",  "0", "#D97706", "#F59E0B", lblValLate);    // Vàng - cảnh báo
    QWidget *cardAbsent  = createStatCard("VẮNG MẶT", "0", "#DC2626", "#EF4444", lblValAbsent);  // Đỏ - tiêu cực

    statsLayout->addWidget(cardTotal);
    statsLayout->addWidget(cardPresent);
    statsLayout->addWidget(cardLate);
    statsLayout->addWidget(cardAbsent);
    statsLayout->addStretch();
    
    centerLayout->addLayout(statsLayout);

    // ============================================================
    // TRANG 2: CLASS MANAGEMENT (Quản lý lớp học)
    // Hiển thị lưới thẻ các lớp học với khả năng thêm/xóa/xem chi tiết
    // ============================================================
    QWidget *pageClassManagement = new QWidget(centerStack);
    QVBoxLayout *classPageLayout = new QVBoxLayout(pageClassManagement);
    classPageLayout->setContentsMargins(24, 20, 24, 20);
    classPageLayout->setSpacing(18);

    // Header trang lớp học: Tiêu đề + Nút thêm lớp
    QHBoxLayout *classHeaderLayout = new QHBoxLayout();
    QLabel *lblClassPageTitle = new QLabel("DANH SÁCH LỚP HỌC QUẢN LÝ", pageClassManagement);
    lblClassPageTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #F8FAFC;");
    classHeaderLayout->addWidget(lblClassPageTitle);
    classHeaderLayout->addStretch();

    // Nút "+ Thêm lớp" ở header (nổi bật màu xanh lá)
    QPushButton *btnAddClassHeader = new QPushButton("+ Thêm lớp", pageClassManagement);
    btnAddClassHeader->setStyleSheet(R"(
        QPushButton {
            background-color: #10B981;
            color: white;
            font-weight: bold;
            padding: 8px 18px;
            border-radius: 6px;
            border: none;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #059669;
        }
    )");
    btnAddClassHeader->setCursor(Qt::PointingHandCursor); // Đổi con trỏ thành tay khi hover
    classHeaderLayout->addWidget(btnAddClassHeader);
    classPageLayout->addLayout(classHeaderLayout);

    // ScrollArea để cuộn qua nhiều thẻ lớp học
    QScrollArea *classScrollArea = new QScrollArea(pageClassManagement);
    classScrollArea->setWidgetResizable(true); // Cho phép nội dung resize theo scrollarea
    classScrollArea->setStyleSheet(R"(
        QScrollArea {
            border: none;
            background: transparent;
        }
        QScrollBar:vertical {
            width: 8px;
            background-color: #0F172A;
        }
        QScrollBar::handle:vertical {
            background-color: #334155;
            border-radius: 4px;
        }
    )");

    // Widget nội dung trong ScrollArea - chứa QGridLayout các thẻ lớp
    QWidget *classScrollContent = new QWidget();
    classScrollContent->setStyleSheet("background: transparent;");
    QGridLayout *classGridLayout = new QGridLayout(classScrollContent);
    classGridLayout->setSpacing(20);          // Khoảng cách 20px giữa các thẻ
    classGridLayout->setContentsMargins(0, 0, 0, 0);
    classScrollArea->setWidget(classScrollContent);
    classPageLayout->addWidget(classScrollArea);

    // Kết nối nút "+ Thêm lớp" với showCreateClassDialog
    QObject::connect(btnAddClassHeader, &QPushButton::clicked, [&mainWindow, classGridLayout]() {
        showCreateClassDialog(&mainWindow, classGridLayout);
    });

    // ============================================================
    // TRANG 3: LATE & ABSENT REPORTS (Báo cáo đi trễ/vắng trong tuần)
    // ============================================================
    QWidget *pageAttendanceLogs = new QWidget(centerStack);
    QVBoxLayout *logsPageLayout = new QVBoxLayout(pageAttendanceLogs);
    logsPageLayout->setContentsMargins(24, 20, 24, 20);
    logsPageLayout->setSpacing(18);

    // Header trang báo cáo
    QHBoxLayout *schedHeaderLayout = new QHBoxLayout();
    QLabel *lblSchedPageTitle = new QLabel("BÁO CÁO ĐI TRỄ & VẮNG MẶT TRONG TUẦN", pageAttendanceLogs);
    lblSchedPageTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #F8FAFC;");
    schedHeaderLayout->addWidget(lblSchedPageTitle);
    
    // Label phụ hiển thị phạm vi thời gian báo cáo
    QLabel *lblCurrentDate = new QLabel("7 ngày gần nhất", pageAttendanceLogs);
    lblCurrentDate->setStyleSheet("font-size: 13px; font-weight: bold; color: #EF4444;");

    // Nút "Xuất báo cáo PDF" màu xanh lá cây đồng bộ
    QPushButton *btnExportPdf = new QPushButton("📄 Xuất báo cáo PDF", pageAttendanceLogs);
    btnExportPdf->setStyleSheet(R"(
        QPushButton {
            background-color: #10B981;
            color: white;
            padding: 6px 14px;
            border-radius: 6px;
            font-weight: bold;
            font-size: 12px;
            border: none;
        }
        QPushButton:hover {
            background-color: #059669;
        }
    )");
    btnExportPdf->setCursor(Qt::PointingHandCursor);

    // Sự kiện click nút Xuất PDF
    QObject::connect(btnExportPdf, &QPushButton::clicked, [&]() {
        showExportPdfDialog(&mainWindow);
    });

    schedHeaderLayout->addStretch();
    schedHeaderLayout->addWidget(btnExportPdf);
    schedHeaderLayout->addSpacing(10);
    schedHeaderLayout->addWidget(lblCurrentDate);
    logsPageLayout->addLayout(schedHeaderLayout);

    // ScrollArea chứa danh sách thẻ báo cáo
    QScrollArea *schedScrollArea = new QScrollArea(pageAttendanceLogs);
    schedScrollArea->setWidgetResizable(true);
    schedScrollArea->setStyleSheet(R"(
        QScrollArea {
            border: none;
            background: transparent;
        }
        QScrollBar:vertical {
            width: 8px;
            background-color: #0F172A;
        }
        QScrollBar::handle:vertical {
            background-color: #334155;
            border-radius: 4px;
        }
    )");

    // Widget nội dung báo cáo - đặt objectName để triggerReportRefresh tìm được
    QWidget *schedScrollContent = new QWidget();
    schedScrollContent->setObjectName("schedScrollContent"); // QUAN TRỌNG: Cần cho triggerReportRefresh
    schedScrollContent->setStyleSheet("background: transparent;");

    // Layout chứa các thẻ báo cáo (được truyền vào refreshLateAbsentReport)
    QVBoxLayout *schedListLayout = new QVBoxLayout(schedScrollContent);
    schedListLayout->setSpacing(16);
    schedListLayout->setContentsMargins(0, 0, 0, 0);
    schedListLayout->setAlignment(Qt::AlignTop); // Thẻ dồn lên trên
    schedScrollArea->setWidget(schedScrollContent);
    logsPageLayout->addWidget(schedScrollArea);

    // ============================================================
    // TRANG 4: SETTINGS (Cài đặt hệ thống)
    // ============================================================
    QWidget *pageSettings = new QWidget(centerStack);
    QVBoxLayout *settingsPageLayout = new QVBoxLayout(pageSettings);
    settingsPageLayout->setContentsMargins(24, 20, 24, 20);
    settingsPageLayout->setSpacing(16);
    settingsPageLayout->setAlignment(Qt::AlignTop); // Nội dung dồn lên trên

    QLabel *lblSettingsTitle = new QLabel("CÀI ĐẶT HỆ THỐNG", pageSettings);
    lblSettingsTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #F8FAFC; margin-bottom: 10px;");
    settingsPageLayout->addWidget(lblSettingsTitle);

    // ============================================================
    // PHẦN 4.1: THÔNG TIN TÀI KHOẢN GIÁO VIÊN
    // ============================================================
    QFrame *accountCard = new QFrame(pageSettings);
    accountCard->setStyleSheet(R"(
        QFrame {
            background-color: #1E293B;
            border: 1px solid #334155;
            border-radius: 10px;
        }
    )");
    QVBoxLayout *accountLayout = new QVBoxLayout(accountCard);
    accountLayout->setContentsMargins(20, 20, 20, 20);
    accountLayout->setSpacing(16);

    QLabel *lblAccountSection = new QLabel("👤 THÔNG TIN TÀI KHOẢN", accountCard);
    lblAccountSection->setStyleSheet("font-size: 12px; font-weight: bold; color: #3B82F6; border: none;");
    accountLayout->addWidget(lblAccountSection);

    QHBoxLayout *accountInfoLayout = new QHBoxLayout();
    accountInfoLayout->setSpacing(14);

    // ============================================================
    // AVATAR GIÁO VIÊN (TRANG SETTINGS) - Thiết kế cải tiến
    // Hình tròn 64x64 với gradient xanh dương -> tím, glow effect
    // ============================================================
    QLabel *lblAvatar = new QLabel(accountCard);
    lblAvatar->setFixedSize(64, 64);
    lblAvatar->setText("GV");
    lblAvatar->setAlignment(Qt::AlignCenter);
    lblAvatar->setStyleSheet(R"(
        QLabel {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #3B82F6,stop:0.5 #6366F1,stop:1 #8B5CF6);
            color: #FFFFFF;
            font-size: 22px;
            font-weight: bold;
            border-radius: 32px;
            border: 3px solid #60A5FA;
        }
    )");

    QVBoxLayout *userDetailLayout = new QVBoxLayout();
    userDetailLayout->setSpacing(4);

    // Nhãn thông tin tài khoản (sẽ được cập nhật sau khi đăng nhập)
    QLabel *lblUserDetail = new QLabel("Tài khoản giáo viên: admin", accountCard);
    lblUserDetail->setStyleSheet("font-size: 14px; color: #F8FAFC; border: none; font-weight: bold;");

    QLabel *lblRoleDetail = new QLabel("Quyền hạn: Quản trị viên · Hệ thống UTH FacieID", accountCard);
    lblRoleDetail->setStyleSheet("font-size: 11px; color: #94A3B8; border: none;");

    // Badge vai trò
    QLabel *lblRoleBadge = new QLabel("👨‍🏫 Giáo viên", accountCard);
    lblRoleBadge->setStyleSheet(
        "background-color: #1D4ED8; color: #BFDBFE; font-size: 10px; font-weight: bold;"
        " padding: 2px 8px; border-radius: 6px; border: none;"
    );

    userDetailLayout->addWidget(lblUserDetail);
    userDetailLayout->addWidget(lblRoleDetail);
    userDetailLayout->addWidget(lblRoleBadge);

    // Nút Đăng xuất (màu đỏ)
    QPushButton *btnLogout = new QPushButton("🚪  Đăng xuất", accountCard);
    btnLogout->setFixedSize(120, 34);
    btnLogout->setStyleSheet(R"(
        QPushButton {
            background-color: #7F1D1D;
            color: #FCA5A5;
            border-radius: 8px;
            font-weight: bold;
            font-size: 12px;
            border: 1px solid #EF4444;
        }
        QPushButton:hover {
            background-color: #EF4444;
            color: white;
        }
    )");

    accountInfoLayout->addWidget(lblAvatar);
    accountInfoLayout->addLayout(userDetailLayout);
    accountInfoLayout->addStretch();
    accountInfoLayout->addWidget(btnLogout, 0, Qt::AlignTop);
    accountLayout->addLayout(accountInfoLayout);
    settingsPageLayout->addWidget(accountCard);

    // ============================================================
    // SỰ KIỆN: Nút Đăng xuất
    // Ẩn cửa sổ chính -> hiển thị dialog đăng nhập lại
    // Nếu đăng nhập thành công -> hiện cửa sổ lại
    // Nếu nhấn Thoát -> thoát ứng dụng hoàn toàn
    // ============================================================
    QObject::connect(btnLogout, &QPushButton::clicked, [&mainWindow, profName, profAvatar, lblUserDetail]() {
        mainWindow.hide(); // Ẩn cửa sổ chính trong khi đăng nhập

        QString nextUser = "";
        if (showLoginDialog(nextUser, &mainWindow)) {
            // Đăng nhập thành công: Cập nhật thông tin tài khoản trên UI
            profName->setText(nextUser);                         // Cập nhật tên ở sidebar
            profAvatar->setText(nextUser.left(1).toUpper());    // Cập nhật chữ cái đầu avatar
            lblUserDetail->setText("Tài khoản giáo viên: " + nextUser); // Cập nhật trang Settings
            mainWindow.show(); // Hiện lại cửa sổ chính
        } else {
            // Người dùng nhấn Thoát ở dialog đăng nhập -> thoát hoàn toàn
            QApplication::quit();
        }
    });

    // ============================================================
    // THÊM TẤT CẢ 4 TRANG VÀO STACKEDWIDGET
    // Thứ tự phải khớp với thứ tự nút menu (index 0, 1, 2, 3)
    // ============================================================
    centerStack->addWidget(pageFaceScan);       // Index 0: Face Scan
    centerStack->addWidget(pageClassManagement); // Index 1: Class Management
    centerStack->addWidget(pageAttendanceLogs); // Index 2: Đi trễ & Vắng
    centerStack->addWidget(pageSettings);       // Index 3: Settings

    // ============================================================
    // KẾT NỐI SỰ KIỆN CLICK MENU TRÁI ĐỂ CHUYỂN TRANG
    // Mỗi nút menu khi click sẽ:
    //   1. Cập nhật style của tất cả nút (active/inactive)
    //   2. Làm mới dữ liệu nếu cần (ComboBox, báo cáo,...)
    //   3. Chuyển QStackedWidget sang trang tương ứng
    // ============================================================
    for (int i = 0; i < menuItems.size(); ++i) {
        QObject::connect(menuButtons[i], &QPushButton::clicked, [i, centerStack, menuButtons, schedListLayout, comboClass]() {
            // Cập nhật style tất cả nút (chỉ nút được click mới active)
            for (int k = 0; k < menuButtons.size(); ++k) {
                menuButtons[k]->setChecked(k == i); // Chỉ nút thứ i được checked

                if (k == i) {
                    // Nút active: Màu xanh nền đậm, chữ trắng
                    menuButtons[k]->setStyleSheet(R"(
                        QPushButton {
                            text-align: left;
                            padding-left: 14px;
                            font-size: 13px;
                            font-weight: bold;
                            color: #FFFFFF;
                            background-color: #2563EB;
                            border: none;
                            border-radius: 8px;
                        }
                    )");
                } else {
                    // Nút inactive: Nền trong suốt, chữ mờ
                    menuButtons[k]->setStyleSheet(R"(
                        QPushButton {
                            text-align: left;
                            padding-left: 14px;
                            font-size: 13px;
                            color: #94A3B8;
                            background-color: transparent;
                            border: none;
                        }
                        QPushButton:hover {
                            background-color: #334155;
                            color: #F8FAFC;
                            border-radius: 8px;
                        }
                    )");
                }
            }
            
            // Khi chuyển về trang Face Scan (index 0): Làm mới ComboBox lớp học
            // (vì có thể đã thêm/xóa lớp ở trang Class Management)
            if (i == 0) {
                refreshClassComboBox(comboClass);
            }

            // Khi chuyển sang trang Báo cáo (index 2): Tự động làm mới danh sách
            if (i == 2) {
                refreshLateAbsentReport(schedListLayout, centerStack->window());
            }
            
            // Thực hiện chuyển trang
            centerStack->setCurrentIndex(i);
        });
    }

    // ====================================================================
    // PHẦN 3: SIDEBAR BÊN PHẢI (LỊCH SỬ ĐIỂM DANH REALTIME)
    // Chiều rộng cố định 280px
    // Hiển thị danh sách sinh viên vừa điểm danh theo thời gian thực
    // ====================================================================
    QWidget *rightSidebar = new QWidget(centralWidget);
    rightSidebar->setFixedWidth(280);
    rightSidebar->setStyleSheet("background-color: #1E293B; border-left: 1px solid #334155;");
    
    QVBoxLayout *rightLayout = new QVBoxLayout(rightSidebar);
    rightLayout->setContentsMargins(16, 20, 16, 20);
    rightLayout->setSpacing(14);

    // Header sidebar phải: Tiêu đề + Badge "LIVE"
    QHBoxLayout *rightHeader = new QHBoxLayout();
    QLabel *lblRightTitle = new QLabel("Lịch sử điểm danh", rightSidebar);
    lblRightTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #F1F5F9;");
    
    // Badge "LIVE" màu xanh lá - biểu thị camera đang hoạt động realtime
    QPushButton *lblStatusLive = new QPushButton("LIVE", rightSidebar);
    lblStatusLive->setStyleSheet("background-color: #10B981; color: #FFFFFF; font-size: 9px; font-weight: bold; padding: 2px 6px; border-radius: 4px; border: none;");
    
    rightHeader->addWidget(lblRightTitle);
    rightHeader->addStretch();
    rightHeader->addWidget(lblStatusLive);
    rightLayout->addLayout(rightHeader);

    // QListWidget cuộn hiển thị lịch sử điểm danh
    // Mỗi item là một custom widget tạo bởi createAttendanceItemWidget
    // Cho phép click để xem popup chi tiết (ảnh + thông tin đầy đủ)
    QListWidget *logList = new QListWidget(rightSidebar);
    logList->setStyleSheet(R"(
        QListWidget {
            border: none;
            background: transparent;
        }
        QListWidget::item {
            border-radius: 8px;
            border: none;
        }
        QListWidget::item:hover {
            background-color: #334155;
        }
        QListWidget::item:selected {
            background-color: #1E3A5F;
        }
    )");
    logList->setFocusPolicy(Qt::NoFocus);
    logList->setCursor(Qt::PointingHandCursor); // Con trỏ tay = có thể click
    rightLayout->addWidget(logList, 4);

    // Placeholder khi chưa có ai điểm danh
    QLabel *lblNoData = new QLabel("Chưa có ai điểm danh.\nMở camera để bắt đầu.", rightSidebar);
    lblNoData->setAlignment(Qt::AlignCenter);
    lblNoData->setStyleSheet("color: #64748B; font-size: 12px; padding: 30px 0;");
    rightLayout->addWidget(lblNoData);

    // Label gợi ý nhấn để xem chi tiết
    QLabel *lblClickHint = new QLabel("💡 Nhấn vào mục để xem chi tiết", rightSidebar);
    lblClickHint->setAlignment(Qt::AlignCenter);
    lblClickHint->setStyleSheet("color: #475569; font-size: 10px; padding: 4px 0;");
    rightLayout->addWidget(lblClickHint);
    lblClickHint->hide(); // Ẩn khi chưa có dữ liệu

    rightLayout->addStretch();

    // Nút "Xem tất cả danh sách" mở dialog xem toàn bộ SV đã đăng ký
    QPushButton *btnViewAll = new QPushButton("📋  Xem tất cả danh sách", rightSidebar);
    btnViewAll->setStyleSheet(R"(
        QPushButton {
            background-color: #334155;
            color: #F8FAFC;
            font-weight: bold;
            padding: 9px;
            border-radius: 6px;
            border: none;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #475569;
        }
    )");
    rightLayout->addWidget(btnViewAll);

    // ============================================================
    // SỰ KIỆN: NHẤN VÀO ITEM TRONG LỊCH SỬ ĐIỂM DANH
    // Hiển thị popup thẻ chi tiết: Ảnh chụp + MSSV + Tên + Lớp + Trễ
    // ============================================================
    QObject::connect(logList, &QListWidget::itemClicked, [&mainWindow, logList](QListWidgetItem *clickedItem) {
        if (!clickedItem) return;

        // Lấy custom widget gắn với item
        QWidget *w = logList->itemWidget(clickedItem);
        if (!w) return;

        // Đọc metadata đã lưu vào properties khi tạo widget
        QString sv_name    = w->property("sv_name").toString();
        QString sv_id      = w->property("sv_id").toString();
        QString sv_maLop   = w->property("sv_maLop").toString();
        QString sv_time    = w->property("sv_time").toString();
        int     sv_late    = w->property("sv_late").toInt();
        QString sv_imgPath = w->property("sv_imgPath").toString();

        // ============================================================
        // TẠO POPUP THẺ CHI TIẾT
        // ============================================================
        QDialog popup(&mainWindow);
        popup.setWindowTitle("Chi tiết điểm danh");
        popup.setFixedSize(340, 440);
        popup.setStyleSheet(R"(
            QDialog {
                background-color: #0F172A;
                border: 1px solid #334155;
            }
            QLabel { border: none; background: transparent; }
        )");

        QVBoxLayout *popLayout = new QVBoxLayout(&popup);
        popLayout->setContentsMargins(20, 20, 20, 20);
        popLayout->setSpacing(14);

        // --- ẢNH CHỤP LÚC ĐIỂM DANH ---
        QLabel *lblPhoto = new QLabel(&popup);
        lblPhoto->setFixedSize(300, 200);
        lblPhoto->setAlignment(Qt::AlignCenter);
        lblPhoto->setStyleSheet(
            "background-color: #1E293B;"
            " border-radius: 12px;"
            " border: 2px solid #334155;"
        );

        if (!sv_imgPath.isEmpty() && QFile::exists(sv_imgPath)) {
            QPixmap pix(sv_imgPath);
            lblPhoto->setPixmap(pix.scaled(300, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            lblPhoto->setText("📷 Không có ảnh");
            lblPhoto->setStyleSheet(
                "background-color: #1E293B; border-radius: 12px; border: 2px solid #334155;"
                " color: #475569; font-size: 14px;"
            );
        }
        popLayout->addWidget(lblPhoto, 0, Qt::AlignCenter);

        // --- THÔNG TIN SINH VIÊN ---
        QFrame *infoBox = new QFrame(&popup);
        infoBox->setStyleSheet(
            "QFrame { background-color: #1E293B; border-radius: 10px; border: 1px solid #334155; }"
        );
        QVBoxLayout *infoBoxLayout = new QVBoxLayout(infoBox);
        infoBoxLayout->setContentsMargins(16, 14, 16, 14);
        infoBoxLayout->setSpacing(8);

        // Hàng tên + badge trạng thái
        QHBoxLayout *nameRow = new QHBoxLayout();
        QLabel *lblPopName = new QLabel(sv_name, &popup);
        lblPopName->setStyleSheet("font-size: 15px; font-weight: bold; color: #F8FAFC;");
        nameRow->addWidget(lblPopName);
        nameRow->addStretch();
        if (sv_late > 0) {
            QLabel *latePill = new QLabel(QString("⏰ Trễ %1 phút").arg(sv_late), &popup);
            latePill->setStyleSheet(
                "background-color: #92400E; color: #FCD34D; font-size: 10px; font-weight: bold;"
                " padding: 3px 8px; border-radius: 6px; border: none;"
            );
            nameRow->addWidget(latePill);
        } else {
            QLabel *onTimePill = new QLabel("✅ Đúng giờ", &popup);
            onTimePill->setStyleSheet(
                "background-color: #064E3B; color: #6EE7B7; font-size: 10px; font-weight: bold;"
                " padding: 3px 8px; border-radius: 6px; border: none;"
            );
            nameRow->addWidget(onTimePill);
        }
        infoBoxLayout->addLayout(nameRow);

        // Separator
        QFrame *sep = new QFrame(&popup);
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("border: none; background-color: #334155; max-height: 1px;");
        infoBoxLayout->addWidget(sep);

        // Các dòng thông tin
        auto addInfoRow = [&](const QString &icon, const QString &label, const QString &value, const QString &valColor = "#E2E8F0") {
            QHBoxLayout *row = new QHBoxLayout();
            row->setSpacing(8);
            QLabel *lblIcon = new QLabel(icon + " " + label + ":", &popup);
            lblIcon->setStyleSheet("font-size: 11px; color: #94A3B8; font-weight: bold;");
            lblIcon->setFixedWidth(90);
            QLabel *lblVal = new QLabel(value, &popup);
            lblVal->setStyleSheet(QString("font-size: 12px; color: %1; font-weight: bold;").arg(valColor));
            lblVal->setWordWrap(true);
            row->addWidget(lblIcon);
            row->addWidget(lblVal, 1);
            infoBoxLayout->addLayout(row);
        };

        addInfoRow("🆔", "MSSV",  sv_id,   "#60A5FA");
        addInfoRow("🏛️", "Lớp",   sv_maLop.isEmpty() ? "—" : sv_maLop, "#A78BFA");
        addInfoRow("🕐", "Giờ vào", sv_time, "#34D399");
        if (sv_late > 0) {
            addInfoRow("⚠️", "Đi trễ", QString("%1 phút").arg(sv_late), "#FCD34D");
        }

        popLayout->addWidget(infoBox);

        // Nút đóng
        QPushButton *btnClose = new QPushButton("Đóng", &popup);
        btnClose->setStyleSheet(R"(
            QPushButton {
                background-color: #2563EB;
                color: white;
                font-weight: bold;
                padding: 8px;
                border-radius: 8px;
                border: none;
                font-size: 13px;
            }
            QPushButton:hover { background-color: #1D4ED8; }
        )");
        QObject::connect(btnClose, &QPushButton::clicked, &popup, &QDialog::accept);
        popLayout->addWidget(btnClose);

        popup.exec();
    });

    // ====================================================================
    // LẮP GHÉP 3 PHẦN VÀO CỬA SỔ CHÍNH
    // [Sidebar trái 220px] [Center - co giãn] [Sidebar phải 280px]
    // ====================================================================
    mainLayout->addWidget(sidebar);       // Cột trái: Menu + Trạng thái
    mainLayout->addWidget(centerStack);   // Cột giữa: Nội dung 4 trang (co giãn)
    mainLayout->addWidget(rightSidebar);  // Cột phải: Lịch sử điểm danh
    mainWindow.setCentralWidget(centralWidget); // Đặt làm widget trung tâm của MainWindow

    // ====================================================================
    // LOGIC SỰ KIỆN VÀ KHỞI TẠO TRỄ
    // ====================================================================

    // ============================================================
    // KHỞI TẠO TRỄ 200ms SAU KHI GIAO DIỆN HIỂN THỊDO
    // Lý do: Đảm bảo cửa sổ đã hiển thị xong trước khi thực hiện
    // các tác vụ nặng (nạp mô hình AI, truy vấn DB...)
    // QTimer::singleShot không lặp lại, chỉ kích hoạt 1 lần
    // ============================================================
    QTimer::singleShot(200, [&]() {
        if (dbConnected) {
            // Cập nhật trạng thái DB thành "Đã kết nối" (màu xanh lá)
            lblDbStatus->setText("Đã kết nối ✓");
            lblDbStatus->setStyleSheet("font-size: 11px; color: #10B981; font-weight: bold; border: none;");

            // ============================================================
            // NẠP CÁC MÔ HÌNH AI ONNX
            // Tìm file theo nhiều đường dẫn fallback vì có thể chạy từ nhiều vị trí
            // ============================================================
            QString appDir = QCoreApplication::applicationDirPath();

            // Tìm file YuNet (phát hiện mặt)
            QString detPath = appDir + "/models/face_detection_yunet_2023mar.onnx";
            if (!QFile::exists(detPath)) detPath = appDir + "/../models/face_detection_yunet_2023mar.onnx";
            if (!QFile::exists(detPath)) detPath = appDir + "/../../models/face_detection_yunet_2023mar.onnx";
            if (!QFile::exists(detPath)) detPath = QDir::currentPath() + "/models/face_detection_yunet_2023mar.onnx";
            if (!QFile::exists(detPath)) detPath = QDir::currentPath() + "/../models/face_detection_yunet_2023mar.onnx";

            // Tìm file SFace (nhận diện mặt)
            QString recPath = appDir + "/models/face_recognition_sface_2021dec.onnx";
            if (!QFile::exists(recPath)) recPath = appDir + "/../models/face_recognition_sface_2021dec.onnx";
            if (!QFile::exists(recPath)) recPath = appDir + "/../../models/face_recognition_sface_2021dec.onnx";
            if (!QFile::exists(recPath)) recPath = QDir::currentPath() + "/models/face_recognition_sface_2021dec.onnx";
            if (!QFile::exists(recPath)) recPath = QDir::currentPath() + "/../models/face_recognition_sface_2021dec.onnx";

            // Nạp cả 2 mô hình
            if (Face::loadModels(detPath, recPath)) {
                qDebug() << "[MAIN] Mô hình YuNet & SFace đã sẵn sàng.";
            } else {
                QMessageBox::warning(&mainWindow, "Cảnh báo AI",
                    "Không thể nạp các mô hình ONNX.\n\n"
                    "Hãy chắc chắn các file nằm trong thư mục 'models/'.");
            }

            // ============================================================
            // TẢI LỊCH SỬ ĐIỂM DANH HÔM NAY TỪ SQLite LÊN UI
            // (Để hiển thị các lượt điểm danh từ phiên trước nếu app được mở lại)
            // ============================================================
            auto todayRecords = Database::loadTodayAttendance();
            for (const auto &rec : todayRecords) {
                QListWidgetItem *item = new QListWidgetItem(logList);
                item->setSizeHint(QSize(0, 58));

                // Tính số phút trễ dựa trên lớp học tương ứng (nếu có)
                int lateMin = 0;
                auto allClasses = Database::loadAllClasses();
                QString recDate = rec.checkInTime.toString("yyyy-MM-dd");
                for (const auto &cls : allClasses) {
                    if (cls.ngayHoc == recDate) {
                        // Tìm lớp của sinh viên này
                        auto svList = Database::loadStudentsByClass(cls.maLop);
                        bool found = false;
                        for (const auto &sv : svList) { if (sv.mssv == rec.mssv) { found = true; break; } }
                        if (found) {
                            QTime startTime = QTime::fromString(cls.gioBatDau, "HH:mm");
                            QTime checkTime = rec.checkInTime.time();
                            if (checkTime > startTime) lateMin = startTime.secsTo(checkTime) / 60;
                            break;
                        }
                    }
                }

                // Tìm mã lớp của sinh viên
                QString svMaLop = "";
                for (const auto &cls : allClasses) {
                    auto svList = Database::loadStudentsByClass(cls.maLop);
                    for (const auto &sv : svList) { if (sv.mssv == rec.mssv) { svMaLop = cls.maLop; break; } }
                    if (!svMaLop.isEmpty()) break;
                }

                QWidget *itemWidget = createAttendanceItemWidget(
                    rec.hoTen, rec.mssv, svMaLop,
                    rec.checkInTime.toString("HH:mm:ss"),
                    lateMin > 0 ? "ĐI TRỄ" : "ĐÃ XÁC THỰC",
                    lateMin > 0 ? "#F59E0B" : "#3B82F6",
                    lateMin,
                    rec.capturedImagePath
                );
                logList->setItemWidget(item, itemWidget);
            }

            // Cập nhật trạng thái hiển thị và bộ đếm
            if (!todayRecords.empty()) {
                lblNoData->hide();
                lblClickHint->show();
                attendanceCount = static_cast<int>(todayRecords.size());
                lblValPresent->setText(QString::number(attendanceCount));
            }

            // Tải và hiển thị lưới thẻ lớp học
            refreshClassGrid(classGridLayout, &mainWindow);
        } else {
            // Kết nối SQLite thất bại
            lblDbStatus->setText("Kết nối thất bại ✗");
            lblDbStatus->setStyleSheet("font-size: 11px; color: #EF4444; font-weight: bold; border: none;");
            QMessageBox::critical(&mainWindow, "Lỗi Database",
                "Không thể khởi tạo SQLite Database. Vui lòng kiểm tra quyền ghi của ứng dụng.");
        }
    });

    // ============================================================
    // SỰ KIỆN: NÚT "THÊM SINH VIÊN" (Ở header trang Face Scan)
    // Mở dialog đăng ký FaceID và cập nhật danh sách sinh viên trong RAM
    // ============================================================
    QObject::connect(btnAddStudent, &QPushButton::clicked, [&]() {
        if (!dbConnected) {
            QMessageBox::warning(&mainWindow, "Chưa kết nối CSDL", "Vui lòng kết nối database trước.");
            return;
        }

        // Mở dialog đăng ký không có lớp pre-filled (người dùng tự nhập mã lớp)
        if (showAddStudentDialog(&mainWindow)) {
            // Đăng ký thành công -> Cập nhật lại danh sách sinh viên trong RAM để AI nhận diện

            // Lấy bộ lọc lớp hiện tại từ ComboBox
            QString filter = comboClass->currentData().toString();
            std::vector<Database::StudentData> students;

            if (filter.isEmpty()) {
                students = Database::loadAllStudents(); // Tải tất cả lớp
            } else {
                students = Database::loadStudentsByClass(filter); // Chỉ tải lớp được chọn
            }

            // Chuyển đổi từ Database::StudentData sang Face::StudentRef
            std::vector<Face::StudentRef> refs;
            for (const auto &s : students) {
                Face::StudentRef ref;
                ref.mssv = s.mssv;
                ref.hoTen = s.hoTen;
                ref.faceVector = s.faceVector;
                refs.push_back(ref);
            }
            Face::setStudentDatabase(refs); // Cập nhật danh sách so khớp trong RAM

            // Cập nhật thẻ thống kê sĩ số
            lblValTotal->setText(QString::number(students.size()));
            
            // Tính lại số vắng (Tổng - Có mặt)
            int present = lblValPresent->text().toInt();
            int absent = students.size() - present;
            lblValAbsent->setText(QString::number(qMax(0, absent)));
        }
    });

    // ============================================================
    // SỰ KIỆN: NÚT "BẮT ĐẦU QUÉT" - Mở Camera và bắt đầu nhận diện
    // ============================================================
    QObject::connect(btnStartCam, &QPushButton::clicked, [&]() {
        if (!dbConnected) {
            QMessageBox::warning(&mainWindow, "Chưa kết nối", "Chưa kết nối được SQLite Database.");
            return;
        }

        // ============================================================
        // TẢI DANH SÁCH SINH VIÊN LÊN RAM ĐỂ SO KHỚP REALTIME
        // Lý do lưu RAM: Tránh truy vấn SQLite trong mỗi frame (30 lần/giây)
        // ============================================================
        QString filter = comboClass->currentData().toString();
        std::vector<Database::StudentData> students;

        if (filter.isEmpty()) {
            students = Database::loadAllStudents(); // Chế độ tất cả lớp
        } else {
            students = Database::loadStudentsByClass(filter); // Chỉ lớp được chọn
        }

        // Chuyển đổi dữ liệu sang format Face::StudentRef
        std::vector<Face::StudentRef> refs;
        for (const auto &s : students) {
            Face::StudentRef ref;
            ref.mssv = s.mssv;
            ref.hoTen = s.hoTen;
            ref.faceVector = s.faceVector;
            refs.push_back(ref);
        }
        Face::setStudentDatabase(refs); // Cập nhật database AI so khớp

        // Cập nhật thẻ thống kê sĩ số
        lblValTotal->setText(QString::number(students.size()));
        int present = lblValPresent->text().toInt();
        int absent = students.size() - present;
        lblValAbsent->setText(QString::number(qMax(0, absent)));

        // ============================================================
        // CALLBACK XỬ LÝ FRAME: onFrameCallback
        // Được gọi mỗi 33ms (30 FPS) bởi Camera module
        // Pipeline: Frame camera -> AI xử lý -> Vẽ kết quả -> Hiển thị
        // ============================================================
        auto onFrameCallback = [&](const QImage &frame) {
            QImage processedFrame; // Frame sau khi AI đã vẽ bounding box
            
            // ============================================================
            // CALLBACK NHẬN DIỆN THÀNH CÔNG: onRecognizedCallback
            // Được gọi mỗi khi AI nhận diện khớp 1 sinh viên trong frame
            // ============================================================
            auto onRecognizedCallback = [&](const Face::FaceResult &res) {
                // Kiểm tra sinh viên đã điểm danh hôm nay chưa (tránh ghi trùng)
                if (Database::hasCheckedInToday(res.mssv)) {
                    return;
                }

                // ============================================================
                // LƯU ẢNH BẰNG CHỨNG ĐIỂM DANH
                // ============================================================
                QString imgDir = QCoreApplication::applicationDirPath() + "/checkin_images";
                QDir().mkpath(imgDir);

                QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
                QString imgFileName = QString("%1/%2_%3.jpg").arg(imgDir, res.mssv, timestamp);

                if (!frame.isNull()) {
                    frame.save(imgFileName, "JPG", 85);
                }

                // ============================================================
                // TÍNH SỐ PHÚT ĐI TRỄ
                // So sánh giờ điểm danh hiện tại với giờ bắt đầu của lớp
                // được chọn trong ComboBox (nếu có chọn lớp cụ thể)
                // ============================================================
                int lateMinutes = 0;
                QString svMaLop = comboClass->currentData().toString(); // Lớp đang chọn

                // Lấy thứ trong tuần viết tắt của ngày hôm nay
                int dayNum = QDate::currentDate().dayOfWeek();
                QString dayStr = "";
                switch (dayNum) {
                    case 1: dayStr = "Mon"; break;
                    case 2: dayStr = "Tue"; break;
                    case 3: dayStr = "Wed"; break;
                    case 4: dayStr = "Thu"; break;
                    case 5: dayStr = "Fri"; break;
                    case 6: dayStr = "Sat"; break;
                    case 7: dayStr = "Sun"; break;
                }

                if (!svMaLop.isEmpty()) {
                    // Có chọn lớp cụ thể: Tìm giờ học của lớp đó hôm nay (so khớp thứ trong tuần)
                    auto allClasses = Database::loadAllClasses();
                    for (const auto &cls : allClasses) {
                        if (cls.maLop == svMaLop && cls.ngayHoc.split(",").contains(dayStr)) {
                            // Chỉ tính trễ nếu hôm nay lớn hơn hoặc bằng ngày tạo lớp
                            QDate creationDate = QDate::fromString(cls.ngayTao, "yyyy-MM-dd");
                            if (!creationDate.isValid() || QDate::currentDate() >= creationDate) {
                                QTime startTime = QTime::fromString(cls.gioBatDau, "HH:mm");
                                QTime nowTime   = QTime::currentTime();
                                if (nowTime > startTime) {
                                    lateMinutes = startTime.secsTo(nowTime) / 60;
                                }
                            }
                            break;
                        }
                    }
                } else {
                    // Chế độ "Tất cả lớp": Tìm lớp học của sinh viên này và kiểm tra thứ học
                    auto allClasses = Database::loadAllClasses();
                    for (const auto &cls : allClasses) {
                        if (cls.ngayHoc.split(",").contains(dayStr)) {
                            // Chỉ tính trễ nếu hôm nay lớn hơn hoặc bằng ngày tạo lớp
                            QDate creationDate = QDate::fromString(cls.ngayTao, "yyyy-MM-dd");
                            if (!creationDate.isValid() || QDate::currentDate() >= creationDate) {
                                auto svList = Database::loadStudentsByClass(cls.maLop);
                                for (const auto &sv : svList) {
                                    if (sv.mssv == res.mssv) {
                                        svMaLop = cls.maLop;
                                        QTime startTime = QTime::fromString(cls.gioBatDau, "HH:mm");
                                        QTime nowTime   = QTime::currentTime();
                                        if (nowTime > startTime) {
                                            lateMinutes = startTime.secsTo(nowTime) / 60;
                                        }
                                        break;
                                    }
                                }
                            }
                            if (!svMaLop.isEmpty()) break;
                        }
                    }
                }

                // Ghi nhận lịch sử điểm danh vào SQLite
                Database::recordCheckIn(res.mssv, imgFileName, res.confidence);

                // ============================================================
                // CẬP NHẬT GIAO DIỆN SAU KHI ĐIỂM DANH THÀNH CÔNG
                // ============================================================

                lblNoData->hide();
                lblClickHint->show();

                // Tạo item mới trong danh sách check-in (mới nhất trên đầu)
                QListWidgetItem *item = new QListWidgetItem();
                item->setSizeHint(QSize(0, 58));
                logList->insertItem(0, item);

                // Xác định màu trạng thái và nhãn dựa vào số phút trễ
                QString statusText  = (lateMinutes > 0) ? "ĐI TRỄ"     : "ĐÃ XÁC THỰC";
                QString statusColor = (lateMinutes > 0) ? "#F59E0B"     : "#10B981";

                QWidget *itemWidget = createAttendanceItemWidget(
                    res.hoTen, res.mssv, svMaLop,
                    QDateTime::currentDateTime().toString("HH:mm:ss"),
                    statusText, statusColor,
                    lateMinutes,
                    imgFileName
                );
                logList->setItemWidget(item, itemWidget);

                // Cập nhật bộ đếm và thẻ thống kê
                attendanceCount++;
                lblValPresent->setText(QString::number(attendanceCount));
                if (lateMinutes > 0) {
                    // Tăng số "Đi muộn"
                    int curLate = lblValLate->text().toInt();
                    lblValLate->setText(QString::number(curLate + 1));
                }

                int total = lblValTotal->text().toInt();
                int absCount = total - attendanceCount;
                lblValAbsent->setText(QString::number(qMax(0, absCount)));
            }; // Kết thúc onRecognizedCallback

            // Xử lý frame qua AI (detect + recognize + draw)
            // Truyền onRecognizedCallback để được gọi khi có kết quả khớp
            Face::processFrame(frame, processedFrame, onRecognizedCallback);

            // Hiển thị frame đã xử lý lên QLabel camera
            QPixmap pix = QPixmap::fromImage(processedFrame);
            lblCameraView->setPixmap(pix.scaled(
                lblCameraView->size(),        // Scale theo kích thước QLabel hiện tại
                Qt::KeepAspectRatio,          // Giữ tỉ lệ chiều rộng/cao
                Qt::SmoothTransformation      // Sử dụng interpolation mượt (chậm hơn nhưng đẹp hơn)
            ));
        }; // Kết thúc onFrameCallback

        // Callback lỗi camera: Cập nhật UI khi camera bị ngắt
        auto onErrorCallback = [&](const QString &err) {
            lblCameraView->setText("LỖI CAMERA:\n" + err);
            btnStartCam->setEnabled(true);  // Cho phép thử lại
            btnStopCam->setEnabled(false);  // Disable nút dừng
        };

        // Mở camera với FPS = 30 và truyền 2 callback vào
        if (Camera::openCamera(0, 30, onFrameCallback, onErrorCallback)) {
            btnStartCam->setEnabled(false); // Disable nút bắt đầu (đã bắt đầu rồi)
            btnStopCam->setEnabled(true);   // Enable nút dừng
            lblCameraView->setText("");     // Xóa text placeholder
        } else {
            QMessageBox::critical(&mainWindow, "Lỗi Camera", "Không thể khởi động camera.");
        }
    }); // Kết thúc sự kiện btnStartCam

    // ============================================================
    // SỰ KIỆN: NÚT "DỪNG QUÉT" - Tắt camera
    // ============================================================
    QObject::connect(btnStopCam, &QPushButton::clicked, [&]() {
        Camera::closeCamera();                // Đóng camera và giải phóng tài nguyên
        btnStartCam->setEnabled(true);        // Cho phép bật lại camera
        btnStopCam->setEnabled(false);        // Disable nút dừng
        lblCameraView->setText("CAMERA ĐÃ DỪNG\nNhấn 'Bắt đầu quét' để tiếp tục");
    });

    // ============================================================
    // SỰ KIỆN: NÚT "RESET ĐIỂM DANH" - Xóa lịch sử hôm nay
    // ============================================================
    QObject::connect(btnResetAttendance, &QPushButton::clicked, [&]() {
        if (!dbConnected) return;

        // Lấy bộ lọc lớp hiện tại
        QString filter = comboClass->currentData().toString();

        // Tạo thông báo xác nhận phù hợp với chế độ reset
        QString confirmMsg = filter.isEmpty() 
            ? "Bạn có chắc chắn muốn xóa toàn bộ lịch sử điểm danh ngày hôm nay?"
            : QString("Bạn có chắc chắn muốn xóa lịch sử điểm danh ngày hôm nay của lớp '%1'?").arg(filter);

        QMessageBox::StandardButton reply = QMessageBox::question(
            pageFaceScan, "Xác nhận Reset", confirmMsg,
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            if (Database::resetTodayAttendance(filter)) {
                // Xóa thành công: Reset toàn bộ UI về trạng thái ban đầu
                logList->clear();           // Xóa danh sách check-in
                lblNoData->show();          // Hiện placeholder
                attendanceCount = 0;        // Reset bộ đếm
                lblValPresent->setText("0"); // Reset thẻ "Có mặt"

                // Cập nhật thẻ "Vắng mặt" = Tổng sĩ số
                int total = lblValTotal->text().toInt();
                lblValAbsent->setText(QString::number(total));

                // Cập nhật lại báo cáo vắng/trễ (vì data đã thay đổi)
                triggerReportRefresh(pageFaceScan);

                QMessageBox::information(pageFaceScan, "Thành công", "Đã reset dữ liệu điểm danh ngày hôm nay thành công.");
            } else {
                QMessageBox::critical(pageFaceScan, "Lỗi", "Không thể reset dữ liệu điểm danh trong CSDL.");
            }
        }
    });

    // ============================================================
    // SỰ KIỆN: NÚT "XEM TẤT CẢ DANH SÁCH" (Sidebar phải)
    // Mở dialog hiển thị toàn bộ sinh viên đã đăng ký FaceID trong hệ thống
    // ============================================================
    QObject::connect(btnViewAll, &QPushButton::clicked, [&]() {
        if (!dbConnected) return;

        // Tạo dialog danh sách sinh viên
        QDialog logDialog(&mainWindow);
        logDialog.setWindowTitle("Chi Tiết Danh Sách Sinh Viên Đã Đăng Ký");
        logDialog.setFixedSize(480, 360);
        logDialog.setStyleSheet("QDialog { background-color: #1E293B; } QLabel { color: #E2E8F0; font-size: 12px; }");

        QVBoxLayout *dialogLayout = new QVBoxLayout(&logDialog);

        // Danh sách sinh viên đơn giản (QListWidget với text thông thường)
        QListWidget *studentList = new QListWidget(&logDialog);
        studentList->setStyleSheet("QListWidget { background-color: #0F172A; border: 1px solid #334155; border-radius: 6px; color: #FFFFFF; padding: 6px; }");
        
        // Tải và hiển thị tất cả sinh viên
        auto allStudents = Database::loadAllStudents();
        for (const auto &s : allStudents) {
            // Định dạng mỗi dòng: "MSSV: 20110123 | Họ tên: Nguyễn Văn A | Lớp: KTPM1"
            QString txt = QString("MSSV: %1 | Họ tên: %2 | Lớp: %3").arg(s.mssv, s.hoTen, s.maLop);
            studentList->addItem(txt);
        }
        
        // Hiển thị thông báo nếu chưa có sinh viên nào
        if (allStudents.empty()) {
            studentList->addItem("Chưa có sinh viên nào đăng ký.");
        }

        dialogLayout->addWidget(studentList);
        
        // Nút đóng dialog
        QPushButton *btnClose = new QPushButton("Đóng", &logDialog);
        btnClose->setStyleSheet("background-color: #3B82F6; color: white; font-weight: bold; padding: 6px; border-radius: 4px; border: none;");
        QObject::connect(btnClose, &QPushButton::clicked, &logDialog, &QDialog::accept);
        dialogLayout->addWidget(btnClose);

        logDialog.exec(); // Hiển thị dialog (blocking)
    });

    // ============================================================
    // HIỂN THỊ DIALOG ĐĂNG NHẬP TRƯỚC KHI MỞ CỬA SỔ CHÍNH
    // Bắt buộc người dùng phải xác thực danh tính trước
    // ============================================================
    QString currentUsername = "";
    if (!showLoginDialog(currentUsername)) {
        // Người dùng nhấn "Thoát" thay vì đăng nhập -> Thoát chương trình
        Camera::closeCamera();          // Dọn dẹp camera (đề phòng)
        Database::disconnectDatabase(); // Đóng kết nối SQLite an toàn
        return 0;                       // Trả về 0 = thoát bình thường
    }

    // Đăng nhập thành công: Cập nhật thông tin tài khoản lên UI
    profName->setText(currentUsername);                          // Tên ở sidebar trái
    profAvatar->setText(currentUsername.left(1).toUpper());      // Chữ cái đầu avatar
    lblUserDetail->setText("Tài khoản giáo viên: " + currentUsername); // Trang Settings

    // Hiển thị cửa sổ chính
    mainWindow.show();

    // ============================================================
    // VÒNG LẶP SỰ KIỆN QT (Qt Event Loop)
    // app.exec() chạy vòng lặp xử lý sự kiện Qt (click, timer, paint...)
    // Chặn ở đây cho đến khi người dùng đóng cửa sổ hoặc gọi QApplication::quit()
    // ============================================================
    int result = app.exec();

    // ============================================================
    // DỌN DẸP TÀI NGUYÊN TRƯỚC KHI THOÁT
    // Thực thi sau khi vòng lặp sự kiện kết thúc
    // ============================================================
    Camera::closeCamera();          // Đóng camera và giải phóng tài nguyên phần cứng
    Database::disconnectDatabase(); // Đóng kết nối SQLite và flush dữ liệu xuống đĩa

    return result; // Trả về mã thoát (0 = thành công)
}

// -----------------------------------------------------------------------
// showConfigureScheduleDialog: Hộp thoại thiết lập lịch học riêng biệt
// -----------------------------------------------------------------------
void showConfigureScheduleDialog(QWidget *parent, const QString &maLop, QGridLayout *gridLayout)
{
    // Tìm thông tin lớp học hiện tại để điền sẵn vào giao diện (pre-fill)
    auto allClasses = Database::loadAllClasses();
    Database::ClassData cls;
    bool found = false;
    for (const auto &c : allClasses) {
        if (c.maLop == maLop) {
            cls = c;
            found = true;
            break;
        }
    }
    if (!found) {
        QMessageBox::critical(parent, "Lỗi", "Không tìm thấy thông tin lớp học " + maLop);
        return;
    }

    QDialog dialog(parent);
    dialog.setWindowTitle("Thiết Lập Lịch Học - " + cls.tenLop);
    dialog.setFixedSize(500, 320);
    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #1E293B;
            border: 1px solid #334155;
        }
        QLabel {
            font-size: 12px;
            color: #E2E8F0;
            font-weight: bold;
        }
        QCheckBox {
            color: #E2E8F0;
            font-size: 11px;
        }
        QTimeEdit {
            padding: 6px;
            border: 1px solid #475569;
            border-radius: 6px;
            font-size: 12px;
            background-color: #0F172A;
            color: #FFFFFF;
        }
        QTimeEdit:focus {
            border: 1px solid #3B82F6;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(16);
    layout->setContentsMargins(24, 24, 24, 24);

    // Tiêu đề nhỏ
    QLabel *lblTitle = new QLabel(QString("Cấu hình lịch học lớp: %1 (%2)").arg(cls.tenLop, cls.maLop), &dialog);
    lblTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #3B82F6;");
    layout->addWidget(lblTitle);

    // Khối 1: Chọn ngày học (7 Checkbox thứ viết tắt)
    QLabel *lblDays = new QLabel("Chọn các ngày học trong tuần:", &dialog);
    layout->addWidget(lblDays);

    QHBoxLayout *daysLayout = new QHBoxLayout();
    daysLayout->setSpacing(8);

    QCheckBox *chkMon = new QCheckBox("Thứ 2", &dialog);
    QCheckBox *chkTue = new QCheckBox("Thứ 3", &dialog);
    QCheckBox *chkWed = new QCheckBox("Thứ 4", &dialog);
    QCheckBox *chkThu = new QCheckBox("Thứ 5", &dialog);
    QCheckBox *chkFri = new QCheckBox("Thứ 6", &dialog);
    QCheckBox *chkSat = new QCheckBox("Thứ 7", &dialog);
    QCheckBox *chkSun = new QCheckBox("Chủ Nhật", &dialog);

    daysLayout->addWidget(chkMon);
    daysLayout->addWidget(chkTue);
    daysLayout->addWidget(chkWed);
    daysLayout->addWidget(chkThu);
    daysLayout->addWidget(chkFri);
    daysLayout->addWidget(chkSat);
    daysLayout->addWidget(chkSun);
    layout->addLayout(daysLayout);

    // Tải dữ liệu các thứ học đã cấu hình trước đó để tích chọn sẵn
    QStringList currentDays = cls.ngayHoc.split(",");
    if (currentDays.contains("Mon")) chkMon->setChecked(true);
    if (currentDays.contains("Tue")) chkTue->setChecked(true);
    if (currentDays.contains("Wed")) chkWed->setChecked(true);
    if (currentDays.contains("Thu")) chkThu->setChecked(true);
    if (currentDays.contains("Fri")) chkFri->setChecked(true);
    if (currentDays.contains("Sat")) chkSat->setChecked(true);
    if (currentDays.contains("Sun")) chkSun->setChecked(true);

    // Khối 2: Chọn khung giờ học (Giờ bắt đầu - Giờ kết thúc)
    QHBoxLayout *timeLayout = new QHBoxLayout();
    timeLayout->setSpacing(20);

    QVBoxLayout *startLayout = new QVBoxLayout();
    startLayout->setSpacing(4);
    startLayout->addWidget(new QLabel("Giờ bắt đầu học:", &dialog));
    QTimeEdit *timeStart = new QTimeEdit(&dialog);
    timeStart->setDisplayFormat("HH:mm");
    if (!cls.gioBatDau.isEmpty()) {
        timeStart->setTime(QTime::fromString(cls.gioBatDau, "HH:mm"));
    } else {
        timeStart->setTime(QTime(8, 0)); // Mặc định 08:00
    }
    startLayout->addWidget(timeStart);

    QVBoxLayout *endLayout = new QVBoxLayout();
    endLayout->setSpacing(4);
    endLayout->addWidget(new QLabel("Giờ kết thúc học:", &dialog));
    QTimeEdit *timeEnd = new QTimeEdit(&dialog);
    timeEnd->setDisplayFormat("HH:mm");
    if (!cls.gioKetThuc.isEmpty()) {
        timeEnd->setTime(QTime::fromString(cls.gioKetThuc, "HH:mm"));
    } else {
        timeEnd->setTime(QTime(10, 30)); // Mặc định 10:30
    }
    endLayout->addWidget(timeEnd);

    timeLayout->addLayout(startLayout);
    timeLayout->addLayout(endLayout);
    layout->addLayout(timeLayout);

    // Khối 3: Hàng nút Hủy / Lưu
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *btnCancel = new QPushButton("Hủy", &dialog);
    btnCancel->setStyleSheet(R"(
        QPushButton {
            background-color: #334155;
            color: #94A3B8;
            padding: 6px 16px;
            border-radius: 6px;
            border: none;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #475569;
        }
    )");

    QPushButton *btnSave = new QPushButton("Lưu lịch học", &dialog);
    btnSave->setStyleSheet(R"(
        QPushButton {
            background-color: #10B981;
            color: white;
            padding: 6px 16px;
            border-radius: 6px;
            border: none;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #059669;
        }
    )");

    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);
    layout->addLayout(btnLayout);

    QObject::connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Xử lý sự kiện lưu lịch học
    QObject::connect(btnSave, &QPushButton::clicked, [&]() {
        // Thu thập các checkbox ngày
        QStringList selectedDays;
        if (chkMon->isChecked()) selectedDays.append("Mon");
        if (chkTue->isChecked()) selectedDays.append("Tue");
        if (chkWed->isChecked()) selectedDays.append("Wed");
        if (chkThu->isChecked()) selectedDays.append("Thu");
        if (chkFri->isChecked()) selectedDays.append("Fri");
        if (chkSat->isChecked()) selectedDays.append("Sat");
        if (chkSun->isChecked()) selectedDays.append("Sun");

        if (selectedDays.isEmpty()) {
            QMessageBox::warning(&dialog, "Thông báo", "Vui lòng chọn ít nhất một ngày học trong tuần!");
            return;
        }

        QString ngayHoc = selectedDays.join(",");
        QString gioBatDau = timeStart->time().toString("HH:mm");
        QString gioKetThuc = timeEnd->time().toString("HH:mm");

        if (timeStart->time() >= timeEnd->time()) {
            QMessageBox::warning(&dialog, "Thông báo", "Giờ bắt đầu học phải nhỏ hơn giờ kết thúc học!");
            return;
        }

        // Cập nhật lịch học vào database
        if (Database::updateClassSchedule(maLop, ngayHoc, gioBatDau, gioKetThuc)) {
            QMessageBox::information(&dialog, "Thành công", "Đã cập nhật lịch học thành công!");
            refreshClassGrid(gridLayout, parent); // Cập nhật lại Grid lớp học
            triggerReportRefresh(parent);         // Làm mới báo cáo vắng/trễ
            dialog.accept();
        } else {
            QMessageBox::critical(&dialog, "Lỗi", "Không thể lưu lịch học mới vào SQLite.");
        }
    });

    dialog.exec();
}

// -----------------------------------------------------------------------
// showExportPdfDialog: Hộp thoại cấu hình xuất báo cáo PDF thông minh
// -----------------------------------------------------------------------
void showExportPdfDialog(QWidget *parent)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Xuất Báo Cáo Điểm Danh PDF");
    dialog.setFixedSize(420, 280);
    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #1E293B;
            border: 1px solid #334155;
        }
        QLabel {
            font-size: 12px;
            color: #E2E8F0;
            font-weight: bold;
        }
        QComboBox, QLineEdit {
            padding: 6px;
            border: 1px solid #475569;
            border-radius: 6px;
            font-size: 12px;
            background-color: #0F172A;
            color: #FFFFFF;
        }
        QComboBox:focus, QLineEdit:focus {
            border: 1px solid #10B981;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(16);
    layout->setContentsMargins(20, 20, 20, 20);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    QComboBox *comboClass = new QComboBox(&dialog);
    QComboBox *comboDate = new QComboBox(&dialog);
    QLineEdit *inputFilename = new QLineEdit(&dialog);
    inputFilename->setPlaceholderText("Ví dụ: bao_cao_diem_danh");

    // Load danh sách lớp học vào ComboBox
    auto allClasses = Database::loadAllClasses();
    for (const auto &c : allClasses) {
        comboClass->addItem(QString("%1 - %2").arg(c.maLop, c.tenLop), c.maLop);
    }

    // Hàm tự động tải các ngày thực tế có dữ liệu điểm danh của lớp đó
    auto updateDatesForClass = [&](const QString &maLop) {
        comboDate->clear();
        QSqlQuery query;
        // Lấy danh sách ngày có dữ liệu điểm danh thực tế
        query.prepare(R"(
            SELECT DISTINCT date(CheckInTime) as DateVal 
            FROM AttendanceHistory 
            WHERE MSSV IN (SELECT MSSV FROM Students WHERE MaLop = :maLop) 
            ORDER BY DateVal DESC
        )");
        query.bindValue(":maLop", maLop);
        
        if (query.exec()) {
            while (query.next()) {
                QString dStr = query.value("DateVal").toString();
                // Format ngày hiển thị dd/MM/yyyy
                QDate date = QDate::fromString(dStr, "yyyy-MM-dd");
                if (date.isValid()) {
                    comboDate->addItem(date.toString("dd/MM/yyyy"), dStr);
                } else {
                    comboDate->addItem(dStr, dStr);
                }
            }
        }
        
        if (comboDate->count() == 0) {
            comboDate->addItem("Chưa có ngày điểm danh nào", "");
        }
    };

    // Lắng nghe sự kiện đổi lớp học để tự động load lại danh sách ngày học thực tế
    QObject::connect(comboClass, &QComboBox::currentIndexChanged, [&]() {
        QString maLop = comboClass->currentData().toString();
        updateDatesForClass(maLop);
        inputFilename->setText(QString("BaoCao_%1_%2").arg(maLop, QDate::currentDate().toString("ddMMyyyy")));
    });

    // Load ngày học ban đầu cho lớp học đầu tiên
    if (comboClass->count() > 0) {
        updateDatesForClass(comboClass->currentData().toString());
        inputFilename->setText(QString("BaoCao_%1_%2").arg(comboClass->currentData().toString(), QDate::currentDate().toString("ddMMyyyy")));
    }

    formLayout->addRow("Chọn lớp học:", comboClass);
    formLayout->addRow("Chọn ngày học:", comboDate);
    formLayout->addRow("Tên file gợi ý:", inputFilename);
    layout->addLayout(formLayout);

    // Hàng nút hành động
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *btnCancel = new QPushButton("Hủy", &dialog);
    btnCancel->setStyleSheet(R"(
        QPushButton {
            background-color: #334155;
            color: #94A3B8;
            padding: 6px 16px;
            border-radius: 6px;
            border: none;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #475569;
        }
    )");

    QPushButton *btnExport = new QPushButton("Xuất PDF", &dialog);
    btnExport->setStyleSheet(R"(
        QPushButton {
            background-color: #10B981;
            color: white;
            padding: 6px 16px;
            border-radius: 6px;
            border: none;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #059669;
        }
    )");

    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnExport);
    layout->addLayout(btnLayout);

    QObject::connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Sự kiện click nút Xuất
    QObject::connect(btnExport, &QPushButton::clicked, [&]() {
        QString maLop = comboClass->currentData().toString();
        QString dateVal = comboDate->currentData().toString(); // Định dạng YYYY-MM-DD
        QString fn = inputFilename->text().trimmed();

        if (maLop.isEmpty()) {
            QMessageBox::warning(&dialog, "Thông báo", "Vui lòng chọn lớp học!");
            return;
        }
        if (dateVal.isEmpty()) {
            QMessageBox::warning(&dialog, "Thông báo", "Không có dữ liệu điểm danh thực tế để xuất!");
            return;
        }
        if (fn.isEmpty()) fn = "bao_cao_diem_danh";

        // Mở hộp thoại lưu tệp tin
        QString defaultPath = QDir::toNativeSeparators(QDir::homePath() + "/" + fn + ".pdf");
        QString savePath = QFileDialog::getSaveFileName(&dialog, "Lưu báo cáo PDF", defaultPath, "Tệp PDF (*.pdf)");

        if (savePath.isEmpty()) return; // Người dùng hủy chọn

        // Tiến hành xuất PDF
        QString outErr = "";
        if (Exporter::exportAttendanceToPdf(savePath, maLop, dateVal, outErr)) {
            QMessageBox::information(&dialog, "Thành công", "Đã xuất báo cáo PDF thành công!");
            dialog.accept();
        } else {
            QMessageBox::critical(&dialog, "Lỗi", "Không thể xuất file PDF:\n" + outErr);
        }
    });

    dialog.exec();
}

