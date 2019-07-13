using System;
using System.IO;

namespace WpfApplication1 {
    class Settings {

        public String settingsPath;
        public String inpuPath;
        public String outputPath;

        public bool enableVisor;
        public bool enableBarrier;

        public int EMPDamage;

        public bool addLeaderboard;
        public bool addTouchRevive;
        public bool addSancColors;

        public bool adjustHPAndWeapons;
        public bool muteUnits;

        public bool useDefaultGunfireSound;
        public bool useDefaultVisorSound;
        public bool useDefaultBackgroundSound;

        public SoundFile gunforeSoundPath;
        public SoundFile visorSound;
        public SoundFile backgroundSound;

        public bool addTimelock;
        public String timeLockMessage;
        public String timeLockRangeFrom;
        public String timeLockRangeTo;

        public int result;

        private Settings() {

        }

   
        public static Settings loadFromFile(String fileName) {
            Settings s = new Settings();
            try {
                byte[] bytes = File.ReadAllBytes(fileName);
                ReadBuffer rb = new ReadBuffer(bytes);
                s.settingsPath = fileName;
                s.inpuPath = rb.readString();
                s.outputPath = rb.readString();
                s.enableVisor = rb.readBool();
                s.enableBarrier = rb.readBool();
                s.EMPDamage = rb.readInt();
                s.addLeaderboard = rb.readBool();
                s.addTouchRevive = rb.readBool();
                s.addSancColors = rb.readBool();
                s.useDefaultGunfireSound = rb.readBool();
                s.useDefaultVisorSound = rb.readBool();
                s.useDefaultBackgroundSound = rb.readBool();
                s.gunforeSoundPath = new SoundFile("STORAGE", rb.readString(), null, 0, false, false, false);
                s.visorSound = new SoundFile("STORAGE", rb.readString(), null, 0, false, false, false);
                s.backgroundSound = new SoundFile("STORAGE", rb.readString(), null, 0, false, false, false);
                s.addTimelock = rb.readBool();
                s.timeLockMessage = rb.readString();
                s.timeLockRangeFrom = rb.readString();
                s.timeLockRangeTo = rb.readString();

                try {
                    s.adjustHPAndWeapons = rb.readBool();
                    s.muteUnits = rb.readBool();
                } catch (Exception) {
                    s.adjustHPAndWeapons = s.enableBarrier;
                    s.muteUnits = false;
                }
                
            } catch (Exception) {
                return null;
            }
            return s;
        }

        public bool saveToFile(String file) {
            try {
                WriteBuffer wb = new WriteBuffer();
                wb.writeString(this.inpuPath);
                wb.writeString(this.outputPath);
                wb.writeBool(this.enableVisor);
                wb.writeBool(this.enableBarrier);
                wb.writeInt(this.EMPDamage);
                wb.writeBool(this.addLeaderboard);
                wb.writeBool(this.addTouchRevive);
                wb.writeBool(this.addSancColors);
                wb.writeBool(this.useDefaultGunfireSound);
                wb.writeBool(this.useDefaultVisorSound);
                wb.writeBool(this.useDefaultBackgroundSound);
                wb.writeString(this.gunforeSoundPath == null ? "" : this.gunforeSoundPath.toStorageString());
                wb.writeString(this.visorSound == null ? "" : this.visorSound.toStorageString());
                wb.writeString(this.backgroundSound == null ? "" : this.backgroundSound.toStorageString());
                wb.writeBool(this.addTimelock);
                wb.writeString(this.timeLockMessage);
                wb.writeString(this.timeLockRangeFrom);
                wb.writeString(this.timeLockRangeTo);

                wb.writeBool(this.adjustHPAndWeapons);
                wb.writeBool(this.muteUnits);

                File.WriteAllBytes(file, wb.ToArray());
                return true;
            } catch (Exception) {
                return false;
            }
        }

        public static Settings getBlank(String file) {
            Settings s = new Settings();
            s.settingsPath = file;

            s.inpuPath = "";
            s.outputPath = "";
            s.enableVisor = true;
            s.enableBarrier = true;
            s.EMPDamage = 7;
            s.addLeaderboard = false;
            s.addTouchRevive = false;
            s.addSancColors = false;
            s.useDefaultGunfireSound = true;
            s.useDefaultVisorSound = true;
            s.useDefaultBackgroundSound = true;
            s.gunforeSoundPath = null;
            s.visorSound = null;
            s.backgroundSound = null;
            s.addTimelock = false;
            s.timeLockMessage = "";
            s.timeLockRangeFrom = "0:0:1:0:0:0";
            s.timeLockRangeTo = "0:0:1:0:0:0";

            s.adjustHPAndWeapons = s.enableBarrier;
            s.muteUnits = false;
            return s;
        }
    }
}
