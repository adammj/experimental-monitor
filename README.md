# Experimental Monitor

---

Repository of the current version (as of 2018) of the code necessary to run on the Experimental Monitor device I built. It also includes the code used to communicate with it on MATLAB (although any software that communicate through the serial port (over USB) will work).

The device is meant to be a real-time monitor for behavioral experiments (although it could easily be modified to do other things). The code can run on either TI's LAUNCHXL-F28379D (dual core) or LAUNCHXL-F28377S (single core). To make development quicker, I made daughterboards to plug into the LaunchPads, and extend their functionality. 

The device, when connected to a USB port, will emulate a COM port. Although the command and response structure is pure JSON (and language-agnostic), I wrote a MATLAB toolbox for communicating with it and setting up the experiments.

Functionality:

- 5V-tolerant digital IO
- 5V analog outputs
- 0-5V or +/-10.24V analog input
- High-speed output to either a Plexon or Blackrock DSP (for annotating experiment)


---

The full git history is not included, because it seemed like more effort than it was worth to sanitize my personal email across all of the branches as well rebasing to include the license from the beginning.

---

Copyright (C) 2024  Adam M. Jones

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
