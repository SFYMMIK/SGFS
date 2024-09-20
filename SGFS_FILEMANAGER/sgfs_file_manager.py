import sys
from PyQt5.QtWidgets import QApplication, QMainWindow, QFileSystemModel, QTreeView, QVBoxLayout, QWidget, QPushButton, QLabel, QFileDialog, QMessageBox
from PyQt5.QtCore import Qt
import subprocess

# Backend functions for SGFS operations
def mount_sgfs(disk):
    result = subprocess.run(['./sgfs_cli', 'm', disk], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return result.stdout.decode('utf-8')

def unmount_sgfs():
    result = subprocess.run(['./sgfs_cli', 'mdd', 'none'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return result.stdout.decode('utf-8')

def format_sgfs(disk):
    result = subprocess.run(['./sgfs_cli', 'f', disk, '4096', '1024'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return result.stdout.decode('utf-8')

class SGFSFileManager(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('SGFS File Manager')
        self.setGeometry(100, 100, 800, 600)

        # File system model
        self.model = QFileSystemModel()
        self.model.setRootPath('/')

        # Tree view for browsing
        self.tree = QTreeView()
        self.tree.setModel(self.model)
        self.tree.setRootIndex(self.model.index('/'))
        self.tree.setColumnWidth(0, 250)

        # Status label to show mounted disk
        self.status_label = QLabel("No SGFS disk mounted.")

        # Buttons
        self.mount_button = QPushButton("Mount SGFS")
        self.unmount_button = QPushButton("Unmount SGFS")
        self.format_button = QPushButton("Format SGFS")

        # Event bindings
        self.mount_button.clicked.connect(self.mount_disk)
        self.unmount_button.clicked.connect(self.unmount_disk)
        self.format_button.clicked.connect(self.format_disk)

        # Layout
        layout = QVBoxLayout()
        layout.addWidget(self.tree)
        layout.addWidget(self.status_label)
        layout.addWidget(self.mount_button)
        layout.addWidget(self.unmount_button)
        layout.addWidget(self.format_button)

        # Set central widget
        central_widget = QWidget()
        central_widget.setLayout(layout)
        self.setCentralWidget(central_widget)

    def mount_disk(self):
        """Mount SGFS disk"""
        disk, _ = QFileDialog.getOpenFileName(self, "Select Disk", "/dev", "All Files (*)")
        if disk:
            output = mount_sgfs(disk)
            self.status_label.setText(output)

    def unmount_disk(self):
        """Unmount SGFS disk"""
        output = unmount_sgfs()
        self.status_label.setText(output)

    def format_disk(self):
        """Format disk with SGFS"""
        disk, _ = QFileDialog.getOpenFileName(self, "Select Disk to Format", "/dev", "All Files (*)")
        if disk:
            reply = QMessageBox.question(self, 'Format Disk', f"Are you sure you want to format {disk} with SGFS?",
                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if reply == QMessageBox.Yes:
                output = format_sgfs(disk)
                self.status_label.setText(output)

if __name__ == '__main__':
    app = QApplication(sys.argv)
    file_manager = SGFSFileManager()
    file_manager.show()
    sys.exit(app.exec_())
