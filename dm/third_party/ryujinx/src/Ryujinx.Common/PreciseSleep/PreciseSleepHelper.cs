using Ryujinx.Common.SystemInterop;
using System;
using System.Threading;

namespace Ryujinx.Common.PreciseSleep
{
    public static class PreciseSleepHelper
    {




        public static IPreciseSleepEvent CreateEvent()
        {
            if (OperatingSystem.IsLinux() || OperatingSystem.IsMacOS() || OperatingSystem.IsIOS() || OperatingSystem.IsAndroid())
            {
                return new NanosleepEvent();
            }
            else if (OperatingSystem.IsWindows())
            {
                return new WindowsSleepEvent();
            }
            else
            {
                return new SleepEvent();
            }
        }







        public static void SleepUntilTimePoint(EventWaitHandle evt, long timePoint)
        {
            if (OperatingSystem.IsWindows())
            {
                WindowsGranularTimer.Instance.SleepUntilTimePointWithoutExternalSignal(evt, timePoint);
            }
            else
            {



                long accuracyBias = PerformanceCounter.TicksPerMillisecond / 2;
                long now = PerformanceCounter.ElapsedTicks + accuracyBias;
                long ms = Math.Min((timePoint - now) / PerformanceCounter.TicksPerMillisecond, int.MaxValue);

                if (ms > 0)
                {
                    evt.WaitOne((int)ms);
                }

                if (OperatingSystem.IsLinux() || OperatingSystem.IsMacOS() || OperatingSystem.IsIOS() || OperatingSystem.IsAndroid())
                {

                    now = PerformanceCounter.ElapsedTicks;
                    long ns = ((timePoint - now) * 1_000_000) / PerformanceCounter.TicksPerMillisecond;

                    Nanosleep.SleepAtMost(ns);
                }
            }
        }







        public static void SpinWaitUntilTimePoint(long timePoint, ref long wakeSignal)
        {
            SpinWait spinWait = new();

            while (Interlocked.Read(ref wakeSignal) != 1 && PerformanceCounter.ElapsedTicks < timePoint)
            {

                if (spinWait.NextSpinWillYield)
                {
                    Thread.Yield();

                    spinWait.Reset();
                }
                else
                {
                    spinWait.SpinOnce();
                }
            }
        }





        public static void SpinWaitUntilTimePoint(long timePoint)
        {
            while (PerformanceCounter.ElapsedTicks < timePoint)
            {
                Thread.SpinWait(5);
            }
        }
    }
}
