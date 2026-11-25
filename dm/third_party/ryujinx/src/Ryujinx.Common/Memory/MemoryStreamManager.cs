using Microsoft.IO;
using System;

namespace Ryujinx.Common.Memory
{
    public static class MemoryStreamManager
    {
        private static readonly RecyclableMemoryStreamManager _shared = new();

        private static readonly ObjectPool<RecyclableMemoryStream> _streamPool =
            new(() => new RecyclableMemoryStream(_shared, Guid.NewGuid(), null, 0));






        public static class Shared
        {




            public static RecyclableMemoryStream GetStream()
            {
                RecyclableMemoryStream stream = _streamPool.Allocate();
                stream.SetLength(0);

                return stream;
            }








            public static RecyclableMemoryStream GetStream(byte[] buffer)
                => GetStream(Guid.NewGuid(), null, buffer, 0, buffer.Length);








            public static RecyclableMemoryStream GetStream(ReadOnlySpan<byte> buffer)
                => GetStream(Guid.NewGuid(), null, buffer);










            public static RecyclableMemoryStream GetStream(Guid id, string tag, ReadOnlySpan<byte> buffer)
            {
                RecyclableMemoryStream stream = null;
                try
                {
                    stream = _streamPool.Allocate();
                    stream.SetLength(0);
                    stream.Write(buffer);
                    stream.Position = 0;
                    return stream;
                }
                catch
                {
                    stream?.Dispose();
                    throw;
                }
            }












            public static RecyclableMemoryStream GetStream(Guid id, string tag, byte[] buffer, int offset, int count)
            {
                RecyclableMemoryStream stream = null;
                try
                {
                    stream = _streamPool.Allocate();
                    stream.SetLength(0);
                    stream.Write(buffer, offset, count);
                    stream.Position = 0;
                    return stream;
                }
                catch
                {
                    stream?.Dispose();
                    throw;
                }
            }

            public static void ReleaseStream(RecyclableMemoryStream stream)
            {
                _streamPool.Release(stream);
            }
        }
    }
}
