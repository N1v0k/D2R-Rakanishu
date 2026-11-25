using Ryujinx.Graphics.Shader.IntermediateRepresentation;
using Ryujinx.Graphics.Shader.Translation;
using static Ryujinx.Graphics.Shader.IntermediateRepresentation.OperandHelper;

namespace Ryujinx.Graphics.Shader.Instructions
{
    static class Lop3Expression
    {
        private enum TruthTable : byte
        {
            False = 0x00,
            True = 0xff,
            In = 0xf0,
            And2 = 0xc0,
            Or2 = 0xfc,
            Xor2 = 0x3c,
            And3 = 0x80,
            Or3 = 0xfe,
            XorAnd = 0x60,
            XorOr = 0xf6,
            OrAnd = 0xe0,
            AndOr = 0xf8,
            Onehot = 0x16,
            Majority = 0xe8,
            Gamble = 0x81,
            InverseGamble = 0x7e,
            Dot = 0x1a,
            Mux = 0xca,
            AndXor = 0x78,
            OrXor = 0x1e,
            Xor3 = 0x96,
        }

        public static Operand GetFromTruthTable(EmitterContext context, Operand srcA, Operand srcB, Operand srcC, int imm)
        {
            for (int i = 0; i < 0x40; i++)
            {
                TruthTable currImm = (TruthTable)imm;

                Operand x = srcA;
                Operand y = srcB;
                Operand z = srcC;

                if ((i & 0x01) != 0)
                {
                    (x, y) = (y, x);
                    currImm = PermuteTable(currImm, 7, 6, 3, 2, 5, 4, 1, 0);
                }

                if ((i & 0x02) != 0)
                {
                    (x, z) = (z, x);
                    currImm = PermuteTable(currImm, 7, 3, 5, 1, 6, 2, 4, 0);
                }

                if ((i & 0x04) != 0)
                {
                    (y, z) = (z, y);
                    currImm = PermuteTable(currImm, 7, 5, 6, 4, 3, 1, 2, 0);
                }

                if ((i & 0x08) != 0)
                {
                    x = context.BitwiseNot(x);
                    currImm = PermuteTable(currImm, 3, 2, 1, 0, 7, 6, 5, 4);
                }

                if ((i & 0x10) != 0)
                {
                    y = context.BitwiseNot(y);
                    currImm = PermuteTable(currImm, 5, 4, 7, 6, 1, 0, 3, 2);
                }

                if ((i & 0x20) != 0)
                {
                    z = context.BitwiseNot(z);
                    currImm = PermuteTable(currImm, 6, 7, 4, 5, 2, 3, 0, 1);
                }

                Operand result = GetExpr(currImm, context, x, y, z);
                if (result != null)
                {
                    return result;
                }

                Operand notResult = GetExpr((TruthTable)((~(int)currImm) & 0xff), context, x, y, z);
                if (notResult != null)
                {
                    return context.BitwiseNot(notResult);
                }
            }

            return null;
        }

        private static Operand GetExpr(TruthTable imm, EmitterContext context, Operand x, Operand y, Operand z)
        {
            return imm switch
            {
#pragma warning disable IDE0055
                TruthTable.False         => Const(0),
                TruthTable.True          => Const(-1),
                TruthTable.In            => x,
                TruthTable.And2          => context.BitwiseAnd(x, y),
                TruthTable.Or2           => context.BitwiseOr(x, y),
                TruthTable.Xor2          => context.BitwiseExclusiveOr(x, y),
                TruthTable.And3          => context.BitwiseAnd(x, context.BitwiseAnd(y, z)),
                TruthTable.Or3           => context.BitwiseOr(x, context.BitwiseOr(y, z)),
                TruthTable.XorAnd        => context.BitwiseAnd(x, context.BitwiseExclusiveOr(y, z)),
                TruthTable.XorOr         => context.BitwiseOr(x, context.BitwiseExclusiveOr(y, z)),
                TruthTable.OrAnd         => context.BitwiseAnd(x, context.BitwiseOr(y, z)),
                TruthTable.AndOr         => context.BitwiseOr(x, context.BitwiseAnd(y, z)),
                TruthTable.Onehot        => context.BitwiseExclusiveOr(context.BitwiseOr(x, y), context.BitwiseOr(z, context.BitwiseAnd(x, y))),
                TruthTable.Majority      => context.BitwiseAnd(context.BitwiseOr(x, y), context.BitwiseOr(z, context.BitwiseAnd(x, y))),
                TruthTable.InverseGamble => context.BitwiseOr(context.BitwiseExclusiveOr(x, y), context.BitwiseExclusiveOr(x, z)),
                TruthTable.Dot           => context.BitwiseAnd(context.BitwiseExclusiveOr(x, z), context.BitwiseOr(context.BitwiseNot(y), z)),
                TruthTable.Mux           => context.BitwiseOr(context.BitwiseAnd(x, y), context.BitwiseAnd(context.BitwiseNot(x), z)),
                TruthTable.AndXor        => context.BitwiseExclusiveOr(x, context.BitwiseAnd(y, z)),
                TruthTable.OrXor         => context.BitwiseExclusiveOr(x, context.BitwiseOr(y, z)),
                TruthTable.Xor3          => context.BitwiseExclusiveOr(x, context.BitwiseExclusiveOr(y, z)),
                _                        => null,
#pragma warning restore IDE0055
            };
        }

        private static TruthTable PermuteTable(TruthTable imm, int bit7, int bit6, int bit5, int bit4, int bit3, int bit2, int bit1, int bit0)
        {
            int result = 0;

            result |= (((int)imm >> 0) & 1) << bit0;
            result |= (((int)imm >> 1) & 1) << bit1;
            result |= (((int)imm >> 2) & 1) << bit2;
            result |= (((int)imm >> 3) & 1) << bit3;
            result |= (((int)imm >> 4) & 1) << bit4;
            result |= (((int)imm >> 5) & 1) << bit5;
            result |= (((int)imm >> 6) & 1) << bit6;
            result |= (((int)imm >> 7) & 1) << bit7;

            return (TruthTable)result;
        }
    }
}
