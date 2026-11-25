







using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json.Nodes;

static unsafe class DmshExports
{
    static byte* Dup(ReadOnlySpan<byte> b, bool zero)
    {
        byte* p = (byte*)NativeMemory.Alloc((nuint)(b.Length + (zero ? 1 : 0)));
        b.CopyTo(new Span<byte>(p, b.Length));
        if (zero) p[b.Length] = 0;
        return p;
    }

    static void SetErr(byte** err, Exception e) { if (err != null) *err = Dup(Encoding.UTF8.GetBytes(e.GetType().Name + ": " + e.Message.Split('\n')[0]), true); }

    [UnmanagedCallersOnly(EntryPoint = "dmsh_xlate_one")]
    static int XlateOne(byte* job, nuint jobLen, byte* code, nuint codeLen, byte** spv, nuint* spvLen, byte** res, nuint* resLen, byte** err)
    {
        if (err != null) *err = null;
        if (spv != null) *spv = null;
        if (res != null) *res = null;
        if (job == null || code == null || spv == null || spvLen == null || res == null || resLen == null)
        {
            SetErr(err, new ArgumentException("null pointer"));
            return 2;
        }
        JsonObject j; string key;
        try
        {
            j = JsonNode.Parse(new ReadOnlySpan<byte>(job, checked((int)jobLen))).AsObject();
            key = (string)j["key"] ?? throw new ArgumentException("job without key");
        }
        catch (Exception e) { SetErr(err, e); return 2; }
        try
        {
            byte[] c = new ReadOnlySpan<byte>(code, checked((int)codeLen)).ToArray();
            (byte[] s, string r) = SwshXlate.TranslateOne(key, j, c);
            byte[] rb = Encoding.UTF8.GetBytes(r);
            *spv = Dup(s, false); *spvLen = (nuint)s.Length;
            *res = Dup(rb, true); *resLen = (nuint)rb.Length;
            return 0;
        }
        catch (Exception e)
        {
            if (*spv != null) { NativeMemory.Free(*spv); *spv = null; }
            SetErr(err, e);
            return 1;
        }
    }

    [UnmanagedCallersOnly(EntryPoint = "dmsh_xlate_free")]
    static void Free(void* p) { if (p != null) NativeMemory.Free(p); }
}
