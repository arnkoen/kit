KIT

A small utility thingie, to quickly make something using bgfx from c. It uses HandMadeMath for math operations, qoi.h for image loading (embedded) and m3d.h for basic model loading. \
This is very much a work in progress, so feel free, to report any issues. \

First, you'll need to build bgfx for your platform, drop the static libraries into the libs/ folder. Then:

    sh ./build.bat <myfile.c> [debug]
    cd shd
    sh ./build.bat

On windows just use "build" instead of "sh ./build.bat". For building the shaders shaderc must be visible in $PATH.