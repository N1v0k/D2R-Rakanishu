using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Ryujinx.Common.Utilities
{
    public static class FileSystemUtils
    {
        public static void CopyDirectory(string sourceDir, string destinationDir, bool recursive)
        {

            DirectoryInfo dir = new(sourceDir);


            if (!dir.Exists)
            {
                throw new DirectoryNotFoundException($"Source directory not found: {dir.FullName}");
            }


            DirectoryInfo[] dirs = dir.GetDirectories();


            Directory.CreateDirectory(destinationDir);


            foreach (FileInfo file in dir.GetFiles())
            {
                string targetFilePath = Path.Combine(destinationDir, file.Name);
                file.CopyTo(targetFilePath);
            }


            if (recursive)
            {
                foreach (DirectoryInfo subDir in dirs)
                {
                    string newDestinationDir = Path.Combine(destinationDir, subDir.Name);
                    CopyDirectory(subDir.FullName, newDestinationDir, true);
                }
            }
        }

        public static void MoveDirectory(string sourceDir, string destinationDir)
        {
            CopyDirectory(sourceDir, destinationDir, true);
            Directory.Delete(sourceDir, true);
        }

        public static string SanitizeFileName(string fileName)
        {
            HashSet<char> reservedChars = [.. Path.GetInvalidFileNameChars()];
            return string.Concat(fileName.Select(c => reservedChars.Contains(c) ? '_' : c));
        }
    }
}
