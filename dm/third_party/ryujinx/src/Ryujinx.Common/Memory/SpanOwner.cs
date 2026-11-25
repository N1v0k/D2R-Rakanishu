using System;
using System.Buffers;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ryujinx.Common.Memory
{








    public readonly ref struct SpanOwner<T>
    {
        private readonly int _length;
        private readonly T[] _array;





        private SpanOwner(int length)
        {
            _length = length;
            _array = ArrayPool<T>.Shared.Rent(length);
        }




        public static SpanOwner<T> Empty
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => new(0);
        }







        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static SpanOwner<T> Rent(int length) => new(length);







        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static SpanOwner<T> RentCleared(int length)
        {
            SpanOwner<T> result = new(length);

            result._array.AsSpan(0, length).Clear();

            return result;
        }






        public static SpanOwner<T> RentCopy(ReadOnlySpan<T> buffer)
        {
            SpanOwner<T> result = new(buffer.Length);

            buffer.CopyTo(result._array);

            return result;
        }




        public int Length
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _length;
        }







        public Span<T> Span
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                ref T firstElementRef = ref MemoryMarshal.GetArrayDataReference(_array);

                return MemoryMarshal.CreateSpan(ref firstElementRef, _length);
            }
        }




        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void Dispose()
        {
            ArrayPool<T>.Shared.Return(_array, RuntimeHelpers.IsReferenceOrContainsReferences<T>());
        }
    }
}
