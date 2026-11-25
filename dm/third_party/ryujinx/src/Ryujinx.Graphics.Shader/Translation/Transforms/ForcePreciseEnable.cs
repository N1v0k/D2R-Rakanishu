using Ryujinx.Graphics.Shader.IntermediateRepresentation;
using System.Collections.Generic;

namespace Ryujinx.Graphics.Shader.Translation.Transforms
{
    class ForcePreciseEnable : ITransformPass
    {
        public static bool IsEnabled(IGpuAccessor gpuAccessor, ShaderStage stage, TargetLanguage targetLanguage, FeatureFlags usedFeatures)
        {
            return stage == ShaderStage.Fragment && gpuAccessor.QueryHostReducedPrecision();
        }

        public static LinkedListNode<INode> RunPass(TransformContext context, LinkedListNode<INode> node)
        {






            Operation operation = (Operation)node.Value;

            if (operation.Inst == (Instruction.FP32 | Instruction.Divide) &&
                operation.GetSource(0).Type == OperandType.Constant &&
                operation.GetSource(0).AsFloat() == 1f &&
                operation.GetSource(1).AsgOp is Operation addOp &&
                addOp.Inst == (Instruction.FP32 | Instruction.Add) &&
                addOp.GetSource(1).Type == OperandType.Constant)
            {
                addOp.ForcePrecise = true;
            }

            return node;
        }
    }
}
