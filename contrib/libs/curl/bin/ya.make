# Generated by devtools/yamaker.

PROGRAM(curl)

LICENSE(
    BSD-3-Clause AND
    ISC AND
    Public-Domain AND
    curl
)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

WITHOUT_LICENSE_TEXTS()

PEERDIR(
    contrib/libs/c-ares
    contrib/libs/curl
)

ADDINCL(
    contrib/libs/curl/include
    contrib/libs/curl/lib
    contrib/libs/curl/src
)

NO_COMPILER_WARNINGS()

NO_RUNTIME()

CFLAGS(
    -DHAVE_CONFIG_H
    -DARCADIA_CURL_DNS_RESOLVER_ARES
)

SRCDIR(contrib/libs/curl)

SRCS(
    lib/curl_multibyte.c
    lib/dynbuf.c
    lib/nonblock.c
    lib/strtoofft.c
    lib/timediff.c
    lib/version_win32.c
    lib/warnless.c
    src/slist_wc.c
    src/tool_binmode.c
    src/tool_bname.c
    src/tool_cb_dbg.c
    src/tool_cb_hdr.c
    src/tool_cb_prg.c
    src/tool_cb_rea.c
    src/tool_cb_see.c
    src/tool_cb_wrt.c
    src/tool_cfgable.c
    src/tool_dirhie.c
    src/tool_doswin.c
    src/tool_easysrc.c
    src/tool_filetime.c
    src/tool_findfile.c
    src/tool_formparse.c
    src/tool_getparam.c
    src/tool_getpass.c
    src/tool_help.c
    src/tool_helpers.c
    src/tool_hugehelp.c
    src/tool_libinfo.c
    src/tool_listhelp.c
    src/tool_main.c
    src/tool_msgs.c
    src/tool_operate.c
    src/tool_operhlp.c
    src/tool_panykey.c
    src/tool_paramhlp.c
    src/tool_parsecfg.c
    src/tool_progress.c
    src/tool_setopt.c
    src/tool_sleep.c
    src/tool_strdup.c
    src/tool_urlglob.c
    src/tool_util.c
    src/tool_vms.c
    src/tool_writeout.c
    src/tool_writeout_json.c
    src/tool_xattr.c
)

END()
