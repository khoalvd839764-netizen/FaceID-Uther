#ifndef FACE_H
#define FACE_H

#include <QString>
#include <QRect>
#include <QImage>
#include <vector>
#include <functional>

// ============================================================================
// HƯỚNG DẪN HOẠT ĐỘNG MODULE: FACE (AI Face Recognition Procedural Version)
// ============================================================================
// 1. CHỨC NĂNG CHÍNH:
//    - Đóng vai trò là "bộ não" xử lý thị giác máy tính và trí tuệ nhân tạo (AI).
//    - Chịu trách nhiệm thực hiện toàn bộ pipeline nhận diện khuôn mặt:
//      * Phát hiện khuôn mặt (Face Detection - dùng mạng nơ-ron YuNet).
//      * Căn chỉnh thẳng khuôn mặt (Face Alignment - xoay thẳng đầu dựa trên 5 điểm mốc landmarks).
//      * Trích xuất vector đặc trưng (Face Embedding - dùng mạng SFace chuyển ảnh mặt thành 128 số float).
//      * So khớp khuôn mặt (Face Matching - tìm kiếm sinh viên khớp dựa trên khoảng cách Euclidean L2).
//
// 2. PHƯƠNG THỨC HOẠT ĐỘNG & KIẾN TRÚC PHI OOP (PROCEDURAL):
//    - Tương tự các module khác, module Face được gom vào một Namespace chứa các cấu trúc
//      dữ liệu như FaceResult, StudentRef và các hàm xử lý tính toán.
//    - Kết nối với UI thông qua callback `FaceRecognizedCallback` (được gọi mỗi khi nhận dạng 
//      thành công một sinh viên có trong database, UI sẽ bắt sự kiện này để ghi vào SQLite).
// ============================================================================
// Forward declaration của OpenCV Mat để tránh include nặng vào header
namespace cv {
    class Mat;
}

// ============================================================================
// Face - Namespace chứa các hàm xử lý AI nhận diện khuôn mặt (Procedural style)
// ============================================================================
namespace Face {

    // -----------------------------------------------------------------------
    // Cấu trúc dữ liệu
    // -----------------------------------------------------------------------

    // Cấu trúc kết quả nhận diện khuôn mặt
    struct FaceResult {
        QString mssv;       // MSSV sinh viên khớp (rỗng nếu không nhận dạng được)
        QString hoTen;      // Tên sinh viên
        double confidence;  // Độ tin cậy (khoảng cách Euclidean)
        QRect faceRect;     // Vị trí khuôn mặt (x, y, w, h)
        bool recognized;    // true nếu nhận diện khớp trong database
    };

    // Cấu trúc tham chiếu sinh viên lưu trên RAM để so khớp
    struct StudentRef {
        QString mssv;
        QString hoTen;
        std::vector<float> faceVector; // Vector đặc trưng 128 chiều
    };

    // Callback phát ra khi nhận diện thành công sinh viên
    using FaceRecognizedCallback = std::function<void(const FaceResult&)>;

    // -----------------------------------------------------------------------
    // Các hàm chức năng
    // -----------------------------------------------------------------------

    // Nạp các mô hình ONNX phát hiện (YuNet) và nhận diện (SFace)
    bool loadModels(const QString &detModelPath, const QString &recModelPath);

    // Kiểm tra mô hình đã được nạp thành công chưa
    bool isModelLoaded();

    // Nạp danh sách sinh viên đặc trưng lên RAM phục vụ so khớp realtime
    void setStudentDatabase(const std::vector<StudentRef> &students);

    // Xử lý 1 frame camera (BGR) -> Trả về danh sách khuôn mặt, vẽ kết quả lên outputFrame
    // Gọi callback onRecognized cho mỗi khuôn mặt nhận diện khớp
    std::vector<FaceResult> processFrame(const QImage &inputFrame,
                                         QImage &outputFrame,
                                         FaceRecognizedCallback onRecognized = nullptr);

    // Điều chỉnh ngưỡng nhận dạng (L2 distance)
    void setThreshold(double threshold);
    double threshold();

    // Các hàm tiện ích phụ trợ
    cv::Mat qImageToMat(const QImage &img);
    QImage matToQImage(const cv::Mat &mat);

    // Phát hiện khuôn mặt trong ảnh tĩnh (dùng khi đăng ký)
    std::vector<QRect> detectFaces(const cv::Mat &frame);

    // Trích xuất vector đặc trưng từ khuôn mặt đã align/crop
    std::vector<float> extractEmbedding(const cv::Mat &faceROI);

} // namespace Face

#endif // FACE_H
