using Ryujinx.Common.Configuration.Hid.Controller.Motion;
using System;
using System.Text.Json.Serialization;

namespace Ryujinx.Common.Configuration.Hid.Controller
{
    public class GenericControllerInputConfig<TButton, TStick> : GenericInputConfigurationCommon<TButton> where TButton : unmanaged where TStick : unmanaged
    {
        [JsonIgnore]
        private float _deadzoneLeft;
        [JsonIgnore]
        private float _deadzoneRight;
        [JsonIgnore]
        private float _triggerThreshold;




        public JoyconConfigControllerStick<TButton, TStick> LeftJoyconStick { get; set; }




        public JoyconConfigControllerStick<TButton, TStick> RightJoyconStick { get; set; }




        public float DeadzoneLeft
        {
            get => _deadzoneLeft; set
            {
                _deadzoneLeft = MathF.Round(value, 3);
                OnPropertyChanged();
            }
        }




        public float DeadzoneRight
        {
            get => _deadzoneRight; set
            {
                _deadzoneRight = MathF.Round(value, 3);
                OnPropertyChanged();
            }
        }




        public float RangeLeft { get; set; }




        public float RangeRight { get; set; }




        public float TriggerThreshold
        {
            get => _triggerThreshold; set
            {
                _triggerThreshold = MathF.Round(value, 3);
                OnPropertyChanged();
            }
        }




        public MotionConfigController Motion { get; set; }




        public RumbleConfigController Rumble { get; set; }




        public LedConfigController Led { get; set; }
    }
}
