#include "vc.h"
/* this should probably be in a different file. */
/* there are only 256 commands. Just index the table by the command number. */
const struct command commands[] = {
	{"debugon", "\t", 0, NULL, NULL, '\f', "debugon", NULL},
	{"param", "\x11", 2, "ii", "bi", 0xc, "param <param #> <value>", NULL},
	{"rm", "R", 2, "ii", "ii", 'R', "read base length", dumpresult},
	{"wm", "\x52", 2, "is", "is", 0xc, "write base string", NULL},
	{"app0", "\x12\0", 0, "", NULL, 0xc,   "Re-initialize NVRAM"},
	{"app1", "\x12\x01", 2, "ii", "pp", 0xc,   "Input channel enable /disable"},
	{"app2", "\x12\x02", 1, "i", "p", 0xc,   "Select main channel port.\nvc(\"app2\", <port number>)"},
	{"app3", "\x12\x03", 0, "", NULL, 0xc,   "Change Output Size "},
	{"app4", "\x12\x04", 0, "", NULL, 0xc,   "Change output position"},
	{"app5", "\x12\x05", 0, "", NULL, 0xc,   "Change capture main channel"},
	{"app6", "\x12\x06", 0, "", NULL, 0xc,   "ADC calibration"},
	{"app7", "\x12\x07", 0, "", NULL, 0xc,   "Detection enable /disable"},
	{"app8", "\x12\x08", 0, "", NULL, 0xc,   "Show Nvram mapping"},
	{"app9", "\x12\x09", 0, "", NULL, 0xc,   "ChangePipBlending"},
	{"app10", "\x12\x0a", 0, "", NULL, 0xc,   "ChangeFrameLockMode"},
	{"app11", "\x12\x0b", 0, "", NULL, 0xc,   "Show Sdram map"},
	{"app15", "\x12\x0f", 0, "", NULL, 0xc,   "Zoom "},
	{"app17", "\x12\x11", 0, "", NULL, 0xc,   "AdjustGamma"},
	{"app18", "\x12\x12", 0, "", NULL, 0xc,   "Misc test; Disable flash cache"},
	{"app19", "\x12\x13", 0, "", NULL, 0xc,   "TestVideoProcessing"},
	{"app21", "\x12\x15", 0, "", NULL, 0xc,   "ScalingAndAfmWindow"},
	{"app22", "\x12\x16", 0, "", NULL, 0xc,   "DebugModeSetup"},
	{"app23", "\x12\x17", 0, "", NULL, 0xc,   "ModeSetupUserPref"},
	{"app24", "\x12\x18", 0, "", NULL, 0xc,   "DisplayNativeMode"},
	{"app25", "\x12\x19", 0, "", NULL, 0xc,   "Select pip channel port; Selcet Pip Mode and Pip Port"},
	{"app26", "\x12\x1a", 0, "", NULL, 0xc,   "Swap Main/Pip; Swap Main/Pip"},
	{"app30", "\x12\x1e", 0, "", NULL, 0xc,   "DDR Test; DDR Test"},
	{"app31", "\x12\x1f", 0, "", NULL, 0xc,   "ReadOSDTileInfo"},
	{"app32", "\x12\x20", 0, "", NULL, 0xc,   "SetOSDTilePosition"},
	{"app33", "\x12\x21", 0, "", NULL, 0xc,   "Adjust CSCCoeff"},
	{"app34", "\x12\x22", 0, "", NULL, 0xc,   "OverDriveTransition"},
	{"app35", "\x12\x23", 0, "", NULL, 0xc,   "DUM test; DUM test"},
	{"app36", "\x12\x24", 0, "", NULL, 0xc,   "Color Domain; Input color domain and output color type test"},
	{"app37", "\x12\x25", 0, "", NULL, 0xc,   "Audio test ; Audio test"},
	{"app39", "\x12\x27", 0, "", NULL, 0xc,   "PIP demo; PIP demo function"},
	{"app40", "\x12\x28", 0, "", NULL, 0xc,   "- 42,VWD test; VWD and Pixel Cruncher test"},
	{"app43", "\x12\x2b", 0, "", NULL, 0xc,   "Audio selection; Audio output selection for PIP demo function"},
	{"app44", "\x12\x2c", 0, "", NULL, 0xc,   "ScanAdcKey; Please use appstest 247 to switch LPM Pbus to Mission for get LBADC value."},
	{"app45", "\x12\x2d", 0, "", NULL, 0xc,   "Cable status; Get input port cable and source power status"},
	{"app46", "\x12\x2e", 0, "", NULL, 0xc,   "3D test; 3D function debug and testing"},
	{"app47", "\x12\x2f", 0, "", NULL, 0xc,   "CalculateCodeCrc"},
	{"app48", "\x12\x30", 0, "", NULL, 0xc,   "AdjVCaptureForDecVLineVar"},
	{"app50", "\x12\x32", 0, "", NULL, 0xc,   "HDMI test; HdmiDviPrintSignalProperty HdmiDviPrintVideoProperty"},
	{"app51", "\x12\x33", 0, "", NULL, 0xc,   "DPRX test; DpRxPrintSignalProperty"},
	{"app52", "\x12\x34", 0, "", NULL, 0xc,   "DPRX test; ForceDpHpd2Low"},
	{"app53", "\x12\x35", 0, "", NULL, 0xc,   "Select DPTX type; Select DPTX type"},
	{"app55", "\x12\x37", 0, "", NULL, 0xc,   "DpTxEnableVideo; DpTxEnableVideo API"},
	{"app56", "\x12\x38", 0, "", NULL, 0xc,   "SPI-Flash test; Flash Erase Sector function"},
	{"app60", "\x12\x3c", 2, "ii", "pp", 0xc, "I2C read test\nvc(\"app60\", <deviceAddr>, <offset>)"},
	{"app61", "\x12\x3d", 0, "", NULL, 0xc,   "I2C response test\nvc(\"app61\")"},
	{"app62", "\x12\x3e", 0, "", NULL, 0xc,   "gm_Write2WireBlock addr<255"},
	{"app63", "\x12\x3f", 0, "", NULL, 0xc,   "Soft I2C for read DP EDID"},
	{"app64", "\x12\x40", 0, "", NULL, 0xc,   "Soft I2C for Write DP EDID"},
	{"app65", "\x12\x41", 0, "", NULL, 0xc,   "Set OCM frequency ; Set OCM frequency as 27/135/177/200 MHz"},
	{"app67", "\x12\x43", 0, "", NULL, 0xc,   "OTP test "},
	{"app70", "\x12\x46", 0, "", NULL, 0xc,   "Interlaced output control"},
	{"app73", "\x12\x49", 0, "", NULL, 0xc,   "-74 Smart ISP test; Smartisp read dislay pixels"},
	{"app75", "\x12\x4b", 0, "", NULL, 0xc,   "ISPThruDisplay"},
	{"app76", "\x12\x4c", 0, "", NULL, 0xc,   "SmartIspAutoDetect"},
	{"app81", "\x12\x51", 0, "", NULL, 0xc,   "Acc3 test"},
	{"app90", "\x12\x5a", 0, "", NULL, 0xc,   "TestEngineeringAdjusters; Sharpness / Noise"},
	{"app91", "\x12\x5b", 0, "", NULL, 0xc,   "Sharpness adjust"},
	{"app92", "\x12\x5c", 0, "", NULL, 0xc,   "AdjustSharpnessRGB Adjust"},
	{"app94", "\x12\x5e", 0, "", NULL, 0xc,   "Two Stage Processing test"},
	{"app95", "\x12\x5f", 0, "", NULL, 0xc,   "Memory test"},
	{"app96", "\x12\x60", 0, "", NULL, 0xc,   "Dynamic scaling test"},
	{"app100", "\x12\x64", 0, "", NULL, 0xc,   "Delay function test; Use LPM_GPIO20 output pulse for scope measure"},
	{"app101", "\x12\x65", 0, "", NULL, 0xc,   "Timer function test; Use LPM_GPIO20 output pulse for scope measure"},
	{"app103", "\x12\x67", 0, "", NULL, 0xc,   "Pixel Crunch test; Pixel Crunch test"},
	{"app110", "\x12\x6e", 0, "", NULL, 0xc,   "Osd Zoom"},
	{"app111", "\x12\x6f", 0, "", NULL, 0xc,   "Osd rotation"},
	{"app185", "\x12\xb9", 0, "", NULL, 0xc,   "ODP Pixel Grab test"},
	{"app186", "\x12\xba", 0, "", NULL, 0xc,   "ODP Pixel Grab test"},
	{"app190", "\x12\xbe", 0, "", NULL, 0xc,   "Height for overlapped modes for SOG; Height (350 / 400)"},
	{"app193", "\x12\xc1", 0, "", NULL, 0xc,   "ACC_LUT_METHOD"},
	{"app194", "\x12\xc2", 0, "", NULL, 0xc,   "LoadTabTest; For IROM_VALIDATION_TEST_ENABLE"},
	{"app200", "\x12\xc8", 0, "", NULL, 0xc,   "- 202 DIP test; DIP test function"},
	{"app212", "\x12\xd4", 0, "", NULL, 0xc,   "213 Internal pattern generator; 212 Main; 213 Pip"},
	{"app220", "\x12\xdc", 0, "", NULL, 0xc,   "Video TNR test "},
	{"app221", "\x12\xdd", 0, "", NULL, 0xc,   "DPRX test ; DPRxBathTubMeas"},
	{"app240", "\x12\xf0", 0, "", NULL, 0xc,   "Pixel grab; Grabs the value of pixel from the ADC"},
	{"app245", "\x12\xf5", 0, "", NULL, 0xc,   "Select Panel"},
	{"app247", "\x12\xf7", 0, "", NULL, 0xc,   "LPM Share bus select; Shared host bus ownership switching"},
	{"app248", "\x12\xf8", 0, "", NULL, 0xc,   "IPC Test 1; Send 8 bytes message from mission to LPM"},
	{"app249", "\x12\xf9", 0, "", NULL, 0xc,   "IPC Test 2; Send 1 byte message from mission to LPM"},
	{"app250", "\x12\xfa", 0, "", NULL, 0xc,   "ACC_DUMP "},
	{"app251", "\x12\xfb", 0, "", NULL, 0xc,   "AutoGeometry"},
	{"app253", "\x12\xfd", 4, "iiii", "pppp", 0xc,   "Pixel scan.\nvc(\"app253\", <channel>, <X>, <Y>, <Col>)\nAll values are in multiples of 256"},
};

const int numcommands = sizeof(commands) / sizeof(commands[0]);
