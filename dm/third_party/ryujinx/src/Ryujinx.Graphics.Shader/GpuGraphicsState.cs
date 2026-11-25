using Ryujinx.Common.Memory;

namespace Ryujinx.Graphics.Shader
{



    public readonly struct GpuGraphicsState
    {



        public readonly bool EarlyZForce;




        public readonly InputTopology Topology;




        public readonly bool TessCw;




        public readonly TessPatchType TessPatchType;




        public readonly TessSpacing TessSpacing;




        public readonly bool AlphaToCoverageEnable;




        public readonly bool AlphaToCoverageDitherEnable;




        public readonly bool ViewportTransformDisable;




        public readonly bool DepthMode;




        public readonly bool ProgramPointSizeEnable;




        public readonly float PointSize;




        public readonly AlphaTestOp AlphaTestCompare;




        public readonly float AlphaTestReference;




        public readonly Array32<AttributeType> AttributeTypes;




        public readonly bool HasConstantBufferDrawParameters;




        public readonly Array8<AttributeType> FragmentOutputTypes;




        public readonly bool DualSourceBlendEnable;




        public readonly bool YNegateEnabled;




        public readonly bool OriginUpperLeft;




        public readonly bool HalvePrimitiveId;
























        public GpuGraphicsState(
            bool earlyZForce,
            InputTopology topology,
            bool tessCw,
            TessPatchType tessPatchType,
            TessSpacing tessSpacing,
            bool alphaToCoverageEnable,
            bool alphaToCoverageDitherEnable,
            bool viewportTransformDisable,
            bool depthMode,
            bool programPointSizeEnable,
            float pointSize,
            AlphaTestOp alphaTestCompare,
            float alphaTestReference,
            in Array32<AttributeType> attributeTypes,
            bool hasConstantBufferDrawParameters,
            in Array8<AttributeType> fragmentOutputTypes,
            bool dualSourceBlendEnable,
            bool yNegateEnabled,
            bool originUpperLeft,
            bool halvePrimitiveId)
        {
            EarlyZForce = earlyZForce;
            Topology = topology;
            TessCw = tessCw;
            TessPatchType = tessPatchType;
            TessSpacing = tessSpacing;
            AlphaToCoverageEnable = alphaToCoverageEnable;
            AlphaToCoverageDitherEnable = alphaToCoverageDitherEnable;
            ViewportTransformDisable = viewportTransformDisable;
            DepthMode = depthMode;
            ProgramPointSizeEnable = programPointSizeEnable;
            PointSize = pointSize;
            AlphaTestCompare = alphaTestCompare;
            AlphaTestReference = alphaTestReference;
            AttributeTypes = attributeTypes;
            HasConstantBufferDrawParameters = hasConstantBufferDrawParameters;
            FragmentOutputTypes = fragmentOutputTypes;
            DualSourceBlendEnable = dualSourceBlendEnable;
            YNegateEnabled = yNegateEnabled;
            OriginUpperLeft = originUpperLeft;
            HalvePrimitiveId = halvePrimitiveId;
        }
    }
}
