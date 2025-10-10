#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>

// Global deðiþkenler
std::vector<cv::Point2f> src_points;
float scale_x = 1.0f;
float scale_y = 1.0f;
cv::Mat display_image_for_callback; // Callback fonksiyonunun eriþebilmesi için

// Mouse týklamalarýný yakalayan fonksiyon
void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        if (src_points.size() < 4) {
            float original_x = x * scale_x;
            float original_y = y * scale_y;
            src_points.push_back(cv::Point2f(original_x, original_y));

            std::cout << "Nokta eklendi (" << src_points.size() << "/4): (" << x << ", " << y << ")" << std::endl;

            cv::circle(display_image_for_callback, cv::Point(x, y), 5, cv::Scalar(0, 0, 255), cv::FILLED);
            cv::imshow("Kalibrasyon - 4 Kose Secin", display_image_for_callback);
        }
    }
}

int main() {
    // 1. AÞAMA: KAMERAYI AÇ VE KALÝBRASYON ÝÇÝN BÝR KARE YAKALA
    cv::VideoCapture cap(0); // 0, varsayýlan web kamerasýný ifade eder
    if (!cap.isOpened()) {
        std::cerr << "HATA: Kamera acilamadi!" << std::endl;
        return -1;
    }

    cv::Mat frame, setup_frame;
    cv::namedWindow("Live Video - Kalibrasyon icin 'c' tusuna basin", cv::WINDOW_NORMAL);

    while (true) {
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "HATA: Kameradan goruntu alinamadi." << std::endl;
            break;
        }

        cv::imshow("Live Video - Kalibrasyon icin 'c' tusuna basin", frame);

        char key = (char)cv::waitKey(1);
        if (key == 'c') {
            setup_frame = frame.clone();
            std::cout << "Goruntu yakalandi. Lutfen 4 koseyi secin." << std::endl;
            cv::destroyWindow("Live Video - Kalibrasyon icin 'c' tusuna basin");
            break;
        }
        else if (key == 27) { // ESC tuþu ile çýkýþ
            return 0;
        }
    }

    if (setup_frame.empty()) {
        return -1;
    }

    // 2. AÞAMA: YAKALANAN KARE ÜZERÝNDE 4 NOKTA SEÇÝMÝ
    const double max_display_width = 1000.0;
    const double max_display_height = 800.0;

    double original_width = setup_frame.cols;
    double original_height = setup_frame.rows;

    if (original_width > max_display_width || original_height > max_display_height) {
        double width_ratio = original_width / max_display_width;
        double height_ratio = original_height / max_display_height;
        double scale_down_ratio = std::max(width_ratio, height_ratio);

        int display_width = static_cast<int>(original_width / scale_down_ratio);
        int display_height = static_cast<int>(original_height / scale_down_ratio);

        cv::resize(setup_frame, display_image_for_callback, cv::Size(display_width, display_height));

        scale_x = (float)original_width / (float)display_width;
        scale_y = (float)original_height / (float)display_height;
    }
    else {
        display_image_for_callback = setup_frame.clone();
    }

    cv::namedWindow("Kalibrasyon - 4 Kose Secin", cv::WINDOW_NORMAL);
    cv::setMouseCallback("Kalibrasyon - 4 Kose Secin", mouseCallback, NULL);
    std::cout << "Lutfen goruntu uzerinde sirasiyla su koseleri tiklayin:" << std::endl;
    std::cout << "1. Sol-Ust -> 2. Sag-Ust -> 3. Sag-Alt -> 4. Sol-Alt" << std::endl;
    cv::imshow("Kalibrasyon - 4 Kose Secin", display_image_for_callback);

    while (src_points.size() < 4) {
        if (cv::waitKey(20) == 27) return 0;
    }
    cv::destroyWindow("Kalibrasyon - 4 Kose Secin");

    // 3. AÞAMA: DÖNÜÞÜM MATRÝSÝNÝ HESAPLA
    float widthA = cv::norm(src_points[1] - src_points[0]);
    float widthB = cv::norm(src_points[2] - src_points[3]);
    float maxWidth = std::max(widthA, widthB);

    float heightA = cv::norm(src_points[3] - src_points[0]);
    float heightB = cv::norm(src_points[2] - src_points[1]);
    float maxHeight = std::max(heightA, heightB);

    std::vector<cv::Point2f> dst_points;
    dst_points.push_back(cv::Point2f(0, 0));
    dst_points.push_back(cv::Point2f(maxWidth - 1, 0));
    dst_points.push_back(cv::Point2f(maxWidth - 1, maxHeight - 1));
    dst_points.push_back(cv::Point2f(0, maxHeight - 1));

    cv::Mat perspective_matrix = cv::getPerspectiveTransform(src_points, dst_points);
    cv::Size output_size(maxWidth, maxHeight);

    std::cout << "Kalibrasyon tamamlandi. Anlik donusum baslatildi. Cikmak icin 'q' tusuna basin." << std::endl;

    // 4. AÞAMA: GERÇEK ZAMANLI ÝÞLEME DÖNGÜSÜ
    cv::Mat warped_frame;
    cv::namedWindow("Live Video", cv::WINDOW_NORMAL);
    cv::namedWindow("Bird's-Eye View", cv::WINDOW_NORMAL);

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        // Hafýzadaki matrisi her yeni kareye uygula
        cv::warpPerspective(frame, warped_frame, perspective_matrix, output_size);

        cv::imshow("Live Video", frame);
        cv::imshow("Bird's-Eye View", warped_frame);

        char key = (char)cv::waitKey(1);
        if (key == 'q' || key == 27) { // q veya ESC tuþu ile çýkýþ
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}