using FuncBinaryInstruction = System.Func<Spv.Generator.Instruction, Spv.Generator.Instruction, Spv.Generator.Instruction, Spv.Generator.Instruction>;
using FuncQuaternaryInstruction = System.Func<Spv.Generator.Instruction, Spv.Generator.Instruction, Spv.Generator.Instruction, Spv.Generator.Instruction, Spv.Generator.Instruction, Spv.Generator.Instruction>;
using FuncTernaryInstruction = System.Func<Spv.Generator.Instruction, Spv.Generator.Instruction, Spv.Generator.Instruction, Spv.Generator.Instruction, Spv.Generator.Instruction>;
using FuncUnaryInstruction = System.Func<Spv.Generator.Instruction, Spv.Generator.Instruction, Spv.Generator.Instruction>;

namespace Ryujinx.Graphics.Shader.CodeGen.Spirv
{



    internal readonly struct SpirvDelegates
    {

        public readonly FuncUnaryInstruction GlslFAbs;
        public readonly FuncUnaryInstruction GlslSAbs;
        public readonly FuncUnaryInstruction GlslCeil;
        public readonly FuncUnaryInstruction GlslCos;
        public readonly FuncUnaryInstruction GlslExp2;
        public readonly FuncUnaryInstruction GlslFloor;
        public readonly FuncUnaryInstruction GlslLog2;
        public readonly FuncUnaryInstruction FNegate;
        public readonly FuncUnaryInstruction SNegate;
        public readonly FuncUnaryInstruction GlslInverseSqrt;
        public readonly FuncUnaryInstruction GlslRoundEven;
        public readonly FuncUnaryInstruction GlslSin;
        public readonly FuncUnaryInstruction GlslSqrt;
        public readonly FuncUnaryInstruction GlslTrunc;


        public readonly FuncUnaryInstruction LogicalNot;


        public readonly FuncUnaryInstruction DPdx;
        public readonly FuncUnaryInstruction DPdy;


        public readonly FuncUnaryInstruction BitCount;
        public readonly FuncUnaryInstruction BitReverse;
        public readonly FuncUnaryInstruction Not;


        public readonly FuncBinaryInstruction FOrdEqual;
        public readonly FuncBinaryInstruction IEqual;
        public readonly FuncBinaryInstruction FOrdGreaterThan;
        public readonly FuncBinaryInstruction SGreaterThan;
        public readonly FuncBinaryInstruction FOrdGreaterThanEqual;
        public readonly FuncBinaryInstruction SGreaterThanEqual;
        public readonly FuncBinaryInstruction FOrdLessThan;
        public readonly FuncBinaryInstruction SLessThan;
        public readonly FuncBinaryInstruction FOrdLessThanEqual;
        public readonly FuncBinaryInstruction SLessThanEqual;
        public readonly FuncBinaryInstruction FOrdNotEqual;
        public readonly FuncBinaryInstruction INotEqual;


        public readonly FuncBinaryInstruction UGreaterThanEqual;
        public readonly FuncBinaryInstruction UGreaterThan;
        public readonly FuncBinaryInstruction ULessThanEqual;
        public readonly FuncBinaryInstruction ULessThan;


        public readonly FuncBinaryInstruction FAdd;
        public readonly FuncBinaryInstruction IAdd;
        public readonly FuncBinaryInstruction FDiv;
        public readonly FuncBinaryInstruction SDiv;
        public readonly FuncBinaryInstruction GlslFMax;
        public readonly FuncBinaryInstruction GlslSMax;
        public readonly FuncBinaryInstruction GlslFMin;
        public readonly FuncBinaryInstruction GlslSMin;
        public readonly FuncBinaryInstruction FMod;
        public readonly FuncBinaryInstruction FMul;
        public readonly FuncBinaryInstruction IMul;
        public readonly FuncBinaryInstruction FSub;
        public readonly FuncBinaryInstruction ISub;


        public readonly FuncBinaryInstruction LogicalAnd;
        public readonly FuncBinaryInstruction LogicalNotEqual;
        public readonly FuncBinaryInstruction LogicalOr;


        public readonly FuncBinaryInstruction BitwiseAnd;
        public readonly FuncBinaryInstruction BitwiseXor;
        public readonly FuncBinaryInstruction BitwiseOr;
        public readonly FuncBinaryInstruction ShiftLeftLogical;
        public readonly FuncBinaryInstruction ShiftRightArithmetic;
        public readonly FuncBinaryInstruction ShiftRightLogical;


        public readonly FuncBinaryInstruction GlslUMax;
        public readonly FuncBinaryInstruction GlslUMin;


        public readonly FuncQuaternaryInstruction AtomicIAdd;
        public readonly FuncQuaternaryInstruction AtomicAnd;
        public readonly FuncQuaternaryInstruction AtomicSMin;
        public readonly FuncQuaternaryInstruction AtomicUMin;
        public readonly FuncQuaternaryInstruction AtomicSMax;
        public readonly FuncQuaternaryInstruction AtomicUMax;
        public readonly FuncQuaternaryInstruction AtomicOr;
        public readonly FuncQuaternaryInstruction AtomicExchange;
        public readonly FuncQuaternaryInstruction AtomicXor;


        public readonly FuncTernaryInstruction GlslFClamp;
        public readonly FuncTernaryInstruction GlslSClamp;
        public readonly FuncTernaryInstruction GlslFma;


        public readonly FuncTernaryInstruction BitFieldSExtract;
        public readonly FuncTernaryInstruction BitFieldUExtract;


        public readonly FuncTernaryInstruction GlslUClamp;


        public readonly FuncQuaternaryInstruction BitFieldInsert;

        public SpirvDelegates(CodeGenContext context)
        {

            GlslFAbs = context.GlslFAbs;
            GlslSAbs = context.GlslSAbs;
            GlslCeil = context.GlslCeil;
            GlslCos = context.GlslCos;
            GlslExp2 = context.GlslExp2;
            GlslFloor = context.GlslFloor;
            GlslLog2 = context.GlslLog2;
            FNegate = context.FNegate;
            SNegate = context.SNegate;
            GlslInverseSqrt = context.GlslInverseSqrt;
            GlslRoundEven = context.GlslRoundEven;
            GlslSin = context.GlslSin;
            GlslSqrt = context.GlslSqrt;
            GlslTrunc = context.GlslTrunc;


            LogicalNot = context.LogicalNot;


            DPdx = context.DPdx;
            DPdy = context.DPdy;


            BitCount = context.BitCount;
            BitReverse = context.BitReverse;
            Not = context.Not;


            FOrdEqual = context.FOrdEqual;
            IEqual = context.IEqual;
            FOrdGreaterThan = context.FOrdGreaterThan;
            SGreaterThan = context.SGreaterThan;
            FOrdGreaterThanEqual = context.FOrdGreaterThanEqual;
            SGreaterThanEqual = context.SGreaterThanEqual;
            FOrdLessThan = context.FOrdLessThan;
            SLessThan = context.SLessThan;
            FOrdLessThanEqual = context.FOrdLessThanEqual;
            SLessThanEqual = context.SLessThanEqual;
            FOrdNotEqual = context.FOrdNotEqual;
            INotEqual = context.INotEqual;


            UGreaterThanEqual = context.UGreaterThanEqual;
            UGreaterThan = context.UGreaterThan;
            ULessThanEqual = context.ULessThanEqual;
            ULessThan = context.ULessThan;


            FAdd = context.FAdd;
            IAdd = context.IAdd;
            FDiv = context.FDiv;
            SDiv = context.SDiv;
            GlslFMax = context.GlslFMax;
            GlslSMax = context.GlslSMax;
            GlslFMin = context.GlslFMin;
            GlslSMin = context.GlslSMin;
            FMod = context.FMod;
            FMul = context.FMul;
            IMul = context.IMul;
            FSub = context.FSub;
            ISub = context.ISub;


            LogicalAnd = context.LogicalAnd;
            LogicalNotEqual = context.LogicalNotEqual;
            LogicalOr = context.LogicalOr;


            BitwiseAnd = context.BitwiseAnd;
            BitwiseXor = context.BitwiseXor;
            BitwiseOr = context.BitwiseOr;
            ShiftLeftLogical = context.ShiftLeftLogical;
            ShiftRightArithmetic = context.ShiftRightArithmetic;
            ShiftRightLogical = context.ShiftRightLogical;


            GlslUMax = context.GlslUMax;
            GlslUMin = context.GlslUMin;


            AtomicIAdd = context.AtomicIAdd;
            AtomicAnd = context.AtomicAnd;
            AtomicSMin = context.AtomicSMin;
            AtomicUMin = context.AtomicUMin;
            AtomicSMax = context.AtomicSMax;
            AtomicUMax = context.AtomicUMax;
            AtomicOr = context.AtomicOr;
            AtomicExchange = context.AtomicExchange;
            AtomicXor = context.AtomicXor;


            GlslFClamp = context.GlslFClamp;
            GlslSClamp = context.GlslSClamp;
            GlslFma = context.GlslFma;


            BitFieldSExtract = context.BitFieldSExtract;
            BitFieldUExtract = context.BitFieldUExtract;


            GlslUClamp = context.GlslUClamp;


            BitFieldInsert = context.BitFieldInsert;
        }
    }
}
