using System;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;

namespace Ryujinx.Common.PreciseSleep
{



    [SupportedOSPlatform("macos")]
    [SupportedOSPlatform("linux")]
    [SupportedOSPlatform("android")]
    [SupportedOSPlatform("ios")]
    internal static partial class Nanosleep
    {
        private const long LinuxBaseNanosleepBias = 50000;


        private const long LinuxNanosleepAccuracyPenaltyThreshold = 200000;
        private const long LinuxNanosleepAccuracyPenalty = 30000;


        private const long LinuxNanosleepBasePenaltyThreshold = 500000;
        private const long LinuxNanosleepBasePenalty = 30000;
        private const long LinuxNanosleepPenaltyPerMillisecond = 18000;
        private const long LinuxNanosleepPenaltyCap = 18000;

        private const long LinuxStrictBiasOffset = 150_000;



        private const long MacosBaseNanosleepBias = 5000;
        private const long MacosBiasPerMillisecond = 140000;
        private const long MacosBiasMaxNanoseconds = 20_000_000;
        private const long MacosStrictBiasOffset = 150_000;

        public static long Bias { get; }







        public static long GetBias(long timeoutNs)
        {
            if (OperatingSystem.IsMacOS() || OperatingSystem.IsIOS())
            {
                long biasNs = Math.Min(timeoutNs, MacosBiasMaxNanoseconds);
                return MacosBaseNanosleepBias + biasNs * MacosBiasPerMillisecond / 1_000_000;
            }
            else
            {
                long bias = LinuxBaseNanosleepBias;

                if (timeoutNs > LinuxNanosleepBasePenaltyThreshold)
                {
                    long penalty = (timeoutNs - LinuxNanosleepBasePenaltyThreshold) * LinuxNanosleepPenaltyPerMillisecond / 1_000_000;
                    bias += LinuxNanosleepBasePenalty + Math.Min(LinuxNanosleepPenaltyCap, penalty);
                }

                return bias;
            }
        }








        public static long GetStrictBias(long timeoutNs)
        {
            if (OperatingSystem.IsMacOS() || OperatingSystem.IsIOS())
            {
                return GetBias(timeoutNs) + MacosStrictBiasOffset;
            }
            else
            {
                long bias = GetBias(timeoutNs) + LinuxStrictBiasOffset;

                if (timeoutNs > LinuxNanosleepAccuracyPenaltyThreshold)
                {
                    bias += LinuxNanosleepAccuracyPenalty;
                }

                return bias;
            }
        }

        static Nanosleep()
        {
            Bias = GetBias(0);
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct Timespec
        {
            public long tv_sec;
            public long tv_nsec;
        }

        [LibraryImport("libc", SetLastError = true)]
        private static partial int nanosleep(ref Timespec req, ref Timespec rem);






        private static Timespec GetTimespecFromNanoseconds(ulong nanoseconds)
        {
            return new Timespec
            {
                tv_sec = (long)(nanoseconds / 1_000_000_000),
                tv_nsec = (long)(nanoseconds % 1_000_000_000)
            };
        }





        public static void Sleep(long nanoseconds)
        {
            nanoseconds -= GetBias(nanoseconds);

            if (nanoseconds >= 0)
            {
                Timespec req = GetTimespecFromNanoseconds((ulong)nanoseconds);
                Timespec rem = new();

                nanosleep(ref req, ref rem);
            }
        }









        public static void SleepAtMost(long nanoseconds)
        {

            nanoseconds -= GetStrictBias(nanoseconds);

            if (nanoseconds >= 0)
            {
                Timespec req = GetTimespecFromNanoseconds((ulong)nanoseconds);
                Timespec rem = new();

                nanosleep(ref req, ref rem);
            }
        }
    }
}
