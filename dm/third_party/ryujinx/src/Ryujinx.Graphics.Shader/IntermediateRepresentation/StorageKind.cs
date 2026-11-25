namespace Ryujinx.Graphics.Shader.IntermediateRepresentation
{
    enum StorageKind
    {
        None,
        Input,
        InputPerPatch,
        Output,
        OutputPerPatch,
        ConstantBuffer,
        StorageBuffer,
        LocalMemory,
        SharedMemory,
        SharedMemory8,
        SharedMemory16,
        GlobalMemory,
        GlobalMemoryS8,
        GlobalMemoryS16,
        GlobalMemoryU8,
        GlobalMemoryU16,
    }

    static class StorageKindExtensions
    {
        public static bool IsInputOrOutput(this StorageKind storageKind)
        {
            return storageKind is StorageKind.Input or
                   StorageKind.InputPerPatch or
                   StorageKind.Output or
                   StorageKind.OutputPerPatch;
        }

        public static bool IsOutput(this StorageKind storageKind)
        {
            return storageKind is StorageKind.Output or
                   StorageKind.OutputPerPatch;
        }

        public static bool IsPerPatch(this StorageKind storageKind)
        {
            return storageKind is StorageKind.InputPerPatch or
                   StorageKind.OutputPerPatch;
        }
    }
}
