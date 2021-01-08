//----------------------------------------------------------------------------------------------//
//                  _                                                                           //
//                 | |                                                                          //
//  ___ _ __   __ _| | _____     __                                                             //
// / __| '_ \ / _` | |/ / _ \   {OO}                                                            //
// \__ \ | | | (_| |   <  __/   \__/                                                            //
// |___/_| |_|\__,_|_|\_\___|   |^|                                                             //
//  ____________________________/ /                                                             //
// /  ___________________________/                                                              //
//  \_______ \                                                                                  //
//          \|                                                                                  //
//                                                                                              //
// FILE: snake.cpp                                                                              //
// AUTHOR: Ian Murfin - github.com/ianmurfinxyz                                                 //
//                                                                                              //
// CREATED: 2nd Jan 2021                                                                        //
// UPDATED: 4th Jan 2021                                                                        //
//                                                                                              //
//----------------------------------------------------------------------------------------------//

#include <chrono>
#include <thread>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <cstring>
#include <sstream>
#include <cassert>
#include <cmath>
#include <array>
#include <vector>
#include <memory>
#include <fstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

namespace sk
{

//------------------------------------------------------------------------------------------------
//  MATH                                                                                          
//------------------------------------------------------------------------------------------------

struct Vector2i
{
  constexpr Vector2i() : _x{0}, _y{0} {}
  constexpr Vector2i(int32_t x, int32_t y) : _x{x}, _y{y} {}

  Vector2i(const Vector2i&) = default;
  Vector2i(Vector2i&&) = default;
  Vector2i& operator=(const Vector2i&) = default;
  Vector2i& operator=(Vector2i&&) = default;

  void zero() {_x = _y = 0;}
  bool isZero() {return _x == 0 && _y == 0;}
  Vector2i operator+(const Vector2i& v) const {return Vector2i{_x + v._x, _y + v._y};}
  void operator+=(const Vector2i& v) {_x += v._x; _y += v._y;}
  Vector2i operator-(const Vector2i& v) const {return Vector2i{_x - v._x, _y - v._y};}
  void operator-=(const Vector2i& v) {_x -= v._x; _y -= v._y;}
  Vector2i operator*(float scale) const {return Vector2i(_x * scale, _y * scale);}
  void operator*=(float scale) {_x *= scale; _y *= scale;}
  void operator*=(int32_t scale) {_x *= scale; _y *= scale;}
  float dot(const Vector2i& v) {return (_x * v._x) + (_y * v._y);}
  float cross(const Vector2i& v) const {return (_x * v._y) - (_y * v._x);}
  float length() const {return std::hypot(_x, _y);}
  float lengthSquared() const {return (_x * _x) + (_y * _y);}
  inline Vector2i normalized() const;
  inline void normalize();

  int32_t _x;
  int32_t _y;
};

Vector2i Vector2i::normalized() const
{
  Vector2i v = *this;
  v.normalize();
  return v;
}

void Vector2i::normalize()
{
  float l = (_x * _x) + (_y * _y);
  if(l) {
    l = std::sqrt(l);
    _x /= l;
    _y /= l;
  }
}

struct iRect
{
  int32_t _x;
  int32_t _y;
  int32_t _w;
  int32_t _h;
};

//------------------------------------------------------------------------------------------------
//  LOG                                                                                           
//------------------------------------------------------------------------------------------------

namespace logstr
{
  constexpr const char* fail_open_log = "failed to open log";
  constexpr const char* fail_sdl_init = "failed to initialize SDL";
  constexpr const char* fail_create_opengl_context = "failed to create opengl context";
  constexpr const char* fail_set_opengl_attribute = "failed to set opengl attribute";
  constexpr const char* fail_create_window = "failed to create window";

  constexpr const char* info_stderr_log = "logging to standard error";
  constexpr const char* info_creating_window = "creating window";
  constexpr const char* info_created_window = "window created";
  constexpr const char* using_opengl_version = "using opengl version";
}; 

class Log
{
public:
  enum Level { FATAL, ERROR, WARN, INFO };
public:
  Log();
  ~Log();
  Log(const Log&) = delete;
  Log(Log&&) = delete;
  Log& operator=(const Log&) = delete;
  Log& operator=(Log&&) = delete;
  void log(Level level, const char* error, const std::string& addendum = std::string{});
private:
  static constexpr const char* filename {"log"};
  static constexpr const char* delim {" : "};
  static constexpr std::array<const char*, 4> lvlstr {"fatal", "error", "warning", "info"};
private:
  std::ofstream _os;
};

Log::Log()
{
  _os.open(filename, std::ios_base::trunc);
  if(!_os){
    log(ERROR, logstr::fail_open_log);
    log(INFO, logstr::info_stderr_log);
  }
}

Log::~Log()
{
  if(_os)
    _os.close();
}

void Log::log(Level level, const char* error, const std::string& addendum)
{
  std::ostream& o {_os ? _os : std::cerr}; 
  o << lvlstr[level] << delim << error;
  if(!addendum.empty())
    o << delim << addendum;
  o << std::endl;
}

std::unique_ptr<Log> log {nullptr};

//------------------------------------------------------------------------------------------------
//  INPUT                                                                                       
//------------------------------------------------------------------------------------------------

class Input
{
public:
  enum KeyCode { 
    KEY_a, KEY_b, KEY_c, KEY_d, KEY_e, KEY_f, KEY_g, KEY_h, KEY_i, KEY_j, KEY_k, KEY_l, KEY_m, 
    KEY_n, KEY_o, KEY_p, KEY_q, KEY_r, KEY_s, KEY_t, KEY_u, KEY_v, KEY_w, KEY_x, KEY_y, KEY_z,
    KEY_SPACE, KEY_BACKSPACE, KEY_ENTER, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_COUNT 
  };
  struct KeyLog
  {
    bool _isDown;
    bool _isPressed;
    bool _isReleased;
  };
public:
  Input();
  ~Input() = default;
  void onKeyEvent(const SDL_Event& event);
  void onUpdate();
  bool isKeyDown(KeyCode key) {return _keys[key]._isDown;}
  bool isKeyPressed(KeyCode key) {return _keys[key]._isPressed;}
  bool isKeyReleased(KeyCode key) {return _keys[key]._isReleased;}
private:
  KeyCode convertSdlKeyCode(int sdlCode);
private:
  std::array<KeyLog, KEY_COUNT> _keys;
};

extern std::unique_ptr<Input> input;

Input::Input()
{
  for(auto& key : _keys)
    key._isDown = key._isReleased = key._isPressed = false;
}

void Input::onKeyEvent(const SDL_Event& event)
{
  assert(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP);

  KeyCode key = convertSdlKeyCode(event.key.keysym.sym);

  if(key == KEY_COUNT) 
    return;

  if(event.type == SDL_KEYDOWN){
    _keys[key]._isDown = true;
    _keys[key]._isPressed = true;
  }
  else{
    _keys[key]._isDown = false;
    _keys[key]._isReleased = true;
  }
}

void Input::onUpdate()
{
  for(auto& key : _keys)
    key._isPressed = key._isReleased = false;
}

Input::KeyCode Input::convertSdlKeyCode(int sdlCode)
{
  switch(sdlCode) {
    case SDLK_a: return KEY_a;
    case SDLK_b: return KEY_b;
    case SDLK_c: return KEY_c;
    case SDLK_d: return KEY_d;
    case SDLK_e: return KEY_e;
    case SDLK_f: return KEY_f;
    case SDLK_g: return KEY_g;
    case SDLK_h: return KEY_h;
    case SDLK_i: return KEY_i;
    case SDLK_j: return KEY_j;
    case SDLK_k: return KEY_k;
    case SDLK_l: return KEY_l;
    case SDLK_m: return KEY_m;
    case SDLK_n: return KEY_n;
    case SDLK_o: return KEY_o;
    case SDLK_p: return KEY_p;
    case SDLK_q: return KEY_q;
    case SDLK_r: return KEY_r;
    case SDLK_s: return KEY_s;
    case SDLK_t: return KEY_t;
    case SDLK_u: return KEY_u;
    case SDLK_v: return KEY_v;
    case SDLK_w: return KEY_w;
    case SDLK_x: return KEY_x;
    case SDLK_y: return KEY_y;
    case SDLK_z: return KEY_z;
    case SDLK_SPACE: return KEY_SPACE;
    case SDLK_BACKSPACE: return KEY_BACKSPACE;
    case SDLK_RETURN: return KEY_ENTER;
    case SDLK_LEFT: return KEY_LEFT;
    case SDLK_RIGHT: return KEY_RIGHT;
    case SDLK_DOWN: return KEY_DOWN;
    case SDLK_UP: return KEY_UP;
    default: return KEY_COUNT;
  }
}

std::unique_ptr<Input> input {nullptr};

//------------------------------------------------------------------------------------------------
//  GFX                                                                                           
//------------------------------------------------------------------------------------------------

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

namespace colors
{
constexpr Color4 white {255, 255, 255};
constexpr Color4 black {0, 0, 0};
constexpr Color4 red {255, 0, 0};
constexpr Color4 green {0, 255, 0};
constexpr Color4 blue {0, 0, 255};
constexpr Color4 cyan {0, 255, 255};
constexpr Color4 magenta {255, 0, 255};
constexpr Color4 yellow {255, 255, 0};

// greys - more grays: https://en.wikipedia.org/wiki/Shades_of_gray 

constexpr Color4 gainsboro {224, 224, 224};
//constexpr Color4 lightgray {0.844f, 0.844f, 0.844f};
//constexpr Color4 silver {0.768f, 0.768f, 0.768f};
//constexpr Color4 mediumgray {0.76f, 0.76f, 0.76f};
//constexpr Color4 spanishgray {0.608f, 0.608f, 0.608f};
//constexpr Color4 gray {0.512f, 0.512f, 0.512f};
//constexpr Color4 dimgray {0.42f, 0.42f, 0.42f};
//constexpr Color4 davysgray {0.34f, 0.34f, 0.34f};
constexpr Color4 jet {53, 53, 53};
};

class Renderer
{
public:
  struct Config
  {
    std::string _windowTitle;
    int32_t _windowWidth;
    int32_t _windowHeight;
  };
public:
  Renderer(const Config& config);
  Renderer(const Renderer&) = delete;
  Renderer* operator=(const Renderer&) = delete;
  ~Renderer();
  void setViewport(iRect viewport);
  void clearWindow(const Color4& color);
  void clearViewport(const Color4& color);
  void drawPixelArray(int first, int count, void* pixels, int pixelSize);
  void show();
  Vector2i getWindowSize() const;
private:
  static constexpr int openglVersionMajor = 2;
  static constexpr int openglVersionMinor = 1;
private:
  SDL_Window* _window;
  SDL_GLContext _glContext;
  Config _config;
  iRect _viewport;
};

Renderer::Renderer(const Config& config)
{
  _config = config;

  std::stringstream ss {};
  ss << "{w:" << _config._windowWidth << ",h:" << _config._windowHeight << "}";
  sk::log->log(Log::INFO, logstr::info_creating_window, std::string{ss.str()});

  _window = SDL_CreateWindow(
      _config._windowTitle.c_str(), 
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      _config._windowWidth,
      _config._windowHeight,
      SDL_WINDOW_OPENGL
  );

  if(_window == nullptr){
    sk::log->log(Log::FATAL, logstr::fail_create_window, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  Vector2i windowSize = getWindowSize();
  std::stringstream().swap(ss);
  ss << "{w:" << windowSize._x << ",h:" << windowSize._y << "}";
  sk::log->log(Log::INFO, logstr::info_created_window, std::string{ss.str()});

  _glContext = SDL_GL_CreateContext(_window);
  if(_glContext == nullptr){
    sk::log->log(Log::FATAL, logstr::fail_create_opengl_context, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, openglVersionMajor) < 0){
    sk::log->log(Log::FATAL, logstr::fail_set_opengl_attribute, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }
  if(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, openglVersionMinor) < 0){
    sk::log->log(Log::FATAL, logstr::fail_set_opengl_attribute, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  sk::log->log(Log::INFO, logstr::using_opengl_version, std::string{reinterpret_cast<const char*>(glGetString(GL_VERSION))});

  setViewport(iRect{0, 0, _config._windowWidth, _config._windowHeight});
}

Renderer::~Renderer()
{
  SDL_GL_DeleteContext(_glContext);
  SDL_DestroyWindow(_window);
}

void Renderer::setViewport(iRect viewport)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, viewport._w, 0.0, viewport._h, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glViewport(viewport._x, viewport._y, viewport._w, viewport._h);
  _viewport = viewport;
}

void Renderer::clearWindow(const Color4& color)
{
  glClearColor(color.getfRed(), color.getfGreen(), color.getfBlue(), color.getfAlpha());
  glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::clearViewport(const Color4& color)
{
  glEnable(GL_SCISSOR_TEST);
  glScissor(_viewport._x, _viewport._y, _viewport._w, _viewport._h);
  glClearColor(color.getfRed(), color.getfGreen(), color.getfBlue(), color.getfAlpha());
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
}

void Renderer::drawPixelArray(int first, int count, void* pixels, int pixelSize)
{
  glInterleavedArrays(GL_C4UB_V2F, 0, pixels);
  glPointSize(pixelSize);
  glDrawArrays(GL_POINTS, first, count);
}

void Renderer::show()
{
  SDL_GL_SwapWindow(_window);
}

Vector2i Renderer::getWindowSize() const
{
  int w, h;
  SDL_GL_GetDrawableSize(_window, &w, &h);
  return Vector2i{w, h};
}

std::unique_ptr<Renderer> renderer {nullptr};

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
  uint32_t _unused0[12];                  // color space info (e.g. gamma); not used.

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
  void extractColorPalette(std::ifstream& file, BitmapInfoHeader& header, std::vector<Color4>& palette);
  void extractPalettedPixels(std::ifstream& file, BitmapFileHeader& fileHeader, BitmapInfoHeader& infoHeader);
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
      extractPalettedPixels(file, fileHeader, infoHeader);
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
  file.read(bytes, readSize);
  header._redMask = extractLittleEndianUint32(bytes);
  header._greenMask = extractLittleEndianUint32(bytes + 4);
  header._blueMask = extractLittleEndianUint32(bytes + 8);
}

void Image::extractColorPalette(std::ifstream& file, BitmapInfoHeader& header, 
                                std::vector<Color4>& palette)
{
  char bytes[4];
  file.seekg(BitmapFileHeader::size_bytes + header._headerSize_bytes);
  for(uint8_t i = 0; i < header._numPaletteColors; ++i){
    file.read(bytes, 4);

    // colors expected in the byte order blue (0), green (1), red (2), alpha (3).
    uint8_t red = static_cast<uint8_t>(bytes[2]);
    uint8_t green = static_cast<uint8_t>(bytes[1]);
    uint8_t blue = static_cast<uint8_t>(bytes[0]);
    uint8_t alpha = static_cast<uint8_t>(bytes[3]);

    palette.push_back(Color4{red, green, blue, alpha});
  }
}

void Image::extractPalettedPixels(std::ifstream& file, BitmapFileHeader& fileHeader, 
                                  BitmapInfoHeader& infoHeader)
{
  // note: this function handles 1-bit, 2-bit, 4-bit and 8-bit pixels.

  // FORMAT OF INDICES IN A BYTE
  //
  // For pixels of 8-bits or less, the pixel data consists of indices into a color palette. The
  // indices are either 1-bit, 2-bit, 4-bit or 8-bit values and are packed into the bytes of a
  // row such that, for example, a bitmap with 2-bit indices, will have 4 indices in each byte
  // of a row.
  //
  // Consider an 8x1 [width, height] bitmap with 2-bit indices permitting 2^2=4 colors in the
  // palette illustrated as:
  //
  //            p0 p1 p2 p3 p4 p5 p6 p7        pN == pixel number in the row
  //           +--+--+--+--+--+--+--+--+
  //           |I0|I1|I0|I2|I0|I3|I0|I1|       IN == index N into color palette
  //           +--+--+--+--+--+--+--+--+
  //                [8x1 bitmap]
  //
  // Since this bitmap uses 2-bits per index, 4 indices (so 4 pixels) can be packed into a 
  // single byte. The specific format for how these indices are packed is such that the 
  // left-most pixel in the row is stored in the most-significant bits of the byte which can 
  // be illustrated as:
  //
  //                 p0 p1 p2 p3
  //              0b 00 01 00 10     <-- the 0rth byte in the bottom row (the only row).
  //                 ^  ^  ^  ^
  //                 |  |  |  |
  //                 I0 I1 I0 I2
  //
  // The bottom row will actually consist of 4 bytes in total. We will have 2 bytes for the 
  // pixels since the row has 8 pixels and we can pack 4 indices (1 for each pixel) into a 
  // single byte, and we will have 2 bytes of padding since rows must be 4-byte aligned in the
  // bitmap file. Thus our full row bytes will read as:
  //
  //                [byte0]          [byte1]         [byte2]          [byte3]
  //               p0 p1 p2 p3      p4 p5 p6 p7
  //          | 0b 00 01 00 10 | 0b 00 11 00 01 { 0b 00 00 00 00 | 0b 00 00 00 00 }
  //               ^  ^  ^  ^       ^  ^  ^  ^
  //               |  |  |  |       |  |  |  |               [padding]
  //               I0 I1 I0 I2      I0 I3 I0 I1
  //              
  // note that although the pixels are stored from left-to-right, the bits in the indices are
  // still read from right-to-left, i.e. decimal 2 = 0b10 and not 0b01.

  std::vector<Color4> palette {};
  extractColorPalette(file, infoHeader, palette);

  int rowSize_bytes = std::ceil((infoHeader._bitsPerPixel * infoHeader._bmpWidth_px) / 32.f) * 4.f;  
  int numPixelsPerByte = 8 / infoHeader._bitsPerPixel;

  uint8_t mask {0};
  for(int i = 0; i < infoHeader._bitsPerPixel; ++i)
    mask |= (0x01 << i);

  int numRows = std::abs(infoHeader._bmpHeight_px);
  bool isTopOrigin = (infoHeader._bmpHeight_px < 0);
  int pixelOffset_bytes = static_cast<int>(fileHeader._pixelOffset_bytes);
  int rowOffset_bytes {rowSize_bytes};
  if(isTopOrigin){
    pixelOffset_bytes += (numRows - 1) * rowSize_bytes;
    rowOffset_bytes *= -1;
  }

  _pixels.reserve(infoHeader._bmpWidth_px * numRows);

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
    while(numPixelsExtracted < infoHeader._bmpWidth_px){
      if(bytePixelNo >= numPixelsPerByte){
        byte = static_cast<uint8_t>(row[++byteNo]);
        bytePixelNo = 0;
      }
      int shift = infoHeader._bitsPerPixel * (numPixelsPerByte - 1 - bytePixelNo);
      uint8_t index = (byte & (mask << shift)) >> shift;
      _pixels.push_back(palette[index]);
      ++numPixelsExtracted;
      ++bytePixelNo;
    }
    seekPos += rowOffset_bytes;
  }
  delete[] row;
}

void Image::extractPixels(std::ifstream& file, BitmapFileHeader& fileHeader, 
                                BitmapInfoHeader& infoHeader)
{
  // note: this function handles 16-bit, 24-bit and 32-bit pixels.

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


// A sprite represents a color image that can be drawn on a virtual screen. Pixels on the sprite
// are positioned on a coordinate space mapped as shown below.
//
//         row
//          ^
//          |
//          |
//   origin o----> col
//
class Sprite
{
public:
  Sprite();
  Sprite(std::vector<Color4> pixels, int width, int height);
  ~Sprite() = default;
  Sprite(const Sprite&) = default;
  Sprite(Sprite&&) = default;
  Sprite& operator=(const Sprite&) = default;
  Sprite& operator=(Sprite&&) = default;
  void setPixel(int row, int col, const Color4& color);
  const std::vector<Color4>& getPixels() const {return _pixels;}
  int getWidth() const {return _width;}
  int getHeight() const {return _height;}
private:
  std::vector<Color4> _pixels;
  int _width;
  int _height;
};

Sprite::Sprite() :
  _pixels{},
  _width{0},
  _height{0}
{}

Sprite::Sprite(std::vector<Color4> pixels, int width, int height) : 
  _width{width},
  _height{height},
  _pixels{pixels}
{}

void Sprite::setPixel(int row, int col, const Color4& color)
{
  _pixels[col + (row * _width)] = color;
}

// A virtual screen with fixed resolution independent of display resolution and window size. The
// screen is positioned centrally in the window with the ratio of virtual pixel size to real
// pixel size being calculated to fit the window dimensions.
//
// Pixels on the screen are arranged on a coordinate system with the origin in the bottom left
// most corner, rows ascending north and columns ascending east as shown below.
//
//      row
//       ^
//       |
//       |
//   pos o----> col
//
// note: virtual pixel sizes are limited to integer mulitiples of real pixels, i.e. integers.
//
class Screen
{
public:
  Screen(Vector2i windowSize);
  ~Screen() = default;
  void clear(const Color4& color);
  void clear(iRect region, const Color4& color);
  void drawPixel(int row, int col, const Color4& color);
  void drawSprite(int x, int y, const Sprite& sprite);
  void rescalePixels(Vector2i windowSize);
  void render();
private:
  // 12 byte pixels designed to work with glInterleavedArrays format GL_C4UB_V2F.
  struct Pixel
  {
    Color4 _color;
    float _x;
    float _y;
  };
private:
  static constexpr int screenWidth = 160;
  static constexpr int screenHeight = 160;
  static constexpr int pixelCount = screenWidth * screenHeight;
private:
  Vector2i _position;
  std::array<Pixel, pixelCount> _pixels; // flattened 2D array accessed (col + (row * width))
  int _pixelSize;
};

Screen::Screen(Vector2i windowSize)
{
  rescalePixels(windowSize);
}

void Screen::clear(const Color4& color)
{
  for(auto& pixel : _pixels)
    pixel._color = color;
}

void Screen::clear(iRect region, const Color4& color)
{

} 

void Screen::drawPixel(int row, int col, const Color4& color)
{
  assert(0 <= row && row < screenHeight);
  assert(0 <= col && col < screenWidth);
  _pixels[col + (row * screenWidth)]._color = color;
}

void Screen::drawSprite(int x, int y, const Sprite& sprite)
{
  assert(x >= 0 && y >= 0);

  const std::vector<Color4>& spritePixels {sprite.getPixels()};
  int spriteWidth {sprite.getWidth()};
  int spriteHeight {sprite.getHeight()};
  
  int spritePixelNum {0};
  for(int spriteRow = 0; spriteRow < spriteHeight; ++spriteRow){
    // if next row is above the screen.
    if(y + spriteRow >= screenHeight)
      break;

    // index of 1st screen pixel in next row being drawn.
    int screenRowIndex {x + ((y + spriteRow) * screenWidth)};   

    for(int spriteCol = 0; spriteCol < spriteWidth; ++spriteCol){
      // if the right side of the sprite falls outside the screen.
      if(x + spriteCol >= screenWidth)
        break;

      _pixels[screenRowIndex + spriteCol]._color = spritePixels[spritePixelNum];  
      ++spritePixelNum;
    }
  }
}

void Screen::rescalePixels(Vector2i windowSize)
{
  int pixelWidth = windowSize._x / screenWidth; 
  int pixelHeight = windowSize._y / screenHeight;
  _pixelSize = std::min(pixelWidth, pixelHeight);
  if(_pixelSize == 0)
    _pixelSize = 1;
  int pixelCenterOffset = _pixelSize / 2;
  _position._x = std::clamp((windowSize._x - (_pixelSize * screenWidth)) / 2, 0, windowSize._x);
  _position._y = std::clamp((windowSize._y - (_pixelSize * screenHeight)) / 2, 0, windowSize._y);
  for(int col = 0; col < screenWidth; ++col){
    for(int row = 0; row < screenHeight; ++row){
      int index = col + (row * screenWidth);
      Pixel& pixel = _pixels[index];
      pixel._x = _position._x + (col * _pixelSize) + pixelCenterOffset;
      pixel._y = _position._y + (row * _pixelSize) + pixelCenterOffset;
    }
  }
}

void Screen::render()
{
  auto now0 = std::chrono::high_resolution_clock::now();
  sk::renderer->drawPixelArray(0, pixelCount, static_cast<void*>(_pixels.data()), _pixelSize);
  auto now1 = std::chrono::high_resolution_clock::now();
  std::cout << "Screen::render execution time (us): "
            << std::chrono::duration_cast<std::chrono::microseconds>(now1 - now0).count()
            << std::endl;
}

std::unique_ptr<Screen> screen {nullptr};

//------------------------------------------------------------------------------------------------
//  SNAKE                                                                                         
//------------------------------------------------------------------------------------------------

//class Snake
//{
//public:
//  enum MoveDirection { NORTH, SOUTH, EAST, WEST };
//  enum MoveAxis { HORIZONTAL = 0, VERTICAL = 1 };
//  enum MoveMagnitude { NEGATIVE = -1, POSITIVE = 1 };
//
//  enum SegmentType { 
//    BODY_EW, BODY_WE, BODY_NS, BODY_SN, CORNER_EN, CORNER_NE, CORNER_WN, CORNER_NW, CORNER_ES, 
//    CORNER_SE, CORNER_WS, CORNER_SW, HEAD_E, HEAD_W, HEAD_N, HEAD_S, TAIL_E, TAIL_W, TAIL_N, 
//    TAIL_S 
//  };
//
//  struct Segment
//  {
//    Vector2i _position;         // [x:col, y:row] w.r.t block world.
//    MoveDirection _direction;   // segment's local direction of movement (seperate to the snake).
//    SegmentType _type;          // used to draw the snake correctly (to select sprites).
//  };
//
//public:
//  Snake(Vector2i worldDimensions, Vector2i);
//  ~Snake() = default;
//  void update(float dt);
//  void setMoveDirection(MoveDirection direction);
//
//private:
//  static constexpr float moveFrequency {2.f};             // unit: move (block) jumps per second.
//  static constexpr float movePeriod {1 / moveFrequency};
//  static constexpr int appetite {1};                      // nuggets eaten until full (until grow).
//
//private:
//  void doMove();
//  void calculateSegmentMoveDirection(Segment& segment, Vector2i oldPosition, Vector2i newPosition);
//  void recalculateSegmentTypes();
//
//private:
//  std::vector<Segment> _segments;
//  Vector2i _worldDimensions;
//  MoveAxis _moveAxis;
//  MoveMagnitude _moveMagnitude;
//  float _moveClock;
//  int _nuggetsEaten;
//};
//
//void Snake::update(float dt)
//{
//  _moveClock += dt;
//  if(_moveClock > movePeriod){
//    _moveClock = 0;
//    doMove();
//  }
//}
//
//void Snake::doMove()
//{
//  // move head segment.
//  Segment& head = _segments[0];
//  Vector2i oldPosition = head._position;
//  head._position._x += (_moveAxis == HORIZONTAL) ? _moveMagnitude : 0;
//  head._position._y += (_moveAxis == VERTICAL) ? _moveMagnitude : 0;
//  calculateSegmentMoveDirection(head, oldPosition, head._position); 
//
//  // wrap head around world if exceed world bounds.
//  if(head._position._x < 0)
//    head._position._x = _worldDimensions._x - 1;
//  else if(head._position._x >= _worldDimensions._x - 1)
//    head._position._x = 0;
//  else if(head._position._y < 0)
//    head._position._y = _worldDimensions._y - 1;
//  else if(head._position._y >= _worldDimensions._y - 1)
//    head._position._y = 0;
//
//  // shift body segments.
//  for(int i = 1; i < _segments.size(); ++i){
//    Segment& segment = _segments[i];
//    Vector2i temp = segment._position;
//    segment._position = oldPosition;
//    calculateSegmentMoveDirection(segment, temp, segment._position); 
//    oldPosition = temp;
//  }
//
//  // add a segment if snake is full of nuggets.
//  if(_neggetsEaten == appetite){
//    Segment newTail {};
//    newTail._position = oldPosition;
//
//    // uses the old tail's old position (oldPosition) and the old tail's new position.
//    calculateSegmentMoveDirection(segment, oldPosition, _segments.back()._position); 
//
//    _segments.push_back(newTail);
//  }
//
//  // ensures the drawing reflects the change in snake position.
//  recalculateSegmentTypes();
//}
//
//void Snake::calculateSegmentMoveDirection(Segment& segment, Vector2i oldPosition, Vector2i newPosition)
//{
//  int deltaRow = newPosition._y - oldPosition._y;
//  int deltaCol = newPosition._x - oldPosition._x;
//
//  // Segments (except the head) shift into the position of their neighbour which is closest to 
//  // the head, this pattern of movement will always satisfy the below conditions. Thus if these
//  // conditions are not met then the snake is dancing something funky!
//  assert(-1 >= deltaRow && deltaRow <= 1);
//  assert(-1 >= deltaCol && deltaCol <= 1);
//  assert(deltaRow == 0 || deltaCol == 0);
//  assert(deltaRow != deltaCol);
//
//  MoveDirection direction;
//  if(deltaRow != 0)
//    direction = (deltaRow == 1) ? NORTH : SOUTH;
//  else
//    direction = (deltaRow == 1) ? NORTH : SOUTH;
//
//  segment._moveDirection = direction;
//}
//
//void Snake::recalculateSegmentTypes()
//{
//  // calculate head type.
//  switch(_segments.front()._moveDirection)
//  {
//  case NORTH:
//    _segments.front()._type = HEAD_N;
//    break;
//  case SOUTH:
//    _segments.front()._type = HEAD_S;
//    break;
//  case EAST:
//    _segments.front()._type = HEAD_E;
//    break;
//  case WEST:
//    _segments.front()._type = HEAD_W;
//    break;
//  };
//
//  // calculate tail type.
//  switch(_segments.back()._moveDirection)
//  {
//  case NORTH:
//    _segments.back()._type = TAIL_N;
//    break;
//  case SOUTH:
//    _segments.back()._type = TAIL_S;
//    break;
//  case EAST:
//    _segments.back()._type = TAIL_E;
//    break;
//  case WEST:
//    _segments.back()._type = TAIL_W;
//    break;
//  };
//
//  // calculate types of all body segments.
//  Segment dummy;
//  for(int i = 1; i < _segments.size() - 1; ++i){
//    Segment& thisSegment = _segments[i];
//
//    Vector2i thisPosition = thisSegment._position;
//    Vector2i headNeighbourPosition = _segments[i - 1]._position;
//    Vector2i tailNeighbourPosition = _segments[i + 1]._position;
//
//    MoveDirection directionToHead, directionToTail;
//    calculateSegmentMoveDirection(dummy, thisPosition, headNeighbourPosition);
//    directionToHead = dummy._moveDirection;
//    calculateSegmentMoveDirection(dummy, thisPosition, tailNeighbourPosition);
//    directionToTail = dummy._moveDirection;
//
//    switch(directionToHead)
//    {
//    case NORTH:
//      switch(directionToTail)
//      {
//        case NORTH:
//          assert(0);
//          break;
//        case SOUTH:
//          thisSegment._type = BODY_SN;
//          break;
//        case EAST:
//          thisSegment._type = CORNER_EN;
//          break;
//        case WEST:
//          thisSegment._type = CORNER_WN;
//          break;
//      }
//      break;
//    case SOUTH:
//      switch(directionToTail)
//      {
//        case NORTH:
//          thisSegment._type = BODY_NS;
//          break;
//        case SOUTH:
//          assert(0);
//          break;
//        case EAST:
//          thisSegment._type = CORNER_ES;
//          break;
//        case WEST:
//          thisSegment._type = CORNER_WS;
//          break;
//      }
//      break;
//    case EAST:
//      switch(directionToTail)
//      {
//        case NORTH:
//          thisSegment._type = CORNER_NE;
//          break;
//        case SOUTH:
//          thisSegment._type = CORNER_SE;
//          break;
//        case EAST:
//          assert(0);
//          break;
//        case WEST:
//          thisSegment._type = BODY_WE;
//          break;
//      }
//      break;
//    case WEST:
//      switch(directionToTail)
//      {
//        case NORTH:
//          thisSegment._type = CORNER_NW;
//          break;
//        case SOUTH:
//          thisSegment._type = CORNER_SW;
//          break;
//        case EAST:
//          thisSegment._type = BODY_EW;
//          break;
//        case WEST:
//          assert(0);
//          break;
//      }
//      break;
//    }
//  }
//}
//
//void Snake::setMoveDirection(MoveDirection direction)
//{
//  switch(direction)
//  {
//  case SOUTH:
//    _moveAxis = VERTICAL;
//    _moveMagnitude = NEGATIVE;
//    break;
//  case north:
//    _moveAxis = VERTICAL;
//    _moveMagnitude = POSITIVE;
//    break;
//  case east:
//    _moveAxis = HORIZONTAL;
//    _moveMagnitude = POSITIVE;
//    break;
//  case west:
//    _moveAxis = HORIZONTAL;
//    _moveMagnitude = NEGATIVE;
//    break;
//  }
//}

class Game
{
public:
  enum ColorID { 
    COLOR_WORLD_BACKGROUND,
    COLOR_SNAKE_BODY_LIGHT,
    COLOR_SNAKE_BODY_SHADED,
    COLOR_SNAKE_BODY_SHADOW,
    COLOR_SNAKE_EYES,
    COLOR_SNAKE_TONGUE,
    COLOR_SNAKE_SPOTS
  };
public:
  Game();
  ~Game() = default;
  void draw();
private:
  static constexpr Vector2i worldDimensions {50, 50}; // [x:width(num cols), y:height(num rows)]
private:
  void generateSprites();
private:
  std::vector<Color4> _palette;

  // Sprite assets.
  std::vector<Sprite> _snakeSprites;
};

Game::Game()
{
  _palette.push_back(colors::jet);
  _palette.push_back(Color4(255, 217,  0));
  _palette.push_back(Color4(172, 146,  0));
  _palette.push_back(Color4( 42,  42, 42));
  _palette.push_back(Color4(214,   0,  0));
  _palette.push_back(Color4(214,   0,  0));
  _palette.push_back(Color4(  4,  69,  0));

  generateSprites();
}

void Game::generateSprites()
{
  const std::vector<Color4>& p = _palette;

  _snakeSprites.push_back({{p[3], p[3], p[3], p[3], p[2], p[2], p[2], p[2], p[1], p[1], p[1], 
                            p[1], p[6], p[1], p[1], p[1]}, 4, 4});

  Image image;
  image.loadBmp("20x20_32bitRGBA_colorspace.bmp");
  _snakeSprites.push_back(Sprite{image.getPixels(), image.getWidth(), image.getHeight()});
}

void Game::draw()
{
  sk::screen->clear(colors::gainsboro);
  sk::screen->drawSprite(30, 30, _snakeSprites[0]);
  sk::screen->drawSprite(50, 50, _snakeSprites[1]);
}

//------------------------------------------------------------------------------------------------
//  APP                                                                                           
//------------------------------------------------------------------------------------------------

class App
{
public:
  using Clock_t = std::chrono::steady_clock;
  using TimePoint_t = std::chrono::time_point<Clock_t>;
  using Duration_t = std::chrono::nanoseconds;
private:
  class RealClock
  {
  public:
    RealClock() : _start{}, _now0{}, _now1{}, _dt{}{}
    ~RealClock() = default;
    void start() {_now0 = _start = Clock_t::now();}
    Duration_t update();
    Duration_t getDt() const {return _dt;}
    Duration_t getNow() const {return _now1 - _start;}
  private:
    TimePoint_t _start;
    TimePoint_t _now0;
    TimePoint_t _now1;
    Duration_t _dt;
  };
  class Metronome
  {
  public:
    Metronome(Duration_t appNow, Duration_t tickPeriod_ns);
    ~Metronome() = default;
    int64_t doTicks(Duration_t appNow);
    Duration_t getTickPeriod_ns() const {return _tickPeriod_ns;}
    float getTickPeriod_s() const {return _tickPeriod_s;}
  private:
    Duration_t _lastTickNow;
    Duration_t _tickPeriod_ns;
    float _tickPeriod_s;
    int64_t _totalTicks;
  };
public:
  App();
  ~App();
  App(const App&) = delete;
  App(const App&&) = delete;
  App& operator=(const App&) = delete;
  App& operator=(App&&) = delete;
  void initialize();
  void shutdown();
  void run();
private:
  void loop();
  void onTick(float dt);
private:
  static constexpr const char* name = "snake";
  static constexpr int appVersionMajor = 0;
  static constexpr int appVersionMinor = 1;
  static constexpr int windowWidth_px = 700;
  static constexpr int windowHeight_px = 200;
  static constexpr int maxTicksPerFrame = 5;
  static constexpr Duration_t minFramePeriod {static_cast<int64_t>(0.01e9)};
private:
  RealClock _clock;
  Metronome _metronome;
  int64_t _ticksAccumulated;
  bool _isDone;

  Game _game;
};

App::Duration_t App::RealClock::update()
{
  _now1 = Clock_t::now();
  _dt = _now1 - _now0;
  _now0 = _now1;
  return _dt;
}

App::Metronome::Metronome(App::Duration_t appNow, App::Duration_t tickPeriod_ns) :
  _lastTickNow{appNow},
  _tickPeriod_ns{tickPeriod_ns},
  _tickPeriod_s{static_cast<float>(_tickPeriod_ns.count()) / 1.0e9f},
  _totalTicks{0}
{
}

int64_t App::Metronome::doTicks(Duration_t appNow)
{
  int64_t ticks;
  while(_lastTickNow + _tickPeriod_ns < appNow){
    _lastTickNow += _tickPeriod_ns;
    ++ticks;
  }
  _totalTicks += ticks;
  return ticks;
}

App::App() : 
  _clock{}, 
  _metronome{_clock.getNow(), Duration_t{static_cast<int64_t>(0.016e9)}},
  _ticksAccumulated{0},
  _isDone{false},
  _game{}
{
}

App::~App()
{
}

void App::initialize()
{
  sk::log = std::make_unique<Log>();
  sk::input = std::make_unique<Input>();
  sk::screen = std::make_unique<Screen>(Vector2i{windowWidth_px, windowHeight_px});

  if(SDL_Init(SDL_INIT_VIDEO) < 0){
    sk::log->log(Log::FATAL, logstr::fail_sdl_init, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  std::stringstream ss{};
  ss << name
     << " - version: "
     << appVersionMajor
     << "."
     << appVersionMinor;

  Renderer::Config rconfig {std::string{ss.str()}, windowWidth_px, windowHeight_px};
  renderer = std::make_unique<Renderer>(rconfig);

  Vector2i windowSize = sk::renderer->getWindowSize();
  if(windowSize._x != windowWidth_px || windowSize._y != windowHeight_px)
    sk::screen->rescalePixels(windowSize);
}

void App::shutdown()
{
  sk::log.reset(nullptr);
  sk::input.reset(nullptr);
  sk::renderer.reset(nullptr);
}

void App::run()
{
  while(!_isDone)
    loop();
}

void App::loop()
{
  auto now0 = Clock_t::now();
  auto realDt = _clock.update();
  auto realNow = _clock.getNow();

  SDL_Event event;
  while(SDL_PollEvent(&event) != 0){
    switch(event.type){
      case SDL_QUIT:
        _isDone = true;
        return;
      case SDL_WINDOWEVENT:
        if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
          int w, h;
          w = event.window.data1;
          h = event.window.data2;
          sk::renderer->setViewport(iRect{0, 0, w, h});
          sk::screen->rescalePixels(Vector2i{w, h});
        }
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        sk::input->onKeyEvent(event);
    }
  }

  _ticksAccumulated += _metronome.doTicks(realNow);
  int64_t ticksDoneThisFrame {0};
  while(_ticksAccumulated > 0 && ticksDoneThisFrame < maxTicksPerFrame){
    ++ticksDoneThisFrame;
    --_ticksAccumulated;
    onTick(_metronome.getTickPeriod_s());
  }

  sk::input->onUpdate();

  auto now1 = Clock_t::now();
  auto framePeriod = now1 - now0;
  if(framePeriod < minFramePeriod)
    std::this_thread::sleep_for(minFramePeriod - framePeriod);
}

void App::onTick(float dt)
{
  sk::renderer->clearWindow(colors::jet);
  _game.draw();
  sk::screen->render();
  sk::renderer->show();
}

std::unique_ptr<App> app {nullptr};

}; // namespace sk

//------------------------------------------------------------------------------------------------
//  MAIN                                                                                          
//------------------------------------------------------------------------------------------------

int main()
{
  sk::app = std::make_unique<sk::App>();
  sk::app->initialize();
  sk::app->run();
  sk::app->shutdown();
  sk::app.reset(nullptr);
}

