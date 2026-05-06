#include "public/math.h"

namespace codex::math {
    bool transform_decompose(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
    {
        // From Hazel (https://github.com/TheCherno/Hazel) which inturn is from glm::decompose in matrix_decompose.inl

        using T = float;

        glm::mat4 local_matrix(transform);

        // Normalize the matrix.
        if (glm::epsilonEqual(local_matrix[3][3], static_cast<float>(0), glm::epsilon<T>()))
            return false;

        // First, isolate perspective.  This is the messiest.
        if (glm::epsilonNotEqual(local_matrix[0][3], static_cast<T>(0), glm::epsilon<T>()) ||
            glm::epsilonNotEqual(local_matrix[1][3], static_cast<T>(0), glm::epsilon<T>()) ||
            glm::epsilonNotEqual(local_matrix[2][3], static_cast<T>(0), glm::epsilon<T>())) {
            // Clear the perspective partition
            local_matrix[0][3] = local_matrix[1][3] = local_matrix[2][3] = static_cast<T>(0);
            local_matrix[3][3]                                           = static_cast<T>(1);
        }

        // Next take care of translation (easy).
        translation     = glm::vec3(local_matrix[3]);
        local_matrix[3] = glm::vec4(0, 0, 0, local_matrix[3].w);

        [[maybe_unused]] glm::vec3 row[3], pdum3;

        // Now get scale and shear.
        for (glm::length_t i = 0; i < 3; ++i)
            for (glm::length_t j = 0; j < 3; ++j)
                row[i][j] = local_matrix[i][j];

        // Compute X scale factor and normalize first row.
        scale.x = glm::length(row[0]);
        row[0]  = glm::detail::scale(row[0], static_cast<T>(1));
        scale.y = glm::length(row[1]);
        row[1]  = glm::detail::scale(row[1], static_cast<T>(1));
        scale.z = glm::length(row[2]);
        row[2]  = glm::detail::scale(row[2], static_cast<T>(1));

        // At this point, the matrix (in rows[]) is orthonormal.
        // Check for a coordinate system flip.  If the determinant
        // is -1, then negate the matrix and the scaling factors.
#if 0
        pdum3 = glm::cross(row[1], row[2]); // v3Cross(row[1], row[2], pdum3);
        if (glm::dot(row[0], pdum3) < 0)
        {
            for (length_t i = 0; i < 3; i++)
            {
                scale[i] *= static_cast<T>(-1);
                row[i] *= static_cast<T>(-1);
            }
        }
#endif

        rotation.y = asin(-row[0][2]);
        if (cos(rotation.y) != 0) {
            rotation.x = atan2(row[1][2], row[2][2]);
            rotation.z = atan2(row[0][1], row[0][0]);
        } else {
            rotation.x = atan2(-row[2][0], row[1][1]);
            rotation.z = 0;
        }

        return true;
    }
} // namespace codex::math
