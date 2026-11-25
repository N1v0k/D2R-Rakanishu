using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Threading;

namespace Ryujinx.Common.SystemInterop
{




    [SupportedOSPlatform("windows")]
    internal partial class WindowsGranularTimer
    {
        private const int MinimumGranularity = 5000;

        private static readonly WindowsGranularTimer _instance = new();
        public static WindowsGranularTimer Instance => _instance;

        private readonly struct WaitingObject
        {
            public readonly long Id;
            public readonly EventWaitHandle Signal;
            public readonly long TimePoint;

            public WaitingObject(long id, EventWaitHandle signal, long timePoint)
            {
                Id = id;
                Signal = signal;
                TimePoint = timePoint;
            }
        }

        [LibraryImport("ntdll.dll", SetLastError = true)]
        private static partial int NtSetTimerResolution(int DesiredResolution, [MarshalAs(UnmanagedType.Bool)] bool SetResolution, out int CurrentResolution);

        [LibraryImport("ntdll.dll", SetLastError = true)]
        private static partial int NtQueryTimerResolution(out int MaximumResolution, out int MinimumResolution, out int CurrentResolution);

        [LibraryImport("ntdll.dll", SetLastError = true)]
        private static partial uint NtDelayExecution([MarshalAs(UnmanagedType.Bool)] bool Alertable, ref long DelayInterval);

        public long GranularityNs => _granularityNs;
        public long GranularityTicks => _granularityTicks;

        private readonly Thread _timerThread;
        private long _granularityNs = MinimumGranularity * 100L;
        private long _granularityTicks;
        private long _lastTicks = PerformanceCounter.ElapsedTicks;
        private long _lastId;

        private readonly Lock _lock = new();
        private readonly List<WaitingObject> _waitingObjects = [];

        private WindowsGranularTimer()
        {
            _timerThread = new Thread(Loop)
            {
                IsBackground = true,
                Name = "Common.WindowsTimer",
                Priority = ThreadPriority.Highest
            };

            _timerThread.Start();
        }




        private void Initialize()
        {
            NtQueryTimerResolution(out _, out int min, out int curr);

            if (min > 0)
            {
                min = Math.Max(min, MinimumGranularity);

                _granularityNs = min * 100L;
                NtSetTimerResolution(min, true, out _);
            }
            else
            {
                _granularityNs = curr * 100L;
            }

            _granularityTicks = (_granularityNs * PerformanceCounter.TicksPerMillisecond) / 1_000_000;
        }





        private void Loop()
        {
            Initialize();
            while (true)
            {
                long delayInterval = -1;
                NtSetTimerResolution((int)(_granularityNs / 100), true, out _);
                NtDelayExecution(false, ref delayInterval);

                long newTicks = PerformanceCounter.ElapsedTicks;
                long nextTicks = newTicks + _granularityTicks;

                lock (_lock)
                {
                    for (int i = 0; i < _waitingObjects.Count; i++)
                    {
                        if (nextTicks > _waitingObjects[i].TimePoint)
                        {

                            _waitingObjects[i].Signal.Set();

                            _waitingObjects.RemoveAt(i--);
                        }
                    }

                    _lastTicks = newTicks;
                }
            }
        }







        public bool SleepUntilTimePoint(AutoResetEvent evt, long timePoint)
        {
            if (evt.WaitOne(0))
            {
                return true;
            }

            long id;

            lock (_lock)
            {

                long nextTicks = _lastTicks + _granularityTicks;

                if (nextTicks > timePoint)
                {
                    return false;
                }

                id = ++_lastId;

                _waitingObjects.Add(new WaitingObject(id, evt, timePoint));
            }

            evt.WaitOne();

            lock (_lock)
            {
                for (int i = 0; i < _waitingObjects.Count; i++)
                {
                    if (id == _waitingObjects[i].Id)
                    {
                        _waitingObjects.RemoveAt(i--);
                        break;
                    }
                }
            }

            return true;
        }










        public bool SleepUntilTimePointWithoutExternalSignal(EventWaitHandle evt, long timePoint)
        {
            long id;

            lock (_lock)
            {

                long nextTicks = _lastTicks + _granularityTicks;

                if (nextTicks > timePoint)
                {
                    return false;
                }

                id = ++_lastId;

                _waitingObjects.Add(new WaitingObject(id, evt, timePoint));
            }

            evt.WaitOne();

            return true;
        }






        public (long, long) ReturnNearestTicks(long timePoint)
        {
            long last = _lastTicks;
            long delta = timePoint - last;

            long lowTicks = delta / _granularityTicks;
            long highTicks = (delta + _granularityTicks - 1) / _granularityTicks;

            return (last + lowTicks * _granularityTicks, last + highTicks * _granularityTicks);
        }
    }
}
