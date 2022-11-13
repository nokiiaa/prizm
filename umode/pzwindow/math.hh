#include <pzapi.h>
#define PI 3.1415926535897932384626433f
#define LN2 .69314718056f
#define LN10 2.30258509299f

namespace Math
{
	float InverseSqrt(float x);

	inline float Sqrt(float x) { return InverseSqrt(x) * x; }

	template<typename T> inline T Lerp(T a, T b, float t)
	{
		return a + (b - a) * t;
	}

	template<typename T> inline T BiLerp(T c00, T c10, T c01, T c11, float tx, float ty)
	{
		return Lerp(Lerp(c00, c10, tx), Lerp(c01, c11, tx), ty);
	}

	template<typename T> inline T Clamp(T x, T min, T max)
	{
		return x < min ? min : x > max ? max : x;
	}

	template<typename T> inline T Min(T a, T b) { return a > b ? b : a; }
	template<typename T> inline T Max(T a, T b) { return a > b ? a : b; }

	inline float Round(float x)
	{
		return (int)(x < 0 ? x - .5 : x + .5);
	}

	inline float Ceil(float x)
	{
		if (x >= 0)
			return x + (x > (int)x);
		else
			return x - (x < (int)x);
	}

	template<typename T> inline T Abs(T x)
	{
		return x < 0 ? -x : x;
	}

	template<typename T> inline T Wrap(T x, T min, T max)
	{
		x -= min;
		x %= max - min;
		return x < 0 ? max + x : min + x;
	}

	float Sin(float x);
	inline float Cos(float x) { return Sin(PI * .5f + x); }
	void InitSineTable(void);
	float Exp(float x);
	inline float Exp2(float x) { return Exp(x * LN2); }
}

struct Vec2
{
	float X, Y;
	inline Vec2(float x = 0, float y = 0) { X = x; Y = y; }
	inline float Magnitude() { return Math::Sqrt(X * X + Y * Y); }
	inline float MagnitudeSquared() { return X * X + Y * Y; }
	inline float Dot(Vec2 b) { return X * b.X + Y * b.Y; }
	inline Vec2 operator+(Vec2 b) { return Vec2(X + b.X, Y + b.Y); }
	inline Vec2 operator-(Vec2 b) { return Vec2(X - b.X, Y - b.Y); }
	inline Vec2 operator*(float k) { return Vec2(X * k, Y * k); }
	inline Vec2 operator/(float k) { return *this * (1.f / k); }
	Vec2 Rotated(float angle, Vec2 origin = Vec2());
	inline float Distance(Vec2 b) { return (*this - b).Magnitude(); }
	inline Vec2 Normalized() { return *this * Math::InverseSqrt(MagnitudeSquared()); }
};

struct Mat3x3;

struct Vec3
{
	float X, Y, Z;
	inline Vec3(float x = 0, float y = 0, float z = 0) { X = x; Y = y; Z = z; }
	inline float Magnitude() { return Math::Sqrt(X * X + Y * Y + Z * Z); }
	inline float MagnitudeSquared() { return X * X + Y * Y + Z * Z; }
	inline float Dot(Vec3 b) { return X * b.X + Y * b.Y + Z * b.Z; }
	inline Vec3 operator+(Vec3 b) { return Vec3(X + b.X, Y + b.Y, Z + b.Z); }
	inline Vec3 operator-(Vec3 b) { return Vec3(X - b.X, Y - b.Y, Z - b.Z); }
	inline Vec3 operator*(float k) { return Vec3(X * k, Y * k, Z * k); }
	inline Vec3 operator/(float k) { return *this * (1.f / k); }
	Vec3 Rotated(Vec3 angle, Vec3 origin = Vec3());
	Vec3 Rotated(Mat3x3 matrix, Vec3 origin = Vec3());
	inline float Distance(Vec3 b) { return (*this - b).Magnitude(); }
	inline Vec3 Normalized() { return *this * Math::InverseSqrt(MagnitudeSquared()); }
	inline Vec3 Cross(Vec3 b)
	{
		return Vec3(
			Y * b.Z - Z * b.Y,
			Z * b.X - X * b.Z,
			X * b.Y - Y * b.X
		);
	}
};

struct Mat3x3
{
	float Values[3 * 3];
	Mat3x3(Vec3 euler);
	inline float &operator[](int index) { return Values[index]; }
};

struct Color
{
	union
	{
		struct
		{
			u8 B, G, R, A;
		};
		u32 Raw;
	};

	Color(u32 raw) : Raw(raw) { }
	Color(u8 r, u8 g, u8 b, u8 a = 255)
		: A(a), R(r), G(g), B(b) { }

	inline operator u32() { return Raw; }

	inline Color operator+(Color b)
	{
		return Color(
			Math::Clamp((int)R + b.R, 0, 255),
			Math::Clamp((int)G + b.G, 0, 255),
			Math::Clamp((int)B + b.B, 0, 255),
			Math::Clamp((int)A + b.A, 0, 255)
		);
	}

	inline Color operator-(Color b)
	{
		return Color(
			Math::Clamp((int)R - b.R, 0, 255),
			Math::Clamp((int)G - b.G, 0, 255),
			Math::Clamp((int)B - b.B, 0, 255),
			Math::Clamp((int)A - b.A, 0, 255)
		);
	}

	inline Color operator*(Color b)
	{
		return Color(
			(int)(R * b.R * (1.f / 255)),
			(int)(G * b.G * (1.f / 255)),
			(int)(B * b.B * (1.f / 255)),
			(int)(A * b.A * (1.f / 255))
		);
	}

	inline Color operator*(float t)
	{
		return Color(
			Math::Clamp((int)(R * t), 0, 255),
			Math::Clamp((int)(G * t), 0, 255),
			Math::Clamp((int)(B * t), 0, 255),
			Math::Clamp((int)(A * t), 0, 255)
		);
	}

	inline Color operator/(float t) { return *this * (1.f / t); }
	inline Color operator^(Color b) { return Color(Raw ^ b.Raw); }
	inline Color operator&(Color b) { return Color(Raw & b.Raw); }
	inline Color operator|(Color b) { return Color(Raw | b.Raw); }
	inline Color operator~() { return Color(~Raw); }

	static Color FromHsl(float h, float s, float l);
	void ToHsl(float &h, float &s, float &l);
};