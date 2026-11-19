using System;
using System.Runtime.InteropServices;
using System.Threading;
using static Ryujinx.Common.Memory.PartialUnmaps.PartialUnmapHelpers;

namespace Ryujinx.Common.Memory.PartialUnmaps
{





    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct ThreadLocalMap<T> where T : unmanaged
    {
        public const int MapSize = 20;

        public Array20<int> ThreadIds;
        public Array20<T> Structs;

        public static readonly int ThreadIdsOffset;
        public static readonly int StructsOffset;




        static ThreadLocalMap()
        {
            ThreadLocalMap<T> instance = new();

            ThreadIdsOffset = OffsetOf(ref instance, ref instance.ThreadIds);
            StructsOffset = OffsetOf(ref instance, ref instance.Structs);
        }









        public int GetOrReserve(int threadId, T initial)
        {

            
            Span<int> threadIdsSpan = ThreadIds.AsSpan();
            Span<T> structsSpan = Structs.AsSpan();

            for (int i = 0; i < MapSize; i++)
            {
                int compare = Interlocked.CompareExchange(ref threadIdsSpan[i], threadId, threadId);

                if (compare == threadId)
                {
                    return i;
                }
            }



            for (int i = 0; i < MapSize; i++)
            {
                int compare = Interlocked.CompareExchange(ref threadIdsSpan[i], threadId, 0);

                if (compare == 0)
                {
                    structsSpan[i] = initial;
                    return i;
                }
            }

            return -1;
        }






        public ref T GetValue(int index)
        {
            return ref Structs[index];
        }





        public void Release(int index)
        {
            Interlocked.Exchange(ref ThreadIds[index], 0);
        }
    }
}
