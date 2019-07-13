using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace WpfApplication1 {

    public class SoundFiles {
        public List<SoundFile> buildInSounds;
        public List<SoundFile> mapSounds;

        public SoundFiles(List<SoundFile> buildInSounds, List<SoundFile> mapSounds) {
            this.buildInSounds = buildInSounds;
            this.mapSounds = mapSounds;
        }
    }
    /*
        public struct EUDSettings {
            public unsafe char action;

            public bool addTouchRevive;
            public bool useDefaultGunShot;
            public bool useDefaultBackgroundMusic;
            public bool enableVisor;
            public bool enableBarrier;
            public bool addLeaderboard;
            public bool addTimeLock;

            public bool useSanctuaryColors;

            public char[] GunShotWavFilePath;
            public char[] VisorUsageFilePath;
            public char[] BackgroundWavFilePath;

            public char[] TimeLockMessage;
            public char[] TimeLockFrom;
            public char[] TimeLockTo;
            public char[] inputFilePath;
            public char[] outputFilePath;

            public short EMPDamage;

            public bool verbose;
        }
        */

    [StructLayout(LayoutKind.Explicit)]
    public unsafe struct EUDSettings {
        [MarshalAs(UnmanagedType.U1), FieldOffset(0)]
        public unsafe char action;

        [MarshalAs(UnmanagedType.U1), FieldOffset(1)]
        public unsafe bool addTouchRevive;
        [MarshalAs(UnmanagedType.U1), FieldOffset(2)]
        public unsafe bool useDefaultGunShot;
        [MarshalAs(UnmanagedType.U1), FieldOffset(3)]
        public unsafe bool useDefaultBackgroundMusic;
        [MarshalAs(UnmanagedType.U1), FieldOffset(4)]
        public unsafe bool enableVisor;
        [MarshalAs(UnmanagedType.U1), FieldOffset(5)]
        public unsafe bool enableBarrier;
        [MarshalAs(UnmanagedType.U1), FieldOffset(6)]
        public unsafe bool addLeaderboard;
        [MarshalAs(UnmanagedType.U1), FieldOffset(7)]
        public unsafe bool addTimeLock;
        [MarshalAs(UnmanagedType.U1), FieldOffset(8)]
        public unsafe bool useSanctuaryColors;
        [MarshalAs(UnmanagedType.U1), FieldOffset(9)]
        public unsafe bool recalculateHPAndDamage;
        [MarshalAs(UnmanagedType.U1), FieldOffset(10)]
        public unsafe bool muteUnits;

        [FieldOffset(12)]
        public unsafe byte* GunShotWavFilePath;
        [FieldOffset(16)]
        public unsafe byte* VisorUsageFilePath;
        [FieldOffset(20)]
        public unsafe byte* BackgroundWavFilePath;

        [FieldOffset(24)]
        public unsafe byte* TimeLockMessage;
        [FieldOffset(28)]
        public unsafe byte* TimeLockFrom;
        [FieldOffset(32)]
        public unsafe byte* TimeLockTo;
        [FieldOffset(36)]
        public unsafe byte* inputFilePath;
        [FieldOffset(40)]
        public unsafe byte* outputFilePath;

        [MarshalAs(UnmanagedType.U2), FieldOffset(44)]
        public unsafe short EMPDamage;

        [MarshalAs(UnmanagedType.U4), FieldOffset(46)]
        public unsafe int result;
    }

    enum Slot {
        Inactive = 0,
        ComputerGame = 1,
        OccupiedByHumanPlayer = 2,
        RescuePassive = 3,
        Unused = 4,
        Computer = 5,
        Human = 6,
        Neutral = 7,
        Closed = 8,
        Invalid = 9
    }

    public class SoundFile {
        public readonly String sourceName;
        public readonly String fileName;
        public readonly byte[] filesContents;
        public readonly int duration;
        public readonly Boolean isNative;
        public readonly Boolean isMapFile;
        public readonly Boolean isSystem;

        public SoundFile(String sourceName, String fileName, byte[] fileContents, int duration, Boolean isNative, Boolean isMapFile, Boolean isSystem) {
            this.fileName = fileName;
            this.filesContents = fileContents;
            this.duration = duration;
            this.isNative = isNative;
            this.isMapFile = isMapFile;
            this.sourceName = sourceName;
            this.isSystem = isSystem;
            if (!this.isNative && !this.isMapFile && !this.isSystem) { // Could be system file
                if (fileName.StartsWith("F:")) {
                    fileName = fileName.Substring(2);
                    this.fileName = fileName;
                    this.isSystem = true;
                }
            }
            if (this.isSystem) { 
                if (this.filesContents == null) {
                    try {
                        this.filesContents = File.ReadAllBytes(fileName);
                    } catch (Exception) {
                        this.isSystem = false;
                        return;
                    }
                }
                if(this.duration == 0) {
                    try {
                        duration = TheLib.getFileDurationInMs(fileName);
                        this.duration = duration;
                    } catch (InvalidSoundException) {
                        throw;
                    } catch (Exception) {
                        this.isSystem = false;
                        return;
                    }
                }
            }
        }

        private String smartDuration() {
            int sec = duration / 1000;
            if(sec < 10) { // Less than 10 seconds
                return duration + " ms";
            } else if(sec < 5 * 60) { // Less than 5 minutes
                return sec + "." + ((duration % 1000) / 100) + " s";
            } else {
                int mins = sec / 60;
                sec = sec % 60;
                return mins + " min, " + sec + " s";
            }
        }

        private String smartSize() {
            string[] sizes = { "B", "KB", "MB", "GB", "TB" };
            double len = filesContents.Length;
            int order = 0;
            while (len >= 1024 && order < sizes.Length - 1) {
                order++;
                len = len / 1024;
            }
            string result = String.Format("{0:0.##} {1}", len, sizes[order]);
            return result;
        }

        public override string ToString() {
            if (isSystem) {
                return "[System] " + fileName + " ["+ smartSize()+", " + smartDuration() + "]";
            } else {
                return "[" + sourceName + "] " + fileName + " [" + smartSize() + ", "+ smartDuration() + "]";
            }
        }

        public String toStorageString() {
            if (isNative) {
                return "S:" + fileName;
            } else if (isMapFile) {
                return "C:" + fileName;
            } else if (isSystem) {
                return "F:" + fileName;
            } else { // Storage string
                return fileName;
            }
            throw new NotImplementedException();
        }
    }

    class TheLib {

        private static unsafe void run(EUDSettings* settings) {
            
            unsafe
            {
                // Move to unmanaged memory
                
                //int dataSize = Marshal.SizeOf<EUDSettings>(settings);
                int dataSize = 512; // Stupid but works
                IntPtr ptr = Marshal.AllocHGlobal(dataSize);
                EUDSettings* rawSettings = (EUDSettings*)ptr;

                rawSettings->action = settings->action;

                rawSettings->addTouchRevive = settings->addTouchRevive;
                rawSettings->useDefaultGunShot = settings->useDefaultGunShot;
                rawSettings->useDefaultBackgroundMusic = settings->useDefaultBackgroundMusic;
                rawSettings->enableVisor = settings->enableVisor;
                rawSettings->enableBarrier = settings->enableBarrier;
                rawSettings->addLeaderboard = settings->addLeaderboard;
                rawSettings->addTimeLock = settings->addTimeLock;
                rawSettings->muteUnits = settings->muteUnits;

                rawSettings->useSanctuaryColors = settings->useSanctuaryColors;

                rawSettings->GunShotWavFilePath = settings->GunShotWavFilePath;
                rawSettings->VisorUsageFilePath = settings->VisorUsageFilePath;
                rawSettings->BackgroundWavFilePath = settings->BackgroundWavFilePath;
                rawSettings->recalculateHPAndDamage = settings->recalculateHPAndDamage;

                rawSettings->TimeLockMessage = settings->TimeLockMessage;
                rawSettings->TimeLockFrom = settings->TimeLockFrom;
                rawSettings->TimeLockTo = settings->TimeLockTo;
                rawSettings->inputFilePath = settings->inputFilePath;
                rawSettings->outputFilePath = settings->outputFilePath;

                rawSettings->EMPDamage = settings->EMPDamage;

                rawSettings->result = 0;

                // Process
                IntPtr ms = (IntPtr)rawSettings;
                Process(ms);

                settings->action = rawSettings->action;

                settings->addTouchRevive = rawSettings->addTouchRevive;
                settings->useDefaultGunShot = rawSettings->useDefaultGunShot;
                settings->useDefaultBackgroundMusic = rawSettings->useDefaultBackgroundMusic;
                settings->enableVisor = rawSettings->enableVisor;
                settings->enableBarrier = rawSettings->enableBarrier;
                settings->addLeaderboard = rawSettings->addLeaderboard;
                settings->addTimeLock = rawSettings->addTimeLock;

                settings->useSanctuaryColors = rawSettings->useSanctuaryColors;
                settings->recalculateHPAndDamage = rawSettings->recalculateHPAndDamage;

                settings->GunShotWavFilePath = rawSettings->GunShotWavFilePath;
                settings->VisorUsageFilePath = rawSettings->VisorUsageFilePath;
                settings->BackgroundWavFilePath = rawSettings->BackgroundWavFilePath;
                settings->muteUnits = rawSettings->muteUnits;

                settings->TimeLockMessage = rawSettings->TimeLockMessage;
                settings->TimeLockFrom = rawSettings->TimeLockFrom;
                settings->TimeLockTo = rawSettings->TimeLockTo;
                settings->inputFilePath = rawSettings->inputFilePath;
                settings->outputFilePath = rawSettings->outputFilePath;

                settings->EMPDamage = rawSettings->EMPDamage;

                settings->result = rawSettings->result;

                Marshal.FreeHGlobal(ptr);
            }
        }

        private static unsafe List<SoundFile> fromListOfStringsFromBuffer(String sourceName, UnsafeReadBuffer rb, Boolean isNative, Boolean isMapFile) {
            int totalFiles = rb.readInt();
            List<SoundFile> result = new List<SoundFile>();
            for(int i = 0; i < totalFiles; i++) {
                StringBuilder fileNameSb = new StringBuilder();
                int stringLen = rb.readInt();
                for (int o = 0; o < stringLen; o++) {
                    byte b = (byte)rb.readByte();
                    fileNameSb.Append((char)b);
                }
                int dataLength = rb.readInt();
                byte[] data = new byte[dataLength];
                for (int o = 0; o < dataLength; o++) {
                    byte b = (byte)rb.readByte();
                    data[o] = b;
                }
                int duration = rb.readInt();
                String fileName = fileNameSb.ToString();
                if (fileName.ToLower().EndsWith(".wav") || fileName.ToLower().EndsWith(".ogg")) {
                    result.Add(new SoundFile(sourceName, fileName, data, duration, isNative, isMapFile, false));
                }
         
            }
            return result;
        }
        [DllImport("QCHK.dll", EntryPoint = "#1", CallingConvention = CallingConvention.Cdecl)]
        protected static unsafe extern void Process(IntPtr settings);
        
        public unsafe static SoundFiles getWavs(Settings settings) {
            if(settings.inpuPath == null) {
                throw new Exception("Invalid file");
            } else if (!File.Exists(settings.inpuPath)) {
                throw new Exception("File doesn't exist");
            }

            EUDSettings es = new EUDSettings();
            es.action = (char) 2;
            es.inputFilePath = toByteArray(settings.inpuPath);
            List<SoundFile> insides = new List<SoundFile>();
            List<SoundFile> maps = new List<SoundFile>();
            Exception e = null;
            try {
                run(&es); // Get the array data string
                UnsafeReadBuffer rb = new UnsafeReadBuffer(es.outputFilePath);
                insides = fromListOfStringsFromBuffer("Native", rb, true, false);
                maps = fromListOfStringsFromBuffer(new FileInfo(settings.inpuPath).Name, rb, false, true);

                es.action = (char)3;
                run(&es); // Free the array data string

            } catch (Exception ex) {
                e = ex;
            }
            killByteArray(es.inputFilePath);
            if(e != null) {
                throw e;
            }
            return new SoundFiles(insides, maps);
        }

        private static unsafe byte* toByteArray(String str) {
            if(str == null) {
                return (byte*) 0;
            }
            if(str == "") {
                return (byte*) 0;
            }
            IntPtr ptr = Marshal.AllocHGlobal(str.Length+1);
            byte* bytes = (byte*)ptr;
            for (int i = 0; i < str.Length; i++) {
                bytes[i] = (byte) str[i];
            }
            bytes[str.Length] = (byte) 0;
#if DEBUG
            Debug.WriteLine("Allocating "+(str.Length+1)+" bytes at "+(ptr)+ " stored at " +((int)bytes));
#endif
            return bytes;
        }

        private static unsafe void killByteArray(byte* str) {
            if(str == (byte*) 0) {
                return;
            }
            IntPtr ptr = (IntPtr)str;
#if DEBUG
            Debug.WriteLine("Freeing bytes at " + (ptr) + " stored at " + ((int)str));
#endif
            Marshal.FreeHGlobal(ptr);
        }

        public unsafe static void process(Settings settings) {
            EUDSettings es = new EUDSettings();
            es.action = (char) 1;
            es.addTouchRevive = settings.addTouchRevive;
            es.useDefaultGunShot = settings.useDefaultGunfireSound;
            es.useDefaultBackgroundMusic = settings.useDefaultBackgroundSound;
            es.enableVisor = settings.enableVisor;
            es.enableBarrier = settings.enableBarrier;
            es.addLeaderboard = settings.addLeaderboard;
            es.addTimeLock = settings.addTimelock;
            es.recalculateHPAndDamage = settings.adjustHPAndWeapons;
            es.muteUnits = settings.muteUnits;

            es.useSanctuaryColors = settings.addSancColors;

            es.GunShotWavFilePath = toByteArray(settings.gunforeSoundPath == null ? "" : settings.gunforeSoundPath.toStorageString());
            es.VisorUsageFilePath = toByteArray(settings.visorSound == null ? "" : settings.visorSound.toStorageString());
            es.BackgroundWavFilePath = toByteArray(settings.backgroundSound == null ? "" : settings.backgroundSound.toStorageString());

            es.TimeLockMessage = toByteArray(settings.timeLockMessage);
            es.TimeLockFrom = toByteArray(settings.timeLockRangeFrom);
            es.TimeLockTo = toByteArray(settings.timeLockRangeTo);
            es.inputFilePath = toByteArray(settings.inpuPath);
            es.outputFilePath = toByteArray(settings.outputPath);

            es.result = 0;
            es.EMPDamage = (short) settings.EMPDamage;

            run(&es);

            killByteArray(es.GunShotWavFilePath);
            killByteArray(es.VisorUsageFilePath);
            killByteArray(es.BackgroundWavFilePath);

            killByteArray(es.TimeLockMessage);
            killByteArray(es.TimeLockFrom);
            killByteArray(es.TimeLockTo);
            killByteArray(es.inputFilePath);
            killByteArray(es.outputFilePath);

            settings.result = es.result;

        }

        public unsafe static int getFileDurationInMs(String filePath) {
            EUDSettings es = new EUDSettings();
            es.action = (char)4;
            es.outputFilePath = (byte*) 0;
            es.inputFilePath = toByteArray(filePath);
            try {
                run(&es); // Free the array data string
            } catch (Exception) {

            }
            int duration = (int) es.outputFilePath;
            killByteArray(es.inputFilePath);
            if(es.result != 0) {
                throw new InvalidSoundException();
            }
            return duration;
          }

        public unsafe static Slot[] getSlots(String filePath) {
            EUDSettings es = new EUDSettings();
            es.action = (char)5;
            es.outputFilePath = (byte*)0;
            es.inputFilePath = toByteArray(filePath);
            try {
                run(&es); // Run extraction
                UnsafeReadBuffer rb = new UnsafeReadBuffer(es.outputFilePath);
                Slot[] ret = new Slot[12];
                for(int i = 0; i < 12; i++) {
                    byte b = (byte) rb.readByte();
                    if(b >= 0 && b < 9) {
                        ret[i] = (Slot)b;
                    } else {
                        ret[i] = Slot.Invalid;
                    }
                }

                es.action = (char)3;
                run(&es); // Free the array data string
                return ret;
            } catch (Exception) {

            }
            return null;
        }
    }
  
}
  class InvalidSoundException : Exception {

}