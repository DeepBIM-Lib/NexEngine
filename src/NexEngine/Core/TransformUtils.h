//
// Created by SherSpidey on 2026/7/2.
//

#ifndef NEXENGINE_TRANSFORMUTILS_H
#define NEXENGINE_TRANSFORMUTILS_H

#include <flecs.h>

#include <NexEngine/Core/MathUtils.h>
#include <NexEngine/Core/Transform.h>

namespace Nex
{
    inline Vector4 ComposeRotorRotation(const Rotor& rotor) noexcept
    {
        const auto q_pitch = ToQuaternion({.x = 1.f, .y = 0.f, .z = 0.f}, rotor.data.x);
        const auto q_yaw = ToQuaternion({.x = 0.f, .y = 1.f, .z = 0.f}, rotor.data.y);

        // apply pitch around the camera-local right axis, then yaw around world up
        // ensure the base orientation is yaw-free so that `q_yaw` acts as the absolute world-space yaw
        const auto local_pitch_then_base = QuaternionMultiply(q_pitch, rotor.base);
        return QuaternionMultiply(local_pitch_then_base, q_yaw);
    }

    inline Transform TransformCompose(
        const Position& position,
        const Rotation* rotation,
        const Scale* scale,
        const Transform* parent
    )
    {
        Matrix4 result;

        // handle rotation first
        if (rotation)
        {
            result = RotationMatrix(rotation->data);
        }
        else result = IdentityMatrix4();

        // apply scale
        if (scale)
        {
            result.values[0] *= scale->data.x;
            result.values[1] *= scale->data.x;
            result.values[2] *= scale->data.x;
            result.values[3] *= scale->data.x;

            result.values[4] *= scale->data.y;
            result.values[5] *= scale->data.y;
            result.values[6] *= scale->data.y;
            result.values[7] *= scale->data.y;

            result.values[8] *= scale->data.z;
            result.values[9] *= scale->data.z;
            result.values[10] *= scale->data.z;
            result.values[11] *= scale->data.z;
        }

        // fill position
        result.values[12] = position.data.x;
        result.values[13] = position.data.y;
        result.values[14] = position.data.z;

        // apply parent's transform
        if (parent) result *= parent->data;

        return {result};
    }

    inline const Transform* GetParentTransform(flecs::entity entity)
    {
        auto parent = entity.parent();

        while (parent)
        {
            if (const auto transform = parent.try_get<Transform>())
                return transform;
            parent = parent.parent();
        }

        return nullptr;
    }

    inline bool UpdateTransform(flecs::entity entity, const Transform* pt = nullptr)
    {
        if (auto transform = entity.try_get_mut<Transform>())
        {
            const auto& p = entity.get<Position>();
            const auto r = entity.try_get<Rotation>();
            const auto s = entity.try_get<Scale>();
            *transform = TransformCompose(p, r, s, pt);

            return true;
        }
        return false;
    }
}

#endif //NEXENGINE_TRANSFORMUTILS_H
