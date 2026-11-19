using System.Text.Json.Serialization;

namespace Ryujinx.Common.Configuration.Hid.Controller
{
    [JsonConverter(typeof(JsonStringEnumConverter<GamepadInputId>))]
    public enum GamepadInputId : byte
    {
        Unbound,
        A,
        B,
        X,
        Y,
        LeftStick,
        RightStick,
        LeftShoulder,
        RightShoulder,


        LeftTrigger,

        RightTrigger,

        DpadUp,
        DpadDown,
        DpadLeft,
        DpadRight,



        Minus,
        Plus,

        Back = Minus,
        Start = Plus,

        Guide,
        Misc1,


        Paddle1,
        Paddle2,
        Paddle3,
        Paddle4,


        Touchpad,


        SingleLeftTrigger0,
        SingleRightTrigger0,


        SingleLeftTrigger1,
        SingleRightTrigger1,

        Count,
    }
}
