#include "jpeg.h"
#include <cstdio>
#include <csetjmp>
#include <jpeglib.h>
namespace {
struct Error { jpeg_error_mgr base;std::jmp_buf jump; };
void fail(j_common_ptr info){std::longjmp(reinterpret_cast<Error*>(info->err)->jump,1);}
}
bool readJpeg(const std::string& path,unsigned& width,unsigned& height,std::vector<unsigned char>& pixels){
    auto* file=std::fopen(path.c_str(),"rb");if(!file)return false;
    jpeg_decompress_struct decoder{};Error error{};
    decoder.err=jpeg_std_error(&error.base);error.base.error_exit=fail;
    if(setjmp(error.jump)){jpeg_destroy_decompress(&decoder);std::fclose(file);return false;}
    jpeg_create_decompress(&decoder);jpeg_stdio_src(&decoder,file);jpeg_read_header(&decoder,TRUE);
    if(!decoder.image_width||!decoder.image_height||decoder.image_width>4096||decoder.image_height>4096){
        jpeg_destroy_decompress(&decoder);std::fclose(file);return false;
    }
    decoder.out_color_space=JCS_RGB;jpeg_start_decompress(&decoder);
    width=decoder.output_width;height=decoder.output_height;pixels.resize(size_t(width)*height*4);
    while(decoder.output_scanline<decoder.output_height){
        auto* row=pixels.data()+size_t(decoder.output_scanline)*width*4;
        jpeg_read_scanlines(&decoder,&row,1);
        for(int x=int(width)-1;x>=0;--x){
            const unsigned char r=row[x*3],g=row[x*3+1],b=row[x*3+2];
            row[x*4]=r;row[x*4+1]=g;row[x*4+2]=b;row[x*4+3]=255;
        }
    }
    jpeg_finish_decompress(&decoder);jpeg_destroy_decompress(&decoder);std::fclose(file);return true;
}
