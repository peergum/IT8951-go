package it8951

import (
	"github.com/peergum/go-rpio/v4"
	"log"
	"time"
)

//

const (
	EpdRstPin  = 17 //11 // Raspberry Pi Pin 17
	EpdCsPin   = 8  //24 // Raspberry Pi Pin 8
	EpdBusyPin = 24 //18 // Raspberry Pi Pin 24
)

var (
	rstPin   rpio.Pin
	csPin    rpio.Pin
	readyPin rpio.Pin
)

// Open sets the I/O ports and SPI
func Open() (err error) {
	Debug("Init start")

	if err := rpio.Open(); err != nil {
		log.Fatalln("RPIO Open Error:", err)
	}

	//
	// init SPI
	//

	Debug("Initializing SPI")

	if err := rpio.SpiBegin(rpio.Spi0); err != nil {
		log.Fatalln("SpiBegin Error:", err)
	}

	rpio.SpiChipSelect(0)
	rpio.SpiSpeed(12000000) // 12Mhz
	rpio.SpiMode(0, 0)

	//
	// init pins
	//

	Debug("Initializing GPIO pins")

	rstPin = rpio.Pin(EpdRstPin)
	csPin = rpio.Pin(EpdCsPin)
	readyPin = rpio.Pin(EpdBusyPin)

	rstPin.Output()
	csPin.Output()
	readyPin.Input()

	csOff()

	Debug("EPD initialization complete")
	return nil
}

// Close ends SPI usage and restores pins
func Close() {
	Debug("Shutting down EPD")
	csPin.Low()
	rstPin.Low()

	rpio.SpiEnd(rpio.Spi0)

	rpio.Close()
}

// csOn selects slave
func csOn() {
	//Debug("CS On")
	csPin.Low()
}

// csOff deselects slave
func csOff() {
	//Debug("CS Off")
	csPin.High()
}

// Reset resets a slave
func Reset() {
	Debug("EPD Reset")
	rstPin.High()
	time.Sleep(time.Duration(200) * time.Millisecond)
	rstPin.Low()
	time.Sleep(time.Duration(10) * time.Millisecond)
	rstPin.High()
	time.Sleep(time.Duration(200) * time.Millisecond)
}
