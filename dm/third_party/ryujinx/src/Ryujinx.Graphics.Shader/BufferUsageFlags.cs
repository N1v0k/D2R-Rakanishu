using System;

namespace Ryujinx.Graphics.Shader
{



    [Flags]
    public enum BufferUsageFlags : byte
    {
        None = 0,




        Write = 1 << 0,
    }
}
