#include "CppUnitTest.h"
#include "opencv2/imgproc.hpp"
#include "ObjectComparer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using objcmp::CompareObjects;
using objcmp::CompareResult;
using objcmp::CompareStatus;

namespace
{
	// 특징점이 충분한 합성 물체 이미지. 밝기 +30 테스트에서 포화되지 않도록 40~200 범위로 제한합니다.
	cv::Mat MakeTexturedObject(uint64 seed, cv::Size size = cv::Size(640, 480))
	{
		cv::RNG rng(seed);
		cv::Mat image(size, CV_8UC3);
		rng.fill(image, cv::RNG::UNIFORM, 40, 200);
		cv::GaussianBlur(image, image, cv::Size(0, 0), 2.0);
		for (int i = 0; i < 80; ++i)
		{
			const cv::Point p(rng.uniform(0, size.width), rng.uniform(0, size.height));
			const cv::Scalar color(rng.uniform(40, 200), rng.uniform(40, 200), rng.uniform(40, 200));
			if (i % 2 == 0)
				cv::circle(image, p, rng.uniform(5, 25), color, cv::FILLED, cv::LINE_AA);
			else
				cv::rectangle(image, cv::Rect(p, cv::Size(rng.uniform(10, 40), rng.uniform(10, 40))), color, cv::FILLED);
		}
		return image;
	}

	// 파손/파임을 흉내 낸 어두운 원형 결함
	void PaintDefect(cv::Mat& image, cv::Point center, int radius)
	{
		cv::circle(image, center, radius, cv::Scalar(20, 20, 20), cv::FILLED, cv::LINE_AA);
	}

	void AssertSingleDefectAt(const CompareResult& result, cv::Point expectedCenter)
	{
		Assert::IsTrue(result.status == CompareStatus::Ok, L"정합에 성공해야 합니다.");
		Assert::AreEqual(size_t(1), result.defects.size(), L"결함 영역은 정확히 1개여야 합니다.");
		Assert::IsTrue(result.defects[0].box.contains(expectedCenter), L"결함 위치가 기대 위치를 포함해야 합니다.");
	}
}

namespace GLSingleTests
{
	TEST_CLASS(ObjectComparerTests)
	{
	public:
		TEST_METHOD(EmptyInput_ReturnsInvalidInput)
		{
			const CompareResult result = CompareObjects(cv::Mat(), MakeTexturedObject(1));
			Assert::IsTrue(result.status == CompareStatus::InvalidInput);
		}

		TEST_METHOD(IdenticalImages_NoDefects)
		{
			const cv::Mat reference = MakeTexturedObject(1);
			const CompareResult result = CompareObjects(reference, reference.clone());

			Assert::IsTrue(result.status == CompareStatus::Ok);
			Assert::AreEqual(size_t(0), result.defects.size());
			Assert::AreEqual(0.0, result.damagedRatio);
		}

		TEST_METHOD(GrayscaleImages_NoDefects)
		{
			cv::Mat reference;
			cv::cvtColor(MakeTexturedObject(1), reference, cv::COLOR_BGR2GRAY);
			const CompareResult result = CompareObjects(reference, reference.clone());

			Assert::IsTrue(result.status == CompareStatus::Ok);
			Assert::AreEqual(size_t(0), result.defects.size());
		}

		TEST_METHOD(PaintedDefect_DetectedAtLocation)
		{
			const cv::Mat reference = MakeTexturedObject(1);
			cv::Mat target = reference.clone();
			PaintDefect(target, cv::Point(400, 240), 30);

			const CompareResult result = CompareObjects(reference, target);

			AssertSingleDefectAt(result, cv::Point(400, 240));
			Assert::IsTrue(result.damagedRatio > 0.0);
			Assert::IsFalse(result.overlay.empty());
			Assert::IsTrue(result.overlay.size() == reference.size());
		}

		TEST_METHOD(BrightnessChange_NoDefects)
		{
			const cv::Mat reference = MakeTexturedObject(1);
			cv::Mat target;
			reference.convertTo(target, -1, 1.0, 30.0);

			const CompareResult result = CompareObjects(reference, target);

			Assert::IsTrue(result.status == CompareStatus::Ok);
			Assert::AreEqual(size_t(0), result.defects.size());
		}

		TEST_METHOD(RotatedScaledShiftedTarget_DefectDetectedInReferenceCoordinates)
		{
			const cv::Mat reference = MakeTexturedObject(1);
			cv::Mat damaged = reference.clone();
			PaintDefect(damaged, cv::Point(400, 240), 30);

			// 촬영 위치가 조금 달라진 상황: 5도 회전, 0.9배, (15, -10) 이동
			// 실제 사진처럼 화면 밖도 내용이 이어지도록 BORDER_REFLECT 사용 (검은 채움선 방지)
			cv::Mat transform = cv::getRotationMatrix2D(cv::Point2f(320, 240), 5.0, 0.9);
			transform.at<double>(0, 2) += 15;
			transform.at<double>(1, 2) -= 10;
			cv::Mat target;
			cv::warpAffine(damaged, target, transform, reference.size(), cv::INTER_LINEAR, cv::BORDER_REFLECT);

			const CompareResult result = CompareObjects(reference, target);

			AssertSingleDefectAt(result, cv::Point(400, 240));
		}

		TEST_METHOD(LowerResolutionBlurredTarget_ThinLinesNotReportedAsDefects)
		{
			// 가는 선(헤어라인, 테두리)은 해상도/초점이 조금만 달라도 대비가 약해집니다.
			cv::Mat reference = MakeTexturedObject(1);
			for (int x = 30; x < 610; x += 40)
				cv::line(reference, cv::Point(x, 20), cv::Point(x, 460), cv::Scalar(150, 150, 150), 2);

			cv::Mat target;
			cv::resize(reference, target, cv::Size(), 0.85, 0.85, cv::INTER_AREA);
			cv::GaussianBlur(target, target, cv::Size(0, 0), 0.6);

			const CompareResult result = CompareObjects(reference, target);

			Assert::IsTrue(result.status == CompareStatus::Ok);
			Assert::AreEqual(size_t(0), result.defects.size());
		}

		TEST_METHOD(UnrelatedObject_ReturnsAlignmentFailed)
		{
			const CompareResult result = CompareObjects(MakeTexturedObject(1), MakeTexturedObject(999));
			Assert::IsTrue(result.status == CompareStatus::AlignmentFailed);
		}

		TEST_METHOD(LargeImage_WorkingResolutionLimitedToMaxSide)
		{
			const cv::Mat reference = MakeTexturedObject(1, cv::Size(2000, 1500));
			const CompareResult result = CompareObjects(reference, reference.clone());

			Assert::IsTrue(result.status == CompareStatus::Ok);
			Assert::AreEqual(1024, std::max(result.reference.cols, result.reference.rows));
			Assert::IsTrue(result.alignedTarget.size() == result.reference.size());
		}
	};
}
