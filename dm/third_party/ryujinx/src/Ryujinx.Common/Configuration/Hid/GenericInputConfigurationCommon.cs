namespace Ryujinx.Common.Configuration.Hid
{
    public class GenericInputConfigurationCommon<TButton> : InputConfig where TButton : unmanaged
    {



        public LeftJoyconCommonConfig<TButton> LeftJoycon { get; set; }




        public RightJoyconCommonConfig<TButton> RightJoycon { get; set; }
    }
}
