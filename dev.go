package it8951

import (
	"log"
	"periph.io/x/conn/v3"
	"periph.io/x/conn/v3/gpio"
	"periph.io/x/conn/v3/gpio/gpioreg"
	"periph.io/x/conn/v3/physic"
	"periph.io/x/conn/v3/spi"
	"periph.io/x/conn/v3/spi/spireg"
	"time"
)

//

const (
	// orange pi zero 2W
	//EpdRstPin  = "GPIO226" //"GPIO17" //11 // Raspberry Pi Pin 17
	//EpdCsPin   = "GPIO229" //"GPIO8"  //24 // Raspberry Pi Pin 8
	//EpdBusyPin = "GPIO228" //"GPIO24" //18 // Raspberry Pi Pin 24
	// raspberry pi 4B
	EpdRstPin  = "GPIO17" //11 // Raspberry Pi Pin 17
	EpdCsPin   = "GPIO8"  //24 // Raspberry Pi Pin 8
	EpdBusyPin = "GPIO24" //18 // Raspberry Pi Pin 24
)

var (
	chipSelect uint8 = 0
	SpiPort    spi.PortCloser
	Conn       spi.Conn
	speed      physic.Frequency = 24 * physic.MegaHertz
	rstPin     gpio.PinOut
	csPin      gpio.PinOut
	readyPin   gpio.PinIn
)

// Open sets the I/O ports and SPI
func Open(spiDev string) (err error) {
	Debug("Init start (SPI port = %s)", spiDev)

	if SpiPort, err = spireg.Open(spiDev); err != nil {
		log.Fatalln("SPI Port Open Failed:", err)
	}

	if err = SpiPort.LimitSpeed(24 * physic.MegaHertz); err != nil {
		Debug("Can't limit speed on SPI port:", err)
	}

	//
	// init SPI
	//

	Debug("Initializing SPI")

	if Conn, err = SpiPort.Connect(speed, spi.Mode0|spi.NoCS, 8); err != nil {
		log.Fatalln("SPI Setup Error:", err)
	}

	Debug("SPI Limit size = %d", conn.Limits.MaxTxSize)

	//
	// init pins
	//

	Debug("Initializing GPIO pins")

	rstPin = gpioreg.ByName(EpdRstPin)
	csPin = gpioreg.ByName(EpdCsPin)
	readyPin = gpioreg.ByName(EpdBusyPin)

	if err = rstPin.Out(gpio.High); err != nil {
		// rstpin error
	}
	if err = csPin.Out(gpio.High); err != nil {
		// cspin error
	}
	if err = readyPin.In(gpio.PullNoChange, gpio.NoEdge); err != nil {
		// readypin error
	}

	Debug("EPD initialization complete")
	return nil
}

// Close ends SPI usage and restores pins
func Close() error {
	Debug("Shutting down EPD")
	_ = csPin.Out(gpio.Low)
	_ = rstPin.Out(gpio.Low)

	if err := SpiPort.Close(); err != nil {
		Debug("Error closing SPI port: %s", err)
		return err
	}
	return nil
}

// csOn selects slave
func csOn() {
	//Debug("CS On")
	_ = csPin.Out(gpio.Low)
}

// csOff deselects slave
func csOff() {
	//Debug("CS Off")
	_ = csPin.Out(gpio.High)
}

// Reset resets a slave
func Reset() {
	Debug("EPD Reset")
	_ = rstPin.Out(gpio.High)
	time.Sleep(time.Duration(200) * time.Millisecond)
	_ = rstPin.Out(gpio.Low)
	time.Sleep(time.Duration(10) * time.Millisecond)
	_ = rstPin.Out(gpio.High)
	time.Sleep(time.Duration(200) * time.Millisecond)
}
