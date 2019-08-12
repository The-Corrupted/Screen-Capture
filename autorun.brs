' HDMI Checker Script
' Author: Nameless_Flamingo

Function Main() as Void
	PRINT "Starting..."
	m = GetGlobalAA()
	SetGlobals()

	sysTime = CreateObject("roSystemTime")

	' Check registry for boot count and if it does not exist, make it
	registry = CreateObject("roRegistrySection", "counter")
	if not registry.Exists("boot_count") then
		registry.Write("boot_count", "1")
	end if
	m.itersStr = registry.Read("boot_count")
	m.iters = strtoi(m.itersStr)

	vm = CreateObject("roVideoMode")
	' Set Background color:
	vm.SetBackgroundColor(16777215)
	vm.SetPort(m.msgPort)
	fileAppend = CreateObject("roAppendFile", "edidInfo.txt")
	outstatus = {}
	outstatus = vm.GetHdmiOutputStatus()
	m.edidFound = "false"
	if type(outstatus) <> "Invalid" then
		if outstatus.output_present then
			edidInfo = vm.GetEdidIdentity(true)
			fileAppend.SendLine("*************** Iter: "+m.itersStr+" ***************")
			dateTime = sysTime.GetUtcDateTime()
			dateTimeStr = dateTime.GetString()
			fileAppend.SendLine("*************** Time: "+dateTimeStr+" ****************")
			fileAppend.Flush()
			fullStr = ""
			for each n in edidInfo
					edidInfoStr = ""
					if type(edidInfo[n]) = "roBoolean" then
						edidInfoStr = BoolToStr(edidInfo[n])
					else if type(edidInfo[n]) = "roInt" then
						edidInfoStr = IntToStr(edidInfo[n])
					else
						edidInfoStr = edidInfo[n]
					end if
					fullStr = n+": "+edidInfoStr
					fileAppend.SendLine(fullStr)
				end for
			fileAppend.Flush()
			m.iters = m.iters + 1
			m.itersStr = strI(m.iters)
			registry.Write("boot_count", m.itersStr)
			registry.Flush()
			PrintDisplay("Found HDMI. Edid Written to file.")
			m.edidFound = "true"
			SetupServer()
			'RebootSystem()
		end if
	end if

	waitForHDMI:
	while true
		msg = wait(0, m.msgPort)
		if type(msg) = "roHdmiOutputChanged" then
			PRINT "HDMI Output Changed"
			edidInfo = vm.GetEdidIdentity(true)
			strIn = "*************** Iter: "+m.itersStr+" ***************"
			fileAppend.SendLine( strIn )
			dateTime = sysTime.GetUtcDateTime()
			dateTimeStr = dateTime.GetString()
			fileAppend.SendLine("*************** Time: "+dateTimeStr+" ****************")
			fileAppend.Flush()
			fullStr = ""
			for each n in edidInfo
				edidInfoStr = ""
				if type(edidInfo[n]) = "roBoolean" then
					edidInfoStr = BoolToStr(edidInfo[n])
				else if type(edidInfo[n]) = "roInt" then
					edidInfoStr = IntToStr(edidInfo[n])
				else
					edidInfoStr = edidInfo[n]
				end if
				fullStr = n+": "+edidInfoStr
				fileAppend.SendLine(fullStr)
			end for
			fileAppend.Flush()
			m.iters = m.iters + 1
			m.itersStr = strI(m.iters)
			registry.Write("boot_count", m.itersStr)
			registry.Flush()
			PrintDisplay("Found HDMI. Edid Written to file.")
			m.edidFound = "true"
			'RebootSystem()
		else if type(msg) = "roHttpEvent" then
			PRINT "Got Http Event"
			method = msg.GetMethod()
			PRINT method
			response = m.edidFound
			msg.SetResponseBodyString(response)
			msg.SendResponse(200)
			RebootSystem()
		else
			PRINT type(msg)
			strIn = "*************** Iter: "+m.itersStr+" ***************"
			fileAppend.SendLine( strIn )
			dateTime = sysTime.GetUtcDateTime()
			dateTimeStr = dateTime.GetString()
			fileAppend.SendLine("*************** Time: "+dateTimeStr+" ****************")
			fileAppend.Flush()
			fileAppend.SendLine("FAILURE: Failed to find hdmi.")
			fileAppend.Flush()
			PrintDisplay("Failed to find HDMI connection in a timely manner.")
			'RebootSystem()
		end if
	end while
End Function

Function BoolToStr(value as Boolean) As String
	if value then
		return "True"
	else
		return "False"
	end if
End Function

Function IntToStr(value as Integer) As String
	return strI(value)
End Function

Function SetGlobals() as Void
	m = GetGlobalAA()
	v = CreateObject("roVideoMode")
	left = v.GetSafeX()
	right = v.GetSafeX() + v.GetSafeWidth()
	height = v.GetSafeHeight()-v.GetSafeY()
	top_txt = int(height*5/10)
	bottom_txt = int(height*8/10)
	txt_rect = CreateObject("roRectangle", left, top_txt, right-left, bottom_txt-top_txt)
	m.msgPort = CreateObject("roMessagePort")
	m.display = CreateObject("roTextWidget", txt_rect, 4, 2, { Alignment: 1})
	m.display.Show()
End Function

Function PrintDisplay(strToPrint as String) as Void
	m.display.Clear()
	m.display.PushString(strToPrint)
End Function

Function SetupServer() as Boolean
	m = GetGlobalAA()
	configuration = {port: "8089"}
	m.server = CreateObject("roHttpServer", configuration)
	if type(m.server) <> "roHttpServer" then
		return False
	end if
	getParam = {url_path: "/hdmistatus", user_data: "hdmi status page requested",
				body: m.edidFound, method: "GET"}
	m.server.AddMethodToString(getParam)
	PRINT m.server.GetFailureReason()
	m.server.SetPort(m.msgPort)
	PRINT m.server
End Function