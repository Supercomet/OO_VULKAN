#pragma once
#include <string>

namespace oGFX { class FileImageData; };

namespace oGFX{

	uint8_t* LoadDDS(const std::string& filename, int* x, int* y, int* comp,uint64_t* imageSize);
	void LoadDDS(const std::string& filename, oGFX::FileImageData& data);

 }// end namespace oGFX

