#include <string>
#include <cstring>
#include <cinttypes>
#include <cmath>
#include <algorithm>
#include <fstream>

#include "bmpimage.h"

static const uint32_t bitmapFileMagic {0x4d42};
static const uint32_t colorSpaceSRGBMagic {0x73524742};

void reverseBytes(char* bytes, int count)
{
  int swaps = ((count % 2) == 1) ? (count - 1) / 2 : count / 2;
  char tmp;
  for(int i = 0; i < swaps; ++i){
    tmp = bytes[i]; 
    bytes[i] = bytes[count - 1 - i];
    bytes[count - i] = tmp;
  }
}

// reminder: Endianess determines the order in which bytes are stored in memory. Consider a 
// 32-bit integer 'n' assigned the hex value 0xa3b2c1d0. Its memory layout on each system can 
// be illustrated as:
//
//    lower addresses --------------------------------------> higher addresses
//            +----+----+----+----+            +----+----+----+----+
//            |0xd0|0xc1|0xb2|0xa3|            |0xa3|0xb2|0xc1|0xd0|
//            +----+----+----+----+            +----+----+----+----+
//            |                                |
//            &x                               &x
//
//              [little-endian]                      [big-endian]
//
//         little-end (LSB) of x at            big-end (MSB) of x at
//         lower address.                      lower address.
//
// Independent of the endianess however &x always returns the byte at the lower address due to
// the way C/C++ implements pointer arithmetic. If this were not the case you could not reliably
// increment a pointer to move through an array or buffer, e.g,
//
//   char buffer[10];
//   char* p = buffer;
//   for(int i{0}; i<10; ++i)
//     cout << (*p)++ << endl;
//
// would be platform dependent if '&' (address-of) operator returned the address of the LSB and
// not (as it does) the lowest address used by the operator target. If the former were true you
// would have to increment the pointer on little-endian systems and decrement it on big-endian
// systems.
//
// Thus this function takes advantage of the platform independence of the address-of operator to
// test for endianess.
bool isSystemLittleEndian()
{
  uint32_t n {0x00000001};                           // LSB == 1.
  uint8_t* p = reinterpret_cast<uint8_t*>(&n);       // pointer to byte at lowest address.
  return (*p);                                       // value of byte at lowest address.
}

// Extracts a type T from a byte buffer containing the bytes of T stored in little-endian 
// format (in the buffer). The endianess of the system is accounted for.

uint16_t extractLittleEndianUint16(char* buffer)
// predicate: buffer size is at least sizeof(uint16_t) bytes.
{
  if(!isSystemLittleEndian())
    reverseBytes(buffer, sizeof(uint16_t));
  return *(reinterpret_cast<uint16_t*>(buffer));
}

uint32_t extractLittleEndianUint32(char* buffer)
// predicate: buffer size is at least sizeof(uint32_t) bytes.
{
  if(!isSystemLittleEndian())
    reverseBytes(buffer, sizeof(uint32_t));
  return *(reinterpret_cast<uint32_t*>(buffer));
}

uint64_t extractLittleEndianUint64(char* buffer)
// predicate: buffer size is at least sizeof(uint64_t) bytes.
{
  if(!isSystemLittleEndian())
    reverseBytes(buffer, sizeof(uint64_t));
  return *(reinterpret_cast<uint64_t*>(buffer));
}

int16_t extractLittleEndianInt16(char* buffer)
{
// predicate: buffer size is at least sizeof(int16_t) bytes.
  if(!isSystemLittleEndian())
    reverseBytes(buffer, sizeof(int16_t));
  return *(reinterpret_cast<int16_t*>(buffer));
}

int32_t extractLittleEndianInt32(char* buffer)
{
// predicate: buffer size is at least sizeof(int32_t) bytes.
  if(!isSystemLittleEndian())
    reverseBytes(buffer, sizeof(int32_t));
  return *(reinterpret_cast<int32_t*>(buffer));
}

int64_t extractLittleEndianInt64(char* buffer)
{
// predicate: buffer size is at least sizeof(int64_t) bytes.
  if(!isSystemLittleEndian())
    reverseBytes(buffer, sizeof(int64_t));
  return *(reinterpret_cast<int64_t*>(buffer));
}

// note: I am choosing NOT to pack these structs for use with reading binary data from a stream;
// struct packing can lead to problems on certain platforms. Read binary data into arrays and
// extract the data manually.

struct BitmapFileHeader
{
  static constexpr int size_bytes {14};

  uint16_t _fileMagic;
  uint32_t _fileSize;
  uint16_t _reserved0;
  uint16_t _reserved1;
  uint32_t _pixelOffset_bytes;
};

// There are multiple versions of the info (DIB) header of BMP files identified by their 
// header size. This module supports: BITMAPCOREHEADER, BITMAPINFOHEADER, BITMAPV2INFOHEADER, 
// BITMAPV3INFOHEADER, BITMAPV4HEADER, BITMAPV5HEADER. It does not support OS2 headers.
//
// note: each version extends the last rather than revising it thus all versions have been
// combined here into a single version (effectively the latest).
//
// note: the version BITMAPINFOHEADER is apparantly the commonly used version by software
// seeking to maintain backwards compatibility.
//
// For more information on the bitmap file format see references.
//
// references:
// [0] https://en.wikipedia.org/wiki/BMP_file_format
// [1] https://solarianprogrammer.com/2018/11/19/cpp-reading-writing-bmp-images/
// [2] https://medium.com/sysf/bits-to-bitmaps-a-simple-walkthrough-of-bmp-image-format-765dc6857393

struct BitmapInfoHeader
{
  // -- BITMAPCOREHEADER --
  
  static constexpr int BITMAPCOREHEADER_SIZE_BYTES {12};
  
  uint32_t _headerSize_bytes;
  int32_t _bmpWidth_px;
  int32_t _bmpHeight_px;
  uint16_t _numColorPlanes;
  uint16_t _bitsPerPixel; 

  // -- added BITMAPINFOHEADER --

  static constexpr int BITMAPINFOHEADER_SIZE_BYTES {40};

  uint32_t _compression; 
  uint32_t _rawImageSize_bytes;
  int32_t _horizontalResolution_pxPm;
  int32_t _verticalResolution_pxPm;
  uint32_t _numPaletteColors;
  uint32_t _numImportantColors;

  // -- added BITMAPV2INFOHEADER --

  static constexpr int BITMAPV2INFOHEADER_SIZE_BYTES {52};
  
  uint32_t _redMask;
  uint32_t _greenMask;
  uint32_t _blueMask;

  // -- added BITMAPV3INFOHEADER --
  
  static constexpr int BITMAPV3INFOHEADER_SIZE_BYTES {56};

  uint32_t _alphaMask;

  // -- added BITMAPV4HEADER --

  static constexpr int BITMAPV4HEADER_SIZE_BYTES {108};

  uint32_t _colorSpaceMagic;
  uint32_t _unused0[12];

  // -- added BITMAPV5HEADER --
  
  static constexpr int BITMAPV5HEADER_SIZE_BYTES {124};

  uint32_t _unused1[4];
};

enum BitmapInfoHeaderOffsets
{
  BIHO_HEADER_SIZE = 0,
  BIHO_BMP_WIDTH = 4,
  BIHO_BMP_HEIGHT = 8,
  BIHO_NUM_COLOR_PLANES = 12,
  BIHO_BITS_PER_PIXEL = 14,
  BIHO_COMPRESSION = 16,
  BIHO_RAW_IMAGE_SIZE = 20,
  BIHO_HORIZONTAL_RESOLUTION = 24,
  BIHO_VERTICAL_RESOLUTION = 28,
  BIHO_NUM_PALETTE_COLORS = 32,
  BIHO_NUM_IMPORTANT_COLORS = 36,
  BIHO_RED_MASK = 40,
  BIHO_GREEN_MASK = 44,
  BIHO_BLUE_MASK = 48,
  BIHO_ALPHA_MASK = 52,
  BIHO_COLOR_SPACE_MAGIC = 56
};

// note: most of these compression formats are not supported by this loader. Only BI_RGB (no
// compression) and BI_BITFIELDS (bit field masks) are supported, RLE (run-length-encoding)
// modes are not supported.
enum BitmapFileCompressionMode
{
  BI_RGB = 0, BI_RLE8, BI_RLE4, BI_BITFIELDS, BI_JPEG, BI_PNG, BI_ALPHABITFIELDS, BI_CMYK = 11,
  BI_CMYKRLE8, BI_CMYKRLE4
};

class Image
{
public:
  int loadBmp(std::string filename);
  const std::vector<Color4>& getPixels() const {return _pixels;}
  int getWidth() const {return _width_px;}
  int getHeight() const {return _height_px;}
private:
  void extractFileHeader(std::ifstream& file, BitmapFileHeader& header);
  void extractInfoHeader(std::ifstream& file, BitmapInfoHeader& header);
  void extractAppendedRGBMasks(std::ifstream& file, BitmapInfoHeader& header);
  void extractPalettedPixels(std::ifstream& file, BitmapInfoHeader& header);
  void extractPixels(std::ifstream& file, BitmapFileHeader& fileHeader, BitmapInfoHeader& infoHeader);
private:
  std::vector<Color4> _pixels;
  int _width_px;
  int _height_px;
};

int Image::loadBmp(std::string filename)
{
  std::ifstream file {filename, std::ios_base::binary};
  if(!file){
    return -1;
  }

  BitmapFileHeader fileHeader {};
  extractFileHeader(file, fileHeader);
  if(fileHeader._fileMagic != bitmapFileMagic){
    return -1;
  }

  BitmapInfoHeader infoHeader {};
  extractInfoHeader(file, infoHeader);

  // other compression modes are not supported.
  if(infoHeader._compression != BI_RGB && infoHeader._compression != BI_BITFIELDS){
    return -1;
  }

  bool V2 {false}, V3_4_5 {false};
  switch(infoHeader._headerSize_bytes)
  {
  case BitmapInfoHeader::BITMAPV5HEADER_SIZE_BYTES:

    // fallthrough
    
  case BitmapInfoHeader::BITMAPV4HEADER_SIZE_BYTES:
    // other color spaces not supported.
    if(infoHeader._colorSpaceMagic != colorSpaceSRGBMagic){
      return -1;
    }

    // fallthrough
    
  case BitmapInfoHeader::BITMAPV3INFOHEADER_SIZE_BYTES:
    V3_4_5 = true;

    // fallthrough
    

  case BitmapInfoHeader::BITMAPV2INFOHEADER_SIZE_BYTES:
    if(!V3_4_5)
      V2 = true;

    // fallthrough
    
  case BitmapInfoHeader::BITMAPINFOHEADER_SIZE_BYTES:
    if(infoHeader._bitsPerPixel <= 8){
      extractPalettedPixels(file, infoHeader);
    }
    else if(infoHeader._bitsPerPixel == 16){
      if(infoHeader._compression == BI_RGB){
        // default masks for this scenario.
        infoHeader._redMask   = 0b00000000000000000111110000000000;
        infoHeader._greenMask = 0b00000000000000000000001111100000;
        infoHeader._blueMask  = 0b00000000000000000000000000011111;

        // BITMAPV3INFOHEADER, BITMAPV4HEADER and BITMAPV5HEADER all use set defaults for RGB 
        // masks but a custom alpha mask, thus we dont want to override this custom mask if 
        // any of those info headers are present.
        if(!V3_4_5)
          infoHeader._alphaMask = 0b00000000000000001000000000000000;

        extractPixels(file, fileHeader, infoHeader); 
      }
      if(infoHeader._compression == BI_BITFIELDS){
        // with BI_BITFIELDS and BITMAPINFOHEADER the masks are found appended to the end of 
        // the info header in an extra block of 12 bytes rather than in the header itself.
        if(!V2 && !V3_4_5)
          extractAppendedRGBMasks(file, infoHeader);

        extractPixels(file, fileHeader, infoHeader); 
      }
    }
    else if(infoHeader._bitsPerPixel == 24){
      // default masks for this scenario.
      infoHeader._redMask   = 0b00000000111111110000000000000000;
      infoHeader._greenMask = 0b00000000000000001111111100000000;
      infoHeader._blueMask  = 0b00000000000000000000000011111111;
      infoHeader._alphaMask = 0b00000000000000000000000000000000;
      extractPixels(file, fileHeader, infoHeader); 
    }
    else if(infoHeader._bitsPerPixel == 32){
      if(infoHeader._compression == BI_RGB){
        // default masks for this scenario.
        infoHeader._redMask   = 0b00000000111111110000000000000000;
        infoHeader._greenMask = 0b00000000000000001111111100000000;
        infoHeader._blueMask  = 0b00000000000000000000000011111111;

        // BITMAPV3INFOHEADER, BITMAPV4HEADER and BITMAPV5HEADER all use set defaults for RGB 
        // masks but a custom alpha mask, thus we dont want to override this custom mask if 
        // any of those info headers are present.
        if(!V3_4_5)
          infoHeader._alphaMask = 0b11111111000000000000000000000000;

        extractPixels(file, fileHeader, infoHeader); 
      }
      else if(infoHeader._compression == BI_BITFIELDS){
        // with BI_BITFIELDS and BITMAPINFOHEADER the masks are found appended to the end of 
        // the info header in an extra block of 12 bytes rather than in the header itself.
        if(!V2 && !V3_4_5)
          extractAppendedRGBMasks(file, infoHeader);

        extractPixels(file, fileHeader, infoHeader); 
      }
    }
  }

  _width_px = infoHeader._bmpWidth_px;
  _height_px = infoHeader._bmpHeight_px;

  return 0;
}

void Image::extractFileHeader(std::ifstream& file, BitmapFileHeader& header)
{
  char bytes[BitmapFileHeader::size_bytes];
  file.seekg(0);
  file.read(static_cast<char*>(bytes), BitmapFileHeader::size_bytes);
  header._fileMagic = extractLittleEndianUint16(bytes);
  header._fileSize = extractLittleEndianUint32(bytes + 2);
  header._pixelOffset_bytes = extractLittleEndianUint32(bytes + 10);
}

void Image::extractInfoHeader(std::ifstream& file, BitmapInfoHeader& header)
{
  memset(&header, 0, sizeof(BitmapInfoHeader));

  char bytes[BitmapInfoHeader::BITMAPINFOHEADER_SIZE_BYTES];

  // start by reading the header size to determine the info header version present.
  file.seekg(BitmapFileHeader::size_bytes);
  file.read(static_cast<char*>(bytes), sizeof(BitmapInfoHeader::_headerSize_bytes)); 
  header._headerSize_bytes = extractLittleEndianUint32(bytes);

  int seekPos, readSize;
  switch(header._headerSize_bytes)
  {
  case BitmapInfoHeader::BITMAPV5HEADER_SIZE_BYTES:
  case BitmapInfoHeader::BITMAPV4HEADER_SIZE_BYTES:
    seekPos = BitmapFileHeader::size_bytes + BIHO_COLOR_SPACE_MAGIC;
    readSize = sizeof(BitmapInfoHeader::_colorSpaceMagic);
    file.seekg(seekPos);
    file.read(bytes, readSize);
    header._colorSpaceMagic = extractLittleEndianUint32(bytes);

    // fallthrough

  case BitmapInfoHeader::BITMAPV3INFOHEADER_SIZE_BYTES:
    seekPos = BitmapFileHeader::size_bytes + BIHO_ALPHA_MASK;
    readSize = sizeof(BitmapInfoHeader::_alphaMask);
    file.seekg(seekPos);
    file.read(bytes, readSize);
    header._alphaMask = extractLittleEndianUint32(bytes);

    // fallthrough
    
  case BitmapInfoHeader::BITMAPV2INFOHEADER_SIZE_BYTES:
    // although the masks are actually part of the V2 header they are in the same file position
    // as the masks appended to the BITMAPINFOHEADER.
    extractAppendedRGBMasks(file, header);

    // fallthrough
    
  case BitmapInfoHeader::BITMAPINFOHEADER_SIZE_BYTES:
    seekPos = BitmapFileHeader::size_bytes;
    readSize = BitmapInfoHeader::BITMAPINFOHEADER_SIZE_BYTES;
    file.seekg(seekPos);
    file.read(bytes, readSize);
    header._bmpWidth_px = extractLittleEndianInt32(bytes + BIHO_BMP_WIDTH);
    header._bmpHeight_px = extractLittleEndianInt32(bytes + BIHO_BMP_HEIGHT);
    header._numColorPlanes = extractLittleEndianUint16(bytes + BIHO_NUM_COLOR_PLANES);
    header._bitsPerPixel = extractLittleEndianUint16(bytes + BIHO_BITS_PER_PIXEL);
    header._compression = extractLittleEndianUint32(bytes + BIHO_COMPRESSION);
    header._rawImageSize_bytes = extractLittleEndianUint32(bytes + BIHO_RAW_IMAGE_SIZE);
    header._horizontalResolution_pxPm = extractLittleEndianInt32(bytes + BIHO_HORIZONTAL_RESOLUTION);
    header._verticalResolution_pxPm = extractLittleEndianInt32(bytes + BIHO_VERTICAL_RESOLUTION);
    header._numPaletteColors = extractLittleEndianUint32(bytes + BIHO_NUM_PALETTE_COLORS);
    header._numImportantColors = extractLittleEndianUint32(bytes + BIHO_NUM_IMPORTANT_COLORS);
  }
}

void Image::extractAppendedRGBMasks(std::ifstream& file, BitmapInfoHeader& header)
{
  char bytes[12];
  int seekPos = BitmapFileHeader::size_bytes + BIHO_RED_MASK;
  int readSize = BIHO_ALPHA_MASK - BIHO_RED_MASK;
  file.seekg(seekPos);
  file.read(static_cast<char*>(bytes), readSize);
  header._redMask = extractLittleEndianUint32(bytes);
  header._greenMask = extractLittleEndianUint32(bytes + 4);
  header._blueMask = extractLittleEndianUint32(bytes + 8);
}

void Image::extractPalettedPixels(std::ifstream& file, BitmapInfoHeader& header)
{

}

void Image::extractPixels(std::ifstream& file, BitmapFileHeader& fileHeader, 
                                BitmapInfoHeader& infoHeader)
{
  // If bitmap height is negative the origin is in top-left corner in the file so the first
  // row in the file is the top row of the image. This class always places the origin in the
  // bottom left so in this case we want to read the last row in the file first to reorder the 
  // in-memory pixels. If the bitmap height is positive then we can simply read the first row
  // in the file first, as this will be the first row in the in-memory bitmap.
  
  int rowSize_bytes = std::ceil((infoHeader._bitsPerPixel * infoHeader._bmpWidth_px) / 32.f) * 4.f;  
  int pixelSize_bytes = infoHeader._bitsPerPixel / 8;

  int numRows = std::abs(infoHeader._bmpHeight_px);
  bool isTopOrigin = (infoHeader._bmpHeight_px < 0);
  int pixelOffset_bytes = static_cast<int>(fileHeader._pixelOffset_bytes);
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

  while((infoHeader._redMask & (0x01 << redShift)) == 0) ++redShift;
  while((infoHeader._greenMask & (0x01 << greenShift)) == 0) ++greenShift;
  while((infoHeader._blueMask & (0x01 << blueShift)) == 0) ++blueShift;
  if(infoHeader._alphaMask)
    while((infoHeader._alphaMask & (0x01 << alphaShift)) == 0) ++alphaShift;

  _pixels.reserve(infoHeader._bmpWidth_px * numRows);

  int seekPos {pixelOffset_bytes};
  char* row = new char[rowSize_bytes];

  // for each row of pixels.
  for(int i = 0; i < numRows; ++i){
    file.seekg(seekPos);
    file.read(static_cast<char*>(row), rowSize_bytes);

    // for each pixel.
    for(int j = 0; j < infoHeader._bmpWidth_px; ++j){
      uint32_t rawPixelBytes {0};

      // for each pixel byte.
      for(int k = 0; k < pixelSize_bytes; ++k){
        uint8_t pixelByte = row[(j * pixelSize_bytes) + k];

        // 0rth byte of pixel stored in LSB of rawPixelBytes.
        rawPixelBytes |= static_cast<uint32_t>(pixelByte << (k * 8));
      }

      uint8_t red = (rawPixelBytes & infoHeader._redMask) >> redShift;
      uint8_t green = (rawPixelBytes & infoHeader._greenMask) >> greenShift;
      uint8_t blue = (rawPixelBytes & infoHeader._blueMask) >> blueShift;
      uint8_t alpha = (rawPixelBytes & infoHeader._alphaMask) >> alphaShift;

      _pixels.push_back(Color4{red, green, blue, alpha});
    }
    seekPos += rowOffset_bytes;
  }
  delete[] row;
}

