using System;

namespace CodexEditor.Util
{
    public static class MathUtil
    {
         public static float Epsilon => 0.00001f;
        
        public static bool IsEqualTo(this float value, float other)
        {
            return Math.Abs(value - other) < Epsilon;
        }

        public static bool IsEqualTo(this float? value, float? other)
        {
            if (!value.HasValue || !other.HasValue) return false;
            return Math.Abs(value.Value - other.Value) < Epsilon;
        }
    }
}
