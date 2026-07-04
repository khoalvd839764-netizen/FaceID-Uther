#include "Face.h"
#include <opencv2/core.hpp>        // OpenCV: cấu trúc Mat, Point, Rect...
#include <opencv2/imgproc.hpp>     // OpenCV: xử lý ảnh (resize, cvtColor, rectangle, putText...)
#include <opencv2/objdetect.hpp>   // OpenCV: FaceDetectorYN (YuNet), FaceRecognizerSF (SFace)
#include <QDebug>                  // Qt: xuất log ra console
#include <cmath>                   // std::sqrt - tính căn bậc hai cho khoảng cách Euclidean
#include <algorithm>               // std::max - lấy giá trị lớn nhất

// ============================================================================
// CHI TIẾT TRIỂN KHAI MODULE: FACE (AI Face Recognition Procedural Version)
// ============================================================================
// - Module này chứa các con trỏ mô hình tĩnh cục bộ:
//   * s_detector: Con trỏ đến cv::FaceDetectorYN (Mạng nơ-ron phát hiện khuôn mặt YuNet).
//   * s_recognizer: Con trỏ đến cv::FaceRecognizerSF (Mạng nơ-ron trích xuất đặc trưng SFace).
//   * s_studentDB: Bộ nhớ đệm RAM chứa danh sách sinh viên được nạp động từ SQLite phục vụ so khớp.
// - Các bước xử lý trong hàm processFrame():
//   1. Phát hiện: Sử dụng s_detector->detect() để tìm các vùng chứa khuôn mặt (bounding boxes)
//      và tọa độ 5 điểm mốc (landmarks: 2 mắt, mũi, 2 khóe miệng).
//   2. Align & Crop: Sử dụng s_recognizer->alignCrop() thực hiện phép biến đổi Affine xoay thẳng khuôn mặt
//      và cắt thành kích thước chuẩn 112x112 pixel.
//   3. Embedding: s_recognizer->feature() trích xuất vector 128 chiều, sau đó chuẩn hóa L2 norm 
//      để giảm thiểu sai lệch độ sáng tối của môi trường.
//   4. Đối khớp (matchFace): Duyệt tuyến tính qua s_studentDB để tính khoảng cách hình học Euclidean 
//      giữa vector hiện tại và vector của sinh viên trong database. Nếu khoảng cách nhỏ nhất dưới ngưỡng
//      s_threshold (mặc định 1.12), nhận dạng thành công.
// ============================================================================
namespace Face {

    // -----------------------------------------------------------------------
    // BIẾN TĨNH NỘI BỘ CỦA MODULE FACE
    // Sử dụng cv::Ptr (smart pointer của OpenCV) để tự động quản lý bộ nhớ
    // -----------------------------------------------------------------------

    // Mô hình phát hiện khuôn mặt YuNet (YuNet là mạng nhỏ gọn, nhanh)
    // File ONNX: face_detection_yunet_2023mar.onnx
    static cv::Ptr<cv::FaceDetectorYN> s_detector = nullptr;

    // Mô hình trích xuất đặc trưng SFace (SFace = Spherical Face)
    // File ONNX: face_recognition_sface_2021dec.onnx
    static cv::Ptr<cv::FaceRecognizerSF> s_recognizer = nullptr;

    // Cờ kiểm tra trạng thái: mô hình đã được nạp thành công chưa
    static bool s_modelLoaded = false;

    // Ngưỡng khoảng cách Euclidean L2 để quyết định "nhận diện khớp hay không"
    // Giá trị nhỏ hơn = nghiêm ngặt hơn (ít false positive)
    // Giá trị lớn hơn = dễ dãi hơn (nhiều false positive)
    // Mặc định 1.12 là ngưỡng được khuyến nghị bởi OpenCV cho SFace
    static double s_threshold = 1.12;

    // Danh sách sinh viên được nạp lên RAM để so khớp realtime
    // (tránh truy vấn SQLite liên tục trong mỗi frame -> hiệu năng cao hơn)
    static std::vector<StudentRef> s_studentDB;

    // Khai báo trước các hàm phụ trợ nội bộ (private trong namespace)
    static double euclideanDistance(const std::vector<float> &a, const std::vector<float> &b);
    static FaceResult matchFace(const std::vector<float> &embedding, const QRect &faceRect);

    // -----------------------------------------------------------------------
    // loadModels: Nạp 2 mô hình ONNX vào bộ nhớ GPU/CPU
    // Phải gọi hàm này một lần trước khi sử dụng bất kỳ chức năng nhận diện nào.
    // Tham số:
    //   detModelPath: Đường dẫn tới file YuNet ONNX (phát hiện mặt)
    //   recModelPath: Đường dẫn tới file SFace ONNX (trích xuất đặc trưng)
    // Trả về: true nếu nạp thành công cả 2 mô hình
    // -----------------------------------------------------------------------
    bool loadModels(const QString &detModelPath, const QString &recModelPath)
    {
        try {
            // Chuyển đổi từ QString (Qt) sang std::string (C++ chuẩn) để OpenCV sử dụng
            std::string detPath = detModelPath.toStdString();
            std::string recPath = recModelPath.toStdString();

            // ================================================================
            // KHỞI TẠO MÔ HÌNH PHÁT HIỆN MẶT: YuNet
            // YuNet là mạng phát hiện mặt nhỏ gọn được tối ưu cho realtime.
            // Tham số quan trọng:
            //   - cv::Size(320, 320): Kích thước đầu vào mô hình (sẽ resize frame về size này)
            //   - 0.6f (scoreThreshold): Ngưỡng tin cậy phát hiện (loại bỏ detection dưới 60%)
            //   - 0.3f (nmsThreshold): Non-Maximum Suppression - loại bỏ hộp trùng lặp
            //   - 5000 (topK): Giữ tối đa 5000 khuôn mặt trước khi lọc NMS
            // ================================================================
            s_detector = cv::FaceDetectorYN::create(
                detPath,
                "",            // Config file (để trống vì ONNX tự chứa config)
                cv::Size(320, 320), // Input size của mô hình YuNet
                0.6f,          // scoreThreshold: Loại bỏ detection có độ tin cậy < 60%
                0.3f,          // nmsThreshold: Ngưỡng NMS để loại hộp trùng nhau
                5000           // topK: Tối đa 5000 face candidates trước NMS
            );

            // Kiểm tra nếu nạp thất bại (file không tồn tại hoặc bị hỏng)
            if (s_detector.empty()) {
                qCritical() << "[AI] Không thể nạp YuNet từ:" << detModelPath;
                s_modelLoaded = false;
                return false;
            }

            // ================================================================
            // KHỞI TẠO MÔ HÌNH TRÍCH XUẤT ĐẶC TRƯNG: SFace
            // SFace (Spherical Face) là mô hình nhận diện mặt dựa trên
            // không gian hình học spherical margin loss, độ chính xác cao
            // và có kích thước nhỏ (phù hợp cho ứng dụng nhúng/desktop)
            // ================================================================
            s_recognizer = cv::FaceRecognizerSF::create(
                recPath,
                ""             // Config file (để trống)
            );

            // Kiểm tra nếu nạp thất bại
            if (s_recognizer.empty()) {
                qCritical() << "[AI] Không thể nạp SFace từ:" << recModelPath;
                s_modelLoaded = false;
                return false;
            }

            // Đánh dấu nạp thành công
            s_modelLoaded = true;
            qDebug() << "[AI] Nạp thành công YuNet & SFace.";
            return true;

        } catch (const cv::Exception &e) {
            // Bắt lỗi từ OpenCV (ví dụ: file ONNX không đúng định dạng)
            qCritical() << "[AI] Lỗi từ OpenCV DNN khi nạp mô hình:" << e.what();
            s_modelLoaded = false;
            return false;
        } catch (const std::exception &e) {
            // Bắt các lỗi C++ chuẩn khác
            qCritical() << "[AI] Lỗi C++ khi nạp mô hình:" << e.what();
            s_modelLoaded = false;
            return false;
        }
    }

    // -----------------------------------------------------------------------
    // isModelLoaded: Kiểm tra trạng thái nạp mô hình
    // -----------------------------------------------------------------------
    bool isModelLoaded()
    {
        return s_modelLoaded;
    }

    // -----------------------------------------------------------------------
    // setStudentDatabase: Nạp danh sách sinh viên vào bộ nhớ RAM để so khớp realtime
    // Được gọi mỗi khi: bắt đầu camera, thêm sinh viên mới, hoặc chuyển lớp học
    // Tham số:
    //   students: Danh sách các StudentRef chứa MSSV, tên và faceVector 128 chiều
    // -----------------------------------------------------------------------
    void setStudentDatabase(const std::vector<StudentRef> &students)
    {
        // Sao chép toàn bộ danh sách vào bộ nhớ đệm RAM nội bộ
        s_studentDB = students;
        qDebug() << "[AI] Đã nạp" << s_studentDB.size() << "sinh viên lên RAM để đối khớp.";
    }

    // -----------------------------------------------------------------------
    // setThreshold / threshold: Getter/Setter cho ngưỡng nhận diện
    // -----------------------------------------------------------------------
    void setThreshold(double threshold)
    {
        s_threshold = threshold;
    }

    double threshold()
    {
        return s_threshold;
    }

    // -----------------------------------------------------------------------
    // processFrame: XỬ LÝ 1 FRAME CAMERA - HÀM TRUNG TÂM CỦA MODULE AI
    // Pipeline đầy đủ: Input Frame -> Detect -> Align -> Extract -> Match -> Draw -> Output
    //
    // Tham số:
    //   inputFrame:   Frame gốc từ camera (QImage định dạng RGB)
    //   outputFrame:  Frame đã được vẽ bounding box và nhãn kết quả (QImage)
    //   onRecognized: Callback được gọi mỗi khi nhận dạng khớp 1 sinh viên
    //
    // Trả về: Danh sách FaceResult cho tất cả khuôn mặt phát hiện được trong frame
    // -----------------------------------------------------------------------
    std::vector<FaceResult> processFrame(const QImage &inputFrame,
                                         QImage &outputFrame,
                                         FaceRecognizedCallback onRecognized)
    {
        std::vector<FaceResult> results; // Danh sách kết quả sẽ trả về

        // Bảo vệ: Nếu mô hình chưa nạp, trả về frame gốc không xử lý
        if (!s_modelLoaded || !s_detector || !s_recognizer) {
            outputFrame = inputFrame;
            return results;
        }

        // Bước 1: Chuyển QImage (RGB) -> cv::Mat (BGR) để OpenCV xử lý
        cv::Mat frame = qImageToMat(inputFrame);
        if (frame.empty()) {
            outputFrame = inputFrame;
            return results;
        }

        try {
            // ============================================================
            // BƯỚC 2: PHÁT HIỆN KHUÔN MẶT (Face Detection với YuNet)
            // - Đặt kích thước input = kích thước frame hiện tại (quan trọng!)
            //   (YuNet cần biết trước kích thước để tính toán stride đúng)
            // - Kết quả facesMat là ma trận Nx15 (N khuôn mặt, mỗi hàng = 15 giá trị):
            //   Cột 0-3: Tọa độ bounding box (x, y, w, h)
            //   Cột 4-13: 5 điểm mốc landmarks (x1,y1, x2,y2, x3,y3, x4,y4, x5,y5)
            //   Cột 14: Score độ tin cậy phát hiện (0.0 - 1.0)
            // ============================================================
            s_detector->setInputSize(cv::Size(frame.cols, frame.rows));
            cv::Mat facesMat; // Ma trận kết quả phát hiện mặt
            s_detector->detect(frame, facesMat);

            // Duyệt qua từng khuôn mặt phát hiện được
            for (int i = 0; i < facesMat.rows; ++i) {
                // Trích xuất tọa độ bounding box từ kết quả phát hiện
                int x = static_cast<int>(facesMat.at<float>(i, 0)); // Tọa độ góc trái
                int y = static_cast<int>(facesMat.at<float>(i, 1)); // Tọa độ góc trên
                int w = static_cast<int>(facesMat.at<float>(i, 2)); // Chiều rộng vùng mặt
                int h = static_cast<int>(facesMat.at<float>(i, 3)); // Chiều cao vùng mặt

                // Đóng gói thành QRect để sử dụng trong FaceResult
                QRect faceRect(x, y, w, h);

                // ============================================================
                // BƯỚC 3: CĂN CHỈNH & CẮT KHUÔN MẶT (Face Alignment)
                // alignCrop thực hiện phép biến đổi Affine dựa trên 5 landmarks:
                //   - Xoay thẳng khuôn mặt (sửa nghiêng đầu)
                //   - Cắt và resize về kích thước chuẩn 112x112 pixel
                // Kết quả: alignedFace là ảnh khuôn mặt đã được chuẩn hóa hình học
                // ============================================================
                cv::Mat alignedFace;
                s_recognizer->alignCrop(frame, facesMat.row(i), alignedFace);

                // ============================================================
                // BƯỚC 4: TRÍCH XUẤT VECTOR ĐẶC TRƯNG (Feature Extraction)
                // SFace chuyển ảnh khuôn mặt 112x112 thành vector 128 chiều.
                // Mỗi chiều đại diện cho 1 đặc điểm khuôn mặt học được từ training data.
                // Vector này là "dấu vân tay số" của khuôn mặt.
                // ============================================================
                cv::Mat feature; // Ma trận 1x128 float
                s_recognizer->feature(alignedFace, feature);

                // Sao chép dữ liệu từ cv::Mat sang std::vector<float>
                std::vector<float> embedding(feature.cols); // Vector 128 chiều
                const float* dataPtr = feature.ptr<float>(); // Con trỏ tới data
                if (dataPtr) {
                    for (int k = 0; k < feature.cols; ++k) {
                        embedding[k] = dataPtr[k];
                    }
                }

                // ============================================================
                // BƯỚC 5: CHUẨN HÓA L2 NORM (L2 Normalization)
                // Công thức: v_normalized = v / ||v||₂
                // Mục đích: Đưa tất cả vector về độ dài đơn vị = 1.
                // Lợi ích: Khoảng cách Euclidean giữa các vector chuẩn hóa
                //          không bị ảnh hưởng bởi độ sáng, tương phản ảnh đầu vào.
                // ============================================================
                float norm = 0.0f;
                // Bước 5a: Tính bình phương L2 norm (tổng bình phương các phần tử)
                for (float v : embedding) norm += v * v;
                // Bước 5b: Lấy căn bậc hai để được L2 norm thực sự
                norm = std::sqrt(norm);
                // Bước 5c: Chia mỗi phần tử cho norm để chuẩn hóa
                if (norm > 0.0f) {
                    for (float &v : embedding) v /= norm;
                }

                // ============================================================
                // BƯỚC 6: ĐỐI KHỚP SINH VIÊN (Face Matching)
                // So sánh embedding vừa trích xuất với tất cả embedding trong s_studentDB
                // Tìm sinh viên có khoảng cách Euclidean nhỏ nhất dưới ngưỡng s_threshold
                // ============================================================
                FaceResult result = matchFace(embedding, faceRect);
                results.push_back(result); // Lưu kết quả vào danh sách

                // ============================================================
                // BƯỚC 7: VẼ KẾT QUẢ LÊN FRAME (Visualization)
                // - Xanh lá (0, 255, 0): Nhận diện thành công - sinh viên đã đăng ký
                // - Đỏ (0, 0, 255): Không nhận diện được - "Unknown"
                // LƯU Ý: OpenCV dùng thứ tự màu BGR, không phải RGB!
                // ============================================================
                cv::Scalar color = result.recognized
                    ? cv::Scalar(0, 255, 0)   // Màu xanh lá (BGR) = nhận diện khớp
                    : cv::Scalar(0, 0, 255);  // Màu đỏ (BGR) = không nhận diện được

                // Vẽ hộp bounding box quanh khuôn mặt
                cv::Rect cvRect(x, y, w, h);
                // Giới hạn rect trong biên frame để tránh vẽ ra ngoài ảnh
                cvRect &= cv::Rect(0, 0, frame.cols, frame.rows);
                cv::rectangle(frame, cvRect, color, 2); // Độ dày đường viền = 2px

                // Chuẩn bị nhãn văn bản (tên sinh viên hoặc "Unknown")
                std::string label = result.recognized
                    ? result.hoTen.toStdString() // Tên sinh viên nhận diện được
                    : "Unknown";                 // Không khớp với ai trong database

                // Tính kích thước hộp nền cho văn bản (để dễ đọc hơn)
                int baseline = 0;
                cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.6, 1, &baseline);
                cv::Point textOrg(cvRect.x, cvRect.y - 6); // Đặt văn bản phía trên bounding box

                // Vẽ hộp nền màu solid phía sau văn bản để tăng độ tương phản
                cv::rectangle(frame,
                              cv::Point(textOrg.x, textOrg.y - textSize.height - 4),
                              cv::Point(textOrg.x + textSize.width + 4, textOrg.y + 4),
                              color, cv::FILLED); // cv::FILLED = tô đặc

                // Vẽ văn bản tên lên frame (màu trắng để nổi bật trên nền)
                cv::putText(frame, label, textOrg,
                            cv::FONT_HERSHEY_SIMPLEX, 0.6,
                            cv::Scalar(255, 255, 255), 1); // Màu trắng, độ dày 1

                // ============================================================
                // BƯỚC 8: GỌI CALLBACK NẾU NHẬN DIỆN THÀNH CÔNG
                // Callback này sẽ kích hoạt ghi điểm danh vào SQLite ở tầng UI (main.cpp)
                // ============================================================
                if (result.recognized && onRecognized) {
                    onRecognized(result);
                }
            }
        } catch (const cv::Exception &e) {
            // Bắt lỗi OpenCV (ví dụ: frame bị hỏng định dạng)
            qWarning() << "[AI] Lỗi OpenCV dnn:" << e.what();
        } catch (const std::exception &e) {
            // Bắt các lỗi C++ chuẩn khác
            qWarning() << "[AI] Lỗi C++:" << e.what();
        }

        // Chuyển cv::Mat (BGR) về QImage (RGB) để hiển thị trên giao diện Qt
        outputFrame = matToQImage(frame);
        return results;
    }

    // -----------------------------------------------------------------------
    // qImageToMat: Chuyển đổi QImage (Qt) sang cv::Mat (OpenCV)
    // Bước này cần thiết vì QImage dùng RGB, nhưng OpenCV xử lý bằng BGR.
    //
    // Quy trình:
    //   1. Chuyển QImage sang định dạng RGB888 (3 kênh, 8-bit/kênh)
    //   2. Tạo cv::Mat từ dữ liệu pixel của QImage (KHÔNG sao chép)
    //   3. Clone (deep copy) để có bộ nhớ độc lập
    //   4. Chuyển màu RGB -> BGR để OpenCV xử lý đúng
    // -----------------------------------------------------------------------
    cv::Mat qImageToMat(const QImage &img)
    {
        // Đảm bảo QImage ở định dạng RGB888 (3 bytes/pixel)
        QImage converted = img.convertToFormat(QImage::Format_RGB888);

        // Tạo cv::Mat tham chiếu vào bộ nhớ của QImage (không sao chép data)
        // CV_8UC3 = 8-bit unsigned, 3 channels (3 kênh màu)
        cv::Mat mat(converted.height(), converted.width(), CV_8UC3,
                    const_cast<uchar*>(converted.bits()),
                    static_cast<size_t>(converted.bytesPerLine()));

        // Clone để tạo bản sao độc lập (QImage có thể bị hủy sau khi hàm kết thúc)
        cv::Mat result = mat.clone();

        // Chuyển đổi từ RGB (Qt) sang BGR (OpenCV)
        cv::cvtColor(result, result, cv::COLOR_RGB2BGR);
        return result;
    }

    // -----------------------------------------------------------------------
    // matToQImage: Chuyển đổi cv::Mat (OpenCV, BGR) sang QImage (Qt, RGB)
    // Bước này cần thiết để hiển thị frame đã xử lý lên QLabel trong giao diện.
    // -----------------------------------------------------------------------
    QImage matToQImage(const cv::Mat &mat)
    {
        // Tạo bản sao và chuyển từ BGR (OpenCV) sang RGB (Qt)
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);

        // Đóng gói thành QImage và trả về bản sao sâu
        QImage img(rgb.data, rgb.cols, rgb.rows,
                   static_cast<int>(rgb.step),
                   QImage::Format_RGB888);
        return img.copy(); // Deep copy để tránh dangling pointer khi rgb bị hủy
    }

    // -----------------------------------------------------------------------
    // detectFaces: Phát hiện khuôn mặt trong ảnh TĨNH (không phải realtime)
    // Được sử dụng khi đăng ký sinh viên mới (kiểm tra ảnh chân dung có mặt không)
    // Khác với processFrame: Hàm này chỉ trả về vị trí mặt, không vẽ hay nhận diện.
    //
    // Tham số: frame - Ảnh BGR (cv::Mat) từ file ảnh chân dung
    // Trả về: Danh sách QRect chứa vị trí từng khuôn mặt tìm thấy
    // -----------------------------------------------------------------------
    std::vector<QRect> detectFaces(const cv::Mat &frame)
    {
        std::vector<QRect> faceRects; // Kết quả trả về
        if (!s_modelLoaded || !s_detector) return faceRects; // Bảo vệ

        try {
            // Cập nhật kích thước input cho YuNet theo kích thước ảnh
            s_detector->setInputSize(cv::Size(frame.cols, frame.rows));
            cv::Mat facesMat; // Ma trận kết quả phát hiện

            // Thực hiện phát hiện khuôn mặt
            s_detector->detect(frame, facesMat);

            // Trích xuất tọa độ bounding box cho từng mặt tìm được
            for (int i = 0; i < facesMat.rows; ++i) {
                int x = static_cast<int>(facesMat.at<float>(i, 0));
                int y = static_cast<int>(facesMat.at<float>(i, 1));
                int w = static_cast<int>(facesMat.at<float>(i, 2));
                int h = static_cast<int>(facesMat.at<float>(i, 3));
                faceRects.push_back(QRect(x, y, w, h));
            }
        } catch (...) {
            // Bắt mọi loại ngoại lệ để tránh crash
            qWarning() << "[AI] Lỗi trong detectFaces.";
        }
        return faceRects;
    }

    // -----------------------------------------------------------------------
    // extractEmbedding: Trích xuất vector đặc trưng từ vùng ảnh khuôn mặt đã cắt
    // Được dùng khi đăng ký sinh viên mới: Lưu "dấu vân tay" khuôn mặt vào SQLite
    //
    // Tham số: faceROI - Vùng ảnh chứa khuôn mặt đã cắt ra (cv::Mat BGR)
    // Trả về: Vector 128 chiều float (đã chuẩn hóa L2), hoặc rỗng nếu thất bại
    // -----------------------------------------------------------------------
    std::vector<float> extractEmbedding(const cv::Mat &faceROI)
    {
        std::vector<float> embedding; // Kết quả trả về
        if (!s_modelLoaded || !s_recognizer || !s_detector) return embedding; // Bảo vệ

        try {
            // Thử phát hiện lại mặt trong vùng ROI để lấy landmarks cho alignCrop
            s_detector->setInputSize(cv::Size(faceROI.cols, faceROI.rows));
            cv::Mat facesMat;
            s_detector->detect(faceROI, facesMat);

            cv::Mat alignedFace;
            if (facesMat.rows > 0) {
                // Căn chỉnh khuôn mặt dựa trên 5 điểm mốc từ YuNet
                s_recognizer->alignCrop(faceROI, facesMat.row(0), alignedFace);
            } else {
                // Fallback: Nếu không detect được mặt trong ROI, chỉ resize về 112x112
                // (Trường hợp này xảy ra khi ảnh đã được cắt chính xác vào mặt)
                cv::resize(faceROI, alignedFace, cv::Size(112, 112));
            }

            // Trích xuất vector đặc trưng 128 chiều từ SFace
            cv::Mat feature;
            s_recognizer->feature(alignedFace, feature);

            // Sao chép dữ liệu từ cv::Mat sang std::vector<float>
            int totalElements = static_cast<int>(feature.total());
            embedding.resize(totalElements);
            const float* dataPtr = feature.ptr<float>();
            if (dataPtr) {
                for (int i = 0; i < totalElements; ++i) {
                    embedding[i] = dataPtr[i];
                }
            }

            // ============================================================
            // CHUẨN HÓA L2 NORM (giống như trong processFrame)
            // Đảm bảo vector lưu trong SQLite và vector realtime đều đã chuẩn hóa,
            // khi tính khoảng cách Euclidean sẽ cho kết quả nhất quán.
            // ============================================================
            float norm = 0.0f;
            // Tính tổng bình phương tất cả phần tử
            for (float v : embedding) norm += v * v;
            // Lấy căn bậc hai để có L2 norm
            norm = std::sqrt(norm);
            // Chia từng phần tử cho norm
            if (norm > 0.0f) {
                for (float &v : embedding) v /= norm;
            }

        } catch (const cv::Exception &e) {
            qWarning() << "[AI] Lỗi trích xuất embedding:" << e.what();
            embedding.clear(); // Trả về vector rỗng khi có lỗi
        }
        return embedding;
    }

    // -----------------------------------------------------------------------
    // euclideanDistance: [HÀM NỘI BỘ - PRIVATE]
    // Tính khoảng cách Euclidean L2 giữa 2 vector đặc trưng
    // Công thức: d = sqrt(sum((a[i] - b[i])^2))
    //
    // Khoảng cách = 0: Hai vector hoàn toàn giống nhau (cùng một người)
    // Khoảng cách nhỏ: Hai người giống nhau
    // Khoảng cách lớn: Hai người khác nhau
    //
    // Tham số: a, b - Hai vector 128 chiều đã chuẩn hóa L2
    // Trả về: Khoảng cách (>= 0), hoặc 999999 nếu kích thước không khớp
    // -----------------------------------------------------------------------
    static double euclideanDistance(const std::vector<float> &a, const std::vector<float> &b)
    {
        // Kiểm tra kích thước phải bằng nhau (đều là 128 chiều)
        if (a.size() != b.size()) return 999999.0;

        double sum = 0.0; // Tổng bình phương hiệu
        for (size_t i = 0; i < a.size(); ++i) {
            double diff = a[i] - b[i]; // Hiệu tại chiều thứ i
            sum += diff * diff;         // Cộng bình phương hiệu
        }
        return std::sqrt(sum); // Lấy căn bậc hai tổng
    }

    // -----------------------------------------------------------------------
    // matchFace: [HÀM NỘI BỘ - PRIVATE]
    // Tìm sinh viên trong s_studentDB có khuôn mặt khớp nhất với embedding đầu vào.
    // Sử dụng thuật toán tìm kiếm tuyến tính (linear search) - phù hợp với quy mô
    // lớp học nhỏ (thường < 100 sinh viên).
    //
    // Tham số:
    //   embedding: Vector 128 chiều của khuôn mặt vừa phát hiện trong frame
    //   faceRect:  Vị trí khuôn mặt trong frame (để gán vào FaceResult)
    // Trả về: FaceResult với thông tin sinh viên khớp (hoặc recognized=false nếu không khớp)
    // -----------------------------------------------------------------------
    static FaceResult matchFace(const std::vector<float> &embedding, const QRect &faceRect)
    {
        // Khởi tạo kết quả mặc định (chưa nhận diện được)
        FaceResult result;
        result.faceRect = faceRect;
        result.recognized = false;
        result.confidence = 0.0;

        // Trường hợp đặc biệt: Database rỗng hoặc embedding rỗng
        if (s_studentDB.empty() || embedding.empty()) {
            return result;
        }

        double minDistance = 999999.0; // Khoảng cách nhỏ nhất tìm được
        int bestIndex = -1;            // Chỉ số sinh viên có khoảng cách nhỏ nhất

        // Duyệt tuyến tính qua toàn bộ danh sách sinh viên trong RAM
        for (int i = 0; i < static_cast<int>(s_studentDB.size()); ++i) {
            // Bỏ qua sinh viên chưa có FaceVector (chưa đăng ký)
            if (s_studentDB[i].faceVector.empty()) continue;

            // Tính khoảng cách Euclidean giữa embedding hiện tại và embedding sinh viên
            double dist = euclideanDistance(embedding, s_studentDB[i].faceVector);

            // Cập nhật nếu tìm được khoảng cách nhỏ hơn
            if (dist < minDistance) {
                minDistance = dist;
                bestIndex = i;
            }
        }

        // Quyết định nhận diện: Khoảng cách nhỏ nhất phải dưới ngưỡng s_threshold
        // Nếu khoảng cách >= ngưỡng -> khuôn mặt quá khác biệt -> không nhận diện
        if (bestIndex >= 0 && minDistance < s_threshold) {
            result.recognized = true;
            result.mssv = s_studentDB[bestIndex].mssv;   // MSSV sinh viên khớp
            result.hoTen = s_studentDB[bestIndex].hoTen; // Tên sinh viên
            // Chuyển đổi khoảng cách thành phần trăm tin cậy (0.0 - 1.0)
            // Công thức: confidence = max(0, 1 - distance/2)
            // Khoảng cách = 0 -> confidence = 1.0 (100%)
            // Khoảng cách = s_threshold ≈ 1.12 -> confidence ≈ 0.44 (44%)
            result.confidence = std::max(0.0, 1.0 - (minDistance / 2.0));
        }

        return result;
    }

} // namespace Face
