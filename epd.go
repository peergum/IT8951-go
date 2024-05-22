package it8951

import (
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"github.com/peergum/go-rpio/v4"
	"log"
	"time"
)

type Command uint16
type Preamble uint16
type Rotate uint16
type PixelMode uint8
type EndianType uint8
type Address uint16
type VCOMCommand uint16
type DataBuffer []uint16
type DataWord uint16
type DisplayMode uint16

type DevInfo struct {
	PanelW     uint16    // width in pixels
	PanelH     uint16    // height in pixels
	MemAddrL   uint16    // low word
	MemAddrH   uint16    // high word
	FWVersion  [8]uint16 // 16 bytes string
	LUTVersion [8]uint16 // 16 bytes string
}

type LoadImgInfo struct {
	EndianType       EndianType // little or Big Endian
	PixelFormat      PixelMode  // bpp
	Rotate           Rotate     // Rotate mode
	SourceBufferAddr DataBuffer // start address of source Frame buffer
	TargetMemAddr    uint32     // base address of target image buffer
}

type AreaImgInfo struct {
	X uint16 // position in pixels
	Y uint16 // position in pixels
	W uint16 // width in pixels
	H uint16 // height in pixels
}

// IT8951 Command list
const (
	TCONSysRun  Command = 0x0001
	TCONStandby Command = 0x0002
	TCONSleep   Command = 0x0003
	TCONRegRd   Command = 0x0010
	TCONRegWr   Command = 0x0011

	TCONMemBstRdT Command = 0x0012
	TCONMemBstRdS Command = 0x0013
	TCONMemBstWr  Command = 0x0014
	TCONMemBstEnd Command = 0x0015

	TCONLdImg     Command = 0x0020
	TCONLdImgArea Command = 0x0021
	TCONLdImgEnd  Command = 0x0022

	// I80 User defined command code

	UserCmdDpyArea    Command = 0x0034
	UserCmdGetDevInfo Command = 0x0302
	UserCmdDpyBufArea Command = 0x0037
	UserCmdVCOM       Command = 0x0039
)

// Preambles
const (
	CommandPreamble Preamble = 0x6000
	WritePreamble   Preamble = 0x0000
	ReadPreamble    Preamble = 0x1000
)

// IT8951 Rotate Mode
const (
	Rotate0 Rotate = iota
	Rotate90
	Rotate180
	Rotate270
)

// PixelMode (Bit per Pixel value for IT8951)
const (
	BPP2 PixelMode = iota
	BPP3
	BPP4
	BPP8
)

// DisplayMode display mode
var (
	InitMode DisplayMode = 0 // INIT mode, for every init or some time after A2 mode refresh
	GC16Mode DisplayMode = 2 // GC16 mode, for every time to display 16 grayscale image
	A2Mode   DisplayMode = 4 // A2 mode, for fast refresh without flash (can be 6 for other displays)
)

// Endian Type
const (
	LoadImgLittleEndian EndianType = iota
	LoadImgBigEndian
)

// VCOMCommand get/set
const (
	GetVCOM VCOMCommand = iota
	SetVCOM
)

// Register Address Map
const (
	// Register Base Address

	DisplayRegBase Address = 0x1000 //Register RW access

	// Base Address of Basic LUT Registers

	LUT0EWHR  = DisplayRegBase + 0x00  //LUT0 Engine Width Height Reg
	LUT0XYR   = DisplayRegBase + 0x40  //LUT0 XY Reg
	LUT0BADDR = DisplayRegBase + 0x80  //LUT0 Base Address Reg
	LUT0MFN   = DisplayRegBase + 0xC0  //LUT0 Mode and Frame number Reg
	LUT01AF   = DisplayRegBase + 0x114 //LUT0 and LUT1 Active Flag Reg

	// Update Parameter Setting Register

	UP0SR     = DisplayRegBase + 0x134 //Update Parameter0 Setting Reg
	UP1SR     = DisplayRegBase + 0x138 //Update Parameter1 Setting Reg
	LUT0ABFRV = DisplayRegBase + 0x13C //LUT0 Alpha blend and Fill rectangle Value
	UPBBADDR  = DisplayRegBase + 0x17C //Update Buffer Base Address
	LUT0IMXY  = DisplayRegBase + 0x180 //LUT0 Image buffer X/Y offset Reg
	LUTAFSR   = DisplayRegBase + 0x224 //LUT Status Reg =status of All LUT Engines)
	BGVR      = DisplayRegBase + 0x250 //Bitmap =1bpp) image color table

	// System Registers

	SysRegBase Address = 0x0000

	// Address of System Registers

	I80CPCR = SysRegBase + 0x04

	// Memory Converter Registers

	McsrBase Address = 0x0200
	MCSR             = McsrBase + 0x0000
	LISAR            = McsrBase + 0x0008
)

var (
	epd bool
)

func init() {
	flag.BoolVar(&epd, "epd", false, "debug mode for EPD")
}

func Debug(format string, args ...interface{}) {
	if epd {
		log.Printf("[EPD] "+format, args...)
	}
}

// Init the EPD modules with desired VCOM value
func Init(vcom uint16) DevInfo {
	Open()
	Reset()
	SystemRun()
	devInfo := GetSystemInfo()
	lut := wordsToString(devInfo.LUTVersion)
	A2Mode = 6
	// special case for 6" e-ink Paper
	if lut == "M641" {
		A2Mode = 4
	}
	WriteRegister(I80CPCR, 0x0001) // packed mode
	waitReady()
	if vcom != ReadVCOM() {
		WriteVCOM(vcom)
		Debug("VCOM = -%.02fV\n", float32(ReadVCOM())/1000)
	}
	return devInfo
}

// Exit properly closes all peripherals used
func Exit() {
	Close()
}

func waitReady() {
	//Debug("...")
	for readyPin.Read() == rpio.Low {
		time.Sleep(time.Duration(10) * time.Microsecond)
	}
	//Debug("SPI Ready")
}

func writeUint16(word uint16) {
	//Debug("-> %04x", word)
	rpio.SpiTransmit(byte(word >> 8))
	rpio.SpiTransmit(byte(word & 0xff))
}

func readUint16() (word uint16) {
	data := rpio.SpiReceive(2)
	word = uint16(data[0])<<8 + uint16(data[1])
	//Debug("<- %04x", word)
	return
}

// WriteCommand writes a Command
func WriteCommand(command Command) {
	Debug("Writing command %04x", command)
	waitReady()
	csOn()
	SendPreamble(CommandPreamble)
	waitReady()
	writeUint16(uint16(command))
	csOff()

}

func SendPreamble(preamble Preamble) {
	writeUint16(uint16(preamble))
}

// WriteData writes a data word
func WriteData(data uint16) {
	//Debug("Writing data %04x", data)
	waitReady()
	csOn()
	SendPreamble(WritePreamble)
	waitReady()
	writeUint16(data)
	csOff()

}

// WriteBuffer writes a DataBuffer
func (buffer DataBuffer) WriteBuffer() {
	Debug("Writing buffer (size=%d)", len(buffer))
	waitReady()
	csOn()
	SendPreamble(WritePreamble)
	for _, d := range buffer {
		waitReady()
		writeUint16(d)
	}
	csOff()

}

// ReadData reads a data word
func ReadData() (data uint16) {
	waitReady()
	csOn()
	SendPreamble(ReadPreamble)
	waitReady()
	_ = readUint16() // read dummy word
	waitReady()
	data = readUint16()
	csOff()
	//Debug("Read data %04x", data)

	return
}

// ReadBuffer reads into a DataBuffer
func (buffer DataBuffer) ReadBuffer() {
	Debug("Reading buffer (%d)", len(buffer))
	waitReady()
	csOn()
	SendPreamble(ReadPreamble)
	waitReady()
	_ = readUint16() // dummy
	for i, _ := range buffer {
		waitReady()
		buffer[i] = readUint16()
	}
	csOff()
	Debug("Read buffer (size=%d)", len(buffer))

}

// WriteCommandBuffer write a command followed by a DataBuffer
func (buffer DataBuffer) WriteCommandBuffer(command Command) {
	Debug("Writing buffer (%d) to command %04x", len(buffer), command)
	WriteCommand(command)
	buffer.WriteBuffer()
}

// ReadRegister reads a register's value
func ReadRegister(address Address) (data uint16) {
	WriteCommand(TCONRegRd)
	WriteData(uint16(address))
	data = ReadData()
	Debug("Read register %04x = %04x", address, data)

	return
}

// WriteRegister sets a register's value
func WriteRegister(address Address, data uint16) {
	Debug("Writing %04x to register %04x", data, address)
	WriteCommand(TCONRegWr)
	writeUint16(uint16(address))
	WriteData(data)

}

// ReadVCOM reads current VCOM
func ReadVCOM() (data uint16) {
	WriteCommand(UserCmdVCOM)
	WriteData(uint16(GetVCOM)) //
	data = ReadData()
	Debug("Read VCOM = %d", data)

	return data
}

// WriteVCOM sets current VCOM
func WriteVCOM(data uint16) {
	Debug("Setting VCOM to %d", data)
	WriteCommand(UserCmdVCOM)
	WriteData(uint16(SetVCOM))
	WriteData(data)

}

// LoadImageStart starts an image transfer
func (imageInfo LoadImgInfo) LoadImageStart() {
	Debug("Starting image load")
	WriteCommand(TCONLdImg)
	WriteData(uint16(imageInfo.EndianType)<<8 | uint16(imageInfo.PixelFormat)<<4 | uint16(imageInfo.Rotate))
}

// LoadImageAreaStart starts an image area transfer
func (imageInfo LoadImgInfo) LoadImageAreaStart(imageArea AreaImgInfo) {
	Debug("Starting image area load")
	data := DataBuffer{
		uint16(imageInfo.EndianType)<<8 | uint16(imageInfo.PixelFormat)<<4 | uint16(imageInfo.Rotate),
		imageArea.X,
		imageArea.Y,
		imageArea.W,
		imageArea.H,
	}
	data.WriteCommandBuffer(TCONLdImgArea)
}

// LoadImageEnd ends an image or image area transfer
func LoadImageEnd() {
	WriteCommand(TCONLdImgEnd)
	Debug("Image loaded")
}

// getSystemInfo obtains device info
func GetSystemInfo() (devInfo DevInfo) {
	Debug("Getting EPD system devInfo")
	WriteCommand(UserCmdGetDevInfo)
	data := make(DataBuffer, (binary.Size(devInfo)+1)/2)
	data.ReadBuffer()
	devInfo.PanelW = data[0]
	devInfo.PanelH = data[1]
	devInfo.MemAddrL = data[2] // Low word is sent first!
	devInfo.MemAddrH = data[3] // High word second...
	for i, _ := range devInfo.FWVersion {
		devInfo.FWVersion[i] = data[4+i]
	}
	for i, _ := range devInfo.LUTVersion {
		devInfo.LUTVersion[i] = data[4+len(devInfo.FWVersion)+i]
	}
	Debug("DevInfo %v", devInfo)
	return devInfo
}

// SetTargetMemoryAddr sets address to transfer to
func SetTargetMemoryAddr(targetAddress uint32) {
	Debug("Set target mem address %x", targetAddress)
	WriteRegister(LISAR+2, uint16(targetAddress>>16))
	WriteRegister(LISAR, uint16(targetAddress&0x0000ffff))
	targetConfirm := uint32(ReadRegister(LISAR+2))<<16 + uint32(ReadRegister(LISAR))
	Debug("Target confirmation = %x", targetConfirm)
}

// WaitForDisplayReady waits for display
func WaitForDisplayReady() {
	Debug("Wait for Display")
	//Check IT8951 Register LUTAFSR => NonZero Busy, Zero - Free
	for ReadRegister(LUTAFSR) != 0 {
		time.Sleep(time.Duration(100) * time.Microsecond)
	}
}

// HostAreaPackedPixelWrite writes an image area
func (imageInfo LoadImgInfo) HostAreaPackedPixelWrite(imageAreaInfo AreaImgInfo, bpp int, packedWrite bool) {
	Debug("HostAreaPackedPixelWrite")
	dataBuffer := imageInfo.SourceBufferAddr
	SetTargetMemoryAddr(imageInfo.TargetMemAddr)
	imageInfo.LoadImageAreaStart(imageAreaInfo)

	// send data
	if true || packedWrite {
		dataBuffer.WriteBuffer()
	} else {
		var ww int // buffer width in words
		switch bpp {
		case 1: // stored as 1 pixel per byte -> W bytes -> W/2 words
			ww = int(imageAreaInfo.W / 2)
			break
		case 2: // stored as 4 pixels per byte -> W/4 bytes -> W/8 words
			ww = int(imageAreaInfo.W / 8)
			break
		case 3: // stored as 3 pixels per byte -> W/2 bytes -> W/4 words
		case 4: // stored as 2 pixels per byte -> W/2 bytes -> W/4 words
			ww = int(imageAreaInfo.W / 4)
			break
		case 8: // stored as 8 pixels per byte -> W bytes -> W/2 words
		default:
			ww = int(imageAreaInfo.W / 2)
			break
		}

		wh := int(imageAreaInfo.H) // buffer height in pixels
		Debug("Slow write %d bytes", ww*wh)
		for h := 0; h < wh; h++ {
			// write one word at a time
			for w := 0; w < ww; w++ {
				WriteData(dataBuffer[h*ww+w])
			}
		}
	}
	LoadImageEnd()
}

// DisplayArea display current area
func DisplayArea(x, y, w, h uint16, mode DisplayMode) {
	Debug("Display Area")
	data := DataBuffer{
		x, y, w, h, uint16(mode),
	}
	data.WriteCommandBuffer(UserCmdDpyArea)
}

// DisplayAreaBuffer displays target address area
func DisplayAreaBuffer(x, y, w, h uint16, mode DisplayMode, targetAddress uint32) {
	Debug("Display Area Buffer")
	data := DataBuffer{
		x, y, w, h, uint16(mode), uint16(targetAddress & 0xffff), uint16(targetAddress >> 16),
	}
	data.WriteCommandBuffer(UserCmdDpyBufArea)
}

// Display1bpp display in monochrome (1bpp mode)
func Display1bpp(x, y, w, h uint16, mode DisplayMode, targetAddress uint32, backGreyValue uint8, frontGreyValue uint8) {
	//Set Display mode to 1 bpp mode - Set 0x18001138 Bit[18](0x1800113A Bit[2])to 1
	Debug("Display 1bpp")
	WriteRegister(UP1SR+2, ReadRegister(UP1SR+2)|uint16(1<<2))

	WriteRegister(BGVR, uint16(frontGreyValue)<<8|uint16(backGreyValue))

	if targetAddress == 0 {
		DisplayArea(x, y, w, h, mode)
	} else {
		DisplayAreaBuffer(x, y, w, h, mode, targetAddress)
	}
	WaitForDisplayReady()
	WriteRegister(UP1SR+2, ReadRegister(UP1SR+2) & ^uint16(1<<2))
}

// EnhanceDrivingCapability can improve display if it appears blurred
func EnhanceDrivingCapability() {
	Debug("Enhancing display capability")
	value := ReadRegister(0x0038)
	Debug("The reg value before writing is %x", value)

	WriteRegister(0x0038, 0x0602)

	value = ReadRegister(0x0038)
	Debug("The reg value after writing is %x", value)
}

// SystemRun switches to RUN mode
func SystemRun() {
	Debug("System Run mode")
	WriteCommand(TCONSysRun)
}

// Sleep switches to SLEEP mode
func Sleep() {
	Debug("Sleep mode")
	WriteCommand(TCONSleep)
}

// StandBy switches to STANDBY mode
func StandBy() {
	Debug("StandBy mode")
	WriteCommand(TCONStandby)
}

func (devInfo DevInfo) ClearRefresh(targetAddress uint32, mode DisplayMode) {
	Debug("Refreshing screen (t=%0x)", targetAddress)
	var imageSize int               // image size in words
	if (devInfo.PanelW*4)%16 == 0 { // exactly 16 bits
		imageSize = int(devInfo.PanelW*4/16) * int(devInfo.PanelH)
	} else {
		imageSize = int(devInfo.PanelW*4/16+1) * int(devInfo.PanelH)
	}
	Debug("image size: %d (%x)", imageSize, imageSize)
	var frameBuffer = make(DataBuffer, imageSize)
	Debug("Init area")
	for i := 0; i < int(imageSize); i++ {
		frameBuffer[i] = 0xffff
	}
	Debug("End init area")
	imageInfo := LoadImgInfo{
		SourceBufferAddr: DataBuffer(frameBuffer),
		EndianType:       LoadImgLittleEndian,
		PixelFormat:      BPP4,
		Rotate:           Rotate0,
		TargetMemAddr:    targetAddress,
	}
	areaInfo := AreaImgInfo{
		X: 0,
		Y: 0,
		W: devInfo.PanelW,
		H: devInfo.PanelH,
	}
	WaitForDisplayReady()

	imageInfo.HostAreaPackedPixelWrite(areaInfo, 4, true)
	DisplayArea(0, 0, devInfo.PanelW, devInfo.PanelH, mode)
}

func Refresh1bpp(buffer DataBuffer, X, Y, W, H uint16, mode DisplayMode, targetAddress uint32, packedWrite bool) {
	Debug("Refresh1bpp")
	WaitForDisplayReady()
	Write1bpp(buffer, X, Y, W, H, targetAddress, packedWrite)
	Display1bpp(X, Y, W, H, mode, targetAddress, 0xF0, 0x00)
}

func Write1bpp(buffer DataBuffer, X, Y, W, H uint16, targetAddress uint32, packedWrite bool) {
	Debug("Write1bpp")
	WaitForDisplayReady()

	imageInfo := LoadImgInfo{
		SourceBufferAddr: buffer,
		EndianType:       LoadImgLittleEndian,
		PixelFormat:      BPP8, //Use 8bpp to set 1bpp
		Rotate:           Rotate0,
		TargetMemAddr:    targetAddress,
	}
	areaInfo := AreaImgInfo{
		X: X,
		Y: Y,
		W: W,
		H: H,
	}
	imageInfo.HostAreaPackedPixelWrite(areaInfo, 1, packedWrite)
}

func MultiFrameRefresh1bpp(X, Y, W, H uint16, targetAddress uint32) {
	Debug("MultiFrameRefresh1bpp")
	WaitForDisplayReady()
	Display1bpp(X, Y, W, H, A2Mode, targetAddress, 0xF0, 0x00)
}

func Refresh2bpp(buffer DataBuffer, X, Y, W, H uint16, hold bool, targetAddress uint32, packedWrite bool) {
	Debug("Refresh2bpp")
	WaitForDisplayReady()

	imageInfo := LoadImgInfo{
		SourceBufferAddr: buffer,
		EndianType:       LoadImgLittleEndian,
		PixelFormat:      BPP2,
		Rotate:           Rotate0,
		TargetMemAddr:    targetAddress,
	}
	areaInfo := AreaImgInfo{
		X: X,
		Y: Y,
		W: W,
		H: H,
	}
	imageInfo.HostAreaPackedPixelWrite(areaInfo, 2, packedWrite)
	if hold {
		DisplayArea(X, Y, W, H, GC16Mode)
	} else {
		DisplayAreaBuffer(X, Y, W, H, GC16Mode, targetAddress)
	}
}

func Refresh4bpp(buffer DataBuffer, X, Y, W, H uint16, hold bool, targetAddress uint32, packedWrite bool) {
	Debug("Refresh4bpp")
	WaitForDisplayReady()

	imageInfo := LoadImgInfo{
		SourceBufferAddr: buffer,
		EndianType:       LoadImgLittleEndian,
		PixelFormat:      BPP4,
		Rotate:           Rotate0,
		TargetMemAddr:    targetAddress,
	}
	areaInfo := AreaImgInfo{
		X: X,
		Y: Y,
		W: W,
		H: H,
	}
	imageInfo.HostAreaPackedPixelWrite(areaInfo, 4, packedWrite)

	if hold {
		DisplayArea(X, Y, W, H, GC16Mode)
	} else {
		DisplayAreaBuffer(X, Y, W, H, GC16Mode, targetAddress)
	}
}

func Refresh8bpp(buffer DataBuffer, X, Y, W, H uint16, hold bool, targetAddress uint32) {
	Debug("Refresh8bpp")
	WaitForDisplayReady()

	imageInfo := LoadImgInfo{
		SourceBufferAddr: buffer,
		EndianType:       LoadImgLittleEndian,
		PixelFormat:      BPP8,
		Rotate:           Rotate0,
		TargetMemAddr:    targetAddress,
	}
	areaInfo := AreaImgInfo{
		X: X,
		Y: Y,
		W: W,
		H: H,
	}
	imageInfo.HostAreaPackedPixelWrite(areaInfo, 8, false)

	if hold {
		DisplayArea(X, Y, W, H, GC16Mode)
	} else {
		DisplayAreaBuffer(X, Y, W, H, GC16Mode, targetAddress)
	}
}

// --- helpers
func wordsToString(data [8]uint16) (result string) {
	var buffer bytes.Buffer
	for _, word := range data {
		buffer.WriteByte(byte(word & 0xff))
		buffer.WriteByte(byte(word >> 8))
	}
	result = buffer.String()
	return result
}

func (devInfo DevInfo) String() string {
	return fmt.Sprintf("System Info\n"+
		"Panel Width  : %d\n"+
		"Panel Height : %d\n"+
		"Mem Addr     : %x\n"+
		"FW Version   : %s\n"+
		"LUT Version  : %s\n",
		devInfo.PanelW,
		devInfo.PanelH,
		uint32(devInfo.MemAddrH)<<16+uint32(devInfo.MemAddrL),
		wordsToString(devInfo.FWVersion),
		wordsToString(devInfo.LUTVersion))
}

func (buffer DataBuffer) String() (result string) {
	line := ""
	cnt := 0
	for i, d := range buffer {
		if i%8 == 0 {
			cnt = i
		}
		if i%8 == 7 {
			result += fmt.Sprintf("%08x:%s %04x\n", cnt, line, d)
			line = ""
		} else {
			line += fmt.Sprintf(" %04x", d)
		}
	}
	if len(line) > 0 {
		result += fmt.Sprintf("%08x:%s\n", cnt, line)
	}
	return result
}

// TargetAddress gets the uint32 target address from system info
func (devInfo DevInfo) TargetAddress() uint32 {
	return uint32(devInfo.MemAddrH)<<16 + uint32(devInfo.MemAddrL)
}

// GetWidthInWords calculates the number of words for a certain width and resolution
func GetWidthInWords(width int, bpp int) int {
	if (width*bpp)%16 == 0 {
		return width * bpp / 16
	}
	return (width*bpp + 1) / 16
}
