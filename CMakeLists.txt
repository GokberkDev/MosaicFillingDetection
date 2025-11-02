# Gerekli minimum CMake versiyonunu belirt
cmake_minimum_required(VERSION 3.10)

# Projenin adını tanımla
project(MosaicFillingDetection)

# Derlediğimiz OpenCV'yi bulmasını sağla. 
# ÖNEMLİ: find_package'in çalışması için bir sonraki adımda CMake'e yol göstereceğiz.
find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc aruco)

# Projemize çalıştırılabilir bir dosya ekle (main.cpp'den oluşacak)
add_executable(MosaicFillingDetectionApp main.cpp)

# OpenCV kütüphanelerini çalıştırılabilir dosyamıza bağla
target_link_libraries(MosaicFillingDetectionApp ${OpenCV_LIBS})