#include <filesystem>
#include "CppUnitTest.h"
#include "opencv2/core.hpp"
#include "ImageFile.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace fs = std::filesystem;

namespace GLSingleTests
{
	TEST_CLASS(ImageFileTests)
	{
	public:
		TEST_METHOD(KoreanPath_WriteThenRead_ReturnsSamePixels)
		{
			const fs::path dir = fs::temp_directory_path() / L"객체비교_테스트";
			fs::create_directories(dir);
			const std::wstring path = (dir / L"기준 이미지.png").wstring();

			cv::Mat image(40, 60, CV_8UC3);
			cv::randu(image, 0, 256);

			Assert::IsTrue(objcmp::WriteImageFile(path, image));
			const cv::Mat loaded = objcmp::ReadImageFile(path);

			Assert::IsFalse(loaded.empty());
			Assert::AreEqual(0.0, cv::norm(image, loaded, cv::NORM_INF));
			fs::remove_all(dir);
		}

		TEST_METHOD(MissingFile_ReturnsEmpty)
		{
			const std::wstring path = (fs::temp_directory_path() / L"존재하지_않는_파일.png").wstring();
			Assert::IsTrue(objcmp::ReadImageFile(path).empty());
		}

		TEST_METHOD(NonImageFile_ReturnsEmpty)
		{
			const fs::path path = fs::temp_directory_path() / L"이미지아님.png";
			FILE* file = nullptr;
			_wfopen_s(&file, path.c_str(), L"wb");
			Assert::IsNotNull(file);
			fputs("not an image", file);
			fclose(file);

			Assert::IsTrue(objcmp::ReadImageFile(path.wstring()).empty());
			fs::remove(path);
		}
	};
}
