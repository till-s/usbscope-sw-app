/**LB-MIT
 *
 * MIT License
 *
 * Copyright (c) 2026 Till Straumann
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **LE-MIT*/

#include <FWComm.hpp>
#include <FileMap.hpp>
#include <Flash.hpp>
#include <getopt.h>

#include <sys/mman.h>

#include <fwUtil.h>

#include <stdio.h>

class Prog : public FlashWriterProgress {
    bool verbose_;
public:
	Prog(bool verbose = false) : verbose_(verbose)
	{
	}

	virtual int advance(const FlashWriterState *state) override {
		if ( verbose_ ) {
		char c;
			switch (state->operation) {
				case Operation::ERASE:          c = 'E'; break;
				case Operation::VERIFY_ERASED:  c = 'Z'; break;
				case Operation::VERIFY_WRITTEN: c = 'V'; break;
				case Operation::WRITE         : c = 'W'; break;
			}
			printf("%c[%3u]: %8u, %8u, %8u\n", c, state->index, state->address, state->size, state->completed);
		} else {
		const char *s;
			switch (state->operation) {
				case Operation::ERASE:          s = "Erasure      "; break;
				case Operation::VERIFY_ERASED:  s = "Erasure Check"; break;
				case Operation::VERIFY_WRITTEN: s = "Verifying    ";; break;
				case Operation::WRITE         : s = "Writing      "; break;
			}
			if ( 0 == state->index && Operation::ERASE != state->operation ) {
				printf("\n");
			}
			printf("\r[%s] %3.0f%%", s, 100.0*(double)state->completed/(double)state->size);
			fflush(stdout);
		}
		return 0;
	}

	~Prog()
	{
		if ( ! verbose_ ) {
			printf("\n");
		}
	}
};

static void
usage(const char *nm)
{
	printf("usage: %s [-f firmware.hex.bin] [-d acmDevice]\n", nm);
	printf("read or program FPGA flash memory\n");
	printf("  -f <firmware>.hex.bin : write firmware file to flash.\n");
	printf("                          If no '-f' given then read the\n");
	printf("                          first 100 bytes from flash and\n");
	printf("                          hexdump to stdout\n");
	printf("  -d <device_file>        path to device (defaults to /dev/ttyACM0).\n");
}

int
main(int argc, char **argv)
{
	int opt;
	const char *fnam = nullptr;
	const char *dnam = "/dev/ttyACM0";
	while ( (opt = getopt(argc, argv, "f:d:h")) > 0 ) {
		switch ( opt ) {
			case 'h': usage(argv[0]); return 0;
			case 'f': fnam = optarg; break;
			case 'd': dnam = optarg; break;
			default: return 1;
		}
	}
	FWPtr fw = FWComm::create( dnam );
	Flash flash( fw );
	if ( ! fnam ) {
		uint8_t buf[100];
		ssize_t got = flash.read(buf, sizeof(buf));
		ssize_t i;
		printf("Reading first 100 bytes from flash:\n");
		for ( i = 0; i < got; ++i ) {
			printf("0x%02x%c", buf[i], ((i & 0xf) == 0xf ? '\n' : ' '));
		}
		if ( (i & 0xf) != 0 ) {
			printf("\n");
		}
	} else {
		{
		FileReadMap map( fnam );
		Prog progress;
		flash.write(map.getMap(), map.getSize(), &progress);
		}
		if ( fw_reconfigure_fpga_on_close( fw->fw_, FW_RECONFIGURE_FPGA ) < 0 ) {
			printf("Firmware does not support recongration of FPGA;\n");
			printf("Please trigger reconfiguration manually.\n");
		} else {
			fw.reset();
			printf("FPGA reconfigured.\n");
		}
	}
}
