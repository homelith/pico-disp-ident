//------------------------------------------------------------------------------
// png2array.go
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// MIT License
//
// Copyright (c) 2022 homelith
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//------------------------------------------------------------------------------

package main

import (
	"encoding/binary"
	"flag"
	"fmt"
	"image"
	"image/png"
	"log"
	"os"
	"regexp"
)

func main() {
	var err error

	// support png only
	image.RegisterFormat("png", "png", png.Decode, png.DecodeConfig)

	// parse args and extract mode string
	var enc_mode string
	var fmt_mode string
	flag.StringVar(&enc_mode, "m", "rgb565", "encoding mode (e.g. rgb565)")
	flag.StringVar(&fmt_mode, "f", "txt", "output format (e.g. txt or bin)")
	flag.Parse()
	if len(flag.Args()) < 1 {
		log.Printf("usage : ./png2array [-m {encoding mode (rgb565)}] [-f {output format (txt or bin)}] {input image file}\n")
		os.Exit(1)
	}

	switch enc_mode {
	case "rgb565":
		{
			log.Printf("encoding mode '%s' specified\n", enc_mode)
		}
	default:
		{
			log.Printf("unrecognized encoding mode '%s', exiting..\n", enc_mode)
			os.Exit(1)
		}
	}
	switch fmt_mode {
	case "txt":
		{
			log.Printf("format mode '%s' specified\n", fmt_mode)
		}
	case "bin":
		{
			log.Printf("format mode '%s' specified\n", fmt_mode)
		}
	default:
		{
			log.Printf("unrecognized format mode '%s', exiting..\n", fmt_mode)
			os.Exit(1)
		}
	}

	// open input image
	var file_in *os.File
	file_in, err = os.Open(flag.Arg(0))
	if err != nil {
		log.Printf("failed to open '%s'\n", flag.Arg(0))
		os.Exit(1)
	}
	defer file_in.Close()

	// decode png
	var img image.Image
	img, _, err = image.Decode(file_in)
	if err != nil {
		log.Printf("failed to decode '%s'", flag.Arg(0))
		os.Exit(1)
	}

	// write header
	var r *regexp.Regexp
	r = regexp.MustCompile(`\..*$`)

	if fmt_mode == "txt" {
		fmt.Printf("unsigned char __attribute__((aligned(4))) %s[] = {\n", r.ReplaceAllString(flag.Arg(0), ""))
	}

	// scan pixels and convert
	var width int
	var height int
	width = img.Bounds().Max.X
	height = img.Bounds().Max.Y

	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			var r uint32
			var g uint32
			var b uint32
			var rgb565 uint16
			r, g, b, _ = img.At(x, y).RGBA()
			r = uint32((r / 257) >> 3)
			g = uint32((g / 257) >> 2)
			b = uint32((b / 257) >> 3)
			rgb565 = uint16(r<<11 + g<<5 + b)

			if fmt_mode == "txt" {
				if y == height-1 && x == width-1 {
					fmt.Printf("0x%02x, 0x%02x \n};", (rgb565 & 0x00FF), (rgb565 >> 8))
				} else {
					fmt.Printf("0x%02x, 0x%02x, ", (rgb565 & 0x00FF), (rgb565 >> 8))
				}
			} else {
				binary.Write(os.Stdout, binary.LittleEndian, rgb565)
			}
		}
		if fmt_mode == "txt" {
			fmt.Printf("\n")
		}
	}
	if fmt_mode == "txt" {
		fmt.Printf("unsigned int %s_rgb565_bin_len = %d;\n", r.ReplaceAllString(flag.Arg(0), ""), width*height*2)
	}
}
