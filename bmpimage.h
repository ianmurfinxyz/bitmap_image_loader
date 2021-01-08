#ifndef _BMPIMAGE_H_
#define _BMPIMAGE_H_

#include <fstream>

#include "color.h"

struct BitmapFileHeader;
struct BitmapInfoHeader;

class BitmapImage
{
public:
  BitmapImage() = default;
  ~BitmapImage() = default;
  BitmapImage(const BitmapImage&) = default;
  BitmapImage(BitmapImage&&) = default;
  BitmapImage& operator=(const BitmapImage&) = default;
  BitmapImage& operator=(BitmapImage&&) = default;

  int load(std::string filename);

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

#endif
