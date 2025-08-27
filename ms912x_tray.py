#!/usr/bin/env python3
"""Simple tray utility for the ms912x driver.

The script lives in the system tray and allows choosing a video mode for the
ms912x USB-HDMI adapter.  It parses ``modetest`` output to find the connected
connector and the currently active mode and then invokes ``modetest`` via
``pkexec`` to switch resolutions.

The program is intended to run under KDE on Wayland.  Notifications are not
used; all interaction happens through the tray icon menu.
"""

from __future__ import annotations

import os
import re
import subprocess
import sys
from typing import Optional

from PyQt6.QtGui import QIcon
from PyQt6.QtWidgets import (
    QAction,
    QApplication,
    QMenu,
    QSystemTrayIcon,
)

DRIVER_NAME = "ms912x"
MODES = ["1920x1080", "1280x720", "1024x768", "800x600"]

# Environment required to access the device on Wayland.
WAYLAND_ENV = {
    "DISPLAY": ":1",
    "WAYLAND_DISPLAY": "wayland-0",
    "XDG_SESSION_TYPE": "wayland",
}


def driver_loaded() -> bool:
    """Return True if the kernel module is currently loaded."""

    try:
        out = subprocess.check_output(["lsmod"], text=True)
    except Exception:
        return False
    return DRIVER_NAME in out


def unload_driver() -> None:
    """Attempt to unload the ms912x kernel module."""

    try:
        subprocess.run(["pkexec", "rmmod", DRIVER_NAME], check=False)
    except FileNotFoundError:
        # Fallback in case pkexec is unavailable.
        subprocess.run(["rmmod", DRIVER_NAME], check=False)


def _run_modetest(args: list[str]) -> str:
    """Run ``modetest`` with the required Wayland environment."""

    cmd = ["modetest", "-M", DRIVER_NAME] + args
    env = dict(os.environ)
    env.update(WAYLAND_ENV)
    result = subprocess.run(
        cmd,
        text=True,
        capture_output=True,
        env=env,
        check=False,
    )
    return result.stdout


def _get_connector_id() -> Optional[str]:
    """Return the id of the connected connector, if any."""

    out = _run_modetest(["-p"])
    for line in out.splitlines():
        if "connected" in line:
            # id is the first column
            return line.strip().split()[0]
    return None


def _get_current_mode() -> Optional[str]:
    """Return current mode name such as ``1920x1080`` or ``None``."""

    out = _run_modetest(["-p"])
    mode_re = re.compile(r"^\s*\*\s*\d+\s+(\d+x\d+)")
    for line in out.splitlines():
        match = mode_re.match(line)
        if match:
            return match.group(1)
    return None


class Tray:
    def __init__(self) -> None:
        self.app = QApplication(sys.argv)
        self.tray = QSystemTrayIcon(QIcon.fromTheme("dialog-information"))

        self.menu = QMenu()

        current = _get_current_mode() or "неизвестно"
        self.current_mode = QAction(f"Текущий режим: {current}")
        self.current_mode.setEnabled(False)
        self.menu.addAction(self.current_mode)
        self.menu.addSeparator()

        for mode in MODES:
            action = self.menu.addAction(mode)
            action.triggered.connect(lambda _=False, m=mode: self.set_mode(m))

        self.menu.addSeparator()
        exit_action = self.menu.addAction("Выход")
        exit_action.triggered.connect(self.on_exit)

        self.tray.setContextMenu(self.menu)
        self.tray.show()

    # ------------------------------------------------------------------
    def set_mode(self, mode: str) -> None:
        connector = _get_connector_id()
        if not connector:
            return

        cmd = [
            "pkexec",
            "env",
            f"DISPLAY={WAYLAND_ENV['DISPLAY']}",
            f"WAYLAND_DISPLAY={WAYLAND_ENV['WAYLAND_DISPLAY']}",
            f"XDG_SESSION_TYPE={WAYLAND_ENV['XDG_SESSION_TYPE']}",
            "modetest",
            "-M",
            DRIVER_NAME,
            "-s",
            f"{connector}:{mode}-60",
        ]
        subprocess.run(cmd, check=False)
        self.current_mode.setText(f"Текущий режим: {mode}")

    # ------------------------------------------------------------------
    def on_exit(self) -> None:
        unload_driver()
        self.app.quit()

    # ------------------------------------------------------------------
    def run(self) -> None:
        self.app.exec()


if __name__ == "__main__":
    if driver_loaded():
        Tray().run()

