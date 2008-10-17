#include "tests.h"

int nbError;
int nbTest;

int nbStreaming;
uid_t nbUid;
gid_t nbGid;

char file [MAX_PATH_LENGTH];
char srcdir [MAX_PATH_LENGTH];

#ifdef HAVE_CLEARENV
int clearenv();
#endif

/**Does some useful startup.
 */
int init (void)
{
	setlocale (LC_ALL, "");

	nbUid = getuid();
	nbGid = getgid();

	if (getenv ("srcdir"))
	{
		strcpy (srcdir, getenv ("srcdir"));
	} else {
		strcpy (srcdir, ".");
		warn_if_fail (0, "srcdir not set, will try current directory");
	}
#ifdef HAVE_CLEARENV
	clearenv();
#else
	unsetenv("HOME");
	unsetenv("USER");
	unsetenv("KDB_HOME");
	unsetenv("KDB_USER");
	unsetenv("KDB_DIR");
#endif

#ifdef HAVE_SETENV
	setenv("KDB_HOME",".",1);
#endif

	nbStreaming =  loadToolsLib();
	warn_if_fail (nbStreaming == 0, "Unable to load elektratools, no stream testing will be done");
	return 0;
}

/**Create a root key for a backend.
 *
 * @return a allocated root key */
Key * create_root_key (const char *backendName)
{
	Key *root = keyNew ("user/tests", KEY_END);
	/*Make mount point beneath root, and do all tests here*/
	keySetDir(root);
	keySetUID(root, nbUid);
	keySetGID(root, nbGid);
	keyAddBaseName (root, backendName);
	keySetString (root, backendName);
	keySetComment (root, "backend root key for tests");
	keySetString (root, backendName);
	return root;
}

/**Create a configuration keyset for a backend.
 *
 * @return a allocated configuration keyset for a backend*/
KeySet *create_conf (const char *filename)
{
	return ksNew (2,
		keyNew("system/path", KEY_VALUE, ".kdb/hosts_kdb", KEY_END),
		KS_END);
}


int compare_line_files (const char *filename, const char *genfilename)
{
	FILE *forg, *fgen;
	char bufferorg [BUFFER_LENGTH + 1];
	char buffergen [BUFFER_LENGTH + 1];
	int line = 0;

	forg = fopen (filename, "r");
	fgen = fopen (genfilename, "r");
	exit_if_fail (forg && fgen, "could not open file");

	while (	fgets (bufferorg, BUFFER_LENGTH, forg) &&
		fgets (buffergen, BUFFER_LENGTH, fgen))
	{
		line ++;
		if (strncmp (bufferorg, buffergen, BUFFER_LENGTH))
		{
			printf ("In file %s, line %d.\n", filename, line);
			fclose (forg); fclose (fgen);
			succeed_if (0, "comparing lines failed");
			return 0;
		}
	}
	fclose (forg); fclose (fgen);
	return 1;
}


/**Compare two files line by line.
 *
 * Fails when there are any differences.
 *
 * The original file is passed as parameter.
 * It will be compared with the file -gen.
 *
 * file.xml -> file-gen.xml (xml comparator)
 * file.txt -> file-gen.txt (line comparator)
 * file.c -> file-gen.c (c comparator)
 *
 */
int compare_files (const char * filename)
{
	char genfilename [MAX_PATH_LENGTH];
	char * dot = strrchr (filename, '.');

	exit_if_fail (dot != 0, "could not find extension in file");

	strncpy (genfilename, filename, dot- filename);
	/* does not terminate string, but strcat need it, so: */
	genfilename[dot-filename] = 0;
	strcat  (genfilename, "-gen");
	if (!strcmp (dot, ".xml"))
	{
		strcat (genfilename, ".xml");
	} else if (!strcmp (dot, ".txt"))
	{
		strcat (genfilename, ".txt");
	} else if (!strcmp (dot, ".c"))
	{
		strcat (genfilename, ".c");
	}

	return compare_line_files (filename, genfilename);
}


int compare_key (Key *k1, Key *k2, KDBCap *cap)
{
	int	ret;
	int	err = nbError;

	/* printf ("compare: "); keyOutput (k1, stdout); keyOutput (k2, stdout); printf ("\n"); */
	ret = keyCompare(k1, k2);

	if (cap)
	{
		succeed_if ((ret & KEY_NAME) == 0 , "compare key: NAME not equal");
		if (!kdbcGetnoTypes(cap))	succeed_if ((ret & KEY_TYPE) == 0 , "compare key: TYPE not equal");
		if (!kdbcGetnoValue(cap))	succeed_if ((ret & KEY_VALUE) == 0 , "compare key: VALUE not equal");
		if (!kdbcGetnoOwner(cap))	succeed_if ((ret & KEY_OWNER) == 0 , "compare key: OWNER not equal");
		if (!kdbcGetnoComment(cap))	succeed_if ((ret & KEY_COMMENT) == 0 , "compare key: COMMENT not equal");
		if (!kdbcGetnoUID(cap))		succeed_if ((ret & KEY_UID) == 0 , "compare key: UID not equal");
		if (!kdbcGetnoGID(cap))		succeed_if ((ret & KEY_GID) == 0 , "compare key: GID not equal");
		if (!kdbcGetnoMode(cap))	succeed_if ((ret & KEY_MODE ) == 0 , "compare key: MODE  not equal");
	} else {
		succeed_if ((ret & KEY_NAME) == 0 , "compare key: NAME not equal");
		succeed_if ((ret & KEY_TYPE) == 0 , "compare key: TYPE not equal");
		succeed_if ((ret & KEY_VALUE) == 0 , "compare key: VALUE not equal");
		succeed_if ((ret & KEY_OWNER) == 0 , "compare key: OWNER not equal");
		succeed_if ((ret & KEY_COMMENT) == 0 , "compare key: COMMENT not equal");
		succeed_if ((ret & KEY_UID) == 0 , "compare key: UID not equal");
		succeed_if ((ret & KEY_GID) == 0 , "compare key: GID not equal");
		succeed_if ((ret & KEY_MODE ) == 0 , "compare key: MODE  not equal");
	}

	return err-nbError;
}

/**Compare two keysets.
 * Compare if two keysets contain the same keys.
 * There is a filter for ks, to remove inactive keys, directories and non-directories
 * You can use:
 * - option_t::KDB_O_NODIR 
 *   remove all directories (keyIsDir)
 * - option_t::KDB_O_DIRONLY 
 *   remove everything but directories (!keyIsDir)
 * - option_t::KDB_O_INACTIVE (keyIsInactive) 
 *   remove all inactive keys
 * - option_t::KDB_O_STATONLY
 *   remove all value/comments
 * */
int compare_keyset (KeySet *ks, KeySet *ks2, int filter, KDBCap *cap)
{
	Key	*key = 0;
	Key     *key2 = 0;
	int	size = 0;
	int	dup = 0;
	int	err = nbError;

	// I would have a _true_ ksCompare() ...
	ksSort(ks); ksRewind(ks);
	ksSort(ks2); ksRewind(ks2);

	//SYNC with ksOutput
	while ((key = ksNext(ks)) != 0)
	{
		if (filter & KDB_O_NODIR)    if (keyIsDir (key)) continue;
		if (filter & KDB_O_DIRONLY)  if (!keyIsDir (key)) continue;
		if (filter & KDB_O_INACTIVE) if (keyIsInactive (key)) continue;
		if (filter & KDB_O_STATONLY)
		{
			if (!kdbcGetnoStat(cap))
			{	// only remove values for backends which are capable of stating keys.
				key = keyDup (key); // use a duplicate here...
				dup = 1;
				keySetRaw (key, NULL, 0);
				keySetComment (key, NULL);
				keySetType (key, KEY_TYPE_UNDEFINED);
			}
		}
		key2 = ksNext(ks2);
		if (!key2)
		{
			succeed_if (0, "Will break, did not find corresponding key2");
			if (dup) keyDel (key);
			break;
		}

		size ++;
		compare_key (key, key2, cap);
		if (dup) keyDel (key);
	}

	if (size == 0) succeed_if (0, "real size was 0");
	if ( size != ksGetSize(ks2) ) {
		printf ("%d, %d\n", (int)ksGetSize(ks), (int)ksGetSize(ks2));
		succeed_if( 0, "There are less keys fetched than keys which have been submitted.");
	}
	if ( err-nbError )
	{
		if (key && key2) printf ("error comparing %s - %s\n", keyName(key), keyName(key2));
		else printf ("error comparing null key\n");
	}
	return err-nbError;
}


int loadToolsLib(void) {
	kdbLibHandle dlhandle=0;

	kdbLibInit();

	dlhandle=kdbLibLoad("libelektratools");
	if (dlhandle == 0) {
		return 1;
	}
	
	ksFromXMLfile=(KSFromXMLfile)kdbLibSym(dlhandle,"ksFromXMLfile");
	ksFromXML=(KSFromXML)kdbLibSym(dlhandle,"ksFromXML");

	ksToStream = (output) kdbLibSym (dlhandle, "ksToStream");
	ksOutput   = (output) kdbLibSym (dlhandle, "ksOutput");
	ksGenerate = (output) kdbLibSym (dlhandle, "ksGenerate");

	return 0;
}

/* return file name in srcdir.
 * No bound checking on file size, may overflow. */
char * srcdir_file(const char * fileName)
{
	strcpy(file, srcdir);
	strcat(file, "/");
	strcat(file, fileName);
	return file;
}

