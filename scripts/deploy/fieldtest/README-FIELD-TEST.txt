===============================================================================
 TECH AIM RANGE MANAGEMENT SYSTEM
 FIELD TEST / DEVELOPMENT EVALUATION PACKAGE
 Milestone 4.5 - target display MVP
===============================================================================

 THIS IS NOT A COMPETITION RELEASE.

 It is a development evaluation build, produced so the user interface can be
 clicked through by a person before a field test. Do not use it to run, record
 or publish a competition.


-------------------------------------------------------------------------------
 HOW TO RUN THE DEMO
-------------------------------------------------------------------------------

 Double-click:

     Launch-TechAimRMS-Demo.cmd

 No hardware, no network and no targets are needed. A demonstration range
 appears after a few seconds and starts shooting.

 EVERYTHING IN DEMO MODE IS GENERATED INSIDE THIS PROGRAM. No station is
 contacted, no real athlete is involved, and none of the names, scores or
 shots are competition data. The window header says DEMO RANGE - NOT REAL
 TARGETS for as long as the demo is running, and the port badge says NOT
 OBSERVING.

 WHAT THE DEMO DOES, AND WHEN
 (seconds from launch)

     3    six stations appear; the range and the match plan are created
     3+   every lane starts shooting
    14    Lane 3 loses the network and STAYS offline
    14    Lane 4 loses the network
    34    Lane 4 comes back, now carrying an unseen-shot warning
    40    the script declares Lane 5 FINISHED       (marked SIMULATED)
    50    the script declares Lane 6 ELIMINATED     (marked SIMULATED)
    93    the 60-shot lanes finish; the range then holds its final state

 So after about a minute you are looking at all six states at once:

     Lane 1   online, shooting
     Lane 2   online, shooting
     Lane 3   OFFLINE - dimmed, still listed, showing its last known data
     Lane 4   online, with an unseen-shot warning
     Lane 5   FINISHED   (SIMULATED)
     Lane 6   ELIMINATED (SIMULATED)

 The state is worth leaving to run for a minute before judging it.


-------------------------------------------------------------------------------
 HOW TO RUN LIVE
-------------------------------------------------------------------------------

 Double-click:

     Launch-TechAimRMS-Live.cmd

 RMS then listens for real target stations broadcasting on UDP 7755.

 If nothing is broadcasting, the range will correctly show nothing. That is
 not a fault - it means nothing was heard.

 LIVE and DEMO cannot run at the same time in one window. LIVE opens the
 network socket and builds no simulator; DEMO builds a simulator and opens no
 socket. The header always says which one you are looking at.


-------------------------------------------------------------------------------
 HOW TO EXIT FULL SCREEN
-------------------------------------------------------------------------------

 Press ESC.

 There is also an EXIT FULL SCREEN button in the top right of the full-screen
 window. Both always work - a display you cannot get out of would be worse
 than no display.

 In full screen you can also use:

     LEFT / RIGHT arrows    previous / next lane
     SPACE                  start or stop auto rotate
     ESC                    leave full screen


-------------------------------------------------------------------------------
 HOW TO RESET THE DEMO
-------------------------------------------------------------------------------

 Double-click:

     Reset-Demo.cmd

 That throws away the demo's saved range, athletes and plan and starts the
 demonstration again from the beginning. You never need to delete anything by
 hand.

 The demo keeps its files in their own folder:

     %LOCALAPPDATA%\Tech Aim\Tech Aim RMS\field-test-demo\

 Your LIVE range configuration lives one level up, in

     %LOCALAPPDATA%\Tech Aim\Tech Aim RMS\

 and is never opened, read or written by a demo run. Resetting the demo cannot
 damage it.


-------------------------------------------------------------------------------
 NETWORK
-------------------------------------------------------------------------------

 Expected port:   UDP 7755   (target stations broadcast; RMS listens)

 RMS only ever RECEIVES on this port. There is no transmitting socket anywhere
 in the product.

 UDP 7756 - the target's own legacy inbound control port - is deliberately not
 used by RMS and is not touched by it.

 Windows Firewall may ask to allow TechAimRMS.exe to receive on a private
 network the first time LIVE mode runs. Allowing it on the range network is
 what lets RMS hear the stations.


-------------------------------------------------------------------------------
 WHAT IS IMPLEMENTED
-------------------------------------------------------------------------------

 - Read-only observation of target stations on UDP 7755, with duplicate
   rejection, out-of-order handling and reconnection after an outage.
 - A configured physical range: lanes that persist, and a persistent link
   between a lane and a station, so a station that returns on a new address
   still lands on its own lane with no operator action.
 - Athletes, match plans and programme selection. A plan records an intention
   and is not sent anywhere.
 - Planned versus observed comparison: when a station reports a different
   athlete or programme from the plan, both are shown and neither is changed.
 - The target display: ALL TARGETS, one target large, previous / next, full
   screen, and auto rotate at 5 / 10 / 15 / 30 seconds.
 - Honest handling of shots RMS did not see: they are counted and stated, not
   invented and not smoothed over.


-------------------------------------------------------------------------------
 WHAT IS NOT IMPLEMENTED
-------------------------------------------------------------------------------

 - RMS CANNOT CONTROL TARGETS. There is no start, stop, reset, match, sighting,
   position change, paper feed, pause, resume or load-match command anywhere in
   the product, and no command exists in the protocol to carry one. The target
   station remains the authority for everything about the match.

 - RMS DOES NOT CALCULATE SCORES. Every score and every total shown comes from
   the target station (or, in demo mode, from the demonstration data). RMS
   places a shot on a face so you can see the group; it never turns a position
   into a value.

 - 3P FINALS ELIMINATION IS SIMULATED IN DEMO ONLY. The current protocol
   carries no competition status at all, so no real station can report FINISHED
   or ELIMINATED yet. Where you see those words in the demo they are stamped
   SIMULATED, because a script put them there. RMS never works elimination out
   for itself - not from score, rank, shot count, or an athlete going quiet.

 - PHYSICAL TARGET DISPLAY PATH IS UNVERIFIED. No real target hardware has been
   used with the display yet. The shot positions drawn from a live station
   should be checked against the physical target before anyone relies on them.

 - Results, ranking, leaderboards, follow-the-leader displays, a finals
   director view and a smart-TV client are not implemented.

 - The demo's shot positions and its scores are generated independently of one
   another, so in DEMO mode the position a shot is drawn at does not correspond
   to the score shown next to it. That is a property of the demonstration data,
   not of the display. Correlating them would require a rule that turns a
   position into a score, which is the one thing this product must not contain.


-------------------------------------------------------------------------------
 IF SOMETHING GOES WRONG
-------------------------------------------------------------------------------

 The package is self-contained: it needs no Qt installation and nothing on
 PATH. If the application will not start, the most useful thing to send back is
 the exact text of the error dialog, plus deployment-manifest.json from this
 folder.

 FIELD-TEST-CHECKLIST.txt in this folder lists what to try, in order.
