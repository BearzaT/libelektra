/**
 * @file
 *
 * @brief A plugin that makes use of libaugeas to read and write configuration files
 *
 * @copyright BSD License (see doc/COPYING or http://www.libelektra.org)
 *
 */


#ifndef ELEKTRA_PLUGIN_RENAME_H
#define ELEKTRA_PLUGIN_RENAME_H

#include <kdberrors.h>
#include <kdbextension.h>
#include <kdbplugin.h>

#define ELEKTRA_ORIGINAL_NAME_META "origname"

int elektraRenameGet (Plugin * handle, KeySet * ks, Key * parentKey);
int elektraRenameSet (Plugin * handle, KeySet * ks, Key * parentKey);
Key * elektraKeyCreateNewName (const Key * key, const Key * parentKey, const char * cutPath, const char * replaceWith, const char * toUpper,
			       const char * toLower, const int initialConversion);

Plugin * ELEKTRA_PLUGIN_EXPORT (rename);

#endif
