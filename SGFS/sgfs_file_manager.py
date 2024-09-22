import sys
import os
import shutil
from PyQt5.QtWidgets import QApplication, QMainWindow, QFileSystemModel, QTreeView, QVBoxLayout, QWidget, QPushButton, QLabel, QFileDialog, QMessageBox, QMenu, QAction
from PyQt5.QtCore import Qt, QDir, QMimeData, QModelIndex
from PyQt5.QtGui import QDragEnterEvent, QDropEvent
import subprocess

# Backend functions for SGFS operations
def mount_sgfs(disk):
    result = subprocess.run(['./sgfs_cli', 'm', disk], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return result.stdout.decode('utf-8')

def unmount_sgfs():
    result = subprocess.run(['./sgfs_cli', 'mdd'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
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
        self.model.setRootPath('/mnt/sgfs')  # Mount point for SGFS

        # Tree view for browsing
        self.tree = QTreeView()
        self.tree.setModel(self.model)
        self.tree.setRootIndex(self.model.index('/mnt/sgfs'))
        self.tree.setColumnWidth(0, 250)

        # Set tree view to accept drops
        self.tree.setAcceptDrops(True)
        self.tree.setDragDropMode(QTreeView.DropOnly)

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

        # Right-click menu for deleting files
        self.tree.setContextMenuPolicy(Qt.CustomContextMenu)
        self.tree.customContextMenuRequested.connect(self.open_menu)

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

    def open_menu(self, position):
        indexes = self.tree.selectedIndexes()
        if len(indexes) > 0:
            menu = QMenu()

            delete_action = QAction("Delete", self)
            delete_action.triggered.connect(self.delete_selected)
            menu.addAction(delete_action)

            menu.exec_(self.tree.viewport().mapToGlobal(position))

    def delete_selected(self):
        index = self.tree.currentIndex()
        file_path = self.model.filePath(index)

        if os.path.exists(file_path):
            os.remove(file_path)
            QMessageBox.information(self, "File Deleted", f"{file_path} deleted.")
        else:
            QMessageBox.warning(self, "Error", f"Could not delete {file_path}")

    def mount_disk(self):
        """Mount SGFS disk"""
        disk, _ = QFileDialog.getOpenFileName(self, "Select Disk", "/dev", "All Files (*)")
        if disk:
            output = mount_sgfs(disk)
            self.status_label.setText(output)
            # Refresh model to show mounted disk
            self.model.setRootPath('/mnt/sgfs')
            self.tree.setRootIndex(self.model.index('/mnt/sgfs'))

    def unmount_disk(self):
        """Unmount SGFS disk"""
        output = unmount_sgfs()
        self.status_label.setText(output)
        # Clear the model since the disk is unmounted
        self.model.setRootPath('/')
        self.tree.setRootIndex(self.model.index('/'))

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
