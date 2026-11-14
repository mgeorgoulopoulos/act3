#pragma once

namespace d3d9 {
// D3D row-major storage, with points multiplied as row vectors.
struct Matrix {
    float m[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
};

inline Matrix operator*(const Matrix& left, const Matrix& right)
{
    Matrix result;
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col) {
            result.m[row][col] = 0;
            for (int k = 0; k < 4; ++k)
                result.m[row][col] += left.m[row][k] * right.m[k][col];
        }
    return result;
}
}
