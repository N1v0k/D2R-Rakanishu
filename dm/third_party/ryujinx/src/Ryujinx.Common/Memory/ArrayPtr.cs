using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ryujinx.Common.Memory
{




    public unsafe struct ArrayPtr<T> : IEquatable<ArrayPtr<T>>, IArray<T> where T : unmanaged
    {
        private nint _ptr;




        public static ArrayPtr<T> Null => new() { _ptr = nint.Zero };




        public readonly bool IsNull => _ptr == nint.Zero;




        public int Length { get; }










        public readonly ref T this[int index] => ref Unsafe.AsRef<T>((T*)_ptr + index);










        public ArrayPtr(ref T value, int length)
        {
            _ptr = (nint)Unsafe.AsPointer(ref value);
            Length = length;
        }






        public ArrayPtr(T* ptr, int length)
        {
            _ptr = (nint)ptr;
            Length = length;
        }






        public ArrayPtr(nint ptr, int length)
        {
            _ptr = ptr;
            Length = length;
        }






        public ArrayPtr<T> Slice(int start) => new(ref this[start], Length - start);





        public Span<T> AsSpan() => Length == 0 ? Span<T>.Empty : MemoryMarshal.CreateSpan(ref this[0], Length);





        public readonly T* ToPointer() => (T*)_ptr;

        public readonly override bool Equals(object obj)
        {
            return obj is ArrayPtr<T> other && Equals(other);
        }

        public readonly bool Equals([AllowNull] ArrayPtr<T> other)
        {
            return _ptr == other._ptr && Length == other.Length;
        }

        public readonly override int GetHashCode()
        {
            return HashCode.Combine(_ptr, Length);
        }

        public static bool operator ==(ArrayPtr<T> left, ArrayPtr<T> right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(ArrayPtr<T> left, ArrayPtr<T> right)
        {
            return !(left == right);
        }
    }
}
