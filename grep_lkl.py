#!/usr/bin/env bash
# todo do it in haskell ?
# might need to export TMPDIR before to work around a ctags bug
# ctags -x --c-kinds=fp tools/lkl/include/lkl.h
# int lkl_add_gateway(int af, void *gwaddr);
# int lkl_add_neighbor(int ifindex, int af, void* addr, void* mac);
ctags -x --c-kinds=fp tools/lkl/include/lkl.h|tr -s '[:blank:]'|cut -d ' ' -f 5-
# https://regex101.com/r/AXEXtm/1

perl -pe "s/lkl_(?'name'\w*)\((?'args'.*)\)/toto_\1 and \2/"

# s/lkl_\(\w\)^(/toto_\1\(/
# |xargs toto.sh

