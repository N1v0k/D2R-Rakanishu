namespace Ryujinx.Common.Configuration.Hid.Keyboard
{
    public class GenericKeyboardInputConfig<TKey> : GenericInputConfigurationCommon<TKey> where TKey : unmanaged
    {



        public JoyconConfigKeyboardStick<TKey> LeftJoyconStick { get; set; }




        public JoyconConfigKeyboardStick<TKey> RightJoyconStick { get; set; }
    }
}
