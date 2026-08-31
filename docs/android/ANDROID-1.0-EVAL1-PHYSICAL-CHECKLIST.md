# Tech Aim Android 1.0.0-EVAL1 — physical checklist

**Read this first.** The Android USB transport is implemented and covered by
42 automated assertions, but **it has never run on a device**. No tablet and no
CH340 were attached to the machine it was built on, so every USB state
transition below is being exercised for the **first time** by whoever runs this
list. That is the entire point of the exercise.

If something fails, that is a result, not a waste — and it is the result this
build exists to produce.

**Ammunition needed: about 15 shots.** The rule and competition logic is
already covered by 3 638 automated checks. What only a physical target can
prove is USB, CH340, Modbus, reconnect, feed and lifecycle.

---

## TEST 1 — install and start

- [ ] APK installs on the tablet
- [ ] Tech Aim launches
- [ ] **landscape**
- [ ] identity reads **Tech Aim** and version **1.0.0-EVAL1**
- [ ] no crash, no error dialog on first run

## TEST 2 — USB host and permission  ← the one that matters most

- [ ] connect the existing target: tablet → **USB-C OTG/host adapter** → the CH340 cable that works on Windows
- [ ] Android shows a **USB permission prompt** — accept it
- [ ] the app reaches **Target ready**

Record, because this is evidence the build could not gather for itself:

- [ ] **VID / PID actually reported by the target:** `____________`
      (expected `VID 0x1A86 PID 0x7523`; anything else must be added to the whitelist)
- [ ] **Did the target RESET when the port opened?**  YES / NO

  The reset question is not idle. The desktop application never touches DTR, so
  this build leaves DTR at the library default rather than inventing a value.
  Some boards wire DTR to a reset line. **If the target resets at connect, say
  so — DTR is the first thing to change.**

If permission is DENIED on purpose:

- [ ] the app says so plainly, does **not** loop the dialog, and offers a retry
- [ ] unplugging and replugging asks again

## TEST 3 — normal shots

Fire about **five** shots.

- [ ] every shot detected, none doubled, none missed
- [ ] coordinates look right against the visible group
- [ ] scores match
- [ ] **exactly one paper feed per accepted shot**

## TEST 4 — the 10-shot boundary

Continue to shot **12**, watching **8, 9, 10, 11, 12**.

- [ ] every one of those five accepted, in order
- [ ] no duplicate at the boundary
- [ ] no missing shot at the boundary
- [ ] feed still one-for-one across it

## TEST 5 — disconnect

Mid-session, **pull the USB cable**.

- [ ] the app shows **Connection lost** promptly
- [ ] no crash
- [ ] the session and shot count are still intact on screen
- [ ] **no paper feed happens** because of the disconnect

## TEST 6 — reconnect  ← the second most important

Plug it back in. Accept the permission prompt if it appears.

- [ ] reaches **Target ready** again
- [ ] fire **three** more shots
- [ ] no duplicate of the last shot before the disconnect
- [ ] no missing shot
- [ ] no corrupted coordinate
- [ ] shot count did **not** reset
- [ ] the clock did **not** restart
- [ ] **no extra paper feed** from the reconnect itself

## TEST 7 — background

With the session still active:

- [ ] press Home, wait about **one minute**, return to the app
- [ ] competition time reflects the **whole** minute that passed — it must not
      have paused while the app was in the background
- [ ] connection state is sane (either still ready, or a clear reconnect)

## TEST 8 — persistence

- [ ] save the session
- [ ] close the app fully and reopen it
- [ ] the saved session is there and opens

## TEST 9 — report

- [ ] generate a report / PDF
- [ ] share or export it — the Android share sheet should appear
- [ ] the shared file opens correctly on the receiving side

## TEST 10 — support bundle

- [ ] generate the Android support bundle
- [ ] share it using the Android share sheet
- [ ] it arrives and contains the session journals

**Please return the support bundle whatever happens** — pass or fail. It
carries the transport diagnostics (VID/PID, permission state, connection state,
serial settings, last error, reconnect count) that make a failure diagnosable
rather than guessable.

---

## What is NOT being asked

A 60-shot Qualification, a full 3P course or a full Final. The rule logic is
automated and does not need ammunition to re-prove. If a representative
competition context is convenient, use one — but do not burn ammunition to
retest rules.

## What to write down if something fails

Which test number, what you saw, roughly what time it happened, and the support
bundle. The approximate time matters more than it sounds: it is what lets the
logs be matched to what you saw.
