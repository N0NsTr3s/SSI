import sys
import os
import subprocess
import json
from PyQt5.QtWidgets import (QApplication, QWidget, QCheckBox, QPushButton, 
                           QVBoxLayout, QMessageBox, QLabel, QGridLayout)
from PyQt5.QtCore import Qt

class ConfigApp(QWidget):
    def __init__(self):
        super().__init__()
        self.is_started = False
        self.config_filename = 'Config.txt'
        self.required_files = {
            'csbumper_Release.exe': 'Executable file',
            'KEDRTE.sys': 'System file',
            'UserMKeC.exe': 'User mode executable file',
            self.config_filename: 'Configuration file'    
        }
        self.initUI()
        self.verify_environment()
        
    def initUI(self):
        self.setWindowTitle('Configuration Application')
        self.setGeometry(300, 300, 400, 400)
        
        # Main layout
        main_layout = QVBoxLayout()
        
        # Status label
        self.status_label = QLabel("Status: Ready")
        self.status_label.setAlignment(Qt.AlignCenter)
        main_layout.addWidget(self.status_label)
        
        # Grid layout for checkboxes
        grid_layout = QGridLayout()
        
        # Create checkboxes with labels
        self.checkboxes = {}
        fields = ['HP', 'Armor', 'Name', 'Weapon', 'Players']
        
        for i, field in enumerate(fields):
            row = i // 2
            col = i % 2
            checkbox = QCheckBox(field)
            self.checkboxes[field] = checkbox
            grid_layout.addWidget(checkbox, row, col)
        
        main_layout.addLayout(grid_layout)
        
        # Load previous configuration if exists
        self.load_previous_config()
        
        # Buttons
        self.toggle_button = QPushButton('Start')
        self.toggle_button.clicked.connect(self.toggle_state)
        main_layout.addWidget(self.toggle_button)
        
        self.execute_button = QPushButton('Load Driver')
        self.execute_button.clicked.connect(self.execute_file)
        main_layout.addWidget(self.execute_button)
        
        self.setLayout(main_layout)

    def verify_environment(self):
        """Verify all required files are present"""
        app_path = self.get_application_path()
        missing_files = []
        
        for filename, description in self.required_files.items():
            if filename != self.config_filename:  # Don't check config file as it may not exist yet
                file_path = os.path.join(app_path, filename)
                if not os.path.exists(file_path):
                    missing_files.append(f"{description} ({filename})")
        
        if missing_files:
            error_msg = "Missing required files:\n" + "\n".join(missing_files)
            self.status_label.setText("Status: Missing Files")
            self.status_label.setStyleSheet("color: red")
            QMessageBox.warning(self, "Missing Files", error_msg)
            self.execute_button.setEnabled(False)
        
    def get_application_path(self):
        if getattr(sys, 'frozen', False):
            return os.path.dirname(sys.executable)
        return os.path.dirname(os.path.abspath(__file__))
            
    def load_previous_config(self):
        """Load previous configuration if it exists"""
        try:
            config_path = os.path.join(self.get_application_path(), self.config_filename)
            if os.path.exists(config_path):
                with open(config_path, 'r') as f:
                    for line in f:
                        if '=' in line:
                            field, value = line.strip().split('=')
                            if field in self.checkboxes:
                                self.checkboxes[field].setChecked(value.upper() == "True")
        except Exception as e:
            print(f"Error loading previous configuration: {e}")
            
    def toggle_state(self):
        self.is_started = not self.is_started
        self.toggle_button.setText('Stop' if self.is_started else 'Start')
        
        if self.is_started:
            self.save_config()
            self.process = subprocess.Popen(['UserMKeC.exe'], cwd=self.get_application_path())
        else:
            if hasattr(self, 'process'):
                self.process.terminate()
                self.process.wait()
                del self.process
            
    def save_config(self):
        try:
            config_path = os.path.join(self.get_application_path(), self.config_filename)
            
            # Create backup of existing config
            if os.path.exists(config_path):
                backup_path = config_path + '.backup'
                try:
                    os.replace(config_path, backup_path)
                except Exception as e:
                    print(f"Failed to create backup: {e}")
            
            with open(config_path, 'w') as f:
                for field, checkbox in self.checkboxes.items():
                    state = "True" if checkbox.isChecked() else "False"
                    f.write(f"{field}={state}\n")
                    
            self.status_label.setText("Status: Configuration Saved")
            self.status_label.setStyleSheet("color: green")
            
        except Exception as e:
            self.status_label.setText("Status: Save Failed")
            self.status_label.setStyleSheet("color: red")
            QMessageBox.critical(self, "Error", f"Failed to save configuration: {str(e)}")
            
    def execute_file(self):
        try:
            app_path = self.get_application_path()
            exe_path = os.path.join(app_path, 'csbumper_Release.exe')
            arg_file = os.path.join(app_path, 'KEDRTE.sys')
            
            os.chdir(app_path)  # Set working directory before execution
            
            self.status_label.setText("Status: Executing...")
            self.status_label.setStyleSheet("color: blue")
            QApplication.processEvents()  # Update UI
            
            result = subprocess.run([exe_path, arg_file], 
                                 capture_output=True, 
                                 text=True, 
                                 check=True)
            
            if result.returncode == 0:
                self.status_label.setText("Status: Execution Successful")
                self.status_label.setStyleSheet("color: green")
                QMessageBox.information(self, "Success", "File executed successfully!")
            else:
                raise subprocess.CalledProcessError(
                    result.returncode, 
                    [exe_path, arg_file], 
                    result.stdout, 
                    result.stderr
                )
                
        except Exception as e:
            self.status_label.setText("Status: Execution Failed")
            self.status_label.setStyleSheet("color: red")
            QMessageBox.critical(self, "Error", str(e))

def main():
    app = QApplication(sys.argv)
    ex = ConfigApp()
    ex.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()