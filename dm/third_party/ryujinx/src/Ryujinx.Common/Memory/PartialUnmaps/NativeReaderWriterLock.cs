using System.Runtime.InteropServices;
using System.Threading;
using static Ryujinx.Common.Memory.PartialUnmaps.PartialUnmapHelpers;

namespace Ryujinx.Common.Memory.PartialUnmaps
{



    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct NativeReaderWriterLock
    {
        public int WriteLock;
        public int ReaderCount;

        public static readonly int WriteLockOffset;
        public static readonly int ReaderCountOffset;




        static NativeReaderWriterLock()
        {
            NativeReaderWriterLock instance = new();

            WriteLockOffset = OffsetOf(ref instance, ref instance.WriteLock);
            ReaderCountOffset = OffsetOf(ref instance, ref instance.ReaderCount);
        }




        public void AcquireReaderLock()
        {


            while (Interlocked.CompareExchange(ref WriteLock, 1, 0) != 0)
            {
            }

            Interlocked.Increment(ref ReaderCount);

            Interlocked.Exchange(ref WriteLock, 0);
        }




        public void ReleaseReaderLock()
        {
            Interlocked.Decrement(ref ReaderCount);
        }




        public void UpgradeToWriterLock()
        {



            Interlocked.Decrement(ref ReaderCount);

            while (Interlocked.CompareExchange(ref WriteLock, 1, 0) != 0)
            {
            }



            while (Interlocked.CompareExchange(ref ReaderCount, 1, 0) != 0)
            {
            }
        }




        public void DowngradeFromWriterLock()
        {


            Interlocked.Exchange(ref WriteLock, 0);
        }
    }
}
