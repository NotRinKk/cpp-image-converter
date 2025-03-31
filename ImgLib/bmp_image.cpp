#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    uint16_t signature = 0x4D42;
    uint32_t file_size = 0;
    uint32_t reserved = 0;
    uint32_t data_offset = 54;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t header_size = 40;
    int32_t width = 0;
    int32_t height = 0;
    uint16_t planes = 1;
    uint16_t bpp = 24;
    uint32_t compression = 0;
    uint32_t data_size = 0;
    uint32_t x_resolution = 11811;
    uint32_t y_resolution = 11811;
    uint32_t colors_used = 0;
    uint32_t significant_colors = 0x1000000;
}
PACKED_STRUCT_END

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);
    
    if (!out) {
        return false;
    }

    const int width = image.GetWidth();
    const int height = image.GetHeight();
    const int stride = GetBMPStride(width);

    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    info_header.width = width;
    info_header.height = height;
    info_header.data_size = stride * height;

    file_header.file_size = sizeof(file_header) +
                            sizeof(info_header) + info_header.data_size;                            
    file_header.data_offset = sizeof(file_header) + sizeof(info_header);


    out.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
    out.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));                        
    
    std::vector<char> buff(stride);

    for (int y = 0; y < height; ++y) {

        const Color* line = image.GetLine(height - 1 - y);

        for (int x = 0; x < width; ++x) {
            buff[x * 3 + 0] = static_cast<char>(line[x].b);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        out.write(buff.data(), stride);
    }
    return out.good();
}

Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);

    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    ifs.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    ifs.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    //'BM' в little-endian
    if (file_header.signature != 0x4D42 ||
        info_header.compression != 0 ||
        info_header.bpp != 24 ||
        info_header.significant_colors != 0x1000000) {

        return {};
    }

    const int width = info_header.width;
    const int height = info_header.height;
    const int stride = GetBMPStride(width);

    ifs.seekg(file_header.data_offset, std::ios::beg);

    Image result(width, height, Color::Black());
    std::vector<char> buff(stride);

    for (int y = height - 1; y >= 0; --y) {
        ifs.read(buff.data(), stride);
        if (!ifs) {
            return {};
        }

        Color* line = result.GetLine(y);

        for (int x = 0; x < width; ++x) {
            line[x].b = std::byte(buff[x * 3 + 0]);
            line[x].g = std::byte(buff[x * 3 + 1]);
            line[x].r = std::byte(buff[x * 3 + 2]);
        }
    }

    return result;

}

}  // namespace img_lib