//
// Created by wyz on 2022/6/17.
//
#include "display.hpp"

TRACER_BEGIN

class IPCChannel{
public:
};

namespace {
    enum DisplayDirective:uint8_t {
        OpenImage = 0,
        ReloadImage = 1,
        CloseImage = 2,
        UpdateImage = 3,
        CreateImage = 4,
    };


}



TRACER_END