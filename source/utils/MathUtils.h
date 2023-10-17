#pragma once

#include <cmath>
#include <algorithm>

namespace WorldAssistant
{

static const int M_MAX_INT = 0x7fffffff;
static const float M_INFINITY = (float)HUGE_VAL;
static const float M_LARGE_VALUE = 100000000.0f;

// Intersection test result.
enum Intersection
{
    OUTSIDE,
    INTERSECTS,
    INSIDE
};

// Round up to next power of two.
inline unsigned NextPowerOfTwo(unsigned value)
{
    // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    --value;
    value |= value >> 1u;
    value |= value >> 2u;
    value |= value >> 4u;
    value |= value >> 8u;
    value |= value >> 16u;
    return ++value;
}

// Return log base two or the MSB position of the given value.
inline unsigned LogBaseTwo(unsigned value)
{
    // http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
    unsigned ret = 0;
    while (value >>= 1)     // Unroll for more speed...
        ++ret;
    return ret;
}

template <class T>
inline T Clamp(T value, T min, T max)
{
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}

template <class T>
inline bool Equals(T lhs, T rhs) { return lhs + std::numeric_limits<T>::epsilon() >= rhs && lhs - std::numeric_limits<T>::epsilon() <= rhs; }

template<typename T = int>
class Vector2
{
public:
	Vector2() :
		x_(0), y_(0)
	{
	}

	Vector2(T x, T y) :
		x_(x), y_(y)
	{
	}

	Vector2(T* buffer) :
		x_(buffer[0]), y_(buffer[1])
	{
	}

    // Test for equality with another vector without epsilon.
	bool operator==(const Vector2<T>& other) const { return x_ == other.x_ && y_ == other.y_; }

    // Test for inequality with another vector without epsilon.
    bool operator !=(const Vector2& rhs) const { return x_ != rhs.x_ || y_ != rhs.y_; }

	// Add a vector.
    Vector2<T> operator +(const Vector2<T>& rhs) const { return Vector2<T>(x_ + rhs.x_, y_ + rhs.y_); }

	// Subtract a vector.
    Vector2<T> operator -(const Vector2<T>& rhs) const { return Vector2<T>(x_ - rhs.x_, y_ - rhs.y_); }

    // Multiply with a scalar.
    Vector2<T> operator *(float rhs) const { return Vector2<T>(x_ * rhs, y_ * rhs); }

    // Multiply with a vector.
    Vector2<T> operator *(const Vector2<T>& rhs) const { return Vector2<T>(x_ * rhs.x_, y_ * rhs.y_); }

    // Divide by a scalar.
    Vector2<T> operator /(float rhs) const { return Vector2<T>(x_ / rhs, y_ / rhs); }

    // Divide by a vector.
    Vector2<T> operator /(const Vector2<T>& rhs) const { return Vector2<T>(x_ / rhs.x_, y_ / rhs.y_); }

	// Add-assign a vector.
    Vector2<T>& operator +=(const Vector2<T>& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        return *this;
    }

    // Subtract-assign a vector.
    Vector2<T>& operator -=(const Vector2<T>& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        return *this;
    }

    // Multiply-assign a scalar.
    Vector2<T>& operator *=(float rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        return *this;
    }

    // Multiply-assign a vector.
    Vector2<T>& operator *=(const Vector2<T>& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        return *this;
    }

    // Divide-assign a scalar.
    Vector2<T>& operator /=(float rhs)
    {
        float invRhs = 1.0f / rhs;
        x_ *= invRhs;
        y_ *= invRhs;
        return *this;
    }

    // Divide-assign a vector.
    Vector2<T>& operator /=(const Vector2<T>& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
        return *this;
    }

	T x_;
	T y_;
};

using Vector2F = Vector2<float>;
using Int32Vector2 = Vector2<int>;
using Int16Vector2 = Vector2<short>;

template<typename T = int>
class IntVector3
{
public:
	IntVector3() :
		x_(0), y_(0), z_(0)
	{
	}

	IntVector3(T x, T y, T z) :
		x_(x), y_(y), z_(z)
	{
	}

	T x_;
	T y_;
	T z_;
};

using Int32Vector3 = IntVector3<int>;
using Int16Vector3 = IntVector3<short>;

template<typename T = float>
class Vector3
{
public:
	Vector3() : 
		x_(0), y_(0), z_(0)
	{
	}

	Vector3(const T* buffer) :
		x_(buffer[0]), y_(buffer[1]), z_(buffer[2])
	{
	}

	Vector3(T x, T y, T z) :
		x_(x), y_(y), z_(z)
	{
	}

	void FromBuffer(const T* buffer)
	{
		x_ = buffer[0];
		y_ = buffer[1];
		z_ = buffer[2];
	}

	Vector3<T> operator - (const Vector3<T>& rhs) const
	{
		return Vector3(x_ - rhs.x_, y_ - rhs.y_, z_ - rhs.z_);
	}
	
	Vector3<T> operator + (const Vector3<T>& rhs) const
	{
		return Vector3<T>(x_ + rhs.x_, y_ + rhs.y_, z_ + rhs.z_);
	}

	Vector3<T> operator / (T t)
	{
		return Vector3<T>(x_ / t, y_ / t, z_ / t);
	}

	float DotProduct(const Vector3<T>& rhs)
	{
		return x_ * rhs.x_ + y_ * rhs.y_ + z_ * rhs.z_;
	}

	// Calculate cross product.
    Vector3<T> CrossProduct(const Vector3<T>& rhs) const
    {
        return Vector3<T>(
            y_ * rhs.z_ - z_ * rhs.y_,
            z_ * rhs.x_ - x_ * rhs.z_,
            x_ * rhs.y_ - y_ * rhs.x_
        );
    }

	float Length() const
	{
		return std::sqrt(x_ * x_ + y_ * y_ + z_ * z_);
	}

	// Return squared length.
    float LengthSquared() const { return x_ * x_ + y_ * y_ + z_ * z_; }

	// Add-assign a vector.
    Vector3<T>& operator +=(const Vector3<T>& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        z_ += rhs.z_;
        return *this;
    }

	// Subtract-assign a vector.
    Vector3& operator -=(const Vector3<T>& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        z_ -= rhs.z_;
        return *this;
    }

	// Multiply with a vector.
    Vector3<T> operator *(const Vector3<T>& rhs) const { return Vector3<T>(x_ * rhs.x_, y_ * rhs.y_, z_ * rhs.z_); }

	// Multiply with a scalar.
    Vector3<T> operator *(float rhs) const { return Vector3<T>(x_ * rhs, y_ * rhs, z_ * rhs); }

	void Normalize()
	{
		const T len = Length();

		x_ /= len;
		y_ /= len;
		z_ /= len;
	}

	// Return normalized to unit length.
    Vector3<T> Normalized() const
    {
        const float lenSquared = LengthSquared();
        if (!Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
        {
            float invLen = 1.0f / sqrtf(lenSquared);
            return *this * invLen;
        }
        else
            return *this;
    }

	void ToArray(T* rhs)
	{
		rhs[0] = x_;
		rhs[1] = y_;
		rhs[2] = z_;
	}

	Int16Vector3 Pack(T multiplier) const
	{
		return Int16Vector3(
			static_cast<short>(x_ * multiplier),
			static_cast<short>(y_ * multiplier),
			static_cast<short>(z_ * multiplier)
		);
	}

	Vector3<T> Discretized(T multiplier)
	{
		IntVector3 packed = Pack(multiplier);

		return Vector3<T>(
			packed.x_ / multiplier,
			packed.y_ / multiplier,
			packed.z_ / multiplier
		);
	}

	T x_;
	T y_;
	T z_;
};

using Vector3F = Vector3<float>;
using Vector3D = Vector3<double>;

template<typename T = float>
class Vector4
{
public:
	Vector4() :
		x_(0), y_(0), z_(0), w_(0)
	{
	}

	Vector4(const T* buffer) :
		x_(buffer[0]), y_(buffer[1]), z_(buffer[2]), w_(buffer[3])
	{
	}

	Vector4(T x, T y, T z, T w) :
		x_(x), y_(y), z_(z), w_(w)
	{
	}

	T Length() const
	{
		return std::sqrt(x_ * x_ + y_ * y_ + z_ * z_ + w_ * w_);
	}

	void FromBuffer(const T* buffer)
	{
		x_ = buffer[0];
		y_ = buffer[1];
		z_ = buffer[2];
		w_ = buffer[3];
	}

	T x_;
	T y_;
	T z_;
	T w_;
};

using Vector4F = Vector4<float>;
using ByteVector4 = Vector4<int8_t>;
using UByteVector4 = Vector4<uint8_t>;

struct Rect
{
	// Construct an undefined rect.
    Rect() noexcept :
        min_(M_INFINITY, M_INFINITY),
        max_(-M_INFINITY, -M_INFINITY)
    {
    }

    // Construct from minimum and maximum vectors.
    Rect(const Vector2F& min, const Vector2F& max) noexcept :
        min_(min),
        max_(max)
    {
    }

    // Construct from coordinates.
    Rect(float left, float top, float right, float bottom) noexcept :
        min_(left, top),
        max_(right, bottom)
    {
    }

	// Copy-construct from another rect.
    Rect(const Rect& rect) noexcept = default;

	// Assign from another rect.
    Rect& operator =(const Rect& rhs) noexcept = default;

    // Test for equality with another rect.
    bool operator ==(const Rect& rhs) const { return min_ == rhs.min_ && max_ == rhs.max_; }

    // Test for inequality with another rect.
    bool operator !=(const Rect& rhs) const { return min_ != rhs.min_ || max_ != rhs.max_; }

    // Add another rect to this one inplace.
    Rect& operator +=(const Rect& rhs)
    {
        min_ += rhs.min_;
        max_ += rhs.max_;
        return *this;
    }

    // Subtract another rect from this one inplace.
    Rect& operator -=(const Rect& rhs)
    {
        min_ -= rhs.min_;
        max_ -= rhs.max_;
        return *this;
    }

    // Divide by scalar inplace.
    Rect& operator /=(float value)
    {
        min_ /= value;
        max_ /= value;
        return *this;
    }

    // Multiply by scalar inplace.
    Rect& operator *=(float value)
    {
        min_ *= value;
        max_ *= value;
        return *this;
    }

    // Divide by scalar.
    Rect operator /(float value) const
    {
        return Rect(min_ / value, max_ / value);
    }

    // Multiply by scalar.
    Rect operator *(float value) const
    {
        return Rect(min_ * value, max_ * value);
    }

    // Add another rect.
    Rect operator +(const Rect& rhs) const
    {
        return Rect(min_ + rhs.min_, max_ + rhs.max_);
    }

    // Subtract another rect.
    Rect operator -(const Rect& rhs) const
    {
        return Rect(min_ - rhs.min_, max_ - rhs.max_);
    }

     // Define from minimum and maximum vectors.
    void Define(const Vector2F& min, const Vector2F& max)
    {
        min_ = min;
        max_ = max;
    }

	// Return center.
    Vector2F Center() const { return (max_ + min_) * 0.5f; }

    // Return size.
    Vector2F Size() const { return max_ - min_; }

    // Return half-size.
    Vector2F HalfSize() const { return (max_ - min_) * 0.5f; }

    // Test whether a point is inside.
    Intersection IsInside(const Vector2F& point) const
    {
        if (point.x_ < min_.x_ || point.y_ < min_.y_ || point.x_ > max_.x_ || point.y_ > max_.y_)
            return OUTSIDE;
        else
            return INSIDE;
    }

    // Test if another rect is inside, outside or intersects.
    Intersection IsInside(const Rect& rect) const
    {
        if (rect.max_.x_ < min_.x_ || rect.min_.x_ > max_.x_ || rect.max_.y_ < min_.y_ || rect.min_.y_ > max_.y_)
            return OUTSIDE;
        else if (rect.min_.x_ < min_.x_ || rect.max_.x_ > max_.x_ || rect.min_.y_ < min_.y_ || rect.max_.y_ > max_.y_)
            return INTERSECTS;
        else
            return INSIDE;
    }

	Vector2F min_;

	Vector2F max_;
};

struct BoundingBox
{
	BoundingBox() noexcept :
		min_(M_INFINITY, M_INFINITY, M_INFINITY),
		max_(-M_INFINITY, -M_INFINITY, -M_INFINITY)
	{
	}

	explicit BoundingBox(const Vector3F& min, const Vector3F& max) noexcept :
		min_(min),
		max_(max)
	{
	}

	// Return center.
    Vector3F Center() const { return (max_ + min_) * 0.5f; }

	// Return size.
    Vector3F Size() const { return max_ - min_; }

	void Clear()
	{
		min_ = Vector3F(M_INFINITY, M_INFINITY, M_INFINITY);
		max_ = Vector3F(-M_INFINITY, -M_INFINITY, -M_INFINITY);
	}

	void Merge(const Vector3F& point)
	{
		if (point.x_ < min_.x_)
            min_.x_ = point.x_;
        if (point.y_ < min_.y_)
            min_.y_ = point.y_;
        if (point.z_ < min_.z_)
            min_.z_ = point.z_;
        if (point.x_ > max_.x_)
            max_.x_ = point.x_;
        if (point.y_ > max_.y_)
            max_.y_ = point.y_;
        if (point.z_ > max_.z_)
            max_.z_ = point.z_;
	}

	// Merge another bounding box.
    void Merge(const BoundingBox& box)
	{
		if (box.min_.x_ < min_.x_)
            min_.x_ = box.min_.x_;
        if (box.min_.y_ < min_.y_)
            min_.y_ = box.min_.y_;
        if (box.min_.z_ < min_.z_)
            min_.z_ = box.min_.z_;
        if (box.max_.x_ > max_.x_)
            max_.x_ = box.max_.x_;
        if (box.max_.y_ > max_.y_)
            max_.y_ = box.max_.y_;
        if (box.max_.z_ > max_.z_)
            max_.z_ = box.max_.z_;
	}

	// Test if a point is inside.
    Intersection IsInside(const Vector3F& point) const
    {
        if (point.x_ < min_.x_ || point.x_ > max_.x_ || point.y_ < min_.y_ || point.y_ > max_.y_ ||
            point.z_ < min_.z_ || point.z_ > max_.z_)
            return OUTSIDE;
        else
            return INSIDE;
    }

	Vector3F min_;
	Vector3F max_;
};

inline Rect RectFromOriginAndSize(const Vector2F& min, const Vector2F& size)
{
    return Rect(min, min + size);
}

}