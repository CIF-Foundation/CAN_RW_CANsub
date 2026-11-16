<?xml version='1.0' encoding='UTF-8'?>
<Project Type="Project" LVVersion="25008000">
	<Property Name="NI.LV.All.SaveVersion" Type="Str">25.0</Property>
	<Property Name="NI.LV.All.SourceOnly" Type="Bool">true</Property>
	<Item Name="My Computer" Type="My Computer">
		<Property Name="server.app.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.control.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.tcp.enabled" Type="Bool">false</Property>
		<Property Name="server.tcp.port" Type="Int">0</Property>
		<Property Name="server.tcp.serviceName" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.tcp.serviceName.default" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.vi.callsEnabled" Type="Bool">true</Property>
		<Property Name="server.vi.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="specify.custom.address" Type="Bool">false</Property>
		<Item Name="Dependencies" Type="Dependencies"/>
		<Item Name="Build Specifications" Type="Build"/>
	</Item>
	<Item Name="Monroe-PXIe-8862" Type="RT PXI Chassis">
		<Property Name="alias.name" Type="Str">Monroe-PXIe-8862</Property>
		<Property Name="alias.value" Type="Str">192.168.0.42</Property>
		<Property Name="CCSymbols" Type="Str">TARGET_TYPE,RT;OS,Linux;CPU,x64;</Property>
		<Property Name="host.ResponsivenessCheckEnabled" Type="Bool">true</Property>
		<Property Name="host.ResponsivenessCheckPingDelay" Type="UInt">5000</Property>
		<Property Name="host.ResponsivenessCheckPingTimeout" Type="UInt">1000</Property>
		<Property Name="host.TargetCPUID" Type="UInt">9</Property>
		<Property Name="host.TargetOSID" Type="UInt">19</Property>
		<Property Name="target.cleanupVisa" Type="Bool">false</Property>
		<Property Name="target.FPProtocolGlobals_ControlTimeLimit" Type="Int">300</Property>
		<Property Name="target.getDefault-&gt;WebServer.Port" Type="Int">80</Property>
		<Property Name="target.getDefault-&gt;WebServer.Timeout" Type="Int">60</Property>
		<Property Name="target.IOScan.Faults" Type="Str"></Property>
		<Property Name="target.IOScan.NetVarPeriod" Type="UInt">100</Property>
		<Property Name="target.IOScan.NetWatchdogEnabled" Type="Bool">false</Property>
		<Property Name="target.IOScan.Period" Type="UInt">10000</Property>
		<Property Name="target.IOScan.PowerupMode" Type="UInt">0</Property>
		<Property Name="target.IOScan.Priority" Type="UInt">0</Property>
		<Property Name="target.IOScan.ReportModeConflict" Type="Bool">true</Property>
		<Property Name="target.IsRemotePanelSupported" Type="Bool">true</Property>
		<Property Name="target.RTCPULoadMonitoringEnabled" Type="Bool">true</Property>
		<Property Name="target.RTDebugWebServerHTTPPort" Type="Int">8001</Property>
		<Property Name="target.RTTarget.ApplicationPath" Type="Path">/c/ni-rt/startup/startup.rtexe</Property>
		<Property Name="target.RTTarget.EnableFileSharing" Type="Bool">true</Property>
		<Property Name="target.RTTarget.IPAccess" Type="Str">+*</Property>
		<Property Name="target.RTTarget.LaunchAppAtBoot" Type="Bool">false</Property>
		<Property Name="target.RTTarget.VIPath" Type="Path">/home/lvuser/natinst/bin</Property>
		<Property Name="target.server.app.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="target.server.control.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="target.server.tcp.access" Type="Str">+*</Property>
		<Property Name="target.server.tcp.enabled" Type="Bool">false</Property>
		<Property Name="target.server.tcp.paranoid" Type="Bool">true</Property>
		<Property Name="target.server.tcp.port" Type="Int">3363</Property>
		<Property Name="target.server.tcp.serviceName" Type="Str">Main Application Instance/VI Server</Property>
		<Property Name="target.server.tcp.serviceName.default" Type="Str">Main Application Instance/VI Server</Property>
		<Property Name="target.server.vi.access" Type="Str">+*</Property>
		<Property Name="target.server.vi.callsEnabled" Type="Bool">true</Property>
		<Property Name="target.server.vi.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="target.WebServer.Config" Type="Str">Listen 8000

NI.ServerName default
DocumentRoot "$LVSERVER_DOCROOT"
TypesConfig "$LVSERVER_CONFIGROOT/mime.types"
DirectoryIndex index.htm
WorkerLimit 10
InactivityTimeout 60

LoadModulePath "$LVSERVER_MODULEPATHS"
LoadModule LVAuth lvauthmodule
LoadModule LVRFP lvrfpmodule

#
# Pipeline Definition
#

SetConnector netConnector

AddHandler LVAuth
AddHandler LVRFP

AddHandler fileHandler ""

AddOutputFilter chunkFilter


</Property>
		<Property Name="target.WebServer.Enabled" Type="Bool">false</Property>
		<Property Name="target.WebServer.LogEnabled" Type="Bool">false</Property>
		<Property Name="target.WebServer.LogPath" Type="Path">/c/ni-rt/system/www/www.log</Property>
		<Property Name="target.WebServer.Port" Type="Int">80</Property>
		<Property Name="target.WebServer.RootPath" Type="Path">/c/ni-rt/system/www</Property>
		<Property Name="target.WebServer.TcpAccess" Type="Str">c+*</Property>
		<Property Name="target.WebServer.Timeout" Type="Int">60</Property>
		<Property Name="target.WebServer.ViAccess" Type="Str">+*</Property>
		<Property Name="target.webservices.SecurityAPIKey" Type="Str">PqVr/ifkAQh+lVrdPIykXlFvg12GhhQFR8H9cUhphgg=:pTe9HRlQuMfJxAG6QCGq7UvoUpJzAzWGKy5SbZ+roSU=</Property>
		<Property Name="target.webservices.ValidTimestampWindow" Type="Int">15</Property>
		<Item Name="Examples" Type="Folder">
			<Item Name="CANsub_API_Loopback_Example.vi" Type="VI" URL="../examples/CANsub_API_Loopback_Example.vi"/>
			<Item Name="Example_Frames_To_Rows.vi" Type="VI" URL="../examples/Example_Frames_To_Rows.vi"/>
			<Item Name="Example_Loopback_Check.vi" Type="VI" URL="../examples/Example_Loopback_Check.vi"/>
			<Item Name="Example_Loopback_Send.vi" Type="VI" URL="../examples/Example_Loopback_Send.vi"/>
			<Item Name="Example_Loopback_Stats.ctl" Type="VI" URL="../examples/Example_Loopback_Stats.ctl"/>
			<Item Name="Example_Table_Append.vi" Type="VI" URL="../examples/Example_Table_Append.vi"/>
		</Item>
		<Item Name="Tests" Type="Folder">
			<Item Name="Echo" Type="Folder">
				<Item Name="subvis" Type="Folder">
					<Item Name="Echo_Classify.vi" Type="VI" URL="../echo/Echo_Classify.vi"/>
					<Item Name="Echo_Classify_API.vi" Type="VI" URL="../echo/Echo_Classify_API.vi"/>
					<Item Name="Echo_Handle_Frames.vi" Type="VI" URL="../echo/Echo_Handle_Frames.vi"/>
					<Item Name="Echo_Handle_Frames_API.vi" Type="VI" URL="../echo/Echo_Handle_Frames_API.vi"/>
					<Item Name="Echo_Send_Echo.vi" Type="VI" URL="../echo/Echo_Send_Echo.vi"/>
					<Item Name="Echo_Send_Echo_API.vi" Type="VI" URL="../echo/Echo_Send_Echo_API.vi"/>
					<Item Name="Echo_Send_Report.vi" Type="VI" URL="../echo/Echo_Send_Report.vi"/>
				</Item>
				<Item Name="Typedefs" Type="Folder">
					<Item Name="Echo_Config.ctl" Type="VI" URL="../echo/Echo_Config.ctl"/>
					<Item Name="Echo_Context.ctl" Type="VI" URL="../echo/Echo_Context.ctl"/>
					<Item Name="Echo_Counters.ctl" Type="VI" URL="../echo/Echo_Counters.ctl"/>
					<Item Name="Echo_Frame.ctl" Type="VI" URL="../echo/Echo_Frame.ctl"/>
					<Item Name="Echo_Frame_Kind.ctl" Type="VI" URL="../echo/Echo_Frame_Kind.ctl"/>
					<Item Name="Echo_Time_Store.ctl" Type="VI" URL="../echo/Echo_Time_Store.ctl"/>
					<Item Name="Test_RX_Stats.ctl" Type="VI" URL="../tests/typedefs/Test_RX_Stats.ctl"/>
				</Item>
				<Item Name="CANsub_Echo_API.vi" Type="VI" URL="../echo/CANsub_Echo_API.vi"/>
				<Item Name="CANsub_Echo_C.vi" Type="VI" URL="../echo/CANsub_Echo_C.vi"/>
			</Item>
			<Item Name="Test_API_Open_Close.vi" Type="VI" URL="../tests/Test_API_Open_Close.vi"/>
			<Item Name="Test_API_Receive.vi" Type="VI" URL="../tests/Test_API_Receive.vi"/>
			<Item Name="Test_API_Write.vi" Type="VI" URL="../tests/Test_API_Write.vi"/>
			<Item Name="Test_API_Write_Raw.vi" Type="VI" URL="../tests/Test_API_Write_Raw.vi"/>
			<Item Name="Test_C_CIF.vi" Type="VI" URL="../tests/Test_C_CIF.vi"/>
			<Item Name="Test_C_Codec_RoundTrip.vi" Type="VI" URL="../tests/Test_C_Codec_RoundTrip.vi"/>
			<Item Name="Test_C_Decode_To_Frames.vi" Type="VI" URL="../tests/Test_C_Decode_To_Frames.vi"/>
			<Item Name="Test_C_Receive.vi" Type="VI" URL="../tests/Test_C_Receive.vi"/>
			<Item Name="Test_C_RT_Path.vi" Type="VI" URL="../tests/Test_C_RT_Path.vi"/>
			<Item Name="Test_C_Smoke.vi" Type="VI" URL="../tests/Test_C_Smoke.vi"/>
			<Item Name="Test_C_W12_W13_Quick.vi" Type="VI" URL="../tests/Test_C_W12_W13_Quick.vi"/>
			<Item Name="Test_C_W12_W13_W14.vi" Type="VI" URL="../tests/Test_C_W12_W13_W14.vi"/>
			<Item Name="Test_C_W12_W13_W14_W15.vi" Type="VI" URL="../tests/Test_C_W12_W13_W14_W15.vi"/>
			<Item Name="Test_C_WS_to_CAN_to_WS.vi" Type="VI" URL="../tests/Test_C_WS_to_CAN_to_WS.vi"/>
			<Item Name="Test_CLFN_Path_Cost.vi" Type="VI" URL="../tests/Test_CLFN_Path_Cost.vi"/>
			<Item Name="Test_Echo_Classify.vi" Type="VI" URL="../tests/Test_Echo_Classify.vi"/>
			<Item Name="Test_Echo_Send_Report.vi" Type="VI" URL="../tests/Test_Echo_Send_Report.vi"/>
			<Item Name="Test_REST_Call.vi" Type="VI" URL="../tests/Test_REST_Call.vi"/>
			<Item Name="Test_RX_Stats_Update.vi" Type="VI" URL="../tests/subvis/Test_RX_Stats_Update.vi"/>
			<Item Name="Test_TLS.vi" Type="VI" URL="../tests/Test_TLS.vi"/>
			<Item Name="Test_WS_Close.vi" Type="VI" URL="../tests/Test_WS_Close.vi"/>
			<Item Name="Test_WS_Handshake.vi" Type="VI" URL="../tests/Test_WS_Handshake.vi"/>
		</Item>
		<Item Name="CANsub_API.lvlib" Type="Library" URL="../api/CANsub_API.lvlib"/>
		<Item Name="CANsub_C.lvlib" Type="Library" URL="../CANsub_C.lvlib"/>
		<Item Name="CANsub_Codec.lvlib" Type="Library" URL="../codec_vis/CANsub_Codec.lvlib"/>
		<Item Name="CANsub_Common.lvlib" Type="Library" URL="../../common/CANsub_Common.lvlib"/>
		<Item Name="Dependencies" Type="Dependencies"/>
		<Item Name="Build Specifications" Type="Build"/>
	</Item>
</Project>
