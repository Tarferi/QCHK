using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Globalization;
using System.Media;
using OggDecoder;
using QChkUI;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Collections;

namespace WpfApplication1 {

    enum AppState {
        START,
        PICK_MAP,
        LOADING_MAP,
        READY,
        PROCESSING
    }

    public partial class MainWindow : Window {

        public bool showConfirmWarning(String title, String contents) {
            return MessageBox.Show(contents, title, MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.Yes;
        }

        public void showErrorMessageBox(String title, String contents) {
            MessageBox.Show(contents, title, MessageBoxButton.OK, MessageBoxImage.Error);
        }

        private AppState state = AppState.START;
        private Settings settings = null;
        ObservableCollection<UniSetting> unitSettingsLst = new ObservableCollection<UniSetting>();

        private void setState(AppState state) {
            this.state = state;
            disableEverything();
            if (state == AppState.START) {
                brwsBtnSaveSettings.IsEnabled = true;
                brwsBtnSettings.IsEnabled = true;
                txtSet.IsEnabled = true;
                brwsBtnSaveSettings.Content = "Open";
            } else if (state == AppState.PICK_MAP) {
                brwsBtnSaveSettings.Content = "Close";
                brwsBtnSaveSettings.IsEnabled = true;
                brwsBtnInMap.IsEnabled = true;
                txtInMap.IsEnabled = true;
                btnOpenMap.IsEnabled = true;
            } else if(state == AppState.LOADING_MAP) {

            } else if(state == AppState.READY) {
                btnOpenMap.IsEnabled = true;
                btnOpenMap.Content = "Unload";
                brwsBtnSaveSettings.Content = "Close";
                brwsBtnSettings.Content = "Save";
                txtEmpDamage.IsEnabled = true;
                txtFromY.IsEnabled = true;
                txtFromM.IsEnabled = true;
                txtFromD.IsEnabled = true;
                txtFromH.IsEnabled = true;
                txtFromm.IsEnabled = true;
                txtFromS.IsEnabled = true;
                txtToY.IsEnabled = true;
                txtToM.IsEnabled = true;
                txtToD.IsEnabled = true;
                txtToH.IsEnabled = true;
                checkMuteUnits.IsEnabled = true;
                checkAdjustHP.IsEnabled = true;
                txtTom.IsEnabled = true;
                txtToS.IsEnabled = true;
                txtGunfire2.IsEnabled = true;
                txtOutMap.IsEnabled = true;
                txtTimeLockMessage.IsEnabled = true;
                btnRun1.IsEnabled = true;
                brwsBtnOutMap.IsEnabled = true;
                brwsBtnVisorSnd.IsEnabled = true;
                brwsBtnAdvisorSound.IsEnabled = true;
                brwsBtnBackgroundSound.IsEnabled = true;
                brwsBtnSaveSettings.IsEnabled = true;
                brwsBtnSettings.IsEnabled = true;
                lstGunSound.IsEnabled = true;
                lstVisorSound.IsEnabled = true;
                lstBackgroundSound.IsEnabled = true;
                checkBarrier.IsEnabled = true;
                checkColors.IsEnabled = true;
                checkLeaderboard.IsEnabled = true;
                checkTimeLock.IsEnabled = true;
                checkGunfire.IsEnabled = true;
                checkBackground.IsEnabled = true;
                checkVisor1.IsEnabled = true;
                checkVisor.IsEnabled = true;
                checkRevive.IsEnabled = true;
                brwsBtnPlayBackground.IsEnabled = true;
                brwsBtnPlayGun.IsEnabled = true;
                brwsBtnPlayVisor.IsEnabled = true;
                brwsBtnMusicBtnStop.IsEnabled = true;
                brwsBtnVisorSndStop.IsEnabled = true;
                brwsBtnGunSoundStop.IsEnabled = true;

                checkUSShotUnused.IsEnabled = true;
                btnUSUseAll.IsEnabled = true;
                btnUSUseAllSel.IsEnabled = true;
                btnUSUseNone.IsEnabled = true;
                btnUSUseNoneSel.IsEnabled = true;
                btnUSUseOriginal.IsEnabled = true;
                btnUSUseOriginalSel.IsEnabled = true;
                btnUSUseRecalc.IsEnabled = true;
                btnUSUseRecalcSel.IsEnabled = true;
                lstUnitSettings.IsEnabled = true;
            }
        }

        private void disableEverything() {
            btnOpenMap.Content = "Load";
            brwsBtnSaveSettings.Content = "Open";
            brwsBtnSettings.Content = "Browse";
            txtEmpDamage.IsEnabled = false;
            txtFromY.IsEnabled = false;
            txtFromM.IsEnabled = false;
            txtFromD.IsEnabled = false;
            txtFromH.IsEnabled = false;
            txtFromm.IsEnabled = false;
            txtFromS.IsEnabled = false;
            txtToY.IsEnabled = false;
            txtToM.IsEnabled = false;
            txtToD.IsEnabled = false;
            checkAdjustHP.IsEnabled = false;
            checkMuteUnits.IsEnabled = false;
            txtToH.IsEnabled = false;
            txtTom.IsEnabled = false;
            txtToS.IsEnabled = false;
            txtGunfire2.IsEnabled = false;
            txtInMap.IsEnabled = false;
            txtOutMap.IsEnabled = false;
            txtSet.IsEnabled = false;
            txtTimeLockMessage.IsEnabled = false;
            btnRun1.IsEnabled = false;
            brwsBtnInMap.IsEnabled = false;
            brwsBtnPlayBackground.IsEnabled = false;
            brwsBtnPlayGun.IsEnabled = false;
            brwsBtnPlayVisor.IsEnabled = false;
            brwsBtnOutMap.IsEnabled = false;
            brwsBtnVisorSnd.IsEnabled = false;
            brwsBtnAdvisorSound.IsEnabled = false;
            brwsBtnBackgroundSound.IsEnabled = false;
            btnOpenMap.IsEnabled = false;
            brwsBtnSaveSettings.IsEnabled = false;
            brwsBtnSettings.IsEnabled = false;
            lstGunSound.IsEnabled = false;
            lstVisorSound.IsEnabled = false;
            lstBackgroundSound.IsEnabled = false;
            checkBarrier.IsEnabled = false;
            checkColors.IsEnabled = false;
            checkLeaderboard.IsEnabled = false;
            checkTimeLock.IsEnabled = false;
            checkGunfire.IsEnabled = false;
            checkBackground.IsEnabled = false;
            checkVisor1.IsEnabled = false;
            checkVisor.IsEnabled = false;
            checkRevive.IsEnabled = false;
            brwsBtnMusicBtnStop.IsEnabled = false;
            brwsBtnVisorSndStop.IsEnabled = false;
            brwsBtnGunSoundStop.IsEnabled = false;

            checkUSShotUnused.IsEnabled = false;
            btnUSUseAll.IsEnabled = false;
            btnUSUseAllSel.IsEnabled = false;
            btnUSUseNone.IsEnabled = false;
            btnUSUseNoneSel.IsEnabled = false;
            btnUSUseOriginal.IsEnabled = false;
            btnUSUseOriginalSel.IsEnabled = false;
            btnUSUseRecalc.IsEnabled = false;
            btnUSUseRecalcSel.IsEnabled = false;
            lstUnitSettings.IsEnabled = false;
            
        }

        private int __getSoundIndex(String name) {
            for (int i = 1; i < this.lstBackgroundSound.Items.Count; i++) {
                SoundFile f = (SoundFile) lstBackgroundSound.Items[i];
                if(f.toStorageString() == name) {
                    return i;
                }
            }
            // Not found yet, create
            return -1;
        }

        private int getSoundIndex(SoundFile file) {
            for (int i = 1; i < this.lstBackgroundSound.Items.Count; i++) {
                SoundFile f = (SoundFile)lstBackgroundSound.Items[i];
                if (f == file) {
                    return i;
                }
            }
            if (file.isSystem) {
                lstBackgroundSound.Items.Add(file);
                lstGunSound.Items.Add(file);
                lstVisorSound.Items.Add(file);
                return lstBackgroundSound.Items.Count - 1;
            }
            return -1;
        }

        private void setSelectedFile(ComboBox combo, CheckBox check, bool useDefault, SoundFile value) {
            if (value != null) {
                if (!useDefault) { // Don't add background sound
                    check.IsChecked = false;
                    combo.SelectedIndex = 0;
                } else { // Use custom sound
                    check.IsChecked = true;
                    combo.SelectedIndex = getSoundIndex(value);
                }
            } else {
                check.IsChecked = false;
                combo.SelectedIndex = 0;
            }
        }

        private void getSelectedFile(ComboBox box, CheckBox check, ref bool useDefault, ref SoundFile value) {
            if (!(bool) check.IsChecked) {
                useDefault = false;
                value = null;
            } else {
                int index = box.SelectedIndex;
                if (index <= 0) { // None
                    useDefault = false;
                    value = null;
                } else {
                    useDefault = true;
                    value =(SoundFile) box.Items[index];
                }
            }
        }

        private void setSettings(Settings settings) {
            this.settings = settings;
            setComboText(txtSet, settings.settingsPath);
            setComboText(txtInMap, settings.inpuPath);
            setComboText(txtOutMap, settings.outputPath);
            checkVisor.IsChecked = settings.enableVisor;
            checkBarrier.IsChecked = settings.enableBarrier;
            txtEmpDamage.Text = settings.EMPDamage + "";
            checkLeaderboard.IsChecked = settings.addLeaderboard;
            checkColors.IsChecked = settings.addSancColors;
            checkRevive.IsChecked = settings.addTouchRevive;
            checkAdjustHP.IsChecked = settings.adjustHPAndWeapons;
            checkMuteUnits.IsChecked = settings.muteUnits;

            checkTimeLock.IsChecked = settings.addTimelock;
            txtTimeLockMessage.Text = settings.timeLockMessage;
            String[] rf = settings.timeLockRangeFrom.Split(':');
            txtFromY.Text = rf[0];
            txtFromM.Text = rf[1];
            txtFromD.Text = rf[2];
            txtFromH.Text = rf[3];
            txtFromm.Text = rf[4];
            txtFromS.Text = rf[5];
            String[] rt = settings.timeLockRangeTo.Split(':');
            txtToY.Text = rt[0];
            txtToM.Text = rt[1];
            txtToD.Text = rt[2];
            txtToH.Text = rt[3];
            txtTom.Text = rt[4];
            txtToS.Text = rt[5];

            // Background sound
            setSelectedFile(lstBackgroundSound, checkBackground, settings.useDefaultBackgroundSound, settings.backgroundSound);
            setSelectedFile(lstVisorSound, checkVisor1, settings.useDefaultVisorSound, settings.visorSound);
            setSelectedFile(lstGunSound, checkGunfire, settings.useDefaultGunfireSound, settings.gunforeSoundPath);
        }

        private static int getInt(String str) {
            try {
                return Int32.Parse(str);
            } catch (Exception) {
                return 0;
            }
        }

        private Settings getSettings() {
            Settings settings = this.settings;

            settings.settingsPath = getComboBox(txtSet);
            settings.inpuPath = getComboBox(txtInMap);
            settings.outputPath = getComboBox(txtOutMap);
            settings.enableVisor = (bool) checkVisor.IsChecked;
            settings.enableBarrier = (bool) checkBarrier.IsChecked;
            settings.EMPDamage = getInt(txtEmpDamage.Text);
            settings.addLeaderboard = (bool) checkLeaderboard.IsChecked;
            settings.addSancColors = (bool) checkColors.IsChecked;
            settings.addTouchRevive = (bool)checkRevive.IsChecked;
            settings.adjustHPAndWeapons = (bool)checkAdjustHP.IsChecked;
            settings.muteUnits = (bool)checkMuteUnits.IsChecked;

            getSelectedFile(lstBackgroundSound, checkBackground, ref settings.useDefaultBackgroundSound, ref settings.backgroundSound);
            getSelectedFile(lstVisorSound, checkVisor1, ref settings.useDefaultVisorSound, ref settings.visorSound);
            getSelectedFile(lstGunSound, checkGunfire, ref settings.useDefaultGunfireSound, ref settings.gunforeSoundPath);

            settings.addTimelock = (bool) checkTimeLock.IsChecked;
            settings.timeLockMessage = txtTimeLockMessage.Text;
            settings.timeLockRangeFrom = txtFromY.Text + ":" + txtFromM.Text + ":" + txtFromD.Text + ":" + txtFromH.Text + ":" + txtFromm.Text + ":" + txtFromS.Text;
            settings.timeLockRangeTo = txtToY.Text + ":" + txtToM.Text + ":" + txtToD.Text + ":" + txtToH.Text + ":" + txtTom.Text + ":" + txtToS.Text;
            return settings;
        }

        void dgPrimaryGrid_AutoGeneratingColumn(object sender, DataGridAutoGeneratingColumnEventArgs e) {
            var desc = e.PropertyDescriptor as PropertyDescriptor;
            var att = desc.Attributes[typeof(ColumnNameAttribute)] as ColumnNameAttribute;
            if (att != null) {
                e.Column.Header = att.Name;
            }
        }

        public MainWindow() {
            InitializeComponent();
            setState(AppState.START);
            History.open(txtSet.Name, txtSet);
            History.open(txtInMap.Name, txtInMap);
            History.open(txtOutMap.Name, txtOutMap);
            this.lstUnitSettings.AutoGeneratingColumn += dgPrimaryGrid_AutoGeneratingColumn;
            this.lstUnitSettings.ItemsSource = unitSettingsLst;
        }

        private bool tryLoadingGivenMap(Settings set, bool errorOnFail, AppState stateOnError) {
            setState(AppState.LOADING_MAP);
            SoundFiles wavs = null;
            try {
                Slot[] slots = TheLib.getSlots(set.inpuPath);
                if(slots[6] != Slot.Computer && slots[6] != Slot.ComputerGame) {
                    if (errorOnFail) {
                        showErrorMessageBox("Map load", "slot 7 must be occupied by computer!");
                        setState(stateOnError);
                    }
                    return false;
                }
                wavs = TheLib.getWavs(set);
            } catch (Exception) {
                if (errorOnFail) {
                    showErrorMessageBox("Map load", "Loading map failed.");
                    setState(stateOnError);
                }
                return false;
            }

            // Load unit settings
            try {
                UnitSettings[] us = TheLib.getUnitSettings(set.inpuPath);
                if (us == null) {
                    if (errorOnFail) {
                        showErrorMessageBox("Map load", "Failed to read unit settings!");
                        setState(stateOnError);
                    }
                    return false;
                }
                if(set.preferredSettings.armor == null) {
                    set.preferredSettings = us[1];
                }
                setUnitSettings(us, set);
            } catch (Exception) {
                if (errorOnFail) {
                    showErrorMessageBox("Map load", "Loading map failed.");
                    setState(stateOnError);
                }
                return false;
            }

            foreach (List<SoundFile> snd in new List<SoundFile>[]{ wavs.buildInSounds, wavs.mapSounds }) {
                foreach (SoundFile str in snd) {
                    if (set.backgroundSound != null) {
                        if (str.toStorageString() == set.backgroundSound.toStorageString()) {
                            set.backgroundSound = str;
                        }
                    }
                    if (set.visorSound != null) {
                        if (str.toStorageString() == set.visorSound.toStorageString()) {
                            set.visorSound = str;
                        }
                    }
                    if(set.gunforeSoundPath != null) {
                        if (str.toStorageString() == set.gunforeSoundPath.toStorageString()) {
                            set.gunforeSoundPath = str;
                        }
                    }
                }
            }
            foreach (ComboBox item in new ComboBox[] { lstBackgroundSound, lstGunSound, lstVisorSound }) {
                ItemCollection items = item.Items;
                int index = item.SelectedIndex;
                items.Clear();
                items.Add("<None>");

                foreach (SoundFile str in wavs.buildInSounds) {
                    items.Add(str);
                }
                foreach (SoundFile str in wavs.mapSounds) {
                    items.Add(str);
                }

                if(index >= 0 && index < items.Count) {
                    item.SelectedIndex = index;
                }
            }
            if (set.backgroundSound != null) {
                if (!set.backgroundSound.isMapFile && !set.backgroundSound.isNative && !set.backgroundSound.isSystem) {
                    set.backgroundSound = null;
                } else if (set.backgroundSound.isSystem) {
                    lstGunSound.SelectedIndex = getSoundIndex(set.backgroundSound);
                }
            }
            if (set.gunforeSoundPath != null) {
                if (!set.gunforeSoundPath.isMapFile && !set.gunforeSoundPath.isNative && !set.gunforeSoundPath.isSystem) {
                    set.gunforeSoundPath = null;
                } else if (set.gunforeSoundPath.isSystem) {
                    lstBackgroundSound.SelectedIndex = getSoundIndex(set.gunforeSoundPath);
                }
            }
            if (set.visorSound != null) {
                if (!set.visorSound.isMapFile && !set.visorSound.isNative && !set.visorSound.isSystem) {
                    set.visorSound = null;
                } else if (set.visorSound.isSystem) {
                    lstVisorSound.SelectedIndex = getSoundIndex(set.visorSound);
                }
            }

            setState(AppState.READY);
            setSettings(set);
            return true;
        }

        private void setComboText(ComboBox box, String text) {
            for(int i = 0; i < box.Items.Count; i++) {
                String item = box.Items[i].ToString();
                if(item == text) {
                    var tmp = box.Items[0];
                    box.Items[0] = box.Items[i];
                    box.Items[i] = tmp;
                    box.SelectedIndex = 0;
                    History.save(box.Name, box);
                    return;
                }
            }
            box.Items.Insert(0, text);
            box.SelectedIndex = 0;
            History.save(box.Name, box);
        }

        private String getComboBox(ComboBox box) {
            return (box.SelectedIndex >= 0 && box.SelectedIndex < box.Items.Count) ? box.Items[box.SelectedIndex].ToString() : box.Text;
        }

        private void brwsBtnSaveSettings_Click(object sender, RoutedEventArgs e) { // Open settings clicked
            if(state == AppState.START) { // First load, open
                String fileName = getComboBox(txtSet);
                Settings set = Settings.loadFromFile(fileName);
                if(set == null) { // Failed to load
                    if(showConfirmWarning("Settings load", "Would you like to create a new file at given location?\n(This might overwrite the file, if it exists)")) {
                        set = Settings.getBlank(fileName);
                        if (!set.saveToFile(fileName)) {
                            showErrorMessageBox("Settings save", "Failed to create settings file");
                        } else {
                            this.setSettings(set);
                            setState(AppState.PICK_MAP);
                        }
                    }
                } else {
                    setSettings(set);
                    if (!tryLoadingGivenMap(set, false, AppState.PICK_MAP)) {
                        setState(AppState.PICK_MAP);
                    }
                }
            } else if (state == AppState.PICK_MAP) { // Picking map, close
                setState(AppState.START);
            } else if(state == AppState.READY) { // Awaiting input, close
                setState(AppState.START);
            }
        }

        private static readonly Regex _regex = new Regex("^-?[0-9]+$"); //regex that matches disallowed text
        private static bool IsTextAllowed(string text) {
            return !_regex.IsMatch(text);
        }
        private void NumericInput(object sender, TextCompositionEventArgs e) {
            e.Handled = !_regex.IsMatch(e.Text);
        }

        private void btnOpenMap_Click(object sender, RoutedEventArgs e) { // Load map click button (can also be close map)
            if (state == AppState.PICK_MAP) { // Load
                String map = getComboBox(txtInMap);
                if (map.Length > 0) {
                    tryLoadingGivenMap(getSettings(), true, AppState.PICK_MAP);
                }
            } else if(state == AppState.READY) { // Unload
                setState(AppState.PICK_MAP);
            }
        }

        private static String getSettingsFile() {
            return getFile("Open Settings File", new String[] { "spt" }, new String[] { "Snipers Power Tool" });
        }

        private static String getMapFile() {
            return getFile("Open Starcraft Map File", new String[] { "scx", "scm" }, new String[] { "Starcraft Map", "Starcraft Map" });
        }

        private static String getFile(String title, String[] extension, String[] extensionDescriptions) {
            OpenFileDialog openFileDialog = new OpenFileDialog();
    

            String[] res = new String[extension.Length];
            for(int i = 0; i  < extension.Length; i++) {
                StringBuilder sb = new StringBuilder();
                res[i] = extensionDescriptions[i] + " (*." + extension[i] + ")|*."+extension[i];
            }
            String type = string.Join("|", res);


            openFileDialog.Filter = type;
            openFileDialog.FilterIndex = 1;
            openFileDialog.RestoreDirectory = true;
            openFileDialog.CheckFileExists = true;
            openFileDialog.CheckPathExists = true;
            bool? b = openFileDialog.ShowDialog();
            if (b == true) {
                String filePath = openFileDialog.FileName;
                return filePath;
            }
            return null;
        }

        private void brwsBtnSettings_Click(object sender, RoutedEventArgs e) {
            if (state == AppState.READY) { // Save
                Settings set = getSettings(); // Scrap settings from UI
                set.saveToFile(set.settingsPath);
                setSettings(set);
                MessageBox.Show("Saved", "Snipers Power Tool", MessageBoxButton.OK, MessageBoxImage.Information);
            } else {
                String f = getSettingsFile();
                if (f != null) {
                    setComboText(txtSet, f);
                    brwsBtnSaveSettings_Click(sender, null);
                }
            }
        }

        private void brwsBtnInMap_Click(object sender, RoutedEventArgs e) {
            String f = getMapFile();
            if (f != null) {
                setComboText(txtInMap, f);
                btnOpenMap_Click(sender, null);
            }
        }

        private void brwsBtnOutMap_Click(object sender, RoutedEventArgs e) {
            SaveFileDialog openFileDialog = new SaveFileDialog();

            String type = "Starcraft Map File (*.scx)|*.scx";


            openFileDialog.Filter = type;
            openFileDialog.FilterIndex = 1;
            openFileDialog.RestoreDirectory = true;
            openFileDialog.CheckFileExists = false;
            openFileDialog.CheckPathExists = true;
            bool? b = openFileDialog.ShowDialog();
            if (b == true) {
                String filePath = openFileDialog.FileName;
                setComboText(txtOutMap, filePath);
            }
        }

        private void brwsBtnPlayGun_Click(object sender, RoutedEventArgs e) {
            playSnd(lstGunSound);
        }

        private void playSnd(ComboBox cb) {
            int index = cb.SelectedIndex;
            stopSnd();
            if (index > 0 && index <= lstBackgroundSound.Items.Count) {
                playSnd((SoundFile) lstBackgroundSound.Items[index]);
            }
        }
        
        private long soundPlayEnd = 0;
        SoundPlayer lastPlayer = null;

        private void stopSnd() {
            long now = (long)(DateTime.Now.ToUniversalTime() - new DateTime(1970, 1, 1)).TotalSeconds;
            if (lastPlayer != null) {
                if (soundPlayEnd > now) { // Something is playing
                    lastPlayer.Stop();
                }
                lastPlayer.Dispose();
                lastPlayer = null;
            }
        }
  
        private void playSnd(SoundFile snd) {
            long now = (long)(DateTime.Now.ToUniversalTime() - new DateTime(1970, 1, 1)).TotalSeconds;
            soundPlayEnd = now + snd.duration;
            if (snd.fileName.ToLower().EndsWith(".wav")) { // Play wav
                lastPlayer = new SoundPlayer(new MemoryStream(snd.filesContents));
                
            } else if (snd.fileName.ToLower().EndsWith(".ogg")) { // Play ogg
                lastPlayer = new SoundPlayer(new OggDecodeStream(new MemoryStream(snd.filesContents)));
            } else { // Invalid sound file?
                showErrorMessageBox("Music file", "Invalid music file format");
                return;
            }
            lastPlayer.Play();
        }

        private void brwsBtnPlayBackground_Click(object sender, RoutedEventArgs e) {
            playSnd(lstBackgroundSound);
        }

        private void brwsBtnPlayVisor_Click(object sender, RoutedEventArgs e) {
            playSnd(lstVisorSound);
        }

        private void brwsBtnVisorSnd_Click(object sender, RoutedEventArgs e) {
            selectSoundFile(lstGunSound);
        }

        private void brwsBtnBackgroundSound_Click(object sender, RoutedEventArgs e) {
            selectSoundFile(lstBackgroundSound);
        }

        private void brwsBtnAdvisorSound_Click(object sender, RoutedEventArgs e) {
            selectSoundFile(lstVisorSound);
        }

        private void selectSoundFile(ComboBox cb) {
            String file = getFile("Open supported sound file", new String[] { "wav", "ogg" }, new String[] { "Wav file", "Vorbis Ogg file" });
            if (file != null) {
                int index = __getSoundIndex(file);
                if(index == -1) { // No such thing
                    SoundFile sndFile;
                    try {
                        sndFile = new SoundFile("ORIGIN", file, null, 0, false, false, true);
                    } catch (InvalidSoundException) {
                        showErrorMessageBox("Sound Manager", "Unsupported sound file");
                        return;
                    }
                    lstBackgroundSound.Items.Add(sndFile);
                    lstGunSound.Items.Add(sndFile);
                    lstVisorSound.Items.Add(sndFile);
                    cb.SelectedIndex = cb.Items.Count - 1;
                } else {
                    cb.SelectedIndex = index;
                }
            }
        }

        private void brwsBtnGunSoundStop_Click(object sender, RoutedEventArgs e) {
            stopSnd();
        }

        private void brwsBtnMusicBtnStop_Click(object sender, RoutedEventArgs e) {
            stopSnd();
        }

        private void brwsBtnVisorSndStop_Click(object sender, RoutedEventArgs e) {
            stopSnd();
        }

        private void btnRun1_Click(object sender, RoutedEventArgs e) {
            Settings set = getSettings();
            TheLib.process(set);
            MessageBox.Show("Finished", "Snipers Power Tool", MessageBoxButton.OK, MessageBoxImage.Information);
        }

        private void setUnitSettings(UnitSettings[] us, Settings set) {
            UnitSettings raw = us[0];
            UnitSettings recalculated = us[1];

            IDisposable d = Dispatcher.DisableProcessing();
            unitSettingsLst.Clear();
            for (int index = 0; index < 228; index++) {
                unitSettingsLst.Add(new UniSetting(index, raw, recalculated, set));
            }
            d.Dispose();
        }

        private void US_copyOriginals(int[] firsts, int[] lasts) {
            IDisposable d = Dispatcher.DisableProcessing();
            for (int i = 0; i < firsts.Length; i++) {
                for (int index = firsts[i]; index < lasts[i]; index++) {
                    unitSettingsLst[index].proposedHP = unitSettingsLst[index].rawHP;
                    unitSettingsLst[index].proposedArmor = unitSettingsLst[index].rawArmor;
                    unitSettingsLst[index].proposedShield = unitSettingsLst[index].rawShield;
                    unitSettingsLst[index].proposedDamage = unitSettingsLst[index].rawDamage;
                }
            }
            lstUnitSettings.ItemsSource = null;
            lstUnitSettings.ItemsSource = unitSettingsLst;
            d.Dispose();
        }

        private void US_copyRecalculated(int[] firsts, int[] lasts) {
            IDisposable d = Dispatcher.DisableProcessing();
            for (int i = 0; i < firsts.Length; i++) {
                for (int index = firsts[i]; index < lasts[i]; index++) {
                    unitSettingsLst[index].proposedHP = unitSettingsLst[index].recalculatedHP;
                    unitSettingsLst[index].proposedArmor = unitSettingsLst[index].recalculatedArmor;
                    unitSettingsLst[index].proposedShield = unitSettingsLst[index].recalculatedShield;
                    unitSettingsLst[index].proposedDamage = unitSettingsLst[index].recalculatedDamage;
                }
            }
            lstUnitSettings.ItemsSource = null;
            lstUnitSettings.ItemsSource = unitSettingsLst;
            d.Dispose();
        }

        private void US_use(bool all, int[] firsts, int[] lasts) {
            IDisposable d = Dispatcher.DisableProcessing();
            for (int i = 0; i < firsts.Length; i++) {
                for (int index = firsts[i]; index < lasts[i]; index++) {
                    unitSettingsLst[index].useProposed = all;
                }
            }
            lstUnitSettings.ItemsSource = null;
            lstUnitSettings.ItemsSource = unitSettingsLst;
            d.Dispose();
        }

        private void btnUSUseOriginal_Click(object sender, RoutedEventArgs e) {
            US_copyOriginals(new int[] { 0 },new int[] { 228 });
        }

        private void btnUSUseRecalc_Click(object sender, RoutedEventArgs e) {
            US_copyRecalculated(new int[] { 0 }, new int[] { 228 });
        }

        private void updateUnitSetingsFilter(bool _checked) {
            ICollectionView Itemlist = CollectionViewSource.GetDefaultView(unitSettingsLst);
            Predicate<object> predicate;
            if (checkUSShotUnused.IsChecked == true) {
                predicate = new Predicate<object>(item => (true));
            } else {
                predicate = new Predicate<object>(item => ((UniSetting)item).isUsed);

            }
            Itemlist.Filter = predicate;
            lstUnitSettings.ItemsSource = null;
            lstUnitSettings.ItemsSource = unitSettingsLst;
        }

        private void checkUSShotUnused_Checked(object sender, RoutedEventArgs e) {
            updateUnitSetingsFilter(true);
        }

        private void checkUSShotUnused_Unchecked(object sender, RoutedEventArgs e) {
            updateUnitSetingsFilter(false);
        }

        private void btnUSUseAll_Click(object sender, RoutedEventArgs e) {
            US_use(true, new int[] { 0 }, new int[] { 228 });
        }

        private void btnUSUseNone_Click(object sender, RoutedEventArgs e) {
            US_use(false, new int[] { 0 }, new int[] { 228 });
        }

        private int[] getSelFirsts() {
            IList sel = lstUnitSettings.SelectedItems;
            int[] firsts = new int[sel.Count];
            for(int i = 0; i < sel.Count; i++) {
                firsts[i] = ((UniSetting)sel[i]).unitID;
            }
            return firsts;
        }

        private int[] getSelLasts() {
            IList sel = lstUnitSettings.SelectedItems;
            int[] lasts = new int[sel.Count];
            for (int i = 0; i < sel.Count; i++) {
                lasts[i] = ((UniSetting)sel[i]).unitID + 1;
            }
            return lasts;
        }

        private void setSelecteds(int[] firsts) {
            lstUnitSettings.SelectedItems.Clear();
            foreach (int index in firsts) {
                lstUnitSettings.SelectedItems.Add(unitSettingsLst[index]);
            }
            lstUnitSettings.Focus();
        }

        private void btnUSUseOriginalSel_Click(object sender, RoutedEventArgs e) {
            int[] firsts = getSelFirsts();
            US_copyOriginals(firsts, getSelLasts());
            setSelecteds(firsts);
        }

        private void btnUSUseRecalcSel_Click(object sender, RoutedEventArgs e) {
            int[] firsts = getSelFirsts();
            US_copyRecalculated(firsts, getSelLasts());
            setSelecteds(firsts);
        }

        private void btnUSUseAllSel_Click(object sender, RoutedEventArgs e) {
            int[] firsts = getSelFirsts();
            US_use(true, firsts, getSelLasts());
            setSelecteds(firsts);
        }

        private void btnUSUseNoneSel_Click(object sender, RoutedEventArgs e) {
            int[] firsts = getSelFirsts();
            US_use(false, firsts, getSelLasts());
            setSelecteds(firsts);
        }
    }

    public class UniSetting {
        private Settings settings;
        public int unitID { get;}
        private String _name;
        public String unitName { get { return "[" + unitID + "] " + _name; } set { _name= value; } }

        public int rawHP { get; set; }
        public byte rawArmor { get; set; }
        public short rawShield { get; set; }
        public short rawDamage { get; set; }

        public int recalculatedHP { get; set; }
        public byte recalculatedArmor { get; set; }
        public short recalculatedShield { get; set; }
        public short recalculatedDamage { get; set; }

        public int proposedHP { get { return settings.preferredSettings.hp[unitID] / 256; } set { settings.preferredSettings.hp[unitID] = 256 * value; } }
        public byte proposedArmor { get { return settings.preferredSettings.armor[unitID]; } set { settings.preferredSettings.armor[unitID] = (byte) value; } }
        public short proposedShield { get { return settings.preferredSettings.shield[unitID]; } set { settings.preferredSettings.shield[unitID] =(short) value; } }
        public short proposedDamage { get { return _weaponID < 130 ? settings.preferredSettings.weapon_damage[_weaponID] : (short) 0; } set { if (_weaponID < 130) { settings.preferredSettings.weapon_damage[_weaponID] = value; } } }

        public bool useProposed { get { return settings.preferredSettings.used[unitID] == 1; } set { settings.preferredSettings.used[unitID] =(byte)( value ? 1 : 0); } }
        public bool isUsed { get; set; }

        private short _weaponID;
        public bool hasWeapon { get { return _weaponID != 130; } set { } }

        private static int toInt(String value) {
            try {
                return Convert.ToInt32(value);
            } catch {
                return 0;
            }
        }

        private static readonly String[] unitNames = new String[] { "Terran Marine", "Terran Ghost", "Terran Vulture", "Terran Goliath", "Goliath Turret", "Terran Siege Tank (Tank Mode)", "Siege Tank Turret (Tank Mode)", "Terran SCV", "Terran Wraith", "Terran Science Vessel", "Gui Montag (Firebat)", "Terran Dropship", "Terran Battlecruiser", "Spider Mine", "Nuclear Missile", "Terran Civilian", "Sarah Kerrigan (Ghost)", "Alan Schezar (Goliath)", "Alan Schezar Turret", "Jim Raynor (Vulture)", "Jim Raynor (Marine)", "Tom Kazansky (Wraith)", "Magellan (Science Vessel)", "Edmund Duke (Tank Mode)", "Edmund Duke Turret (Tank Mode)", "Edmund Duke (Siege Mode)", "Edmund Duke Turret (Siege Mode)", "Arcturus Mengsk (Battlecruiser)", "Hyperion (Battlecruiser)", "Norad II (Battlecruiser)", "Terran Siege Tank (Siege Mode)", "Siege Tank Turret (Siege Mode)", "Terran Firebat", "Scanner Sweep", "Terran Medic", "Zerg Larva", "Zerg Egg", "Zerg Zergling", "Zerg Hydralisk", "Zerg Ultralisk", "Zerg Broodling", "Zerg Drone", "Zerg Overlord", "Zerg Mutalisk", "Zerg Guardian", "Zerg Queen", "Zerg Defiler", "Zerg Scourge", "Torrasque (Ultralisk)", "Matriarch (Queen)", "Infested Terran", "Infested Kerrigan (Infested Terran)", "Unclean One (Defiler)", "Hunter Killer (Hydralisk)", "Devouring One (Zergling)", "Kukulza (Mutalisk)", "Kukulza (Guardian)", "Yggdrasill (Overlord)", "Terran Valkyrie", "Mutalisk Cocoon", "Protoss Corsair", "Protoss Dark Templar (Unit)", "Zerg Devourer", "Protoss Dark Archon", "Protoss Probe", "Protoss Zealot", "Protoss Dragoon", "Protoss High Templar", "Protoss Archon", "Protoss Shuttle", "Protoss Scout", "Protoss Arbiter", "Protoss Carrier", "Protoss Interceptor", "Protoss Dark Templar (Hero)", "Zeratul (Dark Templar)", "Tassadar/Zeratul (Archon)", "Fenix (Zealot)", "Fenix (Dragoon)", "Tassadar (Templar)", "Mojo (Scout)", "Warbringer (Reaver)", "Gantrithor (Carrier)", "Protoss Reaver", "Protoss Observer", "Protoss Scarab", "Danimoth (Arbiter)", "Aldaris (Templar)", "Artanis (Scout)", "Rhynadon (Badlands Critter)", "Bengalaas (Jungle Critter)", "Cargo Ship (Unused)", "Mercenary Gunship (Unused)", "Scantid (Desert Critter)", "Kakaru (Twilight Critter)", "Ragnasaur (Ashworld Critter)", "Ursadon (Ice World Critter)", "Lurker Egg", "Raszagal (Corsair)", "Samir Duran (Ghost)", "Alexei Stukov (Ghost)", "Map Revealer", "Gerard DuGalle (BattleCruiser)", "Zerg Lurker", "Infested Duran (Infested Terran)", "Disruption Web", "Terran Command Center", "Terran Comsat Station", "Terran Nuclear Silo", "Terran Supply Depot", "Terran Refinery", "Terran Barracks", "Terran Academy", "Terran Factory", "Terran Starport", "Terran Control Tower", "Terran Science Facility", "Terran Covert Ops", "Terran Physics Lab", "Starbase (Unused)", "Terran Machine Shop", "Repair Bay (Unused)", "Terran Engineering Bay", "Terran Armory", "Terran Missile Turret", "Terran Bunker", "Norad II (Crashed)", "Ion Cannon", "Uraj Crystal", "Khalis Crystal", "Infested Command Center", "Zerg Hatchery", "Zerg Lair", "Zerg Hive", "Zerg Nydus Canal", "Zerg Hydralisk Den", "Zerg Defiler Mound", "Zerg Greater Spire", "Zerg Queen's Nest", "Zerg Evolution Chamber", "Zerg Ultralisk Cavern", "Zerg Spire", "Zerg Spawning Pool", "Zerg Creep Colony", "Zerg Spore Colony", "Unused Zerg Building1", "Zerg Sunken Colony", "Zerg Overmind (With Shell)", "Zerg Overmind", "Zerg Extractor", "Mature Chrysalis", "Zerg Cerebrate", "Zerg Cerebrate Daggoth", "Unused Zerg Building2", "Protoss Nexus", "Protoss Robotics Facility", "Protoss Pylon", "Protoss Assimilator", "Unused Protoss Building1", "Protoss Observatory", "Protoss Gateway", "Unused Protoss Building2", "Protoss Photon Cannon", "Protoss Citadel of Adun", "Protoss Cybernetics Core", "Protoss Templar Archives", "Protoss Forge", "Protoss Stargate", "Stasis Cell/Prison", "Protoss Fleet Beacon", "Protoss Arbiter Tribunal", "Protoss Robotics Support Bay", "Protoss Shield Battery", "Khaydarin Crystal Formation", "Protoss Temple", "Xel'Naga Temple", "Mineral Field (Type 1)", "Mineral Field (Type 2)", "Mineral Field (Type 3)", "Cave (Unused)", "Cave-in (Unused)", "Cantina (Unused)", "Mining Platform (Unused)", "Independent Command Center (Unused)", "Independent Starport (Unused)", "Independent Jump Gate (Unused)", "Ruins (Unused)", "Khaydarin Crystal Formation (Unused)", "Vespene Geyser", "Warp Gate", "Psi Disrupter", "Zerg Marker", "Terran Marker", "Protoss Marker", "Zerg Beacon", "Terran Beacon", "Protoss Beacon", "Zerg Flag Beacon", "Terran Flag Beacon", "Protoss Flag Beacon", "Power Generator", "Overmind Cocoon", "Dark Swarm", "Floor Missile Trap", "Floor Hatch (Unused)", "Left Upper Level Door", "Right Upper Level Door", "Left Pit Door", "Right Pit Door", "Floor Gun Trap", "Left Wall Missile Trap", "Left Wall Flame Trap", "Right Wall Missile Trap", "Right Wall Flame Trap", "Start Location", "Flag", "Young Chrysalis", "Psi Emitter", "Data Disc", "Khaydarin Crystal", "Mineral Cluster Type 1", "Mineral Cluster Type 2", "Protoss Vespene Gas Orb Type 1", "Protoss Vespene Gas Orb Type 2", "Zerg Vespene Gas Sac Type 1", "Zerg Vespene Gas Sac Type 2", "Terran Vespene Gas Tank Type 1", "Terran Vespene Gas Tank Type 2"};
        private static readonly String[] weaponNames = new String[] { "Gauss Rifle (Normal)", "Gauss Rifle (Jim Raynor-Marine)", "C-10 Concussion Rifle (Normal)", "C-10 Concussion Rifle (Sarah Kerrigan)", "Fragmentation Grenade (Normal)", "Fragmentation Grenade (Jim Raynor-Vulture)", "Spider Mines", "Twin Autocannons (Normal)", "Hellfire Missile Pack (Normal)", "Twin Autocannons (Alan Schezar)", "Hellfire Missile Pack (Alan Schezar)", "Arclite Cannon (Normal)", "Arclite Cannon (Edmund Duke)", "Fusion Cutter", "Fusion Cutter (Harvest)", "Gemini Missiles (Normal)", "Burst Lasers (Normal)", "Gemini Missiles (Tom Kazansky)", "Burst Lasers (Tom Kazansky)", "ATS Laser Battery (Normal)", "ATA Laser Battery (Normal)", "ATS Laser Battery (Norad II+Mengsk+DuGalle)", "ATA Laser Battery (Norad II+Mengsk+DuGalle)", "ATS Laser Battery (Hyperion)", "ATA Laser Battery (Hyperion)", "Flame Thrower (Normal)", "Flame Thrower (Gui Montag)", "Arclite Shock Cannon (Normal)", "Arclite Shock Cannon (Edmund Duke)", "Longbolt Missiles", "Yamato Gun", "Nuclear Missile", "Lockdown", "EMP Shockwave", "Irradiate", "Claws (Normal)", "Claws (Devouring One)", "Claws (Infested Kerrigan)", "Needle Spines (Normal)", "Needle Spines (Hunter Killer)", "Kaiser Blades (Normal)", "Kaiser Blades (Torrasque)", "Toxic Spores (Broodling)", "Spines", "Spines (Harvest)", "Acid Spray (Unused)", "Acid Spore (Normal)", "Acid Spore (Kukulza-Guardian)", "Glave Wurm (Normal)", "Glave Wurm (Kukulza-Mutalisk)", "Venom (Unused-Defiler)", "Venom (Unused-Defiler Hero)", "Seeker Spores", "Subterranean Tentacle", "Suicide (Infested Terran)", "Suicide (Scourge)", "Parasite", "Spawn Broodlings", "Ensnare", "Dark Swarm", "Plague", "Consume", "Particle Beam", "Particle Beam (Harvest)", "Psi Blades (Normal)", "Psi Blades (Fenix-Zealot)", "Phase Disruptor (Normal)", "Phase Disruptor (Fenix-Dragoon)", "Psi Assault (Normal-Unused)", "Psi Assault (Tassadar+Aldaris)", "Psionic Shockwave (Normal)", "Psionic Shockwave (Tassadar/Zeratul Archon)", "Unknown72", "Dual Photon Blasters (Normal)", "Anti-matter Missiles (Normal)", "Dual Photon Blasters (Mojo)", "Anit-matter Missiles (Mojo)", "Phase Disruptor Cannon (Normal)", "Phase Disruptor Cannon (Danimoth)", "Pulse Cannon", "STS Photon Cannon", "STA Photon Cannon", "Scarab", "Stasis Field", "Psi Storm", "Warp Blades (Zeratul)", "Warp Blades (Dark Templar Hero)", "Missiles (Unused)", "Laser Battery1 (Unused)", "Tormentor Missiles (Unused)", "Bombs (Unused)", "Raider Gun (Unused)", "Laser Battery2 (Unused)", "Laser Battery3 (Unused)", "Dual Photon Blasters (Unused)", "Flechette Grenade (Unused)", "Twin Autocannons (Floor Trap)", "Hellfire Missile Pack (Wall Trap)", "Flame Thrower (Wall Trap)", "Hellfire Missile Pack (Floor Trap)", "Neutron Flare", "Disruption Web", "Restoration", "Halo Rockets", "Corrosive Acid", "Mind Control", "Feedback", "Optical Flare", "Maelstrom", "Subterranean Spines", "Gauss Rifle0 (Unused)", "Warp Blades (Normal)", "C-10 Concussion Rifle (Samir Duran)", "C-10 Concussion Rifle (Infested Duran)", "Dual Photon Blasters (Artanis)", "Anti-matter Missiles (Artanis)", "C-10 Concussion Rifle (Alexei Stukov)", "Gauss Rifle1 (Unused)", "Gauss Rifle2 (Unused)", "Gauss Rifle3 (Unused)", "Gauss Rifle4 (Unused)", "Gauss Rifle5 (Unused)", "Gauss Rifle6 (Unused)", "Gauss Rifle7 (Unused)", "Gauss Rifle8 (Unused)", "Gauss Rifle9 (Unused)", "Gauss Rifle10 (Unused)", "Gauss Rifle11 (Unused)", "Gauss Rifle12 (Unused)", "Gauss Rifle13 (Unused)"};
        private static readonly short[] weaponMapping = new short[] { 0x00, 0x02, 0x04, 0x82, 0x07, 0x82, 0x0b, 0x0d, 0x10, 0x82, 0x1a, 0x82, 0x13, 0x06, 0x82, 0x82, 0x03, 0x82, 0x09, 0x05, 0x01, 0x12, 0x82, 0x82, 0x0c, 0x82, 0x1c, 0x15, 0x17, 0x15, 0x82, 0x1b, 0x19, 0x82, 0x82, 0x82, 0x82, 0x23, 0x26, 0x28, 0x2a, 0x2b, 0x82, 0x30, 0x2e, 0x82, 0x82, 0x82, 0x29, 0x82, 0x36, 0x25, 0x82, 0x27, 0x24, 0x31, 0x2f, 0x82, 0x82, 0x82, 0x82, 0x6f, 0x82, 0x82, 0x3e, 0x40, 0x42, 0x82, 0x46, 0x82, 0x49, 0x4d, 0x82, 0x4f, 0x56, 0x55, 0x47, 0x41, 0x43, 0x45, 0x4b, 0x82, 0x82, 0x82, 0x82, 0x52, 0x4e, 0x45, 0x72, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x70, 0x74, 0x82, 0x15, 0x6d, 0x71, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x35, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x50, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x5c, 0x82, 0x5d, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x63, 0x82, 0x82, 0x82, 0x82, 0x82, 0x60, 0x61, 0x62, 0x61, 0x62, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82 };

        public UniSetting(int index, UnitSettings raw, UnitSettings recalculated, Settings settings) {
            this.settings = settings;
            this.unitID = index;
            this._weaponID = weaponMapping[unitID];

            this.unitName = unitNames[this.unitID];
            this.rawHP = raw.hp[unitID] / 256;
            this.rawArmor = raw.armor[unitID];
            this.rawShield = raw.shield[unitID];
            this.rawDamage = _weaponID < 130 ? raw.weapon_damage[_weaponID] : (short) 0;

            this.recalculatedHP = recalculated.hp[unitID] / 256;
            this.recalculatedArmor = recalculated.armor[unitID];
            this.recalculatedShield = recalculated.shield[unitID];
            this.recalculatedDamage = _weaponID < 130 ? recalculated.weapon_damage[_weaponID] : (short) 0;

            this.isUsed = recalculated.used[unitID] == 0 ? true : false;

        }
    }

    public class InvertedBooleanToVisibilityConverter : IValueConverter {

        private BooleanToVisibilityConverter def = new BooleanToVisibilityConverter();

        public object Convert(object value, Type targetType, object parameter, CultureInfo culture) {
            Visibility vis = (Visibility) def.Convert(value, targetType, parameter, culture);
            if(vis == Visibility.Visible) {
                return Visibility.Collapsed;
            } else {
                return Visibility.Visible;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture) {
            Visibility vis = (Visibility)def.ConvertBack(value, targetType, parameter, culture);
            if (vis == Visibility.Visible) {
                return Visibility.Collapsed;
            } else {
                return Visibility.Visible;
            }
        }
    }

    public class ColumnNameAttribute : System.Attribute {
        public ColumnNameAttribute(string Name) { this.Name = Name; }
        public string Name { get; set; }
    }

    public class DataGridNumericColumn : DataGridTextColumn {
        protected override object PrepareCellForEdit(System.Windows.FrameworkElement editingElement, System.Windows.RoutedEventArgs editingEventArgs) {
            TextBox edit = editingElement as TextBox;
            edit.PreviewTextInput += OnPreviewTextInput;

            return base.PrepareCellForEdit(editingElement, editingEventArgs);
        }

        void OnPreviewTextInput(object sender, System.Windows.Input.TextCompositionEventArgs e) {
            try {
                Convert.ToInt32(e.Text);
            } catch {
                // Show some kind of error message if you want

                // Set handled to true
                e.Handled = true;
            }
        }
    }
}
