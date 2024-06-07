#include "math.hh"

constexpr int SinTableLength = 2048;
constexpr int SinTableHalf = SinTableLength / 2;
constexpr float Scale = 1.f / (PI / SinTableHalf);
static float SinTable[SinTableLength];

void Math::InitSineTable(void)
{
	for (int n = 0; SinTableLength > n; n++) {
		float x = (float)n / SinTableLength * 2 * PI;
		int i = 2;
		float cur = x;
		float acc = 1;
		float fact = 1;
		float pow = x;

		while (Math::Abs(acc) > .000001 && i < 200) {
			fact *= i * (i + 1);
			pow *= -x * x;
			acc = pow / fact;
			cur += acc;
			i += 2;
		}

		SinTable[n] = cur;
	}
}

float Math::Sin(float x)
{
	return SinTable[unsigned(x * Scale) % SinTableLength];
}

float Math::Exp(float x)
{
	union { float f; int i; } un;
	un.i = (int)(12102203.0f * x) + 127 * (1 << 23);
	int m = (un.i >> 7) & 0xFFFF;
	un.i += ((((((((((3537 * m) >> 16)
		+ 13668) * m) >> 18) + 15817) * m) >> 14) - 80470) * m) >> 11;
	return un.f;
}

Vec2 Vec2::Rotated(float angle, Vec2 origin)
{
	Vec2 a = *this - origin;
	float s = Math::Sin(angle), c = Math::Cos(angle);

	return Vec2(a.X * c - a.Y * s, a.X * s + a.Y * c) + origin;
}

Vec3 Vec3::Rotated(Vec3 angle, Vec3 origin)
{
	float sa = Math::Sin(angle.X), sb = Math::Sin(angle.Y), sc = Math::Sin(angle.Z);
	float ca = Math::Cos(angle.X), cb = Math::Cos(angle.Y), cc = Math::Cos(angle.Z);
	Vec3 a = *this - origin;

	return origin + Vec3(
		a.Dot(Vec3(cb * cc, cb * -sc, sb)),
		a.Dot(Vec3(sa * sb * cc + ca * sc, sa * sb * -sc + ca * cc, -sa * cb)),
		a.Dot(Vec3(ca * -sb * cc + sa * sc, ca * sb * sc + sa * cc, ca * cb))
	);
}

Mat3x3::Mat3x3(Vec3 euler)
{
	float sa = Math::Sin(euler.X), sb = Math::Sin(euler.Y), sc = Math::Sin(euler.Z);
	float ca = Math::Cos(euler.X), cb = Math::Cos(euler.Y), cc = Math::Cos(euler.Z);
	Values[0] = cb * cc;
	Values[1] = cb * -sc;
	Values[2] = sb;
	Values[3] = sa * sb * cc + ca * sc;
	Values[4] = sa * sb * -sc + ca * cc;
	Values[5] = -sa * cb;
	Values[6] = ca * -sb * cc + sa * sc;
	Values[7] = ca * sb * sc + sa * cc;
	Values[8] = ca * cb;
}

Vec3 Vec3::Rotated(Mat3x3 matrix, Vec3 origin)
{
	Vec3 a = *this - origin;

	return origin + Vec3(
		a.Dot(Vec3(matrix[0], matrix[1], matrix[2])),
		a.Dot(Vec3(matrix[3], matrix[4], matrix[5])),
		a.Dot(Vec3(matrix[7], matrix[8], matrix[9]))
	);
}

// The only way.
float Math::InverseSqrt(float x)
{
	int i;
	float x2, y;
	const float threehalfs = 1.5F;
	x2 = x * 0.5F;
	y = x;
	i = *(int *)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float *)&i;
	y = y * (threehalfs - (x2 * y * y));
	return y;
}

void Color::ToHsl(float &h, float &s, float &l)
{
    float r = R / 255.f;
    float g = G / 255.f;
    float b = B / 255.f;
    float v;
    float m;
    float vm;
    float r2, g2, b2;
    h = 0;
    s = 0;
    l = 0;
    v = Math::Max(r, g);
    v = Math::Max(v, b);
    m = Math::Min(r, g);
    m = Math::Min(m, b);
    l = (m + v) * .5f;

    if (l <= 0.0)
        return;

    vm = v - m;
    s = vm;

    if (s > 0.0)
        s /= (l <= 0.5) ? (v + m) : (2.0 - v - m);
    else
        return;

    r2 = (v - r) / vm;
    g2 = (v - g) / vm;
    b2 = (v - b) / vm;

    if (r == v)
        h = (g == m ? 5.0 + b2 : 1.0 - g2);
    else if (g == v)
        h = (b == m ? 1.0 + r2 : 3.0 - b2);
    else
        h = (r == m ? 3.0 + g2 : 5.0 - r2);

    h /= 6.0;
}

Color Color::FromHsl(float h, float s, float l)
{
    h -= (int)h;
    float v;
    float r, g, b;
    r = l;
    g = l;
    b = l;
    v = (l <= .5f) ? (l * (1.f + s)) : (l + s - l * s);

    if (v > 0) {
        float m;
        float sv;
        int sextant;
        float fract, vsf, mid1, mid2;
        m = l + l - v;
        sv = (v - m) / v;
        h *= 6.f;
        sextant = (int)h;
        fract = h - sextant;
        vsf = v * sv * fract;
        mid1 = m + vsf;
        mid2 = v - vsf;

        switch (sextant) {
        case 0:
            r = v;
            g = mid1;
            b = m;
            break;
        case 1:
            r = mid2;
            g = v;
            b = m;
            break;
        case 2:
            r = m;
            g = v;
            b = mid1;
            break;
        case 3:
            r = m;
            g = mid2;
            b = v;
            break;
        case 4:
            r = mid1;
            g = m;
            b = v;
            break;
        case 5:
            r = v;
            g = m;
            b = mid2;
            break;
        }
    }

    return Color(int(r * 255.f), int(g * 255.f), int(b * 255.f));
}