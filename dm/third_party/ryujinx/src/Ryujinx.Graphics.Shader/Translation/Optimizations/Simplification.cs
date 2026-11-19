using Ryujinx.Graphics.Shader.IntermediateRepresentation;

using static Ryujinx.Graphics.Shader.IntermediateRepresentation.OperandHelper;

namespace Ryujinx.Graphics.Shader.Translation.Optimizations
{
    static class Simplification
    {
        private const int AllOnes = ~0;

        public static void RunPass(Operation operation)
        {
            switch (operation.Inst)
            {
                case Instruction.Add:
                    TryEliminateBinaryOpCommutative(operation, 0);
                    break;

                case Instruction.BitwiseAnd:
                    TryEliminateBitwiseAnd(operation);
                    break;

                case Instruction.BitwiseExclusiveOr:
                    if (!TryEliminateXorSwap(operation))
                    {
                        TryEliminateBinaryOpCommutative(operation, 0);
                    }

                    break;

                case Instruction.BitwiseOr:
                    TryEliminateBitwiseOr(operation);
                    break;

                case Instruction.CompareNotEqual:
                    TryEliminateCompareNotEqual(operation);
                    break;

                case Instruction.ConditionalSelect:
                    TryEliminateConditionalSelect(operation);
                    break;

                case Instruction.Divide:
                    TryEliminateBinaryOpY(operation, 1);
                    break;

                case Instruction.Multiply:
                    TryEliminateBinaryOpCommutative(operation, 1);
                    break;

                case Instruction.ShiftLeft:
                case Instruction.ShiftRightS32:
                case Instruction.ShiftRightU32:
                case Instruction.Subtract:
                    TryEliminateBinaryOpY(operation, 0);
                    break;
            }
        }

        private static void TryEliminateBitwiseAnd(Operation operation)
        {




            Operand x = operation.GetSource(0);
            Operand y = operation.GetSource(1);

            if (IsConstEqual(x, AllOnes))
            {
                operation.TurnIntoCopy(y);
            }
            else if (IsConstEqual(y, AllOnes))
            {
                operation.TurnIntoCopy(x);
            }
            else if (IsConstEqual(x, 0) || IsConstEqual(y, 0))
            {
                operation.TurnIntoCopy(Const(0));
            }
        }

        private static bool TryEliminateXorSwap(Operation xCopyOp)
        {

















            Operand x = xCopyOp.GetSource(0);
            Operand y = xCopyOp.GetSource(1);

            if (x.AsgOp is not Operation tCopyOp || tCopyOp.Inst != Instruction.BitwiseExclusiveOr ||
                y.AsgOp is not Operation yCopyOp || yCopyOp.Inst != Instruction.BitwiseExclusiveOr)
            {
                return false;
            }

            if (tCopyOp == yCopyOp)
            {
                return false;
            }

            if (yCopyOp.GetSource(0) != x ||
                yCopyOp.GetSource(1) != tCopyOp.GetSource(1) ||
                x.UseOps.Count != 2)
            {
                return false;
            }

            x = tCopyOp.GetSource(0);
            y = tCopyOp.GetSource(1);

            tCopyOp.TurnIntoCopy(y);
            yCopyOp.TurnIntoCopy(x);
            xCopyOp.TurnIntoCopy(tCopyOp.Dest);

            return true;
        }

        private static void TryEliminateBitwiseOr(Operation operation)
        {




            Operand x = operation.GetSource(0);
            Operand y = operation.GetSource(1);

            if (IsConstEqual(x, 0))
            {
                operation.TurnIntoCopy(y);
            }
            else if (IsConstEqual(y, 0))
            {
                operation.TurnIntoCopy(x);
            }
            else if (IsConstEqual(x, AllOnes) || IsConstEqual(y, AllOnes))
            {
                operation.TurnIntoCopy(Const(AllOnes));
            }
        }

        private static void TryEliminateBinaryOpY(Operation operation, int comparand)
        {
            Operand x = operation.GetSource(0);
            Operand y = operation.GetSource(1);

            if (IsConstEqual(y, comparand))
            {
                operation.TurnIntoCopy(x);
            }
        }

        private static void TryEliminateBinaryOpCommutative(Operation operation, int comparand)
        {
            Operand x = operation.GetSource(0);
            Operand y = operation.GetSource(1);

            if (IsConstEqual(x, comparand))
            {
                operation.TurnIntoCopy(y);
            }
            else if (IsConstEqual(y, comparand))
            {
                operation.TurnIntoCopy(x);
            }
        }

        private static void TryEliminateCompareNotEqual(Operation operation)
        {



            Operand lhs = operation.GetSource(0);
            Operand rhs = operation.GetSource(1);

            if (lhs.Type == OperandType.Constant)
            {
                (lhs, rhs) = (rhs, lhs);
            }

            if (rhs.Type != OperandType.Constant || rhs.Value != 0)
            {
                return;
            }

            if (lhs.AsgOp is not Operation compareOp || !compareOp.Inst.IsComparison())
            {
                return;
            }

            operation.TurnIntoCopy(lhs);
        }

        private static void TryEliminateConditionalSelect(Operation operation)
        {
            Operand cond = operation.GetSource(0);

            if (cond.Type != OperandType.Constant)
            {
                return;
            }



            int srcIndex = cond.Value != 0 ? 1 : 2;

            Operand source = operation.GetSource(srcIndex);

            operation.TurnIntoCopy(source);
        }

        private static bool IsConstEqual(Operand operand, int comparand)
        {
            if (operand.Type != OperandType.Constant)
            {
                return false;
            }

            return operand.Value == comparand;
        }
    }
}
