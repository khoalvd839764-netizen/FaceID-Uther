#ifndef EXPORTER_H
#define EXPORTER_H

#include <QString>
#include <QVector>

namespace Exporter {
    // Cấu trúc chứa dòng dữ liệu điểm danh phục vụ vẽ báo cáo PDF
    struct AttendanceReportRow {
        int stt;
        QString mssv;
        QString hoTen;
        QString maLop;
        QString tenLop;
        QString gioCheckIn;   // "hh:mm:ss" hoặc "Vắng mặt"
        QString gioLopHoc;    // "HH:mm - HH:mm"
        QString trangThai;     // "Đúng giờ", "Trễ X phút", "Vắng mặt"
        QString avatarPath;    // Đường dẫn ảnh chân dung đăng ký
        QString capturedPath;  // Đường dẫn ảnh chụp lúc quét mặt
    };

    // Xuất báo cáo điểm danh của một lớp học vào ngày chỉ định ra file PDF
    // Trả về: true nếu xuất thành công, false nếu thất bại
    // outError: Lưu chuỗi thông tin lỗi nếu thất bại
    bool exportAttendanceToPdf(const QString &filePath, const QString &maLop, const QString &dateStr, QString &outError);
}

#endif // EXPORTER_H
