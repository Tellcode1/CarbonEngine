#include "Atlas.hpp"

int main() {
    std::cout << "Initializing...\n";

    std::vector<Image> images = {};
    Image img69{};
    img69.name = "texture";
    img69.data = stbi_load("/home/debian/Downloads/texture.jpg", &img69.width, &img69.height, nullptr, 4);
    images.push_back(img69);

    Image img = Loader::LoadImage("/home/debian/Downloads/code.png");;
    img.name = "code";

    Image img2 = Loader::LoadImage("/home/debian/Downloads/gun.jpeg");
    img2.name = "ak48";

    Image img3 = Loader::LoadImage("/home/debian/Downloads/beretta.jpeg");
    img3.name = "beretta";

    images.push_back(img3);
    images.push_back(img2);
    images.push_back(img);

    std::cout << "Writing to atlas...\n";
    Atlas::WriteImage(images);
    
    std::cout << "Done!" << std::endl;
    return 0;
}