#!/usr/bin/env python3
"""Tray indicator for the ms912x kernel module."""

import sys
import subprocess

from PyQt5.QtCore import QTimer
from PyQt5.QtGui import QIcon, QPixmap, QColor, QPainter
from PyQt5.QtWidgets import QApplication, QSystemTrayIcon, QMenu

DRIVER_NAME = "ms912x"


def driver_loaded() -> bool:
    """Return True if the kernel module is currently loaded."""
    try:
        out = subprocess.check_output(["lsmod"], text=True)
    except Exception:
        return False
    return DRIVER_NAME in out


def unload_driver() -> None:
    """Attempt to unload the ms912x module using pkexec."""
    try:
        subprocess.run(["pkexec", "rmmod", DRIVER_NAME], check=False)
    except FileNotFoundError:
        # pkexec not available
        pass


class Tray:
    def __init__(self):
        self.app = QApplication(sys.argv)
        self.tray = QSystemTrayIcon(self._create_icon(), self.app)
        menu = QMenu()
        unload_action = menu.addAction("Выгрузить драйвер")
        unload_action.triggered.connect(self.on_unload)
        exit_action = menu.addAction("Выход")
        exit_action.triggered.connect(self.app.quit)
        self.tray.setContextMenu(menu)
        self.tray.show()
        # Periodically check if the module is still loaded
        self.timer = QTimer()
        self.timer.timeout.connect(self.check_module)
        self.timer.start(5000)

    def _create_icon(self) -> QIcon:
        pix = QPixmap(16, 16)
        pix.fill(QColor("transparent"))
        painter = QPainter(pix)
        painter.setBrush(QColor("red"))
        painter.setPen(QColor("red"))
        painter.drawEllipse(0, 0, 15, 15)
        painter.end()
        return QIcon(pix)

    def on_unload(self):
        unload_driver()
        self.app.quit()

    def check_module(self):
        if not driver_loaded():
            self.app.quit()

    def run(self):
        self.app.exec_()


if __name__ == "__main__":
    if not driver_loaded():
        sys.exit(0)
    Tray().run()
