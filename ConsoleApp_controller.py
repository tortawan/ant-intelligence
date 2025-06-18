import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import ttkbootstrap as bs
import subprocess
import pandas as pd
import os
import threading
import queue

class SimulationLauncherApp(bs.Window):
    """
    A GUI application to launch a C++ ant simulation, configure its parameters,
    and display the resulting data from the output CSV file.
    """
    def __init__(self):
        super().__init__(themename="litera") # You can try other themes like 'superhero', 'darkly', 'litera'
        self.title("Ant Simulation Launcher")
        self.geometry("1200x800") # Increased height for the console

        # --- Data ---
        self.executable_path = tk.StringVar(value=r"D:\New_folder\CPP_Project\test-ant\x64\Release\ConsoleApp_ffmpeg.exe")
        self.output_csv_path = tk.StringVar(value="ground_data.csv")
        self.param_entries = {}
        
        # Queue for thread-safe communication with the GUI
        self.log_queue = queue.Queue()

        # --- UI Setup ---
        self.setup_ui()
        
        # Start the queue processor
        self.process_log_queue()

    def setup_ui(self):
        """Creates and arranges all the widgets in the main window."""
        # Main layout is now vertical to accommodate the console at the bottom
        main_vertical_pane = ttk.PanedWindow(self, orient=tk.VERTICAL)
        main_vertical_pane.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # Top pane contains the original horizontal layout (controls and results)
        top_pane = ttk.PanedWindow(main_vertical_pane, orient=tk.HORIZONTAL)
        main_vertical_pane.add(top_pane, weight=3)

        # --- Left Side: Controls ---
        controls_frame = bs.Frame(top_pane, padding=10)
        top_pane.add(controls_frame, weight=1)
        self.create_controls(controls_frame)

        # --- Right Side: Results Display ---
        results_frame = bs.Frame(top_pane, padding=10)
        top_pane.add(results_frame, weight=3)
        self.create_results_display(results_frame)
        
        # --- Bottom Pane: Console Output ---
        console_frame = bs.Frame(main_vertical_pane, padding=(10,0,10,10))
        main_vertical_pane.add(console_frame, weight=1)
        self.create_console_output(console_frame)


    def create_controls(self, parent_frame):
        """Creates the input fields and buttons for simulation control."""
        # This function's content remains largely the same
        path_frame = bs.Labelframe(parent_frame, text="Executable Configuration", padding=10)
        path_frame.pack(fill=tk.X, pady=(0, 10))

        ttk.Label(path_frame, text="Executable Path:").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        ttk.Entry(path_frame, textvariable=self.executable_path, width=50).grid(row=0, column=1, padx=5, pady=5, sticky="ew")
        bs.Button(path_frame, text="Browse...", command=self.browse_for_exe, bootstyle="secondary").grid(row=0, column=2, padx=5, pady=5)
        
        ttk.Label(path_frame, text="Output CSV Name:").grid(row=1, column=0, padx=5, pady=5, sticky="w")
        ttk.Entry(path_frame, textvariable=self.output_csv_path, width=50).grid(row=1, column=1, padx=5, pady=5, sticky="ew")
        path_frame.columnconfigure(1, weight=1)

        params_frame = bs.Labelframe(parent_frame, text="Simulation Parameters", padding=10)
        params_frame.pack(fill=tk.BOTH, expand=True)

        parameters = {
            "width": ("Grid Width", "100"), "length": ("Grid Length", "100"),
            "ants": ("Number of Ants", "500"), "experiments": ("Number of Experiments", "5"),
            "iterations": ("Iterations per Exp.", "50001"), "memory_size": ("Ant Memory Size", "20"),
            "threshold_start": ("Threshold Start", "0"), "threshold_end": ("Threshold End", "20"),
            "threshold_interval": ("Threshold Interval", "5"), "prob_relu_low": ("Prob. ReLU Low", "0.3"),
            "prob_relu_high": ("Prob. ReLU High", "0.7"),
        }

        row_num = 0
        for name, (label, default) in parameters.items():
            ttk.Label(params_frame, text=label).grid(row=row_num, column=0, padx=5, pady=5, sticky="w")
            entry = ttk.Entry(params_frame, width=15); entry.insert(0, default)
            entry.grid(row=row_num, column=1, padx=5, pady=5, sticky="ew")
            self.param_entries[name] = entry
            row_num += 1
        params_frame.columnconfigure(1, weight=1)

        button_frame = bs.Frame(parent_frame)
        button_frame.pack(fill=tk.X, pady=(10, 0))

        self.run_button = bs.Button(button_frame, text="Run Simulation", command=self.start_simulation_thread, bootstyle="success")
        self.run_button.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))

        self.load_button = bs.Button(button_frame, text="Load Results from CSV", command=self.load_results_from_csv, bootstyle="info-outline")
        self.load_button.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 0))

        self.status_var = tk.StringVar(value="Ready.")
        status_bar = bs.Label(parent_frame, textvariable=self.status_var, relief=tk.SUNKEN, anchor=tk.W, padding=5)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X, pady=(10, 0))

    def create_results_display(self, parent_frame):
        """Creates the table view for displaying CSV data."""
        # This function's content remains the same
        ttk.Label(parent_frame, text="Simulation Results", font="-weight bold").pack(anchor="w")
        tree_container = ttk.Frame(parent_frame)
        tree_container.pack(fill=tk.BOTH, expand=True, pady=5)
        self.results_tree = ttk.Treeview(tree_container, show="headings", bootstyle="primary")
        
        vsb = ttk.Scrollbar(tree_container, orient="vertical", command=self.results_tree.yview)
        hsb = ttk.Scrollbar(tree_container, orient="horizontal", command=self.results_tree.xview)
        self.results_tree.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)
        vsb.pack(side='right', fill='y'); hsb.pack(side='bottom', fill='x')
        self.results_tree.pack(fill=tk.BOTH, expand=True)

    def create_console_output(self, parent_frame):
        """Creates the console output text area."""
        console_labelframe = bs.Labelframe(parent_frame, text="Console Output", padding=5)
        console_labelframe.pack(fill=tk.BOTH, expand=True)
        
        self.console_output = scrolledtext.ScrolledText(console_labelframe, wrap=tk.WORD, state='disabled', font=("Consolas", 9))
        self.console_output.pack(fill=tk.BOTH, expand=True)
        
    def log_message(self, message):
        """Inserts a message into the console output widget."""
        self.console_output.config(state='normal')
        self.console_output.insert(tk.END, message)
        self.console_output.see(tk.END) # Auto-scroll
        self.console_output.config(state='disabled')

    def process_log_queue(self):
        """Processes messages from the log queue to update the GUI."""
        try:
            while True:
                message = self.log_queue.get_nowait()
                self.log_message(message)
        except queue.Empty:
            pass # No more messages
        self.after(100, self.process_log_queue) # Check again after 100ms

    def browse_for_exe(self):
        path = filedialog.askopenfilename(title="Select Executable", filetypes=(("Executable files", "*.exe"), ("All files", "*.*")))
        if path: self.executable_path.set(path)
            
    def start_simulation_thread(self):
        self.run_button.config(state=tk.DISABLED)
        self.status_var.set("Running simulation... please wait.")
        # Clear previous console output before starting
        self.console_output.config(state='normal')
        self.console_output.delete(1.0, tk.END)
        self.console_output.config(state='disabled')
        
        sim_thread = threading.Thread(target=self.run_simulation_logic, daemon=True)
        sim_thread.start()

    def run_simulation_logic(self):
        """
        The core logic for preparing and running the C++ subprocess.
        Captures output in real-time and sends it to the GUI via a queue.
        """
        exe_path = self.executable_path.get()
        if not os.path.exists(exe_path):
            messagebox.showerror("Error", f"Executable not found at:\n{exe_path}")
            self.status_var.set("Error: Executable not found.")
            self.run_button.config(state=tk.NORMAL)
            return

        command = [exe_path]
        try:
            for name, entry in self.param_entries.items():
                command.append(f"--{name}")
                command.append(entry.get())
            command.append("--csv_filename"); command.append(self.output_csv_path.get())
        except ValueError:
            messagebox.showerror("Error", "Invalid parameter value. Please ensure all inputs are correct.")
            self.status_var.set("Error: Invalid parameter.")
            self.run_button.config(state=tk.NORMAL)
            return

        try:
            # Use Popen to run the process and capture output in real-time
            process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
            
            # Read the output line by line as it is generated
            for line in iter(process.stdout.readline, ''):
                self.log_queue.put(line)

            process.stdout.close()
            return_code = process.wait()

            if return_code != 0:
                self.log_queue.put(f"\n--- SIMULATION FAILED (Exit Code: {return_code}) ---\n")
                self.status_var.set(f"Error: Simulation failed (Code: {return_code}).")
            else:
                self.log_queue.put("\n--- SIMULATION FINISHED SUCCESSFULLY ---\n")
                self.status_var.set("Simulation finished successfully. Loading results...")
                self.load_results_from_csv() # Automatically load results
                self.status_var.set("Ready.")

        except FileNotFoundError:
            messagebox.showerror("Error", f"Could not find the executable.\nPath: {exe_path}")
            self.status_var.set("Error: File not found.")
        except Exception as e:
            error_message = f"An unexpected error occurred: {e}"
            messagebox.showerror("Runtime Error", error_message)
            self.status_var.set("Error: Runtime error.")
        finally:
            self.run_button.config(state=tk.NORMAL)

    def load_results_from_csv(self):
        csv_path = self.output_csv_path.get()
        if not os.path.exists(csv_path):
            messagebox.showwarning("Warning", f"Could not find the results file:\n{csv_path}")
            return
        try:
            df = pd.read_csv(csv_path)
            for item in self.results_tree.get_children(): self.results_tree.delete(item)
            self.results_tree["columns"] = list(df.columns)
            self.results_tree["show"] = "headings"
            for col in df.columns:
                self.results_tree.heading(col, text=col)
                self.results_tree.column(col, anchor=tk.CENTER, width=100)
            for index, row in df.iterrows():
                self.results_tree.insert("", "end", values=list(row))
            self.status_var.set(f"Successfully loaded {len(df)} rows from {csv_path}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load or parse CSV file.\n\nError: {e}")
            self.status_var.set("Error: Failed to load CSV.")

if __name__ == "__main__":
    app = SimulationLauncherApp()
    app.mainloop()
