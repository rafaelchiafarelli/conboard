# conboard
Control Surface based on the Orange pi zero
Most of the usb-otg ware used from https://github.com/dpavlin/usb-otg
It is a good project and you definetly should check it out.

# What is done?


* generalized behavior
    * device installed as a keyboard, a mouse, a joystick and a PenDrive (bootable storage)
    * detection of inserted device with udev
    * launch of a service identified with json or a dummy service with a json constructed with udev variables
    * detection of witch device belongs to witch json and witch .service
    * Keyboard and Mouse are standard to the PC
    * All devices can send Keyboard and/or mouse.
    * a websocket will launch any user event to the client
    * operation mode for all devices, no limit of operating modes
    * standarized language for all input devices

* midi device (IO) supported
    * Input/Output MIDI 
    * Memory kept for the current working mode
    * Event for push down and for pull up key
    * Delay in action to send to the PC
    * Multiple output to the device types (blink, normal, etc)

* keyboard device (IO) supported
    * keys are read 

* joystick device (IO) supported

    
# What is Missing?

* generalized behavior 
    * missing installing the system as an ethernet port (it would sinplify access by users)
    * detection of inserted device does not filter by device type (MIDI, Keyboard, Joystick or HID,). Always push this event to the launcher witch in turn, creates a dummy device, even if this device is a usb hub, or something that does not have any actions.
    * lauch of a service is very heavily dependent on the user configuration witch is very dangerous, user should not have that power. 
    * detection of the device is very heavily dependent on the user configuration witch is very dangerous, user should not have that power.
    * no UI yet. only a websocket 
    * no security planned yet, needs a firewall or something that could prevent outside access to the communications. 
    * backend in python-flask is deprecated, but it makes more sense since has a lot of features that would speed up the process
* midi device
    * SysEx commands are not working
    * multiple commands and multiple actions can overrun the system.
* keyboard device 
    * not even working properly
* joystick device
    * not even started.

    

    

