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

namespace WpfApplication1 {
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    /// 

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
            settings.addTouchRevive = (bool) checkRevive.IsChecked;

            getSelectedFile(lstBackgroundSound, checkBackground, ref settings.useDefaultBackgroundSound, ref settings.backgroundSound);
            getSelectedFile(lstVisorSound, checkVisor1, ref settings.useDefaultVisorSound, ref settings.visorSound);
            getSelectedFile(lstGunSound, checkGunfire, ref settings.useDefaultGunfireSound, ref settings.gunforeSoundPath);

            settings.addTimelock = (bool) checkTimeLock.IsChecked;
            settings.timeLockMessage = txtTimeLockMessage.Text;
            settings.timeLockRangeFrom = txtFromY.Text + ":" + txtFromM.Text + ":" + txtFromD.Text + ":" + txtFromH.Text + ":" + txtFromm.Text + ":" + txtFromS.Text;
            settings.timeLockRangeTo = txtToY.Text + ":" + txtToM.Text + ":" + txtToD.Text + ":" + txtToH.Text + ":" + txtTom.Text + ":" + txtToS.Text;
            return settings;
        }

        public MainWindow() {
            InitializeComponent();
            setState(AppState.START);
            History.open(txtSet.Name, txtSet);
            History.open(txtInMap.Name, txtInMap);
            History.open(txtOutMap.Name, txtOutMap);
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
                    SoundFile sndFile = new SoundFile("ORIGIN", file, null, 0, false, false, true);
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
}
