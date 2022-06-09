//
// Created by wyz on 2022/6/3.
//

#ifndef TRACER_MISC_HPP
#define TRACER_MISC_HPP

#include "common.hpp"

TRACER_BEGIN

    template<typename T>
    class ScopedAssignment{
    public:
        ScopedAssignment(T* target = nullptr, T value = T())
        :target(target)
        {
            if(target){
                backup = *target;
                *target = value;
            }
        }
        ~ScopedAssignment(){
            if(target) *target = backup;
        }

        ScopedAssignment(const ScopedAssignment&) = delete;
        ScopedAssignment& operator=(const ScopedAssignment&) = delete;

        ScopedAssignment &operator=(ScopedAssignment &&other) {
            if (target) *target = backup;
            target = other.target;
            backup = other.backup;
            other.target = nullptr;
            return *this;
        }

    private:
        T* target, backup;
    };

    using TempAssign = ScopedAssignment<real>;


TRACER_END

#endif //TRACER_MISC_HPP
