using Ryujinx.Graphics.Shader.CodeGen;
using System;

namespace Ryujinx.Graphics.Shader
{



    public interface IGpuAccessor : ILogger
    {





        uint ConstantBuffer1Read(int offset)
        {
            return 0;
        }







        ReadOnlySpan<ulong> GetCode(ulong address, int minimumSize);






        SetBindingPair CreateConstantBufferBinding(int index);







        SetBindingPair CreateImageBinding(int count, bool isBuffer);






        SetBindingPair CreateStorageBufferBinding(int index);







        SetBindingPair CreateTextureBinding(int count, bool isBuffer);





        int CreateExtraSet()
        {
            return -1;
        }





        int QueryComputeLocalSizeX()
        {
            return 1;
        }





        int QueryComputeLocalSizeY()
        {
            return 1;
        }





        int QueryComputeLocalSizeZ()
        {
            return 1;
        }





        int QueryComputeLocalMemorySize()
        {
            return 0x1000;
        }





        int QueryComputeSharedMemorySize()
        {
            return 0xc000;
        }





        uint QueryConstantBufferUse()
        {
            return 0;
        }





        GpuGraphicsState QueryGraphicsState()
        {
            return new GpuGraphicsState(
                false,
                InputTopology.Points,
                false,
                TessPatchType.Triangles,
                TessSpacing.EqualSpacing,
                false,
                false,
                false,
                false,
                false,
                1f,
                AlphaTestOp.Always,
                0f,
                default,
                true,
                default,
                false,
                false,
                false,
                false);
        }





        bool QueryHasConstantBufferDrawParameters()
        {
            return false;
        }





        bool QueryHasUnalignedStorageBuffer()
        {
            return false;
        }





        int QueryHostGatherBiasPrecision()
        {
            return 0;
        }





        bool QueryHostReducedPrecision()
        {
            return false;
        }





        bool QueryHostHasFrontFacingBug()
        {
            return false;
        }





        bool QueryHostHasVectorIndexingBug()
        {
            return false;
        }





        int QueryHostStorageBufferOffsetAlignment()
        {
            return 16;
        }





        int QueryHostSubgroupSize()
        {
            return 32;
        }





        bool QueryHostSupportsBgraFormat()
        {
            return true;
        }





        bool QueryHostSupportsFragmentShaderInterlock()
        {
            return true;
        }





        bool QueryHostSupportsFragmentShaderOrderingIntel()
        {
            return false;
        }





        bool QueryHostSupportsGeometryShader()
        {
            return true;
        }





        bool QueryHostSupportsGeometryShaderPassthrough()
        {
            return true;
        }





        bool QueryHostSupportsImageLoadFormatted()
        {
            return true;
        }





        bool QueryHostSupportsLayerVertexTessellation()
        {
            return true;
        }





        bool QueryHostSupportsNonConstantTextureOffset()
        {
            return true;
        }





        bool QueryHostSupportsScaledVertexFormats()
        {
            return true;
        }





        bool QueryHostSupportsSeparateSampler()
        {
            return true;
        }





        bool QueryHostSupportsShaderBallot()
        {
            return true;
        }





        bool QueryHostSupportsShaderBarrierDivergence()
        {
            return true;
        }





        bool QueryHostSupportsShaderFloat64()
        {
            return true;
        }
        bool QueryHostSupportsShaderNonUniformIndexing()
        {
            return false;
        }





        bool QueryHostSupportsSnormBufferTextureFormat()
        {
            return true;
        }





        bool QueryHostSupportsTextureGatherOffsets()
        {
            return true;
        }





        bool QueryHostSupportsTextureShadowLod()
        {
            return true;
        }





        bool QueryHostSupportsTransformFeedback()
        {
            return true;
        }





        bool QueryHostSupportsViewportIndexVertexTessellation()
        {
            return true;
        }





        bool QueryHostSupportsViewportMask()
        {
            return true;
        }





        bool QueryHostSupportsDepthClipControl()
        {
            return true;
        }





        int QuerySamplerArrayLengthFromPool();







        SamplerType QuerySamplerType(int handle, int cbufSlot = -1)
        {
            return SamplerType.Texture2D;
        }






        int QueryTextureArrayLengthFromBuffer(int slot);





        int QueryTextureArrayLengthFromPool();







        bool QueryTextureCoordNormalized(int handle, int cbufSlot = -1)
        {
            return true;
        }











        TextureFormat QueryTextureFormat(int handle, int cbufSlot = -1)
        {
            return TextureFormat.R8G8B8A8Unorm;
        }





        bool QueryTransformFeedbackEnabled()
        {
            return false;
        }






        ReadOnlySpan<byte> QueryTransformFeedbackVaryingLocations(int bufferIndex)
        {
            return ReadOnlySpan<byte>.Empty;
        }






        int QueryTransformFeedbackStride(int bufferIndex)
        {
            return 0;
        }






        void RegisterTexture(int handle, int cbufSlot)
        {

        }
    }
}
