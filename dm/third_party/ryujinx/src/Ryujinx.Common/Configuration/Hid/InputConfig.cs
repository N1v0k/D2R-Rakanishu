using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Text.Json.Serialization;

namespace Ryujinx.Common.Configuration.Hid
{
    [JsonConverter(typeof(JsonInputConfigConverter))]
    public class InputConfig : INotifyPropertyChanged
    {



        public const int CurrentVersion = 1;

        public int Version { get; set; }

        public InputBackendType Backend { get; set; }




        public string Id { get; set; }




        public string Name { get; set; }




        public ControllerType ControllerType { get; set; }




        public PlayerIndex PlayerIndex { get; set; }





        public bool EnableDynamicGamepadSwap { get; set; }

        public event PropertyChangedEventHandler PropertyChanged;

        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
