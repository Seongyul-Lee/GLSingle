// ImageFile.h: 한글 등 유니코드 경로를 지원하는 이미지 읽기/쓰기.
// cv::imread/imwrite는 Windows에서 비 ASCII 경로를 열지 못하므로 바이트로 읽어 디코딩합니다.

#pragma once
#include <string>
#include "opencv2/core.hpp"

namespace objcmp
{
	cv::Mat ReadImageFile(const std::wstring& path);  // 실패 시 빈 Mat
	bool WriteImageFile(const std::wstring& path, const cv::Mat& image);  // 확장자로 형식 결정
}
