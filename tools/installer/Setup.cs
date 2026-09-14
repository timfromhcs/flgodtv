using System;
using System.IO;
using System.IO.Compression;
using System.Reflection;

namespace FLGODTV.Installer
{
    class Program
    {
        static int Main(string[] args)
        {
            Console.WriteLine("=================================================");
            Console.WriteLine("       FLGODTV Windows Standalone Installer      ");
            Console.WriteLine("=================================================");

            string targetDir = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "Programs",
                "FLGODTV"
            );
            bool silent = false;
            bool uninstall = false;

            for (int i = 0; i < args.Length; i++)
            {
                if (args[i] == "--install" && i + 1 < args.Length)
                {
                    targetDir = args[++i];
                }
                else if (args[i] == "--silent" || args[i] == "-s")
                {
                    silent = true;
                }
                else if (args[i] == "--uninstall")
                {
                    uninstall = true;
                }
                else if (args[i] == "--help" || args[i] == "-h")
                {
                    Console.WriteLine("Usage: FLGODTV-Setup.exe [options]");
                    Console.WriteLine("  --install <dir>   Specify target installation directory");
                    Console.WriteLine("  --silent          Run unattended without prompts");
                    Console.WriteLine("  --uninstall       Uninstall from target directory");
                    return 0;
                }
            }

            if (uninstall)
            {
                return PerformUninstall(targetDir, silent);
            }

            return PerformInstall(targetDir, silent);
        }

        static int PerformInstall(string targetDir, bool silent)
        {
            try
            {
                Console.WriteLine("[Installer] Target Directory: " + targetDir);
                if (!Directory.Exists(targetDir))
                {
                    Directory.CreateDirectory(targetDir);
                }

                Assembly asm = Assembly.GetExecutingAssembly();
                using (Stream zipStream = asm.GetManifestResourceStream("payload.zip"))
                {
                    if (zipStream == null)
                    {
                        Console.WriteLine("[ERROR] Embedded payload.zip not found in installer binary.");
                        return 1;
                    }

                    using (ZipArchive archive = new ZipArchive(zipStream, ZipArchiveMode.Read))
                    {
                        int total = archive.Entries.Count;
                        int count = 0;
                        Console.WriteLine("[Installer] Extracting " + total + " files...");

                        // Detect common root directory prefix in zip
                        string prefix = "";
                        foreach (ZipArchiveEntry e in archive.Entries)
                        {
                            int slash = e.FullName.IndexOf('/');
                            if (slash > 0)
                            {
                                string rootName = e.FullName.Substring(0, slash + 1);
                                if (string.IsNullOrEmpty(prefix)) prefix = rootName;
                                else if (prefix != rootName) { prefix = ""; break; }
                            }
                            else { prefix = ""; break; }
                        }

                        foreach (ZipArchiveEntry entry in archive.Entries)
                        {
                            string rel = entry.FullName;
                            if (!string.IsNullOrEmpty(prefix) && rel.StartsWith(prefix))
                            {
                                rel = rel.Substring(prefix.Length);
                            }
                            if (string.IsNullOrEmpty(rel)) continue;

                            string destPath = Path.Combine(targetDir, rel.Replace('/', Path.DirectorySeparatorChar));
                            if (string.IsNullOrEmpty(entry.Name) || rel.EndsWith("/"))
                            {
                                Directory.CreateDirectory(destPath);
                            }
                            else
                            {
                                Directory.CreateDirectory(Path.GetDirectoryName(destPath));
                                entry.ExtractToFile(destPath, true);
                                count++;
                            }
                        }
                        Console.WriteLine("[Installer] Successfully extracted " + count + " files.");
                    }
                }

                // Create uninstaller script in target directory
                string uninstallBat = Path.Combine(targetDir, "uninstall.bat");
                File.WriteAllText(uninstallBat, 
                    "@echo off\r\n" +
                    "echo Uninstalling FLGODTV...\r\n" +
                    "taskkill /f /im flgod.exe 2>nul\r\n" +
                    "taskkill /f /im flgodtv_frontend.exe 2>nul\r\n" +
                    "cd ..\r\n" +
                    "rmdir /s /q \"" + targetDir + "\"\r\n" +
                    "echo FLGODTV has been successfully uninstalled.\r\n" +
                    "pause\r\n"
                );

                // Create Desktop Shortcut if not silent
                if (!silent)
                {
                    try
                    {
                        string desktop = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
                        string shortcutPath = Path.Combine(desktop, "FLGODTV.lnk");
                        Type shellType = Type.GetTypeFromProgID("WScript.Shell");
                        if (shellType != null)
                        {
                            dynamic shell = Activator.CreateInstance(shellType);
                            dynamic shortcut = shell.CreateShortcut(shortcutPath);
                            shortcut.TargetPath = Path.Combine(targetDir, "FLGODTV.bat");
                            shortcut.WorkingDirectory = targetDir;
                            shortcut.Description = "FLGODTV Autonomous Simulation TV";
                            shortcut.Save();
                            Console.WriteLine("[Installer] Created Desktop shortcut: " + shortcutPath);
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine("[Installer] Shortcut creation note: " + ex.Message);
                    }
                }

                Console.WriteLine("[Installer] =========================================");
                Console.WriteLine("[Installer] FLGODTV INSTALLATION COMPLETE (SUCCESS)   ");
                Console.WriteLine("[Installer] Installed to: " + targetDir);
                Console.WriteLine("[Installer] =========================================");
                return 0;
            }
            catch (Exception ex)
            {
                Console.WriteLine("[ERROR] Installation failed: " + ex.ToString());
                return 2;
            }
        }

        static int PerformUninstall(string targetDir, bool silent)
        {
            try
            {
                Console.WriteLine("[Installer] Uninstalling FLGODTV from: " + targetDir);
                if (Directory.Exists(targetDir))
                {
                    Directory.Delete(targetDir, true);
                    Console.WriteLine("[Installer] Removed installation directory.");
                }
                string desktop = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
                string shortcutPath = Path.Combine(desktop, "FLGODTV.lnk");
                if (File.Exists(shortcutPath))
                {
                    File.Delete(shortcutPath);
                    Console.WriteLine("[Installer] Removed Desktop shortcut.");
                }
                Console.WriteLine("[Installer] FLGODTV uninstalled successfully.");
                return 0;
            }
            catch (Exception ex)
            {
                Console.WriteLine("[ERROR] Uninstall failed: " + ex.ToString());
                return 3;
            }
        }
    }
}
