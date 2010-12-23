#!/usr/bin/awk
#
# override function calls generator
# Copyright (C) 2010 Alexander Shishkin
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
# This script generates symbols.{c,h} from symbols.list

# template for struct override instance
# ---
# static struct override __lrc_call_XXX = {
#	.name = "XXX",
# ---
function mk_override_st_hdr(__fn_name)
{
    return sprintf("static struct override __lrc_call_%s = {\n"	\
		   "\t.name  = \"%s\",\n", __fn_name, __fn_name)
}

# template for arguments struct definition
# ---
# static struct override __lrc_callctx_XXX = {
#	struct lrcpriv_callctx callctx;
# ---
function mk_args_st_hdr(__fn_name)
{
    return sprintf("struct __lrc_callctx_%s {\n\tstruct lrcpriv_callctx callctx;\n", __fn_name)
}

function get_type_name(str)
{
    type_name = ""

    for (i = 1; i < NF; i++)
	type_name = type_name ? type_name " " $i : $i

    return type_name
}

# generate function body
# ---
# struct __lrc_callctx_XXX args =
#          { NULL, AAA };
# int __ret;
#
# if (!lrc_call_entry(&__lrc_call_XXX, &args))
#         __ret = ((__lrc_XXX_fn)__lrc_call_XXX.orig_func)(args.AAA);
# lrc_call_exit(&__lrc_call_XXX, &args, &__ret);
# return __ret;
# ---
function mk_fn_body(__fn_name, __fn_typename, __fn_addargs, __fn_paramlist, \
		    __prologue, __paramlist_args, __ret0, __ret1, __ret2, __ret3)
{
    if (__fn_typename == "void") {
	__ret2 = "NULL"
    } else {
	__ret0 = sprintf("\t%s __ret;\n\n", __fn_typename)
	__ret1 = "__ret = "
	__ret2 = "&__ret"
	__ret3 = "\n\treturn __ret;"
    }

    if (__fn_addargs)
	__fn_addargs = "\t" __fn_addargs "\n"

    # comma-separated list of arguments, prefixed with "args." each
    __paramlist_args = gensub("([^, ]+)", "args.\\1", "g", __fn_paramlist)

    return sprintf(							\
	"%s"								\
	"\tstruct __lrc_callctx_%s args =\n\t\t{ {}, %s };\n"		\
	"%s\n%s"							\
	"\tif (!lrc_call_entry(&__lrc_call_%s, &args))\n"		\
	"\t\t%s((__lrc_%s_fn)__lrc_call_%s.orig_func)(%s);\n"		\
	"\tlrc_call_exit(&__lrc_call_%s, &args, %s);%s",		\
	__fn_addargs, __fn_name, __fn_paramlist, __ret0, __prologue,	\
	__fn_name,							\
	__ret1, __fn_name, __fn_name, __paramlist_args, __fn_name,	\
	__ret2, __ret3)
}

# output everything to do with a call:
#  - struct override instance
#  - callctx struct definition (to symbols.h)
#  - original function typedef
#  - override function itself
# and clear the global variables
function flush_function()
{
    if (!fn_def)
	return

    fn_struct = fn_struct sprintf("\t.nargs = %d,\n};\n", fn_nargs)
    lrc_list = lrc_list ?
	lrc_list sprintf("\t&__lrc_call_%s,\n", fn_name) :
	sprintf("\t&__lrc_call_%s,\n", fn_name)

    print fn_struct

    args_struct = args_struct "};\n"
    print args_struct >"symbols.h"

    printf("typedef %s (*__lrc_%s_fn)(%s);\n", fn_typename, fn_name, fn_paramlist)

    fn_body = mk_fn_body(fn_name, fn_typename, fn_addargs, fn_paramlist_call, fn_prologue)

    # use void for empty parameters list
    fn_paramlist = fn_paramlist ? fn_paramlist : "void"

    if (fn_tail)
	fn_tail = fn_tail " "

    # the override function
    printf "EXPORT %s%s(%s)\n{\n%s\n}\n\n", fn_tail, fn_def, fn_paramlist, fn_body

    # the original function
    printf "%s%s __lrc_orig_%s(%s)\n{\n%s\treturn ((__lrc_%s_fn)__lrc_call_%s.orig_func)(%s);\n}\n\n", \
	fn_tail, fn_typename, fn_name, fn_paramlist, fn_addargs, fn_name, fn_name, fn_paramlist_call
    printf "%s%s __lrc_orig_%s(%s);\n\n", \
	fn_tail, fn_typename, fn_name, fn_paramlist >"symbols.h"

    # __lrc_orig_*_is_callable() will return true if the corresponding
    # original libc function has been loaded
    printf "bool __lrc_orig_%s_is_callable(void)\n{\n\treturn !!__lrc_call_%s.orig_func;\n}\n\n", \
	fn_name, fn_name

    printf "bool __lrc_orig_%s_is_callable(void);\n\n", \
	fn_name >"symbols.h"

    # the dummy function for testcases
    printf "%s%s __lrc_orig_%s(%s) {}\n", fn_tail, fn_typename, fn_name, fn_paramlist >"dummies.c"

    printf "bool __lrc_orig_%s_is_callable(void)\n{\n\treturn false;\n}\n", \
	fn_name >"dummies.c"

    fn_paramlist = ""
    fn_paramlist_call = ""
    fn_nargs = 0
    fn_tail = ""
    fn_addargs = ""
    fn_prologue = ""
}

BEGIN {
    fn_nargs = 0

    print "#include \"override.h\""
    print "#include \"symbols.h\""
}

END {
    flush_function()
    printf "struct override *lrc_overrides[] = {\n%s\tNULL\n};\n\n", lrc_list
}

# allow for custom additions to the output
/^--/ && $2 == "include" {
	print "#include " $3
	print "#include " $3 >"symbols.h"
	next
}

/^--/ {
    fn_prologue = fn_prologue ? fn_prologue "\t" : "\t"
    for (i = 2; i <= NF; i++)
	fn_prologue = i == 2 ? fn_prologue $i : fn_prologue " " $i
    fn_prologue = fn_prologue ";\n"
    next
}

# skip comments
/#/ || /^ *$/ { next }

# additional modifiers to the function definition (nothrow(), etc)
/^@@/ {
    for (i = 2; i <= NF; i++)
	fn_tail = fn_tail ? fn_tail " " $i : $i

    next
}

# process a function argument
/^\t/ {
    type_name = get_type_name($0)
    arg_name = $NF

    fn_nargs++

    this_param = sprintf("%s %s", type_name, arg_name)

    if ($NF == "...") {
	# va_list will be the last argument, don't append it
	# to the array, but insert a va_copy() instead
	type_name = "va_list"
	arg_name = "ap"
	fn_addargs = type_name " " arg_name ";"
	tmp = "\tva_copy(args.ap, ap);\n"
	fn_prologue = fn_prologue ? fn_prologue "\n" tmp : tmp
    } else
	fn_paramlist_call = fn_paramlist_call ?
	    fn_paramlist_call ", " arg_name : arg_name

    fn_paramlist = fn_paramlist ? fn_paramlist ", " this_param : this_param

    args_struct = args_struct sprintf("\t%s\t%s;\n", type_name, arg_name)

    next
}

# new function definition
{
    flush_function()
    fn_name = $NF
    fn_typename = get_type_name($0)

    fn_def = sprintf("%s %s", fn_typename, fn_name)

    fn_struct = mk_override_st_hdr(fn_name)

    args_struct = mk_args_st_hdr(fn_name)
}
