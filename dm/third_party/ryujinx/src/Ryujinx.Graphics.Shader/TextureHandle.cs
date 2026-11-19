using System;
using System.Runtime.CompilerServices;

namespace Ryujinx.Graphics.Shader
{
    public enum TextureHandleType
    {
        CombinedSampler = 0,
        SeparateSamplerHandle = 1,
        SeparateSamplerId = 2,
        SeparateConstantSamplerHandle = 3,
        Direct = 4,
    }

    public static class TextureHandle
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int PackSlots(int cbufSlot0, int cbufSlot1)
        {
            return cbufSlot0 | ((cbufSlot1 + 1) << 16);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static (int, int) UnpackSlots(int slots, int defaultTextureBufferIndex)
        {
            int textureBufferIndex;
            int samplerBufferIndex;

            if (slots < 0)
            {
                textureBufferIndex = defaultTextureBufferIndex;
                samplerBufferIndex = textureBufferIndex;
            }
            else
            {
                uint high = (uint)slots >> 16;

                textureBufferIndex = (ushort)slots;
                samplerBufferIndex = high != 0 ? (int)high - 1 : textureBufferIndex;
            }

            return (textureBufferIndex, samplerBufferIndex);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int PackOffsets(int cbufOffset0, int cbufOffset1, TextureHandleType type)
        {
            return cbufOffset0 | (cbufOffset1 << 14) | ((int)type << 28);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static (int, int, TextureHandleType) UnpackOffsets(int handle)
        {
            return (handle & 0x3fff, (handle >> 14) & 0x3fff, (TextureHandleType)((uint)handle >> 28));
        }






        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int UnpackTextureId(int packedId)
        {
            return packedId & 0xfffff;
        }






        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int UnpackSamplerId(int packedId)
        {
            return (packedId >> 20) & 0xfff;
        }









        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int ReadPackedId(int wordOffset, ReadOnlySpan<int> cachedTextureBuffer, ReadOnlySpan<int> cachedSamplerBuffer)
        {
            (int textureWordOffset, int samplerWordOffset, TextureHandleType handleType) = UnpackOffsets(wordOffset);

            int handle = textureWordOffset < cachedTextureBuffer.Length ? cachedTextureBuffer[textureWordOffset] : 0;







            if (handleType != TextureHandleType.CombinedSampler)
            {
                int samplerHandle;

                if (handleType != TextureHandleType.SeparateConstantSamplerHandle)
                {
                    samplerHandle = samplerWordOffset < cachedSamplerBuffer.Length ? cachedSamplerBuffer[samplerWordOffset] : 0;
                }
                else
                {
                    samplerHandle = samplerWordOffset;
                }

                if (handleType is TextureHandleType.SeparateSamplerId or
                    TextureHandleType.SeparateConstantSamplerHandle)
                {
                    samplerHandle <<= 20;
                }

                handle |= samplerHandle;
            }

            return handle;
        }
    }
}
