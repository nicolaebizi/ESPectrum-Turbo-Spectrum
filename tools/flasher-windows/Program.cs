using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;

namespace ESPectrumTurboFlasher
{
    internal static class Program
    {
        [STAThread]
        private static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }
    }

    internal sealed class MainForm : Form
    {
        private const string AppTitle = "ESPectrum Turbo Spectrum Flasher";
        private const string EsptoolResource = "ESPectrumTurboFlasher.Assets.esptool.exe";
        private const string FirmwareResource = "ESPectrumTurboFlasher.Assets.ESPectrum-Turbo-Spectrum-FULL.bin";
        private const int Baud = 460800;

        private readonly ComboBox portCombo = new ComboBox();
        private readonly Button refreshButton = new Button();
        private readonly Button flashButton = new Button();
        private readonly ProgressBar progress = new ProgressBar();
        private readonly Label statusLabel = new Label();
        private string workDirectory;

        public MainForm()
        {
            Text = AppTitle;
            StartPosition = FormStartPosition.CenterScreen;
            ClientSize = new Size(620, 390);
            FormBorderStyle = FormBorderStyle.FixedSingle;
            MaximizeBox = false;
            BuildUi();
            RefreshPorts();
        }

        private void BuildUi()
        {
            Controls.Add(new Label { Text="ESPectrum Turbo Spectrum", Font=new Font("Segoe UI",18,FontStyle.Bold), AutoSize=true, Location=new Point(24,22) });
            Controls.Add(new Label { Text="ESP32 firmware installer", Font=new Font("Segoe UI",10), AutoSize=true, Location=new Point(27,58) });
            Controls.Add(new Label { Text="COM port:", AutoSize=true, Location=new Point(28,108) });

            portCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            portCombo.Location = new Point(105,104);
            portCombo.Size = new Size(250,28);
            Controls.Add(portCombo);

            refreshButton.Text="Refresh";
            refreshButton.Location=new Point(370,103);
            refreshButton.Size=new Size(100,30);
            refreshButton.Click += delegate { RefreshPorts(); };
            Controls.Add(refreshButton);

            Controls.Add(new Label { Text="Firmware: FULL.bin -> 0x00000000", AutoSize=true, Location=new Point(28,151) });

            flashButton.Text="FLASH FIRMWARE";
            flashButton.Font=new Font("Segoe UI",11,FontStyle.Bold);
            flashButton.Location=new Point(175,185);
            flashButton.Size=new Size(270,58);
            flashButton.Click += delegate { StartFlash(); };
            Controls.Add(flashButton);

            progress.Location=new Point(28,266);
            progress.Size=new Size(560,24);
            Controls.Add(progress);

            statusLabel.Text="Connect the ESP32 and select the COM port.";
            statusLabel.TextAlign=ContentAlignment.MiddleCenter;
            statusLabel.BorderStyle=BorderStyle.FixedSingle;
            statusLabel.Location=new Point(28,306);
            statusLabel.Size=new Size(560,38);
            Controls.Add(statusLabel);

            Controls.Add(new Label {
                Text="Erases flash, writes the complete firmware at 0x00000000, then performs a hard reset.",
                AutoSize=false, TextAlign=ContentAlignment.MiddleCenter,
                Location=new Point(28,354), Size=new Size(560,24), Font=new Font("Segoe UI",8)
            });
        }

        private void RefreshPorts()
        {
            try
            {
                string current=portCombo.Text;
                portCombo.Items.Clear();
                string[] ports=SerialPort.GetPortNames();
                Array.Sort(ports,StringComparer.OrdinalIgnoreCase);
                foreach(string p in ports) portCombo.Items.Add(p);
                if(ports.Length==0) { statusLabel.Text="No serial port detected."; return; }
                int idx=Array.IndexOf(ports,current);
                portCombo.SelectedIndex=idx>=0?idx:0;
                statusLabel.Text="Ready: "+portCombo.Text;
            }
            catch(Exception ex) { statusLabel.Text="Could not enumerate COM ports: "+ex.Message; }
        }

        private void StartFlash()
        {
            if(portCombo.SelectedItem==null)
            {
                MessageBox.Show(this,"Select an ESP32 COM port first.",AppTitle,MessageBoxButtons.OK,MessageBoxIcon.Warning);
                return;
            }
            SetBusy(true);
            progress.Value=5;
            statusLabel.Text="Preparing embedded flasher and firmware...";
            string port=portCombo.SelectedItem.ToString();
            Thread t=new Thread(delegate { FlashWorker(port); });
            t.IsBackground=true;
            t.Start();
        }

        private void FlashWorker(string port)
        {
            try
            {
                workDirectory=Path.Combine(Path.GetTempPath(),"ESPectrumTurboSpectrumFlasher",Guid.NewGuid().ToString("N"));
                Directory.CreateDirectory(workDirectory);
                string esptool=Path.Combine(workDirectory,"esptool.exe");
                string firmware=Path.Combine(workDirectory,"ESPectrum-Turbo-Spectrum-FULL.bin");
                ExtractResource(EsptoolResource,esptool);
                ExtractResource(FirmwareResource,firmware);

                SetStatus("Erasing flash...",15);
                RunEsptool(esptool,new[] {
                    "--chip","esp32","--port",port,"--baud",Baud.ToString(),
                    "--before","default_reset","--after","no_reset","erase_flash"
                });

                SetStatus("Writing FULL.bin at 0x00000000...",25);
                RunEsptool(esptool,new[] {
                    "--chip","esp32","--port",port,"--baud",Baud.ToString(),
                    "--before","no_reset","--after","hard_reset","write_flash",
                    "--flash_mode","dio","--flash_freq","40m","--flash_size","4MB",
                    "0x00000000",firmware
                });

                SetStatus("Firmware installed successfully. ESP32 hard-reset.",100);
                BeginInvoke((Action)delegate {
                    MessageBox.Show(this,"Firmware installed successfully.\r\n\r\nThe ESP32 has been hard-reset.",
                        AppTitle,MessageBoxButtons.OK,MessageBoxIcon.Information);
                });
            }
            catch(Exception ex)
            {
                SetStatus("Flash failed.",0);
                BeginInvoke((Action)delegate {
                    MessageBox.Show(this,"Flash failed:\r\n\r\n"+ex.Message,
                        AppTitle,MessageBoxButtons.OK,MessageBoxIcon.Error);
                });
            }
            finally
            {
                TryDeleteWorkDirectory();
                BeginInvoke((Action)delegate { SetBusy(false); });
            }
        }

        private void RunEsptool(string exe,string[] args)
        {
            ProcessStartInfo psi=new ProcessStartInfo {
                FileName=exe, Arguments=JoinArguments(args),
                WorkingDirectory=Path.GetDirectoryName(exe),
                UseShellExecute=false, CreateNoWindow=true,
                RedirectStandardOutput=true, RedirectStandardError=true,
                StandardOutputEncoding=Encoding.UTF8, StandardErrorEncoding=Encoding.UTF8
            };
            using(Process p=new Process())
            {
                p.StartInfo=psi;
                p.OutputDataReceived += delegate(object s,DataReceivedEventArgs e) { if(!string.IsNullOrEmpty(e.Data)) HandleOutput(e.Data); };
                p.ErrorDataReceived += delegate(object s,DataReceivedEventArgs e) { if(!string.IsNullOrEmpty(e.Data)) HandleOutput(e.Data); };
                if(!p.Start()) throw new InvalidOperationException("Could not start esptool.");
                p.BeginOutputReadLine();
                p.BeginErrorReadLine();
                p.WaitForExit();
                if(p.ExitCode!=0) throw new InvalidOperationException("esptool.exe exited with code "+p.ExitCode+".");
            }
        }

        private void HandleOutput(string line)
        {
            Match m=Regex.Match(line,@"(\d{1,3})%");
            if(!m.Success) return;
            int value;
            if(!int.TryParse(m.Groups[1].Value,out value)) return;
            value=Math.Max(0,Math.Min(100,value));
            BeginInvoke((Action)delegate { if(statusLabel.Text.StartsWith("Writing")) progress.Value=Math.Max(25,value); });
        }

        private static string JoinArguments(string[] args)
        {
            StringBuilder sb=new StringBuilder();
            foreach(string arg in args)
            {
                if(sb.Length>0) sb.Append(' ');
                sb.Append('"');
                sb.Append(arg.Replace("\\","\\\\").Replace(""","\\""));
                sb.Append('"');
            }
            return sb.ToString();
        }

        private static void ExtractResource(string name,string destination)
        {
            using(Stream input=Assembly.GetExecutingAssembly().GetManifestResourceStream(name))
            {
                if(input==null) throw new FileNotFoundException("Embedded resource not found: "+name);
                using(FileStream output=new FileStream(destination,FileMode.Create,FileAccess.Write,FileShare.None))
                    input.CopyTo(output);
            }
        }

        private void SetStatus(string text,int value)
        {
            BeginInvoke((Action)delegate {
                statusLabel.Text=text;
                progress.Value=Math.Max(0,Math.Min(100,value));
            });
        }

        private void SetBusy(bool busy)
        {
            if(InvokeRequired) { BeginInvoke((Action)delegate { SetBusy(busy); }); return; }
            flashButton.Enabled=!busy;
            refreshButton.Enabled=!busy;
            portCombo.Enabled=!busy;
        }

        private void TryDeleteWorkDirectory()
        {
            try
            {
                if(!string.IsNullOrEmpty(workDirectory) && Directory.Exists(workDirectory))
                    Directory.Delete(workDirectory,true);
            }
            catch { }
        }
    }
}