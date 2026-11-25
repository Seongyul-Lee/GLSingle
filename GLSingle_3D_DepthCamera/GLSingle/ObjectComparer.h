// ObjectComparer.h: 기준 이미지와 비교 대상 이미지의 객체 차이를 추출합니다.
// MFC에 의존하지 않는 순수 OpenCV 모듈이므로 단위 테스트 프로젝트에서도 그대로 컴파일됩니다.

#pragma once
#include <vector>
#include "opencv2/core.hpp"

namespace objcmp
{
	enum class CompareStatus
	{
		Ok,
		InvalidInput,     // 빈 이미지 또는 지원하지 않는 형식
		AlignmentFailed   // 특징점 정합 실패 (다른 물체이거나 촬영 각도 차이가 너무 큼)
	};

	struct DefectRegion
	{
		cv::Rect box;   // 기준 이미지 좌표계(작업 해상도) 기준
		double area = 0.0;
	};

	struct CompareOptions
	{
		int maxSide = 1024;            // 작업 해상도: 긴 변을 이 크기 이하로 축소
		int minInliers = 15;           // 정합 성공으로 인정할 최소 RANSAC inlier 수
		double diffThreshold = 0.25;   // DSSIM((1-SSIM)/2, 0~1)이 이 값을 넘으면 차이로 판단
		double minDefectArea = 100.0;  // 이보다 작은 영역(px², 작업 해상도)은 노이즈로 제거
	};

	struct CompareResult
	{
		CompareStatus status = CompareStatus::InvalidInput;
		int inlierCount = 0;
		cv::Mat reference;      // 작업 해상도로 변환된 기준 이미지 (BGR)
		cv::Mat alignedTarget;  // 기준 좌표계로 정렬된 비교 대상 이미지 (BGR)
		cv::Mat diffMap;        // 차이 맵 (CV_8U, 0~255)
		cv::Mat overlay;        // 결함 영역을 표시한 결과 이미지 (BGR)
		std::vector<DefectRegion> defects;
		double damagedRatio = 0.0;  // 비교 가능 영역 대비 결함 면적 비율 (0~1)
	};

	CompareResult CompareObjects(const cv::Mat& reference, const cv::Mat& target,
		const CompareOptions& options = CompareOptions());
}
