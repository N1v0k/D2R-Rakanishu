using System.Diagnostics;

namespace Ryujinx.Common
{
    public static class PerformanceCounter
    {
        private static readonly double _ticksToNs;




        public static long TicksPerDay { get; }




        public static long TicksPerHour { get; }




        public static long TicksPerMinute { get; }




        public static long TicksPerSecond { get; }




        public static long TicksPerMillisecond { get; }




        public static long ElapsedTicks
        {
            get
            {
                return Stopwatch.GetTimestamp();
            }
        }




        public static long ElapsedMilliseconds
        {
            get
            {
                long timestamp = Stopwatch.GetTimestamp();

                return timestamp / TicksPerMillisecond;
            }
        }




        public static long ElapsedNanoseconds
        {
            get
            {
                long timestamp = Stopwatch.GetTimestamp();

                return (long)(timestamp * _ticksToNs);
            }
        }

        static PerformanceCounter()
        {
            TicksPerMillisecond = Stopwatch.Frequency / 1000;
            TicksPerSecond = Stopwatch.Frequency;
            TicksPerMinute = TicksPerSecond * 60;
            TicksPerHour = TicksPerMinute * 60;
            TicksPerDay = TicksPerHour * 24;

            _ticksToNs = 1000000000.0 / Stopwatch.Frequency;
        }
    }
}
