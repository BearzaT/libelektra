#Error message & handling concept

## Problem

The current error concept has disadvantages in following regards:

- Too verbose error message
  Currently for every error, 9 lines are shown in which most of them are not relevant to end users/administrators. One goal
  is to reduce the verbosity of such messages and let users/administrators see only information they need.
- A lot of redundant errors
  At the moment, each new plugin introduces new error codes which led to about 210+ error codes. Many of those errors
  are duplicated because developers did not know or search for a similar error which is already present. This concept should
  group similar errors together so that there is one coherent and consistent state again.
- Hard to manage specification file
  Since every developer adds its own error individually, a lot of merge conflicts happen which makes contributing to the codebase
  unpleasant. Additionally, if you want to reuse any error you have to scrape to the whole file with ~1300+ lines. As there is no
  senseful ordering or scheme behind the errors (since they grew by time), it is a hassle to find the correct error code.
  The new concept should standardize errors, making it easy to categorize errors from new plugins and avoid merge conflicts.
- No senseful way for application developers to use error codes from elektra
  If developers of plugins/ external tools using elektra want to react to errors, they have to be very specific. At the moment there is
  no possibility to catch all errors easily which force a certain behavior. Eg. if there happens a temporary recoverable error, developers have to
  catch for every specific error code rather than a general hierarchical error. The new concept should make it easy to react to errors as they are
  sensefully grouped together and are hierarchically structured.

## Constraints

- Error numbers must stay because they are more reliable to check against than strings
- Supporting multiple programming languages
- Plugin System

## Assumptions

## Considered Alternatives

- Removing the specification file without requiring error numbers
- Possible variations on what message should be displayed,
  eg. to keep the mountpoint information or on how wordings should be (with or without
  "Sorry, ...", coloring of certain parts of a message, etc.)
- Adding the key of the occurred error to the API which permits reading information from
  additional metadata such as an error message provided by a specification author.
  Reason against: The description of the key should already provide such information.
  Doing it in an extra key would imply redundant information.
- Removal of warnings with error codes from the specification file.

Various projects and standards:

- [GStreamer](https://github.com/GStreamer/gstreamer):
  This project uses 4 domain type errors which are suited to their project:
  CORE, LIBRARY, RESOURCE or STREAM. Every domain type has further sub error codes which are numbered from 1-x where 1 is a
  general purpose error "FAILED" which should be used instead of inventing a new error code (additional enum). You can see an example
  of enum errors [here](https://github.com/GStreamer/gstreamer/blob/a7db80f9a98287f012108845e121f6f6fb62171b/gst/gsterror.h#L63-L80)
- [Apache httpd](https://github.com/apache/httpd):
  This project does not use any error codes at all. They solely rely on the printed message and pass various other information along like
  file, line, level, etc. The primary function they use can be seen [here](https://github.com/apache/httpd/blob/1acebd4933e5315c669605c3c9222ed8bb0ee9ea/include/http_log.h#L378-L403)
- [Jenkins](https://github.com/jenkinsci/jenkins):
  Since Jenkins is a java project they have inheritance of errors by nature. They mostly use reaction based Exception such as
  `MissingDependency`, `RestartRequired`, `FormFillValidation`, `BootFailure`, etc. Some exceptions even have more concrete
  exceptions such as a `NoTempDir` which inherits from `BootFailure`. A very similar approach will be implemented by Elektra,
  except that it is a C project and will use error codes.
- [Postgresql](https://github.com/postgres/postgres):
  Postgres has one of the most advanced error concepts among all investigated projects. It also uses one bigger [specification file](https://github.com/postgres/postgres/blob/master/src/backend/utils/errcodes.txt) which is parsed and generates multiple header files. Also noteworthy is that they once had multiple files containing error codes and
  merged them into a single one (commit [#ddfe26f](https://github.com/postgres/postgres/commit/ddfe26f6441c24660595c5efe5fd0bd3974cdc5c)). Errors are a string
  made up of 5 chars, where the first two chars indicate a certain class. This follows the SQLSTATE conventions.
  Currently they have 43 classes which all come from SQLSTATE. Postgres also throws additional errors but have to subclass it to one of the current 43 classes and have a special naming convention which have to start with a `P` in the subclass.
- [etcd](https://github.com/etcd-io/etcd):
  Etcd's approach for errors are tightly coupled to the programming language Go as well as the [gRPC](https://grpc.io/) standard which currently has
  [16 codes](https://godoc.org/google.golang.org/grpc/codes) defined. Some of these errors are similar or identical to those which will be used in elektra.
  Every error of etcd is associated with one of these categories and gets its own error message which is specified in [this](https://github.com/etcd-io/etcd/blob/master/etcdserver/api/v3rpc/rpctypes/error.go) file. This concept though does not allow easy subclassing which might be useful (eg. further split FailedPrecondition into more specific errors like semantic and syntactic errors)
- [Windows Registry](https://docs.microsoft.com/en-us/windows/desktop/sysinfo/registry):
  The registry does not use any specific error concept but takes the standard [Win32 Error Codes](https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/18d8fbe8-a967-4f1c-ae50-99ca8e491d2d). These are neither hierarchical nor have any special ordering. Basically it is the same as elektra has now except for no duplicated
  errors.
- macOS X plist:
  Just like Windows, plist uses standard macOS X errors which is a [huge catalog](http://krypted.com/lists/comprehensive-list-of-mac-os-x-error-codes/) of unordered
  return codes as integers.
- [SNMP Standard](http://www.snmp.com/protocol/):
  Being a standard network protocol, error codes are very specific to the domain itself. A list can be found [here](https://docs.microsoft.com/en-us/windows/desktop/snmp/snmp-error-codes) and would not meet the needs of elektra at all.
- POSIX:
  Returning a non-zero value and retrieving the concrete information from `errno` would not suffice for elektra as it is too simple. It would not solve any of our current
  problems like having excessive uncategorized codes for errors.

## Decision

The error message has the current format:

```
The command kdb set failed while accessing the key database with the info:
Sorry, the error (#121) occurred ;(
Description: validation failed
Reason: Validation of key "<key>" with string "<value>" failed.
Ingroup: plugin
Module: enum
At: ....../src/plugins/enum/enum.c:218
Mountpoint: <parentKey>
Configfile: ...../<file>.25676:1549919217.284067.tmp
```

The new default message will look like this:

```
Sorry, plugin <PLUGIN> issued [error|warning] code <NR>:
Validation of key "<key>" with string "<value>" failed.
```

The <NR> will be the color red in case of an error or yellow in case of a warning
while <PLUGIN> will be the color blue.

Optionally a third line indicating a solution can be added. Eg. for a permission related error there would be a third line:

```
Possible Solution: Retry the command as sudo (sudo !!)
```

To avoid losing information, the user can use the command line argument `-v` (verbose) to show
`Mountpoint`, `Configfile` in addition to the current error message.
Furthermore a developer can use the command line argument `-d` (debug)
to show `At` for debugging purposes.

All "fatal" errors will be converted to "errors" as the distinguishment is not relevant.

Unused marked errors will be removed from the specification.

Errors will be categorizes into logical groups with subgroups.
Each error will be made up of 5 characters, where the first 2 character indicate the highest level
and character 3 to 5 will be used for subgrouping.

- Permanent errors ("01000")
  - Resource ("01100")
  - Parsing ("01200")
  - Installation ("01300")
  - Logical ("01400")
- Conflict ("02000")
- Timeout ("03000")
- Validation ("04000")
  - Syntactic ("04100")
  - Semantic ("04200")

## Rationale

The new error message is much more succinct which gives end users more relevant information.
Furthermore the solution approach still holds all necessary information if requested by users.

`Ingroup`, `Description` and `Module` will be removed from the error message as they provide no useful
information at all.

The grouping of errors will allow developers to filter for specific as well as more general errors to correctly
react to them programmatically.
The new concept will permit additional subgrouping of errors in case it might be needed in the future. Imagine the case where
"Resource" errors is too general because developers saw a need for splitting the errors in "permission" and "existence" errors.
They can simply take the current subclass 01100 and make "permission" be 01110 and "existence" be 01120. This will also allow
backwards compatibility by applications just checking for resource errors in general.
Splitting/merging/rearranging any category should only be done by a decision (such as this file here) because elektra developers
should not be able to generate a new category as they wish because it would lead to the same proliferation of errors as we have now.

The classification as of now is done with the following concept in mind:

- Permanent errors
  Generally errors which require changes by a user which are not getting fixed by eg. retrying.
- Resource
  The class of errors which require certain permissions (opening a file) or where some directory/file is missing.
  One possible reaction to these kind of errors is to try a different directory/file or create a missing resource.
- Parsing
  Errors while parsing files such as ini/yaml/etc.
  Reactions could be to trim line endings/ try different encodings or even try another input at all.
- Installation
  Errors in the installation process such as missing plugins or more generic errors in beneath
  the plugins/core which cannot be fixed programmatically but needs further investigation.
- Logical
  These errors indicate a bug in beneath elektra and should never have happened.
- Conflict
  Conflicts indicate temporary problems but also could be harmful such as overwriting a file after a merge conflict.
- Timeout
  These errors indicate temporary failures such as connection timeouts or not enough space available. Retrying at a later point in time (or
  immediately) will most likely fix the problem.
- Validation
  These errors usually indicate wrong values for certain keys such as an invalid port number or wrong types. Such errors require user action and
  retry with a different value.
- Syntactic
  Syntactic failures indicate a wrong notation or missing characters for example. They require a certain form which was not given by the user.
- Semantic
  Semantic errors are failures which indicate a different expectation of the program than from the user concerning the meaning of a value.
  One example would be a wrong type or out-of-range errors.

These categories are chosen because they can help developers to react programmatically and cover the majority of use cases to our present knowledge.
If there is ever the need for another reaction based category, it can be extended very easily.

## Implications

The specification file will stay but should be untouched in most of the cases in the future. Also the C++ code generation
file which uses the specification will stay as it is easier to change categories.

Current errors will be migrated.

## Related Decisions

## Notes
