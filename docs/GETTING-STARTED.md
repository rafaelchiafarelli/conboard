# Getting started with conboard

This is the short version, for someone turning on a conboard unit for the first
time — not the engineering documentation. If you're setting up the software itself
(building, installing, developing), start at the [main README](../README.md) instead.

## What you have

A small box that plugs into your computer over USB and acts like a keyboard, mouse,
MIDI device, or joystick — but every button, knob, or key on whatever controller you
plug *into* it can be freely remapped to whatever it should send to your computer.
You configure it from a web page served by the box itself; no software to install on
your computer.

## First power-on

1. Plug the box into power, and connect it to your network (or your computer's USB
   port, depending on how it was set up for you).
2. Find its address — either the IP address you were given, or `conboard.local` if
   your network supports it.
3. Open that address in a browser. First time, your browser will ask for a
   **username and password** — this is the console's login, unique to your unit
   (whoever set it up for you has it). If you have terminal access to the box
   yourself: `sudo conboard-password` shows it, `sudo conboard-password --reset`
   generates a new one if it's ever lost — losing this login is never permanent.
4. You're in the console. From here:
   - **Devices** (left rail) — the controller(s) currently plugged into the box.
     "Add" offers only devices it can actually see attached.
   - Click a device to open its **rule editor** — pick a trigger (a button, key, or
     knob on the controller) and an output (what it should do), save.
   - **Deploy** pushes your edited rules live; the box starts acting on them
     immediately, no reboot.
   - The **monitor** view shows live activity from connected devices as you use
     them — handy for confirming a rule fired the way you expected, or for seeing
     exactly what a button sends before you write a rule for it.

## If something looks unfinished

conboard is feature-complete for its core job (remap any attached controller through
a web console), but it's still short of a full 1.0:
- A few workflows are still rough around the edges — if the console does something
  confusing, it's more likely an unpolished corner than something you're doing wrong.
- There's no small on-device screen yet on units without one wired up; everything is
  driven through the web console described above.

## Getting help

Ask whoever gave you the unit — they'll have the technical documentation
([README](../README.md), [docs/NEXT-SESSION.md](NEXT-SESSION.md) for known open
items) if something needs a closer look.
