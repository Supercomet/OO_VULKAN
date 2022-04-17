#include "DDSLoader.h"

#include "tinyddsloader.h"
#include "VulkanUtils.h"

namespace oGFX
{
    uint8_t* LoadDDS(const std::string& filename, int* x, int* y, int* comp, uint64_t* imageSize)
    {
        using namespace tinyddsloader;

        DDSFile dds;
        auto ret = dds.Load(filename.c_str());
        if (tinyddsloader::Result::Success != ret)
        {
            return  nullptr;
        }
       
        switch (dds.GetTextureDimension()) 
        {
        case DDSFile::TextureDimension::Texture1D:
                 break;
        case DDSFile::TextureDimension::Texture2D:
                 break;
        case DDSFile::TextureDimension::Texture3D:
                 break;
        default:
                 break;
        }
         
        int32_t bitsPerPixel = {static_cast<int32_t>( dds.GetBitsPerPixel(dds.GetFormat())) };

        if (comp)
        {
            *comp = bitsPerPixel;
        }
        switch (dds.GetFormat()) {
        case DDSFile::DXGIFormat::R32G32B32A32_Typeless:
                 break;
        case DDSFile::DXGIFormat::R32G32B32A32_Float:
                 break;
        case DDSFile::DXGIFormat::R32G32B32A32_UInt:
                 break;
        case DDSFile::DXGIFormat::R32G32B32A32_SInt:
                 break;
        case DDSFile::DXGIFormat::R32G32B32_Typeless:
                 break;
        case DDSFile::DXGIFormat::R32G32B32_Float:
                 break;
        case DDSFile::DXGIFormat::R32G32B32_UInt:
                 break;
        case DDSFile::DXGIFormat::R32G32B32_SInt:
                 break;
        case DDSFile::DXGIFormat::R16G16B16A16_Typeless:
                 break;
        case DDSFile::DXGIFormat::R16G16B16A16_Float:
                 break;
        case DDSFile::DXGIFormat::R16G16B16A16_UNorm:
                 break;
        case DDSFile::DXGIFormat::R16G16B16A16_UInt:
                 break;
        case DDSFile::DXGIFormat::R16G16B16A16_SNorm:
                 break;
        case DDSFile::DXGIFormat::R16G16B16A16_SInt:
                 break;
        case DDSFile::DXGIFormat::R32G32_Typeless:
                 break;
        case DDSFile::DXGIFormat::R32G32_Float:
                 break;
        case DDSFile::DXGIFormat::R32G32_UInt:
                 break;
        case DDSFile::DXGIFormat::R32G32_SInt:
                 break;
        case DDSFile::DXGIFormat::R32G8X24_Typeless:
                 break;
        case DDSFile::DXGIFormat::D32_Float_S8X24_UInt:
                 break;
        case DDSFile::DXGIFormat::R32_Float_X8X24_Typeless:
                 break;
        case DDSFile::DXGIFormat::X32_Typeless_G8X24_UInt:
                 break;
        case DDSFile::DXGIFormat::R10G10B10A2_Typeless:
                 break;
        case DDSFile::DXGIFormat::R10G10B10A2_UNorm:
                 break;
        case DDSFile::DXGIFormat::R10G10B10A2_UInt:
                 break;
        case DDSFile::DXGIFormat::R11G11B10_Float:
                 break;
        case DDSFile::DXGIFormat::R8G8B8A8_Typeless:
                 break;
        case DDSFile::DXGIFormat::R8G8B8A8_UNorm:
                 break;
        case DDSFile::DXGIFormat::R8G8B8A8_UNorm_SRGB:
                 break;
        case DDSFile::DXGIFormat::R8G8B8A8_UInt:
                 break;
        case DDSFile::DXGIFormat::R8G8B8A8_SNorm:
                 break;
        case DDSFile::DXGIFormat::R8G8B8A8_SInt:
                 break;
        case DDSFile::DXGIFormat::R16G16_Typeless:
                 break;
        case DDSFile::DXGIFormat::R16G16_Float:
                 break;
        case DDSFile::DXGIFormat::R16G16_UNorm:
                 break;
        case DDSFile::DXGIFormat::R16G16_UInt:
                 break;
        case DDSFile::DXGIFormat::R16G16_SNorm:
                 break;
        case DDSFile::DXGIFormat::R16G16_SInt:
                 break;
        case DDSFile::DXGIFormat::R32_Typeless:
                 break;
        case DDSFile::DXGIFormat::D32_Float:
                 break;
        case DDSFile::DXGIFormat::R32_Float:
                 break;
        case DDSFile::DXGIFormat::R32_UInt:
                 break;
        case DDSFile::DXGIFormat::R32_SInt:
                 break;
        case DDSFile::DXGIFormat::R24G8_Typeless:
                 break;
        case DDSFile::DXGIFormat::D24_UNorm_S8_UInt:
                 break;
        case DDSFile::DXGIFormat::R24_UNorm_X8_Typeless:
                 break;
        case DDSFile::DXGIFormat::X24_Typeless_G8_UInt:
                 break;
        case DDSFile::DXGIFormat::R8G8_Typeless:
                 break;
        case DDSFile::DXGIFormat::R8G8_UNorm:
                 break;
        case DDSFile::DXGIFormat::R8G8_UInt:
                 break;
        case DDSFile::DXGIFormat::R8G8_SNorm:
                 break;
        case DDSFile::DXGIFormat::R8G8_SInt:
                 break;
        case DDSFile::DXGIFormat::R16_Typeless:
                 break;
        case DDSFile::DXGIFormat::R16_Float:
                 break;
        case DDSFile::DXGIFormat::D16_UNorm:
                 break;
        case DDSFile::DXGIFormat::R16_UNorm:
                 break;
        case DDSFile::DXGIFormat::R16_UInt:
                 break;
        case DDSFile::DXGIFormat::R16_SNorm:
                 break;
        case DDSFile::DXGIFormat::R16_SInt:
                 break;
        case DDSFile::DXGIFormat::R8_Typeless:
                 break;
        case DDSFile::DXGIFormat::R8_UNorm:
                 break;
        case DDSFile::DXGIFormat::R8_UInt:
                 break;
        case DDSFile::DXGIFormat::R8_SNorm:
                 break;
        case DDSFile::DXGIFormat::R8_SInt:
                 break;
        case DDSFile::DXGIFormat::A8_UNorm:
                 break;
        case DDSFile::DXGIFormat::R1_UNorm:
                 break;
        case DDSFile::DXGIFormat::R9G9B9E5_SHAREDEXP:
                 break;
        case DDSFile::DXGIFormat::R8G8_B8G8_UNorm:
                 break;
        case DDSFile::DXGIFormat::G8R8_G8B8_UNorm:
                 break;
        case DDSFile::DXGIFormat::BC1_Typeless:
                 break;
        case DDSFile::DXGIFormat::BC1_UNorm:
                 break;
        case DDSFile::DXGIFormat::BC1_UNorm_SRGB:
                 break;
        case DDSFile::DXGIFormat::BC2_Typeless:
                 break;
        case DDSFile::DXGIFormat::BC2_UNorm:
                 break;
        case DDSFile::DXGIFormat::BC2_UNorm_SRGB:
                 break;
        case DDSFile::DXGIFormat::BC3_Typeless:
                 break;
        case DDSFile::DXGIFormat::BC3_UNorm:
                 break;
        case DDSFile::DXGIFormat::BC3_UNorm_SRGB:
                 break;
        case DDSFile::DXGIFormat::BC4_Typeless:
                 break;
        case DDSFile::DXGIFormat::BC4_UNorm:
                 break;
        case DDSFile::DXGIFormat::BC4_SNorm:
                 break;
        case DDSFile::DXGIFormat::BC5_Typeless:
                 break;
        case DDSFile::DXGIFormat::BC5_UNorm:
                 break;
        case DDSFile::DXGIFormat::BC5_SNorm:
                 break;
        case DDSFile::DXGIFormat::B5G6R5_UNorm:
                 break;
        case DDSFile::DXGIFormat::B5G5R5A1_UNorm:
                 break;
        case DDSFile::DXGIFormat::B8G8R8A8_UNorm:
                 break;
        case DDSFile::DXGIFormat::B8G8R8X8_UNorm:
                 break;
        case DDSFile::DXGIFormat::R10G10B10_XR_BIAS_A2_UNorm:
                 break;
        case DDSFile::DXGIFormat::B8G8R8A8_Typeless:
                 break;
        case DDSFile::DXGIFormat::B8G8R8A8_UNorm_SRGB:
                 break;
        case DDSFile::DXGIFormat::B8G8R8X8_Typeless:
                 break;
        case DDSFile::DXGIFormat::B8G8R8X8_UNorm_SRGB:
                 break;
        case DDSFile::DXGIFormat::BC6H_Typeless:
                 break;
        case DDSFile::DXGIFormat::BC6H_UF16:
                 break;
        case DDSFile::DXGIFormat::BC6H_SF16:
                 break;
        case DDSFile::DXGIFormat::BC7_Typeless:
                 break;
        case DDSFile::DXGIFormat::BC7_UNorm:
                 break;
        case DDSFile::DXGIFormat::BC7_UNorm_SRGB:
                 break;
        case DDSFile::DXGIFormat::AYUV:
                 break;
        case DDSFile::DXGIFormat::Y410:
                 break;
        case DDSFile::DXGIFormat::Y416:
                 break;
        case DDSFile::DXGIFormat::NV12:
                 break;
        case DDSFile::DXGIFormat::P010:
                 break;
        case DDSFile::DXGIFormat::P016:
                 break;
        case DDSFile::DXGIFormat::YUV420_OPAQUE:
                 break;
        case DDSFile::DXGIFormat::YUY2:
                 break;
        case DDSFile::DXGIFormat::Y210:
                 break;
        case DDSFile::DXGIFormat::Y216:
                 break;
        case DDSFile::DXGIFormat::NV11:
                 break;
        case DDSFile::DXGIFormat::AI44:
                 break;
        case DDSFile::DXGIFormat::IA44:
                 break;
        case DDSFile::DXGIFormat::P8:
                 break;
        case DDSFile::DXGIFormat::A8P8:
                 break;
        case DDSFile::DXGIFormat::B4G4R4A4_UNorm:
                 break;
        case DDSFile::DXGIFormat::P208:
                 break;
        case DDSFile::DXGIFormat::V208:
                 break;
        case DDSFile::DXGIFormat::V408:
                 break;
        }

        auto test = dds.GetFormat();

        const auto imageInformation = dds.GetImageData(0, 0);

        const uint64_t dataSize = imageInformation->m_memSlicePitch;
        uint8_t* data= new uint8_t[dataSize];
        memcpy(data,imageInformation->m_mem , dataSize);

        //for (uint32_t arrayIdx = 0; arrayIdx < dds.GetArraySize(); arrayIdx++) {
        //    for (uint32_t mipIdx = 0; mipIdx < dds.GetMipCount(); mipIdx++) {
        //        const auto* imageData = dds.GetImageData(mipIdx, arrayIdx);
        //    }
        //}

        if (x)
        {
            *x = imageInformation->m_width;
        }      
        if (y)
        {
            *y =  imageInformation->m_height;
        }
        if (imageSize)
        {
            *imageSize = dataSize;
        }

        return data;
	}

    void LoadDDS(const std::string& filename, oGFX::FileImageData& data)
    {
        using namespace tinyddsloader;

        DDSFile dds;
        auto ret = dds.Load(filename.c_str());
        if (tinyddsloader::Result::Success != ret)
        {
            return;
        }

        auto getFormat = [&]()->VkFormat
        {
            switch (dds.GetFormat())
            {
            case DDSFile::DXGIFormat::R8G8B8A8_UNorm:
            case DDSFile::DXGIFormat::R8G8B8A8_UNorm_SRGB:
            return (data.imgType == FileImageData::ImageType::LINEAR) ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_SRGB;
            break;

            case DDSFile::DXGIFormat::B8G8R8A8_UNorm:
            case DDSFile::DXGIFormat::B8G8R8A8_UNorm_SRGB:
            return  (data.imgType == FileImageData::ImageType::LINEAR) ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_B8G8R8A8_SRGB;
            break;


            case DDSFile::DXGIFormat::B8G8R8X8_UNorm:
            case DDSFile::DXGIFormat::B8G8R8X8_UNorm_SRGB:
            break;

            case DDSFile::DXGIFormat::BC1_UNorm:
            case DDSFile::DXGIFormat::BC1_UNorm_SRGB:
            return (data.imgType == FileImageData::ImageType::LINEAR) ? VK_FORMAT_BC1_RGBA_UNORM_BLOCK : VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            break;

            case DDSFile::DXGIFormat::BC2_UNorm:
            case DDSFile::DXGIFormat::BC2_UNorm_SRGB:
            return  (data.imgType == FileImageData::ImageType::LINEAR) ? VK_FORMAT_BC2_UNORM_BLOCK : VK_FORMAT_BC2_SRGB_BLOCK;
            break;

            case DDSFile::DXGIFormat::BC3_UNorm:
            case DDSFile::DXGIFormat::BC3_UNorm_SRGB:
            return  (data.imgType == FileImageData::ImageType::LINEAR) ? VK_FORMAT_BC3_UNORM_BLOCK : VK_FORMAT_BC3_SRGB_BLOCK;
            break;

            case DDSFile::DXGIFormat::BC5_UNorm:
            return  (data.imgType == FileImageData::ImageType::LINEAR) ? VK_FORMAT_BC5_UNORM_BLOCK : VK_FORMAT_UNDEFINED;

            break;

            default:
            return VK_FORMAT_B8G8R8A8_SRGB;
            break;
            }
        };

        data.format = getFormat();

        uint64_t dataSize{};
        for (size_t i = 0; i < dds.GetMipCount(); i++)
        {
            const auto imageInformation = dds.GetImageData(i, 0);

            VkBufferImageCopy copyRegion{};
            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = i;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.bufferOffset = dataSize;
            copyRegion.imageExtent.width = imageInformation->m_width;
            copyRegion.imageExtent.height = imageInformation->m_height;
            copyRegion.imageExtent.depth = 1;

            data.mipInformation.push_back(copyRegion);
            dataSize += imageInformation->m_memSlicePitch;
        }

        data.imgData.resize(dataSize);
        for (size_t i = 0; i <  dds.GetMipCount(); i++)
        {
            const auto imageInformation = dds.GetImageData(i, 0);
            memcpy(data.imgData.data() +data.mipInformation[i].bufferOffset  ,imageInformation->m_mem , imageInformation->m_memSlicePitch);
        }
        

        const auto imageInformation = dds.GetImageData(0, 0);
        data.w = imageInformation->m_width;
        data.h =  imageInformation->m_height;
        data.dataSize = dataSize;        

    }
}
