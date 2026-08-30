Tech Aim Electronic Target Control
Version 1.0.0


STARTING THE APPLICATION
------------------------
Double-click TechAim.exe in this folder.

Nothing needs to be installed. The folder is self-contained - it can be copied
to another Windows PC and run from there, including from a USB drive.

Only one copy may run on a machine at a time. This is deliberate: one computer
drives one target.


CONNECTING THE TARGET
---------------------
1. Connect the target to the computer.
2. Start TechAim.exe.
3. Choose the COM port for the target on the start screen.

The serial settings are already configured for the target electronics:

    19200 baud, Even parity, 8 data bits, 1 stop bit

You do not normally need to change them. If your installation uses different
settings, they can be changed on the connection screen.

The status strip shows when the target is connected.


WHERE YOUR RESULTS ARE SAVED
----------------------------
Match records are written into THIS folder, next to TechAim.exe, as files
named:

    Match_<date>-<time>.tch

A saved match can be reopened from the start screen using
"Load saved session".

Reports are opened from the toolbar inside a session. "Save PDF" asks you where
to put the PDF file - choose any folder you like.

Keep this folder somewhere you can find it. If you copy the application to a
new PC, copy your .tch files across too.


IF YOU NEED SUPPORT
-------------------
If something goes wrong, or a session did not behave as you expected:

    RUN Collect-Logs.cmd BEFORE CLEANING UP OR REINSTALLING.

Double-click Collect-Logs.cmd in this folder. It writes a support file to your
Desktop containing the application logs, the session records and the
configuration. Send that file with your description of the problem.

Do this after EVERY test session you want looked at, pass or fail. Some of the
logs are stored temporarily by Windows and are removed automatically after a
while - once they are gone they cannot be recovered.

The support file contains no passwords and no personal files.


CONTACT
-------
support@techaim.co.za


Tech Aim Electronic Target Control 1.0.0
Copyright (C) JAC SHOOTING SOLUTIONS (PTY) LTD
