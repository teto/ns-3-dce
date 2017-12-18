#!/usr/bin/env python3
# Example taken out of http://pygccxml.readthedocs.io/en/develop/examples/searching1/example.html
import pygccxml
from pygccxml import utils
from pygccxml import declarations
from pygccxml import parser
from pygccxml.declarations import declaration_utils
from collections import namedtuple
from typing import List, Dict, Tuple
import os
import argparse
import csv
import subprocess
import logging
import sys


"""
Must be able to generate:
    -

todo do same for dl/pthread/rt
"""

log = logging.getLogger("dce")
log.setLevel(logging.DEBUG)
log.addHandler(logging.StreamHandler())

API_COVERAGE_CSV = "doc/source/api_cov.csv"
include_paths = [
                "/nix/store/c30dlkmiyrjxxjv6nv63igjkzcj1fzxi-gcc-6.4.0/lib/gcc/x86_64-unknown-linux-gnu/6.4.0/include",
                "/nix/store/mqalq0v2laqblw00dp7pwwckj2ra6jyh-glibc-2.26-75-dev/include",
]
# Parse the c++ file
# decls = parser.parse([filename], xml_generator_config)

HeadersType = Dict[str, List]


# int_type = declarations.cpptypes.int_t()
# double_type = declarations.cpptypes.double_t()


# int dce___cxa_atexit (void (*func)(void *), void *arg, void *d);
# void dce___cxa_finalize (void *d);
# __newlocale

# list of exceptions for functions thatp ygccxml fail to identify correctly
# hack around https://github.com/gccxml/pygccxml/issues/62
# specifier = noexcept
# location should be just the visible location, i.e. akin to a pattern
ExplicitFn = namedtuple('ExplicitFn', ["rtype", "fullargs", "arg_names", "location", "specifier"])
exceptions = {
    # "sysinfo": ExplicitFn("int", ["struct sysinfo *info"], ["info"],"/usr/include/x86_64-linux-gnu/sys/sysinfo.h", "noexcept"),
    # "sigaction": ExplicitFn("int", ["int signum", "const struct sigaction *act", "struct sigaction *oldact"], "signum, act, oldact","/usr/include/signal.h", "noexcept"),
    "wait": ExplicitFn("pid_t", "int *stat_loc", "stat_loc", "sys/wait.h", ""),
    "__fpurge": ExplicitFn("void", "FILE *fd", "fd", "stdio.h", ""),
    "__fpending": ExplicitFn("size_t", "FILE *fd", "fd", "/usr/include/stdio.h", ""),
    "__cxa_atexit": ExplicitFn("int", "void (*func)(void *), void *arg, void *d", "func, arg, d", "/usr/include/stdlib.h", ""),
    "__cxa_finalize": ExplicitFn("void", "void *d", "d", "/usr/include/stdlib.h", ""),
    "fstat64": ExplicitFn("int", ["int __fd", "struct stat64 *__buf"], ["__fd", "__buf"], "/usr/include/x86_64-linux-gnu/sys/stat.h", "noexcept"),
    "pthread_kill": ExplicitFn("int", "pthread_t thread, int sig", ["thread", "sig"], "/usr/include/signal.h", "noexcept"),
    "uname": ExplicitFn("int", "struct utsname *__name", "__name", "/usr/include/x86_64-linux-gnu/sys/utsname.h", "noexcept"),
    "statvfs": ExplicitFn("int", "const char *path, struct statvfs *buf", "path, buf", "/usr/include/x86_64-linux-gnu/sys/statvfs.h", "noexcept"),
    "statfs": ExplicitFn("int", "const char *path, struct statfs *buf", "path, buf", "/usr/include/x86_64-linux-gnu/sys/vfs.h", "noexcept"),
    "statfs64": ExplicitFn("int", "const char *path, struct statfs64 *buf", "path, buf", "/usr/include/x86_64-linux-gnu/sys/vfs.h", "noexcept"),
    # TODO the attributes should not be in definition for GCC (except is part of the type)
    "abort": ExplicitFn("void", "void", "", "/usr/include/stdlib.h", "noexcept __attribute__ ((__noreturn__))"),
    "exit": ExplicitFn("void", ["int status"], ["status"], "/usr/include/stdlib.h", "noexcept __attribute__ ((__noreturn__))"),
    "pthread_exit": ExplicitFn("void", "void *retval", "retval", "/usr/include/pthread.h", "__attribute__ ((__noreturn__))"),
    "fstatfs": ExplicitFn("int", "int __fildes, struct statfs * __buf", "__fildes, __buf", "/usr/include/x86_64-linux-gnu/sys/vfs.h", "noexcept"),
    "fstatvfs": ExplicitFn("int", "int __fildes, struct statvfs * __buf", "__fildes, __buf", "/usr/include/x86_64-linux-gnu/sys/statvfs.h", "noexcept"),
    "fstatfs64": ExplicitFn("int", "int __fildes, struct statfs64 * __buf", "__fildes, __buf", "/usr/include/x86_64-linux-gnu/sys/vfs.h", "noexcept"),
# int dce_fstatfs (int __fildes, struct statfs * __buf) noexcept;
    "__stack_chk_fail": ExplicitFn("void", "void", "", "/usr/include/x86_64-linux-gnu/sys/vfs.h", "noexcept"),
    }



def gen_declaration(rtype, symbol_name: str, decl_args, specifier, append_column: bool=True):
    """
    TODO tells the types of the variables
    """

    if isinstance(decl_args, list):
        decl_args = ','.join(decl_args)

    tpl = "{extern} {ret} {name} ({fullargs}) {throw} {finalizer}"
    content = tpl.format(
        extern="",
        ret=rtype,
        fullargs=decl_args,
        name=symbol_name,
        throw=specifier,
        finalizer=";\n" if append_column else ""
    )
    return content


def gen_variadic_wrapper(rtype, wrapper_symbol, wrapped_symbol, decl_args: List[str],
        arg_names: List[str], specifier):
    """
    decl_args/arg_names as list
    if rtype is None => void ?
    """

    content = gen_declaration(rtype, wrapper_symbol, decl_args, specifier, append_column=False)
    # tpl = """{extern} {ret} {wrapper_symbol} ({fullargs}) {throw} {{
    content += """ {{
        va_list __dce_va_list;
        va_start (__dce_va_list, {justbeforelastarg});
        {retstmt1} {wrapped_symbol} ( {arg_names}, __dce_va_list);
        va_end (__dce_va_list);
        {retstmt2}
            }};\n
    """.format(
        justbeforelastarg=arg_names[-2],
        wrapped_symbol=wrapped_symbol,
        fullargs=decl_args,
        retstmt1= "auto ret =" if rtype is not "void" else "",
        retstmt2= "return ret;" if rtype is not "void" else "",
        arg_names= ",".join([arg for arg in arg_names[:-1] ]) ,
    )
    return content


class Generator:
    def __init__(self):
        pass

    def parse(self, filename, regen_cache: bool):
        """
        cache parsing
        """
        utils.loggers.set_level(logging.DEBUG)

        # Find the location of the xml generator (castxml or gccxml)
        generator_path, generator_name = utils.find_xml_generator()

        print("generator path=%s" %generator_path)

        # TODO USE DCE_CFLAGS or pass them at launch
        cflags = os.getenv("CFLAGS", "")
        # Configure the xml generator
        # doc at
        # http://pygccxml.readthedocs.io/en/develop/apidocs/pygccxml.parser.config.html?highlight=xml_generator_configuration_t

        xml_generator_config = parser.xml_generator_configuration_t(
            xml_generator_path=generator_path,
            xml_generator=generator_name,
            include_paths=include_paths,
            # maybe it misses the
            # _GNU_SOURCE
            define_symbols=["_GNU_SOURCE=1"],
            # la ca va foirer
            # get from env
            # -nostdinc -I/usr/include
            # -std=c99
            # invalid argument '-std=c99' not allowed with 'C++'
            #  -std=c++11
            cflags="" + cflags,


            castxml_epic_version=1,
            # asked on tracker to generate va_list but not ok
            # flags= ["f1"]
            )
        # config.flags =
        # The c++ file we want to parse
        # printf is declared in stdio.h
        # filename = "/home/teto/glibc/libio/stdio.h"
        filename = "test.h"

        file_config = parser.file_configuration_t(
            data=filename,
            # content_type=parser.CONTENT_TYPE.CACHED_SOURCE_FILE
            )

        self.project_reader = parser.project_reader_t(xml_generator_config)
        self.decls = self.project_reader.read_files(
            [file_config],
            compilation_mode=parser.COMPILATION_MODE.FILE_BY_FILE)



    def generate_alias(self, aliasname, decl):
                   #define weak_alias(name, aliasname) \
        # extern __typeof (name) aliasname __attribute__ ((weak, alias (# name)));
        return ""


    def lookup(self, toto: str) -> pygccxml.declarations.declaration_t:
        """
        returns pygccxml result only
        """

        log.info("Looking for %s" % toto)

        global_namespace = declarations.get_global_namespace(self.decls)
        criteria = declarations.calldef_matcher(name=toto, decl_type=declarations.free_function_t)
        results = declarations.matcher.find(criteria, global_namespace)
        print("resultats=", results)
        for res in results:
            print("resultat:", res)
        return results[0] if len(results) else None

    def generate_decl_string(self, name: str, dce: bool=False) -> Tuple[str, str]:
        """
        name: libc function name
        dce: true if function is overriden by DCE

        Returns:
            location, generated string
        """

        log.debug("Generating decl string for name,")
        has_ellipsis = False
        # look for a match
        # Search for the function by name

        # Some libc functions need to be manually crafted
        # hack around https://github.com/gccxml/pygccxml/issues/62
        if name in exceptions.keys():
            log.debug("Exception [%s] found " % name)
            extern = ""
            rtype, libc_fullargs, arg_names, location, specifier = exceptions[name]
            print("Values:", rtype, libc_fullargs, arg_names, location)
        else:

            decl = self.lookup(name)
            if not decl:
                return None


            # print("decl", results)
            # decl = results[0]
            # print( "islist ? len",len(func1))
            qname = declaration_utils.full_name(decl)
            if qname[:2] == "::":
                qname = qname[2:]
            log.info("Parsing function %s" % name)
            # Add the arguments...
            # default_value', 'ellipsis', 'name', 'type'
            # print(dir(decl.arguments[0]))
            # TODO we should record the location
            # locations.update ({ decl.location })
            # {ret} {name} ({args})
            # proto = "{extern} {ret} {name} ({args})".format(

            extern = "extern" if decl.has_extern else ""
            rtype = "%s" % (decl.return_type if decl.return_type is not None else "void")

            # for a in decl.arguments:
            #     print(dir(a ))
            #     print("a", a )
            # print("decl=", decl.create_decl_type )
            # print("decl=", decl.calling_convention )
            # print("=", decl.does_throw )
            if decl.has_ellipsis:
                print("HAS ELLIPSIS")

            # some hacks to workaround pygccxml/castxml flaws:
            # va_list is not_recognized and function pointer badly displayed

            # temp is a list of types
            decl_args = []
            for arg in decl.arguments:
                s = str(arg.decl_type)
                print("decl_type", type(arg.decl_type))
                print("decl_string", arg.decl_type.decl_string)
                if isinstance(arg.decl_type, declarations.cpptypes.pointer_t):
                    print("ISINSTANCE")
                    log.debug("this is a pointer")
                    new_type = declarations.remove_pointer(arg.decl_type)
                    print("type after removal ", type(new_type))
                    print("type after removal ", new_type)
                if s.startswith("?unknown?"):
                    # handles variables list
                    log.debug("UNKNOWN => variadic")
                    s = "va_list " + decl.arguments[-1].name
                elif "(" in s:
                    # check if it's a function_pointer
                    log.debug("looks like function pointer %s" % s)
                    s= s.rstrip("*")
                else:
                    s += " " + arg.name
                    print("saved as s=%s" % s)
                decl_args.append(s)

            for arg in decl_args:
                print("arg=%s"% arg)

            libc_fullargs = ",".join(decl_args) # only types
            location = decl.location.file_name
            arg_names = [arg.name for arg in decl.arguments]
            specifier = "" if decl.does_throw else "noexcept"
            has_ellipsis = decl.has_ellipsis
            print("found in %s" % location)


        # if "..." in libc_fullargs:
        if has_ellipsis:
            # DCE overrides that accept a va_list are suffixed with "_v" while
            # libc functions are prefix with "v"
            # for instance "vprintf" is wrapped via dce_printf_v
            wrapped_symbol = "dce_" + name + "_v" if dce else "v" + name
            content = gen_variadic_wrapper(rtype, name,
                    wrapped_symbol,
                    decl_args,
                    arg_names,
                    specifier
            )
        else:
            content = gen_declaration(rtype, name, libc_fullargs,
                    specifier, append_column=False)
            content += """{{
                        {retfinalstmt} {instruction} ({arg_names});
                        }}
                        """.format(
                        name=name,
                        instruction="dce_" + name if "dce" else "g_libc.%s_fn" % name,
                        retfinalstmt="return" if rtype is not "void" else "",
                        arg_names=",".join(arg_names) if isinstance(arg_names, list) else arg_names,
                    )
        return location, content


    def generate_wrappers(self,
        input_filename,
        libc_filename,
        write_headers : bool,
        write_impl : bool
    ):
        """
        Generate wrappers + headers aka "dce-stdio.h"
        """
        if not write_impl:
            libc_filename = os.devnull

        # input_filename = "natives.h.txt"
        # str, array
        locations = {} # type: HeadersType
        content = ""
        log.debug("Opening %s" % input_filename)
        with open(input_filename, "r") as src:
            # aliasnames = last columns ?
            reader = csv.DictReader(src, fieldnames=["type","name"], restkey="extra")
            with open(libc_filename, "w+") as libc_fd:

                # for line in src:
                for row in reader:

                    content = ""
                    # function_name = line.rstrip()
                    print('row["name"]=', row["name"], "extra=", row["extra"])

                    # self.generate_decl_string("")

                    # generate aliases for both natives and dce
                    for aliasname in row["extra"]:
                        print("alias=", aliasname)
                        if len(aliasname):
                            # this is ok if at the end
                            tpl = "decltype ({name}) {aliasname} __attribute__ ((weak, alias (\"{name}\")));\n"
                            # the pragam requires the alias to be previously declared in clang ?!
                            # tpl = "#pragma weak {aliasname} = {name}"
                            # tpl = "extern __typeof ({name}) {aliasname} __attribute__ ((weak, alias (\"{name}\")));\n"
                            content += tpl.format(
                                    aliasname=aliasname,
                                    name=row["name"]
                                    )

                            # extern __typeof (name) aliasname __attribute__ ((weak, alias (# name)));

                            log.debug(content)
                            libc_fd.write(content)

                    # TODO
                    location, decl_str = self.generate_decl_string(row["name"], row["type"] == "dce")
                    content += decl_str

                    # now we generate dce-<FILE>.h content
                    #
                    # generate only the dce overrides
                    #if row["type"] == "dce":
                    #    # declaration of dce_{libcfunc}
                    #    # TODO
                    #    if has_ellipsis:
                    #        # then we need to declare the variant accepting va_list
                    #        content = gen_declaration(rtype, "dce_"+ name + "_v",
                    #                decl_args[:-1] + ["va_list"],
                    #                specifier,
                    #            )
                    #        # implement an inline variadic function
                    #        # and a variant accepting va_list
                    #        content += "inline "+ gen_variadic_wrapper(rtype, "dce_"+name,
                    #                "dce_"+name +"_v",
                    #                decl_args,
                    #                arg_names,
                    #                specifier
                    #                )
                    #    else:
                    #        content = gen_declaration(rtype, "dce_"+ name,
                    #            libc_fullargs,
                    #            specifier,
                    #            )

                    items = locations.setdefault(location, [])
                    items.append(content)

            self.generate_headers(locations, write_headers)


    def generate_headers(self, locations : HeadersType, write_headers: bool=False):
        """
        Write all the dce-*.h files
        """

        # Now we generate the header files
        # TODO move to another function "extract_include_path"
        for path, functions in locations.items():
            print("path=", path)
            (head, tail) = os.path.split(path)
            print("head/tail", head, "----" , tail)
            # filename = os.path.basename()
            # filename = "model"
            header = ""

            subfolder = ""
            for folder in ["sys", "net", "arpa"]:
                if head.endswith(folder):
                    subfolder = folder
                    break

                # tail = os.path.join("sys", tail)
            filename = os.path.join("model", subfolder, "dce-" + tail)
            header=os.path.join(subfolder, tail)
            print(filename)
            # TODO # + ".generated.h"
            # header = "model/dce-" + filename
            print("Header name=", filename)

            # TODO strip
            content = """
                /* DO NOT MODIFY - GENERATED BY script */
                #ifndef DCE_HEADER_{guard}
                #define DCE_HEADER_{guard}
                // TODO add extern "C" ?
                #include <{header}>
                #include <stdarg.h> // just in case there is an ellipsis
                // TODO temporary hack
                #define __restrict__

                #ifdef __cplusplus
                extern "C" {{
                #endif
            """.format(guard=header.upper().replace(".","_").replace("/","_"), header=header) #os.path.basename(tail))

            for proto in functions:
                print(proto)
                content += proto + "\n"

            content += """
                #ifdef __cplusplus
                }
                #endif
                #endif
                """
            if write_headers:
                with open(filename, 'w+') as dst:

                    # print("content=", content)
                    dst.write(content)


def main():

    libc_filename = "model/libc.generated.cc"

    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--write-headers', action="store_true",
        default=False, help="Write model/dce-* to files")
    parser.add_argument('-i', '--write-impl', action="store_true", default=False,
        help="write %s" % libc_filename)
    parser.add_argument('-a', '--write-all', action="store_true", default=False,
        help="Enables -i and -h")
    parser.add_argument('-R', '--regen', action="store_true", default=False,
            help="TODO: Disable the cache")
    parser.add_argument('-d', '--write-doc', action="store_true", default=False,
            help="Write %s.csv" % API_COVERAGE_CSV)

    args, unknown = parser.parse_known_args()

    # TODO call that with subprocess.
    # os.system("./gen_natives.sh")
    # redirect output
    output = "model/libc-ns3.h.tmp"
    list_declarations = "model/libc-ns3.h"

    # Preprocess libc-ns3.h
    with open(output, "w") as tmp:
        subprocess.call([
            "gcc", list_declarations, "-E", "-P", "-DNATIVE(name,...)=native,name,__VA_ARGS__",
            "-DDCE(name,...)=dce,name,__VA_ARGS__",
            ], stdout=tmp, stderr=sys.stdout)

    g = Generator()
    g.parse("test.h", args.regen)
    print(unknown)
    if len(unknown) > 0:
        for func in unknown:
            res = g.generate_decl_string(func)
            if not res:
                print("Could not find %s" % func)
            else:
                print(res)
        exit(0)

    # libc-ns3.generated.tmp
    g.generate_wrappers(output, libc_filename, write_headers=args.write_headers or args.write_all,
            write_impl=args.write_impl or args.write_all)
    # if args.write_headers or args.write_all:
    # if args.write_impl or args.write_all:

if __name__ == "__main__":
    main()
