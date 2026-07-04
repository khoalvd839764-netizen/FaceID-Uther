#include "Camera.h"
#include <opencv2/videoio.hpp>   // OpenCV: đọc video / camera
#include <opencv2/imgproc.hpp>   // OpenCV: xử lý ảnh (cvtColor, ...)
#include <QTimer>                // Qt: đồng hồ hẹn giờ để kích hoạt grabFrame theo FPS
#include <QDebug>                // Qt: xuất log ra console

// ============================================================================
// CHI TIẾT TRIỂN KHAI MODULE: CAMERA (Webcam Capture Procedural Version)
// ============================================================================
// - Module này chứa các biến tĩnh cục bộ đại diện cho trạng thái camera hoạt động:
//   * s_capture: Con trỏ trỏ tới cv::VideoCapture của OpenCV để giao tiếp phần cứng.
//   * s_timer: Bộ đếm thời gian QTimer của Qt chạy trên Main Thread để hẹn giờ 30 FPS.
//   * s_onFrameCaptured & s_onError: Các biến std::function lưu trữ con trỏ hàm callback
//     truyền xuống từ giao diện chính.
// - Để tránh lỗi màn hình xanh hoặc trễ trên Windows, hàm openCamera thử mở camera 
//   theo 3 backend khác nhau: DirectShow (CAP_DSHOW - tốt nhất cho Webcam USB),
//   Media Foundation (CAP_MSMF - tốt cho camera tích hợp), và CAP_ANY (tự động dò).
// - Hàm nội bộ grabFrame() trích xuất ảnh ma trận cv::Mat, kiểm tra lỗi, chuyển đổi hệ màu
//   BGR (mặc định OpenCV) -> RGB (mặc định Qt hiển thị), rồi thực hiện sao chép bộ nhớ
//   sâu (.copy()) để tạo vùng nhớ an toàn truyền về giao diện thông qua s_onFrameCaptured.
// ============================================================================
namespace Camera {

    // -----------------------------------------------------------------------
    // BIẾN TĨNH NỘI BỘ - Các biến này chỉ tồn tại trong phạm vi file Camera.cpp
    // Sử dụng 'static' để đảm bảo tính đóng gói mà không cần Class
    // -----------------------------------------------------------------------

    // Con trỏ tới đối tượng capture của OpenCV, quản lý phần cứng camera
    static cv::VideoCapture *s_capture = nullptr;

    // Bộ đếm thời gian Qt - mỗi tick sẽ gọi grabFrame() để lấy 1 khung hình
    static QTimer *s_timer = nullptr;

    // FPS (frames per second) - tốc độ chụp khung hình, mặc định 30 FPS
    static int s_fps = 30;

    // Callback được gọi mỗi khi có frame mới, truyền QImage lên UI
    static FrameCallback s_onFrameCaptured = nullptr;

    // Callback được gọi khi camera gặp lỗi (ngắt kết nối, hỏng thiết bị...)
    static ErrorCallback s_onError = nullptr;

    // Khai báo trước hàm grabFrame nội bộ (định nghĩa ở cuối namespace)
    static void grabFrame();

    // -----------------------------------------------------------------------
    // openCamera: Mở kết nối đến thiết bị camera và khởi động luồng đọc frame
    // Tham số:
    //   cameraIndex: Chỉ số camera (0 = camera mặc định, 1 = camera thứ 2, ...)
    //   fps:         Tốc độ khung hình mong muốn (frames per second)
    //   onFrameCaptured: Hàm callback nhận QImage khi có frame mới
    //   onError:     Hàm callback nhận thông báo lỗi khi camera bị ngắt
    // Trả về: true nếu mở thành công, false nếu thất bại
    // -----------------------------------------------------------------------
    bool openCamera(int cameraIndex, int fps, FrameCallback onFrameCaptured, ErrorCallback onError)
    {
        // Đóng camera cũ nếu đang mở để tránh xung đột tài nguyên
        closeCamera();

        // Khởi tạo đối tượng capture mới
        s_capture = new cv::VideoCapture();
        s_fps = fps;
        s_onFrameCaptured = onFrameCaptured; // Lưu callback frame
        s_onError = onError;                 // Lưu callback lỗi

        bool opened = false;

        // ============================================================
        // Thử lần lượt các backend camera trên Windows:
        // CAP_DSHOW (DirectShow): Tốt nhất cho USB Webcam bên ngoài
        // CAP_MSMF (Media Foundation): Tốt cho camera laptop tích hợp
        // CAP_ANY: Để OpenCV tự chọn backend phù hợp
        // ============================================================

        // Ưu tiên DirectShow trước (tốt nhất cho Windows + USB Webcam)
        if (s_capture->open(cameraIndex, cv::CAP_DSHOW)) {
            opened = true;
            qDebug() << "[CAM] Đã mở camera qua CAP_DSHOW.";
        }
        // Thử Media Foundation nếu DirectShow thất bại
        else if (s_capture->open(cameraIndex, cv::CAP_MSMF)) {
            opened = true;
            qDebug() << "[CAM] Đã mở camera qua CAP_MSMF.";
        }
        // Cuối cùng thử tự động dò backend
        else if (s_capture->open(cameraIndex, cv::CAP_ANY)) {
            opened = true;
            qDebug() << "[CAM] Đã mở camera qua CAP_ANY.";
        }

        // Nếu cả 3 backend đều thất bại, trả về lỗi
        if (!opened) {
            qCritical() << "[CAM] Không thể mở camera index:" << cameraIndex;
            if (s_onError) {
                // Thông báo lỗi lên UI thông qua callback
                s_onError("Không thể kết nối tới Webcam của bạn. Vui lòng kiểm tra lại thiết bị.");
            }
            // Giải phóng bộ nhớ và đặt lại con trỏ về null
            delete s_capture;
            s_capture = nullptr;
            return false;
        }

        // ============================================================
        // Thiết lập các thông số camera:
        // - Độ phân giải 640x480 (chuẩn VGA) - đủ tốt cho nhận diện mặt
        // - FPS theo yêu cầu (thường là 30 FPS)
        // ============================================================
        s_capture->set(cv::CAP_PROP_FRAME_WIDTH, 640);   // Chiều rộng frame (pixel)
        s_capture->set(cv::CAP_PROP_FRAME_HEIGHT, 480);  // Chiều cao frame (pixel)
        s_capture->set(cv::CAP_PROP_FPS, s_fps);         // Tốc độ chụp khung hình

        // ============================================================
        // Khởi tạo QTimer nếu chưa tồn tại.
        // QTimer chạy trên Main Thread (UI Thread) của Qt, giúp tránh
        // các vấn đề thread-safety khi cập nhật giao diện.
        // Mỗi tick (timeout) sẽ gọi grabFrame() để đọc 1 frame từ camera.
        // ============================================================
        if (!s_timer) {
            s_timer = new QTimer();
            // Kết nối tín hiệu timeout của QTimer với lambda gọi grabFrame
            // Lambda này được thực thi trên Main Thread mỗi khi hết thời gian hẹn
            QObject::connect(s_timer, &QTimer::timeout, []() {
                grabFrame();
            });
        }
        
        // Kích hoạt timer với chu kỳ = 1000ms / fps
        // Ví dụ: 30 FPS -> timer tick mỗi 33ms (≈ 33.33ms)
        s_timer->start(1000 / s_fps);
        qDebug() << "[CAM] Bắt đầu capturing ở tốc độ" << s_fps << "FPS.";
        return true;
    }

    // -----------------------------------------------------------------------
    // closeCamera: Dừng luồng đọc frame, đóng camera và giải phóng tài nguyên
    // Được gọi khi người dùng nhấn "Dừng quét" hoặc khi thoát ứng dụng
    // -----------------------------------------------------------------------
    void closeCamera()
    {
        // Dừng timer nếu đang chạy để không còn gọi grabFrame nữa
        if (s_timer && s_timer->isActive()) {
            s_timer->stop();
        }

        // Đóng camera và giải phóng bộ nhớ đối tượng VideoCapture
        if (s_capture) {
            if (s_capture->isOpened()) {
                s_capture->release(); // Trả lại quyền kiểm soát phần cứng camera
            }
            delete s_capture;   // Giải phóng bộ nhớ heap
            s_capture = nullptr; // Đặt lại về null để tránh dangling pointer
        }
        
        // Xóa các callback để tránh gọi hàm không hợp lệ sau khi đóng
        s_onFrameCaptured = nullptr;
        s_onError = nullptr;
        qDebug() << "[CAM] Đã tắt camera và giải phóng tài nguyên.";
    }

    // -----------------------------------------------------------------------
    // isOpened: Kiểm tra camera hiện tại có đang hoạt động không
    // Trả về: true nếu camera đang mở và sẵn sàng đọc frame
    // -----------------------------------------------------------------------
    bool isOpened()
    {
        // Kiểm tra cả hai điều kiện: con trỏ không null VÀ thiết bị đang mở
        return s_capture && s_capture->isOpened();
    }

    // -----------------------------------------------------------------------
    // currentFps: Lấy tốc độ FPS hiện tại của camera
    // -----------------------------------------------------------------------
    int currentFps()
    {
        return s_fps;
    }

    // -----------------------------------------------------------------------
    // grabFrame: [HÀM NỘI BỘ - PRIVATE]
    // Được gọi định kỳ bởi QTimer để đọc 1 khung hình từ camera.
    // Quy trình xử lý:
    //   1. Đọc frame BGR từ OpenCV (cv::Mat)
    //   2. Kiểm tra lỗi / mất kết nối camera
    //   3. Chuyển đổi màu sắc BGR -> RGB (Qt dùng RGB, OpenCV dùng BGR)
    //   4. Đóng gói thành QImage và sao chép sâu (deep copy) vào vùng nhớ an toàn
    //   5. Gọi callback s_onFrameCaptured để gửi frame lên UI xử lý AI
    // -----------------------------------------------------------------------
    static void grabFrame()
    {
        // Bảo vệ: Không làm gì nếu camera chưa mở hoặc đã ngắt
        if (!s_capture || !s_capture->isOpened()) return;

        cv::Mat frame; // Ma trận chứa dữ liệu pixel của frame (BGR, 8-bit mỗi kênh)

        // Đọc 1 frame từ camera vào biến frame
        // Nếu đọc thất bại (camera bị ngắt đột ngột), gọi callback lỗi
        if (!s_capture->read(frame)) {
            qWarning() << "[CAM] Mất kết nối luồng hình ảnh từ Camera!";
            if (s_onError) {
                // Thông báo lỗi lên UI (ví dụ: hiển thị "LỖI CAMERA")
                s_onError("Thiết bị Webcam đã bị ngắt kết nối đột ngột.");
            }
            closeCamera(); // Dọn dẹp tài nguyên camera
            return;
        }

        // Kiểm tra thêm trường hợp frame trống (có thể xảy ra với một số driver)
        if (frame.empty()) return;

        // ============================================================
        // CHUYỂN ĐỔI MÀU SẮC: BGR -> RGB
        // OpenCV lưu ảnh theo thứ tự kênh màu Blue-Green-Red (BGR)
        // Qt hiển thị ảnh theo thứ tự kênh màu Red-Green-Blue (RGB)
        // Nếu không chuyển đổi, ảnh sẽ bị đổi màu (mặt đỏ thành xanh lam)
        // ============================================================
        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

        // ============================================================
        // ĐÓNG GÓI THÀNH QImage:
        // QImage được tạo từ con trỏ data của rgbFrame (không sao chép dữ liệu).
        // Tham số:
        //   - rgbFrame.data: Con trỏ tới vùng nhớ pixel
        //   - rgbFrame.cols: Chiều rộng (số cột pixel)
        //   - rgbFrame.rows: Chiều cao (số hàng pixel)
        //   - rgbFrame.step: Số byte mỗi hàng (có thể lớn hơn cols*3 do padding)
        //   - QImage::Format_RGB888: 3 kênh màu, mỗi kênh 8-bit
        // ============================================================
        QImage qImg(rgbFrame.data,
                    rgbFrame.cols,
                    rgbFrame.rows,
                    static_cast<int>(rgbFrame.step),
                    QImage::Format_RGB888);

        if (s_onFrameCaptured) {
            // QUAN TRỌNG: Gọi .copy() để tạo bản sao sâu (deep copy) của QImage.
            // Lý do: rgbFrame là biến cục bộ, sẽ bị hủy khi hàm grabFrame kết thúc.
            // Nếu không copy, QImage sẽ trỏ vào vùng nhớ đã bị giải phóng (dangling pointer),
            // gây crash ứng dụng.
            s_onFrameCaptured(qImg.copy());
        }
    }

} // namespace Camera
