//----------------------------------------------------------------------------------------------//
// FILE: bmpimage.cpp                                                                           //
// AUTHOR: Ian Murfin - github.com/ianmurfinxyz                                                 //
//----------------------------------------------------------------------------------------------//

#include <cinttypes>
#include <vector>
#include <fstream>
#include <cmath>
#include "color.h"
#include "bmpimage.h"

int BmpImage::load(std::string filename)
{
  std::ifstream file {filename, std::ios_base::binary};
  if(!file){
    return -1;
  }

  FileHeader fileHead {};
  file.read(reinterpret_cast<char*>(&fileHead._fileMagic), sizeof(fileHead._fileMagic));

  if(fileHead._fileMagic != BMPMAGIC){
    return -1;
  }

  file.read(reinterpret_cast<char*>(&fileHead._fileSize_bytes), sizeof(fileHead._fileSize_bytes));
  file.read(reinterpret_cast<char*>(&fileHead._reserved0), sizeof(fileHead._reserved0));
  file.read(reinterpret_cast<char*>(&fileHead._reserved1), sizeof(fileHead._reserved1));
  file.read(reinterpret_cast<char*>(&fileHead._pixelOffset_bytes), sizeof(fileHead._pixelOffset_bytes));

  InfoHeader infoHead {};
  file.read(reinterpret_cast<char*>(&infoHead._headerSize_bytes), sizeof(infoHead._headerSize_bytes));
  file.read(reinterpret_cast<char*>(&infoHead._bmpWidth_px), sizeof(infoHead._bmpWidth_px));
  file.read(reinterpret_cast<char*>(&infoHead._bmpHeight_px), sizeof(infoHead._bmpHeight_px));
  file.read(reinterpret_cast<char*>(&infoHead._numColorPlanes), sizeof(infoHead._numColorPlanes));
  file.read(reinterpret_cast<char*>(&infoHead._bitsPerPixel), sizeof(infoHead._bitsPerPixel));
  file.read(reinterpret_cast<char*>(&infoHead._compression), sizeof(infoHead._compression));
  file.read(reinterpret_cast<char*>(&infoHead._imageSize_bytes), sizeof(infoHead._imageSize_bytes));
  file.read(reinterpret_cast<char*>(&infoHead._xResolution_pxPm), sizeof(infoHead._xResolution_pxPm));
  file.read(reinterpret_cast<char*>(&infoHead._yResolution_pxPm), sizeof(infoHead._yResolution_pxPm));
  file.read(reinterpret_cast<char*>(&infoHead._numPaletteColors), sizeof(infoHead._numPaletteColors));
  file.read(reinterpret_cast<char*>(&infoHead._numImportantColors), sizeof(infoHead._numImportantColors));

  int infoHeadVersion {1};

  if(infoHead._headerSize_bytes >= V2INFOHEADER_SIZE_BYTES ||
    (infoHead._headerSize_bytes == V1INFOHEADER_SIZE_BYTES && infoHead._compression == BI_BITFIELDS))
  {
    file.read(reinterpret_cast<char*>(&infoHead._redMask), sizeof(infoHead._redMask));
    file.read(reinterpret_cast<char*>(&infoHead._greenMask), sizeof(infoHead._greenMask));
    file.read(reinterpret_cast<char*>(&infoHead._blueMask), sizeof(infoHead._blueMask));
    infoHeadVersion = 2;
  }

  if(infoHead._headerSize_bytes >= V3INFOHEADER_SIZE_BYTES){
    file.read(reinterpret_cast<char*>(&infoHead._alphaMask), sizeof(infoHead._alphaMask));
    infoHeadVersion = 3;
  }

  if(infoHead._headerSize_bytes >= V4INFOHEADER_SIZE_BYTES){
    file.read(reinterpret_cast<char*>(&infoHead._colorSpaceMagic), sizeof(infoHead._colorSpaceMagic));
    if(infoHead._colorSpaceMagic != SRGBMAGIC){
      return -1;
    }
    infoHeadVersion = 4;
  }

  if(infoHead._headerSize_bytes >= V5INFOHEADER_SIZE_BYTES){
    infoHeadVersion = 5;
  }

  if(infoHead._compression != BI_RGB && infoHead._compression != BI_BITFIELDS){
    return -1;
  }

  switch(infoHead._bitsPerPixel)
  {
  case 1:
  case 2:
  case 4:
  case 8:
    extractIndexedPixels(file, fileHead, infoHead);
    break;
  case 16:
    if(infoHead._compression == BI_RGB){
      infoHead._redMask   = 0x007c00;      // default masks.
      infoHead._greenMask = 0x0003e0;
      infoHead._blueMask  = 0x00001f;
      if(infoHeadVersion < 3)
        infoHead._alphaMask = 0x8000;
    }
    extractPixels(file, fileHead, infoHead); 
    break;
  case 24:
    infoHead._redMask   = 0xff0000;      // default masks.
    infoHead._greenMask = 0x00ff00;
    infoHead._blueMask  = 0x0000ff;
    infoHead._alphaMask = 0x000000;
    extractPixels(file, fileHead, infoHead); 
    break;
  case 32:
    if(infoHead._compression == BI_RGB){
      infoHead._redMask   = 0xff0000;      // default masks.
      infoHead._greenMask = 0x00ff00;
      infoHead._blueMask  = 0x0000ff;
      if(infoHeadVersion < 3)
        infoHead._alphaMask = 0xff000000;
    }
    extractPixels(file, fileHead, infoHead); 
    break;
  }

  _width_px = infoHead._bmpWidth_px;
  _height_px = infoHead._bmpHeight_px;

  return 0;
}

void BmpImage::extractIndexedPixels(std::ifstream& file, FileHeader& fileHead, InfoHeader& infoHead)
{
  // extract the color palette.
  std::vector<Color4> palette {};
  file.seekg(FILEHEADER_SIZE_BYTES + infoHead._headerSize_bytes, std::ios::beg);
  for(uint8_t i = 0; i < infoHead._numPaletteColors; ++i){
    char bytes[4];
    file.read(bytes, 4);

    // colors expected in the byte order blue (0), green (1), red (2), alpha (3).
    uint8_t red = static_cast<uint8_t>(bytes[2]);
    uint8_t green = static_cast<uint8_t>(bytes[1]);
    uint8_t blue = static_cast<uint8_t>(bytes[0]);
    uint8_t alpha = static_cast<uint8_t>(bytes[3]);

    palette.push_back(Color4{red, green, blue, alpha});
  }

  int rowSize_bytes = std::ceil((infoHead._bitsPerPixel * infoHead._bmpWidth_px) / 32.f) * 4.f;  
  int numPixelsPerByte = 8 / infoHead._bitsPerPixel;

  uint8_t mask {0};
  for(int i = 0; i < infoHead._bitsPerPixel; ++i)
    mask |= (0x01 << i);

  int numRows = std::abs(infoHead._bmpHeight_px);
  bool isTopOrigin = (infoHead._bmpHeight_px < 0);
  int pixelOffset_bytes = static_cast<int>(fileHead._pixelOffset_bytes);
  int rowOffset_bytes {rowSize_bytes};
  if(isTopOrigin){
    pixelOffset_bytes += (numRows - 1) * rowSize_bytes;
    rowOffset_bytes *= -1;
  }

  _pixels.reserve(infoHead._bmpWidth_px * numRows);

  int seekPos {pixelOffset_bytes};
  char* row = new char[rowSize_bytes];

  // for each row of pixels.
  for(int i = 0; i < numRows; ++i){
    file.seekg(seekPos);
    file.read(static_cast<char*>(row), rowSize_bytes);

    // for each pixel in the row.
    int numPixelsExtracted {0};
    int byteNo {0};
    int bytePixelNo {0};
    uint8_t byte = static_cast<uint8_t>(row[byteNo]);
    while(numPixelsExtracted < infoHead._bmpWidth_px){
      if(bytePixelNo >= numPixelsPerByte){
        byte = static_cast<uint8_t>(row[++byteNo]);
        bytePixelNo = 0;
      }
      int shift = infoHead._bitsPerPixel * (numPixelsPerByte - 1 - bytePixelNo);
      uint8_t index = (byte & (mask << shift)) >> shift;
      _pixels.push_back(palette[index]);
      ++numPixelsExtracted;
      ++bytePixelNo;
    }
    seekPos += rowOffset_bytes;
  }
  delete[] row;
}

void BmpImage::extractPixels(std::ifstream& file, FileHeader& fileHead, InfoHeader& infoHead)
{
  // note: this function handles 16-bit, 24-bit and 32-bit pixels.

  // If bitmap height is negative the origin is in top-left corner in the file so the first
  // row in the file is the top row of the image. This class always places the origin in the
  // bottom left so in this case we want to read the last row in the file first to reorder the 
  // in-memory pixels. If the bitmap height is positive then we can simply read the first row
  // in the file first, as this will be the first row in the in-memory bitmap.
  
  int rowSize_bytes = std::ceil((infoHead._bitsPerPixel * infoHead._bmpWidth_px) / 32.f) * 4.f;  
  int pixelSize_bytes = infoHead._bitsPerPixel / 8;

  int numRows = std::abs(infoHead._bmpHeight_px);
  bool isTopOrigin = (infoHead._bmpHeight_px < 0);
  int pixelOffset_bytes = static_cast<int>(fileHead._pixelOffset_bytes);
  int rowOffset_bytes {rowSize_bytes};
  if(isTopOrigin){
    pixelOffset_bytes += (numRows - 1) * rowSize_bytes;
    rowOffset_bytes *= -1;
  }

  // shift values are needed when using channel masks to extract color channel data from
  // the raw pixel bytes.
  int redShift {0};
  int greenShift {0};
  int blueShift {0};
  int alphaShift {0};

  while((infoHead._redMask & (0x01 << redShift)) == 0) ++redShift;
  while((infoHead._greenMask & (0x01 << greenShift)) == 0) ++greenShift;
  while((infoHead._blueMask & (0x01 << blueShift)) == 0) ++blueShift;
  if(infoHead._alphaMask)
    while((infoHead._alphaMask & (0x01 << alphaShift)) == 0) ++alphaShift;

  _pixels.reserve(infoHead._bmpWidth_px * numRows);

  int seekPos {pixelOffset_bytes};
  char* row = new char[rowSize_bytes];

  // for each row of pixels.
  for(int i = 0; i < numRows; ++i){
    file.seekg(seekPos);
    file.read(static_cast<char*>(row), rowSize_bytes);

    // for each pixel.
    for(int j = 0; j < infoHead._bmpWidth_px; ++j){
      uint32_t rawPixelBytes {0};

      // for each pixel byte.
      for(int k = 0; k < pixelSize_bytes; ++k){
        uint8_t pixelByte = row[(j * pixelSize_bytes) + k];

        // 0rth byte of pixel stored in LSB of rawPixelBytes.
        rawPixelBytes |= static_cast<uint32_t>(pixelByte << (k * 8));
      }

      uint8_t red = (rawPixelBytes & infoHead._redMask) >> redShift;
      uint8_t green = (rawPixelBytes & infoHead._greenMask) >> greenShift;
      uint8_t blue = (rawPixelBytes & infoHead._blueMask) >> blueShift;
      uint8_t alpha = (rawPixelBytes & infoHead._alphaMask) >> alphaShift;

      _pixels.push_back(Color4{red, green, blue, alpha});
    }
    seekPos += rowOffset_bytes;
  }
  delete[] row;
}


