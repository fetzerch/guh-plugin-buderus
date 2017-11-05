# guh-plugin-buderus

Plugin for the [guh](https://github.com/guh/guh) IoT platform that
allows to connect to Buderus heating systems (via Buderus KM200 Web Gateway).

## Command Line Client

The project contains a command line client `scripts/buderus-km200-cli.py`
that can either query an actual gateway or act as a web server and emulate a
gateway. This is useful to test the plugin without connecting to real hardware.

Query mode:

    ./buderus-km200-cli.py km200 <KEY> gw.json

Emulation mode:

    ./buderus-km200-cli.py --mode emulate localhost:5000 <KEY> gw.json

## License

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
[GNU General Public License](http://www.gnu.org/licenses/gpl-2.0.html>)
for more details.
