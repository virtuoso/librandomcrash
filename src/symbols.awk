BEGIN {
    sec_attr = "__attribute__((section(\"InitLate\")))"
    fn_nargs = 0

    print "#include \"override.h\""
    print "#include \"symbols.h\""
}

END {
    flush_function()
    printf "struct override *lrc_overrides[] = {\n%s\tNULL\n};\n\n", lrc_list
}

/^--/ && $2 == "include" {
	print "#include " $3
	print "#include " $3 >"symbols.h"
	next
}

/#/ || /^ *$/ { next }

/^@@/ {
    for (i = 2; i <= NF; i++)
	fn_tail = fn_tail ? fn_tail " " $i : $i

    next
}

function get_type_name(str)
{
    type_name = ""

    for (i = 1; i < NF; i++)
	type_name = type_name ? type_name " " $i : $i

    return type_name
}

/^\t/ {
    fn_nargs++
    type_name = get_type_name($0)

    this_param = sprintf("%s %s", type_name, $NF)
    fn_paramlist = fn_paramlist ? fn_paramlist ", " this_param : this_param
    fn_paramlist_call = fn_paramlist_call ?
	fn_paramlist_call ", " $NF : $NF

    args_struct = args_struct sprintf("\t%s\t%s;\n", type_name, $NF)

    next
}

function flush_function()
{
    if (!fn_def)
	return

    fn_struct = fn_struct sprintf("\t.nargs = %d,\n};\n", fn_nargs)
    lrc_list = lrc_list ?
	lrc_list sprintf("\t&__lrc_call_%s,\n", fn_name) :
	sprintf("\t&__lrc_call_%s,\n", fn_name)

    print fn_struct

    args_struct = args_struct "\tvoid *priv;\n};\n"
    print args_struct >"symbols.h"

    printf("typedef %s (*__lrc_%s_fn)(%s);\n", fn_typename, fn_name, fn_paramlist)
    if (fn_typename != "void")
	fn_body = sprintf( \
	    "\tstruct __lrc_callctx_%s args =\n\t\t{ %s, NULL };\n" \
	    "\t%s __ret;\n\n" \
	    "\tif (!__lrc_call_entry(&__lrc_call_%s, &args))\n" \
	    "\t\t__ret = ((__lrc_%s_fn)__lrc_call_%s.orig_func)(%s);\n" \
	    "\t__lrc_call_exit(&__lrc_call_%s, &args, &__ret);\n" \
	    "\treturn __ret;", fn_name, fn_paramlist_call, \
	    fn_typename, fn_name, fn_name, fn_name, \
	    gensub("([^, ]+)", "args.\\1", "g", fn_paramlist_call), fn_name)
    else
	fn_body = sprintf( \
	    "\tstruct __lrc_callctx_%s args =\n\t\t{ %s, NULL };\n" \
	    "\tif (!__lrc_call_entry(&__lrc_call_%s, &args))\n" \
	    "\t\t((__lrc_%s_fn)__lrc_call_%s.orig_func)(%s);\n" \
	    "\t__lrc_call_exit(&__lrc_call_%s, &args, NULL);\n", \
	    fn_name, fn_paramlist_call, fn_name, fn_name, fn_name, \
	    gensub("([^, ]+)", "args.\\1", "g", fn_paramlist_call), fn_name)

    fn_paramlist = fn_paramlist ? fn_paramlist : "void"
    printf "%s %s(%s) {\n%s\n}\n\n", fn_tail, fn_def, fn_paramlist, fn_body

    fn_paramlist = ""
    fn_paramlist_call = ""
    fn_nargs = 0
    fn_tail = ""
}

{
    flush_function()
    fn_name = $NF
    fn_typename = get_type_name($0)

    fn_def = sprintf("%s %s", fn_typename, fn_name)

    fn_struct = sprintf("static struct override __lrc_call_%s %s = {\n" \
			"\t.name  = \"%s\",\n", fn_name, sec_attr, fn_name)

    args_struct = sprintf("struct __lrc_callctx_%s {\n", fn_name)
}
