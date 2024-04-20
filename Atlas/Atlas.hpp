#ifndef __TEXTURE_ATLAS__
#define __TEXTURE_ATLAS__

// #include <stb/stb_image_write.h>
// #include <ctime>
// #include <cstdint>
// #include <string>
// #include <memory.h>
// #include <vector>
// #include <fstream>
// #include <chrono>
// #include <cassert>
// #include <iomanip>
// #include <iostream>
// #include "../include/stdafx.hpp"
#include "stdafx.hpp"

namespace Atlas {

    void Init() {
        char buffer[128];
        std::cout << "Please specify the mode: ";
        std::cin >> buffer;

        if(strcmp(buffer, "text")) {
            
        }
    }

    void memcpyImage(Image const& srcImage, uchar* dstBuffer, const u64 totalWidth, const u64 xOffset) {
        // Calculate the size of each row in bytes
        const u64 rowSize = srcImage.width * 4;
        const u64 totalWidthBytes = totalWidth * 4;
        const u64 offsetBytes = xOffset * 4;
        
        // Copy each row from source to destination
        for(u64 rowIndex = 0; rowIndex < static_cast<u32>(srcImage.height); rowIndex++) {
            memcpy(dstBuffer + rowIndex * totalWidthBytes + offsetBytes, srcImage.data + rowIndex * rowSize, rowSize);
        }
    }

    // Got this from stack overflow
    const std::string currentDateTime() {
        const time_t now = time(0);
        struct tm*    tstruct = localtime(&now);
        char         buf[20];
        
        // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
        // for more information about date/time format
        // strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
        strftime(buf, sizeof(buf), "%d/%m/%Y at %H:%M", tstruct);

        return std::string(buf);
    }

    void WriteImage(std::vector<Image>& images) {
        const auto start = std::chrono::high_resolution_clock::now();
        u64 totalWidth = 0;
        u64 totalHeight = 0;

        // Calculate the total width and height of the atlas
        for (const auto& img : images) {
            // Prechecks
            assert(img.data != nullptr);

            totalWidth += img.width;
            totalHeight = std::max(totalHeight, static_cast<u64>(img.height));
        }
        
        // Allocate memory for the atlas
        const u64 atlasByteSize = totalWidth * totalHeight * 4;
        uchar* data = static_cast<uchar*>(malloc(atlasByteSize));
        memset(data, 0, atlasByteSize);

        std::ofstream header("atlas.hpp");
        assert(header.is_open());

        header  << std::fixed << std::setprecision(5)
            << "/*\n"
            << "    File generated on " << currentDateTime() << '\n'
            << "    THIS FILE WAS GENERATED AUTOMATICALLY BY ATLAS.\n"
            << "    DO NOT EDIT UNLESS YOU KNOW WHAT YOU'RE DOING.\n"
            << "*/\n\n"

            << "#ifndef __ATLAS_OUTPUT_H__\n"
            << "#define __ATLAS_OUTPUT_H__\n"
            << "\n#include \"Atlas/Output_type.hpp\"\n\n";

        header << "enum AvailableImages\n"
                << "{\n";
        u64 i = 0;
        for(const auto& img : images) {
            header << "    ATLAS_IMAGE_" << img.name << " = " << i << ",\n";
            i++;
        }
        header << "};\n\n";

        header << "constexpr Atlas_OutputImage images[" << images.size() << "] = \n{\n";
        
        u64 textureIndex = 0;
        u64 xOffset = 0;

        for(const auto& img : images) {
            const u64 widthPlusOffset = static_cast<u64>(img.width) + xOffset;
            const float totalWidthF = static_cast<float>(totalWidth);
            const float heightF = img.height / static_cast<float>(totalHeight);
            const float offsetoverWidth = xOffset / totalWidthF;
            const float widthPlusOffsetF = widthPlusOffset / totalWidthF;

            std::cout << "Writing image \"" << img.name << "\"...\n";
            memcpyImage(img, data, totalWidth, xOffset);

            header
                << "    {\n"
                "        \"" << img.name << "\",\n"
                "        {\n"
                "           {" << offsetoverWidth << ", " << 0.0f << "},\n"
                "           {" << offsetoverWidth << ", " << heightF << "},\n"
                "           {" << widthPlusOffsetF << ", " << heightF << "},\n"
                "           {" << widthPlusOffsetF << ", " << 0.0f << "},\n"
                "        },\n"
                "    },\n";

            std::cout << "Finsished writing image \"" << img.name << "\"...\n";

            xOffset += img.width;
            textureIndex++;
        }
        
        header << "};\n"; // Close off the array (of output images).
        
        header
            << "\nconstexpr Atlas_OutputImage GetImageFromAtlas(const char* name)\n"
            << "{\n"
            << "    for(unsigned int i = 0; i < " << images.size() << "; i++)\n"
            << "    {\n"
            << "        // constexpr_strcmp() is provided by Atlas/Output_type.hpp\n"
            << "        if(constexpr_strcmp(images[i].name, name))\n"
            << "        {\n"
            << "            return images[i];\n"
            << "        }\n"
            << "    }\n"
            << "}\n"

            << "\nconstexpr Atlas_OutputImage GetImageFromAtlas(unsigned int index)\n"
            << "{\n"
            << "    return images[index];\n"
            << "}\n";

        header << "\n#endif"; // Close off the header guard.

        std::cout << "Finished copying images in " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() << "ms\n";

        const auto writeStart = std::chrono::high_resolution_clock::now();

        //  Write the atlas to a PNG file
        assert(stbi_write_png("atlas.png", totalWidth, totalHeight, 4, data, totalWidth * 4) == true);

        std::cout << "Finished writing atlas image to disk in " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - writeStart).count() << "ms\n";

        std::cout << "Wrote " << images.size() << " images to atlas\n";
    }
}

#endif
