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
#include <Flash.hpp>
#include <getopt.h>

#include <sys/mman.h>

#include <fwUtil.h>

#include <stdio.h>

class Prog : public FlashWriterProgress {
	virtual int advance(const FlashWriterState *state) override {
		char c;
		switch (state->operation) {
			case Operation::ERASE:          c = 'E'; break;
			case Operation::VERIFY_ERASED:  c = 'Z'; break;
			case Operation::VERIFY_WRITTEN: c = 'V'; break;
			case Operation::WRITE         : c = 'W'; break;
		}
		printf("%c[%3u]: %8u, %8u, %8u\n", c, state->index, state->address, state->size, state->completed);
		return 0;
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
		ssize_t got = flash.read(0, buf, sizeof(buf));
		printf("Reading first 100 bytes from flash:\n");
		for ( ssize_t i = 0; i < got; ++i ) {
			printf("0x%02x\n", buf[i]);
		}
	} else {
		uint8_t *data = (uint8_t*)MAP_FAILED;
		off_t    sz;
		if ( fileMap(fnam,  &data, &sz, 0, 1) < 0 ) {
			fprintf(stderr, "fileMap failed");
			return 1;
		}
		Prog progress;
		printf("data %p, size %zu\n", data, sz);

		Flash::WriteEnable ena(&flash);
		flash.erase(0, sz, &progress);
		flash.write(0, data, sz, &progress);
	}
}
