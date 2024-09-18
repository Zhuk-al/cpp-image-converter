#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <iostream>
#include <string_view>

using namespace std;

namespace img_lib {

    PACKED_STRUCT_BEGIN BitmapFileHeader{
        char symb1 = 'B';
        char symb2 = 'M';

        uint32_t header_size = 0;
        uint32_t reserved_space = 0;
        uint32_t indention = 54;
    }
    PACKED_STRUCT_END

    PACKED_STRUCT_BEGIN BitmapInfoHeader{
        uint32_t header_size = 40;

        int32_t width = 0;
        int32_t height = 0;

        uint16_t layers = 1;
        uint16_t bit_per_pixel = 24;

        uint32_t compression_type = 0;
        uint32_t bytes_at_data = 0;

        int32_t horizontal_pixel_per_meter = 11811;
        int32_t vertical_pixel_per_meter = 11811;

        int32_t colors_in_use = 0;
        int32_t colors_sign = 0x1000000;
    }
    PACKED_STRUCT_END

        static int GetBMPStride(int w) {
        return 4 * ((w * 3 + 3) / 4);
    }

    bool SaveBMP(const Path& file, const Image& image) {
        ofstream out(file, ios::binary);

        int width = image.GetWidth();
        int height = image.GetHeight();

        const int bmp_stride = GetBMPStride(width);

        BitmapFileHeader file_header;
        BitmapInfoHeader info_header;

        file_header.header_size = bmp_stride * height + file_header.indention;

        info_header.width = width;
        info_header.height = height;
        info_header.bytes_at_data = height * bmp_stride;

        out.write(reinterpret_cast<const char*>(&file_header), 14);
        out.write(reinterpret_cast<const char*>(&info_header), 40);

        std::vector<char> buff(bmp_stride);

        for (int i = height - 1; i >= 0; --i) {
            const Color* line = image.GetLine(i);

            for (int j = 0; j != width; ++j) {

                buff[3 * j] = static_cast<char>(line[j].b);
                buff[1 + 3 * j] = static_cast<char>(line[j].g);
                buff[2 + 3 * j] = static_cast<char>(line[j].r);
            }

            out.write(buff.data(), bmp_stride);
        }

        return out.good();
    }

    Image LoadBMP(const Path& file) {
        ifstream input(file, ios::binary);

        if (!input) {
            cerr << "Failed to open BMP file: " << file << endl;
            return Image();
        }

        BitmapFileHeader file_header;
        BitmapInfoHeader info_header;

        input.read(reinterpret_cast<char*>(&file_header), sizeof(BitmapFileHeader));
 
        if (file_header.symb1 != 'B' || file_header.symb2 != 'M') {
            cerr << "Incorrect BMP file format: " << file << endl;
            return Image();
        }

        input.read(reinterpret_cast<char*>(&info_header), sizeof(BitmapInfoHeader));

        if (info_header.bit_per_pixel != 24 || info_header.compression_type != 0) {
            cerr << "Unsupported BMP format: " << file << endl;
            return Image();
        }

        int width = info_header.width;
        int height = info_header.height;
        auto color = Color::Black();

        const int bmp_stride = GetBMPStride(width);

        std::vector<char> buff(bmp_stride);
        Image result(width, height, color);

        for (int i = height - 1; i >= 0; --i) {

            input.read(buff.data(), bmp_stride);

            if (!input) {
                cerr << "Failed to read BMP image data: " << file << endl;
                return Image();
            }

            Color* line = result.GetLine(i);

            for (int j = 0; j != width; ++j) {
                line[j].b = static_cast<byte>(buff[j * 3]);
                line[j].g = static_cast<byte>(buff[1 + j * 3]);
                line[j].r = static_cast<byte>(buff[2 + j * 3]);
            }
        }

        return result;
    }

}  // namespace img_libh