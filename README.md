## NES260 - NES emulator for Xilinx KV260 FPGA board

This is a port of [fpganes](https://github.com/strigeus/fpganes) to the KV260 FPGA board.

<img src="doc/nes260_setup.jpg" width="300">

Current status:
* Most of the small games works (<64K), like Super Mario Bros, Adventure Island, Gradius, Battle City.
* Video works well through HDMI.
* Audio is done through a separate PMod module (IceSugar Audio).
* Game loading and controller is handled through a python GUI on PC.

### Running NES260

Download the [binary release zip file](https://github.com/zf3/nes260/releases/tag/v1.0). Inside there's a BOOT.bin image for booting the KV260 board, and nes260.py for running on Windows.

KV260's boot process is different from most FPGA boards. It always boots from on-board QSPI flash and there's no switches/jumpers for other ways. So easiest way to boot BOOT.bin is to program it to QSPI flash. Follow [Xilinx's instructions](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/1641152513/Kria+K26+SOM#Stand-alone-FW-Update-&-Recovery-Utility) to do it. (You will need an Ethernet cable). If you have Xilinx's Vitis/Vivado development tools installed, you can actually switch to [SD bootmode](https://xilinx.github.io/kria-apps-docs/creating_applications/1.0/build/html/docs/creating_applications_bootmodes.html). The MicroSD card preparation is as simple as formating it as FAT and copy the BOOT.BIN to it.

If you see a grey screen after boot, then NES260 is ready for loading .nes ROMs. Connect KV260 to PC with USB cable and run `pc/nes26.py` with python to load games (`pip install pyserial itertools inputs kaitaistruct importlib`, then `python nes26.py`). USB game controller should be connected to the PC (**not** KV260). My Xbox 360 controllers work fine.

To get audio, connect a [IceSugar Audio 1.2](https://www.aliexpress.com/item/1005001505255692.html) module to the PMOD port. It provides both a small speaker and a 3.5mm jack.

### Interested in retro-gaming or learning FPGA programming?

You can,

* Read my notes on some [technical details](doc/design.md) on how NES260 works, and compile/build the project yourself. The required Xilinx tools are free.
* Get the KV260 board. KV260 is currently available from Xilinx at [\$199](https://www.xilinx.com/products/som/kria/kv260-vision-starter-kit.html). It is not cheap. But it is a powerful board with basically a Xilinx Ultrascale+ ZU5EV SoC. Similar boards sell for twice the price or more. You can learn a lot with it if you have programming experience.
* Check out the most mature FPGA gaming project [Mister](https://misterfpga.org/). You can find many Youtube [videos](https://www.youtube.com/watch?v=rhT6YYRH1EI&t=8762s) on FPGA gaming. Mister supports a ton of consoles from Atari all the way to Playstation 1.
* Why make NES260 when there's already Mister? Actually Xilinx KV260 is much more powerful than the board Mister uses (Intel DE10-nano), and at roughly the same price. So it has potential for FPGA retro-gaming. The board was just released in 2021 and I haven't seen an emulator built with it. So here it goes.
* Also check out another FPGA project of mine, [neoapple2](https://github.com/zf3/neoapple2).

### Video demo

[![NES260 demo](https://img.youtube.com/vi/p09k8FfFO0Q/0.jpg)](https://www.youtube.com/watch?v=p09k8FfFO0Q)

Feng Zhou, 2022-7
