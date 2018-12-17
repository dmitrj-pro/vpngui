# OpenVPN Linux GUI

# How build (Linux only)

1) Download DPLib

    wget http://sr2.c-o-d.net/dp/lib.tar.gz

2) Unpack it

    tar -xvf lib.tar.gz

3) Build

    cd lib; make

4) Clone VPNGui

    git clone https://github.com/dmitrj-pro/vpngui

5) Generate makefiles:

    cd vpngui
    
    ./init.sh

6) Generator ask question. Example:

    $ ./init.sh 
    
    Path to builded DPLib
    
    Input path to libdp: /home/diman-pro/tmp/sss/lib  
    
    
    Password for encrypt all conig files (openvpn and vpngui)
    
    Input encrypt password: 1234   
    
    
    Enable all log and disable encryption (y). Disable log and enable encryption (n)
    
    Debug (y)/Release(n): n   
    
    
    Path where vpngui put openvpn configs
    
    Path to configs (Ex: /home/vpn): /opt/vpngui  
    
    
    Path co vpngui config
    
    System config name (Ex: /home/vpn/vpn.conf): /etc/vpngui.conf   
    
7) Build Core:

  7.1) Open Core directory
  
    cd Core
  
  7.2) Build
    
    make
  
  7.3) Install:
  
  7.3.1) Copy server file to other folder (Example:
    
    sudo cp server /opt/vpngui/
    
  7.3.2) Add to autorun (Example:
    
    add line "/opt/vpngui/server &" to file /etc/rc.local as root
    
8) Build gui:

  8.1) Open Gui directory
    
    cd GUI
    
  or
      
    cd ../GUI
  
  8.2) Generate Makefile from pro file
  
    qmake
  
  8.3) build
  
    make
  
  8.4) Edit desktop file vpn.desktop and edit Exec and Icon line to set path to file GUI and vpn.jpg
  
  8.5) Set execute desktop file
  
    chmod +x vpn.desktop
    
    
    
