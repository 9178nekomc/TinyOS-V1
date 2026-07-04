/* TinyOS V1 — Installation Wizard
 *
 * Interactive installer that:
 *   1. Shows a welcome screen
 *   2. Lists detected disks
 *   3. Lets the user select a target disk
 *   4. Writes system files and marks the disk as bootable
 *   5. On next boot, the system starts directly
 */

#ifndef TINYOS_INSTALL_H
#define TINYOS_INSTALL_H

#include "uefi/types.h"

/* Run the installation wizard on the graphical terminal.
 * Returns true if installation was completed successfully.
 * After a successful install, the caller can reboot or start the shell. */
bool install_run();

#endif /* TINYOS_INSTALL_H */
