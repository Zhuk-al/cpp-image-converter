#include "ppm_image.h"

#include <array>
#include <fstream>
#include <stdio.h>
#include <setjmp.h>
#include <vector>

#include <jpeglib.h>

using namespace std;

namespace img_lib {

    struct my_error_mgr {
        struct jpeg_error_mgr pub;
        jmp_buf setjmp_buffer;
    };

    typedef struct my_error_mgr* my_error_ptr;

    METHODDEF(void)
        my_error_exit(j_common_ptr cinfo) {
        my_error_ptr myerr = (my_error_ptr)cinfo->err;
        (*cinfo->err->output_message) (cinfo);
        longjmp(myerr->setjmp_buffer, 1);
    }

    bool SaveJPEG(const Path& file, const Image& image) {
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        FILE* outfile;      
        JSAMPROW row_pointer[1]; 
        int row_stride;

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);

#ifdef _MSC_VER
        if ((outfile = _wfopen(file.wstring().c_str(), "wb")) == NULL) {
#else
        if ((outfile = fopen(file.string().c_str(), "wb")) == NULL) {
#endif
            return false;
        }

        jpeg_stdio_dest(&cinfo, outfile);

        cinfo.image_width = image.GetWidth();
        cinfo.image_height = image.GetHeight();
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;

        jpeg_set_defaults(&cinfo);
        jpeg_start_compress(&cinfo, TRUE);
        row_stride = image.GetWidth() * 3;

        while (cinfo.next_scanline < cinfo.image_height) {
            std::vector<JSAMPLE> jsample(row_stride);

            for (int i = 0; i < image.GetWidth(); ++i) {

                jsample[3 * i] = JSAMPLE(image.GetLine(cinfo.next_scanline)[i].r);
                jsample[1 + 3 * i] = JSAMPLE(image.GetLine(cinfo.next_scanline)[i].g);
                jsample[2 + 3 * i] = JSAMPLE(image.GetLine(cinfo.next_scanline)[i].b);
            }
            row_pointer[0] = &jsample[0];
            (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);
        fclose(outfile);
        jpeg_destroy_compress(&cinfo);

        return true;
    }

    void SaveSñanlineToImage(const JSAMPLE * row, int y, Image & out_image) {
        Color* line = out_image.GetLine(y);
        for (int x = 0; x < out_image.GetWidth(); ++x) {
            const JSAMPLE* pixel = row + x * 3;
            line[x] = Color{ byte{pixel[0]}, byte{pixel[1]}, byte{pixel[2]}, byte{255} };
        }
    }

    Image LoadJPEG(const Path & file) {
        jpeg_decompress_struct cinfo;
        my_error_mgr jerr;

        FILE* infile;
        JSAMPARRAY buffer;
        int row_stride;

#ifdef _MSC_VER
        if ((infile = _wfopen(file.wstring().c_str(), "rb")) == NULL) {
#else
        if ((infile = fopen(file.string().c_str(), "rb")) == NULL) {
#endif
            return {};
        }

        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;

        if (setjmp(jerr.setjmp_buffer)) {
            jpeg_destroy_decompress(&cinfo);
            fclose(infile);
            return {};
        }

        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, infile);
        (void)jpeg_read_header(&cinfo, TRUE);
        cinfo.out_color_space = JCS_RGB;
        cinfo.output_components = 3;
        (void)jpeg_start_decompress(&cinfo);

        row_stride = cinfo.output_width * cinfo.output_components;

        buffer = (*cinfo.mem->alloc_sarray)
            ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
/
        Image result(cinfo.output_width, cinfo.output_height, Color::Black());

        while (cinfo.output_scanline < cinfo.output_height) {
            int y = cinfo.output_scanline;
            (void)jpeg_read_scanlines(&cinfo, buffer, 1);

            SaveSñanlineToImage(buffer[0], y, result);
        }

        (void)jpeg_finish_decompress(&cinfo);

        jpeg_destroy_decompress(&cinfo);
        fclose(infile);

        return result;
        }

    } // of namespace img_lib