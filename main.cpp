#include <iostream>
#include <vector>
#include <map>
#include <algorithm> // Sýralama (std::sort) için eklendi
#include <utility>   // Çift (std::pair) için eklendi
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>

// Marker merkezini hesaplayan yardýmcý fonksiyon
cv::Point2f getMarkerCenter(const std::vector<cv::Point2f>& markerCorners) {
    float x = 0, y = 0;
    for (const auto& corner : markerCorners) {
        x += corner.x;
        y += corner.y;
    }
    return cv::Point2f(x / 4.0f, y / 4.0f);
}

int main() {
    // 1. AÞAMA: KAMERA VE ARUCO AYARLARI
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "HATA: Kamera acilamadi!" << std::endl;
        return -1;
    }

    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_5X5_250);
    cv::aruco::DetectorParameters detectorParams;
    cv::aruco::ArucoDetector detector(dictionary, detectorParams);

    // --- YENÝ MANTIK BAÞLANGICI ---

    // Artýk tek bir hedef ID'miz var
    const int TARGET_ID = 23;

    const int output_width = 800;
    const int output_height = 600;
    const cv::Size output_size(output_width, output_height);

    std::vector<cv::Point2f> dst_points;
    dst_points.push_back(cv::Point2f(0, 0));
    dst_points.push_back(cv::Point2f(output_width - 1, 0));
    dst_points.push_back(cv::Point2f(output_width - 1, output_height - 1));
    dst_points.push_back(cv::Point2f(0, output_height - 1));

    std::cout << "Tam olarak 4 adet ID " << TARGET_ID << " marker'i aranýyor..." << std::endl;
    std::cout << "Cikmak icin 'q' tusuna basin." << std::endl;

    cv::namedWindow("Live Video - Aruco Detection", cv::WINDOW_NORMAL);
    cv::namedWindow("Bird's-Eye View (Alan)", cv::WINDOW_NORMAL);

    // 2. AÞAMA: GERÇEK ZAMANLI ÝÞLEME DÖNGÜSÜ
    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        std::vector<int> marker_ids;
        std::vector<std::vector<cv::Point2f>> marker_corners;
        detector.detectMarkers(frame, marker_corners, marker_ids);

        cv::Mat display_frame = frame.clone();
        bool all_corners_found = false;

        // Sadece ID'si TARGET_ID olan marker'larýn köþe bilgilerini ve indekslerini sakla
        std::vector<std::vector<cv::Point2f>> target_markers_corners;

        if (!marker_ids.empty()) {
            cv::aruco::drawDetectedMarkers(display_frame, marker_corners, marker_ids);

            for (size_t i = 0; i < marker_ids.size(); ++i) {
                if (marker_ids[i] == TARGET_ID) {
                    target_markers_corners.push_back(marker_corners[i]);
                }
            }
        }

        // Tam olarak 4 tane TARGET_ID'li marker bulundu mu?
        if (target_markers_corners.size() == 4) {
            all_corners_found = true;

            // Marker'larý merkezlerine göre sýralamak için bir vektör oluþtur
            // <index, center_point>
            std::vector<std::pair<int, cv::Point2f>> marker_centers;
            for (size_t i = 0; i < target_markers_corners.size(); ++i) {
                marker_centers.push_back({ (int)i, getMarkerCenter(target_markers_corners[i]) });
            }

            std::vector<cv::Point2f> src_points(4);

            // 1. Sýralama: x+y toplamýna göre (Sol-Üst ve Sað-Alt'ý bulmak için)
            std::sort(marker_centers.begin(), marker_centers.end(), [](const auto& a, const auto& b) {
                return (a.second.x + a.second.y) < (b.second.x + b.second.y);
                });

            // en küçük x+y = Sol-Üst (Top-Left)
            int tl_index = marker_centers.front().first;
            src_points[0] = target_markers_corners[tl_index][0]; // Sol-Üst marker'ýn 0. köþesi

            // en büyük x+y = Sað-Alt (Bottom-Right)
            int br_index = marker_centers.back().first;
            src_points[2] = target_markers_corners[br_index][2]; // Sað-Alt marker'ýn 2. köþesi

            // 2. Sýralama: x-y farkýna göre (Sað-Üst ve Sol-Alt'ý bulmak için)
            std::sort(marker_centers.begin(), marker_centers.end(), [](const auto& a, const auto& b) {
                return (a.second.x - a.second.y) < (b.second.x - b.second.y);
                });

            // en küçük x-y = Sol-Alt (Bottom-Left)
            int bl_index = marker_centers.front().first;
            src_points[3] = target_markers_corners[bl_index][3]; // Sol-Alt marker'ýn 3. köþesi

            // en büyük x-y = Sað-Üst (Top-Right)
            int tr_index = marker_centers.back().first;
            src_points[1] = target_markers_corners[tr_index][1]; // Sað-Üst marker'ýn 1. köþesi

            // Dönüþümü gerçekleþtir
            cv::Mat perspective_matrix = cv::getPerspectiveTransform(src_points, dst_points);
            cv::Mat warped_frame;
            cv::warpPerspective(frame, warped_frame, perspective_matrix, output_size);

            cv::imshow("Bird's-Eye View (Alan)", warped_frame);
        }

        if (!all_corners_found) {
            // 4 köþe de bulunamadýysa, bekleme ekranýný göster
            cv::Mat placeholder = cv::Mat::zeros(output_size, CV_8UC3);
            std::string msg = "Bulunan ID " + std::to_string(TARGET_ID) + " sayisi: " + std::to_string(target_markers_corners.size()) + "/4";
            cv::putText(placeholder, msg,
                cv::Point(20, output_height / 2), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
            cv::imshow("Bird's-Eye View (Alan)", placeholder);
        }

        cv::imshow("Live Video - Aruco Detection", display_frame);

        char key = (char)cv::waitKey(1);
        if (key == 'q' || key == 27) {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}