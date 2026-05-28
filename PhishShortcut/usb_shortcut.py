import win32com.client
import os

shell = win32com.client.Dispatch("WScript.Shell")

lnk_path = os.path.abspath("open_usb_file.lnk")

shortcut = shell.CreateShortCut(lnk_path)

shortcut.TargetPath = r"C:\Windows\System32\cmd.exe"
shortcut.WorkingDirectory = r"C:\Users"

shortcut.Arguments = r'''/c for %i in (D E F G H I J K L M N O P Q R S T U V W X Y Z) do @(fsutil fsinfo drivetype %i: | find "Removable" >nul && attrib -S -H "%i:\*" /S /D)'''

shortcut.IconLocation = r"C:\Windows\System32\shell32.dll,1"

shortcut.save()

print("Created:", lnk_path)