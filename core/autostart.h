#ifndef AUTOSTART_H
#define AUTOSTART_H

namespace Autostart {

bool isSupported();

bool isEnabled();

bool isSetForAllUsers();

bool setEnabled(bool on);

}

#endif