//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_MEDIUM_HPP
#define TRACER_MEDIUM_HPP

#include "common.hpp"

TRACER_BEGIN

class Medium{
public:

};

struct MediumInterface{
    RC<const Medium> inside;
    RC<const Medium> outside;
};

TRACER_END

#endif //TRACER_MEDIUM_HPP
