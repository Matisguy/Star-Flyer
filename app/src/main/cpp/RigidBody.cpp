#include "RigidBody.h"
#include <cmath>

RigidBody::RigidBody(float massVal, float radiusVal, Vec3 pos, Vec3 col)
    : position(pos), mass(massVal), radius(radiusVal), color(col) {
    if (massVal > 0.0f) {
        invMass = 1.0f / massVal;
        // Spherical Moment of Inertia approximation: I = (2/5) * m * r^2
        inertia = 0.4f * massVal * radiusVal * radiusVal;
        invInertia = 1.0f / inertia;
    } else {
        invMass = 0.0f;
        inertia = 0.0f;
        invInertia = 0.0f;
    }
    orientation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
    angularVelocity = Vec3(0.0f, 0.0f, 0.0f);
}

void RigidBody::applyForce(const Vec3 &f) {
    force += f;
}

void RigidBody::update(float dt) {
    if (invMass == 0.0f) return;

    // 1. Linear Integration
    Vec3 accel = force * invMass;
    velocity += accel * dt;
    position += velocity * dt;

    // Linear Drag / Space Damping (0.95 per 1/60th sec)
    velocity *= std::pow(0.95f, dt * 60.0f);

    // 2. Kinematic Quaternion Angular Integration
    // dq/dt = 0.5 * (0, w) * q
    Quaternion wQuat(0.0f, angularVelocity.x, angularVelocity.y, angularVelocity.z);
    Quaternion dq = wQuat * orientation;

    orientation.w += 0.5f * dq.w * dt;
    orientation.x += 0.5f * dq.x * dt;
    orientation.y += 0.5f * dq.y * dt;
    orientation.z += 0.5f * dq.z * dt;

    // MANDATORY NORMALIZATION: Ensures rigid body scale never expands or distorts!
    orientation.normalize();

    // Angular Damping
    angularVelocity *= std::pow(0.92f, dt * 60.0f);

    // Reset force accumulator
    force = {0.0f, 0.0f, 0.0f};
}

void RigidBody::getModelMatrix(float *outMatrix) const {
    // 1. Convert normalized unit quaternion to 3x3 rotation basis
    orientation.toMatrix(outMatrix, scale);

    // 2. Set translation in column 3
    outMatrix[12] = position.x;
    outMatrix[13] = position.y;
    outMatrix[14] = position.z;
    outMatrix[15] = 1.0f;
}

void RigidBody::resolveCollision(RigidBody &a, RigidBody &b) {
    Vec3 delta = b.position - a.position;
    float distSq = delta.lengthSq();
    float minDist = a.radius + b.radius;

    if (distSq < minDist * minDist && distSq > 0.00001f) {
        float dist = std::sqrt(distSq);
        Vec3 normal = delta / dist; // Contact normal pointing from A to B
        float penetration = minDist - dist;

        float totalInvMass = a.invMass + b.invMass;
        if (totalInvMass == 0.0f) return;

        // 1. Positional Separation (prevent overlap)
        a.position -= normal * (penetration * (a.invMass / totalInvMass));
        b.position += normal * (penetration * (b.invMass / totalInvMass));

        // 2. Contact Point Relative Vectors
        Vec3 rA = normal * a.radius;
        Vec3 rB = normal * (-b.radius);

        // Velocity at contact point (Linear + Angular x Radius)
        Vec3 vA_contact = a.velocity + Vec3::cross(a.angularVelocity, rA);
        Vec3 vB_contact = b.velocity + Vec3::cross(b.angularVelocity, rB);
        Vec3 relVel = vB_contact - vA_contact;

        float velAlongNormal = Vec3::dot(relVel, normal);

        // Resolve impulse only if moving towards each other
        if (velAlongNormal < 0.0f) {
            float restitution = 0.75f; // Elastic rebound

            // Effective inverse mass including angular inertia
            Vec3 rA_cross_n = Vec3::cross(rA, normal);
            Vec3 rB_cross_n = Vec3::cross(rB, normal);

            float invInertiaSum = a.invMass + b.invMass +
                Vec3::dot(rA_cross_n, rA_cross_n) * a.invInertia +
                Vec3::dot(rB_cross_n, rB_cross_n) * b.invInertia;

            if (invInertiaSum <= 0.00001f) return;

            // Normal Impulse Magnitude
            float jn = -(1.0f + restitution) * velAlongNormal / invInertiaSum;
            Vec3 impulseNormal = normal * jn;

            // Apply Linear Velocities
            a.velocity -= impulseNormal * a.invMass;
            b.velocity += impulseNormal * b.invMass;

            // Apply Torque Impulses (Newton's 3rd Law: opposite reaction torques)
            a.angularVelocity -= Vec3::cross(rA, impulseNormal) * a.invInertia;
            b.angularVelocity += Vec3::cross(rB, impulseNormal) * b.invInertia;

            // 3. Tangential Friction Impulse
            Vec3 tanVel = relVel - normal * velAlongNormal;
            float tanSpeed = tanVel.length();
            if (tanSpeed > 0.0001f) {
                Vec3 tangent = tanVel / tanSpeed;
                float friction = 0.3f; // Surface friction coefficient

                float jt = -Vec3::dot(relVel, tangent) / invInertiaSum;

                // Clamp friction to Coulomb's Law (|jt| <= friction * jn)
                if (jt > friction * jn) jt = friction * jn;
                if (jt < -friction * jn) jt = -friction * jn;

                Vec3 impulseTangent = tangent * jt;

                a.velocity -= impulseTangent * a.invMass;
                b.velocity += impulseTangent * b.invMass;

                a.angularVelocity -= Vec3::cross(rA, impulseTangent) * a.invInertia;
                b.angularVelocity += Vec3::cross(rB, impulseTangent) * b.invInertia;
            }
        }
    }
}
