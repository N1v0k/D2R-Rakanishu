namespace Ryujinx.Graphics.Shader
{
    public enum ShaderStage : byte
    {
        Compute,
        Vertex,
        TessellationControl,
        TessellationEvaluation,
        Geometry,
        Fragment,

        Count,
    }

    public static class ShaderStageExtensions
    {
        extension(ShaderStage shaderStage)
        {



            public bool SupportsRenderScale =>
                shaderStage is ShaderStage.Vertex or ShaderStage.Fragment or ShaderStage.Compute;
            



            public bool IsVtg => 
                shaderStage is ShaderStage.Vertex or
                    ShaderStage.TessellationControl or
                    ShaderStage.TessellationEvaluation or
                    ShaderStage.Geometry;
        }
        
    }
}
