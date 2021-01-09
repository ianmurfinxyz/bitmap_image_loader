#ifndef _BMP_IMAGE_H_
#define _BMP_IMAGE_H_

//----------------------------------------------------------------------------------------------//
// FILE: bmpimage.h                                                                             //
// AUTHOR: Ian Murfin - github.com/ianmurfinxyz                                                 //
//----------------------------------------------------------------------------------------------//

#include <fstream>
#include "color4.h"

class BmpImage
{
public:
  int load(std::string filename);
  const std::vector<Color4>& getPixels() const {return _pixels;}
  int getWidth() const {return _width_px;}
  int getHeight() const {return _height_px;}

private:
  static constexpr uint32_t BMPMAGIC {0x4D42};
  static constexpr uint32_t SRGBMAGIC {0x73524742};

  static constexpr uint32_t FILEHEADER_SIZE_BYTES {14};
  static constexpr uint32_t V1INFOHEADER_SIZE_BYTES {40};
  static constexpr uint32_t V2INFOHEADER_SIZE_BYTES {52};
  static constexpr uint32_t V3INFOHEADER_SIZE_BYTES {56};
  static constexpr uint32_t V4INFOHEADER_SIZE_BYTES {108};
  static constexpr uint32_t V5INFOHEADER_SIZE_BYTES {124};

  enum Compression
  {
    BI_RGB = 0, 
    BI_RLE8, 
    BI_RLE4, 
    BI_BITFIELDS, 
    BI_JPEG, 
    BI_PNG, 
    BI_ALPHABITFIELDS, 
    BI_CMYK = 11,
    BI_CMYKRLE8, 
    BI_CMYKRLE4
  };

  struct FileHeader
  {
    uint16_t _fileMagic;
    uint32_t _fileSize_bytes;
    uint16_t _reserved0;
    uint16_t _reserved1;
    uint32_t _pixelOffset_bytes;
  };
  
  struct InfoHeader
  {
    uint32_t _headerSize_bytes;
    int32_t  _bmpWidth_px;
    int32_t  _bmpHeight_px;
    uint16_t _numColorPlanes;
    uint16_t _bitsPerPixel; 
    uint32_t _compression; 
    uint32_t _imageSize_bytes;
    int32_t  _xResolution_pxPm;
    int32_t  _yResolution_pxPm;
    uint32_t _numPaletteColors;
    uint32_t _numImportantColors;
    uint32_t _redMask;
    uint32_t _greenMask;
    uint32_t _blueMask;
    uint32_t _alphaMask;
    uint32_t _colorSpaceMagic;
  };

private:
  void extractIndexedPixels(std::ifstream& file, FileHeader& fileHead, InfoHeader& infoHead);
  void extractPixels(std::ifstream& file, FileHeader& fileHead, InfoHeader& infoHead);

private:
  std::vector<Color4> _pixels;
  int _width_px;
  int _height_px;
};

#endif
