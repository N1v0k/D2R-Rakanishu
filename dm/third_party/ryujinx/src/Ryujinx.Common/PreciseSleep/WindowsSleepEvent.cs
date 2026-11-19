using Ryujinx.Common.SystemInterop;
using System;
using System.Runtime.Versioning;
using System.Threading;

namespace Ryujinx.Common.PreciseSleep
{




    [SupportedOSPlatform("windows")]
    internal class WindowsSleepEvent : IPreciseSleepEvent
    {



        private const long ErrorBias = 50000;




        private const long ClockAlignedBias = 50000;





        private const long ReverseTimePointFraction = 4;

        private readonly AutoResetEvent _waitEvent = new(false);
        private readonly WindowsGranularTimer _timer = WindowsGranularTimer.Instance;




        public bool Precise { get; set; } = false;

        public long AdjustTimePoint(long timePoint, long timeoutNs)
        {
            if (Precise || timePoint == long.MaxValue)
            {
                return timePoint;
            }



            long granularity = _timer.GranularityNs;
            long misalignment = timeoutNs % granularity;

            if ((misalignment < ClockAlignedBias || misalignment > granularity - ClockAlignedBias) && timeoutNs > ClockAlignedBias)
            {


                (long low, long high) = _timer.ReturnNearestTicks(timePoint);

                if (timePoint - low < _timer.GranularityTicks / ReverseTimePointFraction)
                {
                    timePoint = low;
                }
                else
                {
                    timePoint = high;
                }
            }

            return timePoint;
        }

        public bool SleepUntil(long timePoint)
        {
            return _timer.SleepUntilTimePoint(_waitEvent, timePoint + (ErrorBias * PerformanceCounter.TicksPerMillisecond) / 1_000_000);
        }

        public void Sleep()
        {
            _waitEvent.WaitOne();
        }

        public void Signal()
        {
            _waitEvent.Set();
        }

        public void Dispose()
        {
            GC.SuppressFinalize(this);

            _waitEvent.Dispose();
        }
    }
}
