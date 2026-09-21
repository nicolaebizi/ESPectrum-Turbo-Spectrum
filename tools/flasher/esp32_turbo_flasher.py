import os
import sys
import threading
import tkinter as tk
from tkinter import ttk, messagebox

import esptool
from serial.tools import list_ports


APP_TITLE = "ESPectrum Turbo Spectrum Flasher"
BAUD = "460800"


def resource_path(*parts):
    base = getattr(sys, "_MEIPASS", os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))
    return os.path.join(base, *parts)


def firmware_path():
    return resource_path("firmware", "ESPectrum-Turbo-Spectrum-FULL.bin")


def run_esptool(args):
    old_argv = sys.argv
    try:
        sys.argv = ["esptool"] + args
        try:
            result = esptool.main()
        except SystemExit as exc:
            result = exc.code
        if result not in (None, 0):
            raise RuntimeError(f"esptool exited with code {result}")
    finally:
        sys.argv = old_argv


class Flasher(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title(APP_TITLE)
        self.geometry("560x360")
        self.resizable(False, False)

        self.port_var = tk.StringVar()
        self.status_var = tk.StringVar(value="Connect the ESP32 and select the COM port.")
        self.progress_var = tk.DoubleVar(value=0)

        self._build_ui()
        self.refresh_ports()

    def _build_ui(self):
        pad = {"padx": 18, "pady": 8}

        ttk.Label(
            self,
            text="ESPectrum Turbo Spectrum",
            font=("Segoe UI", 18, "bold"),
        ).pack(pady=(18, 2))

        ttk.Label(
            self,
            text="ESP32 firmware installer",
            font=("Segoe UI", 10),
        ).pack(pady=(0, 14))

        row = ttk.Frame(self)
        row.pack(fill="x", **pad)

        ttk.Label(row, text="Port:").pack(side="left")
        self.port_combo = ttk.Combobox(
            row,
            textvariable=self.port_var,
            state="readonly",
            width=22,
        )
        self.port_combo.pack(side="left", padx=(8, 8))

        self.refresh_button = ttk.Button(
            row,
            text="Refresh",
            command=self.refresh_ports,
        )
        self.refresh_button.pack(side="left")

        self.flash_button = ttk.Button(
            self,
            text="FLASH FIRMWARE",
            command=self.start_flash,
        )
        self.flash_button.pack(pady=(18, 12), ipadx=28, ipady=12)

        self.progress = ttk.Progressbar(
            self,
            variable=self.progress_var,
            maximum=100,
            mode="determinate",
            length=500,
        )
        self.progress.pack(pady=8)

        self.status_label = ttk.Label(
            self,
            textvariable=self.status_var,
            anchor="center",
        )
        self.status_label.pack(fill="x", padx=18, pady=12)

        ttk.Label(
            self,
            text="The installer erases the flash, writes FULL.bin at 0x00000000, then hard-resets the ESP32.",
            wraplength=500,
            justify="center",
        ).pack(padx=18, pady=(2, 12))

    def refresh_ports(self):
        ports = [p.device for p in list_ports.comports()]
        self.port_combo["values"] = ports
        if ports:
            if self.port_var.get() not in ports:
                self.port_var.set(ports[0])
            self.status_var.set(f"Ready: {ports[0]}")
        else:
            self.port_var.set("")
            self.status_var.set("No serial port detected.")

    def set_busy(self, busy):
        state = "disabled" if busy else "normal"
        self.flash_button.config(state=state)
        self.refresh_button.config(state=state)
        self.port_combo.config(state="disabled" if busy else "readonly")

    def start_flash(self):
        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning(APP_TITLE, "Select an ESP32 COM port first.")
            return

        fw = firmware_path()
        if not os.path.isfile(fw):
            messagebox.showerror(APP_TITLE, f"Firmware not found:\n{fw}")
            return

        self.set_busy(True)
        self.progress_var.set(5)
        threading.Thread(target=self.flash_worker, args=(port, fw), daemon=True).start()

    def flash_worker(self, port, fw):
        try:
            self.after(0, lambda: self.status_var.set("Erasing flash..."))
            self.after(0, lambda: self.progress_var.set(15))

            run_esptool([
                "--chip", "esp32",
                "--port", port,
                "--baud", BAUD,
                "--before", "default_reset",
                "--after", "no_reset",
                "erase_flash",
            ])

            self.after(0, lambda: self.status_var.set("Writing firmware..."))
            self.after(0, lambda: self.progress_var.set(25))

            run_esptool([
                "--chip", "esp32",
                "--port", port,
                "--baud", BAUD,
                "--before", "no_reset",
                "--after", "hard_reset",
                "write_flash",
                "--flash_mode", "dio",
                "--flash_freq", "40m",
                "--flash_size", "4MB",
                "0x00000000",
                fw,
            ])

            self.after(0, lambda: self.progress_var.set(100))
            self.after(0, lambda: self.status_var.set("Firmware installed successfully. ESP32 restarted."))
            self.after(0, lambda: messagebox.showinfo(
                APP_TITLE,
                "Firmware installed successfully.\n\nThe ESP32 has been hard-reset."
            ))
        except Exception as exc:
            error = str(exc)
            self.after(0, lambda: self.progress_var.set(0))
            self.after(0, lambda: self.status_var.set("Flash failed."))
            self.after(0, lambda: messagebox.showerror(APP_TITLE, f"Flash failed:\n\n{error}"))
        finally:
            self.after(0, lambda: self.set_busy(False))


if __name__ == "__main__":
    Flasher().mainloop()
