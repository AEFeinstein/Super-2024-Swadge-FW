import serial.tools.list_ports
import serial.tools.list_ports_common
import threading
import esptool
try:
    # for Python2
    import Tkinter as tk
except ImportError:
    # for Python3
    import tkinter as tk

global updateUI
updateUI: bool = True

class LabelThread(threading.Thread):

    def __init__(self, _serialPort: serial.tools.list_ports_common.ListPortInfo):
        super(LabelThread, self).__init__()
        self.serialPort = _serialPort
        self.labelText = str(self.serialPort.device)
        self.labelColor = "gray"

    def getLabelText(self) -> str:
        return self.labelText

    def getLabelColor(self) -> str:
        return self.labelColor

    def run(self):
        global updateUI

        # Draw the label
        self.labelText = "Attempting flash on " + str(self.serialPort.device)
        self.labelColor = "gray"

        # Try to flash the firmware
        try:
            esptool.main([
                "--chip", "esp32s2",
                "-p", str(self.serialPort.device),
                "-b", "2000000",
                "--before=default_reset",
                "--after=no_reset_stub",
                "write_flash",
                "--flash_mode", "dio",
                "--flash_freq", "80m",
                "--flash_size", "4MB",
                "0x1000",   "bootloader.bin",
                "0x10000",  "swadge2024.bin",
                "0x8000",   "partition-table.bin",
                "0x110000", "storage.bin"])

            # It worked! Display a nice green message
            self.labelText = "Flash succeeded on " + str(self.serialPort.device)
            self.labelColor = "green"
            updateUI = True
        except Exception as e:
            # It failed. Display a bad red message
            self.labelText = "Flash failed on " + str(self.serialPort.device) + " because: " + str(e)
            self.labelColor = "red"
            updateUI = True


class ProgrammerApplication:

    def init(self):
        global updateUI

        # Keep track of all the threads
        self.threads: list[LabelThread] = []

        # Keep a list of serial ports
        self.swadgeSerialPorts: list[serial.tools.list_ports_common.ListPortInfo] = []

        # Grid parameters
        self.numRows: int = 1
        self.numCols: int = 1

        # Create the UI root
        self.root = tk.Tk()

        # Set up a menu, just to quit
        menubar = tk.Menu(self.root)
        filemenu = tk.Menu(menubar, tearoff=0)
        filemenu.add_command(label="Exit", command=exit_function)
        menubar.add_cascade(label="File", menu=filemenu)
        self.root.config(menu=menubar)

        # Override the close button
        self.root.protocol("WM_DELETE_WINDOW", exit_function)

        # Start the main UI loop
        self.root.winfo_toplevel().title("ESP32-S2 Swadge Programmer")

        # Set the initial window size
        self.root.geometry("500x200")

        # Run this loop until it should quit
        self.shouldQuit: bool = False
        while not self.shouldQuit:

            # First get a list of all current serial ports
            serialPortList: list[serial.tools.list_ports_common.ListPortInfo] = serial.tools.list_ports.comports()
            serialPort: serial.tools.list_ports_common.ListPortInfo

            # Check for added serial ports
            for serialPort in serialPortList:
                # If the serial port exists but isn't in swadgeSerialPorts
                if serialPort not in self.swadgeSerialPorts:
                    # If the VID and PID match the ESP32-S2's values, use it
                    if(serialPort.vid == 0x303A and serialPort.pid == 0x0002):
                        self.swadgeSerialPorts.append(serialPort)
                        # Then create a thread for esptool associated with this serial port and label and run it
                        programmerThread = LabelThread(_serialPort=serialPort)
                        self.threads.append(programmerThread)
                        updateUI = True
                        programmerThread.start()

            # Check for removed serial ports
            for serialPort in self.swadgeSerialPorts:
                # If the serial port doesn't exist anymore
                if serialPort not in serialPortList:
                    # Remove it
                    self.swadgeSerialPorts.remove(serialPort)
                    # Stop any associated threads
                    for thread in self.threads:
                        if thread.serialPort == serialPort:
                            thread.join()
                            self.threads.remove(thread)
                            updateUI = True

            # Update UI
            self.adjustRows()
            self.root.update_idletasks()
            self.root.update()

    def exit(self):
        global updateUI
        self.shouldQuit = True
        if len(self.threads) > 0:
            self.root.update()
            # Wait for all threads to actually stop
            for thread in self.threads:
                thread.join()
                self.threads.remove(thread)
                updateUI = True
        # Destroy the UI
        self.root.destroy()

    def adjustRows(self):
        global updateUI

        if updateUI:
            updateUI = False

            # Figure out how many rows and columns there are
            self.numRows = len(self.threads)
            if 0 == self.numRows:
                self.numRows = 1
            self.numCols = 1

            # Remove all entries from the grid
            tk.Grid.grid_remove(self.root)
            for widget in self.root.grid_slaves():
                tk.Grid.grid_remove(widget)

            # Set up rows and columns
            for col in range(self.numCols):
                tk.Grid.columnconfigure(self.root, col, weight=1)
            for row in range(self.numRows):
                tk.Grid.rowconfigure(self.root, row, weight=1)

            # Add each label from the thread to the grid
            rowIdx: int = 0
            colIdx: int = 0
            if len(self.threads) > 0:
                # For each serial port
                for thread in self.threads:
                    # Add label from the thread to the grid
                    serialPortLabel = tk.Label(self.root, text=thread.getLabelText(), bg=thread.getLabelColor())
                    serialPortLabel.grid(row=rowIdx, column=colIdx, sticky=[tk.W, tk.E, tk.N, tk.S])
                    colIdx = colIdx + 1
                    if(colIdx == self.numCols):
                        colIdx = 0
                        rowIdx = rowIdx + 1
            else:
                # No serial ports, print a reminder
                serialPortLabel = tk.Label(self.root, text="Connect a Swadge")
                serialPortLabel.grid(row=rowIdx, column=colIdx, sticky=[tk.W, tk.E, tk.N, tk.S])


def exit_function():
    app.exit()


if __name__ == "__main__":
    app = ProgrammerApplication()
    app.init()
