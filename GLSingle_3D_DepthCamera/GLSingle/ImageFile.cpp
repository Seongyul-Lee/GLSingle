#include "ImageFile.h"
#include <cstdio>
#include <vector>
#include "opencv2/imgcodecs.hpp"

namespace objcmp
{
	cv::Mat ReadImageFile(const std::wstring& path)
	{
		FILE* file = nullptr;
		if (_wfopen_s(&file, path.c_str(), L"rb") != 0 || !file)
			return cv::Mat();

		std::vector<uchar> bytes;
		uchar buffer[64 * 1024];
		size_t read;
		while ((read = fread(buffer, 1, sizeof(buffer), file)) > 0)
			bytes.insert(bytes.end(), buffer, buffer + read);
		fclose(file);

		if (bytes.empty())
			return cv::Mat();
		return cv::imdecode(bytes, cv::IMREAD_COLOR);
	}

	bool WriteImageFile(const std::wstring& path, const cv::Mat& image)
	{
		const size_t dot = path.find_last_of(L'.');
		if (image.empty() || dot == std::wstring::npos)
			return false;

		// 확장자는 ASCII이므로 그대로 좁은 문자열로 옮겨도 안전합니다.
		std::string extension;
		for (size_t i = dot; i < path.size(); ++i)
			extension += static_cast<char>(path[i]);

		std::vector<uchar> bytes;
		try
		{
			if (!cv::imencode(extension, image, bytes))
				return false;
		}
		catch (const cv::Exception&)  // 지원하지 않는 확장자
		{
			return false;
		}

		FILE* file = nullptr;
		if (_wfopen_s(&file, path.c_str(), L"wb") != 0 || !file)
			return false;
		const bool written = fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
		return fclose(file) == 0 && written;
	}
}
