using System;

namespace Ryujinx.Graphics.Shader
{



    [Flags]
    public enum TextureUsageFlags
    {
        None = 0,


        ResScaleUnsupported = 1 << 0,
        NeedsScaleValue = 1 << 1,
        ImageStore = 1 << 2,
        ImageCoherent = 1 << 3,
    }
}
