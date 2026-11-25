namespace Ryujinx.Common.Configuration.Hid.Controller.Motion
{
    public class CemuHookMotionConfigController : MotionConfigController
    {



        public int Slot { get; set; }




        public int AltSlot { get; set; }




        public bool MirrorInput { get; set; }




        public string DsuServerHost { get; set; }




        public int DsuServerPort { get; set; }
    }
}
