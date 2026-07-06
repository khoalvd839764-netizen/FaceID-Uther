#include "Exporter.h"
#include <QPrinter>
#include <QPainter>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QPixmap>
#include <QImage>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

namespace Exporter {

    bool exportAttendanceToPdf(const QString &filePath, const QString &maLop, const QString &dateStr, QString &outError)
    {
        // 1. Lấy thông tin lớp học từ SQLite
        QString tenLop = "";
        QString phongHoc = "";
        QString gioBatDau = "";
        QString gioKetThuc = "";
        int siSoDuKien = 0;

        QSqlQuery classQuery;
        classQuery.prepare("SELECT TenLop, PhongHoc, GioBatDau, GioKetThuc, SiSoDuKien FROM Classes WHERE MaLop = :maLop");
        classQuery.bindValue(":maLop", maLop);
        if (!classQuery.exec() || !classQuery.next()) {
            outError = "Không tìm thấy thông tin lớp học " + maLop;
            return false;
        }

        tenLop = classQuery.value(0).toString();
        phongHoc = classQuery.value(1).toString();
        gioBatDau = classQuery.value(2).toString();
        gioKetThuc = classQuery.value(3).toString();
        siSoDuKien = classQuery.value(4).toInt();

        // 2. Chỉ truy vấn các sinh viên đã điểm danh thành công trong ngày
        QVector<AttendanceReportRow> rows;
        QSqlQuery attendanceQuery;
        attendanceQuery.prepare(R"(
            SELECT s.MSSV, s.HoTen, s.AvatarPath, h.CheckInTime, h.CapturedImagePath
            FROM AttendanceHistory h
            JOIN Students s ON h.MSSV = s.MSSV
            WHERE s.MaLop = :maLop AND date(h.CheckInTime) = :dateStr
            ORDER BY h.CheckInTime ASC
        )");
        attendanceQuery.bindValue(":maLop", maLop);
        attendanceQuery.bindValue(":dateStr", dateStr);

        if (!attendanceQuery.exec()) {
            outError = "Lỗi truy vấn điểm danh: " + attendanceQuery.lastError().text();
            return false;
        }

        int index = 1;
        QTime sTime = QTime::fromString(gioBatDau, "HH:mm");

        while (attendanceQuery.next()) {
            QString mssv = attendanceQuery.value(0).toString();
            QString hoTen = attendanceQuery.value(1).toString();
            QString avatarPath = attendanceQuery.value(2).toString();
            QString checkInTimeStr = attendanceQuery.value(3).toString();
            QString capturedPath = attendanceQuery.value(4).toString();

            AttendanceReportRow row;
            row.stt = index++;
            row.mssv = mssv;
            row.hoTen = hoTen;
            row.maLop = maLop;
            row.tenLop = tenLop;
            row.gioLopHoc = QString("%1 - %2").arg(gioBatDau, gioKetThuc);
            row.avatarPath = avatarPath;
            row.capturedPath = capturedPath;

            QDateTime dt = QDateTime::fromString(checkInTimeStr, "yyyy-MM-dd HH:mm:ss");
            QTime checkTime = dt.time();
            row.gioCheckIn = checkTime.toString("HH:mm:ss");

            if (checkTime > sTime) {
                int lateMins = sTime.secsTo(checkTime) / 60;
                row.trangThai = QString("Trễ %1 phút").arg(lateMins);
            } else {
                row.trangThai = "Đúng giờ";
            }
            rows.append(row);
        }

        if (rows.isEmpty()) {
            outError = "Ngày được chọn không ghi nhận sinh viên nào điểm danh thành công.";
            return false;
        }

        // 3. Khởi tạo QPrinter & QPainter để thiết kế PDF
        QPrinter printer(QPrinter::ScreenResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setPageSize(QPageSize(QPageSize::A4));
        printer.setPageOrientation(QPageLayout::Landscape); // Đổi sang Landscape (khổ ngang) để hiển thị 2 cột ảnh siêu rộng rãi và đẹp mắt
        printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
        printer.setOutputFileName(filePath);

        QPainter painter;
        if (!painter.begin(&printer)) {
            outError = "Không thể ghi file báo cáo PDF. Vui lòng kiểm tra quyền ghi hoặc ứng dụng đọc PDF đang mở file này.";
            return false;
        }

        // Sử dụng bộ font chữ Arial chuẩn Unicode của Windows để chống lỗi chữ Tiếng Việt
        QFont fontTitle("Arial", 16, QFont::Bold);
        QFont fontHeader("Arial", 10, QFont::DemiBold);
        QFont fontBody("Arial", 9);
        QFont fontFooter("Arial", 8, QFont::StyleItalic);

        // Kích thước trang vẽ (Logical Coordinates)
        QRect pageRect = painter.viewport();
        int pageW = pageRect.width();
        int pageH = pageRect.height();

        // Cấu hình vẽ phân trang
        int itemsPerPage = 5; // Khổ ngang, mỗi dòng cao khoảng 130px cho vừa hình ảnh chân dung và checkin
        int totalPages = (rows.size() + itemsPerPage - 1) / itemsPerPage;

        for (int p = 0; p < totalPages; ++p) {
            if (p > 0) {
                printer.newPage(); // Ngắt sang trang mới
            }

            // Vẽ tiêu đề báo cáo
            painter.setPen(QColor("#0F172A")); // Charcoal đậm
            painter.setFont(fontTitle);
            painter.drawText(QRect(0, 40, pageW, 40), Qt::AlignCenter, "BÁO CÁO ĐIỂM DANH SINH VIÊN QUA FACEID");

            // Vẽ dải phân cách trang trí bên dưới tiêu đề
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#3B82F6")); // Blue Accent
            painter.drawRect(pageW / 2 - 200, 90, 400, 4);

            // Khối thông tin lớp học
            painter.setFont(fontHeader);
            painter.setPen(QColor("#334155"));
            int metaY = 120;
            int col1X = 50;
            int col2X = pageW / 2 + 50;

            painter.drawText(col1X, metaY,       QString("Lớp học: %1 (%2)").arg(tenLop, maLop));
            painter.drawText(col1X, metaY + 25,  QString("Phòng học: %1").arg(phongHoc));
            painter.drawText(col1X, metaY + 50,  QString("Ngày điểm danh: %1").arg(QDate::fromString(dateStr, "yyyy-MM-dd").toString("dd/MM/yyyy")));

            painter.drawText(col2X, metaY,       QString("Sĩ số lớp: %1 học viên").arg(rows.size()));
            painter.drawText(col2X, metaY + 25,  QString("Khung giờ học: %1").arg(rows[0].gioLopHoc));
            painter.drawText(col2X, metaY + 50,  QString("Hệ thống kiểm soát: FacieID UTH Dashboard"));

            // Vẽ bảng danh sách
            int tableY = 200;
            int rowHeight = 110; // Đủ cao để chứa ảnh 100x80px

            // Định nghĩa các cột (Đã loại bỏ cột Trạng Thái)
            // Tính toán độ rộng cột dựa trên chiều rộng thực tế của bảng (pageW - 60) để tránh tràn lề phải
            int tableWidth = pageW - 60;
            int colWidths[] = {
                (int)(tableWidth * 0.08),  // STT
                (int)(tableWidth * 0.18),  // Ảnh Đăng Ký
                (int)(tableWidth * 0.18),  // Ảnh Điểm Danh
                (int)(tableWidth * 0.14),  // MSSV
                (int)(tableWidth * 0.26),  // Họ tên
                (int)(tableWidth * 0.16)   // Giờ Quét
            };

            int colX[6];
            colX[0] = 30; // Margin trái
            for (int i = 1; i < 6; ++i) {
                colX[i] = colX[i-1] + colWidths[i-1];
            }

            // Vẽ Header bảng
            painter.setFont(fontHeader);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#1E293B")); // Slate dark header
            painter.drawRect(colX[0], tableY, pageW - 60, 35);

            painter.setPen(Qt::white);
            QString headers[] = {"STT", "Ảnh Đăng Ký", "Ảnh Quét Mặt", "MSSV", "Họ và Tên", "Giờ Quét"};
            for (int i = 0; i < 6; ++i) {
                painter.drawText(QRect(colX[i], tableY, colWidths[i], 35), Qt::AlignCenter, headers[i]);
            }

            // Vẽ các dòng dữ liệu
            int currentY = tableY + 35;
            int startIdx = p * itemsPerPage;
            int endIdx = std::min(startIdx + itemsPerPage, static_cast<int>(rows.size()));

            painter.setFont(fontBody);

            for (int i = startIdx; i < endIdx; ++i) {
                const auto &r = rows[i];

                // Vẽ màu nền xen kẽ (Zebra rows)
                painter.setPen(Qt::NoPen);
                if (i % 2 == 0) {
                    painter.setBrush(QColor("#F8FAFC")); // Light slate gray
                } else {
                    painter.setBrush(Qt::white);
                }
                painter.drawRect(colX[0], currentY, pageW - 60, rowHeight);

                // Vẽ viền lưới dòng
                painter.setPen(QColor("#E2E8F0"));
                painter.drawLine(colX[0], currentY + rowHeight, pageW - 30, currentY + rowHeight);

                // Cột 1: STT
                painter.setPen(QColor("#0F172A"));
                painter.drawText(QRect(colX[0], currentY, colWidths[0], rowHeight), Qt::AlignCenter, QString::number(r.stt));

                // Cột 2: Ảnh Đăng Ký (Avatar)
                int imgW = 90;
                int imgH = 90;
                int imgPadX = (colWidths[1] - imgW) / 2;
                int imgPadY = (rowHeight - imgH) / 2;
                QRect rectAvat(colX[1] + imgPadX, currentY + imgPadY, imgW, imgH);
                if (!r.avatarPath.isEmpty() && QFileInfo::exists(r.avatarPath)) {
                    QImage img(r.avatarPath);
                    if (!img.isNull()) {
                        painter.drawImage(rectAvat, img.scaled(imgW, imgH, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                    }
                } else {
                    painter.setPen(QColor("#94A3B8"));
                    painter.drawText(rectAvat, Qt::AlignCenter, "[Không ảnh]");
                }

                // Cột 3: Ảnh Quét Mặt (Captured)
                QRect rectCapt(colX[2] + imgPadX, currentY + imgPadY, imgW, imgH);
                if (!r.capturedPath.isEmpty() && QFileInfo::exists(r.capturedPath)) {
                    QImage img(r.capturedPath);
                    if (!img.isNull()) {
                        painter.drawImage(rectCapt, img.scaled(imgW, imgH, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                    }
                } else {
                    painter.setPen(QColor("#94A3B8"));
                    painter.drawText(rectCapt, Qt::AlignCenter, "[Không ảnh]");
                }

                // Cột 4: MSSV
                painter.setPen(QColor("#0F172A"));
                painter.drawText(QRect(colX[3], currentY, colWidths[3], rowHeight), Qt::AlignCenter, r.mssv);

                // Cột 5: Họ tên
                painter.drawText(QRect(colX[4] + 10, currentY, colWidths[4] - 20, rowHeight), Qt::AlignVCenter | Qt::AlignLeft, r.hoTen);

                // Cột 6: Giờ quét
                painter.drawText(QRect(colX[5], currentY, colWidths[5], rowHeight), Qt::AlignCenter, r.gioCheckIn);

                currentY += rowHeight;
            }

            // Vẽ chân trang (Footer)
            painter.setFont(fontFooter);
            painter.setPen(QColor("#64748B"));
            QString printTimeStr = QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss");
            painter.drawText(30, pageH - 40, QString("Báo cáo xuất ngày: %1 | Thiết lập bởi FacieID Dashboard UTH").arg(printTimeStr));
            painter.drawText(pageW - 100, pageH - 40, QString("Trang %1/%2").arg(p + 1).arg(totalPages));
        }

        painter.end();
        return true;
    }
}
