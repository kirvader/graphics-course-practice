#pragma once

#include <cmath>
#include <tuple>
#include <vector>
#include <cmath>

class FloatFunctionXYT {
public:
    FloatFunctionXYT() = default;

    float get(float x, float y, float t) const {
        return std::cos(x * t) + std::sin(y * t);
    }
};