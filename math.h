#pragma once

#include "common.h"

#include <emmintrin.h>
#include <cmath>
#include <ostream>


#define M_PI 3.14159265359f
#define M_PI_OVER_180 (M_PI / 180.f)
#define M_180_OVER_PI (180.f / M_PI)

#define degreesToRadians(deg) ((deg) * M_PI_OVER_180)
#define radiansToDegrees(rad) ((rad) * M_180_OVER_PI)

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

static inline float clamp(float t, float lower, float upper)
{
	return max(lower, min(t, upper));
}


struct vec2
{
	union
	{
		struct
		{
			float x;
			float y;
		};

		float xy[2];
	};

	inline vec2();
	inline vec2(float v);
	inline vec2(float x, float y);
};

struct vec3
{
	union {
		struct
		{
			float x;
			float y;
			float z;
		};

		float xyz[3];
	};

	inline vec3();
	inline vec3(float v);
	inline vec3(float x, float y, float z);
	inline vec3(const vec2& v2, float z);
};

struct vec4
{
	union
	{
		struct
		{
			float x;
			float y;
			float z;
			float w;
		};

		struct
		{
			vec3 xyz;
			float w;
		};

		__m128 data;

		float xyzw[4];
	};

	inline vec4();
	inline vec4(float v);
	inline vec4(float x, float y, float z, float w);
	inline vec4(const __m128& data);
	inline vec4(const vec3& v3, float w);
};

struct quat
{
	union
	{
		struct
		{
			float x;
			float y;
			float z;
			float w;
		};

		__m128 data;

		struct
		{
			vec4 v4;
		};

		struct
		{
			vec3 v;
			float cosHalfAngle;
		};
	};

	inline quat();
	inline quat(const vec4& v4, bool norm = true);
	inline quat(const vec3& axis, float angle, bool norm = true);
	inline quat(float x, float y, float z, float w, bool norm = true);
	inline quat(const __m128& data, bool norm = true);
};

struct mat3
{
	union
	{
		struct
		{
			float m00; float m01; float m02;
			float m10; float m11; float m12;
			float m20; float m21; float m22;
		};

		float data[9];
	};

	inline mat3();
};

struct mat4
{
	union
	{
		struct
		{
			float m00; float m01; float m02; float m03;
			float m10; float m11; float m12; float m13;
			float m20; float m21; float m22; float m23;
			float m30; float m31; float m32; float m33;
		};

		float data[16];
	};

	inline mat4();
	inline mat4(const mat3& m3);
	inline mat4(const vec3& v3);
};

struct SQT
{
	quat rotation;
	vec3 position;
	float scale;

	inline SQT();
	inline SQT(const vec3& position, const quat& rotation, float scale = 1.f);
};

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// VEC 2 ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

static inline float length(const vec2& v)
{
	return sqrtf(v.x*v.x + v.y*v.y);
}

static inline float sqlength(const vec2& v)
{
	return v.x*v.x + v.y*v.y;
}

static inline void normalize(vec2& v)
{
	float l = length(v);
	v.x /= l;
	v.y /= l;
}

inline vec2 normalized(const vec2& v)
{
	float l = length(v);
	vec2 result;
	result.x = v.x / l;
	result.y = v.y / l;
	return result;
}

inline float dot(const vec2& a, const vec2& b)
{
	return a.x * b.x + a.y * b.y;
}

static inline vec2 operator+(const vec2& a, const vec2& b)
{
	return vec2(a.x + b.x, a.y + b.y);
}

static inline vec2 operator-(const vec2& a, const vec2& b)
{
	return vec2(a.x - b.x, a.y - b.y);
}

static inline vec2 operator*(const vec2& v, float m)
{
	return vec2(v.x * m, v.y * m);
}

static inline vec2 operator*(float m, const vec2& v)
{
	return v * m;
}

static inline vec2 operator/(const vec2& v, float d)
{
	return vec2(v.x / d, v.y / d);
}

static inline bool operator==(const vec2& a, const vec2& b)
{
	return a.x == b.x && a.y == b.y;
}

static inline bool operator!=(const vec2& a, const vec2& b)
{
	return !(a == b);
}

static inline void operator+=(vec2& a, const vec2& b)
{
	a.x += b.x;
	a.y += b.y;
}

static inline void operator-=(vec2& a, const vec2& b)
{
	a.x -= b.x;
	a.y -= b.y;
}

static inline void operator*=(vec2& v, float m)
{
	v.x *= m;
	v.y *= m;
}

static inline void operator/=(vec2& v, float d)
{
	v.x /= d;
	v.y /= d;
}

static inline vec2 operator-(const vec2& v)
{
	vec2 result;
	result.x = -v.x;
	result.y = -v.y;
	return result;
}

static inline std::ostream& operator<<(std::ostream& s, const vec2& v)
{
	s << "[" << v.x << ", " << v.y << "]";
	return s;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// VEC 3 ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

static inline float length(const vec3& v)
{
	return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

static inline float sqlength(const vec3& v)
{
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

static inline void normalize(vec3& v)
{
	__m128 r = _mm_div_ps(_mm_setr_ps(v.x, v.y, v.z, 0.f), _mm_set_ps1(length(v)));
	float* p = (float*)&r;
	v.x = *p;
	v.y = *(p + 1);
	v.z = *(p + 2);
}

static inline vec3 normalized(const vec3& v)
{
	vec3 result = v;
	normalize(result);
	return result;
}

static inline float dot(const vec3& a, const vec3& b)
{
	__m128 r = _mm_mul_ps(_mm_setr_ps(a.x, a.y, a.z, 0.f), _mm_setr_ps(b.x, b.y, b.z, 0.f));
	float* single = (float*)&r;
	return *single + *(single + 1) + *(single + 2);
}

static vec3 cross(const vec3& a, const vec3& b)
{
	vec3 result;
	__m128 va = _mm_setr_ps(a.x, a.y, a.z, 0.f);
	__m128 vb = _mm_setr_ps(b.x, b.y, b.z, 0.f);
	__m128 r = _mm_sub_ps(
		_mm_mul_ps(_mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 1, 0, 2))),
		_mm_mul_ps(_mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 0, 2, 1)))
		);
	float* p = (float*)&r;
	result.x = *p;
	result.y = *(p + 1);
	result.z = *(p + 2);
	return result;
}

static inline void operator+=(vec3& a, const vec3& b)
{
	__m128 r = _mm_add_ps(_mm_setr_ps(a.x, a.y, a.z, 0.f), _mm_setr_ps(b.x, b.y, b.z, 0.f));
	float* p = (float*)&r;
	a.x = *p;
	a.y = *(p + 1);
	a.z = *(p + 2);
}

static inline void operator-=(vec3& a, const vec3& b)
{
	__m128 r = _mm_sub_ps(_mm_setr_ps(a.x, a.y, a.z, 0.f), _mm_setr_ps(b.x, b.y, b.z, 0.f));
	float* p = (float*)&r;
	a.x = *p;
	a.y = *(p + 1);
	a.z = *(p + 2);
}

static inline void operator*=(vec3& v, float m)
{
	__m128 r = _mm_mul_ps(_mm_setr_ps(v.x, v.y, v.z, 0.f), _mm_set_ps1(m));
	float* p = (float*)&r;
	v.x = *p;
	v.y = *(p + 1);
	v.z = *(p + 2);
}

static inline void operator/=(vec3& v, float d)
{
	__m128 r = _mm_div_ps(_mm_setr_ps(v.x, v.y, v.z, 0.f), _mm_set_ps1(d));
	float* p = (float*)&r;
	v.x = *p;
	v.y = *(p + 1);
	v.z = *(p + 2);
}

static inline vec3 operator+(const vec3& a, const vec3& b)
{
	vec3 result = a;
	result += b;
	return result;
}

static inline vec3 operator-(const vec3& a, const vec3& b)
{
	vec3 result = a;
	result -= b;
	return result;
}

static inline vec3 operator*(const vec3& v, float m)
{
	vec3 result = v;
	result *= m;
	return result;
}

static inline vec3 operator*(float m, const vec3& v)
{
	return v * m;
}

static inline vec3 operator/(const vec3& v, float d)
{
	vec3 result = v;
	result /= d;
	return result;
}

static inline bool operator==(const vec3& a, const vec3& b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

static inline bool operator!=(const vec3& a, const vec3& b)
{
	return !(a == b);
}

static inline vec3 operator-(const vec3& v)
{
	vec3 result;
	result.x = -v.x;
	result.y = -v.y;
	result.z = -v.z;
	return result;
}

static inline vec3 rotateAroundAxis(const vec3& v, float angle, const vec3& axis)
{
	float cosAngle = cosf(angle);
	return v * cosAngle + (dot(v, axis) * axis * (1.f - cosAngle)) + (cross(axis, v) * sinf(angle));
}

static inline std::ostream& operator<<(std::ostream& s, const vec3& v)
{
	s << "[" << v.x << ", " << v.y << ", " << v.z << "]";
	return s;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// VEC 4 ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

static inline float length(const vec4& v)
{
	return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
}

static inline float sqlength(const vec4& v)
{
	return v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w;
}

static inline void normalize(vec4& v)
{
	__m128 divisor = _mm_set_ps1(length(v));
	v.data = _mm_div_ps(v.data, divisor);
}

static inline vec4 normalized(const vec4& v)
{
	vec4 result = v;
	normalize(result);
	return result;
}

static inline float dot(const vec4& a, const vec4& b)
{
	__m128 r = _mm_mul_ps(a.data, b.data);
	float* single = (float*)&r;
	return *single + *(single + 1) + *(single + 2) + *(single + 3);
}

static inline void operator+=(vec4& a, const vec4& b)
{
	a.data = _mm_add_ps(a.data, b.data);
}

static inline void operator-=(vec4& a, const vec4& b)
{
	a.data = _mm_sub_ps(a.data, b.data);
}

static inline void operator*=(vec4& v, float m)
{
	v.data = _mm_mul_ps(v.data, _mm_set_ps1(m));
}

static inline void operator/=(vec4& v, float d)
{
	v.data = _mm_div_ps(v.data, _mm_set_ps1(d));
}

static inline vec4 operator+(const vec4& a, const vec4& b)
{
	return _mm_add_ps(a.data, b.data);
}

static inline vec4 operator-(const vec4& a, const vec4& b)
{
	return _mm_sub_ps(a.data, b.data);
}

static inline vec4 operator*(const vec4& v, float m)
{
	return _mm_mul_ps(v.data, _mm_set_ps1(m));
}

static inline vec4 operator*(float m, const vec4& v)
{
	return v * m;
}

static inline vec4 operator/(const vec4& v, float d)
{
	return _mm_div_ps(v.data, _mm_set_ps1(d));
}

static inline bool operator==(const vec4& a, const vec4& b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

static inline bool operator!=(const vec4& a, const vec4& b)
{
	return !(a == b);
}

static inline vec4 operator-(const vec4& v)
{
	return _mm_mul_ps(v.data, _mm_set_ps1(-1.f));
}

static inline std::ostream& operator<<(std::ostream& s, const vec4& v)
{
	s << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
	return s;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// QUAT ////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

static inline float length(const quat& q)
{
	return length(q.v4);
}

static inline void normalize(quat& q)
{
	normalize(q.v4);

	
	/*float qmagsq = sqlength(q.v4);
	if (fabsf(1.f - qmagsq) < 2.107342e-08f) 
	{
		q.v4 *= 2.f / (1.f + qmagsq);
	}
	else 
	{
		normalize(q.v4);
	}*/
}

static inline quat normalized(const quat& q)
{
	quat result = q;
	normalize(result);
	return result;
}

static inline void invert(quat& q)
{
	q.data = _mm_mul_ps(q.data, _mm_setr_ps(-1.f, -1.f, -1.f, 1.f));
}

static inline quat inverted(const quat& q)
{
	quat result = q;
	invert(result);
	return result;
}

static quat operator*(const quat& a, const quat& b)
{
	quat result;
	result.w = a.w * b.w - dot(a.v, b.v);
	result.v = a.v * b.w + b.v * a.w + cross(a.v, b.v);
	return normalized(result);
}

static inline vec3 operator*(const quat& q, const vec3& v)
{
	if (sqlength(v) == 0.f)
		return v;

	quat p(_mm_setr_ps(v.x, v.y, v.z, 0.f), false);
	return (q * p * inverted(q)).v;
}

static inline quat rotateFromTo(const quat& from, const quat& to)
{
	quat invFrom = inverted(from);
	quat result = to * invFrom;
	return normalized(result);
}

static inline quat rotateFromTo(const vec3& from, const vec3& to)
{
	float norm_u_norm_v = sqrt(sqlength(from) * sqlength(to));
	vec3 w = cross(from, to);
	return quat(w.x, w.y, w.z, norm_u_norm_v + dot(from, to));
}

static inline void getAxisRotation(const quat& q, vec3& axis, float& angle)
{
	angle = acosf(clamp(q.w, -1.f, 1.f)) * 2.f;
	if (angle != 0.f)
		axis = q.v / sinf(angle / 2.f);
}

static inline quat slerp(const quat& from, const quat& to, float t)
{
	assert(t >= 0.f);
	assert(t <= 1.f);
	quat completeRot = rotateFromTo(from, to);
	vec3 axis;
	float angle;
	getAxisRotation(completeRot, axis, angle);
	quat scaledRot(axis, angle * t);

	return normalized(scaledRot * from);
}

static inline std::ostream& operator<<(std::ostream& s, const quat& q)
{
	s << vec3(q.x, q.y, q.z) << ", " << q.w;
	return s;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// MAT 3 ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

static mat3 quaternionToMat3(const quat& q)
{
	if (q.w == 1.f)
	{
		return mat3();
	}

	float qxx(q.x * q.x);
	float qyy(q.y * q.y);
	float qzz(q.z * q.z);
	float qxz(q.x * q.z);
	float qxy(q.x * q.y);
	float qyz(q.y * q.z);
	float qwx(q.w * q.x);
	float qwy(q.w * q.y);
	float qwz(q.w * q.z);

	mat3 result;

	result.m00 = 1 - 2 * (qyy + qzz);
	result.m01 = 2 * (qxy + qwz);
	result.m02 = 2 * (qxz - qwy);

	result.m10 = 2 * (qxy - qwz);
	result.m11 = 1 - 2 * (qxx + qzz);
	result.m12 = 2 * (qyz + qwx);

	result.m20 = 2 * (qxz + qwy);
	result.m21 = 2 * (qyz - qwx);
	result.m22 = 1 - 2 * (qxx + qyy);

	return result;
}

static inline void transpose(mat3& m)
{
	std::swap(m.m01, m.m10);
	std::swap(m.m02, m.m20);
	std::swap(m.m12, m.m21);
}

static inline mat3 transposed(const mat3& m)
{
	mat3 result = m;
	transpose(result);
	return result;
}

static inline std::ostream& operator<<(std::ostream& s, const mat3& m)
{
	for (uint32 y = 0; y < 3; ++y)
	{
		s << "[ ";
		for (uint32 x = 0; x < 3; ++x)
		{
			uint32 index = y * 3 + x;
			s << m.data[index] << "\t";
		}
		s << "]\n";
	}
	return s;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// MAT 4 ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

static mat4 operator*(const mat4& a, const mat4& b)
{
	mat4 result;
	for (uint32 y = 0; y < 4; ++y)
	{
		for (uint32 x = 0; x < 4; ++x)
		{
			// switched order because column major
			uint32 indexR = x * 4 + y;
			float sum = 0.f;
			for (uint32 k = 0; k < 4; ++k)
			{
				uint32 indexA = k * 4 + y;
				uint32 indexB = x * 4 + k;
				sum += a.data[indexA] * b.data[indexB];
			}
			result.data[indexR] = sum;
		}
	}
	return result;
}

static vec4 operator*(const mat4& m, const vec4& v)
{
	vec4 result;
	result.x = m.m00 * v.x + m.m10 * v.y + m.m20 * v.z + m.m30 * v.w;
	result.y = m.m01 * v.x + m.m11 * v.y + m.m21 * v.z + m.m31 * v.w;
	result.z = m.m02 * v.x + m.m12 * v.y + m.m22 * v.z + m.m32 * v.w;
	result.w = m.m03 * v.x + m.m13 * v.y + m.m23 * v.z + m.m33 * v.w;

	return result;
}

static inline void operator*=(mat4& m, float f)
{
	for (uint32 i = 0; i < 16; ++i)
	{
		m.data[i] *= f;
	}
}

static mat4 operator*(const mat4& m, float f)
{
	mat4 result = m;
	result *= f;
	return result;
}

static inline mat4 createScaleMatrix(const vec3& scale)
{
	return mat4(scale);
}

static mat4 createProjectionMatrix(float fov, float aspect, float nearPlane, float farPlane)
{
	float depth = farPlane - nearPlane;
	float oneOverDepth = 1.f / depth;

	mat4 result;
	result.m11 = 1.f / tanf(0.5f * fov);
	result.m00 = result.m11 / aspect;
	result.m22 = -(farPlane + nearPlane) * oneOverDepth;
	result.m32 = -2.f * farPlane * nearPlane * oneOverDepth;
	result.m23 = -1.f;
	result.m33 = 0.f;

	return result;
}

static mat4 createTranslationMatrix(const vec3& position)
{
	mat4 result;
	result.m30 = position.x;
	result.m31 = position.y;
	result.m32 = position.z;
	return result;
}

static inline mat4 createModelMatrix(const vec3& position, const quat& rotation, const vec3& scale = vec3(1.f))
{
	mat4 result = createTranslationMatrix(position) * createScaleMatrix(scale) * quaternionToMat3(rotation);
	return result;
}

static inline mat4 createModelMatrix(const vec3& position, const quat& rotation, float scale = 1.f)
{
	return createModelMatrix(position, rotation, vec3(scale));
}

static mat4 lookAt(const vec3& eye, const vec3& target, const vec3& up)
{
	vec3 zAxis = normalized(eye - target);
	vec3 xAxis = normalized(cross(up, zAxis));
	vec3 yAxis = normalized(cross(zAxis, xAxis));

	mat4 result;
	result.m00 = xAxis.x; result.m01 = yAxis.x; result.m02 = zAxis.x; result.m03 = 0.f;
	result.m10 = xAxis.y; result.m11 = yAxis.y; result.m12 = zAxis.y; result.m13 = 0.f;
	result.m20 = xAxis.z; result.m21 = yAxis.z; result.m22 = zAxis.z; result.m23 = 0.f;
	result.m30 = -dot(xAxis, eye); result.m31 = -dot(yAxis, eye); result.m32 = -dot(zAxis, eye); result.m33 = 1.f;

	return result;
}

static inline mat4 createViewMatrix(const vec3& position, const quat& rotation)
{
	vec3 target = position + rotation * vec3(0.f, 0.f, -1.f);
	vec3 up = rotation * vec3(0.f, 1.f, 0.f);
	return lookAt(position, target, up);
}

static mat4 createViewMatrix(const vec3& eye, float pitch, float yaw)
{
	float cosPitch = cosf(pitch);
	float sinPitch = sinf(pitch);
	float cosYaw = cosf(yaw);
	float sinYaw = sinf(yaw);

	vec3 xAxis(cosYaw, 0, -sinYaw);
	vec3 yAxis(sinYaw * sinPitch, cosPitch, cosYaw * sinPitch);
	vec3 zAxis(sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw);

	mat4 result;
	result.m00 = xAxis.x; result.m01 = yAxis.x; result.m02 = zAxis.x; result.m03 = 0.f;
	result.m10 = xAxis.y; result.m11 = yAxis.y; result.m12 = zAxis.y; result.m13 = 0.f;
	result.m20 = xAxis.z; result.m21 = yAxis.z; result.m22 = zAxis.z; result.m23 = 0.f;
	result.m30 = -dot(xAxis, eye); result.m31 = -dot(yAxis, eye); result.m32 = -dot(zAxis, eye); result.m33 = 1.f;

	return result;
}

static inline void transpose(mat4& m)
{
	std::swap(m.m01, m.m10);
	std::swap(m.m02, m.m20);
	std::swap(m.m03, m.m30);
	std::swap(m.m12, m.m21);
	std::swap(m.m23, m.m32);
	std::swap(m.m13, m.m31);
}

static inline mat4 transposed(const mat4& m)
{
	mat4 result = m;
	transpose(result);
	return result;
}

static mat4 inverted(const mat4& m)
{
	mat4 inv;

	inv.m00 = m.m11 * m.m22 * m.m33 -
		m.m11 * m.m23 * m.m32 -
		m.m21 * m.m12 * m.m33 +
		m.m21 * m.m13 * m.m32 +
		m.m31 * m.m12 * m.m23 -
		m.m31 * m.m13 * m.m22;

	inv.m10 = -m.m10 * m.m22 * m.m33 +
		m.m10 * m.m23 * m.m32 +
		m.m20 * m.m12 * m.m33 -
		m.m20 * m.m13 * m.m32 -
		m.m30 * m.m12 * m.m23 +
		m.m30 * m.m13 * m.m22;

	inv.m20 = m.m10 * m.m21 * m.m33 -
		m.m10 * m.m23 * m.m31 -
		m.m20 * m.m11 * m.m33 +
		m.m20 * m.m13 * m.m31 +
		m.m30 * m.m11 * m.m23 -
		m.m30 * m.m13 * m.m21;

	inv.m30 = -m.m10 * m.m21 * m.m32 +
		m.m10 * m.m22 * m.m31 +
		m.m20 * m.m11 * m.m32 -
		m.m20 * m.m12 * m.m31 -
		m.m30 * m.m11 * m.m22 +
		m.m30 * m.m12 * m.m21;

	inv.m01 = -m.m01 * m.m22 * m.m33 +
		m.m01 * m.m23 * m.m32 +
		m.m21 * m.m02 * m.m33 -
		m.m21 * m.m03 * m.m32 -
		m.m31 * m.m02 * m.m23 +
		m.m31 * m.m03 * m.m22;

	inv.m11 = m.m00 * m.m22 * m.m33 -
		m.m00 * m.m23 * m.m32 -
		m.m20 * m.m02 * m.m33 +
		m.m20 * m.m03 * m.m32 +
		m.m30 * m.m02 * m.m23 -
		m.m30 * m.m03 * m.m22;

	inv.m21 = -m.m00 * m.m21 * m.m33 +
		m.m00 * m.m23 * m.m31 +
		m.m20 * m.m01 * m.m33 -
		m.m20 * m.m03 * m.m31 -
		m.m30 * m.m01 * m.m23 +
		m.m30 * m.m03 * m.m21;

	inv.m31 = m.m00 * m.m21 * m.m32 -
		m.m00 * m.m22 * m.m31 -
		m.m20 * m.m01 * m.m32 +
		m.m20 * m.m02 * m.m31 +
		m.m30 * m.m01 * m.m22 -
		m.m30 * m.m02 * m.m21;

	inv.m02 = m.m01 * m.m12 * m.m33 -
		m.m01 * m.m13 * m.m32 -
		m.m11 * m.m02 * m.m33 +
		m.m11 * m.m03 * m.m32 +
		m.m31 * m.m02 * m.m13 -
		m.m31 * m.m03 * m.m12;

	inv.m12 = -m.m00 * m.m12 * m.m33 +
		m.m00 * m.m13 * m.m32 +
		m.m10 * m.m02 * m.m33 -
		m.m10 * m.m03 * m.m32 -
		m.m30 * m.m02 * m.m13 +
		m.m30 * m.m03 * m.m12;

	inv.m22 = m.m00 * m.m11 * m.m33 -
		m.m00 * m.m13 * m.m31 -
		m.m10 * m.m01 * m.m33 +
		m.m10 * m.m03 * m.m31 +
		m.m30 * m.m01 * m.m13 -
		m.m30 * m.m03 * m.m11;

	inv.m32 = -m.m00 * m.m11 * m.m32 +
		m.m00 * m.m12 * m.m31 +
		m.m10 * m.m01 * m.m32 -
		m.m10 * m.m02 * m.m31 -
		m.m30 * m.m01 * m.m12 +
		m.m30 * m.m02 * m.m11;

	inv.m03 = -m.m01 * m.m12 * m.m23 +
		m.m01 * m.m13 * m.m22 +
		m.m11 * m.m02 * m.m23 -
		m.m11 * m.m03 * m.m22 -
		m.m21 * m.m02 * m.m13 +
		m.m21 * m.m03 * m.m12;

	inv.m13 = m.m00 * m.m12 * m.m23 -
		m.m00 * m.m13 * m.m22 -
		m.m10 * m.m02 * m.m23 +
		m.m10 * m.m03 * m.m22 +
		m.m20 * m.m02 * m.m13 -
		m.m20 * m.m03 * m.m12;

	inv.m23 = -m.m00 * m.m11 * m.m23 +
		m.m00 * m.m13 * m.m21 +
		m.m10 * m.m01 * m.m23 -
		m.m10 * m.m03 * m.m21 -
		m.m20 * m.m01 * m.m13 +
		m.m20 * m.m03 * m.m11;

	inv.m33 = m.m00 * m.m11 * m.m22 -
		m.m00 * m.m12 * m.m21 -
		m.m10 * m.m01 * m.m22 +
		m.m10 * m.m02 * m.m21 +
		m.m20 * m.m01 * m.m12 -
		m.m20 * m.m02 * m.m11;

	float det = m.m00 * inv.m00 + m.m01 * inv.m10 + m.m02 * inv.m20 + m.m03 * inv.m30;

	if (det == 0.f)
	{	
		std::cout << "could not invert matrix" << std::endl;
		return mat4();
	}

	det = 1.f / det;

	inv *= det;

	return inv;
}

static inline std::ostream& operator<<(std::ostream& s, const mat4& m)
{
	for (uint32 y = 0; y < 4; ++y)
	{
		s << "[ ";
		for (uint32 x = 0; x < 4; ++x)
		{
			uint32 index = y * 4 + x;
			s << m.data[index] << "\t";
		}
		s << "]\n";
	}
	return s;
}

////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// SQT ////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

static inline mat4 sqtToMat4(const SQT& sqt)
{
	return createModelMatrix(sqt.position, sqt.rotation, sqt.scale);
}

static inline SQT slerp(const SQT& from, const SQT& to, float t)
{
	SQT result;
	result.position = (1 - t) * from.position + t * to.position;
	result.rotation = slerp(from.rotation, to.rotation, t);
	result.scale = (1 - t) * from.scale + t * to.scale;
	return result;
}

static inline SQT operator*(const SQT& sqt, float f)
{
	assert(f >= 0.f);
	assert(f <= 1.f);
	SQT result;
	result.position = sqt.position * f;
	result.scale = sqt.scale * f;
	result.rotation = slerp(sqt.rotation, quat(), 1.f - f);
	return result;
}

////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// CONSTRUCTORS /////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

inline vec2::vec2()
{
	x = y = 0.f;
}

inline vec2::vec2(float v)
{
	x = y = v;
}

inline vec2::vec2(float x, float y)
{
	this->x = x; this->y = y;
}

inline vec3::vec3()
{
	x = y = z = 0.f;
}

inline vec3::vec3(float v)
{
	x = y = z = v;
}

inline vec3::vec3(float x, float y, float z)
{
	this->x = x; this->y = y; this->z = z;
}

inline vec3::vec3(const vec2& v2, float z)
{
	this->x = v2.x; this->y = v2.y; this->z = z;
}

inline vec4::vec4()
{
	data = _mm_set_ps1(0.f);
}

inline vec4::vec4(float v)
{
	data = _mm_set_ps1(v);
}

inline vec4::vec4(float x, float y, float z, float w)
{
	data = _mm_setr_ps(x, y, z, w);
}

inline vec4::vec4(const __m128& data)
{
	this->data = data;
}

inline vec4::vec4(const vec3& v3, float w)
{
	data = _mm_setr_ps(v3.x, v3.y, v3.z, w);
}

inline quat::quat()
{
	data = _mm_setr_ps(0.f, 0.f, 0.f, 1.f);
}

inline quat::quat(const vec4& v4, bool norm)
{
	data = v4.data;
	if (norm)
		normalize(*this);
}

inline quat::quat(const vec3& axis, float angle, bool norm)
{
	w = cosf(angle / 2.f);
	v = axis * sinf(angle / 2.f);
	if (norm)
		normalize(*this);
}

inline quat::quat(float x, float y, float z, float w, bool norm)
{
	data = _mm_setr_ps(x, y, z, w);
	if (norm)
		normalize(*this);
}

inline quat::quat(const __m128& data, bool norm)
{
	this->data = data;
	if (norm)
		normalize(*this);
}

inline mat3::mat3()
{
	for (uint32 y = 0; y < 3; ++y)
	{
		for (uint32 x = 0; x < 3; ++x)
		{
			int index = y * 3 + x;
			if (x == y)
				data[index] = 1.f;
			else
				data[index] = 0.f;
		}
	}
}

inline mat4::mat4()
{
	for (uint32 y = 0; y < 4; ++y)
	{
		for (uint32 x = 0; x < 4; ++x)
		{
			int index = y * 4 + x;
			if (x == y)
				data[index] = 1.f;
			else
				data[index] = 0.f;
		}
	}
}

inline mat4::mat4(const mat3& m3)
{
	m00 = m3.m00;
	m10 = m3.m10;
	m20 = m3.m20;
	m30 = 0.f;
	m01 = m3.m01;
	m11 = m3.m11;
	m21 = m3.m21;
	m31 = 0.f;
	m02 = m3.m02;
	m12 = m3.m12;
	m22 = m3.m22;
	m32 = 0.f;
	m03 = 0.f;
	m13 = 0.f;
	m23 = 0.f;
	m33 = 1.f;
}

inline mat4::mat4(const vec3& v3)
{
	for (uint32 y = 0; y < 4; ++y)
	{
		for (uint32 x = 0; x < 4; ++x)
		{
			int index = y * 4 + x;
			if (x == y && x < 3)
				data[index] = v3.xyz[x];
			else if (x == y && x == 3)
				data[index] = 1.f;
			else
				data[index] = 0.f;
		}
	}
}

inline SQT::SQT()
{
	position = vec3(0.f);
	rotation = quat();
	scale = 1.f;
}

inline SQT::SQT(const vec3& position, const quat& rotation, float scale)
{
	this->position = position;
	this->rotation = rotation;
	this->scale = scale;
}