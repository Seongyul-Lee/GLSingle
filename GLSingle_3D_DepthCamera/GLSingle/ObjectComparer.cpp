#include "ObjectComparer.h"
#include <algorithm>
#include "opencv2/calib3d.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgproc.hpp"

namespace objcmp
{
	namespace
	{
		// 8비트 1/3/4채널 이미지를 BGR로 바꾸고 긴 변을 maxSide 이하로 축소합니다.
		cv::Mat ToWorkingBgr(const cv::Mat& image, int maxSide)
		{
			if (image.empty() || image.depth() != CV_8U)
				return cv::Mat();

			cv::Mat bgr;
			switch (image.channels())
			{
			case 1: cv::cvtColor(image, bgr, cv::COLOR_GRAY2BGR); break;
			case 3: bgr = image.clone(); break;
			case 4: cv::cvtColor(image, bgr, cv::COLOR_BGRA2BGR); break;
			default: return cv::Mat();
			}

			const int longest = std::max(bgr.cols, bgr.rows);
			if (longest > maxSide)
			{
				const double scale = static_cast<double>(maxSide) / longest;
				const cv::Size size(cvRound(bgr.cols * scale), cvRound(bgr.rows * scale));
				cv::resize(bgr, bgr, size, 0, 0, cv::INTER_AREA);
			}
			return bgr;
		}

		// 비교 대상을 기준 좌표계로 옮기는 호모그래피를 SIFT 매칭 + RANSAC으로 추정합니다.
		cv::Mat EstimateHomography(const cv::Mat& referenceGray, const cv::Mat& targetGray, int minInliers, int& inlierCount)
		{
			inlierCount = 0;
			cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
			std::vector<cv::KeyPoint> referenceKeypoints, targetKeypoints;
			cv::Mat referenceDescriptors, targetDescriptors;
			sift->detectAndCompute(referenceGray, cv::noArray(), referenceKeypoints, referenceDescriptors);
			sift->detectAndCompute(targetGray, cv::noArray(), targetKeypoints, targetDescriptors);
			if (referenceDescriptors.empty() || targetDescriptors.empty())
				return cv::Mat();

			std::vector<std::vector<cv::DMatch>> knnMatches;
			cv::BFMatcher(cv::NORM_L2).knnMatch(targetDescriptors, referenceDescriptors, knnMatches, 2);

			// Lowe의 ratio test로 애매한 매칭을 버립니다.
			std::vector<cv::Point2f> targetPoints, referencePoints;
			for (const auto& pair : knnMatches)
			{
				if (pair.size() == 2 && pair[0].distance < 0.75f * pair[1].distance)
				{
					targetPoints.push_back(targetKeypoints[pair[0].queryIdx].pt);
					referencePoints.push_back(referenceKeypoints[pair[0].trainIdx].pt);
				}
			}
			const int required = std::max(minInliers, 4);
			if (static_cast<int>(targetPoints.size()) < required)
				return cv::Mat();

			cv::Mat inlierMask;
			cv::Mat homography = cv::findHomography(targetPoints, referencePoints, cv::RANSAC, 5.0, inlierMask);
			if (homography.empty())
				return cv::Mat();
			inlierCount = cv::countNonZero(inlierMask);
			return inlierCount >= required ? homography : cv::Mat();
		}

		// SSIM 맵 (Wang et al. 2004, 11x11 가우시안 창)
		cv::Mat ComputeSsim(const cv::Mat& x, const cv::Mat& y)
		{
			const double C1 = 6.5025, C2 = 58.5225;  // (0.01*255)^2, (0.03*255)^2
			const cv::Size window(11, 11);
			const double sigma = 1.5;

			cv::Mat muX, muY;
			cv::GaussianBlur(x, muX, window, sigma);
			cv::GaussianBlur(y, muY, window, sigma);
			const cv::Mat muX2 = muX.mul(muX), muY2 = muY.mul(muY), muXY = muX.mul(muY);

			cv::Mat sigmaX2, sigmaY2, sigmaXY;
			cv::GaussianBlur(x.mul(x), sigmaX2, window, sigma);
			cv::GaussianBlur(y.mul(y), sigmaY2, window, sigma);
			cv::GaussianBlur(x.mul(y), sigmaXY, window, sigma);
			sigmaX2 -= muX2;
			sigmaY2 -= muY2;
			sigmaXY -= muXY;

			const cv::Mat numerator = (2 * muXY + C1).mul(2 * sigmaXY + C2);
			const cv::Mat denominator = (muX2 + muY2 + C1).mul(sigmaX2 + sigmaY2 + C2);
			cv::Mat ssim;
			cv::divide(numerator, denominator, ssim);
			return ssim;
		}
	}

	CompareResult CompareObjects(const cv::Mat& reference, const cv::Mat& target, const CompareOptions& options)
	{
		CompareResult result;
		result.reference = ToWorkingBgr(reference, options.maxSide);
		const cv::Mat targetBgr = ToWorkingBgr(target, options.maxSide);
		if (result.reference.empty() || targetBgr.empty())
		{
			result.status = CompareStatus::InvalidInput;
			return result;
		}

		// 1. 정합: 비교 대상을 기준 이미지 좌표계로 변환
		cv::Mat referenceGray, targetGray;
		cv::cvtColor(result.reference, referenceGray, cv::COLOR_BGR2GRAY);
		cv::cvtColor(targetBgr, targetGray, cv::COLOR_BGR2GRAY);
		const cv::Mat homography = EstimateHomography(referenceGray, targetGray, options.minInliers, result.inlierCount);
		if (homography.empty())
		{
			result.status = CompareStatus::AlignmentFailed;
			return result;
		}

		const cv::Size size = result.reference.size();
		cv::warpPerspective(targetBgr, result.alignedTarget, homography, size, cv::INTER_LINEAR, cv::BORDER_CONSTANT);
		cv::Mat validMask;
		cv::warpPerspective(cv::Mat(targetBgr.size(), CV_8U, cv::Scalar(255)), validMask, homography, size,
			cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(0));
		// 비교 대상 바깥(검은 테두리)이 SSIM 창에 섞이는 경계부를 제외합니다.
		cv::erode(validMask, validMask, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15)));
		const int validPixels = cv::countNonZero(validMask);
		if (validPixels == 0)
		{
			result.status = CompareStatus::AlignmentFailed;
			return result;
		}

		// 2. 밝기 정규화: 비교 가능 영역의 평균/표준편차를 기준 이미지에 맞춥니다.
		cv::Mat alignedGray;
		cv::cvtColor(result.alignedTarget, alignedGray, cv::COLOR_BGR2GRAY);
		cv::Scalar referenceMean, referenceStd, targetMean, targetStd;
		cv::meanStdDev(referenceGray, referenceMean, referenceStd, validMask);
		cv::meanStdDev(alignedGray, targetMean, targetStd, validMask);
		const double gain = targetStd[0] > 1e-6 ? referenceStd[0] / targetStd[0] : 1.0;
		cv::Mat referenceFloat, targetFloat;
		referenceGray.convertTo(referenceFloat, CV_32F);
		alignedGray.convertTo(targetFloat, CV_32F, gain, referenceMean[0] - targetMean[0] * gain);
		// 해상도/초점 차이로 생기는 가는 선의 선명도 차이를 같은 강도의 블러로 상쇄합니다.
		cv::GaussianBlur(referenceFloat, referenceFloat, cv::Size(0, 0), 1.5);
		cv::GaussianBlur(targetFloat, targetFloat, cv::Size(0, 0), 1.5);

		// 3. 차이 맵: DSSIM = (1 - SSIM) / 2, 0(동일) ~ 1(완전히 다름)
		cv::Mat dssim = (1.0 - ComputeSsim(referenceFloat, targetFloat)) * 0.5;
		dssim.setTo(0, validMask == 0);
		dssim.convertTo(result.diffMap, CV_8U, 255.0);

		// 4. 이진화 → 모폴로지 → 윤곽선 추출
		cv::Mat changed = (dssim > options.diffThreshold) & validMask;
		cv::morphologyEx(changed, changed, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
		cv::morphologyEx(changed, changed, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7)));

		std::vector<std::vector<cv::Point>> contours;
		cv::findContours(changed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
		cv::Mat defectMask = cv::Mat::zeros(size, CV_8U);
		double defectArea = 0.0;
		for (size_t i = 0; i < contours.size(); ++i)
		{
			const double area = cv::contourArea(contours[i]);
			if (area < options.minDefectArea)
				continue;
			result.defects.push_back({ cv::boundingRect(contours[i]), area });
			cv::drawContours(defectMask, contours, static_cast<int>(i), cv::Scalar(255), cv::FILLED);
			defectArea += area;
		}
		result.damagedRatio = defectArea / validPixels;

		// 5. 시각화: 결함 영역을 빨갛게 반투명 처리하고 박스를 그립니다.
		result.overlay = result.alignedTarget.clone();
		cv::Mat tinted;
		cv::addWeighted(result.alignedTarget, 0.5, cv::Mat(size, CV_8UC3, cv::Scalar(0, 0, 255)), 0.5, 0.0, tinted);
		tinted.copyTo(result.overlay, defectMask);
		for (const DefectRegion& defect : result.defects)
			cv::rectangle(result.overlay, defect.box, cv::Scalar(0, 0, 255), 2);

		result.status = CompareStatus::Ok;
		return result;
	}
}
