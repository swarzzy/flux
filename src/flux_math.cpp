template <typename T, u32 _Size>
union Vector {};

template<typename T>
union Vector<T, 2> {
    static constexpr u32 Size = 2;
    struct {
        T x, y;
    };
    struct {
        T r, g;
    };
    struct {
        T u, v;
    };
    T data[Size];
};

template<typename T>
union Vector<T, 3> {
    static constexpr u32 Size = 3;
    struct {
        T x, y, z;
    };
    struct {
        T r, g, b;
    };
    struct {
        Vector<T, 2> xy;
        T _unused;
    };
    T data[Size];
};

template<typename T>
union Vector<T, 4> {
    static constexpr u32 Size = 4;
    struct {
        T x, y, z, w;
    };
    struct {
        T r, g, b, a;
    };
    struct {
        Vector<T, 2> xy;
        T _unused1[2];
    };
    struct {
        Vector<T, 3> xyz;
        T _unused2;
    };
    T data[Size];
};

typedef Vector<f32, 2> v2;
typedef Vector<f32, 3> v3;
typedef Vector<f32, 4> v4;

typedef Vector<u32, 2> uv2;
typedef Vector<u32, 3> uv3;
typedef Vector<u32, 4> uv4;

typedef Vector<i32, 2> iv2;
typedef Vector<i32, 3> iv3;
typedef Vector<i32, 4> iv4;

template<u32 Size>
union SquareMatrix {};

template<>
union SquareMatrix<3> {
    static const u32 Size = 3;
    v3 columns[Size];
    f32 data[Size * Size];
    struct {
        f32 _11, _21, _31;
        f32 _12, _22, _32;
        f32 _13, _23, _33;
    };

    f32& At(u32 i, u32 j) {
        return this->data[i + Size * j];
    }
};

template<>
union SquareMatrix<4> {
    static const u32 Size = 4;
    v4 columns[Size];
    f32 data[Size * Size];
    struct
    {
        f32 _11, _21, _31, _41;
        f32 _12, _22, _32, _42;
        f32 _13, _23, _33, _43;
        f32 _14, _24, _34, _44;
    };

    f32& At(u32 i, u32 j) {
        return this->data[i + Size * j];
    }
};

typedef SquareMatrix<4> m4x4;
typedef SquareMatrix<3> m3x3;

v2 V2(f32 x, f32 y) { return v2{x, y}; }
v2 V2(f32 val) { return v2{val, val}; }
v2 V2(v3 v) { return v2{v.x, v.y}; }
v2 V2(v4 v) { return v2{v.x, v.y}; }

v3 V3(f32 x, f32 y, f32 z) { return v3{x, y, z}; }
v3 V3(f32 val) { return v3{val, val, val}; }
v3 V3(v2 v, f32 z) { return v3{v.x, v.y, z}; }
v3 V3(v4 v) { return v3{v.x, v.y, v.z}; }

v4 V4(f32 x, f32 y, f32 z, f32 w) { return v4{x, y ,z, w}; }
v4 V4(f32 val) { return v4{val, val , val, val}; }
v4 V4(v2 v, f32 z, f32 w) { return v4{v.x, v.y ,z, w}; }
v4 V4(v3 v, f32 w) { return v4{v.x, v.y ,v.z, w}; }

uv2 UV2(u32 x, u32 y) { return uv2{x, y }; }
uv2 UV2(u32 val) { return uv2{val, val}; }
uv2 UV2(uv3 v) { return uv2{v.x, v.y}; }
uv2 UV2(uv4 v) { return uv2{v.x, v.y}; }

uv3 UV3(u32 x, u32 y, u32 z) { return uv3{x, y, z}; }
uv3 UV3(u32 val) { return uv3{val, val, val}; }
uv3 UV3(uv2 v, u32 z) { return uv3{v.x, v.y, z}; }
uv3 UV3(uv4 v) { return uv3{v.x, v.y, v.z}; }

uv4 UV4(u32 x, u32 y, u32 z, u32 w) { return uv4{x, y, z, w}; }
uv4 UV4(u32 val) { return uv4{val, val , val, val}; }
uv4 UV4(uv2 v, u32 z, u32 w) { return uv4{v.x, v.y ,z, w}; }
uv4 UV4(uv3 v, u32 w) { return uv4{v.x, v.y ,v.z, w}; }

iv2 IV2(i32 x, i32 y) { return iv2{x, y}; }
iv2 IV2(i32 val) { return iv2{val, val}; }
iv2 IV2(iv3 v) { return iv2{v.x, v.y}; }
iv2 IV2(iv4 v) { return iv2{v.x, v.y}; }

iv3 IV3(i32 x, i32 y, i32 z) { return iv3{x, y, z}; }
iv3 IV3(i32 val) { return iv3{val, val, val}; }
iv3 IV3(iv2 v, i32 z) { return iv3{v.x, v.y, z}; }
iv3 IV3(iv4 v) { return iv3{v.x, v.y, v.z}; }

iv4 IV4(i32 x, i32 y, i32 z, i32 w) { return iv4{x, y, z, w}; }
iv4 IV4(i32 val) { return iv4{val, val, val, val}; }
iv4 IV4(iv2 v, i32 z, i32 w) { return iv4{v.x, v.y ,z, w}; }
iv4 IV4(iv3 v, i32 w) { return iv4{v.x, v.y ,v.z, w}; }

m3x3 M3x3(f32 diag) { m3x3 result = {}; result._11 = diag; result._22 = diag; result._33 = diag; return result; }
m4x4 M4x4(f32 diag) { m4x4 result = {};  result._11 = diag; result._22 = diag; result._33 = diag; result._44 = diag; return result; }
m3x3 M3x3(m4x4 m) { m3x3 result; result._11 = m._11; result._12 = m._12; result._13 = m._13; result._21 = m._21; result._22 = m._22; result._23 = m._23; result._31 = m._31; result._32 = m._32; result._33 = m._33; return result; }
m4x4 M4x4(m3x3 m) { m4x4 result = {}; result._11 = m._11; result._12 = m._12; result._13 = m._13; result._21 = m._21; result._22 = m._22; result._23 = m._23; result._31 = m._31; result._32 = m._32; result._33 = m._33; result._44 = 1.0f; return result; }

template <typename T, u32 Size>
Vector<T, Size> operator+(Vector<T, Size> l, Vector<T, Size> r) {
    Vector<T, Size> result;
    for (u32x i = 0; i < Size; i++) {
        result.data[i] = l.data[i] + r.data[i];
    }
    return result;
}

template <typename T, u32 Size>
Vector<T, Size> operator+(Vector<T, Size> v, f32 s) {
    Vector<T, Size> result;
    for (u32x i = 0; i < Size; i++) {
        result.data[i] = v.data[i] + s;
    }
    return result;
}

template <typename T, u32 Size>
Vector<T, Size> operator+(f32 s, Vector<T, Size> v) {
    Vector<T, Size> result;
    for (u32x i = 0; i < Size; i++) {
        result.data[i] = v.data[i] + s;
    }
    return result;
}

template <typename T, u32 Size>
Vector<T, Size>& operator+=(Vector<T, Size>& l, Vector<T, Size> r) {
    for (u32x i = 0; i < Size; i++) {
        l.data[i] += r.data[i];
    }
    return l;
}

template <typename T, u32 Size>
Vector<T, Size>& operator+=(Vector<T, Size>& v, f32 s) {
    for (u32x i = 0; i < Size; i++) {
        v.data[i] += s;
    }
    return l;
}

template <typename T, u32 Size>
Vector<T, Size> operator-(Vector<T, Size> l, Vector<T, Size> r) {
    Vector<T, Size> result;
    for (u32x i = 0; i < Size; i++) {
        result.data[i] = l.data[i] - r.data[i];
    }
    return result;
}

template <typename T, u32 Size>
Vector<T, Size> operator-(Vector<T, Size> v, f32 s) {
    Vector<T, Size> result;
    for (u32x i = 0; i < Size; i++) {
        result.data[i] = v.data[i] - s;
    }
    return result;
}

template <typename T, u32 Size>
Vector<T, Size> operator-(f32 s, Vector<T, Size> v) {
    Vector<T, Size> result;
    for (u32x i = 0; i < Size; i++) {
        result.data[i] = v.data[i] - s;
    }
    return result;
}

template <typename T, u32 Size>
Vector<T, Size>& operator-=(Vector<T, Size>& l, Vector<T, Size> r) {
    for (u32x i = 0; i < Size; i++) {
        l.data[i] -= r.data[i];
    }
    return l;
}

template <typename T, u32 Size>
Vector<T, Size>& operator-=(Vector<T, Size>& v, f32 s) {
    for (u32x i = 0; i < Size; i++) {
        v.data[i] -= s;
    }
    return l;
}

template <typename T, u32 Size>
Vector<T, Size> operator*(Vector<T, Size> v, f32 s) {
    Vector<T, Size> result;
    for (u32x i = 0; i < Size; i++) {
        result.data[i] = v.data[i] * s;
    }
    return result;
}

template <typename T, u32 Size>
Vector<T, Size> operator*(f32 s, Vector<T, Size> v) {
    Vector<T, Size> result;
    for (u32x i = 0; i < Size; i++) {
        result.data[i] = v.data[i] * s;
    }
    return result;
}

template <typename T, u32 Size>
Vector<T, Size>& operator*=(Vector<T, Size>& v, f32 s) {
    for (u32x i = 0; i < Size; i++) {
        v.data[i] *= s;
    }
    return l;
}

template <typename T, u32 Size>
Vector<T, Size> operator/(Vector<T, Size> v, f32 s) {
    Vector<T, Size> result;
    for (u32x i = 0; i < Size; i++) {
        result.data[i] = v.data[i] / s;
    }
    return result;
}

template <typename T, u32 Size>
Vector<T, Size> operator/(f32 s, Vector<T, Size> v) {
    Vector<T, Size> result;
    for (u32x i = 0; i < Size; i++) {
        result.data[i] = v.data[i] / s;
    }
    return result;
}

template <typename T, u32 Size>
Vector<T, Size>& operator/=(Vector<T, Size>& v, f32 s) {
    for (u32x i = 0; i < Size; i++) {
        v.data[i] /= s;
    }
    return l;
}

template <u32 Size>
f32 LengthSq(Vector<f32, Size> v) {
    f32 result = 0.0f;
    for (u32x i = 0; i < Size; i++) {
        result += v.data[i] * v.data[i];
    }
    return result;
}

template <u32 Size>
f32 Length(Vector<f32, Size> v) {
    auto result = Sqrt(LengthSq(v));
    return result;
}

template <u32 Size>
f32 DistSq(Vector<f32, Size> v1, Vector<f32, Size> v2) {
    f32 result = 0.0f;
    for (u32x i = 0; i < Size; i++) {
        result += v2.data[i] - v1.data[i];
    }
    return result;
}

template <u32 Size>
f32 Dist(Vector<f32, Size> v1, Vector<f32, Size> v2) {
    auto result = Sqrt(DistSq(v1, v2));
    return result;
}

template <u32 Size>
Vector<f32, Size> Normalize(Vector<f32, Size> v) {
    Vector<f32, Size> result;
    auto len = Length(v);
    if (len > F32_EPS) {
        for (u32x i = 0; i < Size; i++) {
            result.data[i] = v.data[i] / len;
        }
    }
    return result;
}

template <u32 Size>
Vector<f32, Size> Lerp(Vector<f32, Size> a, Vector<f32, Size> b, f32 t) {
    Vector<f32, Size> result;
    for (u32x i = 0; i < Size; i++) {
        result.data[i] = Lerp(a.data[i], b.data[i], t);
    }
    return result;
}

template <typename T, u32 Size>
Vector<T, Size> Hadamard(Vector<T, Size> a, Vector<T, Size> b) {
    Vector<T, Size> result;
    for (u32x i = 0; i < Size; i++) {
        result.data[i] = a.data[i] * b.data[i];
    }
    return result;
}

template <u32 Size>
f32 Dot(Vector<f32, Size> a, Vector<f32, Size> b) {
    f32 result = 0.0f;
    for (u32x i = 0; i < Size; i++) {
        result += a.data[i] * b.data[i];
    }
    return result;
}

v3 Cross(v3 l, v3 r) {
    auto result = v3 { l.y * r.z - l.z * r.y,
                       l.z * r.x - l.x * r.z,
                       l.x * r.y - l.y * r.x };
    return result;
}

template<u32 Size>
SquareMatrix<Size> operator*(SquareMatrix<Size> left, SquareMatrix<Size> right) {
    SquareMatrix<Size> result = {};
    for (u32x i = 0; i < Size; i++) {
        for (u32x j = 0; j < Size; j++) {
            for (u32x k = 0; k < Size; k++) {
                result.At(i, j) += left.At(i, k) * right.At(k, j);
            }
        }
    }
    return result;
}

template<u32 Size>
Vector<f32, Size> operator*(SquareMatrix<Size> left, Vector<f32, Size> right) {
    Vector<f32, Size> result = {};
    for (u32x i = 0; i < Size; i++) {
        for (u32x j = 0; j < Size; j++) {
            result.data[i] += left.At(i, j) * right.data[j];
        }
    }
    return result;
}

template<u32 Size>
SquareMatrix<Size> Transpose(SquareMatrix<Size> m) {
    SquareMatrix<Size> result;
    for (u32x i = 0; i < Size; i++) {
        for (u32x j = 0; j < Size; j++) {
            result.data[j + Size * i] = m[i + Size * j];
        }
    }
    return result;
}

m4x4 Translation(v3 offset) {
    m4x4 result = {};
    result._11 = 1.0f;
    result._22 = 1.0f;
    result._33 = 1.0f;
    result._44 = 1.0f;

    result._14 = offset.x;
    result._24 = offset.y;
    result._34 = offset.z;

    return result;
}

m4x4 Scaling(v3 scalar) {
    m4x4 result = {};
    result._11 = scalar.x;
    result._22 = scalar.y;
    result._33 = scalar.z;
    result._44 = 1.0f;

    return result;
}

m4x4 OrthoGLRH(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    m4x4 m = {};
    m._11 = 2.0f / (right - left);
    m._22 = 2.0f / (top - bottom);
    m._33 = -2.0f / (far - near);
    m._14 = -(right + left) / (right - left);
    m._24 = -(top + bottom) / (top - bottom);
    m._34 = -(far + near) / (far - near);
    m._44 = 1.0f;
    return m;
}

m4x4 PerspectiveGLRH(f32 near, f32 far, f32 fov, f32 aRatio) {
    m4x4 m = {};
    f32 c = 1.0f / Tan(ToRad(fov) / 2.0f);
    m._11 = c / aRatio;
    m._22 = c;
    m._33 = -(far + near) / (far - near);
    m._34 = -(2.0f * far * near) / (far - near);
    m._43 = -1.0f;
    return m;
}

m4x4 LookAtGLRH(v3 eye, v3 target, v3 up) {
    m4x4 m = {};

    auto z = Normalize(eye - target);
    auto x = Normalize(Cross(up, z));
    auto y = Normalize(Cross(z, x));

    m._11 = x.x;
    m._12 = x.y;
    m._13 = x.z;
    m._14 = -Dot(x, eye);

    m._21 = y.x;
    m._22 = y.y;
    m._23 = y.z;
    m._24 = -Dot(y, eye);

    m._31 = z.x;
    m._32 = z.y;
    m._33 = z.z;
    m._34 = -Dot(z, eye);

    m._44 = 1.0f;

    return m;
}
