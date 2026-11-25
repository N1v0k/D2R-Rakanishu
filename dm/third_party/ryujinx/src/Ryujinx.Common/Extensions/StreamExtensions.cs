using System;
using System.Buffers.Binary;
using System.IO;
using System.Runtime.InteropServices;

namespace Ryujinx.Common
{
    public static class StreamExtensions
    {








        public static void Write(this Stream stream, ReadOnlySpan<int> buffer)
        {
            if (buffer.Length == 0)
            {
                return;
            }

            if (BitConverter.IsLittleEndian)
            {
                ReadOnlySpan<byte> byteBuffer = MemoryMarshal.Cast<int, byte>(buffer);
                stream.Write(byteBuffer);
            }
            else
            {
                Span<byte> byteBuffer = stackalloc byte[sizeof(int)];

                foreach (int value in buffer)
                {
                    BinaryPrimitives.WriteInt32LittleEndian(byteBuffer, value);
                    stream.Write(byteBuffer);
                }
            }
        }







        public static void Write(this Stream stream, int value)
        {
            Span<byte> buffer = stackalloc byte[sizeof(int)];
            BinaryPrimitives.WriteInt32LittleEndian(buffer, value);
            stream.Write(buffer);
        }







        public static void Write(this Stream stream, long value)
        {
            Span<byte> buffer = stackalloc byte[sizeof(long)];
            BinaryPrimitives.WriteInt64LittleEndian(buffer, value);
            stream.Write(buffer);
        }







        public static void Write(this Stream stream, uint value)
        {
            Span<byte> buffer = stackalloc byte[sizeof(uint)];
            BinaryPrimitives.WriteUInt32LittleEndian(buffer, value);
            stream.Write(buffer);
        }







        public static void Write(this Stream stream, ulong value)
        {
            Span<byte> buffer = stackalloc byte[sizeof(ulong)];
            BinaryPrimitives.WriteUInt64LittleEndian(buffer, value);
            stream.Write(buffer);
        }







        public static void Write(this Stream stream, Stream source)
        {
            source.CopyTo(stream);
        }







        public static void WriteByte(this Stream stream, byte value, int count)
        {
            if (count <= 0)
            {
                return;
            }

            const int BlockSize = 16;

            int blockCount = count / BlockSize;
            if (blockCount > 0)
            {
                Span<byte> span = stackalloc byte[BlockSize];
                span.Fill(value);
                for (int x = 0; x < blockCount; x++)
                {
                    stream.Write(span);
                }
            }

            int nonBlockBytes = count % BlockSize;
            for (int x = 0; x < nonBlockBytes; x++)
            {
                stream.WriteByte(value);
            }
        }
    }
}
