#ifndef CAMERA_H
#define CAMERA_H

#include <QImage>
#include <QString>
#include <functional>

// ============================================================================
// HƯỚNG DẪN HOẠT ĐỘNG MODULE: CAMERA (Webcam Capture Procedural Version)
// ============================================================================
// 1. CHỨC NĂNG CHÍNH:
//    - Quản lý Webcam và thu nhận các khung hình (frames) video thời gian thực từ phần cứng.
//    - Cung cấp cơ chế xử lý lỗi khi webcam đột ngột ngắt kết nối hoặc hỏng hóc.
//
// 2. PHƯƠNG THỨC HOẠT ĐỘNG & KIẾN TRÚC PHI OOP (PROCEDURAL):
//    - Module này hoàn toàn không khai báo bất kỳ Class hay cơ chế Kế thừa QObject nào,
//      thay vào đó nó sử dụng cơ chế con trỏ hàm hiện đại của C++11 (std::function).
//    - Thay vì phát tín hiệu Signal của Qt (chỉ hoạt động trong Class), module này định nghĩa
//      các kiểu Callback:
//      * FrameCallback: Được gọi sau mỗi 33.3ms (tương ứng 30 FPS) khi chụp được frame mới, 
//        gửi dữ liệu QImage đến nơi đăng ký xử lý.
//      * ErrorCallback: Được gọi khi camera bị mất tín hiệu hoặc gặp lỗi phần cứng.
//    - Cụ thể trong hàm main(), ta truyền các biểu thức Lambda vào hàm openCamera để làm nhiệm vụ
//      bắt frame và truyền qua module nhận dạng Face.
// ============================================================================
namespace Camera {

    // Định nghĩa các kiểu callback cho luồng video và thông báo lỗi
    using FrameCallback = std::function<void(const QImage&)>;
    using ErrorCallback = std::function<void(const QString&)>;

    // Mở camera với chỉ số cameraIndex, tốc độ FPS và các callback nhận kết quả
    bool openCamera(int cameraIndex = 0,
                    int fps = 30,
                    FrameCallback onFrameCaptured = nullptr,
                    ErrorCallback onError = nullptr);

    // Đóng camera và giải phóng tài nguyên
    void closeCamera();

    // Kiểm tra camera có đang mở hay không
    bool isOpened();

    // Lấy FPS hiện tại
    int currentFps();

} // namespace Camera

#endif // CAMERA_H
