using System;
using System.Collections.Generic;
using System.Runtime.Versioning;
using System.Threading;

namespace Ryujinx.Common.PreciseSleep
{



    [SupportedOSPlatform("macos")]
    [SupportedOSPlatform("linux")]
    [SupportedOSPlatform("android")]
    [SupportedOSPlatform("ios")]
    internal class NanosleepPool : IDisposable
    {
        public const int MaxThreads = 8;






        private class NanosleepThread : IDisposable
        {
            private static readonly long _timePointEpsilon;

            static NanosleepThread()
            {
                _timePointEpsilon = PerformanceCounter.TicksPerMillisecond / 100;
            }

            private readonly Thread _thread;
            private readonly NanosleepPool _parent;
            private readonly AutoResetEvent _newWaitEvent;
            private bool _running = true;

            private long _signalId;
            private long _nanoseconds;
            private long _timePoint;

            public long SignalId => _signalId;






            public NanosleepThread(NanosleepPool parent, int id)
            {
                _parent = parent;
                _newWaitEvent = new(false);

                _thread = new Thread(Loop)
                {
                    Name = $"Common.Nanosleep.{id}",
                    Priority = ThreadPriority.AboveNormal,
                    IsBackground = true
                };

                _thread.Start();
            }




            private void Loop()
            {
                _newWaitEvent.WaitOne();

                while (_running)
                {
                    Nanosleep.Sleep(_nanoseconds);

                    _parent.Signal(this);
                    _newWaitEvent.WaitOne();
                }

                _newWaitEvent.Dispose();
            }







            public void SleepAndSignal(long nanoseconds, long signalId, long timePoint)
            {
                _signalId = signalId;
                _nanoseconds = nanoseconds;
                _timePoint = timePoint;
                _newWaitEvent.Set();
            }







            public bool Resurrect(long signalId, long timePoint)
            {
                if (Math.Abs(timePoint - _timePoint) < _timePointEpsilon)
                {
                    _signalId = signalId;

                    return true;
                }

                return false;
            }




            public void Dispose()
            {
                if (_running)
                {
                    _running = false;
                    _newWaitEvent.Set();
                }
            }
        }

        private readonly Lock _lock = new();
        private readonly List<NanosleepThread> _threads = [];
        private readonly List<NanosleepThread> _active = [];
        private readonly Stack<NanosleepThread> _free = new();
        private readonly AutoResetEvent _signalTarget;

        private long _signalId;





        public NanosleepPool(AutoResetEvent signalTarget)
        {
            _signalTarget = signalTarget;
        }






        private void Signal(NanosleepThread thread)
        {
            lock (_lock)
            {
                _active.Remove(thread);
                _free.Push(thread);

                if (thread.SignalId == _signalId)
                {
                    _signalTarget.Set();
                }
            }
        }








        public bool SleepAndSignal(long nanoseconds, long timePoint)
        {
            lock (_lock)
            {
                _signalId++;


                foreach (NanosleepThread existing in _active)
                {
                    if (existing.Resurrect(_signalId, timePoint))
                    {
                        return true;
                    }
                }

                if (!_free.TryPop(out NanosleepThread thread))
                {
                    if (_threads.Count >= MaxThreads)
                    {
                        return false;
                    }

                    thread = new NanosleepThread(this, _threads.Count);

                    _threads.Add(thread);
                }

                _active.Add(thread);

                thread.SleepAndSignal(nanoseconds, _signalId, timePoint);

                return true;
            }
        }




        public void IgnoreSignal()
        {
            _signalId++;
        }




        public void Dispose()
        {
            GC.SuppressFinalize(this);

            foreach (NanosleepThread thread in _threads)
            {
                thread.Dispose();
            }

            _threads.Clear();
        }
    }
}
