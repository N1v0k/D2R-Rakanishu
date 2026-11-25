using Ryujinx.Graphics.Shader.IntermediateRepresentation;
using System;
using System.Collections.Generic;
using static Ryujinx.Graphics.Shader.IntermediateRepresentation.OperandHelper;

namespace Ryujinx.Graphics.Shader.Translation.Optimizations
{
    static class BindlessToArray
    {
        private const int NvnTextureBufferIndex = 2;
        private const int HardcodedArrayLengthOgl = 4;


        public const int MinimumArrayLength = 2;

        public static void RunPassOgl(BasicBlock block, ResourceManager resourceManager)
        {






            for (LinkedListNode<INode> node = block.Operations.First; node != null; node = node.Next)
            {
                if (node.Value is not TextureOperation texOp)
                {
                    continue;
                }

                if ((texOp.Flags & TextureFlags.Bindless) == 0)
                {
                    continue;
                }

                if (texOp.GetSource(0).AsgOp is not Operation handleAsgOp)
                {
                    continue;
                }

                if (handleAsgOp.Inst != Instruction.Load ||
                    handleAsgOp.StorageKind != StorageKind.ConstantBuffer ||
                    handleAsgOp.SourcesCount != 4)
                {
                    continue;
                }

                Operand ldcSrc0 = handleAsgOp.GetSource(0);

                if (ldcSrc0.Type != OperandType.Constant ||
                    !resourceManager.TryGetConstantBufferSlot(ldcSrc0.Value, out int src0CbufSlot) ||
                    src0CbufSlot != NvnTextureBufferIndex)
                {
                    continue;
                }

                Operand ldcSrc1 = handleAsgOp.GetSource(1);


                if (ldcSrc1.Type != OperandType.Constant || ldcSrc1.Value != 0)
                {
                    continue;
                }

                Operand ldcSrc2 = handleAsgOp.GetSource(2);







                if (ldcSrc2.AsgOp is not Operation shrOp || shrOp.Inst != Instruction.ShiftRightU32)
                {
                    continue;
                }

                if (shrOp.GetSource(0).AsgOp is not Operation shrOp2 || shrOp2.Inst != Instruction.ShiftRightU32)
                {
                    continue;
                }

                if (shrOp2.GetSource(0).AsgOp is not Operation addOp || addOp.Inst != Instruction.Add)
                {
                    continue;
                }

                Operand addSrc1 = addOp.GetSource(1);

                if (addSrc1.Type != OperandType.Constant)
                {
                    continue;
                }

                TurnIntoArray(resourceManager, texOp, NvnTextureBufferIndex, addSrc1.Value / 4, HardcodedArrayLengthOgl);

                Operand index = Local();

                Operand source = addOp.GetSource(0);

                Operation shrBy3 = new(Instruction.ShiftRightU32, index, source, Const(3));

                block.Operations.AddBefore(node, shrBy3);

                texOp.SetSource(0, index);
            }
        }

        public static void RunPass(BasicBlock block, ResourceManager resourceManager, IGpuAccessor gpuAccessor)
        {






            for (LinkedListNode<INode> node = block.Operations.First; node != null; node = node.Next)
            {
                if (node.Value is not TextureOperation texOp)
                {
                    continue;
                }

                if ((texOp.Flags & TextureFlags.Bindless) == 0)
                {
                    continue;
                }

                Operand bindlessHandle = Utils.FindLastOperation(texOp.GetSource(0), block);

                if (bindlessHandle.AsgOp is not Operation handleAsgOp)
                {
                    continue;
                }

                int secondaryCbufSlot = 0;
                int secondaryCbufOffset = 0;
                bool hasSecondaryHandle = false;

                if (handleAsgOp.Inst == Instruction.BitwiseOr)
                {
                    Operand src0 = Utils.FindLastOperation(handleAsgOp.GetSource(0), block);
                    Operand src1 = Utils.FindLastOperation(handleAsgOp.GetSource(1), block);

                    if (src0.Type == OperandType.ConstantBuffer && src1.AsgOp is Operation)
                    {
                        handleAsgOp = src1.AsgOp as Operation;
                        secondaryCbufSlot = src0.GetCbufSlot();
                        secondaryCbufOffset = src0.GetCbufOffset();
                        hasSecondaryHandle = true;
                    }
                    else if (src0.AsgOp is Operation && src1.Type == OperandType.ConstantBuffer)
                    {
                        handleAsgOp = src0.AsgOp as Operation;
                        secondaryCbufSlot = src1.GetCbufSlot();
                        secondaryCbufOffset = src1.GetCbufOffset();
                        hasSecondaryHandle = true;
                    }
                }

                if (handleAsgOp.Inst != Instruction.Load ||
                    handleAsgOp.StorageKind != StorageKind.ConstantBuffer ||
                    handleAsgOp.SourcesCount != 4)
                {
                    continue;
                }

                Operand ldcSrc0 = handleAsgOp.GetSource(0);

                if (ldcSrc0.Type != OperandType.Constant ||
                    !resourceManager.TryGetConstantBufferSlot(ldcSrc0.Value, out int src0CbufSlot))
                {
                    continue;
                }

                Operand ldcSrc1 = handleAsgOp.GetSource(1);


                if (ldcSrc1.Type != OperandType.Constant || ldcSrc1.Value != 0)
                {
                    continue;
                }

                Operand ldcVecIndex = handleAsgOp.GetSource(2);
                Operand ldcElemIndex = handleAsgOp.GetSource(3);

                if (ldcVecIndex.Type != OperandType.LocalVariable || ldcElemIndex.Type != OperandType.LocalVariable)
                {
                    continue;
                }

                int cbufSlot;
                int handleIndex;

                if (hasSecondaryHandle)
                {
                    cbufSlot = TextureHandle.PackSlots(src0CbufSlot, secondaryCbufSlot);
                    handleIndex = TextureHandle.PackOffsets(0, secondaryCbufOffset, TextureHandleType.SeparateSamplerHandle);
                }
                else
                {
                    cbufSlot = src0CbufSlot;
                    handleIndex = 0;
                }

                int length = Math.Max(MinimumArrayLength, gpuAccessor.QueryTextureArrayLengthFromBuffer(src0CbufSlot));

                TurnIntoArray(resourceManager, texOp, cbufSlot, handleIndex, length);

                Operand vecIndex = Local();
                Operand elemIndex = Local();
                Operand index = Local();
                Operand indexMin = Local();

                block.Operations.AddBefore(node, new Operation(Instruction.ShiftLeft, vecIndex, ldcVecIndex, Const(1)));
                block.Operations.AddBefore(node, new Operation(Instruction.ShiftRightU32, elemIndex, ldcElemIndex, Const(1)));
                block.Operations.AddBefore(node, new Operation(Instruction.Add, index, vecIndex, elemIndex));
                block.Operations.AddBefore(node, new Operation(Instruction.MinimumU32, indexMin, index, Const(length - 1)));

                texOp.SetSource(0, indexMin);
            }
        }

        private static void TurnIntoArray(ResourceManager resourceManager, TextureOperation texOp, int cbufSlot, int handleIndex, int length)
        {
            SetBindingPair setAndBinding = resourceManager.GetTextureOrImageBinding(
                texOp.Inst,
                texOp.Type,
                texOp.Format,
                texOp.Flags & ~TextureFlags.Bindless,
                cbufSlot,
                handleIndex,
                length);

            texOp.TurnIntoArray(setAndBinding);
        }
    }
}
