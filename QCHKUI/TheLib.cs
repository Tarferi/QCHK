#if !DEBUG
#define USE_MEMORY_MODULE
#endif

using QChkUI;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace WpfApplication1 {

    public class SoundFiles {
        public List<SoundFile> buildInSounds;
        public List<SoundFile> mapSounds;

        public SoundFiles(List<SoundFile> buildInSounds, List<SoundFile> mapSounds) {
            this.buildInSounds = buildInSounds;
            this.mapSounds = mapSounds;
        }
    }
   
    public unsafe struct EUDSettings {
        public unsafe byte action;
        public unsafe bool addTouchRevive;
        public unsafe bool useDefaultGunShot;
        public unsafe bool useDefaultBackgroundMusic;
        public unsafe bool enableVisor;
        public unsafe bool enableBarrier;
        public unsafe bool addLeaderboard;
        public unsafe bool addTimeLock;
        public unsafe bool useSanctuaryColors;
        public unsafe bool recalculateHPAndDamage;
        public unsafe bool muteUnits;

        public unsafe byte* GunShotWavFilePath;
        public unsafe byte* VisorUsageFilePath;
        public unsafe byte* BackgroundWavFilePath;

        public unsafe byte* TimeLockMessage;
        public unsafe byte* TimeLockFrom;
        public unsafe byte* TimeLockTo;
        public unsafe byte* inputFilePath;
        public unsafe byte* outputFilePath;
        public unsafe short EMPDamage;
        public unsafe int result;
        public unsafe byte* preferredUnitSettings;
        public unsafe byte* ignoreArmors;
    }

    public struct UnitSettings {
        public byte[] used;
        public int[] hp;
        public short[] shield;
        public byte[] armor;
        public short[] build_time;
        public short[] mineral_cost;
        public short[] gas_cost;
        public short[] str_unit_name;
        public short[] weapon_damage;
        public short[] upgrade_bonus;
    };

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

                int dataSize = 1024; // Stupid but works
                IntPtr ptr = Marshal.AllocHGlobal(dataSize);
                byte* data = (byte*)ptr;
                UnsafeWriteBuffer wb = new UnsafeWriteBuffer(data);
                wb.writeByte(settings->action);
                wb.writeBool(settings->addTouchRevive);
                wb.writeBool(settings->useDefaultGunShot);
                wb.writeBool(settings->useDefaultBackgroundMusic);
                wb.writeBool(settings->enableVisor);
                wb.writeBool(settings->enableBarrier);
                wb.writeBool(settings->addLeaderboard);
                wb.writeBool(settings->addTimeLock);

                wb.writeBool(settings->useSanctuaryColors);

                wb.writeBool(settings->recalculateHPAndDamage);
                wb.writeBool(settings->muteUnits);

                wb.writeBytePtr(settings->GunShotWavFilePath);
                wb.writeBytePtr(settings->VisorUsageFilePath);
                wb.writeBytePtr(settings->BackgroundWavFilePath);

                wb.writeBytePtr(settings->TimeLockMessage);
                wb.writeBytePtr(settings->TimeLockFrom);
                wb.writeBytePtr(settings->TimeLockTo);
                wb.writeBytePtr(settings->inputFilePath);
                wb.writeBytePtr(settings->outputFilePath);

                wb.writeShort(settings->EMPDamage);

                wb.writeInt(settings->result);
                wb.writeBytePtr(settings->preferredUnitSettings);
                wb.writeBytePtr(settings->ignoreArmors);

                // Process
                IntPtr ms = (IntPtr)data;
                Process(ms);

                UnsafeReadBuffer rb = new UnsafeReadBuffer(data);
                settings->action = (byte) rb.readByte();
                settings->addTouchRevive = rb.readBool();
                settings->useDefaultGunShot = rb.readBool();
                settings->useDefaultBackgroundMusic = rb.readBool();
                settings->enableVisor = rb.readBool();
                settings->enableBarrier = rb.readBool();
                settings->addLeaderboard = rb.readBool();
                settings->addTimeLock = rb.readBool();
                settings->useSanctuaryColors = rb.readBool();
                settings->recalculateHPAndDamage = rb.readBool();
                settings->muteUnits = rb.readBool();

                settings->GunShotWavFilePath = rb.readBytePtr();
                settings->VisorUsageFilePath = rb.readBytePtr();
                settings->BackgroundWavFilePath = rb.readBytePtr();

                settings->TimeLockMessage = rb.readBytePtr();
                settings->TimeLockFrom = rb.readBytePtr();
                settings->TimeLockTo = rb.readBytePtr();
                settings->inputFilePath = rb.readBytePtr();
                settings->outputFilePath = rb.readBytePtr();
                settings->EMPDamage = (short) rb.readShort();
                settings->result = rb.readInt();
                settings->preferredUnitSettings = rb.readBytePtr();
                settings->ignoreArmors = rb.readBytePtr();
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

#if !USE_MEMORY_MODULE
        [DllImport("kernel32.dll")]
        public static extern IntPtr LoadLibrary(string dllToLoad);

        [DllImport("kernel32.dll")]
        public static extern IntPtr GetProcAddress(IntPtr hModule, IntPtr procOrdinal);

        [DllImport("kernel32.dll")]
        public static extern bool FreeLibrary(IntPtr hModule);
#endif

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void ProcessFunc(IntPtr settings);

        protected static unsafe void Process(IntPtr settings) {
            if (_Process == null) {
#if USE_MEMORY_MODULE
              byte[] data = QChkUI.Properties.Resources.QCHK;
                mem = new MemoryModule(data);
                _Process = (ProcessFunc)mem.GetDelegateFromFuncName(0, typeof(ProcessFunc));
#else
                IntPtr pDll = LoadLibrary("QCHK.dll");
                IntPtr pAddressOfFunctionToCall0 = GetProcAddress(pDll, (IntPtr)1);
                _Process = (ProcessFunc)Marshal.GetDelegateForFunctionPointer(pAddressOfFunctionToCall0, typeof(ProcessFunc));
#endif
            }
            _Process(settings);
        }

#if USE_MEMORY_MODULE
        private static MemoryModule mem = null; // Memory module (if used)
#endif
        private static ProcessFunc _Process = null; // DLL Method


        public unsafe static SoundFiles getWavs(Settings settings) {
            if(settings.inpuPath == null) {
                throw new Exception("Invalid file");
            } else if (!File.Exists(settings.inpuPath)) {
                throw new Exception("File doesn't exist");
            }

            EUDSettings es = new EUDSettings();
            es.action = (byte) 2;
            es.inputFilePath = toByteArray(settings.inpuPath);
            List<SoundFile> insides = new List<SoundFile>();
            List<SoundFile> maps = new List<SoundFile>();
            Exception e = null;
            try {
                run(&es); // Get the array data string
                UnsafeReadBuffer rb = new UnsafeReadBuffer(es.outputFilePath);
                insides = fromListOfStringsFromBuffer("Native", rb, true, false);
                maps = fromListOfStringsFromBuffer(new FileInfo(settings.inpuPath).Name, rb, false, true);

                es.action = (byte)3;
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
            if (str == null) {
                return (byte*)0;
            }
            if (str == "") {
                return (byte*)0;
            }
            IntPtr ptr = Marshal.AllocHGlobal(str.Length + 1);
            byte* bytes = (byte*)ptr;
            for (int i = 0; i < str.Length; i++) {
                bytes[i] = (byte)str[i];
            }
            bytes[str.Length] = (byte)0;
            return bytes;
        }

        private static unsafe byte* toByteArray(byte[] data) {
            if (data == null) {
                return (byte*)0;
            }
            IntPtr ptr = Marshal.AllocHGlobal(data.Length);
            byte* bytes = (byte*)ptr;
            for (int i = 0; i < data.Length; i++) {
                bytes[i] = (byte)data[i];
            }
            return bytes;
        }

        private static unsafe byte* toByteArray(UnitSettings data) {
            int dataSize = 4168;
            IntPtr ptr = Marshal.AllocHGlobal(dataSize);
            byte* bytes = (byte*)ptr;
            UnsafeWriteBuffer wb = new UnsafeWriteBuffer(bytes);
            wb.writeData(data);
            return bytes;
        }

        private static unsafe void killByteArray(byte* str) {
            if(str == (byte*) 0) {
                return;
            }
            IntPtr ptr = (IntPtr)str;
            Marshal.FreeHGlobal(ptr);
        }

        public unsafe static void process(Settings settings) {
            EUDSettings es = new EUDSettings();
            es.action = (byte)1;
            
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
            es.preferredUnitSettings = toByteArray(settings.preferredSettings);
            es.EMPDamage = (short) settings.EMPDamage;
            es.ignoreArmors = toByteArray(settings.ignoreArmors);
            
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

            killByteArray(es.preferredUnitSettings);
            killByteArray(es.ignoreArmors);
            
        }

        public unsafe static int getFileDurationInMs(String filePath) {
            EUDSettings es = new EUDSettings();
            es.action = (byte)4;
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
            es.action = (byte)5;
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

                es.action = (byte)3;
                run(&es); // Free the array data string
                return ret;
            } catch (Exception) {

            }
            return null;
        }

        public unsafe static UnitSettings[] getUnitSettings(String filePath) {
            EUDSettings es = new EUDSettings();
            es.action = (byte)6;
            es.outputFilePath = (byte*)0;
            es.inputFilePath = toByteArray(filePath);
            try {
                run(&es); // Run extraction
                UnsafeReadBuffer rb = new UnsafeReadBuffer(es.outputFilePath);
                UnitSettings[] ret = new UnitSettings[2];
                for (int i = 0; i < 2; i++) {
                    ret[i].used = rb.readByteArray(228);
                    ret[i].hp = rb.readIntArray(228);
                    ret[i].shield = rb.readShortArray(228);
                    ret[i].armor = rb.readByteArray(228);
                    ret[i].build_time = rb.readShortArray(228);
                    ret[i].mineral_cost = rb.readShortArray(228);
                    ret[i].gas_cost = rb.readShortArray(228);
                    ret[i].str_unit_name = rb.readShortArray(228);
                    ret[i].weapon_damage = rb.readShortArray(130);
                    ret[i].upgrade_bonus = rb.readShortArray(130);
                }
                es.action = (byte)3;
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