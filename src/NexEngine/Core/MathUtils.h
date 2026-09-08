//
// Created by SherSpidey on 2026/6/10.
//

#ifndef NEXENGINE_MATHUTILS_H
#define NEXENGINE_MATHUTILS_H

#include <bit>
#include <cstddef>

#include <NexEngine/Core/Math.h>

#if __has_include(<DirectXMath.h>)
//  In this case, DirectXMath is coming from Windows SDK.
//	It is better to use this on Windows as some Windows libraries could depend on the same
//	DirectXMath headers
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
#else
//  In this case, DirectXMath is coming from supplied source code
//	On platforms that don't have Windows SDK, the source code for DirectXMath is provided
//	as part of the engine utilities
#include <DirectXMath/Inc/DirectXMath.h>
#include <DirectXMath/Inc/DirectXPackedVector.h>
#include <DirectXMath/Inc/DirectXCollision.h>
#endif

namespace Nex
{
    inline float ToRadians(float degrees) noexcept
    {
        return DirectX::XMConvertToRadians(degrees);
    }

    inline Matrix4 ScaleMatrix(const Vector3& scale) noexcept
    {
        const auto mat = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);

        DirectX::XMFLOAT4X4 result;
        DirectX::XMStoreFloat4x4(&result, mat);

        return std::bit_cast<Matrix4>(result);
    }

    inline Matrix4 ApplyLocalScale(const Matrix4& transform, const Vector3& scale) noexcept
    {
        auto result = transform;

        // DirectX row vectors apply local scale as Scale * Transform. Scale the
        // first three rows so the translation in the fourth row stays unchanged.
        for (std::size_t i = 0; i < 4; ++i)
        {
            result.values[i] *= scale.x;
            result.values[4 + i] *= scale.y;
            result.values[8 + i] *= scale.z;
        }

        return result;
    }

    inline Vector3 ScaleFromMatrix(const Matrix4& m) noexcept
    {
        auto& data = m.values;

        const auto x_sq = data[0] * data[0] + data[1] * data[1] + data[2] * data[2];
        const auto y_sq = data[4] * data[4] + data[5] * data[5] + data[6] * data[6];
        const auto z_sq = data[8] * data[8] + data[9] * data[9] + data[10] * data[10];

        return {std::sqrt(x_sq), std::sqrt(y_sq), std::sqrt(z_sq)};
    }

    inline Matrix4 RotationMatrix(const Vector3& euler) noexcept
    {
        const auto mat = DirectX::XMMatrixRotationRollPitchYaw(euler.x, euler.y, euler.z);

        DirectX::XMFLOAT4X4 result;
        DirectX::XMStoreFloat4x4(&result, mat);

        return std::bit_cast<Matrix4>(result);
    }

    inline Matrix4 RotationMatrix(const Vector4& quat) noexcept
    {
        const auto vec = DirectX::XMVectorSet(quat.x, quat.y, quat.z, quat.w);
        const auto mat = DirectX::XMMatrixRotationQuaternion(vec);

        DirectX::XMFLOAT4X4 result;
        DirectX::XMStoreFloat4x4(&result, mat);

        return std::bit_cast<Matrix4>(result);
    }

    inline Vector4 RotationFromMatrix(const Matrix4& m) noexcept
    {
        const auto dx_mat = std::bit_cast<DirectX::XMFLOAT4X4>(m);
        auto mat = DirectX::XMLoadFloat4x4(&dx_mat);

        auto col0 = DirectX::XMVector3LengthSq(mat.r[0]);
        auto col1 = DirectX::XMVector3LengthSq(mat.r[1]);
        auto col2 = DirectX::XMVector3LengthSq(mat.r[2]);

        if (DirectX::XMVectorGetX(col0) < 1e-6f ||
            DirectX::XMVectorGetX(col1) < 1e-6f ||
            DirectX::XMVectorGetX(col2) < 1e-6f)
            return {0.0f, 0.0f, 0.0f, 1.0f};

        mat.r[0] = DirectX::XMVector3Normalize(mat.r[0]);
        mat.r[1] = DirectX::XMVector3Normalize(mat.r[1]);
        mat.r[2] = DirectX::XMVector3Normalize(mat.r[2]);

        const auto quat = DirectX::XMQuaternionRotationMatrix(mat);

        DirectX::XMFLOAT4 result;
        DirectX::XMStoreFloat4(&result, quat);
        return std::bit_cast<Vector4>(result);
    }

    inline Matrix4 TranslationMatrix(const Vector3& translation) noexcept
    {
        const auto mat = DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);

        DirectX::XMFLOAT4X4 result;
        DirectX::XMStoreFloat4x4(&result, mat);

        return std::bit_cast<Matrix4>(result);
    }

    inline Vector3 TranslationFromMatrix(const Matrix4& m) noexcept
    {
        auto& data = m.values;
        return {data[12], data[13], data[14]};
    }

    inline Vector4 ToQuaternion(const Vector3& axis, const float angle) noexcept
    {
        const float len_sq = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
        if (len_sq < 1e-12f) return {0.0f, 0.0f, 0.0f, 1.0f};

        auto axis_vector = DirectX::XMVectorSet(axis.x, axis.y, axis.z, 0.0f);
        axis_vector = DirectX::XMVector3Normalize(axis_vector);
        const auto quat = DirectX::XMQuaternionRotationAxis(axis_vector, angle);

        DirectX::XMFLOAT4 result;
        DirectX::XMStoreFloat4(&result, quat);

        return std::bit_cast<Vector4>(result);
    }

    inline Vector4 ToQuaternion(const Vector3& euler) noexcept
    {
        const auto quat = DirectX::XMQuaternionRotationRollPitchYaw(euler.x, euler.y, euler.z);

        DirectX::XMFLOAT4 result;
        DirectX::XMStoreFloat4(&result, quat);

        return std::bit_cast<Vector4>(result);
    }

    inline Vector3 ToEuler(const Vector4& quat) noexcept
    {
        Vector3 result;

        const auto sin_pitch = 2.0f * (quat.w * quat.x - quat.y * quat.z);

        if (sin_pitch <= -1.0f) result.x = -DirectX::XM_PIDIV2;
        else if (sin_pitch >= 1.0f) result.x = DirectX::XM_PIDIV2;
        else result.x = DirectX::XMScalarASin(sin_pitch);

        if (sin_pitch > 0.999999f || sin_pitch < -0.999999f)
        {
            result.y = std::atan2(2.0f * (quat.w * quat.y - quat.x * quat.z),
                                  1.0f - 2.0f * (quat.y * quat.y + quat.z * quat.z));
            result.z = 0.0f;
        }
        else
        {
            result.y = std::atan2(2.0f * (quat.x * quat.z + quat.w * quat.y),
                                  1.0f - 2.0f * (quat.x * quat.x + quat.y * quat.y));
            result.z = std::atan2(2.0f * (quat.x * quat.y + quat.w * quat.z),
                                  1.0f - 2.0f * (quat.x * quat.x + quat.z * quat.z));
        }

        return result;
    }

    inline Vector4 ApplyAngularVelocity(const Vector3& velocity, const float time) noexcept
    {
        const auto vx = velocity.x * time;
        const auto vy = velocity.y * time;
        const auto vz = velocity.z * time;

        const auto len_sq = vx * vx + vy * vy + vz * vz;
        if (len_sq < 1e-12f) return Vector4{.x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f};

        const auto angle = std::sqrt(len_sq);
        auto axis_vector = DirectX::XMVectorSet(vx, vy, vz, 0.0f);
        axis_vector = DirectX::XMVector3Normalize(axis_vector);
        const auto quat = DirectX::XMQuaternionRotationAxis(axis_vector, angle);

        DirectX::XMFLOAT4 result;
        DirectX::XMStoreFloat4(&result, quat);

        return std::bit_cast<Vector4>(result);
    }

    inline Vector4 QuaternionMultiply(const Vector4& a, const Vector4& b) noexcept
    {
        const auto dx_a = DirectX::XMVectorSet(a.x, a.y, a.z, a.w);
        const auto dx_b = DirectX::XMVectorSet(b.x, b.y, b.z, b.w);
        const auto quat = DirectX::XMQuaternionMultiply(dx_a, dx_b);

        DirectX::XMFLOAT4 result;
        DirectX::XMStoreFloat4(&result, quat);

        return std::bit_cast<Vector4>(result);
    }

    inline Vector4 QuaternionConjugate(const Vector4& quat) noexcept
    {
        const auto dx = DirectX::XMVectorSet(quat.x, quat.y, quat.z, quat.w);
        const auto conj = DirectX::XMQuaternionConjugate(dx);

        DirectX::XMFLOAT4 result;
        DirectX::XMStoreFloat4(&result, conj);

        return std::bit_cast<Vector4>(result);
    }

    inline Matrix4 Multiply(const Matrix4& a, const Matrix4& b) noexcept
    {
        const auto dx_a = std::bit_cast<DirectX::XMFLOAT4X4>(a);
        const auto dx_b = std::bit_cast<DirectX::XMFLOAT4X4>(b);

        const auto mat_a = DirectX::XMLoadFloat4x4(&dx_a);
        const auto mat_b = DirectX::XMLoadFloat4x4(&dx_b);
        const auto multi = DirectX::XMMatrixMultiply(mat_a, mat_b);

        DirectX::XMFLOAT4X4 result;
        DirectX::XMStoreFloat4x4(&result, multi);

        return std::bit_cast<Matrix4>(result);
    }

    inline Matrix4 Inverse(const Matrix4& m) noexcept
    {
        const auto dx_mat = std::bit_cast<DirectX::XMFLOAT4X4>(m);
        const auto mat = DirectX::XMLoadFloat4x4(&dx_mat);

        DirectX::XMVECTOR det;
        const auto inv = DirectX::XMMatrixInverse(&det, mat);

        if (std::fabs(DirectX::XMVectorGetX(det)) < 1e-6f) return IdentityMatrix4();

        DirectX::XMFLOAT4X4 result;
        DirectX::XMStoreFloat4x4(&result, inv);

        return std::bit_cast<Matrix4>(result);
    }

    inline Matrix4 RigidInverse(const Matrix4& m) noexcept
    {
        const auto dx_mat = std::bit_cast<DirectX::XMFLOAT4X4>(m);
        const auto mat = DirectX::XMLoadFloat4x4(&dx_mat);

        auto inv = DirectX::XMMatrixTranspose(mat);

        inv.r[0] = DirectX::XMVectorSetW(inv.r[0], 0.0f);
        inv.r[1] = DirectX::XMVectorSetW(inv.r[1], 0.0f);
        inv.r[2] = DirectX::XMVectorSetW(inv.r[2], 0.0f);

        auto neg_t = DirectX::XMVectorNegate(mat.r[3]);

        auto dot_x = DirectX::XMVector3Dot(neg_t, mat.r[0]);
        auto dot_y = DirectX::XMVector3Dot(neg_t, mat.r[1]);
        auto dot_z = DirectX::XMVector3Dot(neg_t, mat.r[2]);

        auto xy = DirectX::XMVectorMergeXY(dot_x, dot_y);
        auto zw = DirectX::XMVectorMergeXY(dot_z, DirectX::XMVectorSplatOne());
        inv.r[3] = DirectX::XMVectorPermute<0, 1, 4, 5>(xy, zw);

        DirectX::XMFLOAT4X4 result;
        DirectX::XMStoreFloat4x4(&result, inv);

        return std::bit_cast<Matrix4>(result);
    }

    inline Matrix4 NormalMatrix(const Matrix4& m) noexcept
    {
        const auto dx_mat = std::bit_cast<DirectX::XMFLOAT4X4>(m);
        const auto mat = DirectX::XMLoadFloat4x4(&dx_mat);

        DirectX::XMVECTOR det;
        const auto inv = DirectX::XMMatrixInverse(&det, mat);

        if (std::fabs(DirectX::XMVectorGetX(det)) < 1e-6f) return IdentityMatrix4();

        const auto inv_t = DirectX::XMMatrixTranspose(inv);

        DirectX::XMFLOAT4X4 result;
        DirectX::XMStoreFloat4x4(&result, inv_t);

        return std::bit_cast<Matrix4>(result);
    }

    inline Matrix4 operator*(const Matrix4& a, const Matrix4& b) noexcept
    {
        return Multiply(a, b);
    }

    inline Matrix4& operator*=(Matrix4& a, const Matrix4& b) noexcept
    {
        a = Multiply(a, b);
        return a;
    }

    inline Vector3 TransformPoint(const Matrix4& m, const Vector3& point) noexcept
    {
        const auto dx_mat = std::bit_cast<DirectX::XMFLOAT4X4>(m);
        const auto dx_point = DirectX::XMVectorSet(point.x, point.y, point.z, 1.0f);

        const auto mat = DirectX::XMLoadFloat4x4(&dx_mat);
        const auto new_point = DirectX::XMVector3Transform(dx_point, mat);

        DirectX::XMFLOAT3 result;
        DirectX::XMStoreFloat3(&result, new_point);

        return std::bit_cast<Vector3>(result);
    }

    inline Vector3 TransformVector(const Matrix4& m, const Vector3& vec) noexcept
    {
        const auto dx_mat = std::bit_cast<DirectX::XMFLOAT4X4>(m);
        const auto dx_vec = DirectX::XMVectorSet(vec.x, vec.y, vec.z, 0.0f);

        const auto mat = DirectX::XMLoadFloat4x4(&dx_mat);
        const auto new_vec = DirectX::XMVector3TransformNormal(dx_vec, mat);

        DirectX::XMFLOAT3 result;
        DirectX::XMStoreFloat3(&result, new_vec);

        return std::bit_cast<Vector3>(result);
    }

    inline Vector3 ToForward(const Vector4& quat) noexcept
    {
        const auto dx_quat = DirectX::XMVectorSet(quat.x, quat.y, quat.z, quat.w);
        const auto dx_forward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

        const auto rotated = DirectX::XMVector3Rotate(dx_forward, dx_quat);

        DirectX::XMFLOAT3 result;
        DirectX::XMStoreFloat3(&result, rotated);

        return std::bit_cast<Vector3>(result);
    }

    inline Vector3 ToForward(const Matrix4& m) noexcept
    {
        const auto dx_mat = std::bit_cast<DirectX::XMFLOAT4X4>(m);
        auto mat = DirectX::XMLoadFloat4x4(&dx_mat);

        auto dx_forward = mat.r[2];

        auto len_sq_vec = DirectX::XMVector3LengthSq(dx_forward);
        if (DirectX::XMVectorGetX(len_sq_vec) < 1e-6f) return {0.0f, 0.0f, 1.0f};

        dx_forward = DirectX::XMVector3Normalize(dx_forward);

        DirectX::XMFLOAT3 result;
        DirectX::XMStoreFloat3(&result, dx_forward);

        return std::bit_cast<Vector3>(result);
    }

    inline Vector3 ToRight(const Vector4& quat) noexcept
    {
        const auto dx_quat = DirectX::XMVectorSet(quat.x, quat.y, quat.z, quat.w);
        const auto dx_right = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

        const auto rotated = DirectX::XMVector3Rotate(dx_right, dx_quat);

        DirectX::XMFLOAT3 result;
        DirectX::XMStoreFloat3(&result, rotated);

        return std::bit_cast<Vector3>(result);
    }

    inline Vector3 ToRight(const Matrix4& m) noexcept
    {
        const auto dx_mat = std::bit_cast<DirectX::XMFLOAT4X4>(m);
        auto mat = DirectX::XMLoadFloat4x4(&dx_mat);

        auto dx_right = mat.r[0];

        auto len_sq_vec = DirectX::XMVector3LengthSq(dx_right);
        if (DirectX::XMVectorGetX(len_sq_vec) < 1e-6f) return {1.0f, 0.0f, 0.0f};

        dx_right = DirectX::XMVector3Normalize(dx_right);

        DirectX::XMFLOAT3 result;
        DirectX::XMStoreFloat3(&result, dx_right);

        return std::bit_cast<Vector3>(result);
    }

    inline Matrix4 PerspectiveMatrix(float fov, float aspect, float near, float far) noexcept
    {
        const auto dx_proj = DirectX::XMMatrixPerspectiveFovLH(fov, aspect, near, far);

        DirectX::XMFLOAT4X4 result;
        DirectX::XMStoreFloat4x4(&result, dx_proj);

        return std::bit_cast<Matrix4>(result);
    }

    inline Matrix4 OrthographicMatrix(float width, float height, float near, float far) noexcept
    {
        const auto dx_proj = DirectX::XMMatrixOrthographicLH(width, height, near, far);

        DirectX::XMFLOAT4X4 result;
        DirectX::XMStoreFloat4x4(&result, dx_proj);

        return std::bit_cast<Matrix4>(result);
    }

    inline Matrix4 LookAtMatrix(
        const Vector3& eye,
        const Vector3& target,
        const Vector3& up = Vector3{.x = 0.0f, .y = 1.0f, .z = 0.0f}) noexcept
    {
        const auto dx_eye = DirectX::XMVectorSet(eye.x, eye.y, eye.z, 0.0f);
        const auto dx_target = DirectX::XMVectorSet(target.x, target.y, target.z, 0.0f);

        auto dx_up = DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f);
        auto dx_forward = DirectX::XMVectorSubtract(dx_target, dx_eye);

        auto len_sq_vec = DirectX::XMVector3LengthSq(dx_forward);
        if (DirectX::XMVectorGetX(len_sq_vec) < 1e-6f) return IdentityMatrix4();

        dx_forward = DirectX::XMVector3Normalize(dx_forward);

        const auto dot_product = DirectX::XMVector3Dot(dx_forward, dx_up);
        if (std::fabs(DirectX::XMVectorGetX(dot_product)) > 0.999f)
            dx_up = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

        auto view = DirectX::XMMatrixLookToLH(dx_eye, dx_forward, dx_up);

        DirectX::XMFLOAT4X4 result;
        DirectX::XMStoreFloat4x4(&result, view);

        return std::bit_cast<Matrix4>(result);
    }

    inline Vector4 LookAtQuaternion(
        const Vector3& eye,
        const Vector3& target,
        const Vector3& up = Vector3{.x = 0.0f, .y = 1.0f, .z = 0.0f}) noexcept
    {
        const auto dx_eye = DirectX::XMVectorSet(eye.x, eye.y, eye.z, 0.0f);
        const auto dx_target = DirectX::XMVectorSet(target.x, target.y, target.z, 0.0f);

        auto dx_up = DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f);
        auto dx_forward = DirectX::XMVectorSubtract(dx_target, dx_eye);

        auto len_sq_vec = DirectX::XMVector3LengthSq(dx_forward);
        if (DirectX::XMVectorGetX(len_sq_vec) < 1e-6f) return {0.0f, 0.0f, 0.0f, 1.0f};

        dx_forward = DirectX::XMVector3Normalize(dx_forward);

        const auto dot_product = DirectX::XMVector3Dot(dx_forward, dx_up);
        if (std::fabs(DirectX::XMVectorGetX(dot_product)) > 0.999f)
            dx_up = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

        auto mat_right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(dx_up, dx_forward));
        auto mat_up = DirectX::XMVector3Cross(dx_forward, mat_right);

        DirectX::XMMATRIX rotation = DirectX::XMMatrixIdentity();
        rotation.r[0] = mat_right;
        rotation.r[1] = mat_up;
        rotation.r[2] = dx_forward;

        auto quat = DirectX::XMQuaternionRotationMatrix(rotation);

        DirectX::XMFLOAT4 result;
        DirectX::XMStoreFloat4(&result, quat);

        return std::bit_cast<Vector4>(result);
    }
}

#endif //NEXENGINE_MATHUTILS_H
