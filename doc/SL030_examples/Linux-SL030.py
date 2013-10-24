# Example code for SL030 card reader
#
# The MIT License
#
# Copyright (C) 2013 Gábor Molnár <gabor@molnar.es>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# 'Software'), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import fcntl
import io
import time

# Parameters
bus_no = 1
sl030_address = 0x50

# The protocol is not compatible with SMBus so we can't use the python-smb module.
# Instead, we use the standard /dev interface exposed by the i2c-dev kernel module.
# Documentation: https://www.kernel.org/doc/Documentation/i2c/dev-interface
#
# Standard file IO is not low level enough: we can't control the size of the buffer size when
# issuing a read syscall, and the i2c dev interface does not work with large read buffers.
# The io module let's us specify the exact buffer size, so we use that.
bus = io.FileIO('/dev/i2c-' + bus_no, 'r+')

# Specifying the address of the I2C slave with the I2C_SLAVE ioctl.
# 0x0703 == I2C_SLAVE from i2c-tools-3.1.0/include/linux/i2c-dev.h
error = fcntl.ioctl(bus, 0x0703, sl030_address)
print error

# Writing the 'Select Mifare card' command
bus.write('\x01\x01')

# Let the device complete the read
time.sleep(0.1)

# Read out the response
response = bus.read(128)
print response
