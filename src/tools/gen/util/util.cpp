#from support.cpp import *
#compiler-settings
directiveStartToken = @
cheetahVarStartToken = $
#end compiler-settings

@@staticmethod
@def generateenum(support, parameters)
@for $key, $info in $parameters.items()
@if $support.isenum(info):
/**
 * Class enum of $key
 */
enum class $support.typeof(info)
{
@for $enum in $support.enumval(info)
	$enum,
@end for
};

/** \brief Convert enum to string
 *
 * \return string that holds value of enum
 * \param e the enum that should be converted
 */
template <>
inline void Key::set($support.enumname(info) e)
{
	switch(e)
	{
@for $enum in $support.enumval(info)
	case $support.typeof(info)::$enum: setString("$enum"); break;
@end for
	}
}

/** \brief Convert enum from string
 *
 * \return enum from string s or default value
 * \param s the string that should be converted
 */
template <>
inline $support.enumname(info) Key::get() const
{
	$support.typeof(info) ret $support.valof(info)
@for $enum in $support.enumval(info)
	if(getString() == "$enum")
		ret = $support.typeof(info)::$enum;
@end for
	return ret;
}

@end if
@end for
@end def



@@staticmethod
@def generatenone()
/**
 * @brief A none type, means that this contextual value actually does
 * not exist in the specification.
 */
class none_t
{};

template <>
inline void Key::set(kdb::none_t)
{}

template <>
inline kdb::none_t Key::get() const
{
	kdb::none_t ret;
	return ret;
}
@end def


@@staticmethod
@def generatebool(support)
/** \brief Convert bool to string
 *
 * \return string that holds value of bool
 * \param e the bool that should be converted
 */
template <>
inline void Key::set(bool e)
{
	if(e)
	{
		setString("true");
	}
	else
	{
		setString("false");
	}
}

/** \brief Convert bool from string
 *
 * \return bool from string s or default value
 * \param s the string that should be converted
 */
template <>
inline bool Key::get() const
{
	bool ret = false;
	if(getString() == "${support.trueval()[0]}")
		ret = true;
@for $b in $support.trueval()[1:]
	else if(getString() == "$b")
		ret = true;
@end for
	return ret;
}
@end def


@@staticmethod
@def generateForwardDecl(support, hierarchy)
@if not hierarchy.children
@return
@end if

@for n in hierarchy.name.split('/')[1:-1]
namespace ${support.nsnpretty($n)}
{
@end for

class ${hierarchy.prettyclassname($support)};

@for n in hierarchy.name.split('/')[1:-1]
}
@end for

@for $child in hierarchy.children
$cpp_util.generateForwardDecl(support, child)
@end for

@end def




@@staticmethod
@def generateGetBySpec(support, key, info)
@if len($support.override(info)) > 0
// override
	kdb::Key search ("${support.override(info)[0]}",
		ckdb::elektraNameOptions::KDB_O_CASCADING_NAME);
	kdb::Key found = ks.lookup(search, 0);
	if (found)
	{
		value = found.get<$support.typeof(info)>();
		$support.generateotransform($info, 0);
	}
@set $oa = $support.override(info)[1:]
@for $o in oa
	elektraKeySetName(*search, "$o",
		ckdb::elektraNameOptions::KDB_O_CASCADING_NAME);
	found = ks.lookup(search, 0);
	if (found)
	{
		value = found.get<$support.typeof(info)>();
		$support.generateotransform($info, $oa.index($o));
	}
@end for
	// now the key itself
	elektraKeySetName(*search, "$key",
		ckdb::elektraNameOptions::KDB_O_CASCADING_NAME);
	found = ks.lookup(search, 0);
@else
kdb::Key search ("$key",
		ckdb::elektraNameOptions::KDB_O_CASCADING_NAME);
	kdb::Key found = ks.lookup(search, 0);
@end if
	if(found)
	{
		return found.get<$support.typeof(info)>();
	}

@set $fa = $support.fallback(info)
@if len(fa) > 0
	// fallback
@for $f in $fa
	elektraKeySetName(*search, "$f",
		ckdb::elektraNameOptions::KDB_O_CASCADING_NAME);
	found = ks.lookup(search, 0);
	if (found)
	{
		value = found.get<$support.typeof(info)>();
		$support.generateftransform($info, $fa.index($f));
	}
@end for
@end if
@end def





@@staticmethod
@def generateSpecKey(support, hierarchy)
					kdb::Key(
@if $hierarchy.info.get('name'):
				"$hierarchy.info.get('name')",
@else
				"/",
@end if
				ckdb::KDB_O_CASCADING_NAME,
@if $hierarchy.info.get('default') != None:
				KEY_META, "default", $support.quote($hierarchy.info.get('default')),
@end if
@if $hierarchy.info.get('unit'):
				KEY_META, "unit", $support.quote($hierarchy.info.get('unit')),
@end if
@if $hierarchy.info.get('explanation'):
				KEY_META, "explanation", $support.quote($hierarchy.info.get('explanation')),
@end if
@if $hierarchy.info.get('rationale'):
				KEY_META, "rationale", $support.quote($hierarchy.info.get('rationale')),
@end if
@set $fa = $support.fallback(hierarchy.info)
@for $f in $fa
				KEY_META, "fallback/#$fa.index($f)", "$f",
@end for
@set $ov = $support.override(hierarchy.info)
@for $o in $ov
				KEY_META, "override/#$ov.index($o)", "$o",
@end for
				KEY_END)
@end def






@@staticmethod
@def outputClasses(support, hierarchy)

@for $child in hierarchy.children
$cpp_util.outputClasses(support, child)
@end for


@for n in hierarchy.name.split('/')[1:-1]
namespace $support.nsnpretty($n)
{
@end for

/** \brief class of $hierarchy.name
 *
 * Full Name (with contextual placeholders):
 * $hierarchy.info.get('name')
 * Dirname: $hierarchy.dirname
 * Basename: $hierarchy.basename
 * */
class $hierarchy.prettyclassname(support) : public ContextualValue
	<$support.typeof($hierarchy.info),
	GetPolicyIs<${hierarchy.prettyclassname(support)}GetPolicy>>
{
public:


	/** \brief Constructor for $hierarchy.prettyclassname(support)
	 * \param ks keyset to work with
	 */
	${hierarchy.prettyclassname(support)}(kdb::KeySet & ks, kdb::Context & context)
		: ContextualValue<$support.typeof($hierarchy.info),
		  GetPolicyIs<${hierarchy.prettyclassname(support)}GetPolicy>>(ks,
			context,
$cpp_util.generateSpecKey(support,$hierarchy)
			)


@for k in hierarchy.children
@set nestedname = $support.nestedpretty(k.basename)
		, ${nestedname}(ks, context)
@end for
	{}

	using ContextualValue<$support.typeof($hierarchy.info), GetPolicyIs<${hierarchy.prettyclassname(support)}GetPolicy>>::operator =;

@for k in hierarchy.children
@set nsname = $support.nspretty(k.dirname)
@set nestedname = $support.nestedpretty(k.basename)
@set nestedclassname = $support.classpretty(k.basename)
	/** \return nested subclass */
	$nsname$nestedclassname ${nestedname};
@end for
};

@if $support.typeof($hierarchy.info)=='std::string':
inline std::ostream & operator<<(std::ostream & os,
		$hierarchy.prettyclassname(support) const & c)
{
	os << static_cast<std::string>(c);
	return os;
}
@end if

@for n in hierarchy.name.split('/')[1:-1]
}
@end for

@end def



