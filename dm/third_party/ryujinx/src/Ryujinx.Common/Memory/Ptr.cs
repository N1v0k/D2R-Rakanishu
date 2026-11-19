using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;

namespace Ryujinx.Common.Memory
{




    public unsafe struct Ptr<T> : IEquatable<Ptr<T>> where T : unmanaged
    {
        private nint _ptr;




        public static Ptr<T> Null => new() { _ptr = nint.Zero };




        public readonly bool IsNull => _ptr == nint.Zero;




        public readonly ref T Value => ref Unsafe.AsRef<T>((void*)_ptr);









        public Ptr(ref T value)
        {
            _ptr = (nint)Unsafe.AsPointer(ref value);
        }

        public readonly override bool Equals(object obj)
        {
            return obj is Ptr<T> other && Equals(other);
        }

        public readonly bool Equals([AllowNull] Ptr<T> other)
        {
            return _ptr == other._ptr;
        }

        public readonly override int GetHashCode()
        {
            return _ptr.GetHashCode();
        }

        public static bool operator ==(Ptr<T> left, Ptr<T> right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(Ptr<T> left, Ptr<T> right)
        {
            return !(left == right);
        }
    }
}
