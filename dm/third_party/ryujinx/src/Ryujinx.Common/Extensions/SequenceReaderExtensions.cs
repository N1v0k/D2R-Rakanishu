using System;
using System.Buffers;
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ryujinx.Common.Extensions
{
    public static class SequenceReaderExtensions
    {






        public static void DumpToFile(this ref SequenceReader<byte> reader, string fileFullName)
        {
            long initialConsumed = reader.Consumed;

            reader.Rewind(initialConsumed);

            using (FileStream fileStream = System.IO.File.Create(fileFullName, 4096, System.IO.FileOptions.None))
            {
                while (reader.End == false)
                {
                    ReadOnlySpan<byte> span = reader.CurrentSpan;
                    fileStream.Write(span);
                    reader.Advance(span.Length);
                }
            }

            reader.SetConsumed(initialConsumed);
        }


















        public static ref readonly T GetRefOrRefToCopy<T>(this scoped ref SequenceReader<byte> reader, out T copyDestinationIfRequiredDoNotUse) where T : unmanaged
        {
            int lengthRequired = Unsafe.SizeOf<T>();

            ReadOnlySpan<byte> span = reader.UnreadSpan;
            if (lengthRequired <= span.Length)
            {
                reader.Advance(lengthRequired);

                copyDestinationIfRequiredDoNotUse = default;

                ReadOnlySpan<T> spanOfT = MemoryMarshal.Cast<byte, T>(span);

                return ref spanOfT[0];
            }
            else
            {
                copyDestinationIfRequiredDoNotUse = default;

                Span<T> valueSpan = MemoryMarshal.CreateSpan(ref copyDestinationIfRequiredDoNotUse, 1);

                Span<byte> valueBytesSpan = MemoryMarshal.AsBytes(valueSpan);

                if (!reader.TryCopyTo(valueBytesSpan))
                {
                    throw new ArgumentOutOfRangeException(nameof(reader), "The sequence is not long enough to read the desired value.");
                }

                reader.Advance(lengthRequired);

                return ref valueSpan[0];
            }
        }







        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void ReadLittleEndian(this ref SequenceReader<byte> reader, out int value)
        {
            if (!reader.TryReadLittleEndian(out value))
            {
                throw new ArgumentOutOfRangeException(nameof(value), "The sequence is not long enough to read the desired value.");
            }
        }








        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void ReadUnmanaged<T>(this ref SequenceReader<byte> reader, out T value) where T : unmanaged
        {
            if (!reader.TryReadUnmanaged(out value))
            {
                throw new ArgumentOutOfRangeException(nameof(value), "The sequence is not long enough to read the desired value.");
            }
        }






        public static void SetConsumed(ref this SequenceReader<byte> reader, long consumed)
        {
            reader.Rewind(reader.Consumed);
            reader.Advance(consumed);
        }














        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static unsafe bool TryReadUnmanaged<T>(ref this SequenceReader<byte> reader, out T value) where T : unmanaged
        {
            ReadOnlySpan<byte> span = reader.UnreadSpan;

            if (span.Length < sizeof(T))
            {
                return TryReadUnmanagedMultiSegment(ref reader, out value);
            }

            value = Unsafe.ReadUnaligned<T>(ref MemoryMarshal.GetReference(span));

            reader.Advance(sizeof(T));

            return true;
        }

        private static unsafe bool TryReadUnmanagedMultiSegment<T>(ref SequenceReader<byte> reader, out T value) where T : unmanaged
        {
            Debug.Assert(reader.UnreadSpan.Length < sizeof(T));


            T buffer = default;

            Span<byte> tempSpan = new(&buffer, sizeof(T));

            if (!reader.TryCopyTo(tempSpan))
            {
                value = default;
                return false;
            }

            value = Unsafe.ReadUnaligned<T>(ref MemoryMarshal.GetReference(tempSpan));

            reader.Advance(sizeof(T));

            return true;
        }
    }
}
