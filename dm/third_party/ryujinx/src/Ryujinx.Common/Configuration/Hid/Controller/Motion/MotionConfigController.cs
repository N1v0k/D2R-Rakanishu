using System.Text.Json.Serialization;

namespace Ryujinx.Common.Configuration.Hid.Controller.Motion
{
    [JsonConverter(typeof(JsonMotionConfigControllerConverter))]
    public class MotionConfigController
    {
        public MotionInputBackendType MotionBackend { get; set; }




        public int Sensitivity { get; set; }




        public double GyroDeadzone { get; set; }




        public bool EnableMotion { get; set; }
    }
}
