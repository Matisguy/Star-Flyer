#ifndef STARFLYER_RIGID_BODY_H
#define STARFLYER_RIGID_BODY_H

#include "Utility.h"
#include <cmath>

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }

    Vec3 &operator+=(const Vec3 &o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3 &operator-=(const Vec3 &o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3 &operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float lengthSq() const { return x * x + y * y + z * z; }
    float length() const { return std::sqrt(lengthSq()); }

    Vec3 normalized() const {
        float len = length();
        if (len > 0.00001f) return *this * (1.0f / len);
        return {0.0f, 0.0f, 0.0f};
    }

    static float dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    static Vec3 cross(const Vec3 &a, const Vec3 &b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
};

struct Quaternion {
    float w = 1.0f;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Quaternion() = default;
    Quaternion(float w_, float x_, float y_, float z_) : w(w_), x(x_), y(y_), z(z_) {}

    Quaternion operator*(const Quaternion &q) const {
        return {
            w * q.w - x * q.x - y * q.y - z * q.z,
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w
        };
    }

    Quaternion conjugate() const {
        return {w, -x, -y, -z};
    }

    void normalize() {
        float lenSq = w * w + x * x + y * y + z * z;
        if (lenSq > 0.000001f) {
            float invLen = 1.0f / std::sqrt(lenSq);
            w *= invLen;
            x *= invLen;
            y *= invLen;
            z *= invLen;
        } else {
            w = 1.0f; x = 0.0f; y = 0.0f; z = 0.0f;
        }
    }

    void toMatrix(float *outMatrix, float scale = 1.0f) const {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = w * x, wy = w * y, wz = w * z;

        // Column 0
        outMatrix[0] = scale * (1.0f - 2.0f * (yy + zz));
        outMatrix[1] = scale * (2.0f * (xy + wz));
        outMatrix[2] = scale * (2.0f * (xz - wy));
        outMatrix[3] = 0.0f;

        // Column 1
        outMatrix[4] = scale * (2.0f * (xy - wz));
        outMatrix[5] = scale * (1.0f - 2.0f * (xx + zz));
        outMatrix[6] = scale * (2.0f * (yz + wx));
        outMatrix[7] = 0.0f;

        // Column 2
        outMatrix[8] = scale * (2.0f * (xz + wy));
        outMatrix[9] = scale * (2.0f * (yz - wx));
        outMatrix[10] = scale * (1.0f - 2.0f * (xx + yy));
        outMatrix[11] = 0.0f;

        // Column 3
        outMatrix[12] = 0.0f;
        outMatrix[13] = 0.0f;
        outMatrix[14] = 0.0f;
        outMatrix[15] = 1.0f;
    }
};

class RigidBody {
public:
    RigidBody(float mass, float radius, Vec3 position, Vec3 color);

    void applyForce(const Vec3 &force);
    void update(float dt);

    // Builds 4x4 column-major Model Transformation Matrix
    void getModelMatrix(float *outMatrix) const;

    // Resolve collision between two rigid bodies with linear & angular momentum exchange
    static void resolveCollision(RigidBody &a, RigidBody &b);

    int type; // what is this object actually supposed to be?

    Vec3 position;
    Vec3 velocity;
    Vec3 force;

    Quaternion orientation;
    Vec3 angularVelocity;  // rad/s

    float mass;
    float invMass;
    float radius;
    float inertia;
    float invInertia;
    float scale = 1.0f;

    Vec3 color;            // RGB color tint

    // Tap Input Interaction Flags
    bool pointedAt = false;
    bool shotAt = false;
    bool chargedShot = false;
};

#endif // STARFLYER_RIGID_BODY_H
