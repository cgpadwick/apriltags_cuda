# AprilTags Installer

## NOTES
As of recent (this update), there is no longer a webserver being served by python. To update versions, MAKE SURE TO UNINSTALL THEN REINSTALL (you do not need to use any parameters with this uninstall) OR run the installer again WITHOUT PRESERVE (being -p).
It is assumed that the shell commands run are executed in the directory of the installer.

## Installation
Installation is fairly simple. Run:
```bash
./install.sh
```
to install the cuda service. If this is for a reinstall and you wish to preserve the backend that is currently stored, then run:
```bash
./install.sh -p
```

## Uninstallation
Uninstallation just needs you to run:
```bash
./uninstall.sh
```
To fully remove everything (like the apps directory and PATH modifications) run:
```bash
./uninstall.sh all
```

## Frequent Issues
Sudo does weird things with the system PATH enviornment variable, so at times, you can not call the helper scripts that are installed by this program. That is ok. You can just call them directly from /apps/bin/ whatever your script is.
