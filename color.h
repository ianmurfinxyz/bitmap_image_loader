#ifndef _COLOR_H_
#define _COLOR_H_

#include <cinttypes>
#include <algorithm>

class Color4
{
private:
  constexpr static float f_lo {0.f};
  constexpr static float f_hi {1.f};

public:
  Color4() : _r{0}, _g{0}, _b{0}, _a{0}{}

  constexpr Color4(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0) :
    _r{r},
    _g{g},
    _b{b},
    _a{a}
  {}

  Color4(const Color4&) = default;
  Color4(Color4&&) = default;
  Color4& operator=(const Color4&) = default;
  Color4& operator=(Color4&&) = default;

  void setRed(uint8_t r){_r = r;}
  void setGreen(uint8_t g){_g = g;}
  void setBlue(uint8_t b){_b = b;}
  void setAlpha(uint8_t a){_a = a;}
  uint8_t getRed() const {return _r;}
  uint8_t getGreen() const {return _g;}
  uint8_t getBlue() const {return _b;}
  uint8_t getAlpha() const {return _a;}
  float getfRed() const {return std::clamp(_r / 255.f, f_lo, f_hi);}    // clamp to cut-off float math errors.
  float getfGreen() const {return std::clamp(_g / 255.f, f_lo, f_hi);}
  float getfBlue() const {return std::clamp(_b / 255.f, f_lo, f_hi);}
  float getfAlpha() const {return std::clamp(_a / 255.f, f_lo, f_hi);}

private:
  uint8_t _r;
  uint8_t _g;
  uint8_t _b;
  uint8_t _a;
};

#endif
