GEOMETRY MARIO - PSP CLOUD BUILD PROJECT

This ZIP contains the Geometry Mario PSP source project plus a GitHub Actions
workflow that compiles it in the cloud. You do NOT need to install PSPSDK locally.

IMPORTANT:
- This builds a PSP homebrew EBOOT.PBP, NOT an ISO.
- A USB cable only transfers the finished file.
- Your PSP 3000 must support homebrew (compatible custom firmware).

CLOUD BUILD STEPS
1. Sign in to GitHub or create an account.
2. Create a NEW repository. You can name it Geometry-Mario-PSP.
3. Upload the CONTENTS of this folder to the repository (main.c, Makefile,
   and the .github folder). Make sure .github/workflows/build.yml is included.
4. Open the repository's Actions tab.
5. Select "Build Geometry Mario for PSP" on the left.
6. Press "Run workflow", then the green "Run workflow" button.
7. Wait for the run to finish successfully.
8. Open that completed run and download the artifact named
   "Geometry-Mario-EBOOT".
9. Extract the downloaded artifact. It should contain EBOOT.PBP.

INSTALL ON PSP
1. On the PSP Memory Stick, open PSP/GAME.
2. Create a folder named GEOMARIO.
3. Copy EBOOT.PBP into PSP/GAME/GEOMARIO/.
4. Safely disconnect USB and open Game > Memory Stick.

IF BUILD FAILS
Open the failed Actions run, copy the error log, and send it here. Do not
copy the source files directly to PSP/GAME; the PSP needs the compiled
EBOOT.PBP.

This is an early prototype with simple placeholder graphics and gameplay.
