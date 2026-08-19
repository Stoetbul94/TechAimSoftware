===============================================================================
 TECH AIM RANGE MANAGEMENT SYSTEM
 FIELD TEST / DEVELOPMENT EVALUATION PACKAGE
 Milestone 4.6 - GEOMETRY-QUALIFIED target display
===============================================================================

 THIS IS NOT A COMPETITION RELEASE.
 NOT FOR OFFICIAL COMPETITION CONTROL.

 This build supersedes the earlier M4.5 package. Use this one.

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

 The shots are CORRELATED FIXTURE DATA: each one's position and its score
 genuinely belong together. They were generated outside this program from the
 official rulebook geometry and the target station's own scoring formula, then
 frozen. RMS reads a position and a score and does not compute the
 relationship - it has no way to.

 So after about a minute you are looking at all six states at once:

     Lane 1   10 m Air Rifle    online, shooting
     Lane 2   10 m Air Pistol   online, shooting
     Lane 3   10 m Air Rifle    OFFLINE - dimmed, still listed, last known data
     Lane 4   50 m Rifle        online, with an unseen-shot warning
     Lane 5   10 m Air Pistol   FINISHED   (SIMULATED)
     Lane 6   50 m Pistol       ELIMINATED (SIMULATED)

 Four different target faces, so all four qualified geometries can be seen in
 one range.

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
 WHAT CHANGED SINCE THE M4.5 PACKAGE
-------------------------------------------------------------------------------

 The target faces were qualified against the current official rulebook
 (ISSF Rule Book 2026, EDITION 2025 Second Print 07/2026, rule 6.3.4). Three
 things were wrong and are now fixed:

 1. 50 M PISTOL WAS DRAWING A 50 M RIFLE FACE. Every 50 m pistol shot was
    plotted at about 4.8 times its true distance from centre. The pistol face
    is a completely different target - 50 mm ten ring against 10.4 mm.

 2. THE 50 M RIFLE BLACK STOPPED TOO SOON. The rule puts it at 112.4 mm
    diameter, which is past the edge of the face RMS draws, so the whole 50 m
    rifle face is black. It previously stopped at the 5 ring.

 3. THE BULLET HOLE WAS AN ARBITRARY DOT. It is now drawn at the real calibre -
    4.5 mm at 10 m, 5.6 mm at 50 m, from the ammunition rules.

 THAT THIRD ONE MATTERS MORE THAN IT SOUNDS, and it is worth understanding
 before you judge what you see:

 ISSF scores by the OUTWARD GAUGE - the EDGE of the hole, not its centre. On a
 10 m air rifle target the ten ring is 0.5 mm across and the pellet is 4.5 mm,
 NINE TIMES WIDER. So a shot scoring exactly 10.0 has its CENTRE about 2.5 mm
 out from the middle, well outside the ten ring. That is correct. If the
 display drew only a small dot it would look like a scoring error; drawn at the
 true size, you can see the hole's edge touching the ring that the score names.

 On the 10 m air rifle lane the holes are large and overlap heavily. That is
 what a real 10 m air rifle card looks like after twenty shots on one aiming
 mark - it is not a rendering fault.

 The demo data was also fixed. In the M4.5 package the demo generated a shot's
 position and its score independently, so they disagreed on purpose-built
 nonsense. They now match.


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
   the target station (or, in demo mode, from the correlated fixture data). RMS
   places a shot on a face so you can see the group; it never turns a position
   into a value. There is no function anywhere in the product that takes a
   coordinate and returns a score, and the build is scanned for one.

 - 3P FINALS ELIMINATION IS SIMULATED IN DEMO ONLY. The current protocol
   carries no competition status at all, so no real station can report FINISHED
   or ELIMINATED yet. Where you see those words in the demo they are stamped
   SIMULATED, because a script put them there. RMS never works elimination out
   for itself - not from score, rank, shot count, or an athlete going quiet.

 - PHYSICAL TARGET DISPLAY PATH IS UNVERIFIED. No real target hardware has been
   used with the display yet, and no shot has been fired. In particular, which
   way is UP is taken from the target application's own renderers, not from a
   measurement. A high shot should appear high; that has never been confirmed
   against a real pellet.

   There is a fifteen-minute range procedure that settles it, in the repository
   at:  docs/test/rms-physical-shot-registration-checklist.md
   It walks through centre, right, left, above, below, and several known
   ring-region impacts. Until it is done and returned, the project's status
   stays PHYSICALLY UNVERIFIED.

 - Results, ranking, leaderboards, follow-the-leader displays, a finals
   director view and a smart-TV client are not implemented.

 - Ranking, results, leaderboards and any Smart-TV client are not implemented.


-------------------------------------------------------------------------------
 IF SOMETHING GOES WRONG
-------------------------------------------------------------------------------

 The package is self-contained: it needs no Qt installation and nothing on
 PATH. If the application will not start, the most useful thing to send back is
 the exact text of the error dialog, plus deployment-manifest.json from this
 folder.

 FIELD-TEST-CHECKLIST.txt in this folder lists what to try, in order.
