#include "d3dx9.h"
#include <cmath>
#include <utility>

void D3DXMatrixPerspectiveFovLH(D3DXMATRIXA16* m, float fov, float aspect,
    float nearZ, float farZ)
{
    *m = {};
    m->_22 = 1 / std::tan(fov / 2);
    m->_11 = m->_22 / aspect;
    m->_33 = farZ / (farZ - nearZ);
    m->_34 = 1;
    m->_43 = -nearZ * m->_33;
    m->_44 = 0;
}

void D3DXVec3Normalize(D3DXVECTOR3* out, const D3DXVECTOR3* in)
{
    const float length = std::sqrt(in->x*in->x + in->y*in->y + in->z*in->z);
    *out = length ? D3DXVECTOR3{in->x/length, in->y/length, in->z/length} : D3DXVECTOR3{};
}

D3DXMATRIXA16* D3DXMatrixInverse(D3DXMATRIXA16* out, float* determinant,
    const D3DXMATRIXA16* in)
{
    double rows[4][8]{};
    double det = 1;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j)
            rows[i][j] = in->m[i][j];
        rows[i][i + 4] = 1;
    }
    for (int i = 0; i < 4; ++i) {
        int pivot = i;
        for (int j = i + 1; j < 4; ++j)
            if (std::abs(rows[j][i]) > std::abs(rows[pivot][i]))
                pivot = j;
        if (rows[pivot][i] == 0) {
            if (determinant) *determinant = 0;
            return nullptr;
        }
        if (pivot != i) {
            std::swap(rows[i], rows[pivot]);
            det = -det;
        }
        const double divisor = rows[i][i];
        det *= divisor;
        for (double& value : rows[i])
            value /= divisor;
        for (int j = 0; j < 4; ++j) {
            if (j == i) continue;
            const double factor = rows[j][i];
            for (int k = 0; k < 8; ++k)
                rows[j][k] -= factor * rows[i][k];
        }
    }
    if (determinant) *determinant = static_cast<float>(det);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            out->m[i][j] = static_cast<float>(rows[i][j + 4]);
    return out;
}

D3DXVECTOR3* D3DXVec3TransformCoord(D3DXVECTOR3* out,
    const D3DXVECTOR3* in, const D3DXMATRIXA16* matrix)
{
    float point[4];
    for (int i = 0; i < 4; ++i)
        point[i] = in->x * matrix->m[0][i] + in->y * matrix->m[1][i] +
                   in->z * matrix->m[2][i] + matrix->m[3][i];
    *out = {point[0] / point[3], point[1] / point[3], point[2] / point[3]};
    return out;
}

D3DXPLANE* D3DXPlaneFromPoints(D3DXPLANE* out, const D3DXVECTOR3* a,
    const D3DXVECTOR3* b, const D3DXVECTOR3* c)
{
    const D3DXVECTOR3 u{b->x-a->x, b->y-a->y, b->z-a->z};
    const D3DXVECTOR3 v{c->x-a->x, c->y-a->y, c->z-a->z};
    const D3DXVECTOR3 n{u.y*v.z-u.z*v.y, u.z*v.x-u.x*v.z, u.x*v.y-u.y*v.x};
    const float length = std::sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
    if (length == 0)
        *out = {};
    else
        *out = {n.x/length, n.y/length, n.z/length,
                -(n.x*a->x + n.y*a->y + n.z*a->z)/length};
    return out;
}

D3DXVECTOR3* D3DXPlaneIntersectLine(D3DXVECTOR3* out, const D3DXPLANE* plane,
    const D3DXVECTOR3* a, const D3DXVECTOR3* b)
{
    const D3DXVECTOR3 direction{b->x-a->x, b->y-a->y, b->z-a->z};
    const auto dot = [&](const D3DXVECTOR3& v) {
        return plane->a*v.x + plane->b*v.y + plane->c*v.z;
    };
    const float divisor = dot(direction);
    if (divisor == 0) return nullptr;
    const float t = -(dot(*a) + plane->d) / divisor;
    *out = {a->x + t*direction.x, a->y + t*direction.y, a->z + t*direction.z};
    return out;
}
