//
//  main.cpp
//  BitmapMsgEncoder
//
//  Created by Raphaël Oundjian on 14/06/2024.
//
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

using namespace std;


#pragma pack(push, 1)
struct BITMAPFILEHEADER {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BITMAPINFOHEADER {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

void encodeMessage(const std::string &imagePath, const std::string &message, const std::string &outputPath) {
    std::ifstream imageFile(imagePath, std::ios::binary);
    if (!imageFile.is_open()) {
        std::cerr << "Unable to open image file\n";
        return;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    imageFile.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    imageFile.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    std::vector<unsigned char> imageData(infoHeader.biSizeImage);
    imageFile.seekg(fileHeader.bfOffBits, std::ios::beg);
    imageFile.read(reinterpret_cast<char*>(imageData.data()), infoHeader.biSizeImage);
    imageFile.close();

    size_t messageLength = message.length();
    size_t messageBitsLength = messageLength * 8;
    if (messageBitsLength > infoHeader.biSizeImage) {
        std::cerr << "Message is too long to be encoded in this image\n";
        return;
    }

    size_t bitIndex = 0;
    for (size_t i = 0; i < messageLength; ++i) {
        for (int j = 0; j < 8; ++j) {
            imageData[bitIndex] = (imageData[bitIndex] & 0xFE) | ((message[i] >> (7 - j)) & 1);
            bitIndex++;
        }
    }

    std::ofstream outputFile(outputPath, std::ios::binary);
    outputFile.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    outputFile.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
    outputFile.write(reinterpret_cast<const char*>(imageData.data()), infoHeader.biSizeImage);
    outputFile.close();

    std::cout << "Message encoded successfully\n";
}

void decodeMessage(const std::string &imagePath, size_t messageLength) {
    std::ifstream imageFile(imagePath, std::ios::binary);
    if (!imageFile.is_open()) {
        std::cerr << "Unable to open image file\n";
        return;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    imageFile.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    imageFile.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    std::vector<unsigned char> imageData(infoHeader.biSizeImage);
    imageFile.seekg(fileHeader.bfOffBits, std::ios::beg);
    imageFile.read(reinterpret_cast<char*>(imageData.data()), infoHeader.biSizeImage);
    imageFile.close();

    std::vector<char> message(messageLength + 1, 0);
    size_t bitIndex = 0;
    for (size_t i = 0; i < messageLength; ++i) {
        for (int j = 0; j < 8; ++j) {
            message[i] |= ((imageData[bitIndex] & 1) << (7 - j));
            bitIndex++;
        }
    }

    std::cout << "Decoded message: " << message.data() << "\n";
}
int main()
{
    string  imagePathSrc= "image.bmp";
    string imagePathDest = "imageSecret.bmp";
    string secretMessage = "Hello Nicolas";
    encodeMessage(imagePathSrc, secretMessage , imagePathDest);
    
    decodeMessage(imagePathDest, secretMessage.length());
    return 0;
}
