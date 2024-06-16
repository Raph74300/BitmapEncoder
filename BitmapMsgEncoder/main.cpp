//
//  main.cpp
//  BitmapMsgEncoder
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

typedef enum
{
    MODIFIY_LEAST_SIGNIFICANT_BIT= 0,
    MODIFY_MOST_SIGNIFICANT_BIT
}TeSteganographyMode;

void initSteganography(TeSteganographyMode _eSteganographyMode = MODIFIY_LEAST_SIGNIFICANT_BIT);

void (*pfConcealMessage)(vector<unsigned char>&, const string&);
void concealMessageAtLSBit(vector<unsigned char>  &imageData, const string &message);
void concealMessageAtMSBit(vector<unsigned char>  &imageData, const string &message);

void (*pfExtractMessage)(const vector<unsigned char>&, vector<char>&, size_t messageLength);
void extractMessageFromLSBit(const vector<unsigned char> &imageData, vector<char> &message, size_t messageLength);
void extractMessageFromHSBit(const vector<unsigned char> &imageData, vector<char> &message, size_t messageLength);


void encodeMessage(const string &imagePath, const string &message, const string &outputPath);
void decodeMessage(const string &imagePath, size_t messageLength);

void concealMessageAtLSBit(vector<unsigned char>  &imageData, const string &message)
{
    size_t bitIndex = 0;
    // Explanations :
    // & 0xFE used to erase the lowest bit of the pixel.
    // >> (7- j )  --> bit 7, 6, 5, 4, 3, 2, 1, 0 are successively positioned at the lowest bit
    // & 1 for isolating the least significant bit 0 or 1.
    // | add the bits of the message.
    for (size_t i = 0; i < message.length(); ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            imageData[bitIndex] = (imageData[bitIndex] & 0xFE) | ((message[i] >> (7 - j)) & 1);
            bitIndex++;
        }
    }
}

void concealMessageAtMSBit(vector<unsigned char>  &imageData, const string &message)
{
    size_t bitIndex = 0;
    // Explanations :
    // & 0xEF used to erase the hightest bit of the pixel.
    // << ( j )  --> bit 7, 6, 5, 4, 3, 2, 1, 0 are successively positioned at the highest bit.
    // & 0x80 for isolating the most significant bit 0 or 1.
    // | add the bits of the message.
    for (size_t i = 0; i < message.length(); ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            imageData[bitIndex] = (imageData[bitIndex] & 0xEF) | ((message[i] << j) & 0x80);
            bitIndex++;
        }
    }
}

void extractMessageFromLSBit(const vector<unsigned char> &imageData, vector<char> &message, size_t messageLength)
{
    size_t bitIndex = 0;
    // Explanations :
    // Get the 1st bit of the hidden message. Must start at bitIndex =0;
    // & 1 for isolating the least significant bit 0 or 1.
    // << (7 - j )  --> ImageDate at bitIndex 0, 1, 2, 3, 4, 5, 6, 7 are progressively repositionned to create a valid char byte.
    // | add the bits 0-1 to the message at i.
    for (size_t i = 0; i < messageLength; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
           message[i] |= ((imageData[bitIndex] & 1) << (7 - j));
           bitIndex++;
        }
    }
}

void extractMessageFromHSBit(const vector<unsigned char> &imageData, vector<char> &message, size_t messageLength)
{
    size_t bitIndex = 0;
    // Explanations :
    // For each, get the MS bit of the hidden message at imageData bitIndex 0;
    // & 0x80 for isolating the most significant bit 0 or 1.
    // >> j  -> Each MS bit from ImageDate at bitIndex 0, 1, 2, 3, 4, 5, 6, 7 are progressively repositionned to create a valid char byte.
    // | add the bits 0-1 to the message at i.
    for (size_t i = 0; i < messageLength; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
           message[i] |= ((imageData[bitIndex] & 0x80) >> j);
           bitIndex++;
        }
    }
}


void encodeMessage(const string &imagePath, const string &message, const string &outputPath)
{
    std::ifstream imageFile(imagePath, ios::binary);
    if (!imageFile.is_open())
    {
        cerr << "Cannot open file n";
    }
    else
    {
        BITMAPFILEHEADER fileHeader;
        BITMAPINFOHEADER infoHeader;
        imageFile.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
        imageFile.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
        
        if (infoHeader.biSizeImage == 0)
        {
            int bytesPerPixel = infoHeader.biBitCount / 8;
            int padding = (4 - (infoHeader.biWidth * bytesPerPixel) % 4) % 4;
            infoHeader.biSizeImage = (infoHeader.biWidth * bytesPerPixel + padding) * abs(infoHeader.biHeight);
            cout << "biSizeImage computed from biWidth and biHeight." << endl;
        }
        else{ /* Nothing to do */ }
        
        cout <<  "Max characters :  " << infoHeader.biSizeImage/8 << endl;
        
        vector<unsigned char> imageData(infoHeader.biSizeImage);
        imageFile.seekg(fileHeader.bfOffBits, ios::beg);
        imageFile.read(reinterpret_cast<char*>(imageData.data()), infoHeader.biSizeImage);
        imageFile.close();
        
        size_t messageLength = message.length();
        size_t messageBitsLength = messageLength * 8;
        if (messageBitsLength > infoHeader.biSizeImage)
        {
            cerr << "Message is too long to be encoded in this image\n";
        }
        else
        {
                pfConcealMessage(imageData, message);  // Be carreful, don't forget to initialize...
          
                ofstream outputFile(outputPath, ios::binary);
                outputFile.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
                outputFile.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
                outputFile.write(reinterpret_cast<const char*>(imageData.data()), infoHeader.biSizeImage);
                outputFile.close();
                
                cout << "Message encoded successfully" << endl;
        }
    }
}

void decodeMessage(const string &imagePath, size_t messageLength)
{
    std::ifstream imageFile(imagePath, ios::binary);
    if (!imageFile.is_open())
    {
        cerr << "Cannot open image\n";
    }
    else
    {
        BITMAPFILEHEADER fileHeader;
        BITMAPINFOHEADER infoHeader;
        imageFile.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
        imageFile.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
        
        vector<unsigned char> imageData(infoHeader.biSizeImage);
        imageFile.seekg(fileHeader.bfOffBits, ios::beg);
        imageFile.read(reinterpret_cast<char*>(imageData.data()), infoHeader.biSizeImage);
        imageFile.close();
        
        vector<char> message(messageLength + 1, 0);
        
        pfExtractMessage(imageData, message, messageLength); // Be carreful, don't forget to initialize...
        
        cout << "Decoded message: " << message.data() << "\n";
    }
}

void initSteganography(TeSteganographyMode _eSteganographyMode)
{
    switch (_eSteganographyMode)
    {
        case MODIFY_MOST_SIGNIFICANT_BIT:
            pfConcealMessage = concealMessageAtMSBit;
            pfExtractMessage = extractMessageFromHSBit;
            break;
        case MODIFIY_LEAST_SIGNIFICANT_BIT:
            pfConcealMessage = concealMessageAtLSBit;
            pfExtractMessage = extractMessageFromLSBit;
            break;
        default:
            break;
    }
}

int main()
{
    string imagePathSrc= "image.bmp";
    string imagePathDest = "encoded_imageHSB.bmp";
    string secretMessage  ="";
    initSteganography(MODIFIY_LEAST_SIGNIFICANT_BIT); // Be carreful, don't forget to initialize...
    
    ifstream file("message.txt");
    if (!file.is_open())
    {
        cerr << "Cannot open message" << endl;
    }
    else
    {
        string line;
        while (getline(file, line))
        {
            secretMessage.append(line);
        };
        file.close();
        
        encodeMessage(imagePathSrc, secretMessage , imagePathDest);
        decodeMessage(imagePathDest, secretMessage.length());
    }

    return 0;
}
