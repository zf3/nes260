#!/usr/bin/python3
from tkinter import *
from tkinter import messagebox
from tkinter import font
from tkinter.filedialog import askopenfilename

import sys, os, threading, queue, time, webbrowser
import itertools

from ines import Ines

# pyserial
import serial
import serial.tools.list_ports
# XInput-Python
# import XInput as xi
# inputs
import inputs
from importlib import reload

ports = serial.tools.list_ports.comports()
devices=[]
for port, desc, hwid in sorted(ports):
    print("{}: {} [{}]".format(port, desc, hwid))
    devices.append(port)

if len(devices) == 0:
    msg="Cannot find any serial port. Connnect USB cable and try again?"
    print(msg)
    messagebox.showerror("Error", msg)
    exit(1)

top = Tk()
device=StringVar(top)
device.set(devices[0])
ser=None

def about():
    messagebox.showinfo("About",
    """
    NES260 - NES emulator for KV260 FPGA board
    
    (c) Feng Zhou, 2022.7
    """)

def site():
    webbrowser.open("https://github.com/zf3")

# Hacky way to support plugging-in of controller when program is running
def refreshController():
    reload(inputs)

def initUi():
    top.title("NES260")
    top.geometry("400x250")

    title=Label(top, text='NES260', fg="#555")
    title.configure(font=("Arial", 22, "bold"))
    title.place(x=150, y=20)

    menu = Menu(top)
    top.config(menu=menu)
    fileMenu = Menu(menu)
    fileMenu.add_command(label="Refresh controllers", command=refreshController)
    helpMenu = Menu(menu)
    helpMenu.add_command(label="Project site", command=site)
    helpMenu.add_command(label="About", command=about)
    menu.add_cascade(label="File", menu=fileMenu)
    menu.add_cascade(label="Help", menu=helpMenu)

    btnLoad = Button(top, text = "Load .nes", command=chooseInes)
    btnLoad.place(x=50, y=90)

    global desc1, desc2, desc3, desc4
    desc1=Label(top, text='No nes file open')
    desc2=Label(top, text='Size: ')
    desc3=Label(top, text='Mapper: ')
    desc4=Label(top, text='PRG:, CHR:')

    desc1.place(x=160, y=70)
    desc2.place(x=160, y=90)
    desc3.place(x=160, y=110)
    desc4.place(x=160, y=130)

    # Serial port selection
    labelSerial=Label(top, text="Serial")
    labelSerial.place(x=50, y=170)
    global labelSerialStatus
    labelSerialStatus=Label(top, text="")
    labelSerialStatus.place(x=110, y=170)
    global device
    optionSerial=OptionMenu(top, device, *devices, command=serialSelected)
    optionSerial.place(x=50, y=190)

    # Controller 1
    labelCtr1=Label(top, text="Controller 1")
    labelCtr1.place(x=160, y=170)
    global labelCtr1Status
    labelCtr1Status=Label(top, text="Disconnected", fg="#888")
    labelCtr1Status.place(x=160, y=195)

    # Controller 2
    labelCtr2=Label(top, text="Controller 2")
    labelCtr2.place(x=270, y=170)
    global labelCtr2Status
    labelCtr2Status=Label(top, text="Disconnected", fg="#888")
    labelCtr2Status.place(x=270, y=195)

def connectSerial():
    global ser
    if ser==None:
        ser=serial.Serial(device.get(), 230400, write_timeout=0)

def serialSelected(choice):
    print("Serial: {}".format(choice))
    global ser
    if ser != None:
        ser.close()
        ser=None
    connectSerial()

def showInesInfo(filename):
    size=os.stat(filename).st_size
    desc1.config(text=os.path.basename(filename))
    desc2.config(text="Size: {}".format(size))

    # parse file using kaitaistruct
    try:
        data=Ines.from_file(filename)
        header=data.header
        desc3.config(text="Mapper: {}".format(header.mapper))
        desc4.config(text="PRG: {}KB, CHR: {}KB".format(16*header.len_prg_rom, 8*header.len_chr_rom))
    except Exception:   # parse error, just ignore
        desc3.config(text="Mapper: ")
        desc4.config(text="PRG:, CHR:")

def chooseInes():
    filename=askopenfilename(title='Choose a .nes file', filetypes=(("nes files", "*.nes"),("all files","*.*")))
    showInesInfo(filename)
    sendInes(filename)

PROGRESS=['/','-','\\','|']

def sendInes(fname):
    if not os.path.isfile(fname):
        print("Cannot open file: {}".format(fname))
        exit(1)

    size=os.stat(fname).st_size
    f=open(fname, 'rb')
    data=bytearray(f.read())
    f.close()

    # send data over serial line
    # 115200,8,N,1
    header=bytearray([1])       # command: ines
    header += size.to_bytes(4, 'little')    # we send little-endian
    connectSerial()
    ser.write(header)
    CHUNK=1024
    for i in range(0,len(data),CHUNK):
        labelSerialStatus.config(text=PROGRESS[(i//CHUNK)%4], fg='#000')  # show an animation for progress
        top.update()
        ser.write(data[i:min(i+CHUNK,len(data))])

    print("Sent {} bytes over serial line.".format(len(data)))

line=''
def dumpSerial():
    global ser, line
    while True:
        try:
            if ser != None and ser.inWaiting():
                din = ser.read(1)
                s = din.decode("iso-8859-1")
                print(s, end='')
                if s == '\n':
                    line=''
                else:
                    line += s
                if 'FPGA' in line:  # PS returns this when ines is sent to PL
                    labelSerialStatus.config(text='OK', fg='#0c0')
            else:
                time.sleep(0.1)
        except Exception:
            time.sleep(0.1)
    
thread = threading.Thread(target=dumpSerial, daemon=True)
thread.start()

initUi()


# Names of first 2 gamepads we see so they do not get mixed up
pad_name=['','']
pad_connected=[False,False]
pad_lock=threading.Lock()

def showControllerInfo():
    if pad_connected[0]:
        labelCtr1Status.config(text='Connected', fg='#0c0')
    else:
        labelCtr1Status.config(text='Disconnected', fg='#888')
    if pad_connected[1]:
        labelCtr2Status.config(text='Connected', fg='#0c0')
    else:
        labelCtr2Status.config(text='Disconnected', fg='#888')

def controllerThread():
    global pad_name, pad_connected
    # The bits are: 0 - A, 1 - B, 2 - Select, 3 - Start, 
    #               4 - Up, 5 - Down, 6 - Left,7 - Right
    item_q = queue.Queue()
    def run_one(i,source):
        while True:
            if source != None:
                try:       
                    for item in source: 
                        x = i,item
                        item_q.put(x)
                except Exception:
                    pad_connected[i]=False
                    source=None
                    reload(inputs)
                    showControllerInfo()
            if not pad_connected[i]:
                # controller disconnected, now look for another, or the original comes back
                pad_lock.acquire()              # do not operate on pad data concurrently
                pads=inputs.devices.gamepads
                for pad in pads:
                    name=pad.get_char_name()
                    # print(name)
                    if name!=pad_name[1-i]:     # make sure it's not the other thread's pad
                        pad_name[i]=name
                        source=pad
                        pad_connected[i]=True
                        print(pad_name)
                        print(pad_connected)
                        showControllerInfo()
                        break
                pad_lock.release()
                time.sleep(0.1)

    pads=inputs.devices.gamepads
    # Enumerate all gamepads
    for i in range(2):
        if i < len(pads):
            pad_name[i]=pads[i].get_char_name()
            pad_connected[i]=True
            t = threading.Thread(target=run_one,args=(i,pads[i]), daemon=True)
        else:
            t = threading.Thread(target=run_one,args=(i,None), daemon=True)
        t.start()
    showControllerInfo()

    btns = [0,0]
    while True:
        # events = xi.get_events()
        u, events = item_q.get()
        btns_old = btns.copy()
        for e in events:
            print(u, e.ev_type, e.code, e.state)
            if (e.ev_type == 'Key' or e.ev_type == 'Absolute') and e.state!=0:      # button pressed
                # print('key pressed')
                if e.code == "BTN_SOUTH":
                    btns[u] |= 1
                elif e.code == "BTN_EAST":
                    btns[u] |= 1 << 1
                elif e.code == "BTN_START":               
                    btns[u] |= 1 << 2
                elif e.code == "BTN_SELECT":               
                    btns[u] |= 1 << 3
                elif e.code == "ABS_HAT0Y" and e.state==-1: # DPAD UP
                    btns[u] |= 1 << 4
                elif e.code == "ABS_HAT0Y" and e.state==1:
                    btns[u] |= 1 << 5
                elif e.code == "ABS_HAT0X" and e.state==-1:
                    btns[u] |= 1 << 6
                elif e.code == "ABS_HAT0X" and e.state==1:
                    btns[u] |= 1 << 7
            elif (e.ev_type == 'Key' or e.ev_type == 'Absolute') and e.state==0:
                # print('key unpressed')
                if e.code == "BTN_SOUTH":
                    btns[u] &= ~1
                elif e.code == "BTN_EAST":
                    btns[u] &= ~(1 << 1)
                elif e.code == "BTN_START":               
                    btns[u] &= ~(1 << 2)
                elif e.code == "BTN_SELECT":               
                    btns[u] &= ~(1 << 3)
                elif e.code == "ABS_HAT0Y":             # UP and DOWN unpressed
                     btns[u] &= ~(3 << 4)
                elif e.code == "ABS_HAT0X":             # LEFT and RIGTH unpressed
                    btns[u] &= ~(3 << 6)
        if btns[0] != btns_old[0] or btns[1] != btns_old[1]:
            print("Buttons: {0:02x}, {1:02x}".format(btns[0], btns[1]))
            b=bytearray(b'\x02')
            b.append(btns[0])
            b.append(btns[1])
            # print(b)
            connectSerial()
            ser.write(b)
            ser.flush()

thread2 = threading.Thread(target=controllerThread, daemon=True)
thread2.start()

# Show main GUI interface

top.mainloop()

