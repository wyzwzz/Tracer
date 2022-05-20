//
// Created by wyz on 2022/5/19.
//

#ifndef TRACER_FACTORY_PRIMIVITE_HPP
#define TRACER_FACTORY_PRIMIVITE_HPP

#include "core/primitive.hpp"

TRACER_BEGIN

    RC<Primitive> create_geometric_primitive(
            const RC<Shape>& shape,const RC<Material>& material,
            const MediumInterface& mi);


TRACER_END

#endif //TRACER_FACTORY_PRIMIVITE_HPP
