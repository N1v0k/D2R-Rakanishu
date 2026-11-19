using System;

namespace Ryujinx.Common.PreciseSleep
{




    public interface IPreciseSleepEvent : IDisposable
    {







        long AdjustTimePoint(long timePoint, long timeoutNs);







        bool SleepUntil(long timePoint);




        void Sleep();




        void Signal();
    }
}
